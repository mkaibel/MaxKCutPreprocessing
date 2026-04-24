//
// Created by michael-kaibel on 25/02/2026.
//

#ifndef MKCP_LOCALSEARCHHEURISTIC_HPP
#define MKCP_LOCALSEARCHHEURISTIC_HPP
#include <chrono>

#include "globals.hpp"
#include "networkit/graph/Graph.hpp"

namespace mkcp
{
    class KCutLocalSearch
    {
    public:
        /**
         * class contains a local search algorithm for maximum k cut with tabu search to escape local minima
         * @param g the graph
         * @param k
         * @param timeLimit in milliseconds
         * @param seed 1234 by default
         * @param iterationLimit Maximum number of iterations (that is restarts with a random solution) the heuristic runs before terminating, maximum of unsigned by default
         * @param tabuSteps how many steps of Tabu search are to be performed before regular 1/2 opt resume
         * @param failedStepsBeforeReset How many iterations of local search then tabu search can be perfomed in a row without finding a new best solution
         */
        KCutLocalSearch(const NetworKit::Graph &g, const unsigned k, const double timeLimit, const unsigned seed = 1234, const unsigned iterationLimit = std::numeric_limits<unsigned>::max(), unsigned tabuSteps = 500, unsigned failedStepsBeforeReset = 1000): _g(g), _k(k), _timeLimit(timeLimit), _iterationLimit(iterationLimit), _seed(seed), _bestSolution(g.numberOfNodes()), _vertexConstraints(g.numberOfNodes(), std::vector<bool>(k, false)), _tabuSteps(tabuSteps), _failedStepsBeforeReset(failedStepsBeforeReset)
        {
            _numVerticesWithFreedom = _g.numberOfNodes();
        }

        /**
         * Add constraints for vertices
         * @param vertexConstraints vector of constraints s.t. vertexConstraints[v][i] == true means v can't have colour i
         */
        void addVertexConstraints(const std::vector<std::vector<bool>> &vertexConstraints);

        void run();

        [[nodiscard]] bool hasRun() const;

        [[nodiscard]] double getSolutionValue() const;

        [[nodiscard]] const Partition &getSolution() const;

    private:
        [[nodiscard]] NetworKit::edgeweight moveNode(NetworKit::node v, partitionID newPartition, Partition &currentSolution, std::vector<std::vector<NetworKit::edgeweight>> &lossForGroup) const;

        //! Does exhaustive one-opt steps, returns if any improvements were made
        [[nodiscard]] NetworKit::edgeweight iterateOneOpt(Partition &currentSolution, std::vector<std::vector<NetworKit::edgeweight>> &lossForGroup) const;
        //! Does numSteps two-opt steps along edges, returns improvement made
        [[nodiscard]] NetworKit::edgeweight iterateTwoOpt(Partition &currentSolution, std::vector<std::vector<NetworKit::edgeweight>> &lossForGroup, unsigned numSteps = 1) const;

        //! returns the best one-opt step with respect to tabu, returns the best such step
        [[nodiscard]] std::pair<NetworKit::node, partitionID> iterateOneTabu(Partition &currentSolution, std::vector<std::vector<NetworKit::edgeweight>> &lossForGroup, unsigned tabuIteration, std::vector<std::vector<unsigned>> &tabuUntil) const;
        [[nodiscard]] std::pair<std::pair<NetworKit::node, partitionID>, std::pair<NetworKit::node, partitionID>> iterateTwoTabu(Partition &currentSolution, std::vector<std::vector<NetworKit::edgeweight>> &lossForGroup, unsigned tabuIteration, std::vector<std::vector<unsigned>> &tabuUntil) const;

        const NetworKit::Graph &_g;
        unsigned _k;
        double _timeLimit;
        unsigned _iterationLimit = std::numeric_limits<unsigned>::max();
        unsigned _seed;
        bool _hasRun = false;

        bool _hasVertexConstraints = false;
        NetworKit::count _numVerticesWithFreedom;
        std::vector<std::vector<bool>> _vertexConstraints;

        std::chrono::time_point<std::chrono::steady_clock> _t0;

        Partition _bestSolution;
        NetworKit::edgeweight _bestValue = 0.0;

        unsigned _tabuLower = 0, _tabuUpper = 0;
        const unsigned _tabuSteps, _failedStepsBeforeReset;
    };
}

#endif //MKCP_LOCALSEARCHHEURISTIC_HPP