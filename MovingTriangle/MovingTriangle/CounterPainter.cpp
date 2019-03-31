#include "stdafx.h"
#include "CounterPainter.h"

#include <sstream>

//*****************************************************************************
static void clear(HDC& hdc, const RECT& rect)
{
    FillRect(hdc, &rect, GetSysColorBrush(COLOR_WINDOW));
}

static void drawString(HDC& hdc, INT x, INT y, const std::wostringstream& stream)
{
    auto str = stream.str();
    TextOut(hdc, x, y, str.c_str(), str.size());
}

LRESULT CounterPainter::do_paint(HDC& hdc, const RECT& rect)
{
    clear(hdc, rect);

    std::wostringstream s;
    s << getCounter();
    drawString(hdc, 30, 10, s);

    return 0;
}
