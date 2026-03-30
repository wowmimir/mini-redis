#pragma once
#include <unordered_map>
#include <string>

class Store {
private:
    std::unordered_map<std::string, std::string> data;

public:
    void set(const std::string &k, const std::string &v);
    bool get(const std::string &k, std::string &out);
    int del(const std::string &k);

    const std::unordered_map<std::string, std::string>& snapshot() const;
};