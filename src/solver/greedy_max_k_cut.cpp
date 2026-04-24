#include "solver/greedy_max_k_cut.hpp"
#include "globals.hpp"
#include "networkit/Globals.hpp"
#include <cassert>
#include <stdexcept>
#include <vector>

namespace mkcp {

bool GreedyMaxKCut::hasRun() const {
  if (!_hasRun) {
    throw std::runtime_error("Algorithm not run yet");
  }

  return _hasRun;
}

double GreedyMaxKCut::getObjectiveValue() const {
  if (!_hasRun) {
    throw std::runtime_error("Algorithm not run yet");
  }

  return _objectiveValue;
}

const Partition &GreedyMaxKCut::getSolution() const {
  if (!_hasRun) {
    throw std::runtime_error("Algorithm not run yet");
  }

  return _solution;
}

void GreedyMaxKCut::run() {
  unsigned int n = _graph.numberOfNodes();
  // TODO assert that partial partition handed only uses values 0 to k-1
  std::vector<std::vector<double>> lossByColour(n,
                                                std::vector<double>(_k, 0.0));

  for (NetworKit::node v = 0; v < n; v++) {
    if (_solution.assignmentOf(v) != none) {
      for (auto e : _graph.weightNeighborRange(v)) {
        lossByColour[e.first][_solution.assignmentOf(v)] += e.second;
      }
    }
  }

  for (NetworKit::node v = 0; v < n; v++) {
    if (_solution.assignmentOf(v) != none) {
      continue;
    }

    unsigned int cMin = 0;
    double minSoFar = lossByColour[v][0];
    for (unsigned int i = 1; i < _k; i++) {
      if (lossByColour[v][i] < minSoFar) {
        cMin = i;
        minSoFar = lossByColour[v][i];
      }
    }

    _solution.assignElement(v, cMin);
    for (auto e : _graph.weightNeighborRange(v)) {
      lossByColour[e.first][cMin] += e.second;
    }
  }

  for (unsigned int j = 0; j < 2; j++) {
    for (NetworKit::node v = 0; v < n; v++) {
      unsigned int cMin = 0;
      double minSoFar = lossByColour[v][0];
      for (unsigned int i = 1; i < _k; i++) {
        if (lossByColour[v][i] < minSoFar) {
          cMin = i;
          minSoFar = lossByColour[v][i];
        }
      }

      unsigned int currentAssignment = _solution.assignmentOf(v);
      if (currentAssignment != cMin) {
        for (auto e : _graph.weightNeighborRange(v)) {
          lossByColour[e.first][cMin] += e.second;
          lossByColour[e.first][currentAssignment] -= e.second;
        }
        _solution.assignElement(v, cMin);
      }
    }
  }

  _solution.sortLexicographically();

  for (auto e : _graph.edgeWeightRange()) {
    if (e.u > e.v) {
      continue;
    }

    if (_solution.assignmentOf(e.u) != _solution.assignmentOf(e.v)) {
      _objectiveValue += e.weight;
    }
  }

  _hasRun = true;
}
} // namespace mkcp
