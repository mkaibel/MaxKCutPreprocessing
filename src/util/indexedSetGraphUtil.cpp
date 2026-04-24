//
// Created by michael-kaibel on 02/02/2026.
//

#include "util/indexedSetGraphUtil.hpp"

void intersectIndexedSetWithNeighbourhood(IndexedSet& set, NetworKit::node v, const NetworKit::Graph& g)
{
    std::deque<unsigned> inIntersection;

    for (NetworKit::node v : g.neighborRange(v))
    {
        if (set.containsElement(v))
        {
            inIntersection.emplace_back(v);
        }
    }

    while (!set.empty())
    {
        set.removeElement(set.currentElements().back());
    }

    for (unsigned v : inIntersection)
    {
        set.insertElement(v);
    }
}
