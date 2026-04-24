import os
import re
import tarfile
import time
from pathlib import Path

import boto3
from botocore import UNSIGNED
from botocore.client import Config

import requests
import zipfile
import gzip
import io
from collections import defaultdict


pathRaw = "instances/instances_raw/"
pathClean = "instances/instances_clean/"

mqlibFiles = {'g001207', 'g000981', 'g000292', 'g000302', "g001918", "g000677", "g001075", "g000087",
     "imgseg_35058", "imgseg_374020", "imgseg_105019", 'imgseg_147062', 'imgseg_271031', 'imgseg_106025',
 }

fileNames = {'soc-firm':'soc-firm-hi-tech.txt',
             'ca-netscience':'ca-netscience.mtx',
             'bio-diseasome':'bio-diseasome.mtx',
             'bio-celegans':'bio-celegans.mtx',
             'bio-DM-LC':'bio-DM-LC.edges',
             'rt-twitter-copen':'rt-twitter-copen.mtx',
             'web-it-2004':'web-it-2004.mtx',
             'road-euroroad':'road-euroroad.edges',
             'bio-yeast':'bio-yeast.mtx',
             'ca-CSphd':'ca-CSphd.mtx',
             'ego-facebook':'ego-facebook.edges',
             'web-google':'web-Google.mtx',
             'inf-power':'inf-power.mtx',
             'ca-Erdos992':'ca-Erdos992.mtx',
             'road-luxembourg-osm':'road-luxembourg-osm.mtx',
             'web-Stanford':'web-Stanford.mtx',
             'ENZYMES295':'ENZYMES295.edges',
             'ca-MathSciNet':'ca-MathSciNet.mtx',
             'ca-coauthors-dblp':'ca-coauthors-dblp.mtx',
             'ca-IMDB':'ca-IMDB.edges',
             'inf-road-central':'inf-road_central.mtx',
             'g001207':'g001207.mql',
             'g000981':'g000981.mql',
             'g000292':'g000292.mql',
             'g000302':'g000302.mql',
             'g001918':'g001918.mql',
             'g000677':'g000677.mql',
             'g001075':'g001075.mql',
             'g000087':'g000087.mql',
             "imgseg_374020":"imgseg_374020.mql",
             'imgseg_35058':'imgseg_35058.mql',
             "imgseg_105019":"imgseg_105019.mql",
             'imgseg_147062':'imgseg_147062.mql',
             'imgseg_271031':'imgseg_271031.mql',
             'imgseg_106025':'imgseg_106025.mql',
             'potts_3g_444_444':'potts_3g_444_444',
             'potts_3pm_345_345':'potts_3pm_345_345',
             }

fileURLs = {'soc-firm':'https://nrvis.com/download/data/soc/soc-firm-hi-tech.zip',
            'ca-netscience':'https://nrvis.com/download/data/ca/ca-netscience.zip',
            'bio-diseasome':'https://nrvis.com/download/data/bio/bio-diseasome.zip',
            'bio-celegans':'https://nrvis.com/download/data/bio/bio-celegans.zip',
            'bio-DM-LC':'https://nrvis.com/download/data/bio/bio-DM-LC.zip',
            'rt-twitter-copen':'https://nrvis.com/download/data/rt/rt-twitter-copen.zip',
            'web-it-2004':'https://nrvis.com/download/data/web/web-it-2004.zip',
            'road-euroroad':'https://nrvis.com/download/data/road/road-euroroad.zip',
            'bio-yeast':'https://nrvis.com/download/data/bio/bio-yeast.zip',
            'ca-CSphd':'https://nrvis.com/download/data/ca/ca-CSphd.zip',
            'ego-facebook':'https://nrvis.com/download/data/misc/ego-facebook.zip',
            'web-google':'https://nrvis.com/download/data/misc/web-Google.zip',
            'inf-power':'https://nrvis.com/download/data/inf/inf-power.zip',
            'ca-Erdos992':'https://nrvis.com/download/data/ca/ca-Erdos992.zip',
            'road-luxembourg-osm':'https://nrvis.com/download/data/road/road-luxembourg-osm.zip',
            'web-Stanford':'https://nrvis.com/download/data/misc/web-Stanford.zip',
            'ENZYMES295':'https://nrvis.com/download/data/labeled/ENZYMES295.zip',
            'ca-MathSciNet':'https://nrvis.com/download/data/ca/ca-MathSciNet.zip',
            'ca-coauthors-dblp':'https://nrvis.com/download/data/ca/ca-coauthors-dblp.zip',
            'ca-IMDB':'https://nrvis.com/download/data/ca/ca-IMDB.zip',
            'inf-road-central':'https://nrvis.com/download/data/dimacs10/inf-road_central.zip',
            }



def getGraph(instance):
    print("Loading " + instance + " raw")
    if not os.path.isfile(pathRaw + fileNames[instance]):
        if instance in mqlibFiles:
            load_from_MQLib(instance)
        else:
            r = requests.get(fileURLs[instance], allow_redirects=True, stream=True)
            if fileURLs[instance].endswith('.zip'):
                z = zipfile.ZipFile(io.BytesIO(r.content))
                z.extract(member=fileNames[instance], path=pathRaw)
            elif fileURLs[instance].endswith('.gz'):
                f = open(file=pathRaw + fileNames[instance], mode='w')
                f.write(gzip.decompress(r.content).decode('utf-8'))
    print("finished loading " + instance + " raw")

def loadGraph(instance):
    print(f"Generating clean file for instance {instance}")

    edges = []
    numNodes = 0
    if instance == "test":
        lines = open(pathRaw + instance, mode='r', encoding='utf-8-sig').readlines()
        for line in lines:
            items = line.rstrip().split(',')
            if items[0] == items[1]:
                continue
            t = int(items[2])
            u, v = order(int(items[0]), int(items[1]))
            edges.append([u, v, t])
        edges.sort(key=lambda x: x[2])
    elif re.fullmatch(r"G\d+", instance) or re.fullmatch(r"t\dg\d+_\d+", instance):
        lines = open(pathRaw + instance, mode='r').readlines()
        for line in lines:
            items = line.rstrip().split(' ')
            if len(items) < 3:
                continue
            if items[0] == items[1]:
                continue
            w = float(items[2])
            u, v = order(int(items[0]), int(items[1]))
            edges.append([u, v, w])
    elif 'potts' in fileNames[instance]:
        lines = open(pathRaw + fileNames[instance], 'r').readlines()
        next(lines)
        for line in lines:
            items = line.rstrip().split()
            if items[0] == items[1]:
                continue
            u, v = order(int(items[0]) - 1, int(items[1]) - 1)
            edges.append([u, v, float(items[2])])
    elif fileNames[instance].endswith("txt"):
        getGraph(instance)
        lines = open(pathRaw + fileNames[instance], 'r').readlines()
        for line in lines:
            items = line.rstrip().split()
            if items[0] == items[1]:
                continue
            u, v = order(int(items[0]), int(items[1]))
            edges.append([u, v, float(items[2])])
    elif fileNames[instance].endswith("mtx"):
        getGraph(instance)
        lines = open(pathRaw + fileNames[instance], 'r').readlines()
        for line in lines[2:]:
            items = line.rstrip().split()

            if len(items) != 2 or items[0].startswith("%"):
                continue

            if items[0] == items[1]:
                continue
            u, v = order(int(items[0]) - 1, int(items[1]) - 1)
            edges.append([u, v, 1.0])
    elif fileNames[instance].endswith("mql"):
        getGraph(instance)
        lines = open(pathRaw + fileNames[instance], 'r').readlines()
        for line in lines[3:]:
            items = line.rstrip().split()

            if len(items) == 0 or items[0].startswith("#"):
                continue

            if items[0] == items[1]:
                continue
            u, v = order(int(items[0]), int(items[1]))
            edges.append([u - 1, v - 1, float(items[2])])
    elif fileNames[instance].endswith("edges"):
        getGraph(instance)
        lines = open(pathRaw + fileNames[instance], 'r').readlines()
        for line in lines:
            items = line.rstrip().split()
            if len(items) == 1:
                items = items[0].rstrip().split(',')

            if len(items) == 0 or items[0].startswith("#") or items[0].startswith("%"):
                continue

            if items[0] == items[1]:
                continue
            u, v = order(int(items[0]), int(items[1]))
            edges.append([u - 1, v - 1, 1.0])

    weightByEdge = defaultdict(int)

    for u, v, w in edges:
        weightByEdge[(u, v)] += w

    # Convert back to list of tuples
    edges = [(a, b, w) for (a, b), w in weightByEdge.items()]

    for edge in edges:
        numNodes = max(numNodes, edge[0], edge[1])

    return edges, numNodes + 1

def load_clean_instance(instance):
    edges, num_nodes = loadGraph(instance)

    out_file = pathClean + instance + ".snap"

    write_clean_instance(out_file, edges, num_nodes)

def write_clean_instance(filename, edges, num_nodes):
    Path(pathClean).mkdir(parents=True, exist_ok=True)
    output = open(filename, "w+")

    min_edge_id = edges[0][0]

    for u, v, weight in edges:
        if u < min_edge_id:
            min_edge_id = u
        if v < min_edge_id:
            min_edge_id = v

    # Maybe write meta data at the start?

    for (u, v, weight) in edges:
        output.write(str(u - min_edge_id) + " " + str(v - min_edge_id) + " " + str(weight) + "\n")

def load_from_MQLib(instance):
    conn = boto3.client('s3', config=Config(signature_version=UNSIGNED))
    key = instance + ".zip"
    response = conn.get_object(Bucket="mqlibinstances", Key=key)
    zip_content = response['Body'].read()
    z = zipfile.ZipFile(io.BytesIO(zip_content))
    with z.open(instance + ".txt") as source, open(pathRaw + instance + ".mql", 'wb') as target:
        target.write(source.read())

def order(a, b):
    if a < b:
        return a, b
    else:
        return b, a

def load_G_set():
    index = list(range(1,68)) + [70, 72, 77, 81]

    for i in index:
        instance = f"G{i}"
        print("Loading " + instance + " raw")
        if not os.path.isfile(pathRaw + instance):
            r = requests.get(f"https://web.stanford.edu/~yyye/yyye/Gset/{instance}", allow_redirects=True, stream=True)
            f = open(file=pathRaw + instance, mode='w')
            f.write(r.text)
            f.close()
        print("finished loading " + instance + " raw")
        load_clean_instance(instance)

physics_sources = ['t2g10', 't2g15', 't2g20', 't3g5', 't3g6', 't3g7']
physics_seeds = ['5555', '6666', '7777']
physics_url = 'https://biqmac.aau.at/library/mac/ising/'

def load_biqnac_torus():
    for source in physics_sources:
        for seed in physics_seeds:
            instance = f"{source}_{seed}"
            print(f"Loading {instance} raw")
            if not os.path.isfile(pathRaw + instance):
                r = requests.get(f"{physics_url}{instance}", allow_redirects=True, stream=True)
                f = open(file = pathRaw + instance, mode = 'w')
                f.write(r.text)
                f.close()
            print(f"finished loading {instance} raw")
            load_clean_instance(instance)

fap_sources = ['CELAR', 'SURPRISE', 'DUTtest1'
               #, 'UK_prob'
               ]

def load_FAP_CALMA():
    file_pattern = re.compile(r".*?(\d{2}|\d{3}\.\d{1})/(cst\.txt|ctr\.txt)$", re.IGNORECASE)
    cost_pattern = re.compile(r"[\S\s]*?a1\s=\s*?(\d+)\s[\S\s]*?[\S\s]*?a2\s=\s*?(\d+)\s[\S\s]*?[\S\s]*?a3\s=\s*?(\d+)\s[\S\s]*?[\S\s]*?a4\s=\s*?(\d+)\s[\S\s]+")

    for source in fap_sources:
        print("Loading " + source + " raw")
        r = requests.get(f"https://fap.zib.de/problems/CALMA/{source}.tar.gz", allow_redirects=True, stream=True)
        tar = tarfile.open(fileobj=io.BytesIO(r.content), mode="r:gz")



        groups = defaultdict(dict)

        for member in tar.getmembers():
            match = file_pattern.match(member.name)
            if match:
                folder_id, filename = match.groups()
                f = tar.extractfile(member)
                if f:
                    groups[folder_id][filename] = f.read().decode("utf-8")
                    print(f"extracting file {folder_id}/{filename}")

        for folder_id, files in groups.items():
            cst = files.get("cst.txt")
            ctr = files.get("ctr.txt")
            if not cst: # Catch case that file names are all capitalized
                cst = files.get("CST.TXT")
            if not ctr:
                ctr = files.get("CTR.TXT")

            if cst and ctr:
                print(f"Processing folder {folder_id}")

                a = [1, 1, 1, 1, 1]
                match = cost_pattern.match(cst)
                if match:
                    a1, a2, a3, a4 = match.groups()
                    a[1] = int(a1)
                    a[2] = int(a2)
                    a[3] = int(a3)
                    a[4] = int(a4)
                    a[0] = 100 * a[1]

                print(a)

                edges = []

                for line in ctr.splitlines():
                    items = line.rstrip().split()

                    if len(items) == 6:
                        u, v = order(int(items[0]), int(items[1]))
                        edges.append([u - 1, v - 1, float(a[int(items[5])])])
                    elif len(items) == 5:
                        u, v = order(int(items[0]), int(items[1]))
                        edges.append([u - 1, v - 1, float(a[0])])

                write_clean_instance(f"{pathClean}/{source}_{folder_id}.snap", edges, 0)

        print("finished loading " + source + " raw")