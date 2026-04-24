//
// Created by michael-kaibel on 09/02/2026.
//

#ifndef MKCP_ODDSHORTESTCYCLE_HPP
#define MKCP_ODDSHORTESTCYCLE_HPP

#include "networkit/graph/Graph.hpp"

class OddShortestCycle
{
public:
    OddShortestCycle(const NetworKit::Graph &g) : _g(g) {}

private:
    const NetworKit::Graph &_g;
};

#endif //MKCP_ODDSHORTESTCYCLE_HPP