#include "framework.h"

#include "Painter.h"
#include "PaintBuffer.h"

#include <sstream>

static void clear(HDC& hdc, const RECT& rect)
{
    FillRect(hdc, &rect, GetSysColorBrush(COLOR_WINDOW));
}

//*****************************************************************************
LRESULT mmc::Painter::paint(HWND hWnd)
{
    ++ivCounter;

    mmc::PaintBuffer buffer(hWnd);

    RECT rect;
    GetClientRect(hWnd, &rect);

    clear(buffer, rect);
    do_paint(buffer, rect);

    return 0;
}
