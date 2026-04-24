# Instance classification
timeout = 1800.0
timeout_long = 7200.0

small_instances = [ 'soc-firm.snap', 'rt-twitter-copen.snap', 'g000292.snap', 'g000981.snap', 'g000302.snap', 'g001207.snap', 'g001918.snap', 'g000677.snap', 'g001075.snap', 'g000087.snap', 'ENZYMES295.snap', 'imgseg_271031.snap', 'imgseg_106025.snap', ]

medium_instances = [ 'bio-diseasome.snap', 'bio-celegans.snap', 'bio-DM-LC.snap', 'ca-netscience.snap', 'imgseg_35058.snap', 'imgseg_374020.snap', 'imgseg_105019.snap', 'road-euroroad.snap', 'bio-yeast.snap', 'ca-CSphd.snap', 'ego-facebook.snap', 'inf-power.snap', 'ca-Erdos992.snap', 'road-luxembourg-osm.snap', 'imgseg_147062.snap', ]

large_instances = [ 'web-it-2004.snap', 'web-google.snap', 'web-Stanford.snap', 'ca-coauthors-dblp.snap', 'ca-IMDB.snap', 'inf-road-central.snap', ]

fap_instances = [ 'CELAR_01.snap', 'CELAR_02.snap', 'CELAR_03.snap', 'CELAR_04.snap', 'CELAR_05.snap', 'CELAR_06.snap', 'CELAR_07.snap', 'CELAR_08.snap', 'CELAR_09.snap', 'CELAR_10.snap', 'CELAR_11.snap', 'DUTtest1_200.1.snap', 'DUTtest1_200.2.snap', 'DUTtest1_200.3.snap', 'DUTtest1_200.4.snap', 'DUTtest1_200.5.snap', 'DUTtest1_916.1.snap', 'DUTtest1_916.2.snap', 'DUTtest1_916.3.snap', 'DUTtest1_916.4.snap', 'DUTtest1_916.5.snap', 'SURPRISE_01.snap', 'SURPRISE_02.snap', 'SURPRISE_03.snap', 'SURPRISE_04.snap', 'SURPRISE_05.snap', 'SURPRISE_06.snap', 'SURPRISE_07.snap', 'SURPRISE_08.snap', 'SURPRISE_09.snap', 'SURPRISE_10.snap', 'SURPRISE_11.snap', ]

torus_instances = [ "t2g10_5555.snap", "t2g10_6666.snap", "t2g10_7777.snap", "t2g15_5555.snap", "t2g15_6666.snap", "t2g15_7777.snap", "t2g20_5555.snap", "t2g20_6666.snap", "t2g20_7777.snap", "t3g5_5555.snap", "t3g5_6666.snap", "t3g5_7777.snap", "t3g6_5555.snap", "t3g6_6666.snap", "t3g6_7777.snap", "t3g7_5555.snap", "t3g7_6666.snap", "t3g7_7777.snap", ]

physics_instances = [ 'G1.snap', 'G2.snap', 'G3.snap', 'G4.snap', 'G5.snap', 'G6.snap', 'G7.snap', 'G8.snap', 'G9.snap', 'G10.snap', 'G11.snap', 'G12.snap', 'G13.snap', 'G14.snap', 'G15.snap', 'G16.snap', 'G17.snap', 'G18.snap', 'G19.snap', 'G20.snap', 'G21.snap', 'G22.snap', 'G23.snap', 'G24.snap', 'G25.snap', 'G26.snap', 'G27.snap', 'G28.snap', 'G29.snap', 'G30.snap', 'G31.snap', 'G32.snap', 'G33.snap', 'G34.snap', 'G35.snap', 'G36.snap', 'G37.snap', 'G38.snap', 'G39.snap', 'G40.snap', 'G41.snap', 'G42.snap', 'G43.snap', 'G44.snap', 'G45.snap', 'G46.snap', 'G47.snap', 'G48.snap', 'G49.snap', 'G50.snap', 'G51.snap', 'G52.snap', 'G53.snap', 'G54.snap', 'G55.snap', 'G56.snap', 'G57.snap', 'G58.snap', 'G59.snap', 'G60.snap', 'G61.snap', 'G62.snap', 'G63.snap', 'G64.snap', 'G65.snap', 'G66.snap', 'G67.snap', 'G70.snap', 'G72.snap', 'G77.snap', ]

instsets = ['easy', 'medium', 'hard', 'torus', 'fap']
instances = {
    'easy': small_instances,
    'medium': medium_instances,
    'hard': large_instances,
    'torus': torus_instances,
    'fap': fap_instances,
}

paper_names = {
    0:'none',
    1:'our',
    2:'naive',
}

paper_palette = {
    'none': 'magenta',
    'naive': 'orange',
    'our': 'blue'
}

paper_markers = {
    'none': 'v',
    'naive':'v',
    'our':'H',
}

# Palettes etc for regular experiment
custom_palette = {
    'Naive & Constraints': 'green',
    'Naive Only': 'black',
    'Reducer & Constraints': 'magenta',
    'Reducer Only': 'blue',
    'Constraints Only': 'orange',
    'Neither': 'purple'
}

custom_markers = {
    'Naive & Constraints': '^',
    'Naive Only': 'v',
    'Reducer & Constraints': 'o',
    'Reducer Only': 'H',
    'Constraints Only': 's',
    'Neither': 'D'
}

custom_names = {
    '2, 1': 'Naive & Constraints',
    '2, 0': 'Naive Only',
    '1, 1': 'Reducer & Constraints',
    '1, 0': 'Reducer Only',
    '0, 1': 'Constraints Only',
    '0, 0': 'Neither'
}

custom_names_ablation = {
    '1, 1, 1, 1, 1, 1, 1': 'All',
    '0, 1, 1, 1, 1, 1, 1': 'NoNegContraction',
    '1, 0, 1, 1, 1, 1, 1': 'NoNegTriangle',
    '1, 1, 0, 1, 1, 1, 1': 'NoSimilarVertices',
    '1, 1, 1, 0, 1, 1, 1': 'NoCliques',
    '1, 1, 1, 1, 0, 1, 1': 'NoTwoLayer',
    '1, 1, 1, 1, 1, 0, 1': 'NoTwoLayerSmallSide',
    '1, 1, 1, 1, 1, 1, 0': 'NoBiconnector',
    '0, 0, 0, 0, 0, 0, 0': 'SimplePreprocessing',
    '0, 0, 0, 1, 1, 1, 1': 'NoDominatingEdges',
}

custom_palette_ablation = {
    'All': 'blue',
    'NoNegContraction': 'red',
    'NoNegTriangle': 'green',
    'NoSimilarVertices': 'cyan',
    'NoCliques': 'orange',
    'NoTwoLayer': 'magenta',
    'NoBiconnector': 'purple',
    'SimplePreprocessing': 'black',
    'NoDominatingEdges': 'purple',
}

custom_markers_ablation = {
    'All': 's',
    'NoNegContraction': 'v',
    'NoNegTriangle': 'v',
    'NoSimilarVertices': 'v',
    'NoCliques': 'o',
    'NoTwoLayer': 'H',
    'NoBiconnector': 'v',
    'SimplePreprocessing': 'D',
    'NoDominatingEdges': '^',
}

custom_lines = ['', (2, 2)]
