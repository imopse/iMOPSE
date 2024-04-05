#include "CTask.h"

CTask::CTask(TTaskID id, const std::vector<SSkill> &skills, TTime duration, const std::vector<TTaskID> &predecessors)
        : m_ID(id), m_RequiredSkills(skills), m_Duration(duration), m_Start(-1), m_Predecessors(predecessors),
          m_ResourceID(-1), m_HasSuccessors(false)
{
}

bool CTask::CanBeExecutedBy(const CResource &resource) const
{
    for (const SSkill &requiredSkill: m_RequiredSkills)
    {
        if (!resource.HasSkill(requiredSkill))
        {
            return false;
        }
    }

    return true;
}
