#ifndef MKCP_MAXKCUTSOLVER_HPP
#define MKCP_MAXKCUTSOLVER_HPP
#include "globals.hpp"
#include "max_k_cut_solver_gurobi.hpp"
#include "networkit/graph/Graph.hpp"
#include "preprocessing/maxKCutPreprocessor.hpp"

namespace mkcp {
class MaxKCutSolver {
public:
  /**
   * Solver framework for MaxKCut wrapping around a gurobi based solver and a preprocessing framework
   * @param g the graph in question, which must have edges indexed and setMaintainCompactEdges()
   * @param k
   * @param usePreprocessor choice of NO, FULL and NAIVE, FULL can still be configured optionally
   * @param useConstraintGenerator should extra constraints be generated
   * @param timeLimit time limit for solving, may go over by a few seconds
   * @param seed the seed
   * @param cuttingPlaneConfig config for which cutting planes should be separated
   * @param preprocessorConfig config for which preprocessing rules should be used, only relevant if preprocessor setting is FULL
   */
  MaxKCutSolver(const NetworKit::Graph &g, unsigned int k, PreprocessorSetting usePreprocessor,
                bool useConstraintGenerator, double timeLimit = 300,
                unsigned seed = 1234, CuttingPlaneConfig cuttingPlaneConfig = {true, true, true, true}, DataReducerConfig preprocessorConfig = {true, true, true, true, true})
      : _graph(g), _k(k), _seed(seed), useConstraintGenerator(useConstraintGenerator),
        usePreprocessor(usePreprocessor),
        _cuttingPlaneConfig(cuttingPlaneConfig), _dataReducerConfig(preprocessorConfig), _solution(g.upperNodeIdBound()), _timeLimit(timeLimit) {}

  void run();

  [[nodiscard]] bool hasRun() const;

  [[nodiscard]] bool foundOptimum() const;

  [[nodiscard]] double getObjectiveValue() const;

  [[nodiscard]] double getLowerBound() const;

  [[nodiscard]] double getUpperBound() const;

  [[nodiscard]] const Partition &getSolution() const;

private:
  const NetworKit::Graph &_graph;

  unsigned int _k;

  unsigned _seed;

  bool useConstraintGenerator;

  PreprocessorSetting usePreprocessor;

  CuttingPlaneConfig _cuttingPlaneConfig;
  DataReducerConfig _dataReducerConfig;

  Partition _solution;
  bool _hasRun = false;
  double _objectiveValue = 0.0, _lowerBound = 0.0, _upperBound = 0.0;
  double _timeLimit;
  bool _foundOptimum = false;
};
} // namespace mkcp

#endif // MKCP_MAXKCUTSOLVER_HPP
