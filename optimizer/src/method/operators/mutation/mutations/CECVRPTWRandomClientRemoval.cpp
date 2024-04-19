#include "CECVRPTWRandomClientRemoval.h"

void CECVRPTWRandomClientRemoval::Mutate(SProblemEncoding& problemEncoding, AIndividual& child) {
	int customersToRemove = CRandom::GetInt(1, problemEncoding.m_Encoding[0].m_SectionDescription.size() - m_problem.GetECVRPTWTemplate().GetVehicleCount() - 1);
	for (int i = 0; i < customersToRemove; i++) {
		int customerIdx = CRandom::GetInt(0, child.m_Genotype.m_IntGenotype.size());
		if (child.m_Genotype.m_IntGenotype[customerIdx] == VEHICLE_DELIMITER) {
			i--;
		}
		else {
			child.m_Genotype.m_IntGenotype.erase(child.m_Genotype.m_IntGenotype.begin() + customerIdx);
		}
	}
}