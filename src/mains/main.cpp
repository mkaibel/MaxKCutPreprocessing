#include <cassert>

#include "globals.hpp"
#include "networkit/generators/ErdosRenyiGenerator.hpp"
#include "networkit/graph/Graph.hpp"
#include "networkit/io/EdgeListReader.hpp"

#include "preprocessing/maxKCutPreprocessor.hpp"
#include "solver/constraintGenerator.hpp"
#include "solver/maxKCutSolver.hpp"
#include "solver/max_k_cut_solver_gurobi.hpp"

#include "preprocessing/tls_checker.hpp"
#include "solver/edgeBasedModel.hpp"
#include "solver/localSearchHeuristic.hpp"
#include "solver/sparsePairingModel.hpp"

bool checkIntegralityGap(unsigned n, double p, unsigned seed)
{
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dis(0.0, 1.0);

    NetworKit::Graph g(2 * n);

    std::vector<NetworKit::node> leftSide(n);

    for (NetworKit::node v = 0; v < n; ++v)
    {
        leftSide[v] = v;
        for (NetworKit::node w = 0; w < n; ++w)
        {
            if (dis(gen) < p)
            {
                g.addEdge(v, n + w);
            }
        }
    }

    mkcp::TLSChecker tlsChecker(g, leftSide);
    tlsChecker.run(GRB_CONTINUOUS);

    mkcp::TLSChecker tlsCheckerInt(g, leftSide);
    tlsCheckerInt.run();

    if (tlsChecker.getObjectiveValue() >= tlsCheckerInt.getObjectiveValue() + 4.0)
    {
        std::cout << "----------\n\nStrong Integrality Gap Detected\n\n----------" << std::endl;
        for (NetworKit::Edge e : g.edgeRange())
        {
            std::cout << e.u << ", " << e.v - n << "\n";
        }
        std::cout << "\n\n";
        return true;
    }

    return false;
}

int main()
{
    // unsigned seed = 0;
    // unsigned tries = 100;
    // double p = 0.8;
    // unsigned n = 15;
    //
    // std::mt19937 gen(seed);
    // std::uniform_int_distribution<unsigned> dis(0);
    //
    // bool notIntegral = false;
    //
    // for (unsigned i = 0; i < tries; ++i)
    // {
    //   notIntegral |= checkIntegralityGap(n, p, dis(gen));
    // }
    //
    // std::cout << "----------\n\nNot integral: " << notIntegral << std::endl;

    // NetworKit::Graph g(8);
    //
    // g.addEdge(0, 4);
    // g.addEdge(0, 5);
    // g.addEdge(1, 4);
    // g.addEdge(1, 5);
    // g.addEdge(2, 6);
    // g.addEdge(2, 7);
    // g.addEdge(3, 6);
    // g.addEdge(3, 7);
    //
    // std::vector<NetworKit::node> leftSide = {0, 1, 2, 3};
    //
    // mkcp::TLSChecker tlsChecker(g, leftSide);
    //
    // tlsChecker.run(GRB_CONTINUOUS);
    //
    // std::cout << "tlsChecker found optimum " << tlsChecker.getObjectiveValue() << std::endl;
    //
    // mkcp::TLSChecker tlsCheckerInt(g, leftSide);
    //
    // tlsCheckerInt.run();
    //
    // std::cout << "tlsCheckerInt found optimum " << tlsChecker.getObjectiveValue() << std::endl;

    NetworKit::Graph g_weighted = NetworKit::EdgeListReader().read("instances/instances_clean/CELAR_01.snap");
    g_weighted.indexEdges();
    g_weighted.setMaintainCompactEdges();

    std::cout << "Number of edges: " << g_weighted.numberOfEdges() << std::endl;

    unsigned int n = 6;
    unsigned int k = 3;

    mkcp::MaxKCutSolver solver(g_weighted, k, mkcp::NO, false, 60);
    solver.run();

    std::cout << "Solver found optimum: " << solver.foundOptimum() << "\nLower Bound: " << solver.getLowerBound() << "\nUpper Bound: " << solver.getUpperBound() << "\n";
}
