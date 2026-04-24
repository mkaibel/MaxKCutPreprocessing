#include "util/indexedSet.hpp"
#include "gtest/gtest.h"
#include <algorithm>

TEST(IndexedSetTest, InsertTest) {
  IndexedSet s(5);
  s.insertElement(0);
  s.insertElement(3);
  s.insertElement(2);

  EXPECT_EQ(s.currentElements().size(), 3);

  EXPECT_FALSE(s.empty());
  EXPECT_EQ(
      std::count(s.currentElements().begin(), s.currentElements().end(), 0), 1);
  EXPECT_EQ(
      std::count(s.currentElements().begin(), s.currentElements().end(), 1), 0);
  EXPECT_EQ(
      std::count(s.currentElements().begin(), s.currentElements().end(), 2), 1);
  EXPECT_EQ(
      std::count(s.currentElements().begin(), s.currentElements().end(), 3), 1);
  EXPECT_EQ(
      std::count(s.currentElements().begin(), s.currentElements().end(), 4), 0);

  EXPECT_TRUE(s.containsElement(0));
  EXPECT_FALSE(s.containsElement(1));
  EXPECT_TRUE(s.containsElement(2));
  EXPECT_TRUE(s.containsElement(3));
  EXPECT_FALSE(s.containsElement(4));

  s.insertElement(0);
  s.insertElement(4);

  EXPECT_EQ(s.currentElements().size(), 4);

  EXPECT_EQ(
      std::count(s.currentElements().begin(), s.currentElements().end(), 0), 1);
  EXPECT_EQ(
      std::count(s.currentElements().begin(), s.currentElements().end(), 1), 0);
  EXPECT_EQ(
      std::count(s.currentElements().begin(), s.currentElements().end(), 2), 1);
  EXPECT_EQ(
      std::count(s.currentElements().begin(), s.currentElements().end(), 3), 1);
  EXPECT_EQ(
      std::count(s.currentElements().begin(), s.currentElements().end(), 4), 1);

  EXPECT_TRUE(s.containsElement(0));
  EXPECT_FALSE(s.containsElement(1));
  EXPECT_TRUE(s.containsElement(2));
  EXPECT_TRUE(s.containsElement(3));
  EXPECT_TRUE(s.containsElement(4));

  EXPECT_ANY_THROW(s.insertElement(5));
}

TEST(IndexedSetTest, InsertDeleteTest) {
  IndexedSet s(5);
  s.insertElement(0);
  s.insertElement(3);
  s.insertElement(2);
  s.insertElement(4);

  s.removeElement(3);
  s.removeElement(4);

  EXPECT_EQ(s.currentElements().size(), 2);

  EXPECT_EQ(
      std::count(s.currentElements().begin(), s.currentElements().end(), 0), 1);
  EXPECT_EQ(
      std::count(s.currentElements().begin(), s.currentElements().end(), 1), 0);
  EXPECT_EQ(
      std::count(s.currentElements().begin(), s.currentElements().end(), 2), 1);
  EXPECT_EQ(
      std::count(s.currentElements().begin(), s.currentElements().end(), 3), 0);
  EXPECT_EQ(
      std::count(s.currentElements().begin(), s.currentElements().end(), 4), 0);

  EXPECT_TRUE(s.containsElement(0));
  EXPECT_FALSE(s.containsElement(1));
  EXPECT_TRUE(s.containsElement(2));
  EXPECT_FALSE(s.containsElement(3));
  EXPECT_FALSE(s.containsElement(4));

  EXPECT_ANY_THROW(s.removeElement(5));
  EXPECT_ANY_THROW(s.removeElement(4));
}
