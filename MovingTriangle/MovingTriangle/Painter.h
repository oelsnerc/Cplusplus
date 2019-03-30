#pragma once

#include "stdafx.h"
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <memory>

namespace mmc
{
    struct GDIPlus
    {
        ULONG_PTR   gdiplusToken;
        explicit GDIPlus();
        ~GDIPlus();
    };

    class Painter
    {
    private:
        GDIPlus gdiPlus;
        size_t  ivCounter;

    protected:
        virtual LRESULT do_paint(Gdiplus::Graphics&, const Gdiplus::Rect&) = 0;

    public:
        Painter() : gdiPlus(), ivCounter(0) {}
        virtual ~Painter() {};

        LRESULT paint(HWND hWnd);

        size_t getCounter() const { return ivCounter; }

    public:
        using ptr = std::unique_ptr<Painter>;

        template<typename T, typename... ARGS>
        static ptr create(ARGS&&... args) { return ptr(new T(std::forward<ARGS>(args)...)); }
    };
}
