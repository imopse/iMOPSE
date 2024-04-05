import os
import pandas as pd
import matplotlib.pyplot as plt

# Define colors and labels for each method
colors = ['blue', 'red']
labels = ['NSGAII', 'NTGA2']

# List all the method folders in the directory
paths = ["./NSGAII__merged.csv",
         "./NTGA2__merged.csv"]

for path, color, label in zip(paths, colors, labels):
    if os.path.isfile(path):
        # Read the CSV file
        data = pd.read_csv(path, delimiter=';', header=None)

        # Only consider the first two dimensions for plotting
        data = data.iloc[:, :2]

        # Plot the data for the first two dimensions
        plt.scatter(data.iloc[:, 0], data.iloc[:, 1], color=color, label=label)

# After plotting both datasets, configure and display the plot
plt.title('Comparison of NTGA2 and NSGAII methods')
plt.xlabel('Duration')
plt.ylabel('Cost')
plt.legend()  # This adds the legend to the plot

plt.show()
plt.close()
