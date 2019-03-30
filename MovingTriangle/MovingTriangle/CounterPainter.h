#pragma once

#include "Painter.h"

class CounterPainter : public mmc::Painter
{
protected:
    LRESULT do_paint(Gdiplus::Graphics&, const Gdiplus::Rect&) override;
};
