#pragma once
#include <string>
#include <vector>

struct Task {
    int id = 0;
    int duration = 0;
    std::string reqSkill;
    int reqLevel = 0;
    std::vector<int> predecessors;

    int imopseIndex = -1;

    std::vector<int> capableResources;

    int start = -1;
    int finish = -1;
    std::vector<int> assignedResources;
};