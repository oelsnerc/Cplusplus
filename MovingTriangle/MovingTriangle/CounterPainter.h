#pragma once

#include "Painter.h"

class CounterPainter : public mmc::Painter
{
protected:
    LRESULT do_paint(HDC& hdc, const RECT& rect) override;
};
