#ifndef GREEDY_MAX_K_CUT_HPP
#define GREEDY_MAX_K_CUT_HPP

#include <utility>

#include "globals.hpp"
#include "networkit/graph/Graph.hpp"
namespace mkcp {

class GreedyMaxKCut {
public:
  GreedyMaxKCut(NetworKit::Graph &g, unsigned int k)
      : _graph(g), _k(k), _solution(g.numberOfNodes()) {}

  GreedyMaxKCut(NetworKit::Graph &g, unsigned int k, Partition startPartition)
      : _graph(g), _k(k), _solution(std::move(startPartition)) {}

  void run();

  [[nodiscard]] bool hasRun() const;

  [[nodiscard]] double getObjectiveValue() const;

  [[nodiscard]] const Partition &getSolution() const;

private:
  NetworKit::Graph &_graph;
  unsigned int _k;
  Partition _solution;
  double _objectiveValue = 0.0;

  bool _hasRun = false;
};

} // namespace mkcp

#endif // GREEDY_MAX_K_CUT_HPP
