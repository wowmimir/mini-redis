#pragma once
#include <string>

struct Connection {
    int fd;
    std::string read_buffer;
    std::string write_buffer;

    Connection(int fd_) : fd(fd_) {}
};