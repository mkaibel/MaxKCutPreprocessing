//
// Created by michael-kaibel on 09/02/2026.
//

#ifndef MKCP_CYCLEASSIGNMENTFINDER_HPP
#define MKCP_CYCLEASSIGNMENTFINDER_HPP

#include "networkit/graph/Graph.hpp"

namespace mkcp
{

    class CycleAssignmentFinder
    {
    public:
        explicit CycleAssignmentFinder(unsigned k, const std::vector<std::vector<double>> &assignmentVariables, const NetworKit::Graph &g) : _k(k), _g(g), _assignmentVariables(assignmentVariables) {}

        void run();

        const std::vector<std::pair<unsigned, std::vector<NetworKit::node>>> &getViolatedCycles();

    private:
        unsigned _k;
        const NetworKit::Graph &_g;
        const std::vector<std::vector<double>> &_assignmentVariables;

        bool _hasRun = false;

        std::vector<std::pair<unsigned, std::vector<NetworKit::node>>> _violatedCycles;

        [[nodiscard]] double assignmentTo(NetworKit::node v, unsigned i) const;
    };
}

#endif //MKCP_CYCLEASSIGNMENTFINDER_HPP