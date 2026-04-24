#include "solver/constraintGenerator.hpp"
#include "globals.hpp"
#include "networkit/Globals.hpp"
#include "networkit/graph/Graph.hpp"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <vector>

namespace mkcp {

DominatingEdges::DominatingEdges(const NetworKit::Graph &graph, unsigned int k)
    : graph(graph), k(k) {
  assert(graph.hasEdgeIds());
  assert(graph.getMaintainCompactEdges());

  edgeIsDominating.resize(graph.numberOfEdges(), EdgeConstraint::NONE);
}

bool DominatingEdges::hasRun() const { return hasBeenRun; }

const std::vector<EdgeConstraint> &DominatingEdges::guaranteedInCut() const {
  return edgeIsDominating;
}

void DominatingEdges::run() {
  hasBeenRun = true;

  for (NetworKit::node v : graph.nodeRange()) {
    NetworKit::edgeweight totalWeight = 0.0;
    bool unitWeightDegreeK = (graph.degree(v) == k);

    if (graph.degree(v) == 0)
      continue;

    NetworKit::edgeweight defaultWeight = graph.getIthNeighborWeight(v, 0);

    for (auto [neighbour, neighbourWeight] : graph.weightNeighborRange(v)) {
      unitWeightDegreeK &= (neighbourWeight == defaultWeight);
      if (neighbourWeight < 0.0) {
        totalWeight += static_cast<NetworKit::edgeweight>(k - 1) *
                       std::abs(neighbourWeight);
      } else {
        totalWeight += neighbourWeight;
      }
    }

    // Make a case distinction between the cut containing k equal edges or not
    if (unitWeightDegreeK) {
      NetworKit::node minNeighbour = graph.getIthNeighbor(v, 0);

      for (NetworKit::node neighbour : graph.neighborRange(v)) {
        minNeighbour = std::min(minNeighbour, neighbour);
      }

      for (NetworKit::node neighbour : graph.neighborRange(v)) {
        if (neighbour > minNeighbour) {
          edgeIsDominating[graph.edgeId(v, neighbour)] = EdgeConstraint::IN_CUT;
        }
      }
    } else {
      for (auto [neighbour, neighbourWeight] : graph.weightNeighborRange(v)) {
        // Check for dominating
        if (static_cast<NetworKit::edgeweight>(k - 1) * neighbourWeight >=
            totalWeight) {
          edgeIsDominating[graph.edgeId(v, neighbour)] = EdgeConstraint::IN_CUT;
        }
      }
    }
  }

  for (NetworKit::node v : graph.nodeRange()) {
    std::vector<NetworKit::WeightedEdge> positiveEdges;

    auto greater = [this](const NetworKit::WeightedEdge &lhs,
                          const NetworKit::WeightedEdge &rhs) {
      if (lhs.weight == rhs.weight) {
        return this->graph.edgeId(lhs.u, lhs.v) >
               this->graph.edgeId(rhs.u, rhs.v);
      }

      return lhs.weight > rhs.weight;
    };

    NetworKit::edgeweight offsetByNegativeEdges = 0.0;

    for (auto [neighbour, neighbourWeight] : graph.weightNeighborRange(v)) {
      if (neighbourWeight > 0.0) {
        positiveEdges.emplace_back(v, neighbour, neighbourWeight);
      } else {
        offsetByNegativeEdges -= neighbourWeight;
      }
    }

    std::sort(positiveEdges.begin(), positiveEdges.end(), greater);

    // Handle a few special cases
    if (positiveEdges.empty())
      continue;

    if (positiveEdges.size() < k) {
      for (NetworKit::WeightedEdge e : positiveEdges) {
        if (e.weight >= offsetByNegativeEdges) {
          edgeIsDominating[graph.edgeId(e.u, e.v)] = EdgeConstraint::IN_CUT;
        }
      }

      continue;
    }

    if (positiveEdges.size() == k) {
      if (offsetByNegativeEdges == 0.0)
        continue; // Case covered by previous dominating edges

      for (NetworKit::count j = 0; j < k - 1; j++) {
        if (positiveEdges[j].weight >=
            offsetByNegativeEdges + positiveEdges.back().weight) {
          edgeIsDominating[graph.edgeId(
              positiveEdges[j].u, positiveEdges[j].v)] = EdgeConstraint::IN_CUT;
        }
      }
    }

    if (positiveEdges.size() > 2 * k - 1) {
      continue;
    }

    NetworKit::count i = 2 * k - positiveEdges.size() - 1;

    NetworKit::edgeweight boundaryWeight =
        std::min(positiveEdges[i].weight,
                 positiveEdges[i + 1].weight + positiveEdges.back().weight);

    // All edges better than the border are dominating
    for (NetworKit::count j = 0; j < i; j++) {
      if (positiveEdges[j].weight >= offsetByNegativeEdges + boundaryWeight) {
        edgeIsDominating[graph.edgeId(positiveEdges[j].u, positiveEdges[j].v)] =
            EdgeConstraint::IN_CUT;
      }
    }

    if (positiveEdges[i].weight >= offsetByNegativeEdges +
                                       positiveEdges[i + 1].weight +
                                       positiveEdges.back().weight) {
      edgeIsDominating[graph.edgeId(positiveEdges[i].u, positiveEdges[i].v)] =
          EdgeConstraint::IN_CUT;
    }
  }
}

} // namespace mkcp
