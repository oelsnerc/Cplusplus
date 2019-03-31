#include "stdafx.h"
#include "Painter.h"
#include "PaintBuffer.h"

#include <sstream>

//*****************************************************************************
LRESULT mmc::Painter::paint(HWND hWnd)
{
    ++ivCounter;

    mmc::PaintBuffer buffer(hWnd);

    RECT rect;
    GetClientRect(hWnd, &rect);

    do_paint(buffer, rect);

    return 0;
}
