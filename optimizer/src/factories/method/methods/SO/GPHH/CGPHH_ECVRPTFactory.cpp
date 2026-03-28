#include "CGPHH_ECVRPTFactory.h"
#include "../../../../../method/methods/SO/GPHH/CGPHH_ECVRP.h"
#include "../../../../../method/operators/selection/selections/CFitnessTournament.h"
#include <sstream>
#include "../../../../../method/methods/SO/GPHH/constructive/CCVRPConstructive.h"
#include "../../../../../problem/problems/CVRP/CCVRP.h"

CGPHHInitialization* CGPHH_ECVRPTFactory::initialization = nullptr;
CGPHHCrossover* CGPHH_ECVRPTFactory::crossover = nullptr;
CGPHHMutation* CGPHH_ECVRPTFactory::mutation = nullptr;
IGPHHConstructive* CGPHH_ECVRPTFactory::constructive = nullptr;

AMethod* CGPHH_ECVRPTFactory::CreateGPHH(SConfigMap* configMap, AProblem& problem) {

	int treeDepthLimit = 5;
	configMap->TakeValue("TreeDepthLimit", treeDepthLimit);

	std::string heuristicsStr;
	std::vector<std::string> terminals;
	if (configMap->TakeValue("EnabledHeuristics", heuristicsStr)) {
		terminals = Split(heuristicsStr, ',');
	}
	else {
		terminals = { "Dist", "Demand", "RC", "DRC", "NCC", "Const" };
	}

	std::string operatorsStr;
	std::vector<std::string> functions;
	if (configMap->TakeValue("EnabledOperators", operatorsStr)) {
		functions = Split(operatorsStr, ',');
	}
	else {
		functions = { "+", "-", "*", "/", "min", "max" };
	}

	float pointMutationRate = 0.3f;
	float subtreeMutationRate = 0.3f;
	configMap->TakeValue("PointMutationRate", pointMutationRate);
	configMap->TakeValue("SubtreeMutationRate", subtreeMutationRate);

	float crossoverRate = 0.9f;
	configMap->TakeValue("CrossoverRate", crossoverRate);

	initialization = new CGPHHInitialization(treeDepthLimit, terminals, functions);
	crossover = new CGPHHCrossover(treeDepthLimit, crossoverRate);
	mutation = new CGPHHMutation(treeDepthLimit, terminals, functions, pointMutationRate, subtreeMutationRate);

	if (dynamic_cast<CCVRP*>(&problem)) {
		constructive = new CCVRPConstructive();
	}
	else {
		constructive = nullptr;
	}

	// Create Fitness Tournament
	static CFitnessTournament* tournament = nullptr;
	if (tournament) delete tournament;

	int tournamentSize = 2;
	configMap->TakeValue("TournamentSize", tournamentSize);
	tournament = new CFitnessTournament(tournamentSize);

	// Objective weights
	static std::vector<float> weights = { 1.0f };

	return new CGPHH_ECVRP(
		weights,
		problem,
		*initialization,
		*tournament,
		*crossover,
		*mutation,
		constructive,
		configMap
	);
}

void CGPHH_ECVRPTFactory::DeleteObjects() {
	if (initialization) { delete initialization; initialization = nullptr; }
	if (crossover) { delete crossover; crossover = nullptr; }
	if (mutation) { delete mutation; mutation = nullptr; }
	if (constructive) { delete constructive; constructive = nullptr; }
}

std::vector<std::string> CGPHH_ECVRPTFactory::Split(const std::string& s, char delimiter) {
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter)) {
		// Trim whitespace
		size_t first = token.find_first_not_of(' ');
		if (std::string::npos == first) {
			continue;
		}
		size_t last = token.find_last_not_of(' ');
		tokens.push_back(token.substr(first, (last - first + 1)));
	}
	return tokens;
}
