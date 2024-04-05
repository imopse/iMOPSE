import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# List of methods to create plots for
methods = ['GA', 'DE', 'SA', 'TS', 'PSO']

# Base path where the methods directories are located
base_path = '../..'  # Adjust this path to the correct base directory

# Function to calculate mean
def create_mean_values(method):
    # Initialize an empty list to store data from all runs
    all_runs_data = []

    # Loop over each run and read the second column
    for i in range(1):  # Assuming there are 10 runs from run_0 to run_9
        file_path = os.path.join(base_path, 'experiments', method, f'run_{i}', 'data.csv')

        # Check if the file exists before reading
        if os.path.exists(file_path):
            data = pd.read_csv(file_path, delimiter=';', header=None, nrows=350)

            # Select the second column (y values)
            y = data.iloc[:, 1]

            # If method is 'SA', calculate mean of every 6th row
            # Append the data to the list
            all_runs_data.append(y)

    # Calculate the mean across all runs (row-wise)
    mean_values = pd.concat(all_runs_data, axis=1).mean(axis=1)

    return mean_values

# Initialize plot
plt.figure()

# Create and plot mean value for each method
for method in methods:
    mean_values = create_mean_values(method)
    x_index = range(len(mean_values))
    plt.plot(x_index, mean_values, label=method)

# Finalize and show the combined plot
plt.xlabel('Iterations')
plt.ylabel('Duration')
plt.legend()
plt.grid(True)
plt.savefig(os.path.join(base_path, 'combined_mean_plot.png'))
plt.xlim(0, 250)
plt.show()
