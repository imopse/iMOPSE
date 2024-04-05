#pragma once

#include <cstdint>
#include "CResource.h"

using TTaskID = int16_t;

class CTask
{
public:

    CTask(TTaskID id, const std::vector<SSkill> &skills, TTime duration, const std::vector<TTaskID> &predecessors);

    TTime GetStart() const
    { return m_Start; }

    void SetStart(TTime startTime)
    { m_Start = startTime; }

    TTime GetDuration() const
    { return m_Duration; }

    TResourceID GetResourceID() const
    { return m_ResourceID; }

    void SetResourceID(TResourceID resourceID)
    { m_ResourceID = resourceID; }

    bool GetHasSuccessors() const
    { return m_HasSuccessors; }

    void SetHasSuccessors(bool hasSuccessors)
    { m_HasSuccessors = hasSuccessors; }

    const std::vector<TTaskID> &GetPredecessors() const
    { return m_Predecessors; }

    TTime GetExpectedFinish() const
    { return m_Start + m_Duration; }

    bool CanBeExecutedBy(const CResource &resource) const;

    const std::vector<SSkill> &GetRequiredSkills() const
    { return m_RequiredSkills; }

private:

    TTaskID m_ID;
    std::vector<SSkill> m_RequiredSkills;
    TTime m_Duration;
    TTime m_Start;
    std::vector<TTaskID> m_Predecessors;
    TResourceID m_ResourceID;
    bool m_HasSuccessors;
};
