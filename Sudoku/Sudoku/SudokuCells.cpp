#include "SudokuCells.h"

int SudokuCells::findCellIndex(const SudokuCell* cell) const
{
    if (not cell) { return -1; }
    for (size_t i = 0; i < ivCells.size(); ++i)
    {
        auto* c = &ivCells[i];
        if (c == cell) { return static_cast<int>(i); }
    }
    return -1;
}

void SudokuCells::setPossibility(size_t x, size_t y, value_t value, bool set)
{
    // horizontal option
    for (size_t xLine = 0; xLine < ivLength; ++xLine)
    {
        toCell(xLine, y).setPossibility(value, set);
    }

    // vertical option
    for (size_t yLine = 0; yLine < ivLength; ++yLine)
    {
        toCell(x, yLine).setPossibility(value, set);
    }

    // square option
    auto xTop = (x / ivDimension) * ivDimension;
    auto yTop = (y / ivDimension) * ivDimension;
    for (size_t xLine = xTop; xLine < xTop + ivDimension; ++xLine)
    {
        for (size_t yLine = yTop; yLine < yTop + ivDimension; ++yLine)
        {
            { toCell(xLine, yLine).setPossibility(value, set); }
        }
    }
}

void SudokuCells::resetCellValue(size_t x, size_t y)
{
    auto& cell = toCell(x, y);
    auto v = cell.getValue();
    if (v == 0) { return; }

    setPossibility(x, y, v, true);
    cell.resetValue();
}

void SudokuCells::setCellValue(size_t x, size_t y, SudokuCell::value_t v)
{
    setPossibility(x, y, v, false);
    toCell(x, y).setValue(v);
}

static SudokuCell::value_t getOnlyPossibility(SudokuCell& cell)
{
    for (SudokuCell::value_t i = 1; i < 16; ++i)
    {
        if (cell.isPossible(i))
        {
            return i;
        }
    }
    return 0;
}

bool SudokuCells::solve()
{
    for (auto& cell : ivCells)
    {
        if (cell.isSet()) { continue; }
        if (cell.isCalculated()) { continue; }

        const auto cnt = cell.countPossibilities();
        if (cnt == 0) { return true; }
        if (cnt == 1)
        { 
            cell.setCalculated();
            setCellValue(&cell,getOnlyPossibility(cell));
            return solve();
        }
    }
    return false;
}
