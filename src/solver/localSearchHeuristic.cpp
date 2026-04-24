//
// Created by michael-kaibel on 25/02/2026.
//

#include "solver/localSearchHeuristic.hpp"

#include <chrono>

namespace mkcp
{
    void KCutLocalSearch::addVertexConstraints(const std::vector<std::vector<bool>> &vertexConstraints)
    {
        _hasVertexConstraints = true;
        _numVerticesWithFreedom = 0;

        // Count number of vertices with at least two possible colours
        for (NetworKit::node v : _g.nodeRange())
        {
            bool freedomOne = false;
            for (unsigned i = 0; i < _k; i++)
            {
                if (!_vertexConstraints[v][i])
                {
                    if (freedomOne)
                    {
                        _numVerticesWithFreedom++;
                        break;
                    }

                    freedomOne = true;
                }
            }
        }
    }

    bool KCutLocalSearch::hasRun() const
    {
        return _hasRun;
    }

    const Partition& KCutLocalSearch::getSolution() const
    {
        if (!_hasRun)
            throw std::runtime_error("Run algorithm first");

        return _bestSolution;
    }

    double KCutLocalSearch::getSolutionValue() const
    {
        if (!_hasRun)
            throw std::runtime_error("Run algorithm first");

        return _bestValue;
    }

    void KCutLocalSearch::run()
    {
        _hasRun = true;

        _t0 = std::chrono::steady_clock::now();

        // TODO move this into the algorithm declaration
        _tabuLower = 3;
        _tabuUpper = std::max(static_cast<unsigned long>(4), _numVerticesWithFreedom / 10);

        std::uniform_int_distribution<unsigned> distTabu(_tabuLower, _tabuUpper);

        unsigned numThreads = omp_get_max_threads();

        std::vector<NetworKit::edgeweight> bestSolutionVals(numThreads, 0.0);
        std::vector<Partition> bestSolutionPartitions(numThreads, Partition(_g.numberOfNodes()));

#pragma omp parallel num_threads(numThreads)
        {
            unsigned tid = omp_get_thread_num();

            std::mt19937 rng(_seed + tid);
            std::uniform_int_distribution<unsigned> distk(0, _k - 1);

            NetworKit::edgeweight currentValue = 0.0;
            Partition currentSolution(_g.numberOfNodes());

            std::vector<std::vector<NetworKit::edgeweight>> lossForGroup(
                _g.numberOfNodes(), std::vector<NetworKit::edgeweight>(_k, 0));
            std::vector<std::vector<unsigned>> tabuUntil(_g.numberOfNodes(), std::vector<unsigned>(_k, 0));
            unsigned tabuIteration = 0;

            unsigned iterations = 0;

            while (static_cast<double>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - _t0)
                .count()) < _timeLimit && iterations < _iterationLimit)
            {
                iterations++;

                // Clear previous data
                currentValue = 0.0;
                for (NetworKit::node v : _g.nodeRange())
                {
                    for (double& x : lossForGroup[v]) x = 0.0;
                }

                // Initialize random solution and its corresponding losses
                for (NetworKit::node v : _g.nodeRange())
                {
                    unsigned i = distk(rng);
                    currentSolution.assignElement(v, i);
                    for (auto [w, weight] : _g.weightNeighborRange(v))
                    {
                        lossForGroup[w][i] += weight;
                    }
                }

                for (const NetworKit::WeightedEdge& e : _g.edgeWeightRange())
                {
                    if (currentSolution.assignmentOf(e.u) != currentSolution.assignmentOf(e.v))
                        currentValue += e.weight;
                }

                unsigned iterationsWithoutImprovement = 0;

                while (iterationsWithoutImprovement < _failedStepsBeforeReset && static_cast<double>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - _t0)
                    .count()) < _timeLimit)
                {
                    bool madeGain = true;
                    while (madeGain && static_cast<double>(
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - _t0)
                        .count()) < _timeLimit)
                    {
                        //std::cout << "Startling local search iteration\n";
                        currentValue += iterateOneOpt(currentSolution, lossForGroup);
                        NetworKit::edgeweight valBeforeTwoOpt = currentValue;
                        currentValue += iterateTwoOpt(currentSolution, lossForGroup);
                        madeGain = (currentValue - valBeforeTwoOpt) / valBeforeTwoOpt > 0.000001;
                    }

                    if (currentValue > bestSolutionVals[tid])
                    {
                        bestSolutionVals[tid] = currentValue;
                        bestSolutionPartitions[tid] = currentSolution;

                        iterationsWithoutImprovement = 0;
                    }
                    else
                    {
                        iterationsWithoutImprovement++;
                    }

                    if (iterationsWithoutImprovement < _failedStepsBeforeReset && static_cast<double>(
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - _t0)
                        .count()) < _timeLimit)
                    {
                        NetworKit::edgeweight valBeforeTabu = currentValue;
                        unsigned stepsInTabuPhase = 0;

                        while (currentValue <= valBeforeTabu && stepsInTabuPhase <= _tabuSteps && static_cast<double>(
                            std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - _t0)
                            .count()) < _timeLimit)
                        {
                            // TODO make this probability a value that can be controlled
                            if (distTabu(rng) % 5 != 0)
                            {
                                std::pair<NetworKit::node, partitionID> tabuMove = iterateOneTabu(currentSolution, lossForGroup, tabuIteration, tabuUntil);
                                tabuUntil[tabuMove.first][currentSolution.assignmentOf(tabuMove.first)] =
                                    tabuIteration + distTabu(rng);
                                currentValue += moveNode(tabuMove.first, tabuMove.second, currentSolution,
                                                         lossForGroup);
                            }
                            else
                            {
                                std::pair<std::pair<NetworKit::node, partitionID>, std::pair<
                                              NetworKit::node, partitionID>> tabuMove = iterateTwoTabu(currentSolution, lossForGroup, tabuIteration, tabuUntil);
                                if (tabuMove.first.first != NetworKit::none)
                                {
                                    tabuUntil[tabuMove.first.first][currentSolution.
                                        assignmentOf(tabuMove.first.first)] = tabuIteration + distTabu(rng);
                                    currentValue += moveNode(tabuMove.first.first, tabuMove.first.second,
                                                             currentSolution, lossForGroup);
                                    tabuUntil[tabuMove.second.first][currentSolution.assignmentOf(
                                        tabuMove.second.first)] = tabuIteration + distTabu(rng);
                                    currentValue += moveNode(tabuMove.second.first, tabuMove.second.second,
                                                             currentSolution, lossForGroup);
                                }
                            }

                            tabuIteration++;
                            stepsInTabuPhase++;
                        }
                    }
                }
            }
        }

        for (unsigned tid = 0; tid < numThreads; tid++)
        {
            if (bestSolutionVals[tid] > _bestValue)
            {
                _bestValue = bestSolutionVals[tid];
                _bestSolution = bestSolutionPartitions[tid];
            }
        }

        if (!_hasVertexConstraints)
            _bestSolution.sortLexicographically();
    }

    NetworKit::edgeweight KCutLocalSearch::moveNode(NetworKit::node v, partitionID newPartition,
                                                    Partition& currentSolution,
                                                    std::vector<std::vector<NetworKit::edgeweight>>& lossForGroup) const
    {
        assert(!_vertexConstraints[v][newPartition]);

        partitionID currentAssignment = currentSolution.assignmentOf(v);

        for (auto [w, weight] : _g.weightNeighborRange(v))
        {
            lossForGroup[w][currentAssignment] -= weight;
            lossForGroup[w][newPartition] += weight;
        }

        currentSolution.assignElement(v, newPartition);

        return lossForGroup[v][currentAssignment] - lossForGroup[v][newPartition];
    }

    NetworKit::edgeweight KCutLocalSearch::iterateOneOpt(Partition& currentSolution,
                                                         std::vector<std::vector<NetworKit::edgeweight>>& lossForGroup)
    const
    {
        NetworKit::edgeweight gainSoFar = 0.0;

        bool madeGain = true;

        // TODO also include timer
        while (madeGain)
        {
            madeGain = false;

            NetworKit::edgeweight currentLoss;

            for (NetworKit::node v : _g.nodeRange())
            {
                currentLoss = lossForGroup[v][currentSolution.assignmentOf(v)];

                for (partitionID i = 0; i < _k; i++)
                {
                    if (_vertexConstraints[v][i])
                        continue;

                    if (currentLoss - lossForGroup[v][i] > 0.0)
                    {
                        madeGain = true;
                        gainSoFar += moveNode(v, i, currentSolution, lossForGroup);
                        break;
                    }
                }
            }
        }

        return gainSoFar;
    }

    std::pair<NetworKit::node, partitionID> KCutLocalSearch::iterateOneTabu(
        Partition& currentSolution, std::vector<std::vector<NetworKit::edgeweight>>& lossForGroup,
        unsigned tabuIteration, std::vector<std::vector<unsigned>>& tabuUntil) const
    {
        NetworKit::node bestNodeToMove = NetworKit::none;
        partitionID moveTo = -1;
        NetworKit::edgeweight bestGain = -std::numeric_limits<NetworKit::edgeweight>::max();

        NetworKit::edgeweight currentLoss;

        for (NetworKit::node v : _g.nodeRange())
        {
            currentLoss = lossForGroup[v][currentSolution.assignmentOf(v)];

            for (partitionID i = 0; i < _k; i++)
            {
                // Skip if current colour is tabu
                if (_vertexConstraints[v][i] || i == currentSolution.assignmentOf(v) || tabuUntil[v][i] > tabuIteration)
                    continue;

                if (currentLoss - lossForGroup[v][i] > bestGain)
                {
                    bestGain = currentLoss - lossForGroup[v][i];
                    bestNodeToMove = v;
                    moveTo = i;
                }
            }
        }

        assert(bestNodeToMove != NetworKit::none);

        return {bestNodeToMove, moveTo};
    }

    NetworKit::edgeweight KCutLocalSearch::iterateTwoOpt(Partition& currentSolution,
                                                         std::vector<std::vector<NetworKit::edgeweight>>& lossForGroup,
                                                         unsigned numSteps) const
    {
        NetworKit::edgeweight totalGain = 0.0;

        for (unsigned step = 0; step < numSteps; step++)
        {
            NetworKit::node uToMove = NetworKit::none, vToMove = NetworKit::none;
            partitionID uTo = none, vTo = none;
            NetworKit::edgeweight bestGain = 0.0;

            for (const NetworKit::WeightedEdge& e : _g.edgeWeightRange())
            {
                partitionID uAssignment = currentSolution.assignmentOf(e.u), vAssignment = currentSolution.
                                assignmentOf(e.v);
                NetworKit::edgeweight uLoss = lossForGroup[e.u][uAssignment], vLoss = lossForGroup[e.v][vAssignment];

                if (e.weight < 0.0)
                {
                    if (uAssignment == vAssignment)
                    {
                        // Case u and v are moved to the same colour
                        for (partitionID i = 0; i < _k; i++)
                        {
                            if (_vertexConstraints[e.u][i] || _vertexConstraints[e.v][i] || i == uAssignment)
                                continue;

                            if (uLoss - lossForGroup[e.u][i] + vLoss - lossForGroup[e.v][i] - 2.0 * e.weight >
                                bestGain)
                            {
                                uToMove = e.u;
                                vToMove = e.v;
                                uTo = i;
                                vTo = i;
                                bestGain = uLoss - lossForGroup[e.u][i] + vLoss - lossForGroup[e.v][i] - 2.0 * e.
                                    weight;
                            }
                        }

                        partitionID uBest = none, uSecondBest = none;
                        NetworKit::edgeweight uBestVal = std::numeric_limits<NetworKit::edgeweight>::max(),
                                              uSecondBestVal = std::numeric_limits<NetworKit::edgeweight>::max();
                        partitionID vBest = none, vSecondBest = none;
                        NetworKit::edgeweight vBestVal = std::numeric_limits<NetworKit::edgeweight>::max(),
                                              vSecondBestVal = std::numeric_limits<NetworKit::edgeweight>::max();

                        // For the case that u and v are moved to different groups we only have to consider them being mapped to the best and second best option
                        for (partitionID i = 0; i < _k; i++)
                        {
                            if (i == uAssignment)
                                continue;

                            if (!_vertexConstraints[e.u][i] && lossForGroup[e.u][i] < uSecondBestVal)
                            {
                                if (lossForGroup[e.u][i] < uBestVal)
                                {
                                    uSecondBest = uBest;
                                    uSecondBestVal = uBestVal;
                                    uBest = i;
                                    uBestVal = lossForGroup[e.u][i];
                                }
                                else
                                {
                                    uSecondBest = i;
                                    uSecondBestVal = lossForGroup[e.u][i];
                                }
                            }

                            if (!_vertexConstraints[e.v][i] && lossForGroup[e.v][i] < vSecondBestVal)
                            {
                                if (lossForGroup[e.v][i] < vBestVal)
                                {
                                    vSecondBest = vBest;
                                    vSecondBestVal = vBestVal;
                                    vBest = i;
                                    vBestVal = lossForGroup[e.v][i];
                                }
                                else
                                {
                                    vSecondBest = i;
                                    vSecondBestVal = lossForGroup[e.v][i];
                                }
                            }
                        }

                        if (uBest != vBest)
                        {
                            if (uLoss - lossForGroup[e.u][uBest] + vLoss - lossForGroup[e.v][vBest] - e.weight >
                                bestGain)
                            {
                                uToMove = e.u;
                                vToMove = e.v;
                                uTo = uBest;
                                vTo = vBest;
                                bestGain = uLoss - lossForGroup[e.u][uBest] + vLoss - lossForGroup[e.v][vBest] - e.
                                    weight;
                            }
                        }
                        else
                        {
                            if (uLoss - lossForGroup[e.u][uBest] + vLoss - lossForGroup[e.v][vSecondBest] - e.weight >
                                bestGain)
                            {
                                uToMove = e.u;
                                vToMove = e.v;
                                uTo = uBest;
                                vTo = vSecondBest;
                                bestGain = uLoss - lossForGroup[e.u][uBest] + vLoss - lossForGroup[e.v][vSecondBest] -
                                    e.weight;
                            }
                            if (uLoss - lossForGroup[e.u][uSecondBest] + vLoss - lossForGroup[e.v][vBest] - e.weight >
                                bestGain)
                            {
                                uToMove = e.u;
                                vToMove = e.v;
                                uTo = uSecondBest;
                                vTo = vBest;
                                bestGain = uLoss - lossForGroup[e.u][uSecondBest] + vLoss - lossForGroup[e.v][vBest] -
                                    e.weight;
                            }
                        }
                    }
                    else
                    {
                        // If u and v are in different groups and w(e) < 0 we only have to consider the case that they are moved to the same group
                        for (partitionID i = 0; i < _k; i++)
                        {
                            if (i == uAssignment || i == vAssignment || _vertexConstraints[e.u][i] || _vertexConstraints[e.v][i])
                                continue;

                            if (uLoss - lossForGroup[e.u][i] + vLoss - lossForGroup[e.v][i] - e.weight > bestGain)
                            {
                                uToMove = e.u;
                                vToMove = e.v;
                                uTo = i;
                                vTo = i;
                                bestGain = uLoss - lossForGroup[e.u][i] + vLoss - lossForGroup[e.v][i] - e.weight;
                            }
                        }
                    }
                }
                else
                {
                    if (uAssignment == vAssignment)
                        continue;
                    // Since we know that there are no positive one-opt-steps left and w(e) > 0 we only have to consider the case that u and v are in different groups
                    // And the subcases that they swap groups or that one moves to the group of the other and the other moves to a third group
                    if (uLoss - lossForGroup[e.u][vAssignment] + vLoss - lossForGroup[e.v][uAssignment] + 2.0 * e.
                        weight > bestGain && !_vertexConstraints[e.u][vAssignment] && !_vertexConstraints[e.v][uAssignment])
                    {
                        uToMove = e.u;
                        vToMove = e.v;
                        uTo = vAssignment;
                        vTo = uAssignment;
                        bestGain = uLoss - lossForGroup[e.u][vAssignment] + vLoss - lossForGroup[e.v][uAssignment] +
                            2.0 * e.weight;
                    }

                    for (partitionID i = 0; i < _k; i++)
                    {
                        if (i == uAssignment || i == vAssignment)
                            continue;

                        if (uLoss - lossForGroup[e.u][vAssignment] + vLoss - lossForGroup[e.v][i] + e.weight >
                            bestGain && _vertexConstraints[e.u][vAssignment] && !_vertexConstraints[e.v][i])
                        {
                            uToMove = e.u;
                            vToMove = e.v;
                            uTo = vAssignment;
                            vTo = i;
                            bestGain = uLoss - lossForGroup[e.u][vAssignment] + vLoss - lossForGroup[e.v][i] + e.
                                weight;
                        }
                        if (uLoss - lossForGroup[e.u][i] + vLoss - lossForGroup[e.v][uAssignment] + e.weight >
                            bestGain && _vertexConstraints[e.v][uAssignment] && !_vertexConstraints[e.u][i])
                        {
                            uToMove = e.u;
                            vToMove = e.v;
                            uTo = i;
                            vTo = uAssignment;
                            bestGain = uLoss - lossForGroup[e.u][i] + vLoss - lossForGroup[e.v][uAssignment] + e.
                                weight;
                        }
                    }
                }
            }

            if (uToMove == NetworKit::none)
            {
                //std::cout << "Gain from two-opt " << totalGain << "\n";
                return totalGain;
            }

            assert(bestGain > 0.0);

            totalGain += moveNode(uToMove, uTo, currentSolution, lossForGroup) + moveNode(
                vToMove, vTo, currentSolution, lossForGroup);
        }

        return totalGain;
    }

    std::pair<std::pair<NetworKit::node, partitionID>, std::pair<NetworKit::node, partitionID>>
    KCutLocalSearch::iterateTwoTabu(Partition& currentSolution, std::vector<std::vector<NetworKit::edgeweight>>& lossForGroup,
        unsigned tabuIteration, std::vector<std::vector<unsigned>>& tabuUntil) const
    {
        NetworKit::node uToMove = NetworKit::none, vToMove = NetworKit::none;
        partitionID uTo = none, vTo = none;
        NetworKit::edgeweight bestGain = -std::numeric_limits<NetworKit::edgeweight>::max();

        for (const NetworKit::WeightedEdge& e : _g.edgeWeightRange())
        {
            partitionID uAssignment = currentSolution.assignmentOf(e.u), vAssignment = currentSolution.
                            assignmentOf(e.v);
            NetworKit::edgeweight uLoss = lossForGroup[e.u][uAssignment], vLoss = lossForGroup[e.v][vAssignment];

            if (e.weight < 0.0)
            {
                if (uAssignment == vAssignment)
                {
                    for (partitionID i = 0; i < _k; i++)
                    {
                        if (i == uAssignment || tabuUntil[e.u][i] > tabuIteration || tabuUntil[e.v][i] >
                            tabuIteration)
                            continue;

                        if (uLoss - lossForGroup[e.u][i] + vLoss - lossForGroup[e.v][i] - 2.0 * e.weight >
                            bestGain)
                        {
                            uToMove = e.u;
                            vToMove = e.v;
                            uTo = i;
                            vTo = i;
                            bestGain = uLoss - lossForGroup[e.u][i] + vLoss - lossForGroup[e.v][i] - 2.0 * e.
                                weight;
                        }
                    }

                    partitionID uBest = none, uSecondBest = none;
                    NetworKit::edgeweight uBestVal = std::numeric_limits<NetworKit::edgeweight>::max(),
                                          uSecondBestVal = std::numeric_limits<NetworKit::edgeweight>::max();
                    partitionID vBest = none, vSecondBest = none;
                    NetworKit::edgeweight vBestVal = std::numeric_limits<NetworKit::edgeweight>::max(),
                                          vSecondBestVal = std::numeric_limits<NetworKit::edgeweight>::max();

                    // For the case that u and v are moved to different groups we only have to consider them being mapped to the best and second best option
                    for (partitionID i = 0; i < _k; i++)
                    {
                        if (i == uAssignment)
                            continue;

                        if (tabuUntil[e.u][i] <= tabuIteration && lossForGroup[e.u][i] < uSecondBestVal)
                        {
                            if (lossForGroup[e.u][i] < uBestVal)
                            {
                                uSecondBest = uBest;
                                uSecondBestVal = uBestVal;
                                uBest = i;
                                uBestVal = lossForGroup[e.u][i];
                            }
                            else
                            {
                                uSecondBest = i;
                                uSecondBestVal = lossForGroup[e.u][i];
                            }
                        }

                        if (tabuUntil[e.v][i] <= tabuIteration && lossForGroup[e.v][i] < vSecondBestVal)
                        {
                            if (lossForGroup[e.v][i] < vBestVal)
                            {
                                vSecondBest = vBest;
                                vSecondBestVal = vBestVal;
                                vBest = i;
                                vBestVal = lossForGroup[e.v][i];
                            }
                            else
                            {
                                vSecondBest = i;
                                vSecondBestVal = lossForGroup[e.v][i];
                            }
                        }
                    }

                    if (uBest != none && vBest != none && uBest != vBest)
                    {
                        if (uLoss - lossForGroup[e.u][uBest] + vLoss - lossForGroup[e.v][vBest] - e.weight >
                            bestGain)
                        {
                            uToMove = e.u;
                            vToMove = e.v;
                            uTo = uBest;
                            vTo = vBest;
                            bestGain = uLoss - lossForGroup[e.u][uBest] + vLoss - lossForGroup[e.v][vBest] - e.
                                weight;
                        }
                    }
                    else
                    {
                        if (uBest != none && vSecondBest != none &&
                            uLoss - lossForGroup[e.u][uBest] + vLoss - lossForGroup[e.v][vSecondBest] - e.weight >
                            bestGain)
                        {
                            uToMove = e.u;
                            vToMove = e.v;
                            uTo = uBest;
                            vTo = vSecondBest;
                            bestGain = uLoss - lossForGroup[e.u][uBest] + vLoss - lossForGroup[e.v][vSecondBest] -
                                e.weight;
                        }
                        if (uSecondBest != none && vBest != none &&
                            uLoss - lossForGroup[e.u][uSecondBest] + vLoss - lossForGroup[e.v][vBest] - e.weight >
                            bestGain)
                        {
                            uToMove = e.u;
                            vToMove = e.v;
                            uTo = uSecondBest;
                            vTo = vBest;
                            bestGain = uLoss - lossForGroup[e.u][uSecondBest] + vLoss - lossForGroup[e.v][vBest] -
                                e.weight;
                        }
                    }
                }
                else
                {
                    // If u and v are in different groups and w(e) < 0 we only have to consider the case that they are moved to the same group
                    for (partitionID i = 0; i < _k; i++)
                    {
                        if (i == uAssignment || i == vAssignment)
                            continue;

                        if (uLoss - lossForGroup[e.u][i] + vLoss - lossForGroup[e.v][i] - e.weight > bestGain)
                        {
                            uToMove = e.u;
                            vToMove = e.v;
                            uTo = i;
                            vTo = i;
                        }
                    }
                }
            }
            else
            {
                if (uAssignment == vAssignment)
                    continue;
                // Since we know that there are no positive one-opt-steps left and w(e) > 0 we only have to consider the case that u and v are in different groups
                // And the subcases that they swap groups or that one moves to the group of the other and the other moves to a third group
                if (tabuUntil[e.u][vAssignment] <= tabuIteration && tabuUntil[e.v][uAssignment] <= tabuIteration
                    && uLoss - lossForGroup[e.u][vAssignment] + vLoss - lossForGroup[e.v][uAssignment] + 2.0 * e.
                    weight > bestGain)
                {
                    uToMove = e.u;
                    vToMove = e.v;
                    uTo = vAssignment;
                    vTo = uAssignment;
                    bestGain = uLoss - lossForGroup[e.u][vAssignment] + vLoss - lossForGroup[e.v][uAssignment] +
                        2.0 * e.weight;
                }

                for (partitionID i = 0; i < _k; i++)
                {
                    if (i == uAssignment || i == vAssignment)
                        continue;

                    if (tabuUntil[e.u][vAssignment] <= tabuIteration && tabuUntil[e.v][i] <= tabuIteration
                        && uLoss - lossForGroup[e.u][vAssignment] + vLoss - lossForGroup[e.v][i] + e.weight >
                        bestGain)
                    {
                        uToMove = e.u;
                        vToMove = e.v;
                        uTo = vAssignment;
                        vTo = i;
                        bestGain = uLoss - lossForGroup[e.u][vAssignment] + vLoss - lossForGroup[e.v][i] + e.
                            weight;
                    }
                    if (tabuUntil[e.u][i] <= tabuIteration && tabuUntil[e.v][uAssignment] <= tabuIteration
                        && uLoss - lossForGroup[e.u][i] + vLoss - lossForGroup[e.v][uAssignment] + e.weight >
                        bestGain)
                    {
                        uToMove = e.u;
                        vToMove = e.v;
                        uTo = i;
                        vTo = uAssignment;
                        bestGain = uLoss - lossForGroup[e.u][i] + vLoss - lossForGroup[e.v][uAssignment] + e.
                            weight;
                    }
                }
            }
        }

        if (uToMove == NetworKit::none)
        {
            return {{NetworKit::none, none}, {NetworKit::none, none}};
        }

        return {{uToMove, uTo}, {vToMove, vTo}};
    }
}
