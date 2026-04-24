//
// Created by michael-kaibel on 27/02/2026.
//

#ifndef MKCP_NEARCLIQUERULES_HPP
#define MKCP_NEARCLIQUERULES_HPP
#include <stdexcept>
#include <vector>

class MultiDimensionalIndex
{
public:
    virtual ~MultiDimensionalIndex() = default;
    virtual unsigned length() const;

    virtual unsigned getEntry(unsigned i) const;
};

template <typename T>
class MultiDimensionalVector
{
public:
    explicit MultiDimensionalVector(unsigned dimension) : _dimension(dimension), _dimensionOne()
    {

    }

    unsigned dimension() const { return _dimensionOne; }

    MultiDimensionalVector<T> &accessSubVector(const MultiDimensionalIndex &index)
    {
        return accessSubVector(index, 0);
    }

    T &accessEntry(const MultiDimensionalIndex &index)
    {
        return accessEntry(index, 0);
    }

    void resize(unsigned newSize)
    {
        if (_dimension == 1)
            _entries.resize(newSize);
        else
            _dimensionOne.resize(newSize, MultiDimensionalVector<T>(_dimension - 1));
    }

    void resize(unsigned newSize, T defaultValue)
    {
        if (_dimension == 1)
            _entries.resize(newSize, defaultValue);
        else
        {
            throw std::runtime_error("Can't resize with default value in subvector with dimension > 1");
        }
    }

    void resize(unsigned newSize, MultiDimensionalVector<T> defaultValue)
    {
        assert(defaultValue.dimension() - 1 == _dimensionOne);

        if (_dimension == 1)
            throw std::runtime_error("Can't resize with default value in subvector with dimension == 1");
        else
        {
            _dimensionOne.resize(newSize, defaultValue);
        }
    }

protected:
    MultiDimensionalVector<T> &accessSubVector(const MultiDimensionalIndex &index, unsigned pos)
    {
        return _dimensionOne[index.getEntry(pos)].accessSubVector(index, pos + 1);
    }

    T &accessEntry(const MultiDimensionalIndex &index, unsigned pos)
    {
        if (_dimension > 1)
            return _dimensionOne[index.getEntry(pos)].accessEntry(index, pos + 1);

        return _entries[index.getEntry(pos)];
    }

private:
    unsigned _dimension;

    //! If dimension > 1 then the 1 lower dimensional entries are stored here
    MultiDimensionalVector<T> _dimensionOne;
    //! Stores values in case that the dimension is 1
    std::vector<T> _entries;
};

// TODO figure out better name
class NIntoKPartitionIterator : public MultiDimensionalIndex
{
public:
    unsigned length() const override;

    unsigned getEntry(unsigned i) const override;

private:
};

#endif //MKCP_NEARCLIQUERULES_HPP