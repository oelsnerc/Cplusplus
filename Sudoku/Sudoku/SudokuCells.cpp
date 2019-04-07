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

//*****************************************************************************
template<typename T>
inline void writeBinary(std::ostream& stream, const T& value)
{
    stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

template<typename T>
inline T readBinary(std::istream& stream)
{
    T value;
    stream.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

static const char magicWord[] = "SUDOKU";
static void writeCell(std::ostream& stream, const SudokuCell& cell, size_t x, size_t y)
{
    if (cell.isCalculated()) { return; }
    if (not cell.isSet()) { return;  }

    writeBinary(stream, static_cast<uint8_t>(x));
    writeBinary(stream, static_cast<uint8_t>(y));
    writeBinary(stream, cell.getValue());
}

std::ostream& SudokuCells::writeTo(std::ostream& stream) const
{
    writeBinary(stream, magicWord);
    writeBinary(stream, static_cast<uint8_t>(getDimension()));
    for_each(writeCell, stream);

    return stream;
}

SudokuCells SudokuCells::readFrom(std::istream& stream)
{
    char starter[sizeof(magicWord)];
    stream.read(starter, sizeof(starter));

    if (memcmp(magicWord, starter, sizeof(magicWord)) != 0)
    {
        throw std::invalid_argument("this is not a SUDOKU stream!");
    }

    SudokuCells result(readBinary<uint8_t>(stream));

    while (stream.peek() != EOF)
    {
        auto x = readBinary<uint8_t>(stream);
        auto y = readBinary<uint8_t>(stream);
        auto v = readBinary<SudokuCell::value_t>(stream);
        result.setCellValue(x, y, v);
    }

    return result;
}
