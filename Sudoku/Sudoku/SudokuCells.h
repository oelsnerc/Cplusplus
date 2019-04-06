#pragma once

#include <cstdint>
#include <vector>
#include <stack>

class SudokuCell
{
public:
    using value_t = uint8_t;

private:
    bool        ivIsCalculated;
    value_t     ivValue;
    uint32_t    ivPossibilities;

    static uint32_t getBit(size_t v)
    {
        return 1 << (v - 1);
    }

public:
    explicit SudokuCell(size_t maxValue) :
        ivIsCalculated(false),
        ivValue(0),
        ivPossibilities( (1 << maxValue) - 1)
    { }

    bool isSet() const { return ivValue != 0; }
    bool isCalculated() const { return ivIsCalculated; }
    uint8_t getValue() const { return ivValue; }

    bool isPossible(value_t value) const
    {
        return ( (ivPossibilities & getBit(value)) != 0);
    }

    size_t countPossibilities() const
    {
        size_t result = 0;
        auto v = ivPossibilities;
        for (; v; ++result) {  v &= v - 1; }
        return result;
    }

    void removePossibility(value_t value)
    { ivPossibilities &= ~getBit(value); }

    void addPossibility(value_t value)
    { ivPossibilities |= getBit(value); }

    void setPossibility(value_t value, bool set)
    {
        if (set) { addPossibility(value); }
        else { removePossibility(value); }
    }

    void setValue(value_t value)
    {
        ivValue = value;
        ivPossibilities |= getBit(value);
    }

    void resetValue()
    {
        ivValue = 0;
        ivIsCalculated = false;
    }

    void setCalculated(bool set = true) { ivIsCalculated = set; }
};

class SudokuCells
{
public:
    using Cells = std::vector<SudokuCell>;
    using value_t = SudokuCell::value_t;

private:
    size_t  ivDimension;
    size_t  ivLength;
    Cells   ivCells;

    size_t toCellIndex(size_t x, size_t y) const { return y * ivLength + x; }
    SudokuCell& toCell(size_t x, size_t y) { return ivCells[toCellIndex(x, y)]; }

    int findCellIndex(const SudokuCell* cell) const;

    void setPossibility(size_t x, size_t y, value_t value, bool set);
 
public:
    explicit SudokuCells(size_t dimension) :
        ivDimension(dimension),
        ivLength(ivDimension * ivDimension),
        ivCells(ivLength * ivLength, SudokuCell(ivLength))
    {}

    size_t getDimension() const { return ivDimension; }
    size_t getLength() const { return ivLength; }
    size_t size() const { return ivCells.size(); }

    const SudokuCell& at(size_t x, size_t y) const
    { return ivCells.at( toCellIndex(x,y)); }

    SudokuCell& at(size_t x, size_t y)
    {
        auto& cell = static_cast<const SudokuCells*>(this)->at(x, y);
        return const_cast<SudokuCell&>(cell);
    }

    void resetCellValue(size_t x, size_t y);
    void resetCellValue(const SudokuCell* cell)
    {
        const auto index = findCellIndex(cell);
        if (index < 0) { return; }

        const auto x = index % ivLength;
        const auto y = index / ivLength;
        return resetCellValue(x, y);
    }

    void setCellValue(size_t x, size_t y, SudokuCell::value_t v);
    void setCellValue(const SudokuCell* cell, SudokuCell::value_t v)
    {
        const auto index = findCellIndex(cell);
        if (index < 0) { return; }

        const auto x = index % ivLength;
        const auto y = index / ivLength;
        return setCellValue(x, y, v);
    }

    bool solve();

    template<typename FUNC, typename... ARGS>
    void for_each(FUNC&& f, ARGS&& ... args)
    {
        for (size_t x = 0; x < ivLength; ++x)
        {
            for (size_t y = 0; y < ivLength; ++y)
            {
                std::invoke(f, args..., ivCells[toCellIndex(x,y)], x, y);
            }
        }
    }

    template<typename FUNC, typename... ARGS>
    void for_each(FUNC&& f, ARGS&& ... args) const
    {
        for (size_t x = 0; x < ivLength; ++x)
        {
            for (size_t y = 0; y < ivLength; ++y)
            {
                std::invoke(f, args..., ivCells[toCellIndex(x, y)], x, y);
            }
        }
    }
};

using SudokuStack = std::stack<SudokuCells>;
