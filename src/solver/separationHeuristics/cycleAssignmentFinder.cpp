//
// Created by michael-kaibel on 09/02/2026.
//

#include "solver/separationHeuristics/cycleAssignmentFinder.hpp"

namespace mkcp
{
    const std::vector<std::pair<unsigned, std::vector<NetworKit::node>>>& CycleAssignmentFinder::getViolatedCycles()
    {
        if (!_hasRun)
        {
            throw std::runtime_error("Run cycle assignment finder first");
        }

        return _violatedCycles;
    }

    void CycleAssignmentFinder::run()
    {
        _hasRun = true;

        for (unsigned i = 0; i < _k; i++)
        {
            NetworKit::Graph gi(_g.numberOfNodes());

            for (const NetworKit::WeightedEdge &e : _g.edgeWeightRange())
            {
                assert(e.weight + 0.5 + 0.5 * (assignmentTo(e.u, i) + assignmentTo(e.v, i)) > -0.00001);
                gi.addEdge(e.u, e.v, std::max(e.weight + 0.5 + 0.5 * (assignmentTo(e.u, i) + assignmentTo(e.v, i)), 0.0));
            }

            // TODO compute odd shortest cycle
        }
    }

    double CycleAssignmentFinder::assignmentTo(NetworKit::node v, unsigned i) const
    {
        assert(v < _g.numberOfNodes());
        assert(i < _k);

        if (i > v)
            return 0.0;

        return _assignmentVariables[v][i];
    }
}
