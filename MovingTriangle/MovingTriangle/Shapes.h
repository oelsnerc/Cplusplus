#pragma once
#include "Painter.h"
#include <vector>

namespace shapes
{

    struct Base
    {
        using Point = POINT;
        using Points = std::vector<Point>;
        using Directions = std::vector<Point>;

        mmc::Pen    ivPen;
        Points      ivPoints;
        Directions  ivDirections;

        explicit Base(const RECT& rect, size_t numberOfPoints);
        Base(const Base& other);

        Base& operator = (const Base& other);

        virtual void draw(HDC& hdc) const = 0;
        void move(const RECT & rect);
        void moveFrom(const Base& other, const RECT& rect);
    };

    struct Triangle : public Base
    {
        explicit Triangle(const RECT& rect);
        void draw(HDC& hdc) const override;
    };

    struct Ellipse : public Base
    {
        explicit Ellipse(const RECT& rect);
        void draw(HDC& hdc) const override;
    };

}
