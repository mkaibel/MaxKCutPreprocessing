#include "solver/max_k_cut_solver_gurobi.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "globals.hpp"
#include "gurobi_c++.h"
#include <gurobi_c.h>
#include <sys/stat.h>

#include "networkit/Globals.hpp"
#include "networkit/graph/Graph.hpp"
#include "solver/localSearchHeuristic.hpp"
#include "solver/separationHeuristics/cliqueFinder.hpp"
#include "solver/separationHeuristics/cycleFinder.hpp"
#include "solver/separationHeuristics/hypermetricFinder.hpp"
#include "solver/separationHeuristics/shiftedColumnFinder.hpp"
#include "solver/separationHeuristics/wheelFinder.hpp"

namespace mkcp
{
    // Initializing global gurobi enviroment
    GRBEnv GlobalGurobiEnv;

    void HeuristicCallback::setMinValueRequired(double minVal)
    {
        _isMinValueRequired = true;
        _minValueRequired = minVal;
    }

    unsigned int HeuristicCallback::maxColour(NetworKit::node v) const
    {
        if (_hasVertexConstraints)
            return _k;

        return std::min(v + 1, static_cast<NetworKit::node>(_k));
    }

    GRBLinExpr HeuristicCallback::getEdgeVar(NetworKit::edgeid id) const
    {
        switch (_edgeConstraints[id])
        {
        case EdgeConstraint::NONE:
            return _edgeVars[id];
            break;
        case EdgeConstraint::IN_CUT:
            return 0.0;
            break;
        default:
            return 1.0;
            break;
        }
    }

    GRBLinExpr HeuristicCallback::getVertexVar(NetworKit::node id, partitionID colour) const
    {
        if (colour >= maxColour(id))
            return 0.0;
        if (_hasVertexConstraints && _vertexConstraints[id][colour])
            return 0.0;

        return _vertexVars[id][colour];
    }

    void HeuristicCallback::callback()
    {
        checkMinValueRequirementCutoff();
        separateCuttingPlanes();
    }

    void HeuristicCallback::checkMinValueRequirementCutoff()
    {
        if (where == GRB_CB_MIP && _isMinValueRequired) {
            double dual_bound = getDoubleInfo(GRB_CB_MIP_OBJBND);

            if (dual_bound > _minValueRequired * (1.0 + epsilon)) {
                abort();
            }
        }
    }

    bool HeuristicCallback::separateCuttingPlanes()
    {
        bool separatedPlanes = false;

        if (where == GRB_CB_MIPNODE &&
            getIntInfo(GRB_CB_MIPNODE_STATUS) == GRB_OPTIMAL)
        {
            // Add symmetry breaking constraints, only if no vertex constraints are present
            if (_config.shiftedColumn && !_hasVertexConstraints)
            {
                std::vector<std::vector<double>> assignmentVariables(_graph.numberOfNodes());

                for (NetworKit::node v = 0; v < _graph.numberOfNodes(); v++)
                {
                    for (unsigned i = 0; i < maxColour(v); i++)
                    {
                        assignmentVariables[v].emplace_back(getNodeRel(_vertexVars[v][i]));
                    }
                }

                ShiftedColumnFinder shiftedColumnFinder(assignmentVariables, _graph.numberOfNodes(), _k);
                shiftedColumnFinder.run();

                for (auto& shiftedColumn : shiftedColumnFinder.getShiftedColumns())
                {
                    GRBLinExpr shiftedConstraint;

                    for (ArrayCoord coord : shiftedColumn.second)
                    {
                        shiftedConstraint -= getVertexVar(coord.x, coord.y);
                    }
                    for (unsigned i = shiftedColumn.first.y; i < maxColour(shiftedColumn.first.x); i++)
                    {
                        shiftedConstraint += getVertexVar(shiftedColumn.first.x, i);
                    }

                    addCut(shiftedConstraint, GRB_LESS_EQUAL, 0.0);
                    separatedPlanes = true;
                }
            }

            NetworKit::Graph g(_graph.numberOfNodes(), true);

            for (NetworKit::Edge e : _graph.edgeRange())
            {
                NetworKit::edgeid id = _graph.edgeId(e.u, e.v);
                switch (_edgeConstraints[id])
                {
                case EdgeConstraint::NONE:
                    // Cutting plane finders assume that variables are set the other way around that the assignment model
                    g.addEdge(e.u, e.v, getNodeRel(_edgeVars[id]));
                    break;
                case EdgeConstraint::IN_CUT:
                    g.addEdge(e.u, e.v, 0.0);
                    break;
                case EdgeConstraint::OUT_CUT:
                    g.addEdge(e.u, e.v, 1.0);
                    break;
                }
            }

            // TODO maybe add this as an option to the config

            CycleFinder cycleFinder(g);
            cycleFinder.run();

            for (const auto& cycle : cycleFinder.getViolatedCycles())
            {
                GRBLinExpr cycleConstraint = getEdgeVar(_graph.edgeId(cycle.first.u, cycle.first.v));

                for (const NetworKit::Edge& e : cycle.second)
                {
                    cycleConstraint -= getEdgeVar(_graph.edgeId(e.u, e.v));
                }

                addCut(cycleConstraint, GRB_GREATER_EQUAL, -static_cast<double>(cycle.second.size()) + 1.0);
                separatedPlanes = true;
            }

            if (_config.generalClique)
            {
                CliqueFinder cliqueFinder(g, _k);
                cliqueFinder.run();

                for (const std::vector<NetworKit::node>& clique : cliqueFinder.getCliques())
                {
                    GRBLinExpr cliqueConstraint;
                    for (unsigned i = 0; i < clique.size(); i++)
                    {
                        for (unsigned j = i + 1; j < clique.size(); j++)
                        {
                            NetworKit::edgeid id = _graph.edgeId(clique[i], clique[j]);
                            cliqueConstraint += getEdgeVar(id);
                        }
                    }

                    const unsigned p = clique.size() % _k;
                    const unsigned q = clique.size() / _k;

                    addCut(cliqueConstraint, GRB_GREATER_EQUAL,
                           static_cast<double>(q * (q + 1) / 2 * p + q * (q -
                               1) / 2 * (_k - p)));
                    separatedPlanes = true;
                }
            }

            if (_config.wheels)
            {
                WheelFinder wheelFinder(g);
                wheelFinder.run();

                for (const auto& wheel : wheelFinder.getWheels())
                {
                    GRBLinExpr wheelConstraint;

                    for (unsigned i = 0; i < wheel.second.size(); i++)
                    {
                        assert(_graph.hasEdge(wheel.first, wheel.second[i]));
                        assert(_graph.hasEdge(wheel.second[i], wheel.second[(i + 1) % wheel.second.size()]));
                        const NetworKit::edgeid idSpine = _graph.edgeId(wheel.first, wheel.second[i]);
                        const NetworKit::edgeid idCycle = _graph.edgeId(wheel.second[i],
                                                                        wheel.second[(i + 1) % wheel.second.size()]);
                        wheelConstraint += getEdgeVar(idSpine);
                        wheelConstraint -= getEdgeVar(idCycle);
                    }

                    addCut(wheelConstraint, GRB_LESS_EQUAL, static_cast<double>((wheel.second.size() - 1) / 2));
                    separatedPlanes = true;
                }

                for (const auto& wheel : wheelFinder.getBicycleWheels())
                {
                    GRBLinExpr bicycleConstraint;

                    for (unsigned i = 0; i < wheel.second.size(); i++)
                    {
                        assert(_graph.hasEdge(wheel.first.first, wheel.second[i]));
                        assert(_graph.hasEdge(wheel.first.second, wheel.second[i]));
                        assert(_graph.hasEdge(wheel.second[i], wheel.second[(i + 1) % wheel.second.size()]));
                        const NetworKit::edgeid idSpineV = _graph.edgeId(wheel.first.first, wheel.second[i]);
                        const NetworKit::edgeid idSpineW = _graph.edgeId(wheel.first.second, wheel.second[i]);
                        const NetworKit::edgeid idCycle = _graph.edgeId(wheel.second[i],
                                                                        wheel.second[(i + 1) % wheel.second.size()]);
                        bicycleConstraint += getEdgeVar(idSpineV);
                        bicycleConstraint += getEdgeVar(idSpineW);
                        bicycleConstraint -= getEdgeVar(idCycle);
                    }

                    bicycleConstraint -= getEdgeVar(_graph.edgeId(wheel.first.first, wheel.first.second));

                    addCut(bicycleConstraint, GRB_LESS_EQUAL, static_cast<double>(wheel.second.size() - 1));
                    separatedPlanes = true;
                }
            }

            if (_config.hypermetric)
            {
                HypermetricFinder hypermetricFinder(g, _k);
                hypermetricFinder.run();

                for (auto [sideA, sideB] : hypermetricFinder.getPlusMinusOne())
                {
                    GRBLinExpr hypermetricConstraint;

                    for (unsigned i = 0; i < sideA.size(); i++)
                    {
                        for (unsigned j = i + 1; j < sideA.size(); j++)
                        {
                            hypermetricConstraint += getEdgeVar(_graph.edgeId(sideA[i], sideA[j]));
                        }
                    }

                    for (unsigned i = 0; i < sideB.size(); i++)
                    {
                        for (unsigned j = i + 1; j < sideB.size(); j++)
                        {
                            hypermetricConstraint += getEdgeVar(_graph.edgeId(sideB[i], sideB[j]));
                        }
                    }

                    for (NetworKit::node i : sideA)
                    {
                        for (NetworKit::node j : sideB)
                        {
                            hypermetricConstraint -= getEdgeVar(_graph.edgeId(i, j));
                        }
                    }

                    unsigned eta = sideA.size() - sideB.size();
                    unsigned p = eta / _k;
                    unsigned q = eta % _k;

                    addCut(hypermetricConstraint, GRB_GREATER_EQUAL,
                           q * p * (p + 1) / 2 + (_k - q) * p * (p - 1) / 2 - sideB.size());
                    separatedPlanes = true;
                }
            }
        }

        return separatedPlanes;
    }

    void HeuristicCallback::runPrimalHeuristic()
    {
        if (where == GRB_CB_MIPNODE &&
            getIntInfo(GRB_CB_MIPNODE_STATUS) == GRB_OPTIMAL)
        {
            _heuristicCalls++;

            NetworKit::Graph g(_graph.numberOfNodes(), true);

            // TODO rethink if this is how the graph for rounding should be build, maybe use negative values only for negative weight edges
            for (NetworKit::Edge e : _graph.edgeRange())
            {
                NetworKit::edgeid id = _graph.edgeId(e.u, e.v);
                switch (_edgeConstraints[id])
                {
                case EdgeConstraint::NONE:
                    // Cutting plane finders assume that variables are set the other way around that the assignment model
                    g.addEdge(e.u, e.v, 1.0 - 2.0 * getNodeRel(_edgeVars[id]));
                    break;
                case EdgeConstraint::IN_CUT:
                    g.addEdge(e.u, e.v, 1.0 * static_cast<NetworKit::edgeweight>(_graph.numberOfEdges()));
                    break;
                case EdgeConstraint::OUT_CUT:
                    g.addEdge(e.u, e.v, -1.0 * static_cast<NetworKit::edgeweight>(_graph.numberOfEdges()));
                    break;
                }
            }

            // TODO add in that there may be vertex constraints
            KCutLocalSearch heuristic(_graph, _k, 1000000, _heuristicCalls, 5, 100, 50);
            if (_hasVertexConstraints)
                heuristic.addVertexConstraints(_vertexConstraints);
            heuristic.run();
            const Partition& heuristicSolution = heuristic.getSolution();

            NetworKit::edgeweight heuristicValue = computeKPartitionValue(_graph, heuristicSolution);

            // If the new heuristic solution beats the current optimum we overwrite it
            if (heuristicValue <
                getDoubleInfo(GRB_CB_MIPNODE_OBJBST))
            {
                bool respectsEdgeConstraints = true;

                for (NetworKit::Edge e : _graph.edgeRange())
                {
                    NetworKit::edgeid id = _graph.edgeId(e.u, e.v);
                    switch (_edgeConstraints[id])
                    {
                    case EdgeConstraint::NONE:
                        // Cutting plane finders assume that variables are set the other way around that the assignment model
                        break;
                    case EdgeConstraint::IN_CUT:
                        respectsEdgeConstraints &= heuristicSolution.assignmentOf(e.u) != heuristicSolution.
                            assignmentOf(e.v);
                        break;
                    case EdgeConstraint::OUT_CUT:
                        respectsEdgeConstraints &= heuristicSolution.assignmentOf(e.u) == heuristicSolution.
                            assignmentOf(e.v);
                        break;
                    }

                    if (!respectsEdgeConstraints)
                        break;
                }

                for (NetworKit::node v = 0; v < _graph.numberOfNodes(); v++)
                {
                    setSolution(_vertexVars[v][heuristicSolution.assignmentOf(v)], 1.0);
                    for (unsigned int i = 0; i < heuristicSolution.assignmentOf(v); i++)
                    {
                        if (!_hasVertexConstraints || _vertexConstraints[v][i] == false)
                            setSolution(_vertexVars[v][i], 0.0);
                    }
                    for (unsigned int i = heuristicSolution.assignmentOf(v) + 1;
                         i < maxColour(v); i++)
                    {
                        if (!_hasVertexConstraints || _vertexConstraints[v][i] == false)
                            setSolution(_vertexVars[v][i], 0.0);
                    }
                }

                for (auto e : _graph.edgeRange())
                {
                    NetworKit::edgeid eid = _graph.edgeId(e.u, e.v);

                    if (_edgeConstraints[eid] != EdgeConstraint::NONE)
                    {
                        if (heuristicSolution.assignmentOf(e.u) ==
                        heuristicSolution.assignmentOf(e.v))
                        {
                            setSolution(_edgeVars[eid], 1.0);
                        }
                        else
                        {
                            setSolution(_edgeVars[eid], 0.0);
                        }
                    }
                }
            }
        }
    }

    bool MaxKCutSolverGurobi::hasRun() const { return _hasRun; }

    unsigned int MaxKCutSolverGurobi::maxColour(NetworKit::node v) const
    {
        if (_hasVertexConstraints)
            return _k;

        return std::min(v + 1, static_cast<NetworKit::node>(_k));
    }

    bool MaxKCutSolverGurobi::foundSatisfactorySolution() const { return _foundOptimum; }

    double MaxKCutSolverGurobi::getObjectiveValue() const
    {
        return _objectiveValue;
    }

    double MaxKCutSolverGurobi::getLowerBound() const { return _lowerBound; }

    double MaxKCutSolverGurobi::getUpperBound() const { return _upperBound; }

    const Partition& MaxKCutSolverGurobi::getSolution() const
    {
        return _solution;
    }

    void MaxKCutSolverGurobi::setEdgeConstraints(
        const std::vector<EdgeConstraint>& edgeConstraints)
    {
        _edgeConstraints = edgeConstraints;
    }

    void MaxKCutSolverGurobi::setVertexConstraints(const std::vector<std::vector<bool>>& vertexConstraints)
    {
        _hasVertexConstraints = true;
        _vertexConstraints = vertexConstraints;
    }

    void MaxKCutSolverGurobi::setMinValRequired(double minVal)
    {
        _isMinValRequired = true;
        _minValRequired = minVal;
    }

    void MaxKCutSolverGurobi::setMinValueSufficien(double minVal)
    {
        _isMinValSufficient = true;
        _minValSufficient = minVal;
    }

    void MaxKCutSolverGurobi::setGroupColourConstraints(
        const std::vector<std::pair<std::pair<partitionID, int>, std::vector<NetworKit::node>>>& groupColourConstraints)
    {
        _groupColourConstraints = groupColourConstraints;
    }

    void MaxKCutSolverGurobi::run()
    {
        _hasRun = true;

        // try
        // {
            GRBModel model(GlobalGurobiEnv);

            unsigned int n = _graph.numberOfNodes();

            double totalEdgeWeight = 0.0;

            // Heuristic and solver measure times in miliseconds vs seconds, warm start should take at most 1/4 of total time
            KCutLocalSearch heuristic(_graph, _k, _timeLimit * 250, _seed, 15, 150, 100);
            if (_hasVertexConstraints)
                heuristic.addVertexConstraints(_vertexConstraints);
            heuristic.run();
            const Partition &warmStart = heuristic.getSolution();

            std::vector<std::vector<GRBVar>> vertexVars(_graph.numberOfNodes());
            for (NetworKit::node v = 0; v < n; v++)
            {
                vertexVars[v].resize(maxColour(v));
            }

            for (NetworKit::node v = 0; v < n; v++)
            {
                GRBLinExpr oneColourConstraint = 0;
                for (unsigned int i = 0; i < maxColour(v); i++)
                {
                    if (!_hasVertexConstraints || !_vertexConstraints[v][i])
                    {
                        vertexVars[v][i] =
                        model.addVar(0.0, 1.0, 0.0, GRB_BINARY,
                                     "x_" + std::to_string(v) + std::to_string(i));
                        vertexVars[v][i].set(GRB_DoubleAttr_Start, warmStart.assignmentOf(v) == i);
                        oneColourConstraint += vertexVars[v][i];
                    }
                }

                model.addConstr(oneColourConstraint == 1,
                                "oneColourConstraint_" + std::to_string(v));
            }

            for (const auto& [constraint, vertices] : _groupColourConstraints)
            {
                GRBLinExpr groupColourConstraint = 0;
                for (NetworKit::node v : vertices)
                {
                    if (constraint.first < maxColour(v) && (!_hasVertexConstraints || !_vertexConstraints[v][constraint.first]))
                        groupColourConstraint += vertexVars[v][constraint.first];
                }

                if (constraint.second >= 0)
                {
                    model.addConstr(groupColourConstraint <= constraint.second, "groupColourConstraint");
                }
                else
                {
                    model.addConstr(groupColourConstraint >= -constraint.second, "groupColourConstraint");
                }
            }

            std::vector<GRBVar> edgeVars(_graph.numberOfEdges());
            unsigned int j = 0;
            for (auto e : _graph.edgeWeightRange())
            {
                if (e.u > e.v)
                {
                    throw std::runtime_error(
                        "Weird behaviour in adding edges, must be investigated");
                }

                totalEdgeWeight += e.weight;

                if (_edgeConstraints[_graph.edgeId(e.u, e.v)] != EdgeConstraint::NONE)
                {
                    if (_edgeConstraints[_graph.edgeId(e.u, e.v)] ==
                        EdgeConstraint::IN_CUT)
                    {
                        for (unsigned int i = 0; i < std::min(maxColour(e.u), maxColour(e.v));
                             i++)
                        {
                            if (_hasVertexConstraints && (_vertexConstraints[e.u][i] || _vertexConstraints[e.v][i]))
                                continue;

                            GRBLinExpr InCut = vertexVars[e.u][i] + vertexVars[e.v][i];
                            model.addConstr(InCut <= 1, "must be cut by preconstraint: " +
                                            std::to_string(e.u) + ", " +
                                            std::to_string(e.v));
                        }
                    }
                    else if (_edgeConstraints[_graph.edgeId(e.u, e.v)] ==
                        EdgeConstraint::OUT_CUT)
                    {
                        _offSetByConstraints += e.weight;

                        for (unsigned int i = 0; i < std::min(maxColour(e.u), maxColour(e.v));
                             i++)
                        {
                            if (_hasVertexConstraints && (_vertexConstraints[e.u][i] || _vertexConstraints[e.v][i]))
                                continue;

                            GRBLinExpr notCut = vertexVars[e.u][i] - vertexVars[e.v][i];
                            model.addConstr(notCut == 0, "must be cut by preconstraint: " +
                                            std::to_string(e.u) + ", " +
                                            std::to_string(e.v));
                        }
                    }
                }
                else
                {
                    j = _graph.edgeId(e.u, e.v);

                    edgeVars[j] = model.addVar(0.0, 1.0, e.weight, GRB_BINARY,
                                               "edge_" + std::to_string(e.u) + "_" +
                                               std::to_string(e.v));
                    edgeVars[j].set(GRB_DoubleAttr_Start, static_cast<double>(warmStart.assignmentOf(e.u) == warmStart.assignmentOf(e.v)));

                    // Constraints that ensure that x_uv = 1 iff u and v have different
                    // colours
                    for (unsigned int i = 0; i < std::min(maxColour(e.u), maxColour(e.v));
                         i++)
                    {
                        if (_hasVertexConstraints && (_vertexConstraints[e.u][i] || _vertexConstraints[e.v][i]))
                            continue;

                        GRBLinExpr onlyInCut =
                            vertexVars[e.u][i] + vertexVars[e.v][i] - edgeVars[j];
                        model.addConstr(onlyInCut <= 1, "same_endpoints_includes_edge_" + std::to_string(e.u) +
                                        "_" + std::to_string(e.v) + "_" +
                                        std::to_string(i));
                        GRBLinExpr cutMustIn1 =
                            vertexVars[e.u][i] - vertexVars[e.v][i] + edgeVars[j];
                        GRBLinExpr cutMustIn2 =
                            vertexVars[e.v][i] - vertexVars[e.u][i] + edgeVars[j];
                        model.addConstr(cutMustIn1 <= 1,
                                        "different_endpoints_excludes_edge_" + std::to_string(e.u) + "_" +
                                        std::to_string(e.v) + "_" + std::to_string(i));
                        model.addConstr(cutMustIn2 <= 1,
                                        "different_endpoints_excludes_edge_" + std::to_string(e.v) + "_" +
                                        std::to_string(e.u) + "_" + std::to_string(i));
                    }
                }
            }

            // TODO make this a flag
            model.set(GRB_IntParam_OutputFlag, 0);
            model.set(GRB_DoubleParam_TimeLimit, _timeLimit);
            // model.set(GRB_IntParam_Threads, 8);
            model.set(GRB_IntParam_Seed, _seed);

            if (_isMinValSufficient)
                model.set(GRB_DoubleParam_Cutoff, (totalEdgeWeight - _minValSufficient - _offSetByConstraints) * (1.0 + epsilon));

            HeuristicCallback heuristicCallback(_graph, _k, vertexVars, edgeVars, _edgeConstraints, _vertexConstraints,
                                                _config);
            if (_isMinValRequired)
            {
                heuristicCallback.setMinValueRequired(totalEdgeWeight - _minValRequired - _offSetByConstraints);
            }

            model.setCallback(&heuristicCallback);

            // Make sure model realized that variables etc. were added
            model.update();
            model.optimize();

            int status = model.get(GRB_IntAttr_Status);

            // if (status == GRB_INFEASIBLE)
            // {
            //     model.computeIIS();
            //
            //     for (int c = 0; c < model.get(GRB_IntAttr_NumConstrs); ++c)
            //     {
            //         if (model.getConstr(c).get(GRB_IntAttr_IISConstr))
            //         {
            //             std::cout << "Contributes to infeasibility: " << model.getConstr(c).get(GRB_StringAttr_ConstrName) << "\n";
            //         }
            //     }
            //
            // }

            bool acceptableSolution = status != GRB_INFEASIBLE && (status == GRB_OPTIMAL || (_isMinValSufficient && model.get(GRB_DoubleAttr_ObjVal) * (1.0 - epsilon) <= totalEdgeWeight - _minValSufficient - _offSetByConstraints));

            if (acceptableSolution)
            {
                std::cout << "Acceptable solution found\n";
                if (model.get(GRB_IntAttr_SolCount) > 0)
                {
                    _solution = Partition(n);

                    for (NetworKit::node v = 0; v < n; v++)
                    {
                        // Find lowest index that is 1 (corresponds to partition of vertex)
                        unsigned int i;
                        for (i = 0; (_hasVertexConstraints && _vertexConstraints[v][i]) || std::round(vertexVars[v][i].get(GRB_DoubleAttr_X)) == 0;
                             i++)
                        {
                        }

                        _solution.assignElement(v, i);
                    }

                    _foundOptimum = true;

                    _objectiveValue = totalEdgeWeight - model.get(GRB_DoubleAttr_ObjVal) - _offSetByConstraints;

                    _lowerBound = _objectiveValue;
                    _upperBound = _objectiveValue;

                    std::cout << "Acceptable solution found, value = " << _objectiveValue << std::endl;
                }
                else
                {
                    throw std::runtime_error("No solution found despite claiming to have found an acceptable solution.");
                }
            }
            else if (status != GRB_INFEASIBLE && (status == GRB_TIME_LIMIT || (_isMinValRequired && model.get(GRB_DoubleAttr_ObjBound) > totalEdgeWeight - _minValRequired - _offSetByConstraints)))
            {
                if (_isMinValRequired)
                {
                    std::cout << " Could not find acceptable solution in required time, dual bound " << totalEdgeWeight - model.get(GRB_DoubleAttr_ObjBound) - _offSetByConstraints << " required " << _minValRequired << "\n";
                }

                _upperBound = totalEdgeWeight - model.get(GRB_DoubleAttr_ObjBound) - _offSetByConstraints;
                _lowerBound = totalEdgeWeight - model.get(GRB_DoubleAttr_ObjVal) - _offSetByConstraints;
                std::cout << "That took some time, lower bound = " << _lowerBound
                    << ", upper bound = " << _upperBound << std::endl;
            }
            else if (status == GRB_INTERRUPTED || status == GRB_CUTOFF || status == GRB_INFEASIBLE)
            {
                std::cout << "Stopped optimization as we could prove no satisfactory solution exists\n";
            }
            else
            {
                std::cout << "Optimization was stopped with status = " << status
                    << std::endl;
                throw std::runtime_error("Something failed when optimizing.");
            }
        // }
        // catch (const GRBException &e)
        // {
        //     std::cout << "Error number: " << e.getErrorCode() << std::endl;
        //     std::cout << e.getMessage() << std::endl;
        // }
    }
} // namespace mkcp
