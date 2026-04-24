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

df['instance'] = df['instance'].apply(lambda x: Path(x).stem)

# Change runtime from miliseconds to seconds
df['runtime'] = df['runtime'].astype(float)
df['runtime'] = df['runtime'] + 1
df['runtime'] = df['runtime'] / 1000.0

grouped = df

# Create a unique label for each combination of usedDataReducer and usedExtraConstraints
grouped['label'] = grouped['used_data_reducer'].astype(str) + ', ' + grouped['used_extra_constraints'].astype(str)

# (Optional) Map the 'label' to custom names
grouped['custom_label'] = grouped['label'].map(custom_names)

# Get unique values of k
k_values = grouped['k'].unique()

for small in {True, False}:
    for k in k_values:
        # Filter data for the current value of k
        df_k = grouped[grouped['k'] == k]

        if (small):
            df_k = df_k[df_k['instance'].isin(small_instances)]
        else:
            df_k = df_k[~df_k['instance'].isin(small_instances)]

        # Create the plot
        plt.figure(figsize=(10, 6))
        sns.barplot(
            data=df_k,
            x='instance',
            y='runtime',
            hue='custom_label',
            palette=custom_palette,
            # legend=False,
            # fill=False
            )
        
        plt.axhline(y=300, color='red', linestyle='--', linewidth=1)
        plt.text(x=4.5, y=timeout, s='Timeout', color='red', verticalalignment='bottom', horizontalalignment='right', fontsize=12, fontweight='bold')
        if small:
            plt.title(f'Runtimes per Instance for k = {k} on easy instances')
        else:
            plt.title(f'Runtimes per Instance for k = {k} on hard instances')
        plt.xlabel('Instance')
        plt.xticks(rotation=45)
        plt.yscale('log')
        plt.ylabel('Average Runtime (s)')
        plt.legend(title='Parameter Combinations')
        plt.grid(True)
        # plt.show()
    
        # Save the plot as an SVG file
        if small:
            plt.savefig(f'plots/average_runtime_k_{k}_easy.pdf', format='pdf', dpi=300, bbox_inches='tight')
        else:
            plt.savefig(f'plots/average_runtime_k_{k}_hard.pdf', format='pdf', dpi=300, bbox_inches='tight')
        plt.close()  # Close the figure to free memory
