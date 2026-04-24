package_add_test(GreedyTest greedy_max_k_cut_test.cpp
        ../src/solver/greedy_max_k_cut.cpp
        ../src/globals.cpp)

package_add_test(IndexedSetTest indexedSet.cpp)

package_add_test(BasicTest basic.cpp
        ../src/globals.cpp)

package_add_test(UtilsTest utils.cpp
        ../src/globals.cpp
        ../src/preprocessing/maxKCutPreprocessingUtils.cpp
)

package_add_test(ExtraConstraintTest extraConstraints.cpp
        ../src/globals.cpp
        ../src/util/indexedSetGraphUtil.cpp
        ../src/solver/max_k_cut_solver_gurobi.cpp
        ../src/solver/separationHeuristics/cliqueFinder.cpp
        ../src/solver/separationHeuristics/wheelFinder.cpp
        ../src/solver/separationHeuristics/shiftedColumnFinder.cpp
        ../src/solver/separationHeuristics/hypermetricFinder.cpp
        ../src/solver/separationHeuristics/cycleFinder.cpp
        ../src/solver/greedy_max_k_cut.cpp
        ../src/solver/localSearchHeuristic.cpp
        ../src/solver/constraintGenerator.cpp
)

package_add_test(PreprocessorTest preprocessorTest.cpp
        ../src/globals.cpp

        ../src/util/indexedSetGraphUtil.cpp
        ../src/util/bipartiteMatching.cpp

        ../src/preprocessing/lowDegreeRules.cpp
        ../src/preprocessing/dominatingEdges.cpp
        ../src/preprocessing/similarVertices.cpp
        ../src/preprocessing/cliqueRules.cpp
        ../src/preprocessing/maxKCutPreprocessingUtils.cpp
        ../src/preprocessing/connectedComponentRules.cpp
        ../src/preprocessing/twoLayerSplit.cpp
        ../src/preprocessing/twoLayerSplitParallel.cpp
        ../src/preprocessing/spqr.cpp
        ../src/preprocessing/maxKCutPreprocessor.cpp

        ../src/solver/max_k_cut_solver_gurobi.cpp
        ../src/solver/separationHeuristics/cliqueFinder.cpp
        ../src/solver/separationHeuristics/wheelFinder.cpp
        ../src/solver/separationHeuristics/shiftedColumnFinder.cpp
        ../src/solver/separationHeuristics/hypermetricFinder.cpp
        ../src/solver/separationHeuristics/cycleFinder.cpp
        ../src/solver/greedy_max_k_cut.cpp
        ../src/solver/localSearchHeuristic.cpp
)
