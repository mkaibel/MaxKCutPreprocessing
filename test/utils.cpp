#include "networkit/graph/Graph.hpp"
#include "preprocessing/maxKCutPreprocessorUtils.hpp"
#include "gtest/gtest.h"
#include <memory>

TEST(UtilsTest, simpleInsertDeleteTest) {
  unsigned int k = 4;

  NetworKit::Graph g(10, true);

  g.addEdge(0, 1, 1.0);
  g.addEdge(0, 3, -1.0);
  g.addEdge(1, 2, -1.0);
  g.addEdge(2, 3, 1.0);
  g.addEdge(2, 4, -1.0);
  g.addEdge(3, 5, 1.0);
  g.addEdge(3, 8, -1.0);
  g.addEdge(4, 5, 1.0);
  g.addEdge(4, 6, -1.0);
  g.addEdge(5, 7, 1.0);
  g.addEdge(5, 9, 1.0);
  g.addEdge(6, 7, -1.0);
  g.addEdge(8, 9, 1.0);

  std::shared_ptr<mkcp::DataReductionReconstructor> dummy =
      std::make_shared<mkcp::DataReductionDummy>();
  mkcp::QueuedGraph qg(g, k, dummy);

  EXPECT_TRUE(qg.locallyPositive(5));
  EXPECT_TRUE(qg.locallyPositive(9));

  EXPECT_FALSE(qg.locallyPositive(0));
  EXPECT_FALSE(qg.locallyPositive(1));
  EXPECT_FALSE(qg.locallyPositive(4));
  EXPECT_FALSE(qg.locallyPositive(6));

  EXPECT_TRUE(qg.locallyUnitWeight(5));
  EXPECT_TRUE(qg.locallyUnitWeight(9));
  EXPECT_TRUE(qg.locallyUnitWeight(6));

  EXPECT_FALSE(qg.locallyUnitWeight(0));
  EXPECT_FALSE(qg.locallyUnitWeight(1));
  EXPECT_FALSE(qg.locallyUnitWeight(4));

  EXPECT_EQ(qg.positiveDegreeLeqKVertices().size(), 1);

  qg.deleteVertex(5);

  EXPECT_EQ(qg.positiveDegreeLeqKVertices().size(), 1);

  qg.deleteEdge(3, 8);

  EXPECT_EQ(qg.positiveDegreeLeqKVertices().size(), 2);

  qg.changeEdgeWeight(0, 3, 1.0);

  EXPECT_TRUE(qg.locallyPositive(3));
  EXPECT_TRUE(qg.locallyPositive(9));
  EXPECT_TRUE(qg.locallyPositive(0));
  EXPECT_TRUE(qg.locallyPositive(8));

  EXPECT_FALSE(qg.locallyPositive(1));
  EXPECT_FALSE(qg.locallyPositive(4));
  EXPECT_FALSE(qg.locallyPositive(6));

  EXPECT_TRUE(qg.locallyUnitWeight(0));
  EXPECT_TRUE(qg.locallyUnitWeight(3));
  EXPECT_TRUE(qg.locallyUnitWeight(4));
  EXPECT_TRUE(qg.locallyUnitWeight(6));
  EXPECT_TRUE(qg.locallyUnitWeight(7));
  EXPECT_TRUE(qg.locallyUnitWeight(8));
  EXPECT_TRUE(qg.locallyUnitWeight(9));

  EXPECT_FALSE(qg.locallyUnitWeight(1));
  EXPECT_FALSE(qg.locallyUnitWeight(2));

  EXPECT_EQ(qg.positiveDegreeLeqKVertices().size(), 4);

  qg.compactNodeIDs();

  EXPECT_TRUE(qg.locallyPositive(3));
  EXPECT_TRUE(qg.locallyPositive(8));
  EXPECT_TRUE(qg.locallyPositive(0));
  EXPECT_TRUE(qg.locallyPositive(7));

  EXPECT_FALSE(qg.locallyPositive(1));
  EXPECT_FALSE(qg.locallyPositive(4));
  EXPECT_FALSE(qg.locallyPositive(5));

  EXPECT_TRUE(qg.locallyUnitWeight(0));
  EXPECT_TRUE(qg.locallyUnitWeight(3));
  EXPECT_TRUE(qg.locallyUnitWeight(4));
  EXPECT_TRUE(qg.locallyUnitWeight(5));
  EXPECT_TRUE(qg.locallyUnitWeight(6));
  EXPECT_TRUE(qg.locallyUnitWeight(7));
  EXPECT_TRUE(qg.locallyUnitWeight(8));

  EXPECT_FALSE(qg.locallyUnitWeight(1));
  EXPECT_FALSE(qg.locallyUnitWeight(2));

  EXPECT_EQ(qg.positiveDegreeLeqKVertices().size(), 4);
}

TEST(UtilTest, contractTest) {
  unsigned int k = 4;

  NetworKit::Graph g(7, true);

  g.addEdge(0, 5, 1.0);
  g.addEdge(1, 2, 1.0);
  g.addEdge(1, 5, 1.0);
  g.addEdge(2, 3, 1.0);
  g.addEdge(2, 5, -2.0);
  g.addEdge(2, 6, 3.0);
  g.addEdge(4, 6, 1.0);

  std::shared_ptr<mkcp::DataReductionReconstructor> dummy =
      std::make_shared<mkcp::DataReductionDummy>();
  mkcp::QueuedGraph qg(g, k, dummy);

  qg.contractNodes(5, 6);

  EXPECT_TRUE(qg.readGraph().hasNode(5));
  EXPECT_FALSE(qg.readGraph().hasNode(6));

  EXPECT_TRUE(qg.locallyPositive(5));

  EXPECT_TRUE(qg.locallyUnitWeight(5));

  EXPECT_EQ(qg.positiveDegreeLeqKVertices().size(), 5);
}
