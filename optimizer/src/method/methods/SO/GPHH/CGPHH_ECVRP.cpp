#include "CGPHH_ECVRP.h"
#include "../utils/aggregatedFitness/CAggregatedFitness.h"
#include "../utils/experiment/CSOExperimentUtils.h"
#include "../../../../utils/logger/ErrorUtils.h"
#include "../../../../utils/logger/CExperimentLogger.h"

#include <iostream>
#include <chrono>
#include <algorithm>

using std::cout;
using std::endl;

CGPHH_ECVRP::CGPHH_ECVRP(
	std::vector<float>& objectiveWeights,
	AProblem& evaluator,
	AInitialization& initialization,
	CFitnessTournament& fitnessTournament,
	ACrossover& crossover,
	AMutation& mutation,
	IGPHHConstructive* constructive,
	SConfigMap* configMap
) : ASOGeneticMethod(evaluator, initialization, crossover, mutation, objectiveWeights),
m_FitnessTournament(fitnessTournament),
m_Constructive(constructive)
{
	configMap->TakeValue("PopulationSize", m_PopulationSize);
	m_Population.reserve(m_PopulationSize);
	ErrorUtils::LowerThanZeroI("GPHH", "PopulationSize", m_PopulationSize);

	configMap->TakeValue("GenerationLimit", m_GenerationLimit);
	ErrorUtils::LowerThanZeroI("GPHH", "GenerationLimit", m_GenerationLimit);

	m_EliteSize = 0;
	configMap->TakeValue("EliteSize", m_EliteSize);
	if (m_EliteSize < 0) m_EliteSize = 0;
	if (m_EliteSize > m_PopulationSize) m_EliteSize = m_PopulationSize;

	m_LogConfig = SGPHHLogConfig::LoadConfig(configMap);
}

void CGPHH_ECVRP::RunOptimization()
{
	cout << "Running GPHH" << endl;
	CExperimentLogger::ResetGPHHHeaderFlags();
	
	int generation = 0;

	for (size_t i = 0; i < m_PopulationSize; ++i)
	{
		CreateIndividual();
	}

	float optimalValue = m_Problem.GetOptimalValue();
	CExperimentLogger::LogGPHHGeneration(generation, m_Population, m_LogConfig, 0, optimalValue);

	while (generation < m_GenerationLimit)
	{
		auto start = std::chrono::high_resolution_clock::now();
		EvolveToNextGeneration();
		auto end = std::chrono::high_resolution_clock::now();
		long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		CExperimentLogger::LogGPHHGeneration(generation, m_Population, m_LogConfig, duration, optimalValue);
		generation++;
	}

	auto* best = CSOExperimentUtils::FindBest(m_Population);
	cout << "Best individual: " << best->m_Fitness << endl;
	CExperimentLogger::LogGPHHFinalResult(best, m_LogConfig);
	CSOExperimentUtils::LogResultData(*best, m_Problem);
}

void CGPHH_ECVRP::CreateIndividual()
{
	SProblemEncoding& problemEncoding = m_Problem.GetProblemEncoding();
	auto* newInd = static_cast<CGPHHIndividual*>(m_Initialization.CreateSOIndividual(problemEncoding));
	
	if (m_Constructive) {
		m_Constructive->BuildSolution(*newInd, m_Problem);
	}

	m_Problem.Evaluate(*newInd);
	CAggregatedFitness::CountFitness(*newInd, m_ObjectiveWeights);

	m_Population.push_back(newInd);
}

void CGPHH_ECVRP::EvolveToNextGeneration()
{
	std::vector<CGPHHIndividual*> elites;
	if (m_EliteSize > 0) {
		std::vector<SSOIndividual*> sortedPop = m_Population;
		std::sort(sortedPop.begin(), sortedPop.end(),
			[](SSOIndividual* a, SSOIndividual* b) {
				return a->m_Fitness < b->m_Fitness;
			});

		for (int i = 0; i < m_EliteSize && i < sortedPop.size(); ++i) {
			elites.push_back(new CGPHHIndividual(*static_cast<CGPHHIndividual*>(sortedPop[i])));
		}
	}

	std::vector<SSOIndividual*> children;
	children.reserve(m_PopulationSize);

	for (size_t i = 0; i < m_PopulationSize; i += 2)
	{
		auto* firstParent = m_FitnessTournament.Select(m_Population);
		auto* secondParent = m_FitnessTournament.Select(m_Population);

		auto* firstChild = new CGPHHIndividual(*static_cast<CGPHHIndividual*>(firstParent));
		auto* secondChild = new CGPHHIndividual(*static_cast<CGPHHIndividual*>(secondParent));

		m_Crossover.Crossover(
			m_Problem.GetProblemEncoding(),
			*firstParent,
			*secondParent,
			*firstChild,
			*secondChild
		);

		m_Mutation.Mutate(m_Problem.GetProblemEncoding(), *firstChild);
		m_Mutation.Mutate(m_Problem.GetProblemEncoding(), *secondChild);

		if (m_Constructive) {
			m_Constructive->BuildSolution(*firstChild, m_Problem);
			m_Constructive->BuildSolution(*secondChild, m_Problem);
		}

		m_Problem.Evaluate(*firstChild);
		CAggregatedFitness::CountFitness(*firstChild, m_ObjectiveWeights);
		m_Problem.Evaluate(*secondChild);
		CAggregatedFitness::CountFitness(*secondChild, m_ObjectiveWeights);

		children.emplace_back(firstChild);
		children.emplace_back(secondChild);
	}

	if (m_EliteSize > 0 && !elites.empty()) {
		std::sort(children.begin(), children.end(),
			[](SSOIndividual* a, SSOIndividual* b) {
				return a->m_Fitness > b->m_Fitness;
			});

		for (size_t i = 0; i < elites.size() && i < children.size(); ++i) {
			delete children[i];
			children[i] = elites[i];
		}
	}

	for (auto& i : m_Population)
	{
		delete i;
	}

	m_Population.clear();
	m_Population.swap(children);
}
