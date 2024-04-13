import os
import pandas as pd
import matplotlib.pyplot as plt

# List of paths to results directories to create plots for
paths = ['../optimizer/experiments/GA'] # Input paths

number_of_runs = 1 # Input number of runs

def create_mean_values(path):
    all_runs_data = []

    for i in range(number_of_runs):
        file_path = os.path.join(path, f'run_{i}', 'data.csv')

        if os.path.exists(file_path):
            data = pd.read_csv(file_path, delimiter=';', header=None, nrows=350)

            y = data.iloc[:, 1]

            all_runs_data.append(y)

    mean_values = pd.concat(all_runs_data, axis=1).mean(axis=1)

    return mean_values

plt.figure()

for path in paths:
    mean_values = create_mean_values(path)
    x_index = range(len(mean_values))
    plt.plot(x_index, mean_values, label=path)

plt.xlabel('Iterations')
plt.ylabel('Fitness')
plt.legend()
plt.grid(True)
plt.show()
