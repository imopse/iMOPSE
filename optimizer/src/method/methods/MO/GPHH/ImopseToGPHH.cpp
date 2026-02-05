#include "ImopseToGPHH.h"
#include <string>

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

    for (const auto& t : sch.GetTasks()) {
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

        tt.start = -1;
        tt.finish = -1;
        tt.assignedResources.clear();

        I.tasks.push_back(std::move(tt));
    }

    I.buildIndex();
    return I;
}
