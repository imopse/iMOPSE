#include "CECVRPTWWorstClientRemoval.h"

void CECVRPTWWorstClientRemoval::Mutate(SProblemEncoding& problemEncoding, AIndividual& child) {
	m_problem.Evaluate(child);
	float evaluationScore = m_problem.GetScore(child);
	auto& genotype = child.m_Genotype.m_IntGenotype;
	std::vector<std::tuple<size_t, float>> calculatedRemovals;
	calculatedRemovals.reserve(problemEncoding.m_Encoding[0].m_SectionDescription.size() + m_problem.GetECVRPTWTemplate().GetVehicleCount() - 1);
	for (int i = 0; i < problemEncoding.m_Encoding[0].m_SectionDescription.size(); i++) {
		int customerIdx = genotype[i];
		if (customerIdx != VEHICLE_DELIMITER) {
			genotype.erase(genotype.begin() + i);
			m_problem.Evaluate(child);
			float newEvaluationScore = m_problem.GetScore(child);
			if (calculatedRemovals.size() == 0) {
				calculatedRemovals.insert(calculatedRemovals.begin()
					, std::tuple<size_t, float>(customerIdx, evaluationScore - newEvaluationScore)
				);
			}
			else {
				bool inserted = false;
				for (int j = 0; j < calculatedRemovals.size(); j++) {
					if (evaluationScore - newEvaluationScore > std::get<1>(calculatedRemovals[j])) {
						calculatedRemovals.insert(calculatedRemovals.begin() + j
							, std::tuple<size_t, float>(customerIdx, evaluationScore - newEvaluationScore)
						);
						j = calculatedRemovals.size();
						inserted = true;
					}
				}
				if (!inserted) {
					calculatedRemovals.emplace_back(std::tuple<size_t, float>(customerIdx, evaluationScore - newEvaluationScore));
				}
			}
			genotype.insert(genotype.begin() + i, customerIdx);
		}		
	}
	size_t indexToRemove = 0;
	if (calculatedRemovals.size() >= 3) {
		indexToRemove = CRandom::GetWeightedInt(std::vector<float> { 0.5, 0.3, 0.2 });
	}

	auto element = std::find(genotype.begin(), genotype.end(), std::get<0>(calculatedRemovals[indexToRemove]));
	genotype.erase(genotype.begin() + (element - genotype.begin()));
}