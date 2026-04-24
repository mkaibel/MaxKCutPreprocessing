#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include "networkit/Globals.hpp"
#include "networkit/graph/Graph.hpp"
#include <cassert>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace mkcp
{
    using partitionID = uint16_t;
    constexpr partitionID none = std::numeric_limits<partitionID>::max();

    enum EdgeConstraint
    {
        NONE,
        IN_CUT,
        OUT_CUT,
    };

    /**
     * Enum to store different modalities for preprocessing
     */
    enum PreprocessorSetting
    {
        NO = 0,
        NAIVE = 2,
        FULL = 1,
    };

    /**
     * Class to store partitions of n elements into groups.
     * Uses n + k space where k is the maximum ID of a partition used (ignoring
     * unassigned)
     */
    class Partition
    {
    public:
        /**
         * Constructs a partition for n elements, none of which are currently assigned
         * to any other element
         * @param n the number of eements
         * @return A partition for n elements, none of which are currently assigned
         */
        Partition(unsigned int n);

        /**
         * Constructs a partition from the assignments in the vector
         * @param partition a vector assigning partition IDs to elements
         * @return a partition that corresponds to the given one
         */
        Partition(const std::vector<partitionID> &partition);

        /**
         * Returns the number of elements in the partition
         */
        unsigned int getN() const;
        /**
         * Returns if all elements are assigned to a partition
         */
        bool completePartition() const;

        /**
         * Swaps around partition IDs such that partition is lexicographically minimal
         */
        void sortLexicographically();

        /**
         * Change the partition that element e is assigned to
         * @param e the ID of the element
         * @param newPartition the ID of the new partition of element e
         */
        void assignElement(unsigned int e, partitionID newPartition);

        /**
         * Returnes the partition ID of element e
         * @param e the ID of the element
         */
        bool isAssigned(unsigned int e) const;

        /**
         * Returnes the partition ID of element e
         * @param e the ID of the element
         */
        partitionID assignmentOf(unsigned int e) const;

        /**
         * Swaps partition names around so that elements that previously were in
         * partition i are then in partition newNames[i]
         * @param newNames a vector of k elements storing a permutation such that i
         * gets send to newNames[i]
         */
        void swapPartitionNames(const std::vector<partitionID>& newNames);

    private:
        std::vector<partitionID> _partition;
        std::vector<unsigned int> _numElementsPerPartition;
        unsigned int _n = 0;
        unsigned int _assignedEements = 0;
    };

    /**
     * Completes a partial mapping to a permutation
     * @param mapsTo a vector of pairs (x, f(x)) that we want to extend to a
     * permutation
     */
    std::vector<partitionID> completePermutation(
        const std::vector<std::pair<partitionID, partitionID>>& mapsTo);

    //! Given a permutation p returns p^{-1}
    std::vector<partitionID> invertPermutation(const std::vector<partitionID>& permutation);

    NetworKit::edgeweight
    computeKCutValue(const NetworKit::Graph& graph,
                     const Partition& vertexPartition);

    NetworKit::edgeweight
    computeKPartitionValue(const NetworKit::Graph& graph,
                     const Partition& vertexPartition);
} // namespace mkcp

#endif // GLOBALS_HPP
