#pragma once
#include <string>
#include <vector>

int parse_one_resp(const std::string &input, std::vector<std::string> &out);
std::string build_resp(const std::vector<std::string> &cmd);