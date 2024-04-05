#include "CResource.h"

CResource::CResource(TResourceID id, float salary, const std::vector<SSkill> &skills)
        : m_ID(id), m_Salary(salary), m_Skills(skills), m_Finish(-1), m_WorkingTime(-1)
{
}

bool CResource::HasSkill(const SSkill &querySkill) const
{
    for (const SSkill &availableSkill: m_Skills)
    {
        if (availableSkill.m_TypeID == querySkill.m_TypeID)
        {
            // We assume that resource can have only one entry per skill type
            return availableSkill.m_Level >= querySkill.m_Level;
        }
    }

    return false;
}

bool CResource::GetSkillLevel(TSkillType skillType, TSkillLevel &skillLevel) const
{
    for (const SSkill &availableSkill: m_Skills)
    {
        if (availableSkill.m_TypeID == skillType)
        {
            // We assume that resource can have only one entry per skill type
            skillLevel = availableSkill.m_Level;
            return true;
        }
    }

    return false;
}
