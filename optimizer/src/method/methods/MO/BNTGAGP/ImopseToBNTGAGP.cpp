#include "ImopseToBNTGAGP.h"
#include "problem/problems/MSRCPSP/CScheduler.h"
#include <string>
#include <algorithm>
#include <vector>
#include <unordered_map>

static std::string skillName(unsigned typeId) {
    return "Q" + std::to_string(typeId);
}

Instance BNTGAGPAdapter::FromScheduler(const CScheduler& sch) {
    Instance I;

    for (const auto& r : sch.GetResources()) {
        Resource rr;
        rr.id = (int)r.GetResourceID();
        rr.salary = (double)r.GetSalary();
        rr.busy = false;
        rr.busyUntil = 0;

        for (const auto& sk : r.GetSkills()) {
            rr.skills[skillName(sk.m_TypeID)] = (int)sk.m_Level;
        }
        I.resources.push_back(std::move(rr));
    }

    const auto& tasks = sch.GetTasks();
    for (size_t ti = 0; ti < tasks.size(); ++ti) {
        const auto& t = tasks[ti];

        Task tt;
        tt.id = (int)t.GetTaskID();
        tt.duration = (int)t.GetDuration();
        tt.predecessors.clear();
        for (auto pid : t.GetPredecessors()) tt.predecessors.push_back((int)pid);

        const auto& req = t.GetRequiredSkills();

        tt.requiredSkills.clear();
        tt.requiredSkills.reserve(req.size());

        for (const auto& rs : req) {
            tt.requiredSkills.push_back(
                SkillRequirement{ skillName(rs.m_TypeID), (int)rs.m_Level }
            );
        }

        std::sort(tt.requiredSkills.begin(), tt.requiredSkills.end(),
            [](const SkillRequirement& a, const SkillRequirement& b) {
                if (a.skill != b.skill) return a.skill < b.skill;
                return a.level < b.level;
            });

        if (!tt.requiredSkills.empty()) {
            tt.reqSkill = tt.requiredSkills.front().skill;
            tt.reqLevel = tt.totalRequiredLevel();
        }
        else {
            tt.reqSkill = "";
            tt.reqLevel = 0;
        }

        tt.imopseIndex = (int)ti;

        std::vector<TResourceID> cap;
        sch.GetCapableResources(t, cap);
        tt.capableResources.assign(cap.begin(), cap.end());
        std::sort(tt.capableResources.begin(), tt.capableResources.end());

        tt.start = -1;
        tt.finish = -1;
        tt.assignedResources.clear();

        I.tasks.push_back(std::move(tt));
    }

    std::sort(I.resources.begin(), I.resources.end(),
        [](const Resource& a, const Resource& b) { return a.id < b.id; });

    std::sort(I.tasks.begin(), I.tasks.end(),
        [](const Task& a, const Task& b) { return a.id < b.id; });

    for (auto& t : I.tasks) {
        std::sort(t.predecessors.begin(), t.predecessors.end());
    }

    I.buildIndex();

    std::unordered_map<int, int> resIdToIndex;
    resIdToIndex.reserve(I.resources.size() * 2);
    for (int i = 0; i < (int)I.resources.size(); ++i) {
        resIdToIndex[I.resources[i].id] = i;
    }

    for (auto& t : I.tasks) {
        t.capableResourceIndices.clear();
        t.capableResourceIndices.reserve(t.capableResources.size());

        for (int rid : t.capableResources) {
            auto it = resIdToIndex.find(rid);
            if (it != resIdToIndex.end()) {
                t.capableResourceIndices.push_back(it->second);
            }
        }
    }

    I.rebuildStructureSignatures();
    return I;
}