//
// Created by michael-kaibel on 22/01/2026.
//

#ifndef MKCP_EDGEBASEDMODEL_HPP
#define MKCP_EDGEBASEDMODEL_HPP

#include <gurobi_c++.h>
#include <networkit/graph/Graph.hpp>
#include <vector>

#include "globals.hpp"
#include "networkit/Globals.hpp"

namespace mkcp
{
    class CuttingPlaneAdder : public GRBCallback {
    public:
        CuttingPlaneAdder(NetworKit::count n, unsigned int k,
                          std::vector<GRBVar> &edgeVars)
            : _n(n), _k(k), _edgeVars(edgeVars) {}

    protected:
        // TODO Implement callback
        void callback() override;

    private:
        NetworKit::count _n;
        unsigned int _k;
        std::vector<GRBVar> &_edgeVars;

        [[nodiscard]] unsigned long edgeID(NetworKit::node v, NetworKit::node w) const;
    };


    class EdgeBasedKCutSolver
    {
    public:
        EdgeBasedKCutSolver(const NetworKit::Graph &g, const unsigned int k,
                          const double timeLimit = 300, const unsigned seed = 1234) : _g(g), _k(k), _seed(seed), _solution(g.upperNodeIdBound()), _timeLimit(timeLimit) {}

        void run();

        void setExtraConstraints(const std::vector<EdgeConstraint> &extraConstraints);

        [[nodiscard]] bool hasRun() const;

        [[nodiscard]] bool foundOptimum() const;

        [[nodiscard]] double getObjectiveValue() const;

        [[nodiscard]] double getLowerBound() const;

        [[nodiscard]] double getUpperBound() const;

        [[nodiscard]] const Partition &getSolution() const;

    private:
        const NetworKit::Graph &_g;
        const unsigned int _k;
        const unsigned _seed;

        std::vector<EdgeConstraint> _extraConstraints;
        bool _hasExtraConstraints = false;
        NetworKit::count _numConstrainedEdges = 0;
        NetworKit::edgeweight _offSetByConstraints = 0.0;

        Partition _solution;
        bool _hasRun = false;
        double _objectiveValue = 0.0, _lowerBound = 0.0, _upperBound = 0.0;
        double _timeLimit;
        bool _foundOptimum = false;

        [[nodiscard]] unsigned long edgeID(NetworKit::node v, NetworKit::node w) const;
    };
}

#endif //MKCP_EDGEBASEDMODEL_HPP