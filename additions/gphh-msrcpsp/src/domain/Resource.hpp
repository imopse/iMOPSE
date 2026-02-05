#pragma once
#include <string>
#include <unordered_map>

struct Resource {
    int id = 0;
    double salary = 0.0;
    std::unordered_map<std::string, int> skills;

    bool busy = false;
    int  busyUntil = 0;
};
