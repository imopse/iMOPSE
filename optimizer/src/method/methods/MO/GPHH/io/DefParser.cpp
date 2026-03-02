#include "DefParser.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include "../util/String.hpp"

static bool isSeparatorLine(const std::string& s) {
    std::string t = trim(s);
    return t.rfind("====", 0) == 0 || t.empty();
}

bool DefParser::parseFile(const std::string& path, Instance& out)
{
    std::ifstream in(path);
    if (!in) return false;

    std::vector<Resource> resources;
    std::vector<Task> tasks;

    bool inResources = false, inTasks = false;
    std::string line;

    while (std::getline(in, line)) {
        line = trim(line);
        if (isSeparatorLine(line)) continue;

        if (line.rfind("ResourceID", 0) == 0) { inResources = true;  inTasks = false; continue; }
        if (line.rfind("TaskID", 0) == 0) { inResources = false; inTasks = true;  continue; }

        if (inResources && startsWithDigit(line)) {
            std::stringstream ss(line);
            Resource r;
            ss >> r.id >> r.salary;

            std::string token;
            while (ss >> token) {
                if (!token.empty() && token.back() == ':') {
                    token.pop_back();
                    int val = 0; ss >> val;
                    r.skills[token] = val;
                }
            }
            resources.push_back(r);
        }
        else if (inTasks && startsWithDigit(line)) {
            std::stringstream ss(line);
            Task t;
            ss >> t.id >> t.duration;

            std::string sk; ss >> sk;
            if (!sk.empty() && sk.back() == ':') sk.pop_back();
            int lev = 0; ss >> lev;

            t.reqSkill = sk;
            t.reqLevel = lev;

            int p;
            while (ss >> p) t.predecessors.push_back(p);

            tasks.push_back(t);
        }
    }

    std::sort(tasks.begin(), tasks.end(),
        [](const Task& a, const Task& b) { return a.id < b.id; });

    out.resources = std::move(resources);
    out.tasks = std::move(tasks);
    out.buildIndex();

    for (auto& t : out.tasks) {
        std::vector<int> ok;
        ok.reserve(t.predecessors.size());
        for (int pid : t.predecessors) {
            if (out.idToIndex.count(pid)) ok.push_back(pid);
        }
        t.predecessors.swap(ok);
    }
    return true;
}
