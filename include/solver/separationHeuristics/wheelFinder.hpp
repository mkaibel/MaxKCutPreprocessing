//
// Created by michael-kaibel on 23/01/2026.
//

#ifndef MKCP_WHEELFINDER_HPP
#define MKCP_WHEELFINDER_HPP

#include "networkit/graph/Graph.hpp"
#include "util/indexedSet.hpp"

namespace mkcp
{
    //! Assumes edge weights are edge variables in pairing formulation (x_uv = 1 means u and v are in the same group)
    class WheelFinder
    {
    public:
        explicit WheelFinder(const NetworKit::Graph &g) : _g(g) {}

        void run();

        const std::vector<std::pair<NetworKit::node, std::vector<NetworKit::node>>> &getWheels();

        const std::vector<std::pair<std::pair<NetworKit::node, NetworKit::node>, std::vector<NetworKit::node>>> &getBicycleWheels();

    private:
        const NetworKit::Graph &_g;

        bool _hasRun = false;

        std::vector<std::pair<NetworKit::node, std::vector<NetworKit::node>>> _wheels;
        std::vector<std::pair<std::pair<NetworKit::node, NetworKit::node>, std::vector<NetworKit::node>>> _bicycleWheels;

        void updateCenterCandidates(std::vector<bool>& endpointNeighbour,
                                    std::vector<NetworKit::edgeweight>& centerWeight,
                                    IndexedSet& centerCandidates, NetworKit::node minWeightNeighbour) const;
        void findViolatedWheel(const std::vector<NetworKit::edgeweight>& centerWeight, const IndexedSet& centerCandidates,
                               const std::vector<NetworKit::node>& wheel, NetworKit::edgeweight wheelCycleWeight);
        void findViolatedBicycleWheel(const std::vector<NetworKit::edgeweight>& centerWeight, const IndexedSet& centerCandidates,
                               const std::vector<NetworKit::node>& wheel, NetworKit::edgeweight wheelCycleWeight);
    };
}

#endif //MKCP_WHEELFINDER_HPP