#pragma once
#include <vector>
#include <string>
#include "../store/store.h"
#include "../persistence/aof.h"
#include <iostream>

std::string execute(Store &store, AOF &aof, const std::vector<std::string> &cmd);