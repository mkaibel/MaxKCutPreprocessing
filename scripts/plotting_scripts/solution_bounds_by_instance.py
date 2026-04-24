from .tables_etc import *

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os
from pathlib import Path
from matplotlib.lines import Line2D

# Create the 'plots' directory if it doesn't exist
os.makedirs('plots', exist_ok=True)

# Load the CSV file
df = pd.read_csv('experimental_results/experiments_basic.csv')

df['instance'] = df['instance'].apply(lambda x: Path(x).stem)
df['gap'] = df['upper_bound'] / df['lower_bound']

# TODO update true to 1 once a full rerun of experiments happened
# df = df[df['found_optimum'] == True]


# Group by k, instance, usedDataReducer, and usedExtraConstraints, then calculate the mean runtime
grouped = df.groupby(
    ['k', 'instance', 'used_data_reducer', 'used_extra_constraints'], as_index=False).agg({
    'lower_bound': 'mean',
    'upper_bound': 'mean',
    'gap': 'mean'
})

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
    sns.lineplot(
        data=df_instance,
        x='k',
        y='upper_bound',
        hue='custom_label',
        palette=custom_palette,
        markers=custom_markers,
        style='custom_label',
        dashes=[(2,2),(2,2),(2,2),(2,2),(2,2),(2,2),],
    )
    sns.lineplot(
        data=df_instance,
        x='k',
        y='lower_bound',
        hue='custom_label',
        palette=custom_palette,
        markers=custom_markers,
        style='custom_label',
        dashes=[(1,0),(1,0),(1,0),(1,0),(1,0),(1,0),],
    )

    legend_elements = [
        Line2D([0], [0], color='black', linestyle='-', label='Lower Bound'),
        Line2D([0], [0], color='black', linestyle='--', label='Upper Bound'),
        Line2D([0], [0], marker='o', color='magenta', label='Reducer & Constraints'),
        Line2D([0], [0], marker='H', color='blue', label='Reducer Only'),
        Line2D([0], [0], marker='^', color='green', label='Naive & Constraints'),
        Line2D([0], [0], marker='v', color='black', label='Naive Only'),
        Line2D([0], [0], marker='s', color='orange', label='Constraints Only'),
        Line2D([0], [0], marker='D', color='purple', label='Straight Optimizer'),
    ]
    plt.title(f'Lower and Upper Bounds on {instance}')
    plt.xlabel('k')
    plt.yscale('log')
    plt.ylabel('Upper and lower bound')
    plt.legend(title='Parameter Combinations',handles=legend_elements)
    plt.grid(True)
    # plt.show()

    # Save the plot as an SVG file
    plt.savefig(f'plots/solution_bounds_{instance}.pdf', format='pdf', dpi=300, bbox_inches='tight')
    plt.close()  # Close the figure to free memory

    # Plot gap
    plt.figure(figsize=(10, 6))
    sns.lineplot(
        data=df_instance,
        x='k',
        y='gap',
        hue='custom_label',
        palette=custom_palette,
        markers=custom_markers,
    )
    plt.title(f'Average Integrality Gap on {instance}')
    plt.xlabel('k')
    plt.ylabel('Integrality Gap')
    plt.legend()
    plt.grid(True)

    plt.savefig(f'plots/integrality_gap_{instance}.pdf', format='pdf', dpi=300, bbox_inches='tight')
    plt.close()  # Close the figure to free memory
