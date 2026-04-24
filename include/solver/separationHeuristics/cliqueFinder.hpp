//
// Created by michael-kaibel on 22/01/2026.
//

#ifndef MKCP_CLIQUEFINDER_HPP
#define MKCP_CLIQUEFINDER_HPP

#include "networkit/graph/Graph.hpp"


namespace mkcp
{
    //! Assumes edge weights are edge variables in pairing formulation (x_uv = 1 means u and v are in the same group)
    class CliqueFinder
    {
        public:
            CliqueFinder(const NetworKit::Graph &g, unsigned k) : _g(g), _k(k) {}

            void run();

            const std::vector<std::vector<NetworKit::node>> &getCliques();
    private:
        const NetworKit::Graph &_g;
        unsigned _k;

        bool _hasRun = false;

        std::vector<std::vector<NetworKit::node>> _cliques;
    };
}

#endif //MKCP_CLIQUEFINDER_HPP