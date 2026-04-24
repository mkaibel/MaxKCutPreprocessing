#include "util/bipartiteMatching.hpp"
#include "networkit/Globals.hpp"
#include "networkit/flow/EdmondsKarp.hpp"
#include "networkit/graph/Graph.hpp"
#include <cassert>
#include <deque>
#include <stdexcept>
#include <vector>

Bipartition::Bipartition(const NetworKit::Graph &graph) : graph(graph) {
  assert(graph.upperNodeIdBound() == graph.numberOfNodes());
}

void Bipartition::run() {
  // Compute bipartition
  std::deque<NetworKit::node> vertexQueue;
  std::vector<bool> inPartitionA(graph.numberOfNodes(), false);
  std::vector<bool> enqueued(graph.numberOfNodes(), false);

  inPartitionA[0] = true;
  enqueued[0] = true;
  bipartition.first.emplace_back(0);

  for (NetworKit::node v : graph.nodeRange()) {
    if (enqueued[v])
      continue;

    vertexQueue.emplace_back(v);

    while (!vertexQueue.empty()) {
      NetworKit::node v = vertexQueue.front();
      vertexQueue.pop_front();

      for (NetworKit::node w : graph.neighborRange(v)) {
        if (!enqueued[w]) {
          enqueued[w] = true;
          vertexQueue.emplace_back(w);
          inPartitionA[w] = !inPartitionA[v];
          if (inPartitionA[w]) {
            bipartition.first.emplace_back(w);
          } else {
            bipartition.second.emplace_back(w);
          }
        } else {
          if (inPartitionA[v] == inPartitionA[w]) {
            throw std::runtime_error("Graph should have been bipartite");
          }
        }
      }
    }
  }
}

const std::pair<std::vector<NetworKit::node>, std::vector<NetworKit::node>> &
Bipartition::getBipartition() const {
  return bipartition;
}

BipartiteMatching::BipartiteMatching(const NetworKit::Graph &graph,
                                     std::vector<NetworKit::node> sideA)
    : graph(graph), sideA(sideA) {
  assert(graph.upperNodeIdBound() == graph.numberOfNodes());
}

const std::vector<NetworKit::Edge> &BipartiteMatching::getMatching() const {
  return matching;
}

void BipartiteMatching::run() {
  std::vector<bool> onSideA(graph.numberOfNodes(), false);

  for (NetworKit::node v : sideA) {
    onSideA[v] = true;
  }

  NetworKit::Graph flowGraph(graph.numberOfNodes() + 2, true, true);
  NetworKit::node s = graph.numberOfNodes(), t = graph.numberOfNodes() + 1;

  for (NetworKit::Edge e : graph.edgeRange()) {
    assert(onSideA[e.u] != onSideA[e.v]);
    if (onSideA[e.u]) {
      flowGraph.addEdge(e.u, e.v, 1.0);
    } else {
      flowGraph.addEdge(e.v, e.u, 1.0);
    }
  }

  for (NetworKit::node v : graph.nodeRange()) {
    if (onSideA[v]) {
      flowGraph.addEdge(s, v, 1.0);
    } else {
      flowGraph.addEdge(v, t, 1.0);
    }
  }

  flowGraph.indexEdges();
  flowGraph.setMaintainCompactEdges();

  NetworKit::EdmondsKarp flow(flowGraph, s, t);

  flow.run();

  for (NetworKit::Edge e : flowGraph.edgeRange()) {
    if ((e.u == s) || (e.v == t)) {
      continue;
    }
    if (flow.getFlow(flowGraph.edgeId(e.u, e.v)) == 1.0) {
      matching.emplace_back(e.u, e.v);
    }
  }
}

NetworKit::Graph bipartiteComplement(const NetworKit::Graph &graph,
                                     std::vector<NetworKit::node> sideA) {
  assert(graph.numberOfNodes() == graph.upperNodeIdBound());
  std::vector<bool> isNeighbour(graph.numberOfNodes(), false);
  std::vector<NetworKit::node> sideB;

  for (NetworKit::node v : sideA) {
    isNeighbour[v] = true;
  }

  for (NetworKit::node v : graph.nodeRange()) {
    if (isNeighbour[v]) {
      isNeighbour[v] = false;
    } else {
      sideB.emplace_back(v);
    }
  }

  NetworKit::Graph complement(graph.numberOfNodes());

  for (NetworKit::node v : sideA) {
    for (NetworKit::node w : graph.neighborRange(v)) {
      isNeighbour[w] = true;
    }

    for (NetworKit::node w : sideB) {
      if (isNeighbour[w]) {
        isNeighbour[w] = false;
      } else {
        assert(!complement.hasEdge(v, w));
        complement.addEdge(v, w);
      }
    }
  }

  return complement;
}
