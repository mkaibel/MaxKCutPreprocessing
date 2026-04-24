import instanceDownloader

names = ["soc-firm", "ca-netscience", "bio-diseasome", 'bio-celegans', 'bio-DM-LC', "rt-twitter-copen", 'road-euroroad',
         'bio-yeast', 'ca-CSphd', 'ego-facebook', 'inf-power', 'ca-Erdos992', 'road-luxembourg-osm', 'web-Stanford',
         "web-it-2004", 'ENZYMES295', 'ca-MathSciNet', 'web-google', 'ca-coauthors-dblp', 'ca-IMDB', 'inf-road-central',
         "g001207", "g000981", "g000292", "g000302", "g001918", "g000677", "g001075", "g000087",
         "imgseg_35058", "imgseg_374020", "imgseg_105019", 'imgseg_147062', 'imgseg_271031', 'imgseg_106025',
#         "potts_3g_444_444", "potts_3pm_345_345",
         ]
# TODO web google is missing (doesn't seem to appear in the network repository in this size, instead we have a very large size)

if __name__ == '__main__':
    for name in names:
        instanceDownloader.load_clean_instance(name)
    instanceDownloader.load_G_set()
    instanceDownloader.load_FAP_CALMA()
    instanceDownloader.load_biqnac_torus()
