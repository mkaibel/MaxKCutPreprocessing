pathRaw = "instances/instances_clean/"

names = ["soc-firm", "ca-netscience", "bio-diseasome", 'bio-celegans', 'bio-DM-LC', "rt-twitter-copen",
	 "g001207", "g000981", "g000292", "g000302", "g001918", "g000677", "g001075", "g000087",
      "imgseg_35058", "imgseg_374020", "imgseg_105019",
 ]

small_instances = {'g000292', 'g000981', 'g000302', 'g001207', 'g001918', 'soc-firm', 'imgseg_105019', 'rt-twitter-copen', 'bio-DM-LC'}

def order(a, b):
    if a < b:
        return a, b
    else:
        return b, a

def loadGraph(instance):
    numNodes = 0
    edges = []
    lines = open(pathRaw + instance + '.snap', 'r').readlines()

    for line in lines:
        items = line.rstrip().split()

        if len(items) == 0 or items[0].startswith("#"):
            continue

        if items[0] == items[1]:
            continue
        u, v = order(int(items[0]), int(items[1]))
        edges.append([u - 1, v - 1, float(items[2])])

    for edge in edges:
        numNodes = max(numNodes, edge[0], edge[1])

    return edges, numNodes + 1

def addInstanceStats(table, instance, n, m, deg_min, deg_max, weight_min, weight_max):
    table += "         & " + instance + " & $" + str(n) + "$ & $" + str(m) + "$ & $" + str(deg_min) + "$ & $" + str(deg_max) + "$ & $" + str(weight_min) + "$ & $" + str(weight_max) + "$ \\\\\n"
    return table

def printTables():
    table_easy = ""
    table_hard = ""
    numEasy, numHard = 0, 0

    for instance in names:
        edges, numNodes = loadGraph(instance)
        minWeight, maxWeight = 100000000.0, 0.0
        minDegree, maxDegree = numNodes, 0
        degree = [0] * numNodes

        for edge in edges:
            minWeight = min(minWeight, edge[2])
            maxWeight = max(maxWeight, edge[2])

            degree[edge[0]] += 1
            degree[edge[1]] += 1

        for v in range(numNodes):
            minDegree = min(minDegree, degree[v])
            maxDegree = max(maxDegree, degree[v])

        if instance in small_instances:
            numEasy += 1
            table_easy = addInstanceStats(table_easy, instance, numNodes, len(edges), minDegree, maxDegree, minWeight, maxWeight)
        else:
            numHard += 1
            table_hard = addInstanceStats(table_hard, instance, numNodes, len(edges), minDegree, maxDegree, minWeight, maxWeight)

    table = "\\begin{table}[h!]\n"
    table += "    \\centering\n"
    table += "    \\begin{tabular}{l|l|c|c|c|c|c|c}\n"
    table += "        Group & Instance & $n$ & $m$ & $\\underline{d}$ & $\\overline{d}$ & $\\underline{w}$ & $\\overline{w}$ \\\\\n"
    table += "        \\hline\n"
    table += f"        \\multirow{{ {numEasy} }}{{*}}{{easy}}"  
    table += table_easy
    table += "        \\hline\n"
    table += f"        \\multirow{{ {numHard} }}{{*}}{{hard}}"
    table += table_hard
    table += "    \\end{tabular}\n"
    table += "    \\caption{Overview of Instances used}\n"
    table += "    \\label{tab:instance_overview}\n"
    table += "\\end{table}\n"
    print(table)
    

printTables()
            
