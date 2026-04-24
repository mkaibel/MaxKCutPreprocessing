//
// Created by michael-kaibel on 09/02/2026.
//

#ifndef MKCP_CYCLEFINDER_HPP
#define MKCP_CYCLEFINDER_HPP

#include "networkit/graph/Graph.hpp"

namespace mkcp
{
    //! Assumes edge weights are edge variables in pairing formulation (x_uv = 1 means u and v are in the same group)
    class CycleFinder
    {
    public:
        explicit CycleFinder(const NetworKit::Graph &g) : _g(g) {}

        void run();

        const std::vector<std::pair<NetworKit::Edge, std::vector<NetworKit::Edge>>> &getViolatedCycles();

    private:
        const NetworKit::Graph &_g;

        bool _hasRun = false;

        std::vector<std::pair<NetworKit::Edge, std::vector<NetworKit::Edge>>> _violatedCycles;
    };
}

#endif //MKCP_CYCLEFINDER_HPP