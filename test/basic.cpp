#include "gtest/gtest.h"

#include "globals.hpp"

TEST(GreedyTest, basicPartitionTest) {
  mkcp::Partition partition(5);

  partition.assignElement(0, 3);
  partition.assignElement(1, 1);
  partition.assignElement(2, 0);
  partition.assignElement(3, 3);

  EXPECT_FALSE(partition.completePartition());

  EXPECT_TRUE(partition.isAssigned(0));
  EXPECT_TRUE(partition.isAssigned(1));
  EXPECT_TRUE(partition.isAssigned(2));
  EXPECT_TRUE(partition.isAssigned(3));
  EXPECT_FALSE(partition.isAssigned(4));

  EXPECT_EQ(partition.assignmentOf(0), 3);
  EXPECT_EQ(partition.assignmentOf(1), 1);
  EXPECT_EQ(partition.assignmentOf(2), 0);
  EXPECT_EQ(partition.assignmentOf(3), 3);
  EXPECT_EQ(partition.assignmentOf(4), mkcp::none);

  EXPECT_EQ(partition.getN(), 5);
}

TEST(GreedyTest, sortingPartitionTest) {
  mkcp::Partition partition(5);

  partition.assignElement(0, 3);
  partition.assignElement(1, 1);
  partition.assignElement(2, 0);
  partition.assignElement(3, 3);

  partition.sortLexicographically();

  EXPECT_TRUE(partition.isAssigned(0));
  EXPECT_TRUE(partition.isAssigned(1));
  EXPECT_TRUE(partition.isAssigned(2));
  EXPECT_TRUE(partition.isAssigned(3));
  EXPECT_FALSE(partition.isAssigned(4));

  EXPECT_EQ(partition.assignmentOf(0), 0);
  EXPECT_EQ(partition.assignmentOf(1), 1);
  EXPECT_EQ(partition.assignmentOf(2), 2);
  EXPECT_EQ(partition.assignmentOf(3), 0);
  EXPECT_EQ(partition.assignmentOf(4), mkcp::none);

  EXPECT_EQ(partition.getN(), 5);
}
