//
// Created by michael-kaibel on 04/03/2026.
//

#include "preprocessing/twoLayerSplitParallel.hpp"

#include "networkit/components/ConnectedComponents.hpp"
#include "preprocessing/maxKCutPreprocessor.hpp"
#include "solver/max_k_cut_solver_gurobi.hpp"
#include "util/bipartiteMatching.hpp"
#include "util/subgraphUtils.hpp"

namespace mkcp
{
    std::shared_ptr<Partition> TwoLayerParallelReconstructor::getPartition() const
    {
        if (!_calledReconstruction)
        {
            throw std::runtime_error(
                "Can't access partition before calling reconstructor function");
        }

        return _partition;
    }

    const std::vector<std::shared_ptr<DataReductionReconstructor>>&
    TwoLayerParallelReconstructor::getChildren() const
    {
        return _childRule;
    }

    void TwoLayerParallelReconstructor::addChildReduction(
        std::shared_ptr<DataReductionReconstructor>&)
    {
        throw std::runtime_error("Attempted to add a child to two-layer-connector, "
            "which should be passed to the constructor instead");
    }

    void TwoLayerParallelReconstructor::reconstructData()
    {
        _calledReconstruction = true;

        _partition = std::make_shared<Partition>(_maxNodeID);

        std::vector<bool> tempBoolValues(_maxNodeID, false);
        std::vector<NetworKit::node> tempNodeValues(_maxNodeID, NetworKit::none);

        // Initialize partition and currently connected components
        UnionFindAlgo1 connectedComponents(_maxNodeID);
        for (unsigned i = 0; i < _originalVertexIDs.size(); ++i)
        {
            const std::vector<NetworKit::node>& component = _originalVertexIDs[i];
            std::shared_ptr<Partition> _componentPartition = _childRule[i]->getPartition();

            assert(!component.empty());
            for (NetworKit::node v = 0; v < component.size(); v++)
            {
                connectedComponents.unionized(component[0], component[v]);
                _partition->assignElement(component[v], _componentPartition->assignmentOf(v));
            }
        }

        for (int i = _twoLayerConnectors.size() - 1; i >= 0; --i)
        {
            std::vector<NetworKit::node> connectorNodes;
            for (const NetworKit::Edge& e : _twoLayerConnectors[i])
            {
                if (!tempBoolValues[e.u])
                {
                    connectorNodes.emplace_back(e.u);
                    tempBoolValues[e.u] = true;
                }
                if (!tempBoolValues[e.v])
                {
                    connectorNodes.emplace_back(e.v);
                    tempBoolValues[e.v] = true;
                }
            }

            // Map twoLayerConnector into graph with node IDs 0,...,|V_C|
            NetworKit::Graph twoLayerConnectorProjectedDown(connectorNodes.size());
            std::vector<NetworKit::node> originalIDs(connectorNodes.size());
            std::vector<NetworKit::node> sideAProjectedDown;

            NetworKit::node currentID = 0;
            for (NetworKit::node v : connectorNodes)
            {
                tempNodeValues[v] = currentID;
                originalIDs[currentID] = v;
                currentID++;
            }

            for (const NetworKit::Edge& e : _twoLayerConnectors[i])
            {
                twoLayerConnectorProjectedDown.addEdge(tempNodeValues[e.u], tempNodeValues[e.v]);
            }

            for (NetworKit::node v : _sideAInConnector[i])
            {
                sideAProjectedDown.emplace_back(tempNodeValues[v]);
            }

            for (NetworKit::node v : connectorNodes)
            {
                tempBoolValues[v] = false;
                tempNodeValues[v] = NetworKit::none;
            }

            std::vector<partitionID> permutation = computeColourPermutation(
                twoLayerConnectorProjectedDown, originalIDs, sideAProjectedDown);

            for (NetworKit::node v : _sideAInConnector[i])
            {
                tempBoolValues[v] = true;
            }

            NetworKit::node aSideNode = NetworKit::none, bSideNode = NetworKit::none;

            for (NetworKit::node v : connectorNodes)
            {
                if (tempBoolValues[v])
                {
                    tempBoolValues[v] = false;

                    if (aSideNode == NetworKit::none)
                        aSideNode = v;
                    else
                        connectedComponents.unionized(aSideNode, v);
                }
                else
                {
                    if (bSideNode == NetworKit::none)
                        bSideNode = v;
                    else
                        connectedComponents.unionized(bSideNode, v);
                }
            }

            assert(aSideNode != NetworKit::none);
            assert(bSideNode != NetworKit::none);
            assert(connectedComponents.find(aSideNode) != connectedComponents.find(bSideNode));

            // Swap it so side b is smaller to ensure O(nlog(n)) runtime
            if (connectedComponents.setSize(aSideNode) < connectedComponents.setSize(bSideNode))
            {
                std::swap(aSideNode, bSideNode);
                permutation = invertPermutation(permutation);
            }

            for (NetworKit::node v : connectedComponents.setOf(bSideNode))
                _partition->assignElement(v, permutation[_partition->assignmentOf(v)]);

            connectedComponents.unionized(aSideNode, bSideNode);
        }
    }

    std::vector<partitionID> TwoLayerParallelReconstructor::computeColourPermutation(
        const NetworKit::Graph& connector, const std::vector<NetworKit::node>& originalIDs,
        const std::vector<NetworKit::node>& sideA) const
    {
        std::vector<partitionID> permutation(_k);

        NetworKit::Graph colourRelationGraph(2 * _k);

        for (NetworKit::node v : sideA)
        {
            for (NetworKit::node w : connector.neighborRange(v))
            {
                if (!colourRelationGraph.hasEdge(
                    _partition->assignmentOf(originalIDs[v]),
                    _partition->assignmentOf(originalIDs[w]) + _k))
                {
                    colourRelationGraph.addEdge(
                        _partition->assignmentOf(originalIDs[v]),
                        _partition->assignmentOf(originalIDs[w]) + _k);
                }
            }
        }

        std::vector<NetworKit::node> sideACRG;

        for (NetworKit::node v = 0; v < _k; v++)
        {
            sideACRG.emplace_back(v);
        }

        NetworKit::Graph colourComplement =
            bipartiteComplement(colourRelationGraph, sideACRG);

        BipartiteMatching matching(colourComplement, sideACRG);

        matching.run();

        for (const NetworKit::Edge e : matching.getMatching())
        {
            if (e.u < e.v)
            {
                permutation[e.v - _k] = e.u;
            }
            else
            {
                permutation[e.u - _k] = e.v;
            }
        }

        return std::move(permutation);
    }

    void DataReducer::collectFinalCuts(const QueuedGraph& currentGraph, unsigned numParallelRuns,
                                       const std::vector<std::vector<std::vector<NetworKit::Edge>>>& twoLayerConnectors,
                                       const std::vector<std::vector<std::vector<NetworKit::node>>>&
                                       twoLayerConnectorsSideA, std::vector<std::vector<NetworKit::Edge>>& finalCuts,
                                       std::vector<std::vector<NetworKit::node>>& finalCutsSideA)
    {
        std::vector<bool> edgeRemoved(currentGraph.readGraph().numberOfEdges(), false);

        for (unsigned i = 0; i < numParallelRuns; i++)
        {
            for (unsigned j = 0; j < twoLayerConnectors[i].size(); j++)
            {
                // Figure out of any of the edges were not covered by a previous cut
                std::vector<NetworKit::Edge> edgeNewlyCut;

                for (NetworKit::Edge e : twoLayerConnectors[i][j])
                {
                    if (!edgeRemoved[currentGraph.readGraph().edgeId(e.u, e.v)])
                    {
                        edgeRemoved[currentGraph.readGraph().edgeId(e.u, e.v)] = true;
                        edgeNewlyCut.emplace_back(e);
                        _tempBoolValues[e.u] = true;
                        _tempBoolValues[e.v] = true;
                    }
                }

                if (!edgeNewlyCut.empty())
                {
                    std::vector<NetworKit::node> sideAOfCut;
                    for (NetworKit::node v : twoLayerConnectorsSideA[i][j])
                    {
                        if (_tempBoolValues[v])
                        {
                            sideAOfCut.emplace_back(v);
                        }
                    }

                    finalCuts.emplace_back(edgeNewlyCut);
                    finalCutsSideA.emplace_back(sideAOfCut);
                }

                for (NetworKit::Edge e : edgeNewlyCut)
                {
                    _tempBoolValues[e.u] = false;
                    _tempBoolValues[e.v] = false;
                }
            }
        }
    }

    TwoLayerConnector DataReducer::getTwoLayerConnector(std::vector<bool>& tempBoolValues,
                                                        std::vector<NetworKit::node>& tempNodeValues,
                                                        ContractorGraph& contractedGraph) const
    {
        const std::vector<NetworKit::Edge>& edges =
            contractedGraph.currentEdges();

        // Figure out which vertices are the sides of the two layer connector
        std::vector<NetworKit::node> boundaryVertices;

        for (const NetworKit::Edge e : edges)
        {
            if (!tempBoolValues[e.u])
            {
                tempBoolValues[e.u] = true;
                boundaryVertices.emplace_back(e.u);
            }
            if (!tempBoolValues[e.v])
            {
                tempBoolValues[e.v] = true;
                boundaryVertices.emplace_back(e.v);
            }
        }

        for (NetworKit::node v : boundaryVertices)
        {
            tempBoolValues[v] = false;
        }

        assert(!boundaryVertices.empty());

        // We now use tempNodeValues to assign labels 0,...,size to the boundary
        // vertices and construct the graph induced by them
        // TODO maybe this can be done cleaner by the subgraph function on
        // currentGraph
        NetworKit::node downLabel = 0;
        std::vector<NetworKit::node> originalID(boundaryVertices.size());

        for (NetworKit::node v : boundaryVertices)
        {
            tempNodeValues[v] = downLabel;
            originalID[downLabel] = v;
            downLabel++;
        }

        NetworKit::Graph twoLayerConnector(boundaryVertices.size());

        for (NetworKit::Edge e : edges)
        {
            assert(!twoLayerConnector.hasEdge(tempNodeValues[e.u],
                tempNodeValues[e.v]));
            twoLayerConnector.addEdge(tempNodeValues[e.u], tempNodeValues[e.v]);
        }

        std::vector<std::vector<NetworKit::node>> sides =
            contractedGraph.contractedSets();

        assert(sides.size() == 2);
        assert(!sides[0].empty());
        assert(!sides[1].empty());

        NetworKit::node sideARepresentative =
            contractedGraph.setRepresentative(sides[0][0]);

        std::vector<NetworKit::node> sideABoundary, sideBBoundary;

        for (NetworKit::node v : boundaryVertices)
        {
            if (contractedGraph.setRepresentative(v) == sideARepresentative)
            {
                sideABoundary.emplace_back(tempNodeValues[v]);
            }
            else
            {
                sideBBoundary.emplace_back(tempNodeValues[v]);
            }
            tempNodeValues[v] = NetworKit::none;
        }

        if (sides[0].size() < sides[1].size())
        {
            return {
                std::move(sideBBoundary), std::move(sideABoundary), std::move(sides[0]), std::move(originalID),
                std::move(twoLayerConnector)
            };
        }

        return {
            std::move(sideABoundary), std::move(sideBBoundary), std::move(sides[1]), std::move(originalID),
            std::move(twoLayerConnector)
        };
    }

    void DataReducer::checkSplitability(std::vector<std::vector<std::vector<NetworKit::Edge>>>& twoLayerConnectors,
                                        std::vector<std::vector<std::vector<NetworKit::node>>>& twoLayerConnectorsSideA,
                                        unsigned i, std::vector<bool>& tempBoolValues,
                                        std::vector<NetworKit::node>& tempNodeValues,
                                        ContractorGraph& contractedGraph) const
    {
        TwoLayerConnector tlc = getTwoLayerConnector(tempBoolValues, tempNodeValues, contractedGraph);

        // Condition to skip separator
        if ((tlc.sideABoundary.size() > k - 1) || (tlc.sideBBoundary.size() > k - 1))
        {
            return;
        }

        bool canSplit = checkTwoLayerSplitCriteria(tlc.twoLayerConnectorGraph, tlc.sideABoundary, k);

        if (canSplit)
        {
            std::vector<NetworKit::Edge> edges;
            std::vector<NetworKit::node> sideABoundary;

            for (const NetworKit::Edge& e : tlc.twoLayerConnectorGraph.edgeRange())
            {
                edges.emplace_back(tlc.originalID[e.u], tlc.originalID[e.v]);
            }
            for (const NetworKit::node v : tlc.sideABoundary)
            {
                sideABoundary.emplace_back(tlc.originalID[v]);
            }

            twoLayerConnectors[i].emplace_back(edges);
            twoLayerConnectorsSideA[i].emplace_back(sideABoundary);
        }
    }

    bool checkTwoLayerSplitCriteria(const NetworKit::Graph& connector, const std::vector<NetworKit::node>& oneSide,
                                    unsigned k)
    {
        if (connector.numberOfEdges() < k || connector.numberOfNodes() <= k)
        {
            return true;
        }

        const NetworKit::Graph complement =
            bipartiteComplement(connector, oneSide);

        BipartiteMatching matching(complement, oneSide);
        matching.run();
        if (static_cast<int>(matching.getMatching().size()) >
            -1 + 2 * (static_cast<int>(connector.numberOfNodes()) -
                static_cast<int>(k)))
        {
            return true;
        }

        return false;
    }

    bool DataReducer::splitTwoLayerConnectorParallel(QueuedGraph& currentGraph)
    {
        if (!config.splitTwoLayer || !currentGraph.flagActive(RuleFlag::TwoLayerSeperators))
            return false;

        // TODO 40 is temporary value, maybe pick a smaller one?
        if (currentGraph.readGraph().numberOfNodes() < 40)
            return false;

        assertHelpersAreClean();

        currentGraph.markRule(RuleFlag::TwoLayerSeperators, false);

        currentGraph.compactNodeIDs();
        std::mt19937 randomizer(seed);
        std::uniform_int_distribution<unsigned> dis;

        ContractorGraph contractedGraph(currentGraph.readGraph().numberOfNodes(), dis(randomizer));

        for (NetworKit::WeightedEdge e :
             currentGraph.readGraph().edgeWeightRange())
        {
            if (e.weight < 0.0)
            {
                contractedGraph.contractNodes(e.u, e.v);
                _tempBoolValues[e.u] = true;
                _tempBoolValues[e.v] = true;
            }
            else
            {
                contractedGraph.addEdge(e.u, e.v);
            }
        }

        if (contractedGraph.numNodes() < 2)
        {
            for (NetworKit::node v : currentGraph.readGraph().nodeRange())
                _tempBoolValues[v] = false;
            return false;
        }


        unsigned numThreads = omp_get_max_threads();

        std::vector<std::deque<ContractorGraph>> contractedGraphQueues(numThreads,
                                                                       std::deque<ContractorGraph>(1, contractedGraph));
        std::vector<std::vector<std::vector<NetworKit::Edge>>> twoLayerConnectors(numThreads);
        std::vector<std::vector<std::vector<NetworKit::node>>> twoLayerConnectorsSideA(numThreads);

#pragma omp parallel for
        for (unsigned i = 0; i < numThreads; i++)
        {
            unsigned startSeed = i;

            // As this code is run in parallel we need a tempXarray for each parallel run
            std::vector<bool> tempBoolValues = _tempBoolValues;
            std::vector<NetworKit::node> tempNodeValues(currentGraph.readGraph().numberOfNodes(), NetworKit::none);

            for (NetworKit::node v : currentGraph.readGraph().nodeRange())
            {
                if (tempBoolValues[v] || currentGraph.readGraph().degree(v) > k + 2)
                    continue;

                unsigned contractToNeighbour = dis(randomizer) % currentGraph.readGraph().degree(v);

                contractedGraphQueues[i].back().contractNodes(
                    v, currentGraph.readGraph().getIthNeighbor(v, contractToNeighbour));

                tempBoolValues[v] = true;
                tempBoolValues[currentGraph.readGraph().getIthNeighbor(v, contractToNeighbour)] = true;
            }

            for (NetworKit::node v : currentGraph.readGraph().nodeRange())
            {
                tempBoolValues[v] = false;
            }

            if (contractedGraphQueues[i].back().numNodes() == 1)
                continue;

            contractedGraphQueues[i].back().reseed(i);

            // TODO 200 is just a magic number here, maybe reconsider
            if (contractedGraphQueues[i].back().numNodes() > 200)
            {
                contractedGraphQueues[i].back().contractKEdges(contractedGraphQueues[i].back().numNodes() - 200);
            }

            while (!contractedGraphQueues[i].empty())
            {
                //std::cout << "Contracted graph stack size: " << contractedGraphQueues[i].size() << "\n";
                ContractorGraph poppedGraph = contractedGraphQueues[i].back();
                contractedGraphQueues[i].pop_back();
                //std::cout << "Current graph size: " << poppedGraph.numNodes() << "\n";

                // TODO Performance check if other splits work better here
                if (poppedGraph.numNodes() > 6)
                {
                    // Contract the graph some more
                    poppedGraph.reseed(startSeed);
                    startSeed++;

                    unsigned int t =
                        poppedGraph.numNodes() -
                        static_cast<unsigned>(std::ceil(static_cast<float>(poppedGraph.numNodes()) / 2.0));

                    poppedGraph.contractKEdges(t);

                    assert(poppedGraph.numNodes() >= 2);

                    contractedGraphQueues[i].emplace_back(poppedGraph);
                    contractedGraphQueues[i].emplace_back(poppedGraph);
                }
                else
                {
                    unsigned int t = poppedGraph.numNodes() - 2;
                    poppedGraph.contractKEdges(t);

                    // See if we found a cut that is a 2 layer separator
                    assert(poppedGraph.numNodes() == 2);

                    checkSplitability(twoLayerConnectors, twoLayerConnectorsSideA, i, tempBoolValues, tempNodeValues,
                                      poppedGraph);
                }
            }
        }

        for (NetworKit::node v : currentGraph.readGraph().nodeRange())
            _tempBoolValues[v] = false;

        std::vector<std::vector<NetworKit::Edge>> finalCuts;
        std::vector<std::vector<NetworKit::node>> finalCutsSideA;


        collectFinalCuts(currentGraph, numThreads, twoLayerConnectors, twoLayerConnectorsSideA, finalCuts,
                         finalCutsSideA);

        bool splitAnything = !finalCuts.empty();

        if (splitAnything)
        {
            for (const std::vector<NetworKit::Edge>& cut : finalCuts)
            {
                for (const NetworKit::Edge& e : cut)
                {
                    objectiveOffset += currentGraph.readGraph().weight(e.u, e.v);
                    stats.twoLayerConnector += currentGraph.deleteEdge(e.u, e.v);
                }
            }
            stats.twoLayerSplits += finalCuts.size();

            NetworKit::ConnectedComponents components(currentGraph.readGraph());
            components.run();

            assert(components.numberOfComponents() > 1);

            std::vector<std::vector<NetworKit::node>> connectedComponents = components.getComponents();

            std::vector<std::vector<NetworKit::node>> originalIDs;
            std::vector<std::shared_ptr<DataReductionReconstructor>> childRules;

            for (const auto& connectedComponent : connectedComponents)
            {
                std::pair<NetworKit::Graph, std::vector<NetworKit::node>> subgraph =
                    getInducedSubgraph(currentGraph.readGraph(), connectedComponent, _tempNodeValues);

                childRules.emplace_back(std::make_shared<DataReductionDummy>());
                graph_queue.emplace_back(subgraph.first, k, childRules.back());

                originalIDs.emplace_back(subgraph.second);
            }

            std::shared_ptr<TwoLayerParallelReconstructor> twoLayerParallelReconstructor = std::make_shared<
                TwoLayerParallelReconstructor>(k, currentGraph.readGraph().upperNodeIdBound(), originalIDs, childRules,
                                               finalCuts, finalCutsSideA);

            currentGraph.attachNewRule(twoLayerParallelReconstructor);
        }

        assertHelpersAreClean();
        return splitAnything;
    }

    const std::vector<std::shared_ptr<DataReductionReconstructor>>& TwoLayyerConnectorSideSolvedReconstructor::getChildren() const
    {
        return _childRule;
    }

    std::shared_ptr<Partition> TwoLayyerConnectorSideSolvedReconstructor::getPartition() const
    {
        if (!_calledReconstruction)
        {
            throw std::runtime_error(
                "Can't access partition before calling reconstructor function");
        }

        return _partition;
    }

    void TwoLayyerConnectorSideSolvedReconstructor::addChildReduction(std::shared_ptr<DataReductionReconstructor>& child)
    {
        if (!_childRule.empty()) {
            throw std::runtime_error(
                "Attempted to add a second child rule to a non-split rule");
        }
        _childRule.emplace_back(std::shared_ptr<DataReductionReconstructor>(child));
    }

    void TwoLayyerConnectorSideSolvedReconstructor::reconstructData()
    {
        _calledReconstruction = true;

        assert(_childRule.size() == 1);
        _partition = _childRule[0]->getPartition();
        assert(_partition.get() != nullptr);

        for (NetworKit::node v = 0; v < _subgraphOriginalIDs.size(); v++)
        {
            _partition->assignElement(_subgraphOriginalIDs[v], _subgraphSolution.assignmentOf(v));
        }

        std::vector<partitionID> permutation = computeColourPermutation();

        for (unsigned long v : _subgraphOriginalIDs)
        {
            _partition->assignElement(v, permutation[_partition->assignmentOf(v)]);
        }
    }

    std::vector<partitionID> TwoLayyerConnectorSideSolvedReconstructor::computeColourPermutation() const
    {
        std::vector<partitionID> permutation(_k);

        NetworKit::Graph colourRelationGraph(2 * _k);

        for (NetworKit::node v : _largeSideBoundary)
        {
            for (NetworKit::node w : _twoLayerConnector.neighborRange(v))
            {
                if (!colourRelationGraph.hasEdge(
                    _partition->assignmentOf(_connectorOriginalIDs[v]),
                    _partition->assignmentOf(_connectorOriginalIDs[w]) + _k))
                {
                    colourRelationGraph.addEdge(
                        _partition->assignmentOf(_connectorOriginalIDs[v]),
                        _partition->assignmentOf(_connectorOriginalIDs[w]) + _k);
                }
            }
        }

        std::vector<NetworKit::node> sideACRG;

        for (NetworKit::node v = 0; v < _k; v++)
        {
            sideACRG.emplace_back(v);
        }

        NetworKit::Graph colourComplement =
            bipartiteComplement(colourRelationGraph, sideACRG);

        BipartiteMatching matching(colourComplement, sideACRG);

        matching.run();

        for (const NetworKit::Edge e : matching.getMatching())
        {
            if (e.u < e.v)
            {
                permutation[e.v - _k] = e.u;
            }
            else
            {
                permutation[e.u - _k] = e.v;
            }
        }

        return std::move(permutation);
    }

    bool DataReducer::removeTwoLayerConnectorSmallSide(QueuedGraph& currentGraph)
    {
        if (!config.TwoLayerSmallSide || !currentGraph.flagActive(RuleFlag::TwoLayerSolveSmallSide))
            return false;

        // TODO 40 is temporary value, maybe pick a smaller one?
        if (currentGraph.readGraph().numberOfNodes() < 40)
            return false;

        bool removedAnything = false;

        assertHelpersAreClean();

        currentGraph.markRule(RuleFlag::TwoLayerSeperators, false);

        currentGraph.compactNodeIDs();
        std::mt19937 randomizer(seed);
        std::uniform_int_distribution<unsigned> dis;

        ContractorGraph contractedGraph(currentGraph.readGraph().numberOfNodes(), dis(randomizer));

        for (NetworKit::WeightedEdge e :
             currentGraph.readGraph().edgeWeightRange())
        {
            if (e.weight < 0.0)
            {
                contractedGraph.contractNodes(e.u, e.v);
                _tempBoolValues[e.u] = true;
                _tempBoolValues[e.v] = true;
            }
            else
            {
                contractedGraph.addEdge(e.u, e.v);
            }
        }

        if (contractedGraph.numNodes() < 2)
        {
            for (NetworKit::node v : currentGraph.readGraph().nodeRange())
                _tempBoolValues[v] = false;
            return false;
        }

        unsigned numThreads = omp_get_max_threads();

        std::vector<std::deque<ContractorGraph>> contractedGraphQueues(numThreads,
                                                                       std::deque<ContractorGraph>(1, contractedGraph));
        std::vector<std::vector<TwoLayerConnector>> twoLayerConnectors(numThreads);

#pragma omp parallel for
        for (unsigned i = 0; i < numThreads; i++)
        {
            unsigned startSeed = i;

            // As this code is run in parallel we need a tempXarray for each parallel run
            std::vector<bool> tempBoolValues = _tempBoolValues;
            std::vector<NetworKit::node> tempNodeValues(currentGraph.readGraph().numberOfNodes(), NetworKit::none);

            for (NetworKit::node v : currentGraph.readGraph().nodeRange())
            {
                if (tempBoolValues[v] || currentGraph.readGraph().degree(v) > k + 2)
                    continue;

                unsigned contractToNeighbour = dis(randomizer) % currentGraph.readGraph().degree(v);

                contractedGraphQueues[i].back().contractNodes(
                    v, currentGraph.readGraph().getIthNeighbor(v, contractToNeighbour));

                tempBoolValues[v] = true;
                tempBoolValues[currentGraph.readGraph().getIthNeighbor(v, contractToNeighbour)] = true;
            }

            for (NetworKit::node v : currentGraph.readGraph().nodeRange())
            {
                tempBoolValues[v] = false;
            }

            if (contractedGraphQueues[i].back().numNodes() == 1)
                continue;

            contractedGraphQueues[i].back().reseed(i);

            // TODO 200 is just a magic number here, maybe reconsider
            if (contractedGraphQueues[i].back().numNodes() > 200)
            {
                contractedGraphQueues[i].back().contractKEdges(contractedGraphQueues[i].back().numNodes() - 200);
            }

            while (!contractedGraphQueues[i].empty())
            {
                //std::cout << "Contracted graph stack size: " << contractedGraphQueues[i].size() << "\n";
                ContractorGraph poppedGraph = contractedGraphQueues[i].back();
                contractedGraphQueues[i].pop_back();
                //std::cout << "Current graph size: " << poppedGraph.numNodes() << "\n";

                // TODO Performance check if other splits work better here
                if (poppedGraph.numNodes() > 6)
                {
                    // Contract the graph some more
                    poppedGraph.reseed(startSeed);
                    startSeed++;

                    unsigned int t =
                        poppedGraph.numNodes() -
                        static_cast<unsigned>(std::ceil(static_cast<float>(poppedGraph.numNodes()) / 2.0));

                    poppedGraph.contractKEdges(t);

                    assert(poppedGraph.numNodes() >= 2);

                    contractedGraphQueues[i].emplace_back(poppedGraph);
                    contractedGraphQueues[i].emplace_back(poppedGraph);
                }
                else
                {
                    unsigned int t = poppedGraph.numNodes() - 2;
                    poppedGraph.contractKEdges(t);

                    // See if we found a cut that is a 2 layer separator
                    assert(poppedGraph.numNodes() == 2);

                    checkTwoLayerSolvableCandidate(twoLayerConnectors, i, tempBoolValues, tempNodeValues,
                                                   poppedGraph);
                }
            }
        }

        for (NetworKit::node v : currentGraph.readGraph().nodeRange())
            _tempBoolValues[v] = false;

        NetworKit::count pot2OverN = 1;
        while (pot2OverN < currentGraph.readGraph().upperNodeIdBound())
            pot2OverN *= 2;

        std::vector<bool> hashTable(pot2OverN, false);

        std::vector<NetworKit::node> deletedVertices;
        for (unsigned i = 0; i < numThreads; i++)
        {
            for (unsigned j = 0; j < twoLayerConnectors[i].size(); j++)
            {
                TwoLayerConnector &tlc = twoLayerConnectors[i][j];

                bool deletable = true;
                NetworKit::node nodeSetHash = 0;
                for (NetworKit::node v : tlc.sideBNodes) // Check if parts were already deleted
                {
                    deletable &= !_tempBoolValues[v];
                    nodeSetHash ^= v;
                }

                if (deletable && !hashTable[nodeSetHash])
                {
                    hashTable[nodeSetHash] = true;
                    // Mark this node set as processed so we don't accidentally try to solve the same component multiple times

                    // As some nodes on the large side of the TLC may already have been deleted, remove such nodes from the side A boundary
                    unsigned i = 0;
                    while (i < tlc.sideABoundary.size())
                    {
                        NetworKit::node v = tlc.sideABoundary[i];

                        if (_tempBoolValues[tlc.originalID[v]])
                        {
                            tlc.twoLayerConnectorGraph.removeNode(v);
                            tlc.sideABoundary[i] = tlc.sideABoundary.back();
                            tlc.sideABoundary.pop_back();
                        }
                        else
                        {
                            i++;
                        }
                    }

                    auto [subgraph, originalIDs] = getInducedSubgraph(currentGraph.readGraph(),
                                                                      tlc.sideBNodes,
                                                                      _tempNodeValues);

                    subgraph.indexEdges();
                    subgraph.setMaintainCompactEdges();

                    // Figure out which nodews in the subgraph are part of the original boundary
                    std::vector<NetworKit::node> subgraphBoundary;
                    for (NetworKit::node v : tlc.sideBBoundary)
                    {
                        _tempNodeValues[tlc.originalID[v]] = 0;
                    }
                    for (NetworKit::node v : subgraph.nodeRange())
                    {
                        if (_tempNodeValues[originalIDs[v]] == 0)
                        {
                            _tempNodeValues[originalIDs[v]] = NetworKit::none;
                            subgraphBoundary.emplace_back(v);
                        }
                    }

                    bool canBeRemoved = false;
                    Partition removalSolution(0);
                    NetworKit::edgeweight removalWeight = 0.0;

                    if (tlc.sideBBoundary.size() > k)
                    {
                        unsigned maxColourUsed = k - 1;

                        MaxKCutSolverGurobi solver(subgraph, k, 1.0);
                        solver.run();

                        if (solver.foundSatisfactorySolution())
                        {
                            NetworKit::edgeweight optVal = solver.getObjectiveValue();
                            std::vector<std::vector<bool>> vertexConstraints(subgraph.numberOfNodes(), std::vector<bool>(k, false));

                            while (!canBeRemoved && maxColourUsed > 0)
                            {
                                for (NetworKit::node v : subgraph.nodeRange())
                                {
                                    for (unsigned c = maxColourUsed; c < k; c++)
                                    {
                                        vertexConstraints[v][c] = true;
                                    }
                                }

                                MaxKCutSolverGurobi constrainedSolver(subgraph, k, 1.0);
                                constrainedSolver.setVertexConstraints(vertexConstraints);
                                constrainedSolver.setMinValRequired(optVal);
                                constrainedSolver.setMinValueSufficien(optVal);
                                constrainedSolver.run();

                                if (!constrainedSolver.foundSatisfactorySolution())
                                    break;

                                Partition currentSolution = constrainedSolver.getSolution();

                                std::vector<bool> colourUsedInBoundary(k, false);
                                unsigned numColoursUsedInBoundary = 0;
                                std::vector<std::pair<partitionID, partitionID>> partialColourMap;
                                // Used to permute solution so colour IDs 0,...,max used on boundary
                                partitionID currentColour = 0;
                                // Upper bound on the number of colours required on the boundary while still getting an optimum solution
                                unsigned lowerColoursUsedInOptBoundary = 0;
                                // At least 1 below the minimum number of colours required in the boundary
                                for (NetworKit::node v : subgraphBoundary)
                                {
                                    if (!colourUsedInBoundary[currentSolution.assignmentOf(v)])
                                    {
                                        colourUsedInBoundary[currentSolution.assignmentOf(v)] = true;
                                        partialColourMap.emplace_back(currentSolution.assignmentOf(v),
                                                                      currentColour);
                                        numColoursUsedInBoundary++;
                                        currentColour++;
                                    }
                                }
                                std::vector<partitionID> permutation = completePermutation(partialColourMap);

                                currentSolution.swapPartitionNames(permutation);

                                // Compute contracted TLC and check it
                                auto [contractedTLC, sideAInContractedTLC] = constructContractedTwoLayerConnector(tlc, subgraphBoundary, originalIDs, currentSolution, _tempCountValues, numColoursUsedInBoundary);

                                canBeRemoved = checkTwoLayerSplitCriteria(contractedTLC, sideAInContractedTLC, k);
                                if (canBeRemoved)
                                {
                                    removalSolution = currentSolution;
                                    removalWeight = solver.getObjectiveValue();
                                }

                                maxColourUsed = numColoursUsedInBoundary - 1;
                            }
                        }
                    }
                    else
                    {
                        // TODO maybe try modifying to try and force solutions where B-side boundary has more different colours/colours are forced to be "nice"
                        MaxKCutSolverGurobi solver(subgraph, k, 1.0);
                        solver.run();

                        if (solver.foundSatisfactorySolution())
                        {
                            Partition currentSolution = solver.getSolution();

                            std::vector<bool> colourUsedInBoundary(k, false);
                            unsigned numColoursUsedInBoundary = 0;
                            std::vector<std::pair<partitionID, partitionID>> partialColourMap;
                            // Used to permute solution so colour IDs 0,...,max used on boundary
                            partitionID currentColour = 0;
                            // Upper bound on the number of colours required on the boundary while still getting an optimum solution
                            unsigned lowerColoursUsedInOptBoundary = 0;
                            // At least 1 below the minimum number of colours required in the boundary
                            for (NetworKit::node v : subgraphBoundary)
                            {
                                if (!colourUsedInBoundary[currentSolution.assignmentOf(v)])
                                {
                                    colourUsedInBoundary[currentSolution.assignmentOf(v)] = true;
                                    partialColourMap.emplace_back(currentSolution.assignmentOf(v),
                                                                  currentColour);
                                    numColoursUsedInBoundary++;
                                    currentColour++;
                                }
                            }
                            std::vector<partitionID> permutation = completePermutation(partialColourMap);

                            currentSolution.swapPartitionNames(permutation);

                            // Compute contracted TLC
                            auto [contractedTLC, sideAInContractedTLC] = constructContractedTwoLayerConnector(tlc, subgraphBoundary, originalIDs, currentSolution, _tempCountValues, numColoursUsedInBoundary);

                            std::vector<NetworKit::count> compressedDegrees;
                            for (partitionID c = 0; c < numColoursUsedInBoundary; c++)
                            {
                                compressedDegrees.emplace_back(contractedTLC.degree(c));
                            }

                            std::sort(compressedDegrees.begin(), compressedDegrees.end());
                            bool degreesFit = true;

                            for (unsigned c = 0; c < numColoursUsedInBoundary; c++)
                            {
                                degreesFit &= (compressedDegrees[numColoursUsedInBoundary - c - 1] < k - c);
                            }

                            canBeRemoved = degreesFit;
                            if (canBeRemoved)
                            {
                                removalSolution = currentSolution;
                                removalWeight = solver.getObjectiveValue();
                            }
                        }
                    }

                    if (canBeRemoved)
                    {
                        removedAnything = true;

                        objectiveOffset += removalWeight;
                        for (const NetworKit::Edge &e : tlc.twoLayerConnectorGraph.edgeRange())
                        {
                            objectiveOffset += currentGraph.readGraph().weight(tlc.originalID[e.u], tlc.originalID[e.v]);
                        }

                        for (NetworKit::node v : tlc.sideBNodes)
                        {
                            _tempBoolValues[v] = true;
                            stats.twoLayerSmallSide += currentGraph.deleteVertex(v);
                            deletedVertices.emplace_back(v);
                        }

                        std::shared_ptr<TwoLayyerConnectorSideSolvedReconstructor> reconstructor = std::make_shared<TwoLayyerConnectorSideSolvedReconstructor>(k, removalSolution, originalIDs, tlc.twoLayerConnectorGraph, tlc.originalID, tlc.sideABoundary);
                        currentGraph.attachNewRule(reconstructor);
                    }
                }
            }
        }

        for (NetworKit::node v : deletedVertices)
        {
            _tempBoolValues[v] = false;
        }

        assertHelpersAreClean();
        return removedAnything;
    }

    std::pair<NetworKit::Graph, std::vector<NetworKit::node>> constructContractedTwoLayerConnector(const TwoLayerConnector &tlc, const std::vector<NetworKit::node> &subgraphBoundary, const std::vector<NetworKit::node> &subgraphOriginalIDs, const Partition &solution, std::vector<NetworKit::count> &tempCountValues, unsigned numColoursUsedInBoundary)
    {
        NetworKit::Graph contractedTLC(numColoursUsedInBoundary + tlc.sideABoundary.size());
        NetworKit::node currentID = numColoursUsedInBoundary;
        std::vector<NetworKit::node> idInContractedTLC(tlc.twoLayerConnectorGraph.upperNodeIdBound(), NetworKit::none);
        std::vector<NetworKit::node> sideAInContractedTLC;

        for (NetworKit::node v : tlc.sideABoundary)
        {
            idInContractedTLC[v] = currentID;
            sideAInContractedTLC.emplace_back(currentID);
            currentID++;
        }

        for (NetworKit::node v : subgraphBoundary)
        {
            tempCountValues[subgraphOriginalIDs[v]] = solution.assignmentOf(v);
        }

        for (NetworKit::Edge e : tlc.twoLayerConnectorGraph.edgeRange())
        {
            assert((idInContractedTLC[e.u] == NetworKit::none) != (idInContractedTLC[e.v] == NetworKit::none));

            if (idInContractedTLC[e.v] == NetworKit::none)
            {
                std::swap(e.u, e.v);
            }

            if (!contractedTLC.hasEdge(tempCountValues[tlc.originalID[e.u]], idInContractedTLC[e.v]))
            {
                contractedTLC.addEdge(tempCountValues[tlc.originalID[e.u]], idInContractedTLC[e.v]);
            }
        }

        for (NetworKit::node v : subgraphBoundary)
        {
            tempCountValues[subgraphOriginalIDs[v]] = 0;
        }

        return {contractedTLC, sideAInContractedTLC};
    }

    void DataReducer::checkTwoLayerSolvableCandidate(
        std::vector<std::vector<TwoLayerConnector>>& twoLayerConnectors, unsigned i,
        std::vector<bool>& tempBoolValues, std::vector<NetworKit::node>& tempNodeValues,
        ContractorGraph& contractedGraph) const
    {
        TwoLayerConnector tlc = getTwoLayerConnector(tempBoolValues, tempNodeValues, contractedGraph);

        if (tlc.sideABoundary.size() <= (k * (k - 1) / 2 - (k - tlc.sideBBoundary.size()) * (k - tlc.sideBBoundary.
                size() - 1) / 2)
            && std::min(tlc.sideABoundary.size(), tlc.sideBBoundary.size()) < k
            && tlc.sideBNodes.size() <= 20)
        {
            bool degreesFine = true;

            for (NetworKit::node v : tlc.sideBBoundary)
            {
                degreesFine &= tlc.twoLayerConnectorGraph.degree(v) < k;
            }

            if (degreesFine)
            {
                twoLayerConnectors[i].emplace_back(tlc);
            }
        }
    }
}
