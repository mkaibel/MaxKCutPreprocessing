//
// Created by michael-kaibel on 09/02/2026.
//

#include "solver/separationHeuristics/cycleFinder.hpp"

#include "networkit/distance/Dijkstra.hpp"

namespace mkcp
{
    const std::vector<std::pair<NetworKit::Edge, std::vector<NetworKit::Edge>>>& CycleFinder::getViolatedCycles()
    {
        if (!_hasRun)
        {
            throw std::runtime_error("Run cycle finder first");
        }

        return _violatedCycles;
    }

    void CycleFinder::run()
    {
        _hasRun = true;

        // Phase 1: Try to find violated triangles
        std::vector<NetworKit::edgeweight> tempWeights(_g.numberOfNodes(), 0.0);

        for (const NetworKit::WeightedEdge &e : _g.edgeWeightRange())
        {
            if (e.weight >= 1.0)
                continue;

            for (auto [w, weight] : _g.weightNeighborRange(e.u))
            {
                tempWeights[w] = weight;
            }
            NetworKit::node bestTriangleNeighbour = NetworKit::none;
            NetworKit::edgeweight bestTriangleWeight = 0.0;

            for (auto [w, weight] : _g.weightNeighborRange(e.v))
            {
                if (tempWeights[w] + weight > bestTriangleWeight)
                {
                    bestTriangleNeighbour = w;
                    bestTriangleWeight = tempWeights[w] + weight;
                }
            }

            for (NetworKit::node w : _g.neighborRange(e.u))
            {
                tempWeights[w] = 0.0;
            }

            if (bestTriangleWeight > 1.001)
            {
                assert(bestTriangleNeighbour != NetworKit::none);
                assert(_g.hasEdge(e.u, bestTriangleNeighbour));

                _violatedCycles.emplace_back(NetworKit::Edge{e.u, e.v}, std::vector<NetworKit::Edge>{{e.u, bestTriangleNeighbour}, {bestTriangleNeighbour, e.v}});
            }
        }

        if (!_violatedCycles.empty())
            return;

        NetworKit::Graph gInv(_g.numberOfNodes(), true);

        for (const NetworKit::WeightedEdge &e : _g.edgeWeightRange())
        {
            gInv.addEdge(e.u, e.v, std::max(1.0 - e.weight, 0.0));
        }

        NetworKit::Dijkstra dijkstra(gInv, 0);
        std::vector<NetworKit::count> posInPath(_g.numberOfNodes(), 0);

        for (const NetworKit::WeightedEdge &e : _g.edgeWeightRange())
        {
            dijkstra.setSource(e.u);
            dijkstra.setTarget(e.v);
            dijkstra.run();

            if (dijkstra.distance(e.v) < 1.0 - e.weight)
            {
                NetworKit::node v = e.v;
                std::vector<NetworKit::Edge> path;
                NetworKit::edgeweight pathWeight = 0.0;
                unsigned pos = 1;

                while (v != e.u)
                {
                    v = dijkstra.getPredecessors(v).front();
                    posInPath[v] = pos;
                    pos++;
                }

                v = e.v;

                while (v != e.u)
                {
                    NetworKit::node farthestNeighbour = NetworKit::none;
                    NetworKit::edgeweight farthestWeight = 0.0;
                    NetworKit::count maxPos = 0;
                    for (auto [w, weight] : gInv.weightNeighborRange(v))
                    {
                        if (v == e.v && w == e.u)
                            continue;

                        if (posInPath[w] > maxPos)
                        {
                            farthestNeighbour = w;
                            farthestWeight = weight;
                            maxPos = posInPath[w];
                        }
                    }
                    assert(farthestNeighbour != NetworKit::none);

                    path.emplace_back(v, farthestNeighbour);
                    pathWeight += farthestWeight;
                    v = farthestNeighbour;
                }

                if (e.weight > pathWeight)
                    _violatedCycles.emplace_back(NetworKit::Edge{e.u, e.v}, path);

                // Cleanup
                v = e.v;
                while (v != e.u)
                {
                    v = dijkstra.getPredecessors(v).front();
                    posInPath[v] = 0;
                }
            }
        }
    }
}
