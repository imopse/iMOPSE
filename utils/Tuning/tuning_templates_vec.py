from tuning_generator import generate_configs_for_method


generate_configs_for_method(
    template={
       'MethodName': ['NTGA2_ALNS'],
       'InitializationName': ['ECVRPTW'],
       'PopulationSize': [50],
       'GenerationLimit': [200],
       'Crossover CVRP_OX': [0.6, 0.7],
       'Mutation CVRP_Reverse_Flip': [0.01, 0.1],
       'GapSelectionPercent': [40, 70],
       'GapSelection': [2, 6, 10],
       'RankedTournament': [3, 6, 8],
       'ALNSTournament': [5, 10],
       'EliteSize': [4],
       'ALNSIterations': [50],
       'ALNSNoImprovementIterations': [25],
       'ALNSProbabilityStepsIterations': [5],
       'ALNSStartTemperature': [300],
       'ALNSTemperatureAnnealingRate': [0.91],
    },
    configs_path=''
)