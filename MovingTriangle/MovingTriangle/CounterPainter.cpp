#include "stdafx.h"
#include "CounterPainter.h"

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment (lib,"Gdiplus.lib")

#include <sstream>

namespace colors
{
    static const Color backGround(255, 128, 128, 255);
    static const Color text(255, 0, 0, 255);
}

//*****************************************************************************
static void clear(Graphics& g, const Rect& rect)
{
    static const SolidBrush brush(colors::backGround);
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

mmc::CounterPainter::CounterPainter(HWND hWnd) :
    mmc::Painter()
{
}

mmc::CounterPainter::~CounterPainter()
{
}

LRESULT mmc::CounterPainter::do_paint(Gdiplus::Graphics& g, const Gdiplus::Rect& rect)
{
    clear(g, rect);

    std::wostringstream s;
    s << getCounter();
    drawString(g, 30, 10, 24, s);

    return 0;
}
