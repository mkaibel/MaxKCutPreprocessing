//
// Created by michael-kaibel on 29/01/2026.
//

#include "solver/sparsePairingModel.hpp"

#include "networkit/components/ConnectedComponents.hpp"
#include "networkit/distance/Dijkstra.hpp"
#include "ogdf/fileformats/GML.h"
#include "solver/separationHeuristics/cliqueFinder.hpp"

namespace mkcp
{
    bool SparsePairingModel::hasRun() const { return _hasRun; }

    bool SparsePairingModel::foundOptimum() const { return _foundOptimum; }

    double SparsePairingModel::getObjectiveValue() const
    {
        return _objectiveValue;
    }

    double SparsePairingModel::getLowerBound() const { return _lowerBound; }

    double SparsePairingModel::getUpperBound() const { return _upperBound; }

    const mkcp::Partition& SparsePairingModel::getSolution() const
    {
        return _solution;
    }

    void SparsePairingModel::run()
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
            std::vector<bool> trueEdge(n * (n - 1) / 2, false);

            GRBLinExpr repConstraint = 0;

            for (NetworKit::node v = 0; v < n; v++)
            {
                repVars[v] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
                repConstraint += repVars[v];
            }

            model.addConstr(repConstraint <= _k, "<= K representatives constraint");

            std::vector<NetworKit::edgeweight> tempWeights(_g.numberOfNodes(), 0.0);

            for (NetworKit::node v = 0; v < n; v++)
            {
                for (auto [w, weight] : _g.weightNeighborRange(v))
                {
                    tempWeights[w] = weight;
                }

                GRBLinExpr largeMustLeadConstraint = repVars[v];

                GRBLinExpr NoMultiDeferral;

                for (NetworKit::node w = v + 1; w < n; w++)
                {
                    // TODO Performance (maybe) make reading out edge weights more efficient
                    // TODO cover case with extra constraints
                    edgeVars[edgeID(v, w)] = model.addVar(0.0, 1.0, tempWeights[w], GRB_BINARY);

                    if (tempWeights[w] == 0.0)
                    {
                        model.addConstr(edgeVars[edgeID(v, w)] <= repVars[w], "Leadership deferral constraint");
                        NoMultiDeferral += edgeVars[edgeID(v, w)];
                    }
                    else
                    {
                        trueEdge[edgeID(v, w)] = true;
                        totalEdgeWeight += tempWeights[w];
                    }
                    model.addConstr(repVars[v] + edgeVars[edgeID(v, w)] <= 1, "Only largest leader constraint");
                    largeMustLeadConstraint += edgeVars[edgeID(v, w)];
                }

                model.addConstr(largeMustLeadConstraint >= 1, "Large must lead constraint");

                for (NetworKit::node w : _g.neighborRange(v))
                {
                    tempWeights[w] = 0.0;
                }

                for (NetworKit::node w = 0; w < n; w++)
                {
                    assert(tempWeights[w] == 0.0);
                }
            }

            unsigned numtriangles = 0;
            std::vector<bool> vNeighbours(n, false);
            for (NetworKit::node v = 0; v < n; v++)
            {
                for (NetworKit::node w : _g.neighborRange(v))
                {
                    vNeighbours[w] = true;
                }

                for (NetworKit::node w : _g.neighborRange(v))
                {
                    if (w < v)
                        continue;

                    for (NetworKit::node x : _g.neighborRange(w))
                    {
                        if (x < w || !vNeighbours[x])
                            continue;

                        model.addConstr(edgeVars[edgeID(x, v)] >= edgeVars[edgeID(x, w)] + edgeVars[edgeID(v, w)] - 1.0,
                                        "Triangle inequality");
                        model.addConstr(edgeVars[edgeID(x, w)] >= edgeVars[edgeID(x, v)] + edgeVars[edgeID(v, w)] - 1.0,
                                        "Triangle inequality");
                        model.addConstr(edgeVars[edgeID(v, w)] >= edgeVars[edgeID(x, v)] + edgeVars[edgeID(x, w)] - 1.0,
                                        "Triangle inequality");
                        numtriangles += 3;
                    }
                }

                for (NetworKit::node w : _g.neighborRange(v))
                {
                    vNeighbours[w] = true;
                }
            }

            std::cout << "Number of triangle inequalities: " << numtriangles << "\n";

            // model.set(GRB_IntParam_OutputFlag, 0);
            model.set(GRB_DoubleParam_TimeLimit, _timeLimit);
            // model.set(GRB_IntParam_Threads, 8);
            model.set(GRB_IntParam_Seed, _seed);
            model.set(GRB_IntParam_LazyConstraints, 1);

            model.update();

            SparsePairingCallback callback(_g, _k, edgeVars, trueEdge, _extraConstraints);
            model.setCallback(&callback);

            model.optimize();

            int status = model.get(GRB_IntAttr_Status);

            if (status == GRB_OPTIMAL)
            {
                if (model.get(GRB_IntAttr_SolCount) > 0)
                {
                    _solution = Partition(n);

                    // TODO read out solution
                    NetworKit::Graph gSolution(n);
                    for (NetworKit::node v = 0; v < n; v++)
                    {
                        for (NetworKit::node w = v + 1; w < n; w++)
                        {
                            if (_g.hasEdge(v, w) && _extraConstraints[_g.edgeId(v, w)] == EdgeConstraint::OUT_CUT)
                            {
                                gSolution.addEdge(v, w);
                                std::cout << "Edge (" << v << ", " << w << ") has value 1\n";
                                continue;
                            }

                            if (std::round(edgeVars[edgeID(v, w)].get(GRB_DoubleAttr_X)) == 1.0)
                            {
                                std::cout << "Edge (" << v << ", " << w << ") has value 1\n";
                                gSolution.addEdge(v, w);
                            }
                        }
                    }

                    NetworKit::ConnectedComponents components(gSolution);
                    components.run();

                    int j = 0;

                    for (const std::vector<NetworKit::node> &component : components.getComponents())
                    {
                        std::cout << "In component " << j << ": ";
                        for (NetworKit::node v : component)
                        {
                            std::cout << v << ", ";
                        }
                        std::cout << "\n";
                    }
                    std::cout << "\nGraph has edges:\n";
                    for (NetworKit::WeightedEdge e : _g.edgeWeightRange())
                    {
                        std::cout << "(" << e.u << ", " << e.v << "), w_e = " << e.weight << "\n";
                    }

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
        catch (GRBException& e)
        {
            std::cout << "Error number: " << e.getErrorCode() << std::endl;
            std::cout << e.getMessage() << std::endl;
        }
        catch (std::runtime_error& e)
        {
            std::cout << "Error during optimization: " << e.what() << std::endl;
        }
    }

    void SparsePairingModel::setExtraConstraints(
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

    unsigned long SparsePairingModel::edgeID(NetworKit::node v, NetworKit::node w) const
    {
        assert(v != w);
        if (v > w)
        {
            std::swap(v, w);
        }

        return _g.numberOfNodes() * v - ((v * (v + 1)) >> 1) + w - v - 1;
    }

    unsigned long SparsePairingCallback::edgeID(NetworKit::node v, NetworKit::node w) const
    {
        assert(v != w);
        if (v > w)
        {
            std::swap(v, w);
        }

        return _n * v - ((v * (v + 1)) >> 1) + w - v - 1;
    }

    void SparsePairingCallback::callback()
    {
        // try
        // {
            if (where == GRB_CB_MIPSOL)
            {
                std::cout << "Adding chordless cycle constraints\n";

                addChordlessCycleConstraints();
            }
            if (where == GRB_CB_MIPNODE && getIntInfo(GRB_CB_MIPNODE_STATUS) == GRB_OPTIMAL)
            {
                std::cout << "Adding chordless cycle constraints\n";

                bool infeasible = addChordlessCycleConstraints();

                if (infeasible)
                    return;

                std::cout << "Starting cutting plane finding\n";
                NetworKit::Graph g(_n, true);

                for (NetworKit::node v = 0; v < _n; v++)
                {
                    for (NetworKit::node w = v + 1; w < _n; w++)
                    {
                        if (_trueEdge[edgeID(v, w)])
                            g.addEdge(v, w, getNodeRel(_edgeVars[edgeID(v, w)]));
                    }
                }

                CliqueFinder cliqueFinder(g, _k);
                cliqueFinder.run();

                std::cout << "Adding " << cliqueFinder.getCliques().size() << " clique constraints\n";

                for (const std::vector<NetworKit::node>& clique : cliqueFinder.getCliques())
                {
                    GRBLinExpr cliqueConstraint;
                    for (unsigned i = 0; i < clique.size(); i++)
                    {
                        for (unsigned j = i + 1; j < clique.size(); j++)
                        {
                            cliqueConstraint += _edgeVars[edgeID(clique[i], clique[j])];
                        }
                    }

                    const unsigned p = clique.size() % _k;
                    const unsigned q = clique.size() / _k;

                    addCut(cliqueConstraint, GRB_GREATER_EQUAL,
                           static_cast<double>(q * (q + 1) / 2 * p + q * (q - 1) / 2 * (_k - p)));
                }

                std::cout << "Finished cutting plane finding\n";
            }
        // }
        // catch (GRBException& e)
        // {
        //     std::cout << "Error number: " << e.getErrorCode() << std::endl;
        //     std::cout << e.getMessage() << std::endl;
        // }
        // catch (std::runtime_error &e)
        // {
        //     std::cout << "Error during callback" << std::endl;
        // }
    }

    bool SparsePairingCallback::addChordlessCycleConstraints()
    {
        unsigned numNewConstraints = 0;

        NetworKit::Graph gx(_n, true);

        std::vector<NetworKit::WeightedEdge> susEdges;

        for (NetworKit::node v = 0; v < _n; v++)
        {
            for (NetworKit::node w = v + 1; w < _n; w++)
            {
                double x_vw = getEdgeVal(v, w);

                if (x_vw > 0.0 || _trueEdge[edgeID(v, w)])
                {
                    // We have to cover the special case that gurobi variables are 1.0 + epsilon
                    gx.addEdge(v, w, std::max(1.0 - x_vw, 0.0));
                }
                if (_trueEdge[edgeID(v, w)] && x_vw < 1.0)
                {
                    susEdges.emplace_back(v, w, 1.0 - x_vw);
                }
            }
        }

        std::vector<NetworKit::count> posInPath(_n, 0);

        for (NetworKit::WeightedEdge e : susEdges)
        {
            NetworKit::count eID = edgeID(e.u, e.v);
            // Edges for which the chordless cycle constraint either doesn't apply or
            if (!_trueEdge[eID] || e.weight == 0.0)
                continue;

            NetworKit::Dijkstra shortestPath(gx, e.u, true, false, e.v);
            shortestPath.run();

            // Check for violeted cycle with e violating
            if (shortestPath.distance(e.v) < e.weight)
            {
                std::cout << "Found edge " << e.u << ", " << e.v << " violating cycle constraint\n";

                std::vector<NetworKit::node> path(1, e.v);
                NetworKit::edgeweight distTotal = shortestPath.distance(e.v);

                NetworKit::node v = e.v;
                NetworKit::count pos = 1;
                while (v != e.u)
                {
                    v = shortestPath.getPredecessors(v).front();
                    posInPath[v] = pos;
                    pos++;
                }

                //Shorten path by chords to obtain chordless path
                while (path.back() != e.u)
                {
                    NetworKit::node farthestNeighbour = NetworKit::none;
                    NetworKit::count maxPos = 0;
                    for (NetworKit::node w : gx.neighborRange(path.back()))
                    {
                        // Only shorten by chords in E
                        if (path.back() == e.v && w == e.u)
                            continue;

                        // Skip chords that are not "original edges"
                        if (!_trueEdge[edgeID(path.back(), w)] && posInPath[w] != (posInPath[path.back()] + 1))
                            continue;

                        if (posInPath[w] > maxPos)
                        {
                            farthestNeighbour = w;
                            maxPos = posInPath[w];
                        }
                    }

                    assert(farthestNeighbour != NetworKit::none);

                    path.emplace_back(farthestNeighbour);
                }

                // Reset path positions
                v = e.v;
                while (v != e.u)
                {
                    v = shortestPath.getPredecessors(v).front();
                    posInPath[v] = 0;
                }

                // Check if chordless path still violates constraint (if not then one of the chords must violate it for a shorter cycle)
                NetworKit::edgeweight chordlessPathWeight = 0.0;
                for (unsigned i = 0; i + 1 < path.size(); i++)
                {
                    chordlessPathWeight += 1.0 - getEdgeVal(path[i], path[i + 1]);
                }

                if (e.weight > chordlessPathWeight)
                {
                    GRBLinExpr chordlessPathConstraint = -1 * static_cast<double>(path.size()) + 2.0;

                    for (unsigned i = 0; i + 1 < path.size(); i++)
                    {
                        chordlessPathConstraint += getEdgeVar(path[i], path[i + 1]);
                    }

                    chordlessPathConstraint -= getEdgeVar(e.u, e.v);

                    assert(where == GRB_CB_MIPSOL || where == GRB_CB_MIPNODE);
                    addLazy(chordlessPathConstraint, GRB_LESS_EQUAL, 0.0);
                    numNewConstraints++;

                    std::cout << "Adding cycle constraint for cycle length " << path.size() + 1 << "\n";
                }
            }

        }

        std::cout << "Adding " << numNewConstraints << " chordless cycle constraints\n";
        return numNewConstraints != 0;
    }

    GRBLinExpr SparsePairingCallback::getEdgeVar(NetworKit::node v, NetworKit::node w) const
    {
        NetworKit::count eidFull = edgeID(v, w);
        if (_trueEdge[eidFull])
        {
            switch (_edgeConstraints[_g.edgeId(v, w)])
            {
            case EdgeConstraint::IN_CUT:
                return 0.0;
            case EdgeConstraint::OUT_CUT:
                return 1.0;
            default:
                return _edgeVars[eidFull];
            }
        }
        return _edgeVars[eidFull];
    }

    double SparsePairingCallback::getEdgeVal(NetworKit::node v, NetworKit::node w)
    {
        NetworKit::count eidFull = edgeID(v, w);
        if (_trueEdge[eidFull])
        {
            switch (_edgeConstraints[_g.edgeId(v, w)])
            {
            case EdgeConstraint::IN_CUT:
                return 1.0;
            case EdgeConstraint::OUT_CUT:
                return 0.0;
            default:
                return stateSpecificEdgeVal(eidFull);
            }
        }
        return stateSpecificEdgeVal(eidFull);
    }

    double SparsePairingCallback::stateSpecificEdgeVal(NetworKit::count eid)
    {
        if (where == GRB_CB_MIPSOL)
        {
            return getSolution(_edgeVars[eid]);
        }
        if (where == GRB_CB_MIPNODE)
        {
            return getNodeRel(_edgeVars[eid]);
        }
        throw std::runtime_error("Accessing variable value outside of MIPSOL and MIPNODE");
    }
}
