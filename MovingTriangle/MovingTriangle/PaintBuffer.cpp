#include "stdafx.h"

#include "PaintBuffer.h"

mmc::PaintBuffer::PaintBuffer(HWND window) :
    ivWindow(window),
    ivPaintStruct(),
    ivDeviceContext(BeginPaint(ivWindow, &ivPaintStruct)),
    ivBufferContext(CreateCompatibleDC(ivDeviceContext)),
    ivClientRect(getClientRect(ivWindow)),
    ivBuffer(CreateCompatibleBitmap(ivDeviceContext, ivClientRect.right - ivClientRect.left, ivClientRect.bottom - ivClientRect.top)),
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
