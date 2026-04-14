#include "CMethodFactory.h"
#include "configMap/CConfigFactory.h"
#include "operators/initialization/CInitializationFactory.h"
#include "operators/crossover/CCrossoverFactory.h"
#include "operators/mutation/CMutationFactory.h"
#include "utils/fileReader/CReadUtils.h"
#include "method/methods/MO/GPHH/CGPHH.h"
#include "factories/method/operators/selection/CSelectionFactory.h"
#include "method/methods/SO/ACO/TSP-BASED/CACO_TSP.h"
#include "factories/method/methods/GPHH_ECVRPTW/CGPHH_ECVRPTWFactory.h"
#include "method/methods/SO/SA/CSA.h"
#include "method/methods/SO/TS/CTS.h"
#include "method/methods/SO/DEGR/CDE.h"
#include "method/methods/SO/PSO/CPSO.h"
#include "method/methods/SO/GA/CGA.h"
#include "method/methods/MO/NTGA2/CNTGA2.h"
#include "method/methods/MO/NSGAII/CNSGAII.h"
#include "method/methods/MO/MOEAD/CMOEAD.h"
#include "method/methods/MO/MOEAD/CMOEAD_FRRMAB.h"
#include "method/methods/MO/ANTGA/CANTGA.h"
#include "method/methods/MO/FANGA/CFANGA.h"
#include "method/methods/MO/BNTGA/CBNTGA.h"
#include "method/methods/MO/SPEA2/CSPEA2.h"
#include "method/methods/MO/NTGA2_ALNS/CNTGA2_ALNS.h"

AMethod* CMethodFactory::CreateMethod( const char* optimizerConfigPath, AProblem* problem )
{
	auto* configMap = CConfigFactory::CreateConfigMap(optimizerConfigPath);
	if (configMap == nullptr) {
		throw std::runtime_error("Error while reading method configuration");
	}

	std::string methodName;
	if (!configMap->TakeValue("MethodName", methodName)) {
		throw std::runtime_error("MethodName not provided in method configuration");
	}

	auto* initialization = CInitializationFactory::Create(configMap, problem);

	if (methodName == "GPHH_ECVRPTW")
		return CGPHH_ECVRPTWFactory::CreateGPHH(configMap, problem);

	if (methodName == "ACO")
		return new CACO_TSP(
				problem,
				initialization,
				configMap,
				CMethodFactory::ProcessObjectiveWeights(configMap)
		);
	if (methodName == "SA")
		return new CSA(
				problem,
				initialization,
				configMap,
				CMethodFactory::ProcessObjectiveWeights(configMap)
		);
	if (methodName == "TS")
		return new CTS(
				problem,
				initialization,
				configMap,
				CMethodFactory::ProcessObjectiveWeights(configMap)
		);
	if (methodName == "DE")
		return new CDE(
				problem,
				initialization,
				configMap,
				CMethodFactory::ProcessObjectiveWeights(configMap)
		);
	if (methodName == "PSO")
		return new CPSO(
				problem,
				initialization,
				configMap,
				CMethodFactory::ProcessObjectiveWeights(configMap)
		);
	if (methodName == "GPHH")
		return new CGPHH(problem, initialization, configMap);

	auto* crossover = CCrossoverFactory::Create(configMap, problem);
	if (crossover == nullptr) {
		throw std::runtime_error("Error while reading crossover configuration");
	}

	auto* mutation = CMutationFactory::Create(configMap, problem);
	if (mutation == nullptr) {
		throw std::runtime_error("Error while reading mutation configuration");
	}
	
	if (methodName == "GA")
		return new CGA(
				problem,
				initialization,
                CSelectionFactory::CreateFitnessTournamentSelection(configMap),
				crossover,
				mutation,
				configMap,
                CMethodFactory::ProcessObjectiveWeights(configMap)
		);
	if (methodName == "NTGA2")
		return new CNTGA2(
				problem,
				initialization,
				CSelectionFactory::CreateRankedTournamentSelection(configMap),
				CSelectionFactory::CreateGapSelection(configMap, false),
				crossover,
				mutation,
				configMap
		);
	if (methodName == "NSGAII")
		return new CNSGAII(
				problem,
				initialization,
				CSelectionFactory::CreateRankedTournamentSelection(configMap),
				crossover,
				mutation,
				configMap
		);
	if (methodName == "MOEAD")
		return new CMOEAD(
				problem,
				initialization,
				crossover,
				mutation,
				configMap
		);
	if (methodName == "MOEAD_FRRMAB")
		return new CMOEAD_FRRMAB(
				problem,
				initialization,
				crossover,
				mutation,
				configMap
		);
	if (methodName == "ANTGA")
		return new CANTGA(
				problem,
				initialization,
				crossover,
				mutation,
				CSelectionFactory::CreateGapSelection(configMap, true),
				configMap
		);
	if (methodName == "FANGA")
		return new CFANGA(
				problem,
				initialization,
				crossover,
				mutation,
				CSelectionFactory::CreateGapSelection(configMap, true),
				configMap
		);
	if (methodName == "BNTGA")
		return new CBNTGA(
				problem,
				initialization,
				crossover,
				mutation,
				CSelectionFactory::CreateGapSelection(configMap, true),
				configMap
		);
	if (methodName == "SPEA2")
		return new CSPEA2(
				problem,
				initialization,
				crossover,
				mutation,
				configMap
		);
	if (methodName == "NTGA2_ALNS")
	{
		return new CNTGA2_ALNS(
				problem,
				initialization,
				CSelectionFactory::CreateRankedTournamentSelection(configMap),
				CSelectionFactory::CreateGapSelection(configMap, false),
				crossover,
				mutation,
				configMap,
				CMutationFactory::CreateRemovalOperators(problem),
				CMutationFactory::CreateInsertionOperators(problem)
		);
	}

    delete configMap;
    
	throw std::runtime_error("Method name: " + methodName + " not supported");
}

std::vector<float>* CMethodFactory::ProcessObjectiveWeights(SConfigMap* configMap)
{
	std::string rawWeightsString;
	configMap->TakeValue("ObjectiveWeights", rawWeightsString);

	if (!rawWeightsString.empty())
	{
		return CReadUtils::ReadWeights(rawWeightsString);
	} else {
		return new std::vector<float>({1.0f});
	}
}