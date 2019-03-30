#pragma once

#include "Painter.h"

namespace mmc
{
    class CounterPainter : public mmc::Painter
    {
    protected:
        LRESULT do_paint(Gdiplus::Graphics&, const Gdiplus::Rect&) override;

    public:
        CounterPainter(HWND hWnd);
        ~CounterPainter();
    };


}