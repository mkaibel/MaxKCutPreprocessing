//
// Created by michael-kaibel on 02/02/2026.
//

#ifndef MKCP_HYPERMETRICFINDER_HPP
#define MKCP_HYPERMETRICFINDER_HPP

#include "networkit/graph/Graph.hpp"
#include "util/indexedSet.hpp"

namespace mkcp
{
    //! Assumes edge weights are edge variables in pairing formulation (x_uv = 1 means u and v are in the same group)
    class HypermetricFinder
    {
    public:
        explicit HypermetricFinder(const NetworKit::Graph& g, unsigned k) : _g(g), _k(k)
        {
        }

        void run();

        const std::vector<std::pair<std::vector<NetworKit::node>, std::vector<NetworKit::node>>>& getPlusMinusOne();

    private:
        const NetworKit::Graph& _g;
        unsigned _k;

        bool _hasRun = false;

        std::vector<std::pair<std::vector<NetworKit::node>, std::vector<NetworKit::node>>> _hypermetricSetsPlusMinusOne;

        //! Updates the gains for either side
        void updateSideGain(NetworKit::node v, const IndexedSet& candidates, bool sideA,
                            std::vector<NetworKit::edgeweight>& sideAGain) const;

        //! Computes f(eta, k) the minimum number of edges not cut on eta nodes with k = _k as defined in RAO (adapted for x_uv = 1 means p(u) = p(v))
        NetworKit::edgeweight computeConstantForSize(unsigned numNodes);
    };
}

#endif //MKCP_HYPERMETRICFINDER_HPP
