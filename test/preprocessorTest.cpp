#include "globals.hpp"
#include "networkit/Globals.hpp"
#include "networkit/components/BiconnectedComponents.hpp"
#include "networkit/components/ConnectedComponents.hpp"
#include "networkit/generators/ErdosRenyiGenerator.hpp"
#include "networkit/graph/Graph.hpp"
#include "preprocessing/maxKCutPreprocessor.hpp"
#include "solver/max_k_cut_solver_gurobi.hpp"
#include "gtest/gtest.h"
#include <cstdlib>

TEST(PreprocessorTest, connectedComponentTest) {
  unsigned int k = 3;
  NetworKit::Graph g(12, true);

  g.addEdge(0, 1, 3.0);
  g.addEdge(0, 2, 2.0);
  g.addEdge(0, 7, 1.0);
  g.addEdge(1, 7, 5.0);
  g.addEdge(1, 8, 3.0);
  g.addEdge(1, 9, 2.0);
  g.addEdge(2, 8, 2.0);
  g.addEdge(2, 9, 4.0);
  g.addEdge(7, 8, 3.0);
  g.addEdge(7, 9, 1.0);
  g.addEdge(3, 4, 3.0);
  g.addEdge(3, 5, 7.0);
  g.addEdge(3, 6, 3.0);
  g.addEdge(4, 5, 5.0);
  g.addEdge(4, 6, 3.0);
  g.addEdge(5, 6, 1.0);

  g.addEdge(9, 10, 1.0);
  g.addEdge(9, 11, 1.0);
  g.addEdge(10, 11, -1.0);

  g.indexEdges();
  g.setMaintainCompactEdges();

  mkcp::DataReducer preprocessor(g, k);
  preprocessor.run();
  assert(preprocessor.hasRun());

  std::vector<mkcp::Partition> kernelSolutions;
  for (NetworKit::Graph kernel : preprocessor.getGraphKernel()) {
    std::cout << "\n\nKernel graph with " << kernel.numberOfNodes()
              << " nodes and " << kernel.numberOfEdges()
              << " edges is being handed to the solver\n"
              << std::endl;

    NetworKit::ConnectedComponents components(kernel);
    components.run();
    EXPECT_EQ(components.numberOfComponents(), 1);

    mkcp::MaxKCutSolverGurobi kernelSolver(kernel, k, 30);
    kernelSolver.run();
    kernelSolutions.emplace_back(kernelSolver.getSolution());
  }

  preprocessor.constructFullSolution(kernelSolutions);

  NetworKit::edgeweight objValuePreprocessor =
      mkcp::computeKCutValue(g, preprocessor.getSolution());

  mkcp::MaxKCutSolverGurobi solver(g, k, 30);
  solver.run();

  NetworKit::edgeweight objValueILP =
      mkcp::computeKCutValue(g, solver.getSolution());

  EXPECT_EQ(objValuePreprocessor, objValueILP);
}

TEST(PreprocessorTest, biconnectedComponentTest) {
  unsigned int k = 3;
  NetworKit::Graph g(10, true);

  g.addEdge(0, 1, 1.0);
  g.addEdge(0, 2, 2.0);
  g.addEdge(0, 3, 3.0);
  g.addEdge(0, 4, 2.0);
  g.addEdge(1, 2, 5.0);
  g.addEdge(1, 3, 1.0);
  g.addEdge(1, 4, 3.0);
  g.addEdge(2, 3, 7.0);
  g.addEdge(2, 4, 3.0);
  g.addEdge(3, 4, 1.0);

  g.addEdge(4, 5, 2.0);
  g.addEdge(4, 6, 8.0);

  g.addEdge(5, 6, 4.0);
  g.addEdge(5, 7, 2.0);
  g.addEdge(5, 8, 3.0);
  g.addEdge(5, 9, 8.0);
  g.addEdge(6, 7, 5.0);
  g.addEdge(6, 8, 1.0);
  g.addEdge(6, 9, 2.0);
  g.addEdge(7, 8, 2.0);
  g.addEdge(7, 9, 4.0);
  g.addEdge(8, 9, 1.0);

  g.indexEdges();
  g.setMaintainCompactEdges();

  mkcp::DataReducer preprocessor(g, k);
  preprocessor.run();
  assert(preprocessor.hasRun());

  std::vector<mkcp::Partition> kernelSolutions;
  for (NetworKit::Graph kernel : preprocessor.getGraphKernel()) {
    std::cout << "\n\nKernel graph with " << kernel.numberOfNodes()
              << " nodes and " << kernel.numberOfEdges()
              << " edges is being handed to the solver\n"
              << std::endl;

    NetworKit::BiconnectedComponents components(kernel);
    components.run();
    EXPECT_EQ(components.numberOfComponents(), 1);

    mkcp::MaxKCutSolverGurobi kernelSolver(kernel, k, 30);
    kernelSolver.run();
    kernelSolutions.emplace_back(kernelSolver.getSolution());

    std::cout << "\n\n\n";
  }

  preprocessor.constructFullSolution(kernelSolutions);

  std::cout << "\n\n\n";

  NetworKit::edgeweight objValuePreprocessor =
      mkcp::computeKCutValue(g, preprocessor.getSolution());

  mkcp::MaxKCutSolverGurobi solver(g, k, 30);
  solver.run();

  NetworKit::edgeweight objValueILP =
      mkcp::computeKCutValue(g, solver.getSolution());

  EXPECT_EQ(objValuePreprocessor, objValueILP);
}

TEST(PreprocessorTest, simpleTest) {
  unsigned int n = 50;
  unsigned int k = 5;

  std::srand(1234);

  NetworKit::ErdosRenyiGenerator generator(n, 0.08);
  NetworKit::Graph g = generator.generate();

  NetworKit::Graph g_weighted(n, true);
  for (auto e : g.edgeRange()) {
    if (e.u > e.v) {
      continue;
    }
    if (std::rand() % 10 == 0) {
      g_weighted.addEdge(e.u, e.v, -1.0);
    } else {
      g_weighted.addEdge(e.u, e.v, 1.0);
    }
  }

  g_weighted.indexEdges();
  g_weighted.setMaintainCompactEdges();

  mkcp::MaxKCutSolverGurobi solver(g_weighted, k, 30);
  solver.run();

  mkcp::DataReducer preprocessor(g_weighted, k);
  preprocessor.run();
  assert(preprocessor.hasRun());

  std::vector<mkcp::Partition> kernelSolutions;
  for (NetworKit::Graph kernel : preprocessor.getGraphKernel()) {
    std::cout << "\n\nKernel graph with " << kernel.numberOfNodes()
              << " nodes and " << kernel.numberOfEdges()
              << " edges is being handed to the solver\n"
              << std::endl;

    kernel.indexEdges();
    kernel.setMaintainCompactEdges();

    mkcp::MaxKCutSolverGurobi kernelSolver(kernel, k, 30);

    kernelSolver.run();
    kernelSolutions.emplace_back(kernelSolver.getSolution());
  }

  preprocessor.constructFullSolution(kernelSolutions);

  EXPECT_EQ(mkcp::computeKCutValue(g_weighted, preprocessor.getSolution()),
            mkcp::computeKCutValue(g_weighted, solver.getSolution()));
}
