#include "MSRAInstance.h"

void CMSRAInstance::Clear()
{
	m_Tasks.clear();
	m_Resources.clear();
	m_ProbMtx.clear();
    m_ResourceTaskTransition.clear();
}
