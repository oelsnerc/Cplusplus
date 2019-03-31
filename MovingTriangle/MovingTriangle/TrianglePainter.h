#pragma once
#include "Painter.h"

#include <vector>

struct Shape
{
    using Point = POINT;
    using Points = std::vector<Point>;
    using Directions = std::vector<Point>;
    using ptr = std::unique_ptr<Shape>;

    mmc::Pen    ivPen;
    Points      ivPoints;
    Directions  ivDirections;

    explicit Shape(const RECT& rect, size_t numberOfPoints);
    Shape(const Shape& other);

    Shape& operator = (const Shape& other);

    void draw(HDC& hdc) const;
    void move(const RECT & rect);
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
    LRESULT do_paint(HDC& hdc, const RECT& rect) override;
};
