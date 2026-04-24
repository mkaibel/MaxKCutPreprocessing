//
// Created by michael-kaibel on 29/01/2026.
//

#ifndef MKCP_SPARSEPAIRINGMODEL_HPP
#define MKCP_SPARSEPAIRINGMODEL_HPP

#include <gurobi_c++.h>
#include <networkit/graph/Graph.hpp>
#include <vector>

#include "globals.hpp"
#include "networkit/Globals.hpp"

namespace mkcp
{
    class SparsePairingCallback : public GRBCallback
    {
    public:
        SparsePairingCallback(const NetworKit::Graph &g, unsigned int k,
                              std::vector<GRBVar>& edgeVars,
                              std::vector<bool> &trueEdge,
                              const std::vector<EdgeConstraint>& edgeConstraints)
            : _g(g), _n(g.numberOfNodes()), _k(k), _edgeVars(edgeVars), _trueEdge(trueEdge), _edgeConstraints(edgeConstraints)
        {
        }

    protected:
        // TODO Implement callback
        void callback() override;

    private:
        const NetworKit::Graph &_g;
        const NetworKit::count _n;
        unsigned int _k;
        std::vector<GRBVar>& _edgeVars;
        std::vector<bool> &_trueEdge;
        const std::vector<EdgeConstraint>& _edgeConstraints;

        /**
         * Checks if the fractional solution violates any chordless cycle constraints, adds appropriate inequalities
         * @return true iff any chordless cycle constraints were broken
         */
        bool addChordlessCycleConstraints();

        [[nodiscard]] unsigned long edgeID(NetworKit::node v, NetworKit::node w) const;

        /**
         * Returns a linear expression consisting out of the edges variable, which account for the case that it may have a constraint placed on it
         * @param id the id of the edge
         * @return a linear expression corresponding to the edge variable
         */
        [[nodiscard]] GRBLinExpr getEdgeVar(NetworKit::node v, NetworKit::node w) const;

        [[nodiscard]] double getEdgeVal(NetworKit::node v, NetworKit::node w);

        [[nodiscard]] double stateSpecificEdgeVal(NetworKit::count eid);
    };


    class SparsePairingModel
    {
    public:
        SparsePairingModel(const NetworKit::Graph& g, const unsigned int k,
                           const double timeLimit = 300, const unsigned seed = 1234) : _g(g), _k(k), _seed(seed),
            _extraConstraints(g.numberOfEdges()), _solution(g.upperNodeIdBound()), _timeLimit(timeLimit)
        {
        }

        void run();

        void setExtraConstraints(const std::vector<EdgeConstraint>& extraConstraints);

        [[nodiscard]] bool hasRun() const;

        [[nodiscard]] bool foundOptimum() const;

        [[nodiscard]] double getObjectiveValue() const;

        [[nodiscard]] double getLowerBound() const;

        [[nodiscard]] double getUpperBound() const;

        [[nodiscard]] const Partition& getSolution() const;

    private:
        const NetworKit::Graph& _g;
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

#endif //MKCP_SPARSEPAIRINGMODEL_HPP
