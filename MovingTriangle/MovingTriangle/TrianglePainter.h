#pragma once
#include "Painter.h"

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>

#include <vector>

struct Shape
{
    using Points = std::vector<Gdiplus::Point>;
    using Directions = std::vector<Gdiplus::Point>;

    Points ivPoints;
    Directions ivDirections;

    explicit Shape(const RECT& rect, size_t numberOfPoints);

    void draw(Gdiplus::Graphics&) const;
    void move(const Gdiplus::Rect &);
};

class TrianglePainter : public mmc::Painter
{
private:
    using Shapes = std::vector<Shape>;
    Shapes ivShapes;

public:
    TrianglePainter(HWND hWnd);
    ~TrianglePainter();

    // Inherited via Painter
    LRESULT do_paint(Gdiplus::Graphics &, const Gdiplus::Rect &) override;
};

