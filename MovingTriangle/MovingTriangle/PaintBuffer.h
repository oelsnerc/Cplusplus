#pragma once

#include "stdafx.h"

namespace mmc
{

    class PaintBuffer
    {
    private:
        HWND        ivWindow;
        PAINTSTRUCT ivPaintStruct;
        HDC         ivDeviceContext;
        HDC         ivBufferContext;
        RECT        ivClientRect;
        HBITMAP     ivBuffer;
        int         ivSaveDC;

    public:
        explicit PaintBuffer(HWND window);

        PaintBuffer(const PaintBuffer&) = delete;
        PaintBuffer& operator = (const PaintBuffer&) = delete;
        PaintBuffer(PaintBuffer&&) = default;
        PaintBuffer& operator = (PaintBuffer&&) = default;

        ~PaintBuffer();

        operator HDC& () { return ivBufferContext; }

        const RECT& rect() const { return ivClientRect; }
    };
}
