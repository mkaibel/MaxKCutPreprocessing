//
// Created by michael-kaibel on 02/02/2026.
//

#ifndef MKCP_INDEXEDSETGRAPHUTIL_HPP
#define MKCP_INDEXEDSETGRAPHUTIL_HPP

#include "indexedSet.hpp"
#include "networkit/graph/Graph.hpp"

/**
 * Intersects an indexed set of vertices with the neighbourhood of v in graph g
 * @param set the set (is modified)
 * @param v the vertex
 * @param g the graph
 */
void intersectIndexedSetWithNeighbourhood(IndexedSet &set, NetworKit::node v, const NetworKit::Graph &g);

#endif //MKCP_INDEXEDSETGRAPHUTIL_HPP