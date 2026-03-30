#include "command.h"

std::string execute(Store &store, AOF &aof, const std::vector<std::string> &cmd)
{
    if (cmd.empty())
        return "-ERR invalid request\r\n";

    const std::string &op = cmd[0];

    ////////////////////////////////////////////////////////////
    // SET
    ////////////////////////////////////////////////////////////
    if (op == "SET")
    {
        if (cmd.size() < 3)
            return "-ERR wrong args\r\n";

        if (!aof.append(cmd))   // 🔥 MUST return bool now
            return "-ERR AOF write failed\r\n";

        store.set(cmd[1], cmd[2]);
        return "+OK\r\n";
    }

    ////////////////////////////////////////////////////////////
    // GET
    ////////////////////////////////////////////////////////////
    if (op == "GET")
    {
        if (cmd.size() < 2)
            return "-ERR wrong args\r\n";

        std::string val;
        if (store.get(cmd[1], val))
            return "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";

        return "$-1\r\n";
    }

    ////////////////////////////////////////////////////////////
    // DEL
    ////////////////////////////////////////////////////////////
    if (op == "DEL")
    {
        if (cmd.size() < 2)
            return "-ERR wrong args\r\n";

        if (!aof.append(cmd))   // 🔥 same fix
            return "-ERR AOF write failed\r\n";

        int deleted = store.del(cmd[1]);
        return ":" + std::to_string(deleted) + "\r\n";
    }

    ////////////////////////////////////////////////////////////
    // REWRITE
    ////////////////////////////////////////////////////////////
    if (op == "REWRITE")
    {
        // ⚠️ still blocking — we’ll fix later
        aof.rewrite(store);
        return "+OK\r\n";
    }

    return "-ERR unknown command\r\n";
}
