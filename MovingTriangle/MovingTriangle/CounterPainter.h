#pragma once

#include "Painter.h"
#include <sstream>

class CounterPainter : public mmc::Painter
{
protected:
    LRESULT do_paint(HDC& hdc, const RECT& rect) override
    {
        std::wostringstream s;
        s << getCounter();

        auto str = s.str();
        TextOut(hdc, 30, 10, str.c_str(), str.size());

        return LRESULT();
    }
};
