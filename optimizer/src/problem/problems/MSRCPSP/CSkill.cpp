#include "CSkill.h"

SSkill::SSkill()
        : m_TypeID(0), m_Level(0)
{
}

SSkill::SSkill(TSkillType typeID, TSkillLevel level)
        : m_TypeID(typeID), m_Level(level)
{
}
