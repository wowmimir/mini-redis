#pragma once
#include <vector>
#include <string>
#include "../store/store.h"

class AOF {
private:
    int fd;
    bool is_rewriting;
    std::string rewrite_buffer;

public:
    AOF();
    ~AOF();

    bool append(const std::vector<std::string> &cmd);
    void load(Store &store);

    void rewrite(Store &store);
};
