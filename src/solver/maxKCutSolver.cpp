#include "solver/maxKCutSolver.hpp"

#include <chrono>

#include "networkit/graph/Graph.hpp"
#include "preprocessing/maxKCutPreprocessor.hpp"
#include "solver/constraintGenerator.hpp"
#include "solver/max_k_cut_solver_gurobi.hpp"

namespace mkcp {
bool MaxKCutSolver::hasRun() const { return _hasRun; }

bool MaxKCutSolver::foundOptimum() const { return _foundOptimum; }

double MaxKCutSolver::getObjectiveValue() const { return _objectiveValue; }

double MaxKCutSolver::getLowerBound() const { return _lowerBound; }

double MaxKCutSolver::getUpperBound() const { return _upperBound; }

const Partition &MaxKCutSolver::getSolution() const { return _solution; }

void MaxKCutSolver::run() {
  _hasRun = true;

  if (usePreprocessor != PreprocessorSetting::NO) {
    auto t0 = std::chrono::steady_clock::now();

    DataReducer preprocessor(_graph, _k, _seed, _dataReducerConfig);
    bool naive = (usePreprocessor == PreprocessorSetting::NAIVE);
    preprocessor.run(naive);

    bool allKernelsSolved = true;

    std::vector<Partition> kernelSolutions;
    for (NetworKit::Graph kernel : preprocessor.getGraphKernel()) {
      kernel.indexEdges();
      kernel.setMaintainCompactEdges();

      NetworKit::edgeweight veryOptimisticDualBound = 0.0;
      for (NetworKit::WeightedEdge e : kernel.edgeWeightRange())
      {
        if (e.weight > 0.0)
          veryOptimisticDualBound += e.weight;
      }

      if (useConstraintGenerator) {
        DominatingEdges extraConstraints(kernel, _k);
        extraConstraints.run();

        MaxKCutSolverGurobi kernelSolver(
            kernel, _k,
            std::max(0.5,
                     _timeLimit -
                         static_cast<double>(
                             std::chrono::duration_cast<std::chrono::seconds>(
                                 std::chrono::steady_clock::now() - t0)
                                 .count())),
            _seed, _cuttingPlaneConfig);
        kernelSolver.setEdgeConstraints(extraConstraints.guaranteedInCut());

        kernelSolver.run();

        // If the ILP model doesn't have enough time to find any proper solution we get bad bounds, so in that case we use the trivial bounds
        _lowerBound += std::max(kernelSolver.getLowerBound(), 0.0);
        _upperBound += std::min(kernelSolver.getUpperBound(), veryOptimisticDualBound);

        if (kernelSolver.foundSatisfactorySolution()) {
          kernelSolutions.emplace_back(kernelSolver.getSolution());
        } else {
          allKernelsSolved = false;
          continue;
        }
      } else {
        MaxKCutSolverGurobi kernelSolver(
            kernel, _k,
            std::max(0.5,
                     _timeLimit -
                         static_cast<double>(
                             std::chrono::duration_cast<std::chrono::seconds>(
                                 std::chrono::steady_clock::now() - t0)
                                 .count())),
            _seed, _cuttingPlaneConfig);

        kernelSolver.run();

        // If the ILP model doesn't have enough time to find any proper solution we get bad bounds, so in that case we use the trivial bounds
        _lowerBound += std::max(kernelSolver.getLowerBound(), 0.0);
        _upperBound += std::min(kernelSolver.getUpperBound(), veryOptimisticDualBound);

        if (kernelSolver.foundSatisfactorySolution()) {
          kernelSolutions.emplace_back(kernelSolver.getSolution());
        } else {
          allKernelsSolved = false;
          continue;
        }
      }
    }

    _lowerBound += preprocessor.getOffset();
    _upperBound += preprocessor.getOffset();

    if (allKernelsSolved) {
      if (static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(
                                  std::chrono::steady_clock::now() - t0)
                                  .count()) < _timeLimit) {
        preprocessor.constructFullSolution(kernelSolutions);
        _solution = preprocessor.getSolution();

        _objectiveValue = computeKCutValue(_graph, _solution);
        _foundOptimum = true;
      }
    }
  } else {
    if (useConstraintGenerator) {
      auto t0 = std::chrono::steady_clock::now();

      NetworKit::Graph indexedGraph = _graph;

      indexedGraph.indexEdges();
      indexedGraph.setMaintainCompactEdges();

      DominatingEdges extraConstraints(indexedGraph, _k);
      extraConstraints.run();

      MaxKCutSolverGurobi gurobiSolver(
          indexedGraph, _k,
          _timeLimit - static_cast<double>(
                           std::chrono::duration_cast<std::chrono::seconds>(
                               std::chrono::steady_clock::now() - t0)
                               .count()),
          _seed, _cuttingPlaneConfig);

      gurobiSolver.setEdgeConstraints(extraConstraints.guaranteedInCut());

      gurobiSolver.run();

      _lowerBound = gurobiSolver.getLowerBound();
      _upperBound = gurobiSolver.getUpperBound();

      if (gurobiSolver.foundSatisfactorySolution()) {
        _objectiveValue = gurobiSolver.getObjectiveValue();
        _solution = gurobiSolver.getSolution();

        _foundOptimum = true;
      } else {
        _foundOptimum = false;
      }
    } else {
      MaxKCutSolverGurobi gurobiSolver(_graph, _k, _timeLimit, _seed, _cuttingPlaneConfig);

      gurobiSolver.run();

      _lowerBound = gurobiSolver.getLowerBound();
      _upperBound = gurobiSolver.getUpperBound();

      if (gurobiSolver.foundSatisfactorySolution()) {
        _objectiveValue = gurobiSolver.getObjectiveValue();
        _solution = gurobiSolver.getSolution();
        _foundOptimum = true;
      } else {
        _foundOptimum = false;
      }
    }
  }
}

} // namespace mkcp
