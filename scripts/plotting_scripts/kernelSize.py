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

# Get unique values of instances
instances = df['instance'].unique()

for instance in instances:
    # Filter data for the current value of k
    df_instance = df[df['instance'] == instance]

    # Create the plot
    plt.figure(figsize=(10, 6))
    # sns.lineplot(
    #     data=df_instance,
    #     x='k',
    #     y='n',
    #     hue='type',
    #     palette=['purple'],
    #     style='type',
    #     markers=['o'],
    #     dashes=[(2,4)],
    # )
    # sns.lineplot(
    #     data=df_instance,
    #     x='k',
    #     y='m',
    #     hue='type',
    #     palette=['purple'],
    #     style='type',
    #     markers=['^'],
    #     dashes=[(2,4)],
    # )
    sns.lineplot(
        data=df_instance,
        x='k',
        y='nMaxKernel',
        hue='type',
        palette=['blue'],
        style='type',
        markers=['o'],
        dashes=[(1,0)],
    )
    sns.lineplot(
        data=df_instance,
        x='k',
        y='mMaxKernel',
        hue='type',
        palette=['blue'],
        style='type',
        markers='^',
        dashes=[(1,0)],
    )
    sns.lineplot(
        data=df_instance,
        x='k',
        y='mKernels',
        hue='type',
        palette=['magenta'],
        style='type',
        markers='o',
        dashes=[(2,2)],
    )
    sns.lineplot(
        data=df_instance,
        x='k',
        y='nKernels',
        hue='type',
        palette=['magenta'],
        style='type',
        markers=['^'],
        dashes=[(2,2)],
    )

    legend_elements = [
        # Line2D([0], [0], marker='^', color='purple', linestyle=(0,(2, 4)), label='n Whole Graph'),
        # Line2D([0], [0], marker='o', color='purple', linestyle=(0,(2, 4)), label='m Whole Graph'),
        Line2D([0], [0], marker='^', color='magenta', linestyle=(0, (2,2)), label='n Kernels Total'),
        Line2D([0], [0], marker='o', color='magenta', linestyle=(0, (2,2)), label='m Kernels Total'),
        Line2D([0], [0], marker='^', color='blue', linestyle=(0,(1, 0)), label='n Max Kernel'),
        Line2D([0], [0], marker='o', color='blue', linestyle=(0,(1, 0)), label='m Max Kernel'),
    ]
    plt.title(f'Kernel Sizes by k on {instance}')
    plt.xlabel('k')
    plt.ylabel('Kernel Sizes')
    plt.legend(title='Parameter Combinations',handles=legend_elements)
    plt.grid(True)
    # plt.show()

    # Save the plot as an SVG file
    plt.savefig(f'plots/kernel_sizes_{instance}.pdf', format='pdf', dpi=300, bbox_inches='tight')
    plt.close()  # Close the figure to free memory
