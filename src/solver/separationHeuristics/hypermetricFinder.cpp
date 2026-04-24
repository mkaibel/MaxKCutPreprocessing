//
// Created by michael-kaibel on 02/02/2026.
//

#include "solver/separationHeuristics/hypermetricFinder.hpp"

#include "util/indexedSet.hpp"
#include "util/indexedSetGraphUtil.hpp"

namespace mkcp
{
    const std::vector<std::pair<std::vector<NetworKit::node>, std::vector<NetworKit::node>>>& HypermetricFinder::getPlusMinusOne()
    {
        if (!_hasRun)
        {
            throw std::runtime_error("Run hypermetric finder first");
        }

        return _hypermetricSetsPlusMinusOne;
    }

    void HypermetricFinder::run()
    {
        _hasRun = true;

        std::vector<NetworKit::edgeweight> sideAGain(_g.numberOfNodes(), 0.0);
        IndexedSet expansionCandidates(_g.numberOfNodes());

        for (NetworKit::node v : _g.nodeRange())
        {
            std::vector<NetworKit::node> sideA(1, v), sideB;
            NetworKit::edgeweight currentVarSum = 0.0;

            NetworKit::edgeweight bestSideBStartWeight = 2.0;
            NetworKit::node bestSideBStart = NetworKit::none;

            for (auto [w, weight] : _g.weightNeighborRange(v))
            {
                if (w < v)
                    continue;

                sideAGain[w] += weight;
                expansionCandidates.insertElement(w);

                if (weight < bestSideBStartWeight)
                {
                    bestSideBStart = w;
                    bestSideBStartWeight = weight;
                }
            }

            if (bestSideBStart == NetworKit::none)
            {
                // Case that v has no neighbours of higher ID
                continue;
            }

            sideB.emplace_back(bestSideBStart);
            currentVarSum -= sideAGain[bestSideBStart];

            intersectIndexedSetWithNeighbourhood(expansionCandidates, bestSideBStart, _g);
            updateSideGain(v, expansionCandidates, false, sideAGain);

            while (!expansionCandidates.empty())
            {
                NetworKit::node bestCandidate = NetworKit::none;
                NetworKit::edgeweight bestWeight = -1.0 * (sideA.size() + sideB.size());
                bool onSideA = false;

                for (unsigned w : expansionCandidates.currentElements())
                {
                    if (sideAGain[w] > bestWeight)
                    {
                        bestCandidate = w;
                        bestWeight = sideAGain[w];
                        onSideA = true;
                    }
                    if (-sideAGain[w] > bestWeight)
                    {
                        bestCandidate = w;
                        bestWeight = -sideAGain[w];
                        onSideA = false;
                    }
                }

                assert(bestCandidate != NetworKit::none);

                if (onSideA)
                {
                    sideA.emplace_back(bestCandidate);
                }
                else
                {
                    sideB.emplace_back(bestCandidate);
                }
                currentVarSum += bestWeight;

                intersectIndexedSetWithNeighbourhood(expansionCandidates, bestCandidate, _g);
                updateSideGain(bestCandidate, expansionCandidates, onSideA, sideAGain);

                int eta = sideA.size() - sideB.size();
                if (eta < 0)
                    eta = -eta;

                int vMinusSize = std::min(sideA.size(), sideB.size());

                if ((sideA.size() > _k || sideB.size() > _k) && (eta % _k != 0))
                {
                    if (currentVarSum < computeConstantForSize(eta) - vMinusSize)
                    {
                        if (sideB.size() > sideA.size())
                        {
                            _hypermetricSetsPlusMinusOne.emplace_back(sideB, sideA);
                        }
                        else
                        {
                            _hypermetricSetsPlusMinusOne.emplace_back(sideA, sideB);
                        }

                        for (NetworKit::node v : sideA)
                        {
                            for (NetworKit::node w : sideA)
                            {
                                if (v != w)
                                    assert(_g.hasEdge(v, w));
                            }
                        }

                        for (NetworKit::node v : sideA)
                        {
                            for (NetworKit::node w : sideB)
                            {
                                if (v != w)
                                    assert(_g.hasEdge(v, w));
                            }
                        }

                        for (NetworKit::node v : sideB)
                        {
                            for (NetworKit::node w : sideB)
                            {
                                if (v != w)
                                    assert(_g.hasEdge(v, w));
                            }
                        }
                    }
                }
            }

            for (NetworKit::node w : _g.neighborRange(v))
            {
                sideAGain[w] = 0.0;
            }
        }
    }

    void HypermetricFinder::updateSideGain(NetworKit::node v, const IndexedSet &candidates, bool sideA, std::vector<NetworKit::edgeweight>& sideAGain) const
    {
        for (auto [w, weight] : _g.weightNeighborRange(v))
        {
            if (!candidates.containsElement(w))
                continue;

            if (sideA)
            {
                sideAGain[w] += weight;
            }
            else
            {
                sideAGain[w] -= weight;
            }
        }
    }

    NetworKit::edgeweight HypermetricFinder::computeConstantForSize(unsigned numNodes)
    {
        unsigned p = numNodes / _k;
        unsigned q = numNodes % _k;

        return q * p * (p + 1) / 2 + (_k - q) * p * (p - 1) / 2;
    }
}
