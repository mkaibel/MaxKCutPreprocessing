//
// Created by michael-kaibel on 22/01/2026.
//

#include "solver/edgeBasedModel.hpp"

#include "solver/separationHeuristics/cliqueFinder.hpp"


namespace mkcp
{
    bool EdgeBasedKCutSolver::hasRun() const { return _hasRun; }

    bool EdgeBasedKCutSolver::foundOptimum() const { return _foundOptimum; }

    double EdgeBasedKCutSolver::getObjectiveValue() const
    {
        return _objectiveValue;
    }

    double EdgeBasedKCutSolver::getLowerBound() const { return _lowerBound; }

    double EdgeBasedKCutSolver::getUpperBound() const { return _upperBound; }

    const mkcp::Partition& EdgeBasedKCutSolver::getSolution() const
    {
        return _solution;
    }

    void EdgeBasedKCutSolver::run()
    {
        _hasRun = true;

        try
        {
            const GRBEnv env;
            GRBModel model(env);

            const unsigned int n = _g.numberOfNodes();

            double totalEdgeWeight = 0.0;

            std::vector<GRBVar> repVars(n);
            std::vector<GRBVar> edgeVars(n * (n - 1) / 2);
            std::vector<GRBVar> repLinVars(n * (n - 1) / 2);

            GRBLinExpr repConstraint = 0;

            for (NetworKit::node v = 0; v < n; v++)
            {
                repVars[v] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
                repConstraint += repVars[v];

                for (NetworKit::node w = v + 1; w < n; w++)
                {
                    // TODO Performance (maybe) make reading out edge weights more efficient
                    double edgeWeight = _g.weight(v, w);
                    totalEdgeWeight += edgeWeight;

                    edgeVars[edgeID(v, w)] = model.addVar(0.0, 1.0, edgeWeight, GRB_BINARY);
                    repLinVars[edgeID(v, w)] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);

                    model.addConstr(repLinVars[edgeID(v, w)] <= edgeVars[edgeID(v, w)],
                                    "Linearisation constraint 1" + std::to_string(v) + "," + std::to_string(w));
                    model.addConstr(repLinVars[edgeID(v, w)] <= repVars[v],
                                    "Linearisation constraint 1" + std::to_string(v) + "," + std::to_string(w));
                    model.addConstr(repLinVars[edgeID(v, w)] >= edgeVars[edgeID(v, w)] + repVars[v] - 1.0,
                                    "Linearisation constraint 3" + std::to_string(v) + "," + std::to_string(w));
                }

                GRBLinExpr linCon4 = 1.0;

                for (NetworKit::node w = 0; w < v; w++)
                {
                    linCon4 -= repLinVars[edgeID(w, v)];
                }

                model.addConstr(repVars[v] == linCon4, "Linearisation constraint 4");
            }

            model.addConstr(repConstraint <= _k, "K representatives constraint");

            for (NetworKit::node u = 0; u < n; u++)
            {
                for (NetworKit::node v = u + 1; v < n; v++)
                {
                    for (NetworKit::node w = v + 1; w < n; w++)
                    {
                        model.addConstr(edgeVars[edgeID(u, v)] >= edgeVars[edgeID(u, w)] + edgeVars[edgeID(v, w)] - 1.0,
                                        "Triangle inequality");
                        model.addConstr(edgeVars[edgeID(u, w)] >= edgeVars[edgeID(u, v)] + edgeVars[edgeID(v, w)] - 1.0,
                                        "Triangle inequality");
                        model.addConstr(edgeVars[edgeID(v, w)] >= edgeVars[edgeID(u, v)] + edgeVars[edgeID(u, w)] - 1.0,
                                        "Triangle inequality");
                    }
                }
            }

            // TODO Implement extra edge constraints

            // model.set(GRB_IntParam_OutputFlag, 0);
            model.set(GRB_DoubleParam_TimeLimit, _timeLimit);
            // model.set(GRB_IntParam_Threads, 8);
            model.set(GRB_IntParam_Seed, _seed);

            model.update();

            CuttingPlaneAdder cuttingPlaneAdder(_g.numberOfNodes(), _k, edgeVars);
            model.setCallback(&cuttingPlaneAdder);

            model.optimize();

            int status = model.get(GRB_IntAttr_Status);

            if (status == GRB_OPTIMAL)
            {
                if (model.get(GRB_IntAttr_SolCount) > 0)
                {
                    _solution = Partition(n);

                    // TODO read out solution

                    _foundOptimum = true;

                    std::cout << "Solution found has objective value " << model.get(GRB_DoubleAttr_ObjVal) <<
                        " while the total edge weight is " << totalEdgeWeight << "\n";

                    _objectiveValue = totalEdgeWeight - model.get(GRB_DoubleAttr_ObjVal);
                    _objectiveValue += _offSetByConstraints;

                    _lowerBound = _objectiveValue;
                    _upperBound = _objectiveValue;
                }
                else
                {
                    std::cout << "No feasible integer solutions found" << std::endl;
                }
            }
            else if (status == GRB_TIME_LIMIT)
            {
                _upperBound = totalEdgeWeight - model.get(GRB_DoubleAttr_ObjBound) + _offSetByConstraints;
                _lowerBound = totalEdgeWeight - model.get(GRB_DoubleAttr_ObjVal) + _offSetByConstraints;
                std::cout << "That took some time, lower bound = " << _lowerBound
                    << ", upper bound = " << _upperBound << std::endl;
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

    void EdgeBasedKCutSolver::setExtraConstraints(
        const std::vector<EdgeConstraint>& extraConstraints)
    {
        _extraConstraints = extraConstraints;
        _hasExtraConstraints = true;

        for (unsigned int eid = 0; eid < _g.upperEdgeIdBound(); eid++)
        {
            if (_extraConstraints[eid])
                _numConstrainedEdges++;
        }
    }

    unsigned long EdgeBasedKCutSolver::edgeID(NetworKit::node v, NetworKit::node w) const
    {
        assert(v != w);
        if (v > w)
        {
            std::swap(v, w);
        }

        return _g.numberOfNodes() * v - ((v * (v + 1)) >> 1) + w - v - 1;
    }

    unsigned long CuttingPlaneAdder::edgeID(NetworKit::node v, NetworKit::node w) const
    {
        assert(v != w);
        if (v > w)
        {
            std::swap(v, w);
        }

        return _n * v - ((v * (v + 1)) >> 1) + w - v - 1;
    }

    void CuttingPlaneAdder::callback()
    {
        try
        {
            if (where == GRB_CB_MIPNODE && getIntInfo(GRB_CB_MIPNODE_STATUS) == GRB_OPTIMAL)
            {
		std::cout << "Starting cutting plane finding\n";
                NetworKit::Graph g(_n, true);

                for (NetworKit::node v = 0; v < _n; v++)
                {
                    for (NetworKit::node w = v + 1; w < _n; w++)
                    {
                        g.addEdge(v, w, getNodeRel(_edgeVars[edgeID(v, w)]));
                    }
                }

                CliqueFinder cliqueFinder(g, _k);
                cliqueFinder.run();

		std::cout << "Adding " << cliqueFinder.getCliques().size() << " clique constraints\n";

                for (const std::vector<NetworKit::node> &clique : cliqueFinder.getCliques())
                {
                    GRBLinExpr cliqueConstraint;
                    for (unsigned i = 0; i < clique.size(); i++)
                    {
                        for (unsigned j = i + 1; j < clique.size(); j++)
                        {
                            cliqueConstraint += _edgeVars[edgeID(clique[i],clique[j])];
                        }
                    }

                    const unsigned p = clique.size() % _k;
                    const unsigned q = clique.size() / _k;

                    addCut(cliqueConstraint, GRB_GREATER_EQUAL, static_cast<double>(q * (q+1) / 2 * p + q * (q - 1) / 2 * (_k - p)));
                }

		std::cout << "Finished cutting plane finding\n";
            }
        }
        catch (GRBException e)
        {
            std::cout << "Error number: " << e.getErrorCode() << std::endl;
            std::cout << e.getMessage() << std::endl;
        }
        catch (...)
        {
            std::cout << "Error during callback" << std::endl;
        }
    }
}
