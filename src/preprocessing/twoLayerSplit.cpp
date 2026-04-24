#include "preprocessing/twoLayerSplit.hpp"
#include "globals.hpp"
#include "networkit/Globals.hpp"
#include "networkit/graph/Graph.hpp"
#include "preprocessing/maxKCutPreprocessor.hpp"
#include "preprocessing/maxKCutPreprocessorUtils.hpp"
#include "util/bipartiteMatching.hpp"
#include "util/subgraphUtils.hpp"

#include <cassert>
#include <deque>
#include <memory>
#include <random>
#include <utility>
#include <vector>

namespace mkcp
{
    TwoLayerReconstructor::TwoLayerReconstructor(
        unsigned int k, const NetworKit::Graph& twoLayerConnector,
        const std::vector<NetworKit::node> &sideA,
        const std::vector<NetworKit::node>& originalConnectorVertexIDs,
        NetworKit::node maxNodeID, const std::vector<NetworKit::node> &originalVertexIDsA,
        const std::vector<NetworKit::node> &originalVertexIDsB,
        const std::shared_ptr<DataReductionReconstructor>& childA,
        const std::shared_ptr<DataReductionReconstructor>& childB)
        : k(k), twoLayerConnector(twoLayerConnector), sideA(sideA),
          originalConnectorVertexIDs(originalConnectorVertexIDs),
          maxNodeID(maxNodeID), originalVertexIDsA(originalVertexIDsA),
          originalVertexIDsB(originalVertexIDsB)
    {
        childRule.emplace_back(childA);
        childRule.emplace_back(childB);
    }

    std::shared_ptr<Partition> TwoLayerReconstructor::getPartition() const
    {
        if (!calledReconstruction)
        {
            throw std::runtime_error(
                "Can't access partition before calling reconstructor function");
        }

        return partition;
    }

    const std::vector<std::shared_ptr<DataReductionReconstructor>>&
    TwoLayerReconstructor::getChildren() const
    {
        return childRule;
    }

    void TwoLayerReconstructor::addChildReduction(
        std::shared_ptr<DataReductionReconstructor>&)
    {
        throw std::runtime_error("Attempted to add a child to two-layer-connector, "
            "which should be passed to the constructor instead");
    }

    void TwoLayerReconstructor::reconstructData()
    {
        calledReconstruction = true;

        assert(childRule.size() == 2);

        partition = std::make_shared<Partition>(maxNodeID);

        std::shared_ptr<Partition> childPartitionA = childRule[0]->getPartition(),
                                   childPartitionB = childRule[1]->getPartition();

        for (NetworKit::node v = 0; v < originalVertexIDsA.size(); v++)
        {
            partition->assignElement(originalVertexIDsA[v],
                                     childPartitionA->assignmentOf(v));
        }

        for (NetworKit::node v = 0; v < originalVertexIDsB.size(); v++)
        {
            partition->assignElement(originalVertexIDsB[v],
                                     childPartitionB->assignmentOf(v));
        }

        std::vector<partitionID> colourSwap(k);

        NetworKit::Graph colourRelationGraph(2 * k);

        for (NetworKit::node v : sideA)
        {
            for (NetworKit::node w : twoLayerConnector.neighborRange(v))
            {
                if (!colourRelationGraph.hasEdge(
                    partition->assignmentOf(originalConnectorVertexIDs[v]),
                    partition->assignmentOf(originalConnectorVertexIDs[w]) + k))
                {
                    colourRelationGraph.addEdge(
                        partition->assignmentOf(originalConnectorVertexIDs[v]),
                        partition->assignmentOf(originalConnectorVertexIDs[w]) + k);
                }
            }
        }

        std::vector<NetworKit::node> sideA;

        for (NetworKit::node v = 0; v < k; v++)
        {
            sideA.emplace_back(v);
        }

        NetworKit::Graph colourComplement =
            bipartiteComplement(colourRelationGraph, sideA);

        BipartiteMatching matching(colourComplement, sideA);

        matching.run();

        for (NetworKit::Edge e : matching.getMatching())
        {
            if (e.u < e.v)
            {
                colourSwap[e.v - k] = e.u;
            }
            else
            {
                colourSwap[e.u - k] = e.v;
            }
        }

        childPartitionB->swapPartitionNames(colourSwap);

        for (NetworKit::node v = 0; v < originalVertexIDsB.size(); v++)
        {
            partition->assignElement(originalVertexIDsB[v],
                                     childPartitionB->assignmentOf(v));
        }
    }

    ContractorGraph::ContractorGraph(NetworKit::count n, unsigned int seed)
        : randomizer(seed), contractedVertices(n), n(n)
    {
    }

    ContractorGraph::ContractorGraph(NetworKit::count n,
                                     std::vector<NetworKit::Edge> edges,
                                     unsigned int seed)
        : randomizer(seed), contractedVertices(n), edges(std::move(edges)), n(n)
    {
    }

    void ContractorGraph::addEdge(NetworKit::node u, NetworKit::node v)
    {
        assert(u < n && v < n);

        edges.emplace_back(u, v);
    }

    const std::vector<NetworKit::Edge>& ContractorGraph::currentEdges()
    {
        for (unsigned int i = 0; i < edges.size(); i++)
        {
            if (contractedVertices.find(edges[i].u) ==
                contractedVertices.find(edges[i].v))
            {
                std::swap(edges[i], edges.back());
                edges.pop_back();
                // to account for the fact that a new edge is at position i, which we
                // haven't checked yet
                i--;
            }
        }

        return edges;
    }

    bool ContractorGraph::contractKEdges(unsigned int k)
    {
        while (k > 0 && (!edges.empty()) && (contractedVertices.numberOfSets() > 2))
        {
            std::swap(edges.back(), edges[randomizer() % edges.size()]);

            if (contractedVertices.find(edges.back().u) !=
                contractedVertices.find(edges.back().v))
            {
                contractedVertices.unionized(edges.back().u, edges.back().v);
                k--;
            }

            edges.pop_back();
        }

        return k == 0;
    }

    void ContractorGraph::contractNodes(NetworKit::node v, NetworKit::node w)
    {
        contractedVertices.unionized(v, w);
    }

    std::vector<std::vector<NetworKit::node>> ContractorGraph::contractedSets()
    {
        return contractedVertices.obtainSets();
    }

    void ContractorGraph::reseed(unsigned int newSeed)
    {
        randomizer = std::mt19937(newSeed);
    }

    NetworKit::node ContractorGraph::setRepresentative(NetworKit::node v)
    {
        return contractedVertices.find(v);
    }

    NetworKit::count ContractorGraph::setSize(NetworKit::node v)
    {
        return contractedVertices.setSize(v);
    }

    NetworKit::count ContractorGraph::numNodes() const
    {
        return contractedVertices.numberOfSets();
    }

    bool DataReducer::splitTwoLayerConnector(QueuedGraph& currentGraph)
    {
        if (!currentGraph.flagActive(RuleFlag::TwoLayerSeperators))
            return false;

        currentGraph.markRule(RuleFlag::TwoLayerSeperators, false);

        currentGraph.compactNodeIDs();
        std::mt19937 randomizer(seed);
        std::uniform_int_distribution<unsigned> dis;

        // TODO Currently magic number for the amound of repetititons
        for (unsigned int i = 0; i < 1; i++)
        {
            std::deque<ContractorGraph> contractedGraphStack;

            contractedGraphStack.emplace_back(currentGraph.readGraph().numberOfNodes(), dis(randomizer));

            for (NetworKit::WeightedEdge e :
                 currentGraph.readGraph().edgeWeightRange())
            {
                if (e.weight < 0.0)
                {
                    contractedGraphStack.back().contractNodes(e.u, e.v);
                    _tempBoolValues[e.u] = true;
                    _tempBoolValues[e.v] = true;
                }
                else
                {
                    contractedGraphStack.back().addEdge(e.u, e.v);
                }
            }

            for (NetworKit::node v : currentGraph.readGraph().nodeRange())
            {
                if (_tempBoolValues[v] || currentGraph.readGraph().degree(v) > k + 2)
                    continue;

                unsigned contractToNeighbour = dis(randomizer) % currentGraph.readGraph().degree(v);

                contractedGraphStack.back().contractNodes(
                    v, currentGraph.readGraph().getIthNeighbor(v, contractToNeighbour));

                _tempBoolValues[v] = true;
                _tempBoolValues[currentGraph.readGraph().getIthNeighbor(v, contractToNeighbour)] = true;
            }

            for (NetworKit::node v : currentGraph.readGraph().nodeRange())
            {
                _tempBoolValues[v] = false;
            }

            // Catches case that there are too many negative edges
            if (contractedGraphStack.back().numNodes() < 2)
            {
                assertHelpersAreClean();
                return false;
            }

            while (!contractedGraphStack.empty())
            {
                ContractorGraph contractedGraph = contractedGraphStack.back();
                contractedGraphStack.pop_back();

                // TODO Performance check if other splits work better here
                if (contractedGraph.numNodes() > 6)
                {
                    // Contract the graph some more
                    contractedGraph.reseed(dis(randomizer));

                    unsigned int t =
                        contractedGraph.numNodes() - 2 -
                        std::ceil(static_cast<float>(contractedGraph.numNodes()) / 3.0);

                    bool hadEdgesToContract = contractedGraph.contractKEdges(t);

                    assert(hadEdgesToContract);
                    assert(contractedGraph.numNodes() >= 2);

                    contractedGraphStack.emplace_back(contractedGraph);
                    contractedGraphStack.emplace_back(contractedGraph);
                }
                else
                {
                    assertHelpersAreClean();

                    unsigned int t = contractedGraph.numNodes() - 2;
                    bool hadEdgesToCOntract = contractedGraph.contractKEdges(t);

                    assert(hadEdgesToCOntract);

                    // See if we found a cur that is a 2 layer separator
                    assert(contractedGraph.numNodes() == 2);

                    const std::vector<NetworKit::Edge>& edges =
                        contractedGraph.currentEdges();

#ifndef NDEBUG
                    std::cout << "\nFastCut found cutset with " << edges.size() << "\n";
                    std::cout << "Nodes on side A: " << contractedGraph.contractedSets()[0].size() <<
                        ", Nodes on side B: " << contractedGraph.contractedSets()[1].size() << "\n";
#endif

                    if (edges.size() > (k - 1) * (k - 1))
                    {
#ifndef NDEBUG
                        std::cout << "Aborting due to too many edges in cutset\n";
#endif
                        continue;
                    }

                    // Figure out which vertices are the sides of the two layer connector
                    std::vector<NetworKit::node> boundaryVertices;

                    for (const NetworKit::Edge e : edges)
                    {
                        if (!_tempBoolValues[e.u])
                        {
                            _tempBoolValues[e.u] = true;
                            boundaryVertices.emplace_back(e.u);
                        }
                        if (!_tempBoolValues[e.v])
                        {
                            _tempBoolValues[e.v] = true;
                            boundaryVertices.emplace_back(e.v);
                        }
                    }

                    for (NetworKit::node v : boundaryVertices)
                    {
                        _tempBoolValues[v] = false;
                    }

                    assert(!boundaryVertices.empty());

#ifndef NDEBUG
                    std::cout << "Cutset has " << boundaryVertices.size() << " vertices\n";
#endif

                    // Definitely not splittable
                    if ((boundaryVertices.size() > 2 * k - 2))
                    {
#ifndef NDEBUG
                        std::cout << "Aborting due to too many boundary vertices\n";
#endif
                        continue;
                    }
                    // We now use tempNodeValues to assign labels 0,...,size to the boundary
                    // vertices and construct the graph induced by them
                    // TODO maybe this can be done cleaner by the subgraph function on
                    // currentGraph
                    NetworKit::node downLabel = 0;
                    std::vector<NetworKit::node> originalID(boundaryVertices.size());

                    for (NetworKit::node v : boundaryVertices)
                    {
                        _tempNodeValues[v] = downLabel;
                        originalID[downLabel] = v;
                        downLabel++;
                    }

                    NetworKit::Graph twoLayerConnector(boundaryVertices.size());

                    for (NetworKit::Edge e : edges)
                    {
                        assert(!twoLayerConnector.hasEdge(_tempNodeValues[e.u],
                            _tempNodeValues[e.v]));
                        twoLayerConnector.addEdge(_tempNodeValues[e.u], _tempNodeValues[e.v]);
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
                            sideABoundary.emplace_back(v);
                        }
                        else
                        {
                            sideBBoundary.emplace_back(v);
                        }
                    }

#ifndef NDEBUG
                    std::cout << "TLS has " << sides[0].size() << " vertices on side A with " << sideABoundary.size() <<
                        " in the boundary\n";
                    if (sides[0].size() == 1)
                        std::cout << "Vertex in side A: " << sides[0][0] << "\n";
                    std::cout << "TLS has " << sides[1].size() << " vertices on side B with " << sideBBoundary.size() <<
                        " in the boundary\n";
                    if (sides[1].size() == 1)
                        std::cout << "Vertex in side B: " << sides[1][0] << "\n";
#endif

                    // Condition to skip separator
                    if ((sideABoundary.size() > k - 1) || (sideBBoundary.size() > k - 1))
                    {
                        for (NetworKit::node v : boundaryVertices)
                        {
                            _tempNodeValues[v] = NetworKit::none;
                        }
#ifndef NDEBUG
                        std::cout << "Aborting due to too many vertices in one boundary\n";
#endif
                        continue;
                    }

                    bool canSplit = false;

                    if (twoLayerConnector.numberOfEdges() < k)
                    {
                        canSplit = true;
                    }

                    std::vector<NetworKit::node> sideABoundaryInSmallgraph;

                    for (NetworKit::node v : sideABoundary)
                    {
                        sideABoundaryInSmallgraph.emplace_back(_tempNodeValues[v]);
                    }

                    NetworKit::Graph complement =
                        bipartiteComplement(twoLayerConnector, sideABoundaryInSmallgraph);

                    if (!canSplit)
                    {
                        BipartiteMatching matching(complement, sideABoundaryInSmallgraph);
                        matching.run();
                        if (static_cast<int>(matching.getMatching().size()) >
                            -1 + 2 * (static_cast<int>(boundaryVertices.size()) -
                                static_cast<int>(k)))
                        {
                            canSplit = true;
                        }
                    }

                    for (NetworKit::node v : boundaryVertices)
                    {
                        _tempNodeValues[v] = NetworKit::none;
                    }

                    assertHelpersAreClean();

                    // Check if matching is of sufficient size
                    if (canSplit)
                    {
                        std::pair<NetworKit::Graph, std::vector<NetworKit::node>> subgraphA =
                            getInducedSubgraph(currentGraph.readGraph(), sides[0], _tempNodeValues);
                        std::pair<NetworKit::Graph, std::vector<NetworKit::node>> subgraphB =
                            getInducedSubgraph(currentGraph.readGraph(), sides[1], _tempNodeValues);

                        std::shared_ptr<DataReductionReconstructor>
                            childRuleA = std::make_shared<DataReductionDummy>(),
                            childRuleB = std::make_shared<DataReductionDummy>();

                        graph_queue.emplace_back(QueuedGraph(subgraphA.first, k, childRuleA));
                        graph_queue.emplace_back(QueuedGraph(subgraphB.first, k, childRuleB));

                        std::shared_ptr<TwoLayerReconstructor> reconstructor =
                            std::make_shared<TwoLayerReconstructor>(
                                k, twoLayerConnector, sideABoundaryInSmallgraph, originalID,
                                currentGraph.readGraph().upperNodeIdBound(), subgraphA.second,
                                subgraphB.second, childRuleA, childRuleB);

                        currentGraph.attachNewRule(reconstructor);

                        // TODO Performance this weight can probably be obtained easier
                        for (NetworKit::Edge e : edges)
                        {
                            objectiveOffset += currentGraph.readGraph().weight(e.u, e.v);
                        }

                        stats.twoLayerConnector +=
                            {0, static_cast<int>(twoLayerConnector.numberOfEdges())};
                        stats.twoLayerSplits += 1;

                        assertHelpersAreClean();

#ifndef NDEBUG
                        std::cout << "Split\n\n";
#endif
                        return true;
                    }
#ifndef NDEBUG
                    std::cout << "Didn't split\n\n";
#endif
                }
            }
        }

        return false;
    }
} // namespace mkcp
