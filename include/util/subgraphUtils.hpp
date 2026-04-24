#ifndef SUBGRAPHUTILS_HPP
#define SUBGRAPHUTILS_HPP

#include <vector>

#include "networkit/Globals.hpp"
#include "networkit/graph/Graph.hpp"

/**
 * Computes the induced subgraph in time O(n + m)
 * @param original the original graph
 * @param nodes the nodes for which we compute the induced subgraph
 * @returns a pair (Graph, originalIDs)
 */
inline std::pair<NetworKit::Graph, std::vector<NetworKit::node>>
getInducedSubgraph(const NetworKit::Graph& original,
                   const std::vector<NetworKit::node>& nodes, std::vector<NetworKit::node>& tempNodeValues)
{
    NetworKit::Graph inducedSubgraph(nodes.size(), true);

    assert(tempNodeValues.size() >= original.upperNodeIdBound());
    std::vector<NetworKit::node> originalID(nodes.size());

    for (NetworKit::node currentAlias = 0; currentAlias < nodes.size();
         currentAlias++)
    {
        tempNodeValues[nodes[currentAlias]] = currentAlias;
        originalID[currentAlias] = nodes[currentAlias];
    }

    for (NetworKit::node v = 0; v < nodes.size(); v++)
    {
        for (std::pair<NetworKit::node, NetworKit::edgeweight> w :
             original.weightNeighborRange(nodes[v]))
        {
            // Add edges to neighbouring vertices only if they are in the subgraph
            // (and therefore have a down alias)
            if (tempNodeValues[w.first] != NetworKit::none && w.first > nodes[v])
            {
                inducedSubgraph.addEdge(tempNodeValues[w.first], tempNodeValues[nodes[v]],
                                        w.second);
            }
        }
    }

    for (NetworKit::node currentAlias = 0; currentAlias < nodes.size();
         currentAlias++)
    {
        tempNodeValues[nodes[currentAlias]] = NetworKit::none;
    }

    return std::pair<NetworKit::Graph, std::vector<NetworKit::node>>{
        inducedSubgraph, originalID
    };
}

inline bool subgraphIsClique(const NetworKit::Graph& graph,
                             const std::vector<NetworKit::node>& vertexSet,
                             std::vector<bool>& boolHelper,
                             NetworKit::edgeweight cliqueEdgeWeight)
{
    assert(boolHelper.size() >= graph.upperNodeIdBound());

    for (NetworKit::node v : vertexSet)
    {
        boolHelper[v] = true;
    }

    bool isClique = true;

    for (NetworKit::node v : vertexSet)
    {
        NetworKit::count numInSetNeighbours = 0;

        for (auto [neighbour, weight] : graph.weightNeighborRange(v))
        {
            if (boolHelper[neighbour])
            {
                if (weight == cliqueEdgeWeight)
                {
                    numInSetNeighbours++;
                }
                else
                {
                    break;
                }
            }
        }

        if (numInSetNeighbours < vertexSet.size() - 1)
        {
            isClique = false;
            break;
        }
    }

    for (NetworKit::node v : vertexSet)
    {
        boolHelper[v] = false;
    }

    return isClique;
}

#endif // SUBGRAPHUTILS_HPP
