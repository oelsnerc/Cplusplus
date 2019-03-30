#include "stdafx.h"
#include "Painter.h"
#include "PaintBuffer.h"

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
// some GDI+ managing things
mmc::GDIPlus::GDIPlus()
{   // Initialize GDI+.
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

mmc::GDIPlus::~GDIPlus()
{
    GdiplusShutdown(gdiplusToken);
}

static Rect getRect(const RECT& r)
{
    return Rect(r.left, r.top, r.right - r.left, r.bottom - r.top);
}

//*****************************************************************************
LRESULT mmc::Painter::paint(HWND hWnd)
{
    ++ivCounter;

    mmc::PaintBuffer buffer(hWnd);
    Graphics g(buffer);
    auto rect = getRect(buffer.rect());

    do_paint(g, rect);

    return 0;
}
