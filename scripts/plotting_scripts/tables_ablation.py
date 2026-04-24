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

def get_winner_number(table, prepro):
    return int(table.loc[table['prepro_config'] == prepro, 'runtime'].iloc[0])

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
    df = pd.read_csv('experimental_results/experiments_ablation.csv')

    # Change runtime from miliseconds to seconds
    df['runtime'] = df['runtime'].astype(float)
    df['runtime'] = df['runtime'] / 1000.0
    df['runtime'] = df['runtime'].apply(lambda x: normalize_runtime(x)) # Gurobi sometimes goes a few seconds over the timeout and we want to cover this

    df = df.groupby(
        ['instance', 'k', 'cne', 'cnt', 'csv', 'clq', 'tlc', 'tlcs', 'spqr'
         ], as_index=False
    ).agg({
        'runtime': 'mean',
        'found_optimum': 'mean',
        'lower_bound':'mean',
        'upper_bound':'mean',
    })
    df['label'] = df['cne'].astype(str) + ', ' + df['cnt'].astype(str) + ', ' + df['csv'].astype(str) + ', ' +  df['clq'].astype(str) + ', ' + df['tlc'].astype(str) + ', ' + df['tlcs'].astype(str) + ', ' + df['spqr'].astype(str)
    df['prepro_config'] = df['label'].map(custom_names_ablation)


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
    df_stats['Vperc'] = df_stats['nKernels'].astype(float) / df_stats['n'].astype(float)
    df_stats['Eperc'] = df_stats['mKernels'].astype(float) / df_stats['m'].astype(float)

    k_vals = sorted(df_stats['k'].unique())

    for k in k_vals:
        df_kval = df[df['k'] == k]
        df_prepro_kval = df_stats[df_stats['k'] == k]

        print(f"\\midrule \n \\multirow{{ {len(instsets) - 1} }}{{*}}{{ {k} }}", end='')

        for instset in instsets:
            if instset == 'hard':
                continue

            df_instset = df_kval[df_kval['instance'].isin(instances[instset])]
            ablation_set = df_instset[~df_instset['prepro_config'].isin(['All', 'SimplePreprocessing'])]
            df_prepro_instset = df_prepro_kval[df_prepro_kval['instance'].isin(instances[instset])]

            winners_df = (
                ablation_set
                .groupby(['instance', 'k'], group_keys=False)
                .apply(get_winners)
            )

            winners_count = winners_df['prepro_config'].value_counts()

            wins = [1000, 1000, int(winners_count.get('NoDominatingEdges',0)), int(winners_count.get('NoCliques',0)), int(winners_count.get('NoBiconnector',0)), int(winners_count.get('NoTwoLayer',0)), int(winners_count.get('NoTwoLayerSmallSide',0))]
            winsmin = min(wins)

            df_reduction = df_prepro_instset.groupby(['prepro_config'], as_index=False).agg({'Vperc':'mean', 'Eperc':'mean'})
            vperc = [0.0, 0.0, getReductionPerc(df_reduction, 'NoDominatingEdges', 'Vperc'), getReductionPerc(df_reduction, 'NoCliques', 'Vperc'), getReductionPerc(df_reduction, 'NoBiconnector', 'Vperc'), getReductionPerc(df_reduction, 'NoTwoLayer', 'Vperc'), getReductionPerc(df_reduction, 'NoTwoLayerSmallSide', 'Vperc')]
            vpercmax = max(vperc)
            eperc = [0.0, 0.0, getReductionPerc(df_reduction, 'NoDominatingEdges', 'Eperc'), getReductionPerc(df_reduction, 'NoCliques', 'Eperc'), getReductionPerc(df_reduction, 'NoBiconnector', 'Eperc'), getReductionPerc(df_reduction, 'NoTwoLayer', 'Eperc'), getReductionPerc(df_reduction, 'NoTwoLayerSmallSide', 'Eperc')]
            epercmax = max(eperc)

            print(f" & {instset} & {getReductionPerc(df_reduction, 'All', 'Eperc'):.2f} & {getReductionPerc(df_reduction, 'SimplePreprocessing', 'Eperc'):.2f} & {mark_if_best(eperc[2], epercmax)} & {mark_if_best(wins[2], winsmin)} & {mark_if_best(eperc[3], epercmax)} & {mark_if_best(wins[3], winsmin)} & {mark_if_best(eperc[4], epercmax)} & {mark_if_best(wins[4], winsmin)} & {mark_if_best(eperc[5], epercmax)} & {mark_if_best(wins[5], winsmin)} & {mark_if_best(eperc[6], epercmax)} & {mark_if_best(wins[6], winsmin)} \\\\")