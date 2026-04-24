#include "preprocessing/maxKCutPreprocessor.hpp"
#include "networkit/Globals.hpp"
#include "networkit/graph/Graph.hpp"
#include "preprocessing/maxKCutPreprocessorUtils.hpp"
#include <cassert>
#include <memory>

#include "solver/max_k_cut_solver_gurobi.hpp"

namespace mkcp
{
    DataReducer::DataReducer(const NetworKit::Graph& graph, unsigned int k,
                             unsigned seed, DataReducerConfig config)
        : k(k), seed(seed), config(config), solution(graph.numberOfNodes())
    {
        rootRule = std::make_shared<DataReductionDummy>();
        graph_queue.emplace_back(graph, k, rootRule);

        _tempNodeValues.resize(graph.upperNodeIdBound(), NetworKit::none);
        _tempCountValues.resize(graph.upperNodeIdBound(), 0);
        _tempWeightValues.resize(graph.upperNodeIdBound(), 0.0);
        _tempBoolValues.resize(graph.upperNodeIdBound(), false);
    }

    void DataReducer::run(bool naive)
    {
        preprocessedGraph = true;

        while (!graph_queue.empty())
        {
            QueuedGraph currentGraph = std::move(graph_queue.back());
            graph_queue.pop_back();

            bool reducedSomething = true;
            bool splitGraph = false;

            bool leftUnitWeight = false;
            bool leftLocallyPositive = false;
            bool allRulesFailedOnce = false;

            while (reducedSomething)
            {
                if (currentGraph.readGraph().numberOfNodes() == 0)
                {
                    break;
                }

                reducedSomething = false;

                assertHelpersAreClean();

                reducedSomething |= reduceLowDegreePositiveVertices(currentGraph);
                if (reducedSomething)
                {
                    continue;
                }

                if (splitByConnectedComponents(currentGraph))
                {
                    splitGraph = true;
                    break;
                }
                if (splitByBiconnectedComponents(currentGraph))
                {
                    splitGraph = true;
                    break;
                }

                if (naive)
                {
                    break;
                }

                assertHelpersAreClean();

                // Since two-layer-connectors generalize the low degree rule we moved this
                // split here, so the low degree vertices get removed by the cheaper
                // degree based rule
                if (splitTwoLayerConnectorParallel(currentGraph))
                {
                    splitGraph = true;
                    break;
                }

                assertHelpersAreClean();

                reducedSomething |= removeFullCliques(currentGraph);
                if (reducedSomething)
                    continue;

                if (!leftUnitWeight)
                {
                    leftUnitWeight = true;
                    currentGraph.markRule(RuleFlag::AllGraphSeparators, true);
                    // As two layer separators are especially costly, we don't eant to call
                    // them too often
                    currentGraph.markRule(RuleFlag::TwoLayerSeperators, false);
                    reducedSomething = true;
                    continue;
                }

                assertHelpersAreClean();

                reducedSomething |= removeCliques(currentGraph);
                if (reducedSomething)
                    continue;

                assertHelpersAreClean();

                reducedSomething |= contractNegativeEdges(currentGraph);
                if (reducedSomething)
                    continue;
                reducedSomething |= contractNegativeTriangles(currentGraph);
                if (reducedSomething)
                    continue;
                reducedSomething |= contractConnectedSimilarVertices(currentGraph);
                if (reducedSomething)
                    continue;

                reducedSomething |= contractDisconnectedSimilarVertices(currentGraph);
                if (reducedSomething)
                    continue;

                // Rules that break local positivity
                reducedSomething |= spqrReduction(currentGraph);
                if (reducedSomething)
                    continue;

                reducedSomething |= removeTwoLayerConnectorSmallSide(currentGraph);
                if (reducedSomething)
                    continue;

                // If all rules ran through once and changed nothing, we pretend like
                // something was removed so that all whole graph rules get reset and try
                // again
                if (!allRulesFailedOnce)
                {
                    allRulesFailedOnce = true;
                    reducedSomething = true;
                }

                currentGraph.markRule(RuleFlag::AllGraphRules, true);
            }

            assertHelpersAreClean();

            if (splitGraph)
            {
                continue;
            }

            if (currentGraph.readGraph().numberOfNodes() == 0)
            {
                currentGraph.attachNewRule(std::make_shared<EmptyGraphReconstructor>(
                    currentGraph.readGraph().upperNodeIdBound()));

                continue;
            }

            currentGraph.compactNodeIDs();

            std::shared_ptr<DataReductionKernel> kernelRule =
                std::make_shared<DataReductionKernel>();
            kernels.emplace_back(currentGraph.readGraph());
            kernelRules.emplace_back(kernelRule);
            currentGraph.attachNewRule(kernelRule);
        }
    }

    void DataReducer::constructFullSolution(
        const std::vector<Partition>& kernelSolutions)
    {
        computedSolution = true;

        assert(kernelSolutions.size() == kernels.size());

        for (unsigned int i = 0; i < kernelSolutions.size(); i++)
        {
            kernelRules[i]->setPartition(kernelSolutions[i]);
        }

        std::deque<std::shared_ptr<DataReductionReconstructor>>
            reconstructorTraversal;
        std::deque<std::shared_ptr<DataReductionReconstructor>> reconstructionOrder;

        reconstructorTraversal.emplace_back(rootRule);

        // Construct a DFS order of the reconstruction tree
        while (!reconstructorTraversal.empty())
        {
            std::shared_ptr<DataReductionReconstructor> currentRule =
                reconstructorTraversal.back();
            reconstructorTraversal.pop_back();

            reconstructionOrder.emplace_front(currentRule);
            for (const std::shared_ptr<DataReductionReconstructor>& childRule :
                 currentRule->getChildren())
            {
                reconstructorTraversal.emplace_back(childRule);
            }
        }

        for (std::shared_ptr<DataReductionReconstructor>& rule :
             reconstructionOrder)
        {
            rule->reconstructData();
        }

        solution = *(rootRule->getPartition().get());
    }

    void DataReducer::cleanVertexHelpers(NetworKit::node v)
    {
        _tempNodeValues[v] = NetworKit::none;
        _tempCountValues[v] = 0;
        _tempBoolValues[v] = false;
        _tempWeightValues[v] = 0.0;
    }

    void DataReducer::assertHelpersAreClean() const
    {
#ifndef NDEBUG
        assert(_tempNodeValues.size() == _tempWeightValues.size());
        assert(_tempNodeValues.size() == _tempCountValues.size());
        assert(_tempNodeValues.size() == _tempBoolValues.size());

        for (unsigned int i = 0; i < _tempNodeValues.size(); i++)
        {
            assert(_tempNodeValues[i] == NetworKit::none);
            assert(_tempCountValues[i] == 0);
            assert(_tempWeightValues[i] == 0.0);
            assert(_tempBoolValues[i] == false);
        }
#endif
    }
} // namespace mkcp
