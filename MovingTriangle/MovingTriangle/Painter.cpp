#include "stdafx.h"
#include "Painter.h"
#include "PaintBuffer.h"

mmc::Painter::Painter(HWND hWnd)
{
}

mmc::Painter::~Painter()
{
}

LRESULT mmc::Painter::paint(HWND hWnd)
{
    mmc::PaintBuffer buffer(hWnd);

    HBRUSH hBrush = CreateSolidBrush(RGB(128, 128, 255));
    FillRect(buffer, &buffer.rect(), hBrush);
    DeleteObject(hBrush);

    return 0;
}
