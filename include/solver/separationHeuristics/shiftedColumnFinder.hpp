//
// Created by michael-kaibel on 30/01/2026.
//

#ifndef MKCP_SHIFTEDCOLUMNFINDER_HPP
#define MKCP_SHIFTEDCOLUMNFINDER_HPP
#include <vector>

#include "networkit/Globals.hpp"
#include "networkit/auxiliary/Multiprecision.hpp"


namespace mkcp
{
    struct ArrayCoord;

    /**
     * Represents coordinates in diagonal space, x is the diagonal, y is the how manyth entry in the diagonal
     */
    struct DiagonalCoord
    {
        DiagonalCoord(ArrayCoord coords);

        int x, y;
    };

    /**
     * Represents coordinates in regular coordinate space
     */
    struct ArrayCoord
    {
        ArrayCoord(DiagonalCoord coords)
        {
            x = coords.x + coords.y;
            y = coords.y;
        }

        ArrayCoord(int x, int y) : x(x), y(y) {}

        int x, y;
    };

    //! Weird order so we can have diagonal coords <-> array coords translation using constructors
    inline DiagonalCoord::DiagonalCoord(ArrayCoord coords)
    {
        x = coords.x - coords.y;
        y = coords.y;
    }

    class ShiftedColumnFinder
    {
    public:
        ShiftedColumnFinder(const std::vector<std::vector<double>> &assignmentVariables, const NetworKit::count n, const unsigned k) : _n(n), _k(k), _assignmentVariables(assignmentVariables) {};

        void run();

        [[nodiscard]] bool hasRun() const
        {
            return _hasRun;
        };

        /**
         *
         * @return Shifted columns in array coordinates for which the shifted column inequalities were found to be violated
         */
        [[nodiscard]] const std::vector<std::pair<ArrayCoord, std::vector<ArrayCoord>>> &getShiftedColumns() const;

    private:
        bool _hasRun = false;

        const NetworKit::count _n;
        const unsigned _k;
        const std::vector<std::vector<double>> &_assignmentVariables;

        std::vector<std::pair<ArrayCoord, std::vector<ArrayCoord>>> _violatedShiftedColumns;

        [[nodiscard]] unsigned maxColour(unsigned v) const;

        void addShiftedColumnInequality(unsigned v, unsigned i, const std::vector<std::vector<bool>> &inMinColumn);
    };
}

#endif //MKCP_SHIFTEDCOLUMNFINDER_HPP