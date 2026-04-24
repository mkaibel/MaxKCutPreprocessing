//
// Created by michael-kaibel on 15/01/2026.
//

#ifndef CODE_TLS_CHECKER_HPP
#define CODE_TLS_CHECKER_HPP

#include <gurobi_c.h>
#include <vector>
#include <networkit/graph/Graph.hpp>

#include "networkit/Globals.hpp"

namespace mkcp
{

    class TLSChecker
    {
    public:
        TLSChecker(NetworKit::Graph &g, const std::vector<NetworKit::node> &leftSide);

        /**
         *
         * @param vType The type of variables (GRB_BINARY or GRB_CONTINUOUS) for ILP or LP-relaxation
         */
        void run(char vType = GRB_BINARY);

        [[nodiscard]] bool hasRun() const;

        [[nodiscard]] double getObjectiveValue() const;

    private:
        NetworKit::Graph &_g;

        const std::vector<NetworKit::node> &_leftSide;

        double _objectiveValue = 0.0;

        bool _hasRun = false;

        const char LEFT = 0, RIGHT = 1;
    };
}

#endif //CODE_TLS_CHECKER_HPP