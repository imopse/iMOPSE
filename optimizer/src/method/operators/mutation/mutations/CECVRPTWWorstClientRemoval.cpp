#include "CECVRPTWWorstClientRemoval.h"

void CECVRPTWWorstClientRemoval::Mutate(SProblemEncoding& problemEncoding, AIndividual& child) {
	m_calculatedRemovals->clear();

	m_problem.Evaluate(child);
	float evaluationScore = child.m_Evaluation[m_objectiveIndex];
	auto& genotype = child.m_Genotype.m_IntGenotype;
	for (int i = 0; i < problemEncoding.m_Encoding[0].m_SectionDescription.size(); i++) {
		int customerIdx = genotype[i];
		if (customerIdx != VEHICLE_DELIMITER) {
			genotype.erase(genotype.begin() + i);
			m_problem.Evaluate(child);
			float newEvaluationScore = child.m_Evaluation[m_objectiveIndex];
			if (m_calculatedRemovals->size() == 0) {
				m_calculatedRemovals->insert(m_calculatedRemovals->begin()
					, std::tuple<size_t, float>(customerIdx, evaluationScore - newEvaluationScore)
				);
			}
			else {
				bool inserted = false;
				for (int j = 0; j < m_calculatedRemovals->size(); j++) {
					if (evaluationScore - newEvaluationScore > std::get<1>((*m_calculatedRemovals)[j])) {
						m_calculatedRemovals->insert(m_calculatedRemovals->begin() + j
							, std::tuple<size_t, float>(customerIdx, evaluationScore - newEvaluationScore)
						);
						j = m_calculatedRemovals->size();
						inserted = true;
					}
				}
				if (!inserted) {
					m_calculatedRemovals->emplace_back(std::tuple<size_t, float>(customerIdx, evaluationScore - newEvaluationScore));
				}
			}
			genotype.insert(genotype.begin() + i, customerIdx);
		}		
	}
	size_t indexToRemove = 0;
	if (m_calculatedRemovals->size() >= 3) {
		indexToRemove = CRandom::GetWeightedInt(std::vector<float> { 0.5, 0.3, 0.2 });
	}

	auto element = std::find(genotype.begin(), genotype.end(), std::get<0>((*m_calculatedRemovals)[indexToRemove]));
	genotype.erase(genotype.begin() + (element - genotype.begin()));
}