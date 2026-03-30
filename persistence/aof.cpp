#include "aof.h"
#include "../protocol/resp.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>

using namespace std;

////////////////////////////////////////////////////////////
// SAFE WRITE
////////////////////////////////////////////////////////////
static bool safe_write(int fd, const string &data)
{
    size_t total = 0;
    while (total < data.size())
    {
        ssize_t n = write(fd, data.c_str() + total, data.size() - total);
        if (n <= 0)
        {
            if (errno == EINTR) continue;
            return false;
        }
        total += n;
    }
    return true;
}

////////////////////////////////////////////////////////////
// CONSTRUCTOR / DESTRUCTOR
////////////////////////////////////////////////////////////
AOF::AOF()
{
    fd = open("appendonly.aof", O_CREAT | O_APPEND | O_WRONLY, 0644);
    if (fd < 0)
    {
        perror("AOF open failed");
        exit(1);
    }

    is_rewriting = false;
}

AOF::~AOF()
{
    if (fd >= 0)
        close(fd);
}

////////////////////////////////////////////////////////////
// LOAD AOF
////////////////////////////////////////////////////////////
void AOF::load(Store &store)
{
    int file = open("appendonly.aof", O_RDONLY);
    if (file < 0) return;

    string data;
    char buf[1024];

    while (true)
    {
        int n = read(file, buf, sizeof(buf));
        if (n <= 0) break;
        data.append(buf, n);
    }

    close(file);

    while (true)
    {
        vector<string> cmd;
        int consumed = parse_one_resp(data, cmd);

        if (consumed == 0) break;

        data.erase(0, consumed);

        if (cmd[0] == "SET" && cmd.size() >= 3)
            store.set(cmd[1], cmd[2]);
        else if (cmd[0] == "DEL" && cmd.size() >= 2)
            store.del(cmd[1]);
    }
}

////////////////////////////////////////////////////////////
// APPEND
////////////////////////////////////////////////////////////
bool AOF::append(const vector<string> &cmd)
{
    string resp = build_resp(cmd);

    if (!safe_write(fd, resp))
        return false;

    if (is_rewriting)
        rewrite_buffer += resp;

    return true;
}

////////////////////////////////////////////////////////////
// REWRITE (FIXED VERSION)
////////////////////////////////////////////////////////////
void AOF::rewrite(Store &store)
{
    is_rewriting = true;

    int new_fd = open("temp.aof", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (new_fd < 0)
    {
        perror("rewrite open failed");
        is_rewriting = false;
        return;
    }

    ////////////////////////////////////////////////////////////
    // Phase 1: snapshot
    ////////////////////////////////////////////////////////////
    for (const auto &kv : store.snapshot())
    {
        vector<string> cmd = {"SET", kv.first, kv.second};
        string resp = build_resp(cmd);
        safe_write(new_fd, resp);
    }

    ////////////////////////////////////////////////////////////
    // Phase 2: swap buffer (CRITICAL FIX)
    ////////////////////////////////////////////////////////////
    string buffer_copy;
    buffer_copy.swap(rewrite_buffer);

    safe_write(new_fd, buffer_copy);

    ////////////////////////////////////////////////////////////
    // Phase 3: append remaining writes
    ////////////////////////////////////////////////////////////
    safe_write(new_fd, rewrite_buffer);

    ////////////////////////////////////////////////////////////
    // Finish
    ////////////////////////////////////////////////////////////
    close(new_fd);

    rename("temp.aof", "appendonly.aof");

    close(fd);
    fd = open("appendonly.aof", O_CREAT | O_APPEND | O_WRONLY, 0644);

    rewrite_buffer.clear();
    is_rewriting = false;

    cout << "AOF rewrite complete\n";
}