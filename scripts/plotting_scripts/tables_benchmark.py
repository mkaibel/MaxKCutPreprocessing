from .tables_etc import *

import pandas as pd


def get_winners(group):
    max_found = group['found_optimum'].max()
    candidates = group[group['found_optimum'] == max_found]

    min_gap = (candidates['upper_bound'] / candidates['lower_bound']).min()
    candidates = candidates[candidates['upper_bound'] / candidates['lower_bound'] == min_gap]

    min_time = candidates['runtime'].min()

    winners = candidates[candidates['runtime'] == min_time]

    return winners

def get_runtime(table, prepro):
    return int(table.loc[table['used_data_reducer'] == prepro, 'runtime'].iloc[0])

def getReductionPerc(table, config, val):
    return float(table.loc[table['prepro_config'] == config, val].iloc[0]) * 100.0

def mark_if_best(val, best):
    if val == best:
        if type(val) is float:
            return f"\\textbf{{{val:,.2f}}}"
        return f"\\textbf{{{val:,}}}"
    if type(val) is float:
        return f"{val:,.2f}"
    return f"{val:,}"

def normalize_runtime(time):
    if time >= timeout:
        return timeout
    return time

def run():
    # Load the CSV file
    df = pd.read_csv('experimental_results/experiments_basic.csv')
    df = df[df['used_extra_constraints'] == 0] # Filter out extra constraint runs as they don't go into the paper

    df_stats = pd.read_csv('experimental_results/experiments_stats.csv')
    df_stats = df_stats.groupby(
        ['instance', 'k', 'cne', 'cnt', 'csv', 'clq', 'tlc', 'tlcs', 'spqr'
         ], as_index=False
    ).agg({
        'runtimePreprocessor': 'mean',
        'nKernels': 'mean',
        'mKernels': 'mean',
        'n':'mean',
        'm':'mean',
    })
    df_stats['label'] = df_stats['cne'].astype(str) + ', ' + df_stats['cnt'].astype(str) + ', ' + df_stats['csv'].astype(str) + ', ' +  df_stats['clq'].astype(str) + ', ' + df_stats['tlc'].astype(str) + ', ' + df_stats['tlcs'].astype(str) + ', ' + df_stats['spqr'].astype(str)
    df_stats['prepro_config'] = df_stats['label'].map(custom_names_ablation)
    df_stats = df_stats[df_stats['prepro_config'].isin(['All', 'SimplePreprocessing'])]
    df_stats['Vperc'] = df_stats['nKernels'].astype(float) / df_stats['n'].astype(float)
    df_stats['Eperc'] = df_stats['mKernels'].astype(float) / df_stats['m'].astype(float)

    # Change runtime from miliseconds to seconds
    df['runtime'] = df['runtime'].astype(float)
    df['runtime'] = df['runtime'] / 1000.0
    df['runtime'] = df['runtime'].apply(lambda x: normalize_runtime(x)) # Gurobi sometimes goes a few seconds over the timeout and we want to cover this

    df_agg = df.groupby(['instance', 'k', 'used_data_reducer'], as_index=False).agg({
        'runtime': 'mean',
        'found_optimum': 'mean',
        'lower_bound': 'mean',
        'upper_bound': 'mean'
    })

    for instset in instsets:
        print(f"Running for {instset}\n\n--------------------\n\n")

        df_instset = df_agg[df_agg['instance'].isin(instances[instset])]
        df_prepro_instset = df_stats[df_stats['instance'].isin(instances[instset])]

        k_vals = sorted(df_instset['k'].unique())

        for k in k_vals:
            winners_df = (
                df_instset[df_instset['k'] == k]
                .groupby(['instance', 'k'], group_keys=False)
                .apply(get_winners)
            )

            winners_count = winners_df['used_data_reducer'].value_counts().sort_index()

            wins = [int(winners_count.get(0,0)), int(winners_count.get(1,0)), int(winners_count.get(2,0))]
            winsmax = max(wins)

            df_time = df_instset[df_instset['k'] == k].groupby(['used_data_reducer'], as_index=False).agg({'runtime':'mean'})
            times =[get_runtime(df_time, 0), get_runtime(df_time, 1), get_runtime(df_time, 2)]
            timesmin = min(times)

            df_reduction = df_prepro_instset[df_prepro_instset['k'] == k].groupby(['prepro_config'], as_index=False).agg({'Vperc':'mean', 'Eperc':'mean'})
            vperc = [100.0, getReductionPerc(df_reduction, 'All', 'Vperc'), getReductionPerc(df_reduction, 'SimplePreprocessing', 'Vperc')]
            vpercmin = min(vperc)
            eperc = [100.0, getReductionPerc(df_reduction, 'All', 'Eperc'), getReductionPerc(df_reduction, 'SimplePreprocessing', 'Eperc')]
            epercmin = min(eperc)

            print(f"{k} & {mark_if_best(wins[0], winsmax)} & {mark_if_best(times[0], timesmin)} & {mark_if_best(vperc[2], vpercmin)} & {mark_if_best(eperc[2], epercmin)} & {mark_if_best(wins[2], winsmax)} & {mark_if_best(times[2], timesmin)} & {mark_if_best(vperc[1], vpercmin)} & {mark_if_best(eperc[1], epercmin)} & {mark_if_best(wins[1], winsmax)} & {mark_if_best(times[1], timesmin)} \\\\")

        print("\n--------------------\n\n")