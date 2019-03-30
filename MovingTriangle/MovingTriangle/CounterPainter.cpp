#include "stdafx.h"
#include "CounterPainter.h"

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
using namespace Gdiplus;

#include <sstream>

namespace colors
{
    static const Color& backGround = Color::Wheat;
    static const Color& text = Color::Black;
}

//*****************************************************************************
static void clear(Graphics& g, const Rect& rect)
{
    static const SolidBrush brush( colors::backGround );
    g.FillRectangle(&brush, rect);
}

static void drawString(Graphics& g, INT x, INT y, INT size, const std::wostringstream& str)
{
    static const SolidBrush brush(colors::text);

    FontFamily  fontFamily(L"Times New Roman");
    Font        font(&fontFamily, static_cast<REAL>(size), FontStyleRegular, UnitPixel);
    PointF      pointF(static_cast<REAL>(x), static_cast<REAL>(y));

    g.DrawString(str.str().c_str(), -1, &font, pointF, &brush);
}

LRESULT CounterPainter::do_paint(Gdiplus::Graphics& g, const Gdiplus::Rect& rect)
{
    clear(g, rect);

    std::wostringstream s;
    s << getCounter();
    drawString(g, 30, 10, 24, s);

    return 0;
}
