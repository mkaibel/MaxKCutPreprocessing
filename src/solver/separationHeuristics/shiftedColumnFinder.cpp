//
// Created by michael-kaibel on 30/01/2026.
//

#include "solver/separationHeuristics/shiftedColumnFinder.hpp"

#include <stdexcept>

namespace mkcp
{
    const std::vector<std::pair<ArrayCoord, std::vector<ArrayCoord>>>& ShiftedColumnFinder::getShiftedColumns() const
    {
        if (!_hasRun)
        {
            throw std::runtime_error("Run clique finder first");
        }

        return _violatedShiftedColumns;
    }

    void ShiftedColumnFinder::run()
    {
        _hasRun = true;

        std::vector<std::vector<double>> minColumnValue(_n);
        std::vector<std::vector<bool>> inMinColumn(_n);

        for (NetworKit::node v = 0; v < _n; v++)
        {
            minColumnValue[v].resize(maxColour(v), 0.0);
            inMinColumn[v].resize(maxColour(v), false);
        }

        minColumnValue[0][0] = _assignmentVariables[0][0];
        inMinColumn[0][0] = true;

        for (NetworKit::node v = 1; v < std::min(_k, static_cast<unsigned>(_assignmentVariables.size())); v++)
        {
            if (_assignmentVariables[v][v] < minColumnValue[v-1][v-1])
            {
                minColumnValue[v][v] = _assignmentVariables[v][v];
                inMinColumn[v][v] = true;
            }
            else
            {
                minColumnValue[v][v] = minColumnValue[v-1][v-1];
            }
        }

        for (unsigned v = 1; v < _n; v++)
        {
            minColumnValue[v][0] = minColumnValue[v-1][0] + _assignmentVariables[v][0];
            inMinColumn[v][0] = true;

            for (unsigned i = 1; i < std::min(v, maxColour(v)); i++)
            {
                if (minColumnValue[v - 1][i] + _assignmentVariables[v][i] < minColumnValue[v-1][i-1])
                {
                    minColumnValue[v][i] = minColumnValue[v - 1][i] + _assignmentVariables[v][i];
                    inMinColumn[v][i] = true;
                }
                else
                {
                    minColumnValue[v][i] = minColumnValue[v-1][i - 1];
                }
            }
        }

        for (unsigned v = 1; v < _n; v++)
        {
            double rowSumValue = _assignmentVariables[v][maxColour(v) - 1];

            if (rowSumValue > minColumnValue[v - 1][maxColour(v) - 2])
                addShiftedColumnInequality(v, maxColour(v) - 1, inMinColumn);

            for (unsigned i = maxColour(v) - 1; i > 0; i--)
            {
                rowSumValue += _assignmentVariables[v][i];

                if (rowSumValue > minColumnValue[v - 1][i - 1])
                    addShiftedColumnInequality(v, i, inMinColumn);
            }
        }
    }

    unsigned ShiftedColumnFinder::maxColour(unsigned v) const
    {
        return std::min(v + 1, _k);
    }

    void ShiftedColumnFinder::addShiftedColumnInequality(unsigned v, unsigned i, const std::vector<std::vector<bool>>& inMinColumn)
    {
        std::vector<ArrayCoord> shiftedColumn;

        ArrayCoord coord(v-1, i-1);

        while (coord.x >= 0 && coord.y <= coord.x)
        {
            if (inMinColumn[coord.x][coord.y])
            {
                shiftedColumn.emplace_back(coord);
                coord.x--;
            }
            else
            {
                coord.x--;
                coord.y--;
            }
        }

        _violatedShiftedColumns.emplace_back(ArrayCoord(v, i), shiftedColumn);
    }
}


