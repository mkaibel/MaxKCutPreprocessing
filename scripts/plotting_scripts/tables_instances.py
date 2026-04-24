from .tables_etc import *

pathRaw = "instances/instances_clean/"

def order(a, b):
    if a < b:
        return a, b
    else:
        return b, a

def loadGraph(instance):
    numNodes = 0
    edges = []
    lines = open(pathRaw + instance, 'r').readlines()

    for line in lines:
        items = line.rstrip().split()

        if len(items) == 0 or items[0].startswith("#"):
            continue

        if items[0] == items[1]:
            continue
        u, v = order(int(items[0]), int(items[1]))
        edges.append([u, v, float(items[2])])

    for edge in edges:
        numNodes = max(numNodes, edge[0], edge[1])

    return edges, (numNodes + 1)

def get_graph_stats(name):
    edges, numNodes = loadGraph(name)
    minWeight, maxWeight = 100000000.0, 0.0
    minDegree, maxDegree = numNodes, 0
    degree = [0] * numNodes

    for edge in edges:
        minWeight = min(minWeight, edge[2])
        maxWeight = max(maxWeight, edge[2])

        degree[edge[0]] += 1
        degree[edge[1]] += 1

    for v in range(numNodes):
        if degree[v] == 0:
            numNodes -= 1
        else:
            minDegree = min(minDegree, degree[v])
            maxDegree = max(maxDegree, degree[v])

    return name, numNodes, len(edges), minDegree, maxDegree, minWeight, maxWeight

def print_weight(weight):
    if weight.is_integer():
        return f"{int(weight):,}".replace(',', '.')
    return f"{weight:,.2f}"


def printTables():
    print("\\begin{table}[h!]")
    print("    \\centering")
    print("    \\begin{tabular}{llcccccc}")
    print("        \\toprule")
    print("        Group & Instance & $n$ & $m$ & $\\underline{d}$ & $\\overline{d}$ & $\\underline{w}$ & $\\overline{w}$ \\\\")

    for instset in instsets:
        print("        \\midrule")
        print(f"        \\multirow{{ {len(instances[instset])} }}{{*}}{{ {instset} }}", end='')

        graphStats = []

        for graph in instances[instset]:
            graphStats.append(get_graph_stats(graph))

        graphStats.sort(key=lambda stat : (stat[1], stat[0]))

        for stat in graphStats:
            print(f"         & {stat[0].removesuffix('.snap').replace('_', '\_')} & {stat[1]:,} & {stat[2]:,} & {stat[3]:,} & {stat[4]:,} & {print_weight(stat[5])} & {print_weight(stat[6])} \\\\")

    print("    \\bottomrule")
    print("    \\end{tabular}")
    print("    \\caption{Overview of Instances used. $\\underline{d}, \\overline{d}, \\underline{w}$ and $\\overline{w}$ are the minimum and maximum degree and weight respectively.}")
    print("    \\label{tab:instance_overview}")
    print("\\end{table}")

def run():
    printTables()

