from .tables_etc import *

import pandas as pd

import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
import seaborn as sns

import os

fontsize=24

# Create the 'plots' directory if it doesn't exist
os.makedirs('plots', exist_ok=True)

def run():
    plt.rcParams.update({'font.size': fontsize})
    plt.style.use('ggplot')

    # Load the CSV file
    df = pd.read_csv('experimental_results/experiments_basic.csv')
    df = df[df['used_extra_constraints'] == 0] # Filter out extra constraint runs as they don't go into the paper

    # Change runtime from miliseconds to seconds
    df['runtime'] = df['runtime'].astype(float)
    df['runtime'] = df['runtime']
    df['runtime'] = df['runtime'] / 1000.0
    df['runtime'] = df['runtime'].apply(lambda x: min(x, timeout)) # Gurobi sometimes goes a few seconds over the timeout and we want to cover this

    df_agg = df.groupby(['instance', 'k', 'used_data_reducer'], as_index=False).agg({
        'runtime': 'mean',
        'found_optimum': 'mean',
        'lower_bound': 'mean',
        'upper_bound': 'mean'
    })

    df_agg['name'] = df_agg['used_data_reducer'].map(paper_names)

    first = True

    for sets in [['easy', 'medium'], ['fap', 'torus']]:
        fig, axes = plt.subplots(1, 2, figsize=(25, 7))

        handles = None
        labels = None

        for i, ax in enumerate(axes):
            print(f"Running for {sets[i]}\n\n--------------------\n\n")
            df_instset = df_agg[df_agg['instance'].isin(instances[sets[i]])]
            numInstances = len(sorted(df_instset['instance'].unique()))

            df_instset = df_instset.groupby(['k', 'name']).agg({'found_optimum':'sum'})

            sns.barplot(
                data=df_instset,
                x='k',
                y='found_optimum',
                hue='name',
                palette=paper_palette,
                #markers=paper_markers,
                #style='name',
                edgecolor='white',
                ax=ax,
            )

            ax.axhline(y=numInstances, color='black', linestyle='--', linewidth=1)
            ax.text(x=3.5, y=numInstances, s='All', color='black', verticalalignment='bottom', horizontalalignment='right', fontsize=fontsize)

            ax.set_title(f"Solved instances by k on {sets[i]} instances", fontsize=fontsize)
            ax.set_xlabel('k', fontsize=fontsize)
            ax.set_ylim(0, numInstances * 1.11)
            ax.set_ylabel('#Solved Instances',fontsize=fontsize)
            ax.tick_params(axis='both', labelsize=fontsize)
            ax.legend(title='Preprocessing',fontsize=fontsize, title_fontsize=fontsize)

            if handles is None:
                handles, labels = ax.get_legend_handles_labels()

        for ax in axes:
            ax.legend().remove()

        plt.gca().yaxis.grid(True)
        plt.gca().yaxis.set_major_locator(MaxNLocator(integer=True))
        plt.gca().set_axisbelow(True)

        if first:
            fig.legend(handles, labels, title='Preprocessing', loc="center", bbox_to_anchor=(0.5, 0.5), ncol=1, fontsize=fontsize, title_fontsize=fontsize)
            first=False

        plt.subplots_adjust(wspace=0.55)

        plt.savefig(f'plots/solved_instances_by_k_{sets[0]}_{sets[1]}.pdf', format='pdf', dpi=300, bbox_inches='tight')


