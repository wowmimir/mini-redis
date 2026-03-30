#pragma once

#include <string>
#include <unordered_map>

#include "../persistence/aof.h"
#include "../store/store.h"

class Server {
public:
    Server();
    ~Server();
    

    void run();

private:
    int server_fd;
    Store store;
    AOF aof;
    
    int epfd;

    // Per-client state.
    std::unordered_map<int, std::string> buffers;
    std::unordered_map<int, std::string> write_buffers;

    void handle_accept();
    void handle_read(int fd);
    void handle_write(int fd);
    void close_client(int fd);
};
