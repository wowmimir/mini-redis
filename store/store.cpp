#include "store.h"

void Store::set(const std::string &k, const std::string &v) {
    data[k] = v;
}

bool Store::get(const std::string &k, std::string &out) {
    if (data.count(k)) {
        out = data[k];
        return true;
    }
    return false;
}

int Store::del(const std::string &k) {
    return data.erase(k);
}

const std::unordered_map<std::string, std::string>& Store::snapshot() const {
    return data;
}