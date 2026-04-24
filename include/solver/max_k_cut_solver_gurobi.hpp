#ifndef MAX_K_CUT_SOLVER_GUROBI_HPP
#define MAX_K_CUT_SOLVER_GUROBI_HPP

#include <gurobi_c++.h>
#include <networkit/graph/Graph.hpp>
#include <vector>

#include "globals.hpp"
#include "networkit/Globals.hpp"

namespace mkcp
{
    // Global enviroment so it doesn't get loaded all the time
    extern GRBEnv GlobalGurobiEnv;

    constexpr double epsilon = 0.00001;

    struct CuttingPlaneConfig
    {
        bool shiftedColumn, generalClique, wheels, hypermetric;
    };

    class HeuristicCallback : public GRBCallback
    {
    public:
        HeuristicCallback(const NetworKit::Graph& g, const unsigned int k,
                          const std::vector<std::vector<GRBVar>>& vertexVars,
                          const std::vector<GRBVar>& edgeVars,
                          const std::vector<EdgeConstraint>& edgeConstraints,
                          const std::vector<std::vector<bool>> &vertexConstraints,
                          CuttingPlaneConfig config = {true, true, true, true})
            : _graph(g), _k(k), _vertexVars(vertexVars), _edgeVars(edgeVars), _edgeConstraints(edgeConstraints), _hasVertexConstraints(!vertexConstraints.empty()), _vertexConstraints(vertexConstraints),
              _config(config)
        {
        }

        void setMinValueRequired(double minVal);

    protected:
        void callback() override;

        void checkMinValueRequirementCutoff();

        bool separateCuttingPlanes();

        void runPrimalHeuristic();

    private:
        const NetworKit::Graph& _graph;
        unsigned int _k;
        const std::vector<std::vector<GRBVar>>& _vertexVars;
        const std::vector<GRBVar>& _edgeVars;
        const std::vector<EdgeConstraint>& _edgeConstraints;
        bool _hasVertexConstraints;
        const std::vector<std::vector<bool>> &_vertexConstraints;
        const CuttingPlaneConfig _config;
        unsigned _heuristicCalls = 0;

        bool _isMinValueRequired = false;
        double _minValueRequired = 0.0;

        [[nodiscard]] unsigned int maxColour(NetworKit::node v) const;

        /**
         * Returns a linear expression consisting out of the edges variable, which account for the case that it may have a constraint placed on it
         * @param id the id of the edge
         * @return a linear expression corresponding to the edge variable
         */
        [[nodiscard]] GRBLinExpr getEdgeVar(NetworKit::edgeid id) const;

        /**
         * Returns a linear expression consisting out of the vertex variable, which account for the case that it may have a constraint placed on it
         * @param id the id of the vertex
         * @param colour the colour for which the variable is needed
         * @return a linear expression corresponding to the vertex variable
         */
        [[nodiscard]] GRBLinExpr getVertexVar(NetworKit::node id, partitionID colour) const;
    };

    class MaxKCutSolverGurobi
    {
    public:
        /**
         * Solver for maximum k cut using Gurobi as the underlying ILP solver
         * @param g the graph, which must have edges indexed and setMaintainCompactEdges()
         * @param k value of k
         * @param timeLimit the time limit to solve (may go a few seconds over)
         * @param seed the seed
         * @param config config for which cutting planes should be used, by default all
         */
        MaxKCutSolverGurobi(const NetworKit::Graph& g, unsigned int k,
                            double timeLimit = 300, unsigned seed = 1234,
                            CuttingPlaneConfig config = {true, true, true, true})
            : _graph(g), _k(k), _seed(seed), _config(config),
              _edgeConstraints(g.numberOfEdges(), EdgeConstraint::NONE),
              _solution(g.upperNodeIdBound()), _timeLimit(timeLimit)
        {
        }

        /**
         * Sets an objective value s.t. the solver terminates if as it realizes no solution can be as good as the requirement
         * @param minVal the value
         */
        void setMinValRequired(double minVal);

        /**
         * Sets an objective value s.t. the solver terminates as soon as it finds a primal solution as good as the bound
         * @param minVal the value
         */
        void setMinValueSufficien(double minVal);

        /**
         * Set constraints for edges of the form "edge e must be cut/must not be cut"
         * @param edgeConstraints a vector of constraints indexed by edge IDs with 1 constraint per edge (NONE for no constraint)
         */
        void setEdgeConstraints(const std::vector<EdgeConstraint>& edgeConstraints);

        /**
         * Set constraints for vertices of the form "colour of v < constraint"
         * @param vertexConstraints a vector of bool vectors such that vertexConstraints[v][i] == true means v is not allowed in partition i
         */
        void setVertexConstraints(const std::vector<std::vector<bool>> &vertexConstraints);

        /**
         * Set constraints of the form "at least/at most x vertices in set S of vertices must have colour C"
         * @param groupColourConstraints a vector of constraints of form ((colour, min/max nodes), S = {v, w,...}) where at most x nodes is encoded by positive int and at least by negative
         */
        void setGroupColourConstraints(const std::vector<std::pair<std::pair<partitionID, int>, std::vector<NetworKit::node>>> &groupColourConstraints);

        void run();

        [[nodiscard]] bool hasRun() const;

        [[nodiscard]] bool foundSatisfactorySolution() const;

        [[nodiscard]] double getObjectiveValue() const;

        [[nodiscard]] double getLowerBound() const;

        [[nodiscard]] double getUpperBound() const;

        [[nodiscard]] const Partition& getSolution() const;

    private:
        const NetworKit::Graph& _graph;

        unsigned int _k;

        unsigned _seed;

        CuttingPlaneConfig _config;

        std::vector<EdgeConstraint> _edgeConstraints;
        NetworKit::edgeweight _offSetByConstraints = 0.0;

        bool _hasVertexConstraints = false;
        std::vector<std::vector<bool>> _vertexConstraints;
        std::vector<std::pair<std::pair<partitionID, int>, std::vector<NetworKit::node>>> _groupColourConstraints;

        bool _isMinValRequired = false;
        double _minValRequired = 0.0;

        bool _isMinValSufficient = false;
        double _minValSufficient = 0.0;

        Partition _solution;
        bool _hasRun = false;
        double _objectiveValue = 0.0, _lowerBound = 0.0, _upperBound = 0.0;
        double _timeLimit;
        bool _foundOptimum = false;

        /**
         * Returns min(v + 1, k), the number of colours that v can have in a
         * lexicographically minimal solution
         * @returns min(v+1, k)
         */
        [[nodiscard]] unsigned int maxColour(NetworKit::node v) const;
    };
} // namespace mkcp
#endif // MAX_K_CUT_SOLVER_GUROBI_HPP
