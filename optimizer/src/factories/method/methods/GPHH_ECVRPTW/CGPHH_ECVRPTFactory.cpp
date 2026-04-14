#include "CGPHH_ECVRPTWFactory.h"
#include "method/methods/SO/GPHH/CGPHH_ECVRP.h"
#include "method/operators/selection/selections/CFitnessTournament.h"
#include <sstream>
#include "method/methods/SO/GPHH/constructive/CCVRPConstructive.h"
#include "problem/problems/CVRP/CCVRP.h"

AMethod* CGPHH_ECVRPTWFactory::CreateGPHH(SConfigMap* configMap, AProblem* problem) {

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

	auto initialization = new CGPHHInitialization(treeDepthLimit, terminals, functions);
	auto crossover = new CGPHHCrossover(treeDepthLimit, crossoverRate);
	auto mutation = new CGPHHMutation(treeDepthLimit, terminals, functions, pointMutationRate, subtreeMutationRate);

	auto constructive = (dynamic_cast<CCVRP*>(problem)) ? new CCVRPConstructive() : nullptr;

	// Create Fitness Tournament
	static CFitnessTournament* tournament = nullptr;
	if (tournament) delete tournament;

	int tournamentSize = 2;
	configMap->TakeValue("TournamentSize", tournamentSize);
	tournament = new CFitnessTournament(tournamentSize);

	// Objective weights
	auto* weights = new std::vector({ 1.0f });
	
	return new CGPHH_ECVRP(
		problem,
		initialization,
		tournament,
		crossover,
		mutation,
		constructive,
		configMap,
		weights
	);
}

std::vector<std::string> CGPHH_ECVRPTWFactory::Split(const std::string& s, char delimiter) {
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
