#ifndef INDEXEDSET_HPP
#define INDEXEDSET_HPP

#include <stdexcept>
#include <vector>

/**
 * Class to store a subset of numbers from 0,...,n-1
 */
class IndexedSet {
public:
  /**
   * Creates an Indexed Set that can store numbers from 0,...,n-1
   */
  IndexedSet(unsigned int n) : n(n) {
    inSet.resize(n, 0);
    currentPosition.resize(n, 0);
  };

  /**
   * @returns true iff the set currently contains any numbers
   */
  bool empty() const { return currentSet.empty(); }

  /**
   * @returns a read-only vector with all numbers currently in the set
   */
  const std::vector<unsigned int> &currentElements() const {
    return currentSet;
  }

  /**
   * @returns true iff e is contained in the set
   */
  bool containsElement(unsigned int e) const { return inSet[e]; };

  /**
   * Insert an element into the set
   * @param e the number to be inserted
   */
  void insertElement(unsigned int e) {
    if (e >= n) {
      throw std::runtime_error(
          "Tried to insert a too large number into an indexed set");
    }

    if (inSet[e]) {
      return;
    }

    currentPosition[e] = currentSet.size();
    currentSet.emplace_back(e);
    inSet[e] = true;
  }

  /**
   * @param e the number to be deleted from the set
   */
  void removeElement(unsigned int e) {
    if (e >= n) {
      throw std::runtime_error(
          "Tried to remove too large number from IndexedSet");
    }

    if (!inSet[e]) {
      throw std::runtime_error(
          "Tried to number element from indexed set that isn't in the set");
    }

    inSet[e] = false;
    currentSet[currentPosition[e]] = currentSet.back();
    currentPosition[currentSet.back()] = currentPosition[e];
    currentSet.pop_back();
  }

private:
  unsigned int n;
  std::vector<bool> inSet;
  std::vector<unsigned int> currentSet;
  std::vector<unsigned int> currentPosition;
};

#endif // INDEXEDSET_HPP
