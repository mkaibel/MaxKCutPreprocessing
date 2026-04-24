#include "preprocessing/dominatingEdges.hpp"
#include "networkit/Globals.hpp"
#include "networkit/graph/Graph.hpp"
#include "preprocessing/maxKCutPreprocessor.hpp"
#include "preprocessing/maxKCutPreprocessorUtils.hpp"
#include <cassert>
#include <cstdlib>
#include <memory>

namespace mkcp {

NegativeDominatingEdgesReconstructor::NegativeDominatingEdgesReconstructor() {}

std::shared_ptr<Partition>
NegativeDominatingEdgesReconstructor::getPartition() const {
  if (!calledReconstruction) {
    throw std::runtime_error(
        "Can't access partition before calling reconstructor function");
  }

  return partition;
}

void NegativeDominatingEdgesReconstructor::reconstructData() {
  calledReconstruction = true;

  assert(childRule.size() == 1);
  partition = childRule[0]->getPartition();
  assert(partition.get() != nullptr);

  while (!contractedNodes.empty()) {
    auto [remained, removed] = contractedNodes.back();
    contractedNodes.pop_back();

    partition->assignElement(removed, partition->assignmentOf(remained));
  }
}

const std::vector<std::shared_ptr<DataReductionReconstructor>> &
NegativeDominatingEdgesReconstructor::getChildren() const {
  return childRule;
}

void NegativeDominatingEdgesReconstructor::addChildReduction(
    std::shared_ptr<DataReductionReconstructor> &child) {
  if (!childRule.empty()) {
    throw std::runtime_error(
        "Attempted to add a second child rule to a non-split rule");
  }
  childRule.emplace_back(std::shared_ptr<DataReductionReconstructor>(child));
}

void NegativeDominatingEdgesReconstructor::contractedVertices(
    NetworKit::node remainingVertex, NetworKit::node removedVertex) {
  contractedNodes.emplace_back(remainingVertex, removedVertex);
}

bool DataReducer::contractNegativeEdges(QueuedGraph &currentGraph) {
  if (!config.contractNegativeEdges || !currentGraph.flagActive(RuleFlag::NegativeDominatingEdge))
    return false;

  bool reducedAnything = false;

  std::shared_ptr<NegativeDominatingEdgesReconstructor> reconstructor =
      std::make_shared<NegativeDominatingEdgesReconstructor>();

  while (!currentGraph.negativeDominatingCandidates().empty()) {
    NetworKit::node v = currentGraph.negativeDominatingCandidates().back();
    currentGraph.markVertexRule(RuleFlag::NegativeDominatingEdge, v, false);

    if (currentGraph.readGraph().degree(v) == 0)
      continue;

    NetworKit::WeightedEdge mostNegativeEdge = {NetworKit::none,
                                                NetworKit::none, 0.0};

    NetworKit::edgeweight sumOfAbsouteWeights = 0.0;

    for (auto [neighbour, weight] :
         currentGraph.readGraph().weightNeighborRange(v)) {
      if (weight < mostNegativeEdge.weight) {
        mostNegativeEdge = {v, neighbour, weight};
      }

      sumOfAbsouteWeights += std::abs(weight);
    }

    if ((mostNegativeEdge.weight < 0) &&
        (sumOfAbsouteWeights + 2 * mostNegativeEdge.weight <= 0)) {
      reducedAnything = true;

      stats.dominatingEdges +=
          currentGraph.contractNodes(mostNegativeEdge.u, mostNegativeEdge.v);

      reconstructor->contractedVertices(mostNegativeEdge.u, mostNegativeEdge.v);
    }
  }

  if (reducedAnything) {
    currentGraph.attachNewRule(reconstructor);
  }

  return reducedAnything;
}

bool DataReducer::contractNegativeTriangles(QueuedGraph &currentGraph) {
  if (!config.contractNegativeTriangles || !currentGraph.flagActive(RuleFlag::Triangle))
    return false;

  assertHelpersAreClean();

  bool reducedAnything = false;

  std::shared_ptr<NegativeDominatingEdgesReconstructor> reconstructor =
      std::make_shared<NegativeDominatingEdgesReconstructor>();

  while (!currentGraph.negativeDominatingTriangleCandidates().empty()) {
    NetworKit::node u =
        currentGraph.negativeDominatingTriangleCandidates().back();
    currentGraph.markVertexRule(RuleFlag::Triangle, u, false);

    bool contractedForU = false;

    if (currentGraph.readGraph().degree(u) < 2 ||
        currentGraph.locallyPositive(u))
      continue;

    NetworKit::edgeweight uSumOfAbsoluteWeights = 0.0;

    // Compute sum of absolute edge weights of u and all neighbours that may
    // form a negative triangle
    for (auto [neighbour, weight] :
         currentGraph.readGraph().weightNeighborRange(u)) {
      uSumOfAbsoluteWeights += std::abs(weight);

      if (weight < 0.0) {
        _tempBoolValues[neighbour] = true;

        for (auto [x, w] :
             currentGraph.readGraph().weightNeighborRange(neighbour)) {
          _tempWeightValues[neighbour] += std::abs(w);
        }
      }
    }

    for (auto [v, UVweight] : currentGraph.readGraph().weightNeighborRange(u)) {
      // If v is flagged for triangle checking, then we can find the triangle
      // from v later
      if (UVweight > 0.0 ||
          currentGraph.flagActiveForVertex(RuleFlag::Triangle, v))
        continue;

      for (auto [w, VWweight] :
           currentGraph.readGraph().weightNeighborRange(v)) {
        if (VWweight > 0.0 || !_tempBoolValues[w] ||
            currentGraph.flagActiveForVertex(RuleFlag::Triangle, w))
          continue;

        NetworKit::edgeweight UWweight = currentGraph.readGraph().weight(u, w);

        assert(UVweight < 0.0);
        assert(UWweight < 0.0);
        assert(VWweight < 0.0);

        // Check if any edge in the triangle can be contracted
        // Check if {u, v} dominates the triangle
        if ((-2.0 * UVweight - 1.0 * UWweight >= uSumOfAbsoluteWeights) &&
            (-2.0 * UVweight - 1.0 * VWweight >= _tempWeightValues[v])) {
          reducedAnything = true;

          reconstructor->contractedVertices(u, v);
          stats.dominatingTriangles += currentGraph.contractNodes(u, v);
          cleanVertexHelpers(v);

          contractedForU = true;
          break;
        }
        // Check if {u, w} dominates the triangle
        if ((-2.0 * UWweight - 1.0 * UVweight >= uSumOfAbsoluteWeights) &&
            (-2.0 * UWweight - 1.0 * VWweight >= _tempWeightValues[w])) {
          reducedAnything = true;

          reconstructor->contractedVertices(u, w);
          stats.dominatingTriangles += currentGraph.contractNodes(u, w);
          cleanVertexHelpers(w);

          contractedForU = true;
          break;
        }
        // Check if {v, w} dominates the triangle
        if ((-2.0 * VWweight - 1.0 * UVweight >= _tempWeightValues[v]) &&
            (-2.0 * VWweight - 1.0 * UWweight >= _tempWeightValues[w])) {
          reducedAnything = true;

          reconstructor->contractedVertices(v, w);
          stats.dominatingTriangles += currentGraph.contractNodes(v, w);
          cleanVertexHelpers(w);

          contractedForU = true;
          currentGraph.markVertexRule(RuleFlag::Triangle, u, true); // As nothing changed for u it may still be part of other triangles for which we can apply a contraction rule
          break;
        }
      }

      if (contractedForU)
        break;
    }

    for (NetworKit::node neighbour :
         currentGraph.readGraph().neighborRange(u)) {
      _tempBoolValues[neighbour] = false;
      _tempWeightValues[neighbour] = 0.0;
    }

    assertHelpersAreClean();
  }

  if (reducedAnything) {
    currentGraph.attachNewRule(reconstructor);
  }

  assertHelpersAreClean();

  return reducedAnything;
}

// TODO add the part where the rule is applied
} // namespace mkcp
