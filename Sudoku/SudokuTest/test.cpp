#include "pch.h"

#include "../Sudoku/SudokuCells.h"
#include <vector>

//*****************************************************************************
using BinData = std::vector<uint8_t>;
template<typename T>
static void writeTo(BinData& data, const T& v)
{
    data.emplace_back(static_cast<BinData::value_type>(v));
}

static BinData writeToBinData(const SudokuCells& cells)
{
    BinData result;
    writeTo(result, cells.getDimension());
    cells.for_each([&result](const SudokuCell & cell, size_t x, size_t y)
        {
            writeTo(result, x);
            writeTo(result, y);
            writeTo(result, cell.getValue());
            return false;
        });
    return result;
}

static SudokuCells readFrom(const BinData& data)
{
    if (data.empty()) {  throw std::invalid_argument("empty data stream"); }

    auto pos = data.begin();

    SudokuCells cells(*pos++);
    while (pos != data.end())
    {
        auto x = *pos++;
        auto y = *pos++;
        cells.setCellValue(x, y, *pos++ );
    }
    return cells;
}

static std::ostream& print(std::ostream& s, const BinData& data)
{
    for(auto& c : data)
    {
        s << static_cast<int>(c) << ',';
    }
    return s;
}

namespace EASY {

    static const BinData input{ 3,
        0,0,5, 0,1,6, 0,3,8, 0,4,4, 0,5,7,
        1,0,3, 1,2,9, 1,6,6,
        2,2,8,
        3,1,1, 3,4,8, 3,7,4,
        4,0,7, 4,1,9, 4,3,6, 4,5,2, 4,7,1,
        5,1,5, 5,4,3, 5,7,9,
        6,6,2,
        7,2,6, 7,6,8, 7,8,7,
        8,3,3, 8,4,1, 8,5,6, 8,7,5
    };

    static const BinData unsolved{ 3,
        0,0,5,0,1,6,0,2,0,0,3,8,0,4,4,0,5,7,0,6,0,0,7,0,0,8,0,
        1,0,3,1,1,0,1,2,9,1,3,0,1,4,0,1,5,0,1,6,6,1,7,0,1,8,0,
        2,0,0,2,1,0,2,2,8,2,3,0,2,4,0,2,5,0,2,6,0,2,7,0,2,8,0,
        3,0,0,3,1,1,3,2,0,3,3,0,3,4,8,3,5,0,3,6,0,3,7,4,3,8,0,
        4,0,7,4,1,9,4,2,0,4,3,6,4,4,0,4,5,2,4,6,0,4,7,1,4,8,0,
        5,0,0,5,1,5,5,2,0,5,3,0,5,4,3,5,5,0,5,6,0,5,7,9,5,8,0,
        6,0,0,6,1,0,6,2,0,6,3,0,6,4,0,6,5,0,6,6,2,6,7,0,6,8,0,
        7,0,0,7,1,0,7,2,6,7,3,0,7,4,0,7,5,0,7,6,8,7,7,0,7,8,7,
        8,0,0,8,1,0,8,2,0,8,3,3,8,4,1,8,5,6,8,6,0,8,7,5,8,8,0
    };

    static const BinData solved{ 3,
        0,0,5, 0,1,6, 0,2,1, 0,3,8, 0,4,4, 0,5,7, 0,6,9, 0,7,2, 0,8,3,
        1,0,3, 1,1,7, 1,2,9, 1,3,5, 1,4,2, 1,5,1, 1,6,6, 1,7,8, 1,8,4,
        2,0,4, 2,1,2, 2,2,8, 2,3,9, 2,4,6, 2,5,3, 2,6,1, 2,7,7, 2,8,5,
        3,0,6, 3,1,1, 3,2,3, 3,3,7, 3,4,8, 3,5,9, 3,6,5, 3,7,4, 3,8,2,
        4,0,7, 4,1,9, 4,2,4, 4,3,6, 4,4,5, 4,5,2, 4,6,3, 4,7,1, 4,8,8,
        5,0,8, 5,1,5, 5,2,2, 5,3,1, 5,4,3, 5,5,4, 5,6,7, 5,7,9, 5,8,6,
        6,0,9, 6,1,3, 6,2,5, 6,3,4, 6,4,7, 6,5,8, 6,6,2, 6,7,6, 6,8,1,
        7,0,1, 7,1,4, 7,2,6, 7,3,2, 7,4,9, 7,5,5, 7,6,8, 7,7,3, 7,8,7,
        8,0,2, 8,1,8, 8,2,7, 8,3,3, 8,4,1, 8,5,6, 8,6,4, 8,7,5, 8,8,9
    };

};


TEST(Sudoku, basic)
{
    auto cells = readFrom(EASY::input);

    EXPECT_EQ(3, cells.getDimension());
    EXPECT_EQ(9, cells.getLength());

    auto data = writeToBinData(cells);
    // print(std::cout, data) << std::endl;

    EXPECT_EQ(EASY::unsolved, data);
}

TEST(Sudoku, solve_easy)
{
    auto cells = readFrom(EASY::input);
    cells.solve();

    const auto data = writeToBinData(cells);
    // print(std::cout, data) << std::endl;

    EXPECT_EQ(EASY::solved, data);
}

TEST(Sudoku, solve_easy_loop)
{
    const auto orig_cells = readFrom(EASY::input);
    for (size_t i = 0; i < 1000; ++i)
    {
        auto cells = orig_cells;
        cells.solve();
    }
}
