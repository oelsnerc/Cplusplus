#include "SudokuPainter.h"

//*****************************************************************************
namespace {
//*****************************************************************************

    class SudokuRect
    {
    private:
        size_t dimension;
        size_t lineNumber;
        size_t lineDistanceX;
        size_t lineDistanceY;
        LONG left;
        LONG right;
        LONG top;
        LONG bottom;

        void drawLineX(HDC& hdc, size_t xLine) const
        {
            const auto x = left + xLine * lineDistanceX;
            ::MoveToEx(hdc, x, top, NULL);
            ::LineTo(hdc, x, bottom);
        }

        void drawLineY(HDC& hdc, size_t yLine) const
        {
            const auto y = top + yLine * lineDistanceY;
            ::MoveToEx(hdc, left, y, NULL);
            ::LineTo(hdc, right, y);
        }

        void drawBoldGrid(HDC& hdc) const
        {
            static const mmc::Pen boldPen(CreatePen(PS_SOLID, 5, GetSysColor(COLOR_WINDOWTEXT)));
            auto p = mmc::select(hdc, boldPen);

            for (size_t xLine = 0; xLine < dimension + 1; ++xLine) { drawLineX(hdc, xLine * dimension); }
            for (size_t yLine = 0; yLine < dimension + 1; ++yLine) { drawLineY(hdc, yLine * dimension); }
        }

        void drawThinGrid(HDC& hdc) const
        {
            static const mmc::Pen thinPen(CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWTEXT)));
            auto p = mmc::select(hdc, thinPen);

            for (size_t xLine = 1; xLine < lineNumber; ++xLine) { drawLineX(hdc, xLine); }
            for (size_t yLine = 1; yLine < lineNumber; ++yLine) { drawLineY(hdc, yLine); }
        }

        mmc::Object<HFONT> createFont() const
        {
            LOGFONT logFont{};
            GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(logFont), &logFont);
            logFont.lfHeight = -(static_cast<LONG>(lineDistanceY / 2));
            logFont.lfWeight = FW_BOLD;

            return mmc::Object<HFONT>{CreateFontIndirect(&logFont)};
        }

        void drawCell(HDC& hdc, const SudokuCell& cell, size_t xLine, size_t yLine) const
        {
            const auto xBegin = static_cast<LONG>(left + xLine * lineDistanceX);
            const auto yBegin = static_cast<LONG>(top + yLine * lineDistanceY);

            auto cnt = cell.countPossibilities();
            if (cnt < 4)
            {
                static const mmc::Object<HBRUSH> brushes[4] =
                {
                    mmc::Object<HBRUSH>(CreateSolidBrush(RGB(123,36, 28))),     // no options left
                    mmc::Object<HBRUSH>(CreateSolidBrush(RGB(82,190,128))),     // one option
                    mmc::Object<HBRUSH>(CreateSolidBrush(RGB(125,206,160))),    // 2 options
                    mmc::Object<HBRUSH>(CreateSolidBrush(RGB(169,223,191))),    // 3 options
                };
                RECT rect{ xBegin, yBegin,
                    static_cast<LONG>(xBegin + lineDistanceX),
                    static_cast<LONG>(yBegin + lineDistanceY)};
                FillRect(hdc, &rect, brushes[cnt]);
            }

            auto n = cell.getValue();
            if (n == 0) { return; }
            auto* str = toString(n);
            if (not str) { return; }

            RECT rect{};
            DrawText(hdc, str, 1, &rect, DT_CALCRECT);

            const auto width = rect.right - rect.left;
            const auto height = rect.bottom - rect.top;

            const auto x = xBegin + (lineDistanceX - width) / 2;
            const auto y = yBegin + (lineDistanceY - height) / 2;
            SetBkMode(hdc, TRANSPARENT);
            
            if (cell.isCalculated())
            // { SetTextColor(hdc, RGB(26,82,118)); }
            { SetTextColor(hdc, RGB(100,30,22)); }
            else
            { SetTextColor(hdc, RGB(23, 32, 42)); }
            TextOut(hdc, x, y, str, 1);
        }

    public:
        explicit SudokuRect(const RECT& parentRect, size_t d) :
            dimension(d),
            lineNumber(d* d)
        {
            const auto parentRectWidth = parentRect.right - parentRect.left;
            const auto parentRectHeight = parentRect.bottom - parentRect.top;
            const auto length = (parentRectWidth > parentRectHeight) ? parentRectHeight : parentRectWidth;

            lineDistanceX = length / lineNumber;
            const auto rectWidth = lineDistanceX * lineNumber;
            left = static_cast<LONG>(parentRect.left + ((parentRectWidth - rectWidth) / 2));
            right = static_cast<LONG>(left + rectWidth);

            lineDistanceY = length / lineNumber;
            const auto rectHeight = lineDistanceY * lineNumber;
            top = static_cast<LONG>(parentRect.top + ((parentRectHeight - rectHeight) / 2));
            bottom = static_cast<LONG>(top + rectHeight);
        }

        void drawGrid(HDC & hdc) const
        {
            drawBoldGrid(hdc);
            drawThinGrid(hdc);
        }

        void drawCells(HDC& hdc, const SudokuCells& cells) const
        {
            const auto font = createFont();
            auto f = mmc::select(hdc, font);

            cells.for_each(&SudokuRect::drawCell, this, hdc);
        }

        LONG getXLine(LONG x) const
        {
            if (x < left) { return -1; }
            if (x > right) { return -1; }
            return (x - left) / lineDistanceX;
        }

        LONG getYLine(LONG y) const
        {
            if (y < top) { return -1; }
            if (y > bottom) { return -1; }
            return (y - top) / lineDistanceY;
        }

    };

//*****************************************************************************
}   // end of anonymous
//*****************************************************************************
const WCHAR* toString(size_t n)
{
    static const WCHAR* strings[10] =
    {
        L"0",
        L"1",
        L"2",
        L"3",
        L"4",
        L"5",
        L"6",
        L"7",
        L"8",
        L"9"
    };

    if (n > 9) { return nullptr; }
    return strings[n];
}

LRESULT SudokuPainter::do_paint(HDC& hdc, const RECT& parentRect)
{
    const SudokuRect rect(parentRect, ivCells.getDimension());
    rect.drawCells(hdc, ivCells);
    rect.drawGrid(hdc);

    return LRESULT();
}

const SudokuCell* SudokuPainter::getCellAt(LONG x, LONG y, const RECT& parentRect) const
{
    const SudokuRect rect(parentRect, ivCells.getDimension());

    auto xLine = rect.getXLine(x);
    if (xLine < 0) { return nullptr; }

    auto yLine = rect.getYLine(y);
    if (yLine < 0) { return nullptr; }

    return &(ivCells.at(xLine, yLine));
}
