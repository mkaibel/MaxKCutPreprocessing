//
// Created by michael-kaibel on 22/01/2026.
//

#include "solver/separationHeuristics/cliqueFinder.hpp"

#include "util/indexedSet.hpp"

namespace mkcp
{
    const std::vector<std::vector<NetworKit::node>>& CliqueFinder::getCliques()
    {
        if (!_hasRun)
        {
            throw std::runtime_error("Run clique finder first");
        }

        return _cliques;
    }

    void CliqueFinder::run()
    {
        _hasRun = true;

        std::vector<bool> tempBoolValues(_g.numberOfNodes(), false);
        std::vector<NetworKit::edgeweight> tempWeights(_g.numberOfNodes(), 0.0);

        IndexedSet neighbourhoodCandidates(_g.numberOfNodes());

        for (NetworKit::node v = 0; v < _g.numberOfNodes() - std::min(_g.numberOfNodes(), static_cast<NetworKit::node>(_k)); v++)
        {
            // Greedily find a clique
            std::vector<NetworKit::node> clique(1, v);

            for (auto [w, weight] : _g.weightNeighborRange(v))
            {
                // We only care about a clique with v as the smallest vertex ID
                if (w < v)
                {
                    continue;
                }

                tempWeights[w] = weight;
                neighbourhoodCandidates.insertElement(w);
            }

            double lossSoFar = 0.0;

            double minLoss;
            NetworKit::node minLossNeighbour;

            unsigned noGoodCliqueCounter = 0;

            while (!neighbourhoodCandidates.empty())
            {
                minLoss = clique.size()+ 10.0;
                minLossNeighbour = NetworKit::none;

                for (NetworKit::node neighbour : neighbourhoodCandidates.currentElements())
                {
                    if (tempWeights[neighbour] < minLoss)
                    {
                        minLoss = tempWeights[neighbour];
                        minLossNeighbour = neighbour;
                    }
                }

                assert(minLossNeighbour != NetworKit::none);

                lossSoFar += minLoss;

                for (auto [w, weight] : _g.weightNeighborRange(minLossNeighbour))
                {
                    if (neighbourhoodCandidates.containsElement(w))
                    {
                        tempWeights[w] += weight;
                        tempBoolValues[w] = true;
                    }
                }

                neighbourhoodCandidates.removeElement(minLossNeighbour);
                clique.emplace_back(minLossNeighbour);

                std::deque<NetworKit::node> kickList;

                for (NetworKit::node w : neighbourhoodCandidates.currentElements())
                {
                    if (!tempBoolValues[w])
                    {
                        kickList.push_back(w);
                    }
                    else
                    {
                        tempBoolValues[w] = false;
                    }
                }

                for (const NetworKit::node w : kickList)
                {
                    neighbourhoodCandidates.removeElement(w);
                }

                unsigned p = clique.size() % _k;
                unsigned q = clique.size() / _k;
                // Check if clique is facet defining
                if (p != 0 && clique.size() > _k && (lossSoFar < q * (q+1) / 2 * p + q * (q - 1) / 2 * (_k - p)))
                {
                    _cliques.emplace_back(clique);
                }
                else
                {
                    noGoodCliqueCounter++;
                    if (noGoodCliqueCounter > 2 * _k)
                    {
                        break;
                    }
                }
            }

            // Clean up helper data structures
            for (NetworKit::node w : _g.neighborRange(v))
            {
                tempBoolValues[w] = false;
                tempWeights[w] = 0.0;
                if (neighbourhoodCandidates.containsElement(w))
                {
                    neighbourhoodCandidates.removeElement(w);
                }
            }
        }
    }
}
