#include "ImopseToGPHH.h"
#include "problem/problems/MSRCPSP/CScheduler.h"
#include <string>
#include <algorithm>
#include <vector>

static std::string skillName(unsigned typeId) {
    return "Q" + std::to_string(typeId);
}

Instance GPHHAdapter::FromScheduler(const CScheduler& sch) {
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
        if (!req.empty()) {
            tt.reqSkill = skillName(req[0].m_TypeID);
            tt.reqLevel = (int)req[0].m_Level;
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
    I.rebuildResourceStructureSignature();
    return I;
}
