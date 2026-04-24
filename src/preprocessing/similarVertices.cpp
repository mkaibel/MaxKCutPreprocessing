#include "preprocessing/similarVertices.hpp"
#include "networkit/Globals.hpp"
#include "preprocessing/maxKCutPreprocessor.hpp"
#include "preprocessing/maxKCutPreprocessorUtils.hpp"
#include <cassert>
#include <deque>
#include <memory>
#include <vector>

namespace mkcp {

SimilarVerticesReconstructor::SimilarVerticesReconstructor() {}

std::shared_ptr<Partition> SimilarVerticesReconstructor::getPartition() const {
  if (!calledReconstruction) {
    throw std::runtime_error(
        "Can't access partition before calling reconstructor function");
  }

  return partition;
}

void SimilarVerticesReconstructor::reconstructData() {
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
SimilarVerticesReconstructor::getChildren() const {
  return childRule;
}

void SimilarVerticesReconstructor::addChildReduction(
    std::shared_ptr<DataReductionReconstructor> &child) {
  if (!childRule.empty()) {
    throw std::runtime_error(
        "Attempted to add a second child rule to a non-split rule");
  }
  childRule.emplace_back(std::shared_ptr<DataReductionReconstructor>(child));
}

void SimilarVerticesReconstructor::similarVertices(
    NetworKit::node remainingVertex, NetworKit::node removedVertex) {
  contractedNodes.emplace_back(remainingVertex, removedVertex);
}

bool DataReducer::contractConnectedSimilarVertices(QueuedGraph &currentGraph) {
  assertHelpersAreClean();

  if (!config.contractNegativeTriangles || !currentGraph.flagActive(RuleFlag::ConnectedSimilarVertices)) {
    return false;
  }

  bool reducedAnything = false;

  std::shared_ptr<SimilarVerticesReconstructor> reconstructor =
      std::make_shared<SimilarVerticesReconstructor>();

  while (!currentGraph.connectedSimilarVerticesCandidates().empty()) {
    NetworKit::node v =
        currentGraph.connectedSimilarVerticesCandidates().back();
    // Will be reactivated if vertex is contracted
    currentGraph.markVertexRule(ConnectedSimilarVertices, v, false);

    for (auto [neighbour, weight] :
         currentGraph.readGraph().weightNeighborRange(v)) {
      _tempWeightValues[neighbour] = weight;
    }

    for (auto [neighbour, weight] :
         currentGraph.readGraph().weightNeighborRange(v)) {
      // Edge either can't be contracted because it's positive, or won't be
      // contracted
      if (weight > 0.0 || currentGraph.flagActiveForVertex(
                              RuleFlag::ConnectedSimilarVertices, neighbour))
        continue;

      if (alphaSimilarNeighbourhood(currentGraph, v, neighbour,
                                    _tempWeightValues) > 0.0) {
        reducedAnything = true;

        stats.connectedSimilarVertices +=
            currentGraph.contractNodes(v, neighbour);
        _tempWeightValues[neighbour] = 0.0; // Necessary as neighbour is deleted and therefore no longer a neighbour of v during clean up

        reconstructor->similarVertices(v, neighbour);

        break;
      }
    }

    for (NetworKit::node neighbour :
         currentGraph.readGraph().neighborRange(v)) {
      _tempWeightValues[neighbour] = 0.0;
    }
  }

  if (reducedAnything) {
    currentGraph.attachNewRule(reconstructor);
  }

  assertHelpersAreClean();

  return reducedAnything;
}

bool DataReducer::contractDisconnectedSimilarVertices(
    QueuedGraph &currentGraph) {
  assertHelpersAreClean();
  if (!config.contractSimilarVertices || !currentGraph.flagActive(RuleFlag::SimilarVertices)) {
    return false;
  }

  bool reducedAnything = false;

  std::shared_ptr<SimilarVerticesReconstructor> reconstructor =
      std::make_shared<SimilarVerticesReconstructor>();

  // We separate nodes into candidate sets by size of neighbourhood and hash of
  // neighbouring vertices, which we store in the temp arrays
  for (NetworKit::node v : currentGraph.readGraph().nodeRange()) {
    _tempCountValues[v] = currentGraph.readGraph().degree(v);
    _tempNodeValues[v] = v;
    for (NetworKit::node neighbour :
         currentGraph.readGraph().neighborRange(v)) {
      _tempNodeValues[v] ^= neighbour;
    }
  }

  std::vector<std::vector<NetworKit::node>> groupingsByHash(1);
  std::vector<std::vector<NetworKit::node>> subGroupingsBySize(1);
  NetworKit::node maxHashValue = 0;
  std::deque<NetworKit::node> hashesUsed;

  for (NetworKit::node v : currentGraph.readGraph().nodeRange()) {
    if (_tempNodeValues[v] > maxHashValue) {
      maxHashValue = _tempNodeValues[v];
      groupingsByHash.resize(maxHashValue + 1, std::vector<NetworKit::node>());
    }

    if (groupingsByHash[_tempNodeValues[v]].empty()) {
      hashesUsed.emplace_back(_tempNodeValues[v]);
    }

    groupingsByHash[_tempNodeValues[v]].emplace_back(v);
  }

  NetworKit::count maxSizeValue = 0;

  std::deque<NetworKit::node> removedNodes;

  for (NetworKit::node hash : hashesUsed) {
    std::deque<NetworKit::count> sizesUsed;

    for (NetworKit::node v : groupingsByHash[hash]) {
      if (_tempCountValues[v] > maxSizeValue) {
        maxSizeValue = _tempCountValues[v];
        subGroupingsBySize.resize(maxSizeValue + 1);
      }

      if (subGroupingsBySize[_tempCountValues[v]].empty()) {
        sizesUsed.emplace_back(_tempCountValues[v]);
      }

      subGroupingsBySize[_tempCountValues[v]].emplace_back(v);
    }

    // Now we have promising candidate sets
    for (NetworKit::count d : sizesUsed) {
      for (unsigned int i = 0; i < subGroupingsBySize[d].size(); i++) {
        // Check if the vertex has already been merged into another
        if (_tempBoolValues[subGroupingsBySize[d][i]])
          continue;

        for (auto [neighbour, weight] :
             currentGraph.readGraph().weightNeighborRange(
                 subGroupingsBySize[d][i])) {
          _tempWeightValues[neighbour] = weight;
        }

        for (unsigned int j = i + 1; j < subGroupingsBySize[d].size(); j++) {
          // Skip vertices that have already been removed or are neighbouring to
          // the vertex in question
          if (_tempBoolValues[subGroupingsBySize[d][j]] ||
              _tempWeightValues[subGroupingsBySize[d][j]] != 0.0)
            continue;

          if (alphaSimilarNeighbourhood(currentGraph, subGroupingsBySize[d][i],
                                        subGroupingsBySize[d][j],
                                        _tempWeightValues) > 0.0) {
            reducedAnything = true;
            _tempBoolValues[subGroupingsBySize[d][j]] = true;

            stats.disconnectedSimilarVertices += currentGraph.contractNodes(
                subGroupingsBySize[d][i], subGroupingsBySize[d][j]);

            removedNodes.emplace_back(subGroupingsBySize[d][j]);

            reconstructor->similarVertices(subGroupingsBySize[d][i],
                                           subGroupingsBySize[d][j]);
          }
        }

        for (NetworKit::node neighbour :
             currentGraph.readGraph().neighborRange(subGroupingsBySize[d][i])) {
          _tempWeightValues[neighbour] = 0.0;
        }
      }
    }

    for (NetworKit::count size : sizesUsed) {
      subGroupingsBySize[size].clear();
    }
  }

  for (NetworKit::node v : currentGraph.readGraph().nodeRange()) {
    _tempNodeValues[v] = NetworKit::none;
    _tempCountValues[v] = 0;
    _tempBoolValues[v] = false;
  }

  for (NetworKit::node v : removedNodes) {
    _tempNodeValues[v] = NetworKit::none;
    _tempCountValues[v] = 0;
    _tempBoolValues[v] = false;
  }

  assertHelpersAreClean();

  if (reducedAnything) {
    currentGraph.attachNewRule(reconstructor);
  }

  return reducedAnything;
}

NetworKit::edgeweight DataReducer::alphaSimilarNeighbourhood(
    const QueuedGraph &currentGraph, NetworKit::node v,
    NetworKit::node neighbour,
    const std::vector<NetworKit::edgeweight> &weights) {
  NetworKit::edgeweight alpha = 0.0;

  if (currentGraph.readGraph().degree(v) !=
      currentGraph.readGraph().degree(neighbour)) {
    return 0.0;
  }

  for (auto [x, weight] :
       currentGraph.readGraph().weightNeighborRange(neighbour)) {
    if (x == v)
      continue;

    if (weights[x] == 0.0) {
      // x is not a neighbour of v
      return 0.0;
    }

    assert(weight != 0.0);

    if (alpha == 0.0) {
      alpha = weight / weights[x];
    } else if (alpha != (weight / weights[x])) {
      // No consistent scaling in neighbour weights
      return 0.0;
    }
  }

  return alpha;
}

} // namespace mkcp
