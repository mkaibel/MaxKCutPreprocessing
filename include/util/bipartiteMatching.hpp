#ifndef BIPARTITEMATCHING_HPP
#define BIPARTITEMATCHING_HPP

#include "networkit/Globals.hpp"
#include "networkit/graph/Graph.hpp"
#include <utility>
#include <vector>

NetworKit::Graph bipartiteComplement(const NetworKit::Graph &graph,
                                     std::vector<NetworKit::node> sideA);

class Bipartition {
public:
  Bipartition(const NetworKit::Graph &graph);

  void run();

  const std::pair<std::vector<NetworKit::node>, std::vector<NetworKit::node>> &
  getBipartition() const;

private:
  const NetworKit::Graph &graph;

  std::pair<std::vector<NetworKit::node>, std::vector<NetworKit::node>>
      bipartition;
};

class BipartiteMatching {
public:
  /**
   * Constructs a solver for bipartite matching on
   * @param the graph
   * @param sideA the set of vertices on one side of the bipartition
   */
  BipartiteMatching(const NetworKit::Graph &graph,
                    std::vector<NetworKit::node> sideA);

  void run();

  const std::vector<NetworKit::Edge> &getMatching() const;

private:
  const NetworKit::Graph &graph;

  std::vector<NetworKit::node> sideA;

  std::vector<NetworKit::Edge> matching;
};

#endif // BIPARTITEMATCHING_HPP
