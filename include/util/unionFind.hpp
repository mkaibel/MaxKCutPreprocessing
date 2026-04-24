#ifndef UNIONFIND_HPP
#define UNIONFIND_HPP

#include "networkit/Globals.hpp"
#include <utility>
#include <vector>

/**
 * Implements the union find data structure
 */
class UnionFind
{
public:
    UnionFind(NetworKit::node n) : n(n)
    {
        parent.resize(n);
        size.resize(n, 1);

        numSets = n;

        for (NetworKit::node i = 0; i < n; i++)
        {
            parent[i] = i;
        }
    };

    /**
     * Returns a representative of the set containing e
     */
    NetworKit::node find(NetworKit::node e)
    {
        std::vector<NetworKit::node> path;

        while (parent[e] != e)
        {
            path.emplace_back(e);
            e = parent[e];
        }

        for (NetworKit::node x : path)
        {
            parent[x] = e;
        }

        return e;
    }

    /**
     * Unionizes the sets of e1 and e2
     * @param e1, e2 two elements
     */
    void unionized(NetworKit::node e1, NetworKit::node e2)
    {
        e1 = find(e1);
        e2 = find(e2);

        if (e1 == e2)
            return;

        numSets--;

        if (size[e1] < size[e2])
            std::swap(e1, e2);

        parent[e2] = e1;
        size[e1] += size[e1];
    }

    /**
     * Returns how many elements are in the same set as e
     * @param e the element
     */
    NetworKit::node setSize(NetworKit::node e) { return size[find(e)]; }

    NetworKit::node numberOfSets() const { return numSets; }

    /**
     * Returns all sets that currently exist
     */
    std::vector<std::vector<NetworKit::node>> obtainSets()
    {
        std::vector<std::vector<NetworKit::node>> sets(numSets);

        std::vector<NetworKit::node> setID(n, 0);

        NetworKit::node currentSetID = 0;

        // Assign sets compressed ids
        for (NetworKit::node i = 0; i < n; i++)
        {
            if (parent[i] == i)
            {
                setID[i] = currentSetID;
                currentSetID++;
            }
        }

        for (NetworKit::node i = 0; i < n; i++)
        {
            sets[setID[find(i)]].emplace_back(i);
        }

        return sets;
    }

private:
    NetworKit::node n;
    NetworKit::node numSets;

    std::vector<NetworKit::node> parent;
    std::vector<NetworKit::node> size;
};

class UnionFindAlgo1
{
public:
    UnionFindAlgo1(NetworKit::node n) : n(n)
    {
        _representative.resize(n);
        _set.resize(n);

        numSets = n;

        for (NetworKit::node i = 0; i < n; i++)
        {
            _representative[i] = i;
            _set[i].emplace_back(i);
        }
    };

    /**
     * Returns a representative of the set containing e
     */
    [[nodiscard]] NetworKit::node find(NetworKit::node e) const
    {
        return _representative[e];
    }

    /**
     * Unionizes the sets of e1 and e2
     * @param e1, e2 two elements
     */
    void unionized(NetworKit::node e1, NetworKit::node e2)
    {
        if (find(e1) == find(e2))
            return;

        e1 = find(e1);
        e2 = find(e2);

        if (_set[e1].size() < _set[e2].size())
            std::swap(e1, e2);

        for (NetworKit::node v : _set[e2])
        {
            _representative[v] = e1;
            _set[e1].emplace_back(v);
        }
        _set[e2].clear();
    }

    /**
     * Returns how many elements are in the same set as e
     * @param e the element
     */
    [[nodiscard]] NetworKit::count setSize(const NetworKit::node e) const { return _set[find(e)].size(); }

    [[nodiscard]] NetworKit::count numberOfSets() const { return numSets; }

    /**
     * Returns the set containing e
     * @param e the element
     */
    [[nodiscard]] const std::vector<NetworKit::node>& setOf(const NetworKit::node e) const { return _set[find(e)]; }

    /**
     * Returns all sets that currently exist
     */
    std::vector<std::vector<NetworKit::node>> obtainSets()
    {
        std::vector<std::vector<NetworKit::node>> sets;

        for (NetworKit::node v = 0; v < n; v++)
        {
            if (_representative[v] == v)
            {
                sets.emplace_back(_set[v]);
            }
        }

        return sets;
    }

private:
    NetworKit::node n;
    NetworKit::count numSets;

    std::vector<NetworKit::node> _representative;
    std::vector<std::vector<NetworKit::node>> _set;
};

#endif // UNIONFIND_HPP
