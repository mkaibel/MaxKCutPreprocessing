//
// Created by michael-kaibel on 15/01/2026.
//

#include "preprocessing/tls_checker.hpp"

#include <gurobi_c++.h>

#include "ogdf/fileformats/DotParser.h"
#include "util/bipartiteMatching.hpp"

namespace mkcp
{
    TLSChecker::TLSChecker(NetworKit::Graph& g, const std::vector<NetworKit::node>& leftSide) : _g(g), _leftSide(leftSide)
    {
    }

    double TLSChecker::getObjectiveValue() const
    {
        if (!_hasRun)
        {
            throw std::runtime_error("Run algorithm before ");
        }

        return _objectiveValue;
    }

    bool TLSChecker::hasRun() const
    {
        return _hasRun;
    }

    void TLSChecker::run(char vType)
    {
        _hasRun = true;

        assert(vType == GRB_BINARY || vType == GRB_CONTINUOUS);

        std::vector<char> onSide(_g.upperNodeIdBound(), 2);
        std::vector<NetworKit::node> IDonSide(_g.upperNodeIdBound(), NetworKit::none);

        uint i = 0;
        for (const NetworKit::node v : _leftSide)
        {
            onSide[v] = LEFT;
            IDonSide[v] = i;
            i++;
        }

        std::vector<NetworKit::node> _rightSide;

        i = 0;
        for (const NetworKit::node v : _g.nodeRange())
        {
            if (onSide[v] != LEFT)
            {
                onSide[v] = RIGHT;
                _rightSide.emplace_back(v);
                IDonSide[v] = i;
                i++;
            }
        }

#ifdef DEBUG
        // Ensure graph is bipartite with the given bipartition
        for (const NetworKit::Edge e : _g.edgeRange())
        {
            assert(onSide[e.u] != onSide[e.v]);
        }

        for (const NetworKit::node v : _g.nodeRange())
        {
            assert(onSide[v] == LEFT || onSide[v] == RIGHT);

            if (onSide[v] == LEFT)
            {
                assert(_leftSide[IDonSide[v]] == v);
            }
            else
            {
                assert(_rightSide[IDonSide[v]] == v);
            }
        }
#endif

        _g.indexEdges();
        _g.setMaintainCompactEdges();

        BipartiteMatching matching(_g, _leftSide);
        matching.run();
        double matchingValue = matching.getMatching().size();


        try
        {
            GRBEnv env;
            GRBModel model(env);

            std::vector<std::vector<std::vector<GRBVar>>> contractionVars(2);
            contractionVars[LEFT].resize(_leftSide.size() - 1);
            contractionVars[RIGHT].resize(_rightSide.size() - 1);

            // Add contraction variables and constraints
            for (NetworKit::node v = 0; v < _leftSide.size() - 1; v++)
            {
                contractionVars[LEFT][v].resize(_leftSide.size() - v - 1);
                for (NetworKit::node w = v + 1; w < _leftSide.size(); w++)
                {
                    contractionVars[LEFT][v][w - v - 1] = model.addVar(0.0, 1.0, -1.0, vType,
                                                                       "Y^L_" + std::to_string(v) + std::to_string(w));

                    // Add constraint that w can't be contracted into v if v was contracted into u
                    GRBLinExpr contractionConstraint = contractionVars[LEFT][v][w - v - 1];

                    for (NetworKit::node u = 0; u < v; u++)
                    {
                        contractionConstraint += contractionVars[LEFT][u][v - u - 1];
                    }

                    model.addConstr(contractionConstraint <= 1,
                                    "contractionConstraint^L_" + std::to_string(v) + "_" + std::to_string(w));
                }
            }

            for (NetworKit::node v = 0; v < _rightSide.size() - 1; v++)
            {
                contractionVars[RIGHT][v].resize(_rightSide.size() - v - 1);
                for (NetworKit::node w = v + 1; w < _rightSide.size(); w++)
                {
                    contractionVars[RIGHT][v][w - v - 1] = model.addVar(0.0, 1.0, -1.0, vType,
                                                                        "Y^R_" + std::to_string(v) + std::to_string(w));

                    // Add constraint that w can't be contracted into v if v was contracted into u
                    GRBLinExpr contractionConstraint = contractionVars[RIGHT][v][w - v - 1];

                    for (NetworKit::node u = 0; u < v; u++)
                    {
                        contractionConstraint += contractionVars[RIGHT][u][v - u - 1];
                    }

                    model.addConstr(contractionConstraint <= 1,
                                    "contractionConstraint^R_" + std::to_string(v) + "_" + std::to_string(w));
                }
            }

            std::vector<GRBVar> matchingVariables;
            std::vector<std::vector<GRBVar>> coverVariables;

            coverVariables.resize(2);
            coverVariables[LEFT].reserve(_leftSide.size());
            coverVariables[RIGHT].reserve(_rightSide.size());

            matchingVariables.resize(_g.numberOfEdges());

            for (NetworKit::node v = 0; v < _leftSide.size(); v++)
            {
                coverVariables[LEFT].emplace_back(model.addVar(0.0, 1.0, 0.0, vType, "Z^L_" + std::to_string(v)));
            }
            for (NetworKit::node v = 0; v < _rightSide.size(); v++)
            {
                coverVariables[RIGHT].emplace_back(model.addVar(0.0, 1.0, 0.0, vType, "Z^R_" + std::to_string(v)));
            }

            for (const NetworKit::Edge e : _g.edgeRange())
            {
                NetworKit::node u = e.u, v = e.v;
                if (onSide[u] != LEFT)
                {
                    std::swap(u, v);
                }
                u = IDonSide[u];
                v = IDonSide[v];

                matchingVariables[_g.edgeId(e.u, e.v)] = model.addVar(0.0, 1.0, -1.0, vType,
                                                                      "x_" + std::to_string(e.u) + "_" + std::to_string(
                                                                          e.v));

                GRBLinExpr edgeBlockConstraint = matchingVariables[_g.edgeId(e.u, e.v)];
                for (NetworKit::node w = v + 1; w < _rightSide.size(); w++)
                {
                    if (!_g.hasEdge(_leftSide[u], _rightSide[w]))
                    {
                        edgeBlockConstraint += contractionVars[RIGHT][v][w - v - 1];
                    }
                }
                for (NetworKit::node w = u + 1; w < _leftSide.size(); w++)
                {
                    if (!_g.hasEdge(_leftSide[w], _rightSide[v]))
                    {
                        edgeBlockConstraint += contractionVars[LEFT][u][w - u - 1];
                    }
                }

                model.addConstr(edgeBlockConstraint <= 1,
                                "EdgeBlockConstraint_" + std::to_string(u) + "_" + std::to_string(v));

                GRBLinExpr edgeCoverConstraint = coverVariables[LEFT][u] + coverVariables[RIGHT][v];
                for (NetworKit::node w = 0; w < v; w++)
                {
                    edgeCoverConstraint += contractionVars[RIGHT][w][v - w - 1];
                }
                for (NetworKit::node w = v + 1; w < _rightSide.size(); w++)
                {
                    if (!_g.hasEdge(_leftSide[u], _rightSide[w]))
                    {
                        edgeCoverConstraint += contractionVars[RIGHT][v][w - v - 1];
                    }
                }
                for (NetworKit::node w = 0; w < u; w++)
                {
                    edgeCoverConstraint += contractionVars[LEFT][w][u - w - 1];
                }
                for (NetworKit::node w = u + 1; w < _leftSide.size(); w++)
                {
                    if (!_g.hasEdge(_leftSide[w], _rightSide[v]))
                    {
                        edgeCoverConstraint += contractionVars[LEFT][u][w - u - 1];
                    }
                }

                model.addConstr(edgeCoverConstraint >= 1,
                                "EdgeCoverConstraint_" + std::to_string(u) + "_" + std::to_string(v));
            }

            for (NetworKit::node v = 0; v < _leftSide.size(); v++)
            {
                GRBLinExpr matchingConstraint = 0;
                for (NetworKit::node w : _g.neighborRange(_leftSide[v]))
                {
                    matchingConstraint += matchingVariables[_g.edgeId(_leftSide[v], w)];
                }
                for (NetworKit::node w = 0; w < v; w++)
                {
                    matchingConstraint += contractionVars[LEFT][w][v - w - 1];
                }

                model.addConstr(matchingConstraint <= 1, "MatchingConstraint^L_" + std::to_string(v));
            }

            for (NetworKit::node v = 0; v < _rightSide.size(); v++)
            {
                GRBLinExpr matchingConstraint = 0;
                for (NetworKit::node w : _g.neighborRange(_rightSide[v]))
                {
                    matchingConstraint += matchingVariables[_g.edgeId(w, _rightSide[v])];
                }
                for (NetworKit::node w = 0; w < v; w++)
                {
                    matchingConstraint += contractionVars[RIGHT][w][v - w - 1];
                }

                model.addConstr(matchingConstraint <= 1, "MatchingConstraint^R_" + std::to_string(v));
            }

            GRBLinExpr dualityConstraint = 0;

            for (NetworKit::Edge e : _g.edgeRange())
            {
                dualityConstraint += matchingVariables[_g.edgeId(e.u, e.v)];
            }

            for (NetworKit::node v = 0; v < _leftSide.size(); v++)
            {
                dualityConstraint -= coverVariables[LEFT][v];
            }
            for (NetworKit::node v = 0; v < _rightSide.size(); v++)
            {
                dualityConstraint -= coverVariables[RIGHT][v];
            }

            model.addConstr(dualityConstraint == 0, "DualityConstraint");

            model.set(GRB_IntAttr_ModelSense, GRB_MAXIMIZE);
            model.set(GRB_IntParam_OutputFlag, 0);
            model.set(GRB_DoubleParam_TimeLimit, 10);

            model.update();

            model.optimize();

            int status = model.get(GRB_IntAttr_Status);

            if (status == GRB_OPTIMAL)
            {
                if (model.get(GRB_IntAttr_SolCount) > 0)
                {
                    std::cout << "Found Optimum with value: " << matchingValue + model.get(GRB_DoubleAttr_ObjVal) <<
                        std::endl;

                    for (NetworKit::node v = 0; v < _leftSide.size(); v++)
                    {
                        for (NetworKit::node w = v + 1; w < _leftSide.size(); w++)
                        {
                            if (contractionVars[LEFT][v][w - v - 1].get(GRB_DoubleAttr_X) > 0.0)
                            {
                                std::cout << "Contracts left " << w << " into " << v << " with value " <<
                                    contractionVars[LEFT][v][w - v - 1].get(GRB_DoubleAttr_X) << std::endl;
                            }
                        }
                    }

                    for (NetworKit::node v = 0; v < _rightSide.size(); v++)
                    {
                        for (NetworKit::node w = v + 1; w < _rightSide.size(); w++)
                        {
                            if (contractionVars[RIGHT][v][w - v - 1].get(GRB_DoubleAttr_X) > 0.0)
                            {
                                std::cout << "Contracts right " << w << " into " << v << " with value " <<
                                    contractionVars[RIGHT][v][w - v - 1].get(GRB_DoubleAttr_X) << std::endl;
                            }
                        }
                    }


                    for (NetworKit::Edge e : _g.edgeRange())
                    {
                        if (matchingVariables[_g.edgeId(e.u, e.v)].get(GRB_DoubleAttr_X) > 0.0)
                        {
                            NetworKit::node u = e.u, v = e.v;
                            if (onSide[u] != LEFT)
                            {
                                std::swap(u, v);
                            }
                            std::cout << "Matched edge {" << IDonSide[u] << ", " << IDonSide[v] << "} with value " <<
                                matchingVariables[_g.edgeId(e.u, e.v)].get(GRB_DoubleAttr_X) << std::endl;
                        }
                    }

                    _objectiveValue = matchingValue + model.get(GRB_DoubleAttr_ObjVal);
                }
            }
            else if (status == GRB_TIME_LIMIT)
            {
                std::cout << "Timeout" << std::endl;
            }
            else
            {
                std::cout << "Optimization was stopped with status = " << status
                    << std::endl;
                throw std::runtime_error("Something failed when optimizing.");
            }
        }
        catch (GRBException e)
        {
            std::cout << "Error number: " << e.getErrorCode() << std::endl;
            std::cout << e.getMessage() << std::endl;
        }
        catch (std::runtime_error& e)
        {
            std::cout << "Error during optimization: " << e.what() << std::endl;
        }
    }
}
