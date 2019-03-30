#pragma once

#include "stdafx.h"
#include <memory>

namespace mmc
{
    class Painter
    {
    public:
        Painter(HWND hWnd);
        ~Painter();

        LRESULT paint(HWND hWnd);

    public:
        using ptr = std::unique_ptr<Painter>;
        static ptr create(HWND hWnd) { return ptr(new Painter(hWnd)); }
    };
}
