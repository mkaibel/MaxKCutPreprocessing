#include "globals.hpp"
#include "networkit/Globals.hpp"
#include <algorithm>
#include <cassert>
#include <vector>

namespace mkcp
{
    Partition::Partition(unsigned int n)
    {
        _partition = std::vector<partitionID>(n, none);
        _n = n;
        _assignedEements = 0;
    }

    Partition::Partition(const std::vector<partitionID>& partition)
    {
        _partition = partition;
        _n = partition.size();

        for (partitionID pID : _partition)
        {
            if (pID != none)
            {
                _assignedEements++;
                if (pID >= _numElementsPerPartition.size())
                {
                    _numElementsPerPartition.resize(pID + 1, 0);
                }
                _numElementsPerPartition[pID]++;
            }
        }
    }

    unsigned int Partition::getN() const { return _n; }

    bool Partition::completePartition() const { return _n == _assignedEements; }

    void Partition::sortLexicographically()
    {
        // TODO performance this can be done with only O(k) extra space
        std::vector<NetworKit::node> minNodeByColour(_numElementsPerPartition.size(),
                                                     NetworKit::none);

        for (NetworKit::node i = 0; i < _n; i++)
        {
            if (_partition[i] != none)
            {
                minNodeByColour[_partition[i]] =
                    std::min(minNodeByColour[_partition[i]], i);
            }
        }

        std::vector<partitionID> bucketSortPartitions(_n, none);

        for (partitionID i = 0; i < minNodeByColour.size(); i++)
        {
            if (minNodeByColour[i] != NetworKit::none)
            {
                bucketSortPartitions[minNodeByColour[i]] = i;
            }
        }

        partitionID j = 0;
        std::vector<partitionID> recolour_map(minNodeByColour.size());

        for (NetworKit::node v = 0; v < _n; v++)
        {
            if (bucketSortPartitions[v] != none)
            {
                recolour_map[bucketSortPartitions[v]] = j;
                j++;
            }
        }

        for (unsigned int i = 0; i < minNodeByColour.size(); i++)
        {
            if (minNodeByColour[i] == NetworKit::none)
            {
                recolour_map[i] = j;
                j++;
            }
        }

        for (NetworKit::node v = 0; v < _n; v++)
        {
            if (_partition[v] != none)
            {
                _partition[v] = recolour_map[_partition[v]];
            }
        }
    }

    void Partition::assignElement(unsigned int e, partitionID newPartition)
    {
        assert(e < _n);

        if (_partition[e] != none)
        {
            _numElementsPerPartition[_partition[e]]--;
            if (newPartition == none)
            {
                _assignedEements--;
            }
        }
        else if (newPartition != none)
        {
            _assignedEements++;
        }

        if (newPartition != none)
        {
            if (newPartition >= _numElementsPerPartition.size())
            {
                _numElementsPerPartition.resize(newPartition + 1, 0);
            }

            _numElementsPerPartition[newPartition]++;
        }

        _partition[e] = newPartition;

        while (_numElementsPerPartition.back() == 0)
        {
            _numElementsPerPartition.pop_back();
        }
    }

    bool Partition::isAssigned(unsigned int e) const
    {
        assert(e < _n);

        return _partition[e] != none;
    }

    partitionID Partition::assignmentOf(unsigned int e) const
    {
        assert(e < _n);

        return _partition[e];
    }

    void Partition::swapPartitionNames(const std::vector<partitionID>& newNames)
    {
        unsigned int k = newNames.size();
        if (k > _numElementsPerPartition.size())
        {
            _numElementsPerPartition.resize(k, 0);
        }

        for (unsigned int i = 0; i < k; i++)
        {
            assert(newNames[i] < k);
            for (unsigned int j = i + 1; j < k; j++)
            {
                // Ensure that a permutation was given
                assert(newNames[i] != newNames[j]);
            }
        }

        for (unsigned int i = 0; i < _n; i++)
        {
            if (_partition[i] < k)
            {
                _partition[i] = newNames[_partition[i]];
            }
        }

        std::vector<unsigned int> swappedPartitionSizes(k);

        for (unsigned int i = 0; i < k; i++)
        {
            swappedPartitionSizes[newNames[i]] = _numElementsPerPartition[i];
        }

        for (unsigned int i = 0; i < k; i++)
        {
            _numElementsPerPartition[i] = swappedPartitionSizes[i];
        }
    }


    std::vector<partitionID> invertPermutation(const std::vector<partitionID>& permutation)
    {
        std::vector<partitionID> inverse(permutation.size(), none);

        for (partitionID i = 0; i < permutation.size(); i++)
        {
            inverse[permutation[i]] = i;
        }

#ifdef DEBUG
        for (partitionID i = 0; i < permutation.size(); i++)
        {
            assert(inverse[i] != none);
        }
#endif

        return inverse;
    }

    NetworKit::edgeweight
    computeKCutValue(const NetworKit::Graph& graph,
                     const Partition& vertexPartition)
    {
        for (NetworKit::node v = 0; v < graph.upperNodeIdBound(); v++)
        {
            if (graph.hasNode(v))
            {
                assert(vertexPartition.isAssigned(v));
            }
            else
            {
                assert(!vertexPartition.isAssigned(v));
            }
        }

        NetworKit::edgeweight objValue = 0.0;

        for (NetworKit::WeightedEdge e : graph.edgeWeightRange())
        {
            if (vertexPartition.assignmentOf(e.u) !=
                vertexPartition.assignmentOf(e.v))
            {
                objValue += e.weight;
            }
        }

        return objValue;
    }

    NetworKit::edgeweight
    computeKPartitionValue(const NetworKit::Graph& graph,
                     const Partition& vertexPartition)
    {
        for (NetworKit::node v = 0; v < graph.upperNodeIdBound(); v++)
        {
            if (graph.hasNode(v))
            {
                assert(vertexPartition.isAssigned(v));
            }
            else
            {
                assert(!vertexPartition.isAssigned(v));
            }
        }

        NetworKit::edgeweight objValue = 0.0;

        for (NetworKit::WeightedEdge e : graph.edgeWeightRange())
        {
            if (vertexPartition.assignmentOf(e.u) ==
                vertexPartition.assignmentOf(e.v))
            {
                objValue += e.weight;
            }
        }

        return objValue;
    }

    std::vector<partitionID> completePermutation(
        const std::vector<std::pair<partitionID, partitionID>>& mapsTo)
    {
        std::vector<partitionID> permutation;
        std::vector<bool> alreadyMappedTo;

        for (auto [from, to] : mapsTo)
        {
            partitionID max = std::max(from, to);

            if (max >= permutation.size())
            {
                permutation.resize(max + 1, none);
                alreadyMappedTo.resize(max + 1, false);
            }

            assert(alreadyMappedTo[to] == false);
            permutation[from] = to;
            alreadyMappedTo[to] = true;
        }

        unsigned nextFreeGroup = 0;

        for (unsigned i = 0; i < permutation.size(); i++)
        {
            if (permutation[i] == none)
            {
                while (alreadyMappedTo[nextFreeGroup])
                {
                    nextFreeGroup++;
                }
                permutation[i] = nextFreeGroup;
                nextFreeGroup++;
            }
        }

        return permutation;
    }
} // namespace mkcp
