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
    return int(table.loc[table['prepro_config'] == prepro, 'runtimePreprocessor'].iloc[0])


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

def get_objective_value(table, prepro):
    return int(table.loc[table['used_data_reducer'] == prepro, 'objective_value'].iloc[0])

def mark_if_best(val, best):
    if val == best:
        if type(val) is float:
            return f"\\textbf{{{val:,.2f}}}"
        return f"\\textbf{{{val:,}}}"
    if type(val) is float:
        return f"{val:,.2f}"
    return f"{val:,}"

def run():
    # Load the CSV file
    df = pd.read_csv('experimental_results/experiments_heuristic.csv')

    df_agg = df.groupby(['instance', 'k', 'used_data_reducer'], as_index=False).agg({
        'objective_value': 'mean',
    })

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
    df_stats['runtimePreprocessor'] = df_stats['runtimePreprocessor'].astype(float) / 1000.0

    for instset in instsets:
        print(f"Running for {instset}\n\n--------------------\n\n")

        df_instset = df_agg[df_agg['instance'].isin(instances[instset])]
        df_prepro_instset = df_stats[df_stats['instance'].isin(instances[instset])]

        k_vals = sorted(df_instset['k'].unique())

        for k in k_vals:
            df_runs = df_instset[df_instset['k'] == k]
            df_prepro_stats = df_prepro_instset[df_prepro_instset['k'] == k]

            graphs = sorted(df_runs['instance'].unique())

            print(f"\\midrule \n \\multirow{{ {len(graphs)} }}{{*}}{{ {k} }}", end='')

            for graph in graphs:
                df_final = df_runs[df_runs['instance'] == graph]

                objVals = [get_objective_value(df_final, 0), get_objective_value(df_final, 1), get_objective_value(df_final, 2)]
                max_gain = max(objVals[1] - objVals[0], objVals[2] - objVals[0])

                df_reduction = df_prepro_stats[df_prepro_stats['instance'] == graph]

                vperc = [100.0, getReductionPerc(df_reduction, 'All', 'Vperc'),
                         getReductionPerc(df_reduction, 'SimplePreprocessing', 'Vperc')]
                vpercmin = min(vperc)
                eperc = [100.0, getReductionPerc(df_reduction, 'All', 'Eperc'),
                 getReductionPerc(df_reduction, 'SimplePreprocessing', 'Eperc')]
                epercmin = min(eperc)

                times = [0, get_runtime(df_reduction, 'All'), get_runtime(df_reduction, 'SimplePreprocessing')]

                graph = graph.split('.')[0]

                print(f" & {graph} & {objVals[0]:,} & {mark_if_best(vperc[2], vpercmin)} & {mark_if_best(eperc[2], epercmin)} & {mark_if_best(objVals[2] - objVals[0], max_gain)} & {times[2]:,} & {mark_if_best(vperc[1], vpercmin)} & {mark_if_best(eperc[1], epercmin)} & {mark_if_best(objVals[1] - objVals[0], max_gain)} & {times[1]:,} \\\\")

        print("\n--------------------\n\n")
