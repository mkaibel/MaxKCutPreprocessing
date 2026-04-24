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
            return f"\\textbf{{{val:.2f}}}"
        return f"\\textbf{{{val}}}"
    if type(val) is float:
        return f"{val:.2f}"
    return val
def run():
    # Load the CSV file
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
    df_stats['VpercMin'] = df_stats['Vperc']
    df_stats['EpercMin'] = df_stats['Eperc']

    for instset in instsets:
        print(f"Running for {instset}\n\n--------------------\n\n")

        df_prepro_instset = df_stats[df_stats['instance'].isin(instances[instset])]

        k_vals = sorted(df_prepro_instset['k'].unique())

        for k in k_vals:
            df_reduction = df_prepro_instset[df_prepro_instset['k'] == k].groupby(['prepro_config'], as_index=False).agg({'Vperc':'mean', 'Eperc':'mean', 'VpercMin':'min', 'EpercMin':'min'})
            vperc_avg = [100.0, getReductionPerc(df_reduction, 'All', 'Vperc'), getReductionPerc(df_reduction, 'SimplePreprocessing', 'Vperc')]
            vperc_avg_min = min(vperc_avg)
            eperc_avg = [100.0, getReductionPerc(df_reduction, 'All', 'Eperc'), getReductionPerc(df_reduction, 'SimplePreprocessing', 'Eperc')]
            eperc_avg_min = min(eperc_avg)

            vperc_min = [100.0, getReductionPerc(df_reduction, 'All', 'VpercMin'), getReductionPerc(df_reduction, 'SimplePreprocessing', 'VpercMin')]
            vperc_min_min = min(vperc_min)
            eperc_min = [100.0, getReductionPerc(df_reduction, 'All', 'EpercMin'), getReductionPerc(df_reduction, 'SimplePreprocessing', 'EpercMin')]
            eperc_min_min = min(eperc_min)

            print(f"{k} & {mark_if_best(vperc_avg[2], vperc_avg_min)} & {mark_if_best(vperc_min[2], vperc_min_min)} & {mark_if_best(eperc_avg[2], eperc_avg_min)} & {mark_if_best(eperc_min[2], eperc_min_min)} & {mark_if_best(vperc_avg[1], vperc_avg_min)} & {mark_if_best(vperc_min[1], vperc_min_min)} & {mark_if_best(eperc_avg[1], eperc_avg_min)} & {mark_if_best(eperc_min[1], eperc_min_min)} \\\\")

        print("\n--------------------\n\n")