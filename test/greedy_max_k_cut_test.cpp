#include "networkit/graph/Graph.hpp"
#include "solver/greedy_max_k_cut.hpp"
#include "gtest/gtest.h"

TEST(GreedyTest, basicTest) {
  NetworKit::Graph g(6, true);

  g.addEdge(0, 1, 1.0);
  g.addEdge(0, 2, 1.0);
  g.addEdge(0, 4, 1.0);
  g.addEdge(1, 2, 1.0);
  g.addEdge(1, 3, 1.0);
  g.addEdge(2, 3, 1.0);
  g.addEdge(3, 5, 1.0);
  g.addEdge(4, 5, 1.0);

  mkcp::GreedyMaxKCut greedy(g, 3);

  greedy.run();

  EXPECT_TRUE(greedy.hasRun());

  EXPECT_GT(greedy.getObjectiveValue(), 5);
}
