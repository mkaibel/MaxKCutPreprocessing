from .tables_etc import *

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os
from pathlib import Path

custom_lines_dashed = [(2, 2), (2, 2), (2, 2), (2, 2), (2, 2), (2, 2)]

# Create the 'plots' directory if it doesn't exist
os.makedirs('plots', exist_ok=True)

# Load the CSV file
df = pd.read_csv('experimental_results/experiments_basic.csv')

df['instance'] = df['instance'].apply(lambda x: Path(x).stem)

# Filter for rows where foundOptimum is 1
df_optimum = df[df['found_optimum'] == 1]

for small in {True, False}:
    df_filter = df_optimum
    if (small):
        df_filter = df_filter[df_filter['instance'].isin(small_instances)]
    else:
        df_filter = df_filter[~df_filter['instance'].isin(small_instances)]

    # Group by k, usedDataReducer, and usedExtraConstraints, then count
    grouped = df_filter.groupby(
        ['k', 'used_data_reducer', 'used_extra_constraints']
    ).size().reset_index(name='count')

    # Create a unique label for each combination of usedDataReducer and usedExtraConstraints
    grouped['label'] = grouped['used_data_reducer'].astype(str) + ', ' + grouped['used_extra_constraints'].astype(str)

    # (Optional) Map the 'label' to custom names
    # Example: Replace with your own mapping
    grouped['custom_label'] = grouped['label'].map(custom_names)

    # Plot
    plt.figure(figsize=(10, 6))
    sns.lineplot(
        data=grouped,
        x='k',
        y='count',
        hue='custom_label',
        palette=custom_palette,
        markers=custom_markers,
        style='custom_label',
        dashes=custom_lines_dashed,
    )
    if small:
        plt.title('Number of Optimum Found by k on Small Instances')
    else:
        plt.title('Number of Optimum Found by k on Large Instances')
    plt.xlabel('k')
    plt.ylim(0, None)
    plt.ylabel('Number of solved instances')
    plt.legend(title='used_data_reducer, used_extra_constraints')
    plt.grid(True)
    # plt.show()


    # Save the plot as an SVG file
    if small:
        plt.savefig(f'plots/solved_instances_by_k_small.pdf', format='pdf', dpi=300, bbox_inches='tight')
    else:
        plt.savefig(f'plots/solved_instances_by_k_large.pdf', format='pdf', dpi=300, bbox_inches='tight')
        
    plt.close()  # Close the figure to free memory
