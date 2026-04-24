from .tables_etc import *

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os
from pathlib import Path

# Create the 'plots' directory if it doesn't exist
os.makedirs('plots', exist_ok=True)

# Load the CSV file
df = pd.read_csv('experimental_results/experiments_basic.csv')

# Change runtime from miliseconds to seconds
df['runtime'] = df['runtime'].astype(float)
df['runtime'] = df['runtime'] + 1
df['runtime'] = df['runtime'] / 1000.0

df['instance'] = df['instance'].apply(lambda x: Path(x).stem)

# TODO update true to 1 once a full rerun of experiments happened
# df = df[df['found_optimum'] == True]


# Group by k, instance, usedDataReducer, and usedExtraConstraints, then calculate the mean runtime
# grouped = df.groupby(
#     ['k', 'instance', 'used_data_reducer', 'used_extra_constraints']
# )['runtime'].mean().reset_index()
grouped = df

# Create a unique label for each combination of usedDataReducer and usedExtraConstraints
grouped['label'] = grouped['used_data_reducer'].astype(str) + ', ' + grouped['used_extra_constraints'].astype(str)

# (Optional) Map the 'label' to custom names
grouped['custom_label'] = grouped['label'].map(custom_names)

# Get unique values of instances
instances = grouped['instance'].unique()

for instance in instances:
    # Filter data for the current value of k
    df_instance = grouped[grouped['instance'] == instance]

    # Create the plot
    plt.figure(figsize=(10, 6))
    sns.barplot(
        data=df_instance,
        x='k',
        y='runtime',
        hue='custom_label',
        palette=custom_palette,
        # legend=False,
        # fill=False
        )

    plt.axhline(y=300, color='red', linestyle='--', linewidth=1)
    plt.text(x=4.5, y=timeout, s='Timeout', color='red', verticalalignment='bottom', horizontalalignment='right', fontsize=12, fontweight='bold')
    plt.title(f'Runtimes per on {instance}')
    plt.xlabel('k')
    plt.yscale('log')
    plt.ylabel('Average Runtime (s)')
    plt.legend(title='Parameter Combinations')
    plt.grid(True)
    # plt.show()

    # Save the plot as an SVG file
    plt.savefig(f'plots/average_runtime_{instance}.pdf', format='pdf', dpi=300, bbox_inches='tight')
    plt.close()  # Close the figure to free memory
