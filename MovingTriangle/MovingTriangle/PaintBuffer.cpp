#include "stdafx.h"

#include "PaintBuffer.h"

static RECT getClientRect(HWND window)
{
    RECT r;
    GetClientRect(window, &r);
    return r;
}

static HBITMAP createCompatibleBitmap(HDC dc, const RECT& r)
{ return CreateCompatibleBitmap(dc, r.right - r.left, r.bottom - r.top); }

mmc::PaintBuffer::PaintBuffer(HWND window) :
    ivWindow(window),
    ivPaintStruct(),
    ivDeviceContext(BeginPaint(ivWindow, &ivPaintStruct)),
    ivBufferContext(CreateCompatibleDC(ivDeviceContext)),
    ivClientRect(getClientRect(ivWindow)),
    ivBuffer(createCompatibleBitmap(ivDeviceContext, ivClientRect)),
    ivSaveDC(SaveDC(ivBufferContext))
{
    SelectObject(ivBufferContext, ivBuffer);
}

mmc::PaintBuffer::~PaintBuffer()
{
    BitBlt(ivDeviceContext, ivClientRect.left, ivClientRect.top, ivClientRect.right, ivClientRect.bottom, ivBufferContext, 0, 0, SRCCOPY);
    RestoreDC(ivBufferContext, ivSaveDC);

    DeleteObject(ivBuffer);
    DeleteObject(ivBufferContext);
    EndPaint(ivWindow, &ivPaintStruct);
}
