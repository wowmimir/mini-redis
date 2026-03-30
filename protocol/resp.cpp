#include "resp.h"
#include <string>
#include <vector>
#include <stdexcept>

using namespace std;

////////////////////////////////////////////////////////////
// RESP BUILDER
////////////////////////////////////////////////////////////
string build_resp(const vector<string> &cmd)
{
    string resp = "*" + to_string(cmd.size()) + "\r\n";
    for (const auto &arg : cmd)
    {
        resp += "$" + to_string(arg.size()) + "\r\n";
        resp += arg + "\r\n";
    }
    return resp;
}

////////////////////////////////////////////////////////////
// PARSER (IMPROVED)
////////////////////////////////////////////////////////////
int parse_one_resp(const string &input, vector<string> &result)
{
    result.clear();  // 🔥 FIX 1

    int i = 0;
    int n = input.size();

    if (i >= n || input[i] != '*')
        return 0;
    i++;

    // ---- parse array length ----
    int start = i;
    while (i < n && input[i] != '\r') i++;
    if (i + 1 >= n || input[i] != '\r' || input[i+1] != '\n')
        return 0;

    int num;
    try {
        num = stoi(input.substr(start, i - start));
    } catch (...) {
        return -1;  // 🔥 FIX 2: malformed
    }

    i += 2;

    // ---- parse elements ----
    for (int j = 0; j < num; j++)
    {
        if (i >= n || input[i] != '$')
            return -1;
        i++;

        start = i;
        while (i < n && input[i] != '\r') i++;
        if (i + 1 >= n || input[i] != '\r' || input[i+1] != '\n')
            return 0;

        int len;
        try {
            len = stoi(input.substr(start, i - start));
        } catch (...) {
            return -1;
        }

        i += 2;

        if (i + len + 2 > n)
            return 0;

        result.emplace_back(input.substr(i, len));

        i += len;

        // check \r\n after data
        if (input[i] != '\r' || input[i+1] != '\n')
            return -1;

        i += 2;
    }

    return i;  // bytes consumed
}