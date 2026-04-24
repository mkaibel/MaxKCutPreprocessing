#include "preprocessing/cliqueRules.hpp"
#include "networkit/Globals.hpp"

#include "globals.hpp"
#include "preprocessing/maxKCutPreprocessor.hpp"
#include "preprocessing/maxKCutPreprocessorUtils.hpp"
#include "util/subgraphUtils.hpp"

#include <cassert>
#include <cmath>
#include <memory>
#include <vector>

namespace mkcp {

std::shared_ptr<Partition> CliqueReductionReconstructor::getPartition() const {
  if (!calledReconstruction) {
    throw std::runtime_error(
        "Can't access partition before calling reconstructor function");
  }

  return partition;
}

void CliqueReductionReconstructor::reconstructData() {
  calledReconstruction = true;

  assert(childRule.size() == 1);
  partition = childRule[0]->getPartition();
  assert(partition.get() != nullptr);

  NetworKit::count cliqueSize = internalNodes.size() + externalNodes.size();
  NetworKit::count minVerticesPerClique = cliqueSize / k;

  std::vector<NetworKit::count> verticesPerColour(k, 0);

  for (NetworKit::node v : externalNodes) {
    assert(partition->isAssigned(v));

    verticesPerColour[partition->assignmentOf(v)]++;
  }

  for (partitionID c = 0; c < k; c++) {
    assert(verticesPerColour[c] <= ((cliqueSize + k - 1) / k));
  }

  partitionID currentPartition = 0;
  while (currentPartition < k &&
         verticesPerColour[currentPartition] >= minVerticesPerClique) {
    currentPartition++;
  }
  bool allMinSizesReached = false;

  if (currentPartition == k) {
    // This occurs if the clique was very small (and had no external vertices)
    allMinSizesReached = true;
    currentPartition = 0;
    while (verticesPerColour[currentPartition] > minVerticesPerClique)
      currentPartition++;
  }

  for (NetworKit::node v : internalNodes) {
    if (allMinSizesReached) {
      // Add element to current partition, which has minSize
      partition->assignElement(v, currentPartition);
      verticesPerColour[currentPartition]++;

      // Find next partition that can fit one more element
      while (verticesPerColour[currentPartition] > minVerticesPerClique)
        currentPartition++;
    } else {
      partition->assignElement(v, currentPartition);
      verticesPerColour[currentPartition]++;

      // Find next partition still lacking vertices
      for (; currentPartition < k &&
             verticesPerColour[currentPartition] >= minVerticesPerClique;
           currentPartition++)
        ;

      if (currentPartition == k) {
        allMinSizesReached = true;
        currentPartition = 0;

        while (verticesPerColour[currentPartition] > minVerticesPerClique)
          currentPartition++;
      }
    }
  }
}

const std::vector<std::shared_ptr<DataReductionReconstructor>> &
CliqueReductionReconstructor::getChildren() const {
  return childRule;
}

void CliqueReductionReconstructor::addChildReduction(
    std::shared_ptr<DataReductionReconstructor> &child) {
  if (!childRule.empty()) {
    throw std::runtime_error(
        "Attempted to add a second child rule to a non-split rule");
  }
  childRule.emplace_back(std::shared_ptr<DataReductionReconstructor>(child));
}

bool DataReducer::removeFullCliques(QueuedGraph &currentGraph) {
  if (!config.removeCliques || !currentGraph.flagActive(RuleFlag::FullClique))
    return false;
  assertHelpersAreClean();

  bool reducedAnything = false;

  // std::shared_ptr<CliqueReductionReconstructor> reconstructor =
  //     std::make_shared<CliqueReductionReconstructor>();

  while (!currentGraph.fullCliqueCandidates().empty()) {
    NetworKit::node v = currentGraph.fullCliqueCandidates().back();
    currentGraph.markVertexRule(RuleFlag::FullClique, v, false);

    assert(currentGraph.readGraph().hasNode(v));

    if (currentGraph.readGraph().degree(v) == 0)
      continue;

    if (!(currentGraph.locallyUnitWeight(v) && currentGraph.locallyPositive(v)))
      continue;

    bool internalVerticesGood = true;
    std::vector<NetworKit::node> internalVertices(1, v);
    std::vector<NetworKit::node> externalVertices;

    for (NetworKit::node w : currentGraph.readGraph().neighborRange(v)) {
      if (currentGraph.readGraph().degree(w) >
          currentGraph.readGraph().degree(v)) {
        currentGraph.markVertexRule(RuleFlag::FullClique, w, false);
        currentGraph.markVertexRule(RuleFlag::Clique, w, false);

        externalVertices.emplace_back(w);
      } else if (currentGraph.readGraph().degree(w) ==
                 currentGraph.readGraph().degree(v)) {
        if (currentGraph.locallyUnitWeight(w) &&
            currentGraph.locallyPositive(w)) {
          currentGraph.markVertexRule(RuleFlag::FullClique, w, false);
          internalVertices.emplace_back(w);

        } else {
          currentGraph.markVertexRule(RuleFlag::Clique, v, false);
          currentGraph.markVertexRule(RuleFlag::FullClique, w, false);
          currentGraph.markVertexRule(RuleFlag::Clique, w, false);

          internalVerticesGood = false;
          break;
        }
      } else {
        internalVerticesGood = false;
        currentGraph.markVertexRule(RuleFlag::Clique, v, false);
        break;
      }
    }

    if (!internalVerticesGood) {
      continue;
    }

    if (externalVertices.size() >
        ((internalVertices.size() + externalVertices.size() + k - 1) / k)) {
      continue;
    }

    NetworKit::edgeweight cliqueWeight =
        currentGraph.readGraph().getIthNeighborWeight(v, 0);

    std::vector<NetworKit::node> fullVertexSet(internalVertices);
    fullVertexSet.reserve(internalVertices.size() + externalVertices.size());

    for (NetworKit::node w : externalVertices)
      fullVertexSet.emplace_back(w);

    if (subgraphIsClique(currentGraph.readGraph(), fullVertexSet,
                         _tempBoolValues, cliqueWeight)) {
      // Compute Offset
      unsigned numNodes = currentGraph.readGraph().degree(v) + 1;
      unsigned groupSizeRoundedDown = numNodes / k;
      unsigned numEdgesCut =
          (numNodes * (numNodes - 1) -
           (k - (numNodes % k)) * groupSizeRoundedDown *
               (groupSizeRoundedDown - 1) -
           (numNodes % k) * (groupSizeRoundedDown + 1) * groupSizeRoundedDown) /
          2;
      objectiveOffset +=
          static_cast<NetworKit::edgeweight>(numEdgesCut) * cliqueWeight;

      for (NetworKit::node w : internalVertices) {
        stats.fullClique += currentGraph.deleteVertex(w);
      }

      for (unsigned int i = 0; i < externalVertices.size(); i++) {
        for (unsigned int j = i + 1; j < externalVertices.size(); j++) {
          stats.fullClique +=
              currentGraph.deleteEdge(externalVertices[i], externalVertices[j]);
        }
      }

      currentGraph.attachNewRule(std::make_shared<CliqueReductionReconstructor>(
          internalVertices, externalVertices, k));
    }
  }

  assertHelpersAreClean();
  return reducedAnything;
}

bool DataReducer::removeCliques(QueuedGraph &currentGraph) {
  if (!config.removeCliques || !currentGraph.flagActive(RuleFlag::Clique))
    return false;
  assertHelpersAreClean();

  bool reducedAnything = false;

  // std::shared_ptr<CliqueReductionReconstructor> reconstructor =
  //     std::make_shared<CliqueReductionReconstructor>();

  while (!currentGraph.cliqueCandidates().empty()) {
    NetworKit::node v = currentGraph.cliqueCandidates().back();
    currentGraph.markVertexRule(RuleFlag::Clique, v, false);

    assert(currentGraph.readGraph().hasNode(v));

    if (currentGraph.readGraph().degree(v) == 0)
      continue;

    if (!(currentGraph.locallyUnitWeight(v) && currentGraph.locallyPositive(v)))
      continue;

    bool internalVerticesGood = true;
    std::vector<NetworKit::node> internalVertices(1, v);
    std::vector<NetworKit::node> externalVertices;

    for (NetworKit::node w : currentGraph.readGraph().neighborRange(v)) {
      if (currentGraph.readGraph().degree(w) >
          currentGraph.readGraph().degree(v)) {
        currentGraph.markVertexRule(RuleFlag::FullClique, w, false);
        currentGraph.markVertexRule(RuleFlag::Clique, w, false);

        externalVertices.emplace_back(w);
      } else if (currentGraph.readGraph().degree(w) ==
                 currentGraph.readGraph().degree(v)) {
        if (currentGraph.locallyUnitWeight(w) &&
            currentGraph.locallyPositive(w)) {
          currentGraph.markVertexRule(RuleFlag::FullClique, w, false);
          internalVertices.emplace_back(w);

        } else {
          currentGraph.markVertexRule(RuleFlag::FullClique, v, false);
          currentGraph.markVertexRule(RuleFlag::FullClique, w, false);
          currentGraph.markVertexRule(RuleFlag::Clique, w, false);

          internalVerticesGood = false;
          break;
        }
      } else {
        internalVerticesGood = false;
        currentGraph.markVertexRule(RuleFlag::Clique, v, false);
        break;
      }
    }

    if (!internalVerticesGood) {
      continue;
    }

    if (externalVertices.size() >
        ((internalVertices.size() + externalVertices.size() + k - 1) / k)) {
      continue;
    }

    for (NetworKit::node w : internalVertices) {
      _tempBoolValues[w] = true;
    }
    for (NetworKit::node w : externalVertices) {
      _tempBoolValues[w] = true;
    }

    bool noNeighboursButExternal = true;

    // TODO Performance: Maybe skip here once the first counterexample is found
    for (NetworKit::node w : internalVertices) {
      for (NetworKit::node x : currentGraph.readGraph().neighborRange(w)) {
        noNeighboursButExternal &= _tempBoolValues[x];
      }
    }

    for (NetworKit::node w : internalVertices) {
      _tempBoolValues[w] = false;
    }
    for (NetworKit::node w : externalVertices) {
      _tempBoolValues[w] = false;
    }

    assertHelpersAreClean();

    NetworKit::edgeweight cliqueWeight =
        currentGraph.readGraph().getIthNeighborWeight(v, 0);

    if (noNeighboursButExternal &&
        subgraphIsClique(currentGraph.readGraph(), internalVertices,
                         _tempBoolValues, cliqueWeight)) {
      // Compute Offset
      unsigned numNodes = currentGraph.readGraph().degree(v) + 1;
      unsigned groupSizeRoundedDown = numNodes / k;
      unsigned numEdgesCut =
          (numNodes * (numNodes - 1) -
           (k - (numNodes % k)) * groupSizeRoundedDown * (groupSizeRoundedDown - 1) -
           (numNodes % k) * (groupSizeRoundedDown + 1) *
               groupSizeRoundedDown) /
          2;
      objectiveOffset +=
          static_cast<NetworKit::edgeweight>(numEdgesCut) * cliqueWeight;

      for (NetworKit::node w : internalVertices) {
        stats.clique += currentGraph.deleteVertex(w);
      }

      for (unsigned int i = 0; i < externalVertices.size(); i++) {
        for (unsigned int j = i + 1; j < externalVertices.size(); j++) {
          stats.clique += currentGraph.changeEdgeWeight(
              externalVertices[i], externalVertices[j],
              currentGraph.readGraph().weight(externalVertices[i],
                                              externalVertices[j]) -
                  cliqueWeight);
        }
      }

      currentGraph.attachNewRule(std::make_shared<CliqueReductionReconstructor>(
          internalVertices, externalVertices, k));
    }
  }

  assertHelpersAreClean();
  return reducedAnything;
}

} // namespace mkcp
