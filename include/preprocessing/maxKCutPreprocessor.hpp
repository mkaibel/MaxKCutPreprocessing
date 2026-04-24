#ifndef MAXKCUTPREPROCESSOR_HPP
#define MAXKCUTPREPROCESSOR_HPP

#include <deque>
#include <memory>
#include <stdexcept>
#include <vector>

#include "networkit/Globals.hpp"
#include "networkit/graph/Graph.hpp"

#include "globals.hpp"
#include "twoLayerSplit.hpp"
#include "ogdf/decomposition/StaticSPQRTree.h"
#include "preprocessing/maxKCutPreprocessorUtils.hpp"
#include "preprocessing/spqr.hpp"
#include "preprocessing/twoLayerSplitParallel.hpp"

namespace mkcp
{
    struct DataReducerConfig
    {
        bool removeCliques = true, contractNegativeEdges = true, contractNegativeTriangles = true, contractSimilarVertices = true, splitTwoLayer = true, SPQRReduction = true, TwoLayerSmallSide = true;
    };

    struct DataReducerStats
    {
        ruleStats lowDegreeStats, dominatingEdges, dominatingTriangles,
                  connectedSimilarVertices, disconnectedSimilarVertices, fullClique, clique,
                  twoLayerConnector, spqr, twoLayerSmallSide;
        unsigned connectedComponentSPlits = 0, biconnectedComponentSplits = 0,
                 twoLayerSplits = 0;
    };

    class DataReducer
    {
    public:
        /**
         * Preprocessor class for MaxKCut
         * @param graph the graph to be preprocessed, which must have indexed edges with setMaintainCompactEdges()
         * @param k value of k
         * @param seed the seed
         * @param config for which preprocessing rules should be used, by default all are employed
         */
        DataReducer(const NetworKit::Graph& graph, unsigned int k,
                    unsigned seed = 1234, DataReducerConfig config = DataReducerConfig{true, true, true, true, true, true, true});

        /*
         * Run the data reduction algorithm
         */
        void run(bool naive = false);

        /**
         * @return true iff the data reduction algorithm has been run already
         */
        [[nodiscard]] bool hasRun() const { return preprocessedGraph; };

        [[nodiscard]] const DataReducerStats& getStats() const
        {
            if (!preprocessedGraph)
            {
                throw std::runtime_error("Must call preprocessing first");
            }

            return stats;
        }


        [[nodiscard]] NetworKit::edgeweight getOffset() const
        {
            if (!preprocessedGraph)
            {
                throw std::runtime_error("Must call preprocessing first");
            }

            return objectiveOffset;
        }

        /**
         * Returns the graph kernels that can't be further reducted by data reduction
         * rules
         * @return A vector of graphs that can't be further reducted by preprocessing
         * and must be handed to a solver
         */
        const std::vector<NetworKit::Graph>& getGraphKernel() const
        {
            if (!preprocessedGraph)
            {
                throw std::runtime_error("Graph kernels can't be accessed before "
                    "preprocessing has been computed");
            }

            return kernels;
        }

        /**
         * Reconstructs the optimum solution of the full graph given the optimum
         * solution to the kernels (can be optained by getGraphLernel)
         */
        void constructFullSolution(const std::vector<Partition>& kernelSolutions);

        /**
         * Returns the optimum solution for the whole graph, assuming that optimum
         * solutions for the graph kernels have been provided
         */
        const Partition& getSolution() const
        {
            if (!computedSolution)
            {
                throw std::runtime_error(
                    "solution can't be accessed before being computed");
            }

            return solution;
        }

    private:
        unsigned int k;

        unsigned seed;

        DataReducerConfig config;

        //! Stores the offset of the kernel optimum solution values of the kernel compared to the original graph
        NetworKit::edgeweight objectiveOffset = 0.0;

        bool preprocessedGraph = false;
        bool computedSolution = false;
        Partition solution;

        DataReducerStats stats;

        std::deque<QueuedGraph> graph_queue;
        std::shared_ptr<DataReductionReconstructor> rootRule;

        //! Stores the final kernelized graphs
        std::vector<NetworKit::Graph> kernels;
        //! Stores the leafs of the reconstruction rules
        std::vector<std::shared_ptr<DataReductionKernel>> kernelRules;

        //! Helper vector of size n to store temporary node values, has to be all none
        //! after an operation completes
        std::vector<NetworKit::node> _tempNodeValues;
        //! Helper vector of size n to store temporary edgeweight values, has to be
        //! all 0s after an operation completes
        std::vector<NetworKit::edgeweight> _tempWeightValues;
        //! helper vector of size n to store temporary counts, has to be all 0s after
        //! operation is complete
        std::vector<NetworKit::count> _tempCountValues;
        //! helper vector of size n to store temporary bools, has to be all false
        //! after operation is complete.
        std::vector<bool> _tempBoolValues;

        //! Removes vertices with positive edges and degree < k
        bool reduceLowDegreePositiveVertices(QueuedGraph& currentGraph);

        //! Contracts dominating edges with negative weights
        //! Checks only the cuts induced by a single vertex
        bool contractNegativeEdges(QueuedGraph& currentGraph);

        //! Contracts edges if they dominate a trinagle of 3 negative edges
        bool contractNegativeTriangles(QueuedGraph& currentGraph);

        //! Checks if neighbourhoods of v and neighbour are similar and only differ in
        //! an alpha factor with weights
        //! Returns 0.0 if neighbourhoods are not similar
        NetworKit::edgeweight
        alphaSimilarNeighbourhood(const QueuedGraph& currentGraph, NetworKit::node v,
                                  NetworKit::node neighbour,
                                  const std::vector<NetworKit::edgeweight>& weights);

        //! Contracts vertices with an identical neighbourhood connected by a negative
        //! weight edge
        bool contractConnectedSimilarVertices(QueuedGraph& currentGraph);

        //! Contracts vertices with an identical neighbourhood that are not connected
        bool contractDisconnectedSimilarVertices(QueuedGraph& currentGraph);

        /**
         * Removes all cliques with a small boundary and unit weight edges from the
         * graph
         */
        bool removeFullCliques(QueuedGraph& currentGraph);

        /**
         * Removes all cliques with few neighbours outside the clique and unit weight
         * edges from the graph
         */
        bool removeCliques(QueuedGraph& currentGraph);

        /**
         * Splits current graph into its connected components and adds the connected
         * components as their own graph to the queue
         */
        bool splitByConnectedComponents(QueuedGraph& currentGraph);

        /**
         * Splits current graph into its biconnected components and adds the connected
         * components as their own graph to the queue
         */
        bool splitByBiconnectedComponents(QueuedGraph& currentGraph);

        /**
         * Splits the current graph along two-layer-connectors
         */
        bool splitTwoLayerConnector(QueuedGraph& currentGraph);

        /**
         * Tries to heuristically find multiple two-layer-connectors in parallel
         * Removes them all at once
         */
        bool splitTwoLayerConnectorParallel(QueuedGraph& currentGraph);

        /**
         * Returns a two layer connector
         * @param tempBoolValues vector to store temporary bools
         * @param tempNodeValues vector to store temporary node values
         * @param contractedGraph the contracted graph from which to extract the TLC
         * @return a two-layer connector including the boundary nodes (in the returned graph), their IDs in the original graph, the nodes on the smaller side of the cutset and the TLC-graph
         */
        TwoLayerConnector getTwoLayerConnector(std::vector<bool>& tempBoolValues, std::vector<NetworKit::node>& tempNodeValues,
                                               ContractorGraph& contractedGraph) const;

        //! Helper for splitTwoLayerConnectorParallel
        void collectFinalCuts(const QueuedGraph& currentGraph, unsigned numParallelRuns,
                              const std::vector<std::vector<std::vector<NetworKit::Edge>>>& twoLayerConnectors,
                              const std::vector<std::vector<std::vector<NetworKit::node>>>& twoLayerConnectorsSideA,
                              std::vector<std::vector<NetworKit::Edge>>& finalCuts,
                              std::vector<std::vector<NetworKit::node>>& finalCutsSideA);
        //! Helper for splitTwoLayerConnectorParallel
        void checkSplitability(std::vector<std::vector<std::vector<NetworKit::Edge>>>& twoLayerConnectors,
                               std::vector<std::vector<std::vector<NetworKit::node>>>& twoLayerConnectorsSideA,
                               unsigned i,
                               std::vector<bool>& tempBoolValues, std::vector<NetworKit::node>& tempNodeValues,
                               ContractorGraph& contractedGraph) const;

        /**
         * Tries heuristically to find multiple two-layer-connectors s.t. one side can be removed by solving to optimality
         */
        bool removeTwoLayerConnectorSmallSide(QueuedGraph& currentGraph);

        //! Helper for splitTwoLayerConnectorParallel
        void checkTwoLayerSolvableCandidate(std::vector<std::vector<TwoLayerConnector>>& twoLayerConnectors,
                               unsigned i,
                               std::vector<bool>& tempBoolValues, std::vector<NetworKit::node>& tempNodeValues,
                               ContractorGraph& contractedGraph) const;

        /**
         * Calls SPQR-based data reduction
         */
        bool spqrReduction(QueuedGraph& currentGraph);

        /**
         *
         */
        bool processSPQRNode(QueuedGraph& currentGraph, ogdf::StaticSPQRTree& spqr,
                             SPQRCurrentStatus& spqrStatus, LeafParent leafParent);

        /**
         * Cleans the vertex from all helper structures.
         * To be used when vertices are deleted and therefore won't be affected by
         * later cleanup
         */
        void cleanVertexHelpers(NetworKit::node v);

        void assertHelpersAreClean() const;
    };
} // namespace mkcp

#endif // MAXKCUTPREPROCESSOR_HPP
