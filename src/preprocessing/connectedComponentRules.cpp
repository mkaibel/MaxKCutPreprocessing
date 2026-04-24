#include <algorithm>
#include <cassert>
#include <deque>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "globals.hpp"
#include "networkit/Globals.hpp"
#include "networkit/components/BiconnectedComponents.hpp"
#include "networkit/components/ConnectedComponents.hpp"
#include "networkit/graph/Graph.hpp"
#include "preprocessing/connectedComponentRules.hpp"
#include "preprocessing/maxKCutPreprocessor.hpp"
#include "preprocessing/maxKCutPreprocessorUtils.hpp"
#include "util/subgraphUtils.hpp"

namespace mkcp {

ConnectedComponentReconstructor::ConnectedComponentReconstructor(
    NetworKit::count maximumNodeID,
    const std::vector<std::vector<NetworKit::node>>& originalIDs,
    const std::vector<std::shared_ptr<DataReductionReconstructor>> &childRule)
    : childRule(childRule), maximumNodeID(maximumNodeID),
      originalIDs(originalIDs) {
  partition = std::make_shared<Partition>(maximumNodeID);

  assert(originalIDs.size() == childRule.size());
}

std::shared_ptr<Partition>
ConnectedComponentReconstructor::getPartition() const {
  if (!calledReconstruction) {
    throw std::runtime_error(
        "Can't access partition before calling reconstructor function");
  }

  return partition;
}

void ConnectedComponentReconstructor::addChildReduction(
    std::shared_ptr<DataReductionReconstructor> &) {
  throw std::runtime_error(
      "component splitting rules don't use add child reduction functions");
}

void ConnectedComponentReconstructor::reconstructData() {
  calledReconstruction = true;

  for (unsigned int i = 0; i < originalIDs.size(); i++) {
    std::shared_ptr<Partition> componentSolution = childRule[i]->getPartition();

    for (NetworKit::node v = 0; v < originalIDs[i].size(); v++) {
      assert(originalIDs[i][v] < maximumNodeID);
      assert(partition->assignmentOf(originalIDs[i][v]) == none);

      partition->assignElement(originalIDs[i][v],
                               componentSolution->assignmentOf(v));
    }
  }
}

const std::vector<std::shared_ptr<DataReductionReconstructor>> &
ConnectedComponentReconstructor::getChildren() const {
  return childRule;
}

bool DataReducer::splitByConnectedComponents(QueuedGraph &currentGraph) {
  NetworKit::ConnectedComponents componentsAlgo(currentGraph.readGraph());

  componentsAlgo.run();

  if (componentsAlgo.numberOfComponents() <= 1) {
    return false;
  }

  stats.connectedComponentSPlits += componentsAlgo.numberOfComponents() - 1;

  std::vector<std::vector<NetworKit::node>> connectedComponents =
      componentsAlgo.getComponents();

  std::vector<std::vector<NetworKit::node>> originalIDs;
  std::vector<std::shared_ptr<DataReductionReconstructor>> childRules;

  for (const auto & connectedComponent : connectedComponents) {
    std::pair<NetworKit::Graph, std::vector<NetworKit::node>> subgraph =
        getInducedSubgraph(currentGraph.readGraph(), connectedComponent, _tempNodeValues);

    childRules.emplace_back(std::make_shared<DataReductionDummy>());
    graph_queue.emplace_back(subgraph.first, k, childRules.back());

    originalIDs.emplace_back(subgraph.second);
  }

  std::shared_ptr<DataReductionReconstructor> reconstructionRule =
      std::make_shared<ConnectedComponentReconstructor>(
          currentGraph.readGraph().upperNodeIdBound(), originalIDs, childRules);

  currentGraph.attachNewRule(reconstructionRule);

  return true;
}

BiconnectedComponentReconstructor::BiconnectedComponentReconstructor(
    NetworKit::count maximumNodeID,
    const std::vector<std::vector<NetworKit::node>>& originalIDs,
    const std::vector<std::shared_ptr<DataReductionReconstructor>> &childRule)
    : childRule(childRule), maximumNodeID(maximumNodeID),
      originalIDs(originalIDs) {
  partition = std::make_shared<Partition>(maximumNodeID);

  assert(originalIDs.size() == childRule.size());
}

std::shared_ptr<Partition>
BiconnectedComponentReconstructor ::getPartition() const {
  if (!calledReconstruction) {
    throw std::runtime_error(
        "Can't access partition before calling reconstructor function");
  }

  return partition;
}

void BiconnectedComponentReconstructor::addChildReduction(
    std::shared_ptr<DataReductionReconstructor> &) {
  throw std::runtime_error(
      "component splitting rules don't use add child reduction functions");
}

void BiconnectedComponentReconstructor::reconstructData() {
  calledReconstruction = true;

  // Construct a proper order in which to reinsert the components
  NetworKit::Graph biConnectedComponentTree(childRule.size());
  std::vector<NetworKit::count> coveredByComponent(partition->getN(),
                                                   NetworKit::none);

  for (unsigned i = 0; i < originalIDs.size(); i++) {
    for (NetworKit::node v : originalIDs[i]) {
      if (coveredByComponent[v] == NetworKit::none) {
        coveredByComponent[v] = i;
      } else {
        biConnectedComponentTree.addEdge(coveredByComponent[v], i);
      }
    }
  }

  std::deque<NetworKit::node> componentOrder(1, 0);
  std::vector<bool> componentProcessed(originalIDs.size(), false);

  while (!componentOrder.empty()) {
    unsigned i = componentOrder.front();
    componentOrder.pop_front();

    componentProcessed[i] = true;

    for (NetworKit::node v : biConnectedComponentTree.neighborRange(i)) {
      if (!componentProcessed[v]) {
        componentOrder.emplace_back(v);
      }
    }

    std::shared_ptr<Partition> componentSolution = childRule[i]->getPartition();

    for (NetworKit::node v = 0; v < originalIDs[i].size(); v++) {
      if ((partition->assignmentOf(originalIDs[i][v]) != none) &&
          (partition->assignmentOf(originalIDs[i][v]) !=
           componentSolution->assignmentOf(v))) {

        std::vector<partitionID> colourSwap(
            std::max(partition->assignmentOf(originalIDs[i][v]),
                     componentSolution->assignmentOf(v)) +
            1);

        for (partitionID c = 0;
             c < std::max(partition->assignmentOf(originalIDs[i][v]),
                          componentSolution->assignmentOf(v));
             c++) {
          colourSwap[c] = c;
        }

        colourSwap[partition->assignmentOf(originalIDs[i][v])] =
            componentSolution->assignmentOf(v);
        colourSwap[componentSolution->assignmentOf(v)] =
            partition->assignmentOf(originalIDs[i][v]);

        componentSolution->swapPartitionNames(colourSwap);
      }
    }

    for (NetworKit::node v = 0; v < originalIDs[i].size(); v++) {
      assert(originalIDs[i][v] < maximumNodeID);
      assert(partition->assignmentOf(originalIDs[i][v]) == none ||
             partition->assignmentOf(originalIDs[i][v]) ==
                 componentSolution->assignmentOf(v));

      partition->assignElement(originalIDs[i][v],
                               componentSolution->assignmentOf(v));
    }
  }
}

const std::vector<std::shared_ptr<DataReductionReconstructor>> &
BiconnectedComponentReconstructor::getChildren() const {
  return childRule;
}

bool DataReducer::splitByBiconnectedComponents(QueuedGraph &currentGraph) {
  NetworKit::BiconnectedComponents componentsAlgo(currentGraph.readGraph());

  componentsAlgo.run();

  if (componentsAlgo.numberOfComponents() <= 1) {
    return false;
  }

  stats.biconnectedComponentSplits += componentsAlgo.numberOfComponents() - 1;

  std::vector<std::vector<NetworKit::node>> biconnectedComponents =
      componentsAlgo.getComponents();

  std::vector<std::vector<NetworKit::node>> originalIDs;
  std::vector<std::shared_ptr<DataReductionReconstructor>> childRules;

  for (const auto & biconnectedComponent : biconnectedComponents) {
    std::pair<NetworKit::Graph, std::vector<NetworKit::node>> subgraph =
        getInducedSubgraph(currentGraph.readGraph(), biconnectedComponent, _tempNodeValues);

    childRules.emplace_back(std::make_shared<DataReductionDummy>());
    graph_queue.emplace_back(subgraph.first, k, childRules.back());

    originalIDs.emplace_back(subgraph.second);
  }

  std::shared_ptr<DataReductionReconstructor> reconstructionRule =
      std::make_shared<BiconnectedComponentReconstructor>(
          currentGraph.readGraph().upperNodeIdBound(), originalIDs, childRules);

  currentGraph.attachNewRule(reconstructionRule);

  return true;
}

} // namespace mkcp
