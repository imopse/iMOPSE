import os
import pandas as pd
import matplotlib.pyplot as plt

# Set the paths to the directories containing the results of each method
paths = ['../paretoAnalyzer/NSGAII_merged.csv',
         '../paretoAnalyzer/NTGA2_merged.csv']  # Replace with the correct paths

# Prepare a dictionary to hold the data from each method
pareto_data = {}

for path in paths:
    if os.path.isfile(path):
        data = pd.read_csv(path, delimiter=';', header=None)

        pareto_data[path] = data.iloc[:, :2]
    else:
        print(f'Results file for the first run of {path} does not exist.')

plt.figure(figsize=(10, 6))
for method, data in pareto_data.items():
    plt.scatter(data.iloc[:, 0], data.iloc[:, 1], label=method)

plt.title('Merged Pareto Fronts Comparison')
plt.xlabel('Objective 1')
plt.ylabel('Objective 2')
plt.legend()
plt.grid(True)

plt.show()
