//
// Created by michael-kaibel on 23/01/2026.
//

#include "solver/separationHeuristics/wheelFinder.hpp"

#include <deque>

#include "util/indexedSet.hpp"

namespace mkcp
{
    const std::vector<std::pair<NetworKit::node, std::vector<NetworKit::node>>>& WheelFinder::getWheels()
    {
        if (!_hasRun)
        {
            throw std::runtime_error("Run wheel finder first");
        }

        return _wheels;
    }

    const std::vector<std::pair<std::pair<NetworKit::node, NetworKit::node>, std::vector<NetworKit::node>>>& WheelFinder::getBicycleWheels()
    {
        if (!_hasRun)
        {
            throw std::runtime_error("Run wheel finder first");
        }

        return _bicycleWheels;
    }

    void WheelFinder::updateCenterCandidates(std::vector<bool> &endpointNeighbour, std::vector<NetworKit::edgeweight> &centerWeight, IndexedSet &centerCandidates, NetworKit::node minWeightNeighbour) const
    {
        for (auto [w, weight] : _g.weightNeighborRange(minWeightNeighbour))
        {
            if (!centerCandidates.containsElement(w))
                continue;

            centerWeight[w] += weight;
            endpointNeighbour[w] = true;
        }

        std::deque<NetworKit::node> killList;
        for (NetworKit::node w : centerCandidates.currentElements())
        {
            if (!endpointNeighbour[w])
            {
                killList.push_back(w);
            }

            endpointNeighbour[w] = false;
        }

        for (NetworKit::node w : killList)
        {
            centerCandidates.removeElement(w);
        }
    }

    void WheelFinder::findViolatedWheel(const std::vector<NetworKit::edgeweight> &centerWeight, const IndexedSet &centerCandidates, const std::vector<NetworKit::node> &wheel, NetworKit::edgeweight wheelCycleWeight)
    {
        if (!centerCandidates.empty())
        {
            NetworKit::node bestCenter = NetworKit::none;
            NetworKit::edgeweight bestCenterVal = wheel.size() / 2;

            // Add a wheel inequality for every center that violates it
            for (NetworKit::node w : centerCandidates.currentElements())
            {
                if (centerWeight[w] - wheelCycleWeight > bestCenterVal)
                {
                    bestCenter = w;
                    bestCenterVal = centerWeight[w] - wheelCycleWeight;
                }
            }

            if (bestCenter != NetworKit::none)
            {
                _wheels.emplace_back(bestCenter, wheel);
            }
        }
    }

    void WheelFinder::findViolatedBicycleWheel(const std::vector<NetworKit::edgeweight> &centerWeight, const IndexedSet &centerCandidates, const std::vector<NetworKit::node> &wheel, NetworKit::edgeweight wheelCycleWeight)
    {
        if (centerCandidates.currentElements().size() >= 2)
        {
            std::pair<NetworKit::node, NetworKit::node> bestCenter = {NetworKit::none, NetworKit::none};
            NetworKit::edgeweight bestCenterVal = wheel.size() - 1;

            // Add a wheel inequality for every center that violates it
            for (NetworKit::node v : centerCandidates.currentElements())
            {
                for (NetworKit::node w : centerCandidates.currentElements())
                {
                    if (v > w || !_g.hasEdge(v, w))
                        continue;

                    NetworKit::edgeweight vwWeight = _g.weight(v, w);

                    // This case means that v and w are definitely centers of violated wheel inequalities, which would also separate this bicycle wheel
                    if (vwWeight == 1.0)
                        continue;

                    if (centerWeight[w] + centerWeight[w] - wheelCycleWeight - vwWeight> bestCenterVal)
                    {
                        bestCenter = {v, w};
                        bestCenterVal = centerWeight[w] + centerWeight[w] - wheelCycleWeight;
                    }
                }
            }

            if (bestCenter.first != NetworKit::none)
            {
                _bicycleWheels.emplace_back(bestCenter, wheel);
            }
        }
    }

    void WheelFinder::run()
    {
        _hasRun = true;

        std::vector<bool> vNeighbour(_g.numberOfNodes(), false);
        std::vector<bool> endpointNeighbour(_g.numberOfNodes(), false);
        std::vector<NetworKit::edgeweight> cycleWeight(_g.numberOfNodes(), 0.0);
        std::vector<NetworKit::edgeweight> centerWeight(_g.numberOfNodes(), 0.0);

        IndexedSet centerCandidates(_g.numberOfNodes());

        for (NetworKit::node v = 0; v < _g.numberOfNodes() - 2; v++)
        {
            if (_g.degree(v) == 0)
                continue;

            // For every vertex we try to greedily find odd wheels with v as the lowest ID vertex

            std::vector<NetworKit::node> wheel(1, v);
            NetworKit::edgeweight wheelCycleWeight = 0.0;

            NetworKit::edgeweight minWeight = 3.0;
            NetworKit::node minWeightNeighbour = NetworKit::none;

            // Find cheapest first entry for wheel and initialise helper data structures for wheel finding
            for (auto [w, weight] : _g.weightNeighborRange(v))
            {
                centerCandidates.insertElement(w);
                centerWeight[w] = weight;

                if (w < v)
                    continue;

                vNeighbour[w] = true;
                cycleWeight[w] = weight;

                if (weight < minWeight)
                {
                    minWeight = weight;
                    minWeightNeighbour = w;
                }
            }

            if (minWeightNeighbour == NetworKit::none)
            {
                for (const NetworKit::node w : _g.neighborRange(v))
                {
                    centerCandidates.removeElement(w);
                    centerWeight[w] = 0.0;
                }

                continue;
            }

            assert(minWeightNeighbour != NetworKit::none);

            // Account for the fact that the edge will be removed from the weight tally once
            wheelCycleWeight += 2.0 * minWeight;
            wheel.emplace_back(minWeightNeighbour);
            vNeighbour[minWeightNeighbour] = false;
            centerCandidates.removeElement(minWeightNeighbour);

            updateCenterCandidates(endpointNeighbour, centerWeight, centerCandidates, minWeightNeighbour);

            while (!centerCandidates.empty())
            {
                minWeight = 3.0;
                minWeightNeighbour = NetworKit::none;

                for (auto [w, weight] : _g.weightNeighborRange(wheel.back()))
                {
                    if (!vNeighbour[w])
                        continue;

                    if (cycleWeight[w] + weight < minWeight)
                    {
                        minWeight = cycleWeight[w] + weight;
                        minWeightNeighbour = w;
                    }
                }

                if (minWeightNeighbour == NetworKit::none)
                    break;

                wheelCycleWeight -= cycleWeight[wheel.back()];
                wheelCycleWeight += minWeight;
                wheel.emplace_back(minWeightNeighbour);
                vNeighbour[minWeightNeighbour] = false;
                if (centerCandidates.containsElement(minWeightNeighbour))
                    centerCandidates.removeElement(minWeightNeighbour);

                updateCenterCandidates(endpointNeighbour, centerWeight, centerCandidates, minWeightNeighbour);

                if (wheel.size() % 2 == 1)
                {
                    // Find violated wheels and bicycle wheels
                    findViolatedWheel(centerWeight, centerCandidates, wheel, wheelCycleWeight);
                    findViolatedBicycleWheel(centerWeight, centerCandidates, wheel, wheelCycleWeight);
                }
            }

            for (NetworKit::node w : _g.neighborRange(v))
            {
                vNeighbour[w] = false;
                cycleWeight[w] = 0.0;
                centerWeight[w] = 0.0;
                if (centerCandidates.containsElement(w))
                {
                    centerCandidates.removeElement(w);
                }
            }
        }
    }
}
