#pragma once
#include "Painter.h"

#include <vector>

struct Shape
{
    using Point = POINT;
    using Points = std::vector<Point>;
    using Directions = std::vector<Point>;

    Points ivPoints;
    Directions ivDirections;

    explicit Shape(const RECT& rect, size_t numberOfPoints);

    void draw(HDC& hdc, const mmc::Pen& pen) const;
    void move(const RECT & rect);
};

class TrianglePainter : public mmc::Painter
{
private:
    mmc::Pen    ivPen;

    using Shapes = std::vector<Shape>;
    Shapes ivShapes;

public:
    TrianglePainter(HWND hWnd);
    ~TrianglePainter();

    // Inherited via Painter
    LRESULT do_paint(HDC& hdc, const RECT& rect) override;
};

