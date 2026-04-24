#include "gtest/gtest.h"

#include "globals.hpp"
#include "networkit/generators/ErdosRenyiGenerator.hpp"
#include "solver/constraintGenerator.hpp"
#include "solver/max_k_cut_solver_gurobi.hpp"

TEST(ExtraConstraintTest, simpleTest)
{
    unsigned int n = 40;
    unsigned int k = 6;

    mkcp::Partition partition(6);

    partition.assignElement(2, 1);

    NetworKit::ErdosRenyiGenerator generator(n, 0.25);
    NetworKit::Graph g = generator.generate();

    NetworKit::Graph g_weighted(n, true);
    for (auto e : g.edgeRange())
    {
        if (e.u > e.v)
        {
            continue;
        }
        g_weighted.addEdge(e.u, e.v, 1.0);
    }

    g_weighted.indexEdges();
    g_weighted.setMaintainCompactEdges();

    mkcp::MaxKCutSolverGurobi solver(g_weighted, k, 30);
    solver.run();

    mkcp::DominatingEdges extraConstraints(g_weighted, k);
    extraConstraints.run();

    mkcp::MaxKCutSolverGurobi constrainedSolver(g_weighted, k, 30);
    constrainedSolver.setEdgeConstraints(extraConstraints.guaranteedInCut());
    constrainedSolver.run();

    EXPECT_EQ(solver.getObjectiveValue(), constrainedSolver.getObjectiveValue());
    EXPECT_EQ(
        constrainedSolver.getObjectiveValue(),
        mkcp::computeKCutValue(g_weighted, constrainedSolver.getSolution()));
}
