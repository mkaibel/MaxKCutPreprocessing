instances_small=("soc-firm.snap" "rt-twitter-copen.snap" "g000292.snap" "g000981.snap" "g000302.snap" "g001207.snap" "g001918.snap" "g000677.snap" "g001075.snap" "g000087.snap" "ENZYMES295.snap" "imgseg_271031.snap" "imgseg_106025.snap")
instances_medium=("bio-diseasome.snap" "bio-celegans.snap" "bio-DM-LC.snap" "ca-netscience.snap" "imgseg_35058.snap" "imgseg_374020.snap" "imgseg_105019.snap" "road-euroroad.snap" "bio-yeast.snap" "ca-CSphd.snap" "ego-facebook.snap" "inf-power.snap" "ca-Erdos992.snap" "road-luxembourg-osm.snap" "imgseg_147062.snap")
instances_large=("web-it-2004.snap" "web-google.snap" "web-Stanford.snap" "ca-coauthors-dblp.snap" "ca-IMDB.snap" "inf-road-central.snap")
#"ca-coauthors-dblp.snap"

instances_celar=("CELAR_01.snap" "CELAR_02.snap" "CELAR_03.snap" "CELAR_04.snap" "CELAR_05.snap" "CELAR_06.snap" "CELAR_07.snap" "CELAR_08.snap" "CELAR_09.snap" "CELAR_10.snap" "CELAR_11.snap")
instances_duttest=("DUTtest1_200.1.snap" "DUTtest1_200.2.snap" "DUTtest1_200.3.snap" "DUTtest1_200.4.snap" "DUTtest1_200.5.snap" "DUTtest1_916.1.snap" "DUTtest1_916.2.snap" "DUTtest1_916.3.snap" "DUTtest1_916.4.snap" "DUTtest1_916.5.snap")
instances_surprise=("SURPRISE_01.snap" "SURPRISE_02.snap" "SURPRISE_03.snap" "SURPRISE_04.snap" "SURPRISE_05.snap" "SURPRISE_06.snap" "SURPRISE_07.snap" "SURPRISE_08.snap" "SURPRISE_09.snap" "SURPRISE_10.snap" "SURPRISE_11.snap")

instances_fap=("${instances_celar[@]}" "${instances_duttest[@]}" "${instances_surprise[@]}")

instances_torus=("t2g10_5555.snap" "t2g10_6666.snap" "t2g10_7777.snap" "t2g15_5555.snap" "t2g15_6666.snap" "t2g15_7777.snap" "t2g20_5555.snap" "t2g20_6666.snap" "t2g20_7777.snap" "t3g5_5555.snap" "t3g5_6666.snap" "t3g5_7777.snap" "t3g6_5555.snap" "t3g6_6666.snap" "t3g6_7777.snap" "t3g7_5555.snap" "t3g7_6666.snap" "t3g7_7777.snap")

instances_all=("${instances_small[@]}" "${instances_medium[@]}" "${instances_large[@]}" "${instances_fap[@]}" "${instances_torus[@]}")

gset_indices=($(seq 1 67) 70 72 77)

k=(3 4 5 6 7 8 10 12)

ablation_configs=("--cne --cnt --csv --clq --tlc --tlcs --spqr" "--clq --tlc  --tlcs --spqr" "--cne --cnt --csv --tlc  --tlcs --spqr" "--cne --cnt --csv --clq  --tlcs --spqr" "--cne --cnt --csv --clq --tlc --spqr" "--cne --cnt --csv --clq --tlc  --tlcs" "")
ablation_config_names=("all" "noDomEdges" "noCliques" "noTwoLayer" "noTLCOneSide" "noBiconnector" "none")

basic_presolve=(0 1 2)
#basic_constraints=("" "-c")
basic_constraints=("")
