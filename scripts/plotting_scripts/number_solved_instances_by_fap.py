from .tables_etc import *

import pandas as pd

def get_num_solved(table, config):
    return float(table.loc[table['name'] == config, 'found_optimum'].iloc[0])

def run():
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

    df_instset = df_agg[df_agg['instance'].isin(instances['fap'])]
    numInstances = len(sorted(df_instset['instance'].unique()))

    k_vals = sorted(df_instset['k'].unique())

    for k in k_vals:
        df_for_k = df_instset[df_instset['k'] == k]

        print(f"For k = {k}")

        for subset in ['CELAR', 'DUTtest', 'SURPRISE']:
            df_subset = df_for_k[df_for_k['instance'].str.startswith(subset)]

            df_subset = df_subset.groupby(['k', 'name'], as_index=False).agg({'found_optimum':'sum'})

            print(f"{subset}: none: {get_num_solved(df_subset, 'none')} naive: {get_num_solved(df_subset, 'naive')} our: {get_num_solved(df_subset, 'our')}")