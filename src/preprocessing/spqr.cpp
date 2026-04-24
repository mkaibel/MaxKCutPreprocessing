#include "preprocessing/spqr.hpp"
#include "globals.hpp"
#include "networkit/Globals.hpp"
#include "networkit/graph/Graph.hpp"
#include "ogdf/basic/Graph_d.h"
#include "ogdf/decomposition/SPQRTree.h"
#include "ogdf/decomposition/StaticSPQRTree.h"
#include "preprocessing/maxKCutPreprocessor.hpp"
#include "preprocessing/maxKCutPreprocessorUtils.hpp"
#include "solver/max_k_cut_solver_gurobi.hpp"
#include "util/subgraphUtils.hpp"
#include <cassert>
#include <optional>
#include <utility>
#include <vector>

namespace mkcp
{
    SPQRReconstructor::SPQRReconstructor(Partition connectorSame,
                                         Partition connectorDifferent,
                                         std::vector<NetworKit::node> originalIDs,
                                         NetworKit::node sep1down,
                                         NetworKit::node sep2down)
        : originalIDs(originalIDs), connectorU(sep1down), connectorV(sep2down),
          connectorSame(connectorSame), connectorDifferent(connectorDifferent)
    {
    }

    void SPQRReconstructor::reconstructData()
    {
        calledReconstruction = true;

        assert(childRule.size() == 1);
        partition = childRule[0]->getPartition();
        assert(partition.get() != nullptr);

        std::vector<std::pair<partitionID, partitionID>> partialPermutation;

        // Check which solution we want to extend into
        if (partition->assignmentOf(originalIDs[connectorU]) ==
            partition->assignmentOf(originalIDs[connectorV]))
        {
            partialPermutation.emplace_back(
                connectorSame.assignmentOf(connectorU),
                partition->assignmentOf(originalIDs[connectorU]));

            std::vector<partitionID> permutation =
                completePermutation(partialPermutation);

            connectorSame.swapPartitionNames(permutation);

            for (NetworKit::node v = 0; v < originalIDs.size(); v++)
            {
                assert(partition->assignmentOf(originalIDs[v]) == none ||
                    partition->assignmentOf(originalIDs[v]) ==
                    connectorSame.assignmentOf(v));

                partition->assignElement(originalIDs[v], connectorSame.assignmentOf(v));
            }
        }
        else
        {
            partialPermutation.emplace_back(
                connectorDifferent.assignmentOf(connectorU),
                partition->assignmentOf(originalIDs[connectorU]));
            partialPermutation.emplace_back(
                connectorDifferent.assignmentOf(connectorV),
                partition->assignmentOf(originalIDs[connectorV]));

            std::vector<partitionID> permutation =
                completePermutation(partialPermutation);

            connectorDifferent.swapPartitionNames(permutation);

            for (NetworKit::node v = 0; v < originalIDs.size(); v++)
            {
                assert(partition->assignmentOf(originalIDs[v]) == none ||
                    partition->assignmentOf(originalIDs[v]) ==
                    connectorDifferent.assignmentOf(v));

                partition->assignElement(originalIDs[v],
                                         connectorDifferent.assignmentOf(v));
            }
        }
    }

    std::shared_ptr<Partition> SPQRReconstructor::getPartition() const
    {
        if (!calledReconstruction)
        {
            throw std::runtime_error(
                "Can't access partition before calling reconstructor function");
        }

        return partition;
    }

    const std::vector<std::shared_ptr<DataReductionReconstructor>>&
    SPQRReconstructor::getChildren() const
    {
        return childRule;
    }

    void SPQRReconstructor::addChildReduction(
        std::shared_ptr<DataReductionReconstructor>& child)
    {
        if (!childRule.empty())
        {
            throw std::runtime_error(
                "Attempted to add a second child rule to a non-split rule");
        }
        childRule.emplace_back(std::shared_ptr<DataReductionReconstructor>(child));
    }

    bool DataReducer::spqrReduction(QueuedGraph& currentGraph)
    {
        if (!config.SPQRReduction || !currentGraph.flagActive(RuleFlag::SPQRTree) || currentGraph.readGraph().
            numberOfNodes() <= 2)
        {
            return false;
        }

        // TODO experimental, maybe adjust value in future
        if (currentGraph.readGraph().numberOfNodes() <= 40)
        {
            return false;
        }

        currentGraph.markRule(RuleFlag::SPQRTree, false);

        bool reducedSomething = false;

        SPQRCurrentStatus spqrStatus;

        spqrStatus.nkToOGDFNode.resize(currentGraph.readGraph().upperNodeIdBound());
        spqrStatus.ogdfToNKNode.resize(currentGraph.readGraph().upperNodeIdBound());

        for (NetworKit::node v : currentGraph.readGraph().nodeRange())
        {
            auto w = spqrStatus.ogdfGraph.newNode();
            spqrStatus.nkToOGDFNode[v] = w;
            spqrStatus.ogdfToNKNode[w->index()] = v;
        }

        for (NetworKit::Edge e : currentGraph.readGraph().edgeRange())
        {
            assert(spqrStatus.ogdfGraph.searchEdge(spqrStatus.nkToOGDFNode[e.u],
                    spqrStatus.nkToOGDFNode[e.v]) ==
                nullptr);
            spqrStatus.ogdfGraph.newEdge(spqrStatus.nkToOGDFNode[e.u],
                                         spqrStatus.nkToOGDFNode[e.v]);
        }

        ogdf::StaticSPQRTree tree(spqrStatus.ogdfGraph);

        assert(tree.tree().numberOfNodes() > 0);

        if (tree.tree().numberOfNodes() == 1)
        {
            return false;
        }

        for (auto v : tree.tree().nodes)
        {
            if (static_cast<size_t>(v->index()) >= spqrStatus.spqrNodeStatus.size())
            {
                spqrStatus.spqrNodeStatus.resize(v->index() + 1, 1);
                spqrStatus.spqrNodeDegree.resize(v->index() + 1, 0);
            }

            spqrStatus.spqrNodeDegree[v->index()] = v->degree();
            if (v->degree() == 1)
            {
                spqrStatus.currentLeafs.emplace_back(v);
            }
        }

        bool foundOne = true;

        while (foundOne)
        {
            auto leafParent = findContractibleEdge(spqrStatus);

            if (leafParent.has_value())
            {
                reducedSomething |=
                    processSPQRNode(currentGraph, tree, spqrStatus, leafParent.value());
            }
            else
            {
                foundOne = false;
            }
        }

        return reducedSomething;
    }

    bool DataReducer::processSPQRNode(QueuedGraph& currentGraph,
                                      ogdf::StaticSPQRTree& spqr,
                                      SPQRCurrentStatus& spqrStatus,
                                      LeafParent leafParent)
    {
        // Process "trivial" nodes
        if (spqr.typeOf(leafParent.leaf) == ogdf::SPQRTree::NodeType::PNode)
        {
            // Doesn't need processing, can just be marked as removed
            spqrStatus.spqrNodeStatus[leafParent.leaf->index()] = 0;
            spqrStatus.spqrNodeDegree[leafParent.parent->index()] -= 1;
            if (spqrStatus.spqrNodeStatus[leafParent.parent->index()] == 1)
            {
                spqrStatus.currentLeafs.emplace_back(leafParent.parent);
            }

            return false;
        }

        // Collect the vertices forming the subgraph
        auto& spqrSubgraphNodes = spqr.skeleton(leafParent.leaf);

        // TODO Experimental, adjust size in future maybe?
        if (spqrSubgraphNodes.getGraph().nodes.size() > 40)
        {
            spqrStatus.spqrNodeStatus[leafParent.leaf->index()] = 2;
            return false;
        }

        std::vector<NetworKit::node> nkSubgraphNodes;

        for (ogdf::node v : spqrSubgraphNodes.getGraph().nodes)
        {
            nkSubgraphNodes.emplace_back(
                spqrStatus.ogdfToNKNode[spqrSubgraphNodes.original(v)->index()]);
        }

        // Figure out the two vertices connecting the subgraph to the rest

        ogdf::edge edgeInLeafSkeleton;
        if (leafParent.leaf == leafParent.edge->source())
        {
            edgeInLeafSkeleton = spqr.skeletonEdgeSrc(leafParent.edge);
        }
        else
        {
            edgeInLeafSkeleton = spqr.skeletonEdgeTgt(leafParent.edge);
        }

        auto uInLeafSkeleton =
            spqrSubgraphNodes.original(edgeInLeafSkeleton->source());
        auto vInLeafSkeleton =
            spqrSubgraphNodes.original(edgeInLeafSkeleton->target());
        NetworKit::node sep1orig = spqrStatus.ogdfToNKNode[uInLeafSkeleton->index()];
        NetworKit::node sep2orig = spqrStatus.ogdfToNKNode[vInLeafSkeleton->index()];
        NetworKit::node sep1down = NetworKit::none, sep2down = NetworKit::none;

        auto [subGraph, originalIDs] =
            getInducedSubgraph(currentGraph.readGraph(), nkSubgraphNodes, _tempNodeValues);

        // Figure out the down IDs of the separator vertices
        for (NetworKit::node v = 0; v < originalIDs.size(); v++)
        {
            if (originalIDs[v] == sep1orig)
            {
                sep1down = v;
            }
            else if (originalIDs[v] == sep2orig)
            {
                sep2down = v;
            }
        }

        assert(sep1down != NetworKit::none);
        assert(sep2down != NetworKit::none);

        subGraph.indexEdges();
        subGraph.setKeepEdgesSorted();

        // If necessary, add dummy edge
        if (!subGraph.hasEdge(sep1down, sep2down))
        {
            subGraph.addEdge(sep1down, sep2down, 0.0);
        }

        std::vector<EdgeConstraint> constraints(subGraph.upperEdgeIdBound(),
                                                EdgeConstraint::NONE);

        // Solve case that the separator vertices are in different partitions
        constraints[subGraph.edgeId(sep1down, sep2down)] = EdgeConstraint::IN_CUT;
        MaxKCutSolverGurobi solverEdgeIn(subGraph, k, 1.0);
        solverEdgeIn.setEdgeConstraints(constraints);

        solverEdgeIn.run();

        if (!solverEdgeIn.foundSatisfactorySolution())
        {
            spqrStatus.spqrNodeStatus[leafParent.leaf->index()] = 2;
            return false;
        }

        assert(solverEdgeIn.getSolution().assignmentOf(sep1down) !=
            solverEdgeIn.getSolution().assignmentOf(sep2down));

        // Solve case that the separator vertices are in different partitions
        constraints[subGraph.edgeId(sep1down, sep2down)] = EdgeConstraint::OUT_CUT;
        MaxKCutSolverGurobi solverEdgeOut(subGraph, k, 1.0);
        solverEdgeOut.setEdgeConstraints(constraints);

        solverEdgeOut.run();

        if (!solverEdgeOut.foundSatisfactorySolution())
        {
            spqrStatus.spqrNodeStatus[leafParent.leaf->index()] = 2;
            return false;
        }

        assert(solverEdgeOut.getSolution().assignmentOf(sep1down) ==
            solverEdgeOut.getSolution().assignmentOf(sep2down));

        NetworKit::edgeweight objEdgeIn = solverEdgeIn.getObjectiveValue();
        NetworKit::edgeweight objEdgeOut = solverEdgeOut.getObjectiveValue();

        currentGraph.attachNewRule(std::make_shared<SPQRReconstructor>(
            solverEdgeOut.getSolution(), solverEdgeIn.getSolution(), originalIDs,
            sep1down, sep2down));

        for (NetworKit::node v : nkSubgraphNodes)
        {
            if (v != sep1orig && v != sep2orig)
            {
                stats.spqr += currentGraph.deleteVertex(v);
            }
        }

        objectiveOffset += objEdgeOut;

        stats.spqr +=
            currentGraph.changeEdgeWeight(sep1orig, sep2orig, objEdgeIn - objEdgeOut);

        return true;
    }

    std::optional<LeafParent> findContractibleEdge(SPQRCurrentStatus& status)
    {
        while (!status.currentLeafs.empty())
        {
            ogdf::node v = status.currentLeafs.back();
            status.currentLeafs.pop_back();

            if (status.spqrNodeStatus[v->index()] != 1)
                continue;

            for (auto edge : v->adjEntries)
            {
                if (status.spqrNodeStatus[edge->theEdge()->opposite(v)->index()] > 0)
                {
                    return {LeafParent(v, edge->theEdge()->opposite(v), edge->theEdge())};
                }
            }
        }

        return {};
    }
} // namespace mkcp
