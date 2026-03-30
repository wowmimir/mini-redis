#include "server.h"

#include "../command/command.h"
#include "../protocol/resp.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
    constexpr int kPort = 8000;
    constexpr int kBacklog = 128;
    constexpr int kBufferSize = 4096;

    bool set_non_blocking(int fd)
    {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1)
            return false;

        return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
    }

    void log_errno(const char *msg)
    {
        std::cerr << msg << ": " << std::strerror(errno) << "\n";
    }
}

////////////////////////////////////////////////////////////
// CONSTRUCTOR
////////////////////////////////////////////////////////////
Server::Server() : server_fd(-1), epfd(-1)
{
    aof.load(store);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
        throw std::runtime_error("socket() failed");

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("setsockopt() failed");

    if (!set_non_blocking(server_fd))
        throw std::runtime_error("failed to set non-blocking");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(kPort);

    if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed");

    if (listen(server_fd, kBacklog) < 0)
        throw std::runtime_error("listen() failed");

    ////////////////////////////////////////////////////////////
    // EPOLL SETUP
    ////////////////////////////////////////////////////////////
    epfd = epoll_create1(0);
    if (epfd < 0)
        throw std::runtime_error("epoll_create1 failed");

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev) < 0)
        throw std::runtime_error("epoll_ctl ADD failed");
}

////////////////////////////////////////////////////////////
// DESTRUCTOR
////////////////////////////////////////////////////////////
Server::~Server()
{
    for (auto &[fd, _] : buffers)
        close(fd);

    if (server_fd >= 0)
        close(server_fd);

    if (epfd >= 0)
        close(epfd); // 🔥 important
}

////////////////////////////////////////////////////////////
// MAIN LOOP
////////////////////////////////////////////////////////////
void Server::run()
{
    std::cout << "Server listening on port " << kPort << "\n";

    epoll_event events[64];

    while (true)
    {
        int n = epoll_wait(epfd, events, 64, -1);

        if (n < 0)
        {
            if (errno == EINTR)
                continue;

            log_errno("epoll_wait failed");
            continue;
        }

        for (int i = 0; i < n; i++)
        {
            int fd = events[i].data.fd;

            if (events[i].events & (EPOLLERR | EPOLLHUP))
            {
                close_client(fd);
                continue;
            }

            if (fd == server_fd)
            {
                handle_accept();
                continue;
            }

            if (events[i].events & EPOLLIN)
                handle_read(fd);

            if (events[i].events & EPOLLOUT)
                handle_write(fd);
        }
    }
}

////////////////////////////////////////////////////////////
// ACCEPT
////////////////////////////////////////////////////////////
void Server::handle_accept()
{
    while (true)
    {
        int client_fd = accept(server_fd, nullptr, nullptr);

        if (client_fd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;

            log_errno("accept failed");
            return;
        }

        if (!set_non_blocking(client_fd))
        {
            close(client_fd);
            continue;
        }

        buffers[client_fd] = "";
        write_buffers[client_fd] = "";

        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = client_fd;

        epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
    }
}

////////////////////////////////////////////////////////////
// READ
////////////////////////////////////////////////////////////
void Server::handle_read(int fd)
{
    char buf[kBufferSize];

    while (true)
    {
        ssize_t n = read(fd, buf, sizeof(buf));

        if (n == 0)
        {
            close_client(fd);
            return;
        }

        if (n < 0)
        {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            log_errno("read failed");
            close_client(fd);
            return;
        }

        buffers[fd].append(buf, n);
    }

    std::string &buffer = buffers[fd];

    while (!buffer.empty())
    {
        std::vector<std::string> cmd;
        int consumed = parse_one_resp(buffer, cmd);

        if (consumed == 0)
            break;

        if (consumed < 0)
        {
            write_buffers[fd] += "-ERR protocol error\r\n";
            close_client(fd);
            return;
        }

        buffer.erase(0, consumed);

        bool was_empty = write_buffers[fd].empty();

        write_buffers[fd] += execute(store, aof, cmd);

        // 🔥 only enable EPOLLOUT when needed
        if (was_empty)
        {
            epoll_event ev{};
            ev.events = EPOLLIN | EPOLLOUT;
            ev.data.fd = fd;
            epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
        }
    }
}

////////////////////////////////////////////////////////////
// WRITE
////////////////////////////////////////////////////////////
void Server::handle_write(int fd)
{
    auto it = write_buffers.find(fd);
    if (it == write_buffers.end())
        return;

    std::string &out = it->second;

    while (!out.empty())
    {
        ssize_t n = write(fd, out.data(), out.size());

        if (n < 0)
        {
            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;

            close_client(fd);
            return;
        }

        out.erase(0, n);
    }

    // 🔥 disable EPOLLOUT
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = fd;

    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}

////////////////////////////////////////////////////////////
// CLOSE CLIENT
////////////////////////////////////////////////////////////
void Server::close_client(int fd)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);

    buffers.erase(fd);
    write_buffers.erase(fd);
}