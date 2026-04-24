#include "preprocessing/lowDegreeRules.hpp"
#include "globals.hpp"
#include "networkit/Globals.hpp"
#include "preprocessing/maxKCutPreprocessor.hpp"
#include "preprocessing/maxKCutPreprocessorUtils.hpp"
#include <cassert>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mkcp {

LowDegreePositiveReconstructor::LowDegreePositiveReconstructor(unsigned int k)
    : k(k) {}

std::shared_ptr<Partition>
LowDegreePositiveReconstructor::getPartition() const {
  if (!calledReconstruction) {
    throw std::runtime_error(
        "Can't access partition before calling reconstructor function");
  }

  return partition;
}

void LowDegreePositiveReconstructor::reconstructData() {
  calledReconstruction = true;

  assert(childRule.size() == 1);
  partition = childRule[0]->getPartition();
  assert(partition.get() != nullptr);

  while (!reconstructionOrder.empty()) {
    std::vector<bool> occupied(k, false);

    std::pair<NetworKit::node, std::vector<NetworKit::node>>
        currentReinsterion = reconstructionOrder.front();
    reconstructionOrder.pop_front();

    assert(currentReinsterion.second.size() < k);
    assert(currentReinsterion.first < partition.get()->getN());
    assert(!partition.get()->isAssigned(currentReinsterion.first));

    for (NetworKit::node w : currentReinsterion.second) {
      assert(partition.get()->assignmentOf(w) != none);

      occupied[partition.get()->assignmentOf(w)] = true;
    }

    for (unsigned int i = 0; i < k; i++) {
      if (!occupied[i]) {
        partition.get()->assignElement(currentReinsterion.first, i);
        break;
      }
    }
  }
}

const std::vector<std::shared_ptr<DataReductionReconstructor>> &
LowDegreePositiveReconstructor::getChildren() const {
  return childRule;
}

void LowDegreePositiveReconstructor::addChildReduction(
    std::shared_ptr<DataReductionReconstructor> &child) {
  if (!childRule.empty()) {
    throw std::runtime_error(
        "Attempted to add a second child rule to a non-split rule");
  }
  childRule.emplace_back(std::shared_ptr<DataReductionReconstructor>(child));
}

void LowDegreePositiveReconstructor::addLowDegreeVertex(
    NetworKit::node v, const std::vector<NetworKit::node> &neighbours) {
  reconstructionOrder.emplace_front(v, neighbours);
}

bool DataReducer::reduceLowDegreePositiveVertices(QueuedGraph &currentGraph) {
  if (currentGraph.positiveDegreeLeqKVertices().empty()) {
    return false;
  }

  std::shared_ptr<LowDegreePositiveReconstructor> reconstructor =
      std::make_shared<LowDegreePositiveReconstructor>(k);

  while (!currentGraph.positiveDegreeLeqKVertices().empty()) {
    NetworKit::node v = currentGraph.positiveDegreeLeqKVertices().back();

    assert(currentGraph.readGraph().degree(v) < k);

    std::vector<NetworKit::node> neighbours;

    for (NetworKit::node w : currentGraph.readGraph().neighborRange(v)) {
      assert(currentGraph.readGraph().hasEdge(v, w));
      assert(currentGraph.readGraph().weight(v, w) > 0);
      neighbours.emplace_back(w);
    }

    reconstructor.get()->addLowDegreeVertex(v, neighbours);

    objectiveOffset += currentGraph.readGraph().weightedDegree(v);

    stats.lowDegreeStats += currentGraph.deleteVertex(v);
  }

  currentGraph.attachNewRule(reconstructor);

  return true;
}

} // namespace mkcp
