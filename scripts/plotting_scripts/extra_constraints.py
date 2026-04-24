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
df = pd.read_csv('experimental_results/experiments_stats.csv')

df['instance'] = df['instance'].apply(lambda x: Path(x).stem)

# TODO update true to 1 once a full rerun of experiments happened
# df = df[df['found_optimum'] == True]

# Get unique values of instances
instances = df['instance'].unique()

for instance in instances:
    # Filter data for the current value of k
    df_instance = df[df['instance'] == instance]

    # Create the plot
    plt.figure(figsize=(10, 6))
    sns.lineplot(
        data=df_instance,
        x='k',
        y='m',
        hue='type',
        palette=['purple'],
        style='type',
        markers=[' '],
        dashes=[(2,2)],
    )
    sns.lineplot(
        data=df_instance,
        x='k',
        y='numConstraints',
        hue='type',
        palette=['magenta'],
        style='type',
        markers=['o'],
        dashes=[(1,0)],
    )
    sns.lineplot(
        data=df_instance,
        x='k',
        y='mKernels',
        hue='type',
        palette=['orange'],
        style='type',
        markers='^',
        dashes=[(2,2)],
    )
    sns.lineplot(
        data=df_instance,
        x='k',
        y='numKernelConstraints',
        hue='type',
        palette=['blue'],
        style='type',
        markers='x',
        dashes=[(1,0)],
    )

    legend_elements = [
        Line2D([0], [0], marker=' ', color='purple', linestyle=(0,(2, 2)), label='Num edges whole graph'),
        Line2D([0], [0], marker='o', color='magenta', linestyle=(0, (1,0)), label='Num constraints whole graph'),
        Line2D([0], [0], marker='x', color='orange', linestyle=(0,(2, 2)), label='Num edges kernels'),
        Line2D([0], [0], marker='^', color='blue', linestyle=(0,(1, 0)), label='Num constraints kernel'),
    ]
    # plt.axhline(y=300, color='red', linestyle='--', linewidth=1)
    # plt.text(x=4.5, y=300, s='Timeout', color='red', verticalalignment='bottom', horizontalalignment='right', fontsize=12, fontweight='bold')
    plt.title(f'Number of Extra Constraints by k on {instance}')
    plt.xlabel('k')
    plt.ylabel('Kernel Sizes')
    plt.legend(title='Parameter Combinations',handles=legend_elements)
    plt.grid(True)
    # plt.show()

    # Save the plot as an SVG file
    plt.savefig(f'plots/extra_constraints_{instance}.pdf', format='pdf', dpi=300, bbox_inches='tight')
    plt.close()  # Close the figure to free memory
