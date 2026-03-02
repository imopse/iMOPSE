#pragma once
#include <vector>
#include <unordered_map>
#include "Resource.hpp"
#include "Task.hpp"

struct Instance {
    std::vector<Resource> resources;
    std::vector<Task> tasks;
    std::unordered_map<int, int> idToIndex;

    void buildIndex() {
        idToIndex.clear();
        for (size_t i = 0;i < tasks.size();++i) idToIndex[tasks[i].id] = int(i);
    }
};
