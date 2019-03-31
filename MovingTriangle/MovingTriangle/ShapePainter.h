#pragma once
#include "Painter.h"
#include "Shapes.h"

template<size_t NUMBER, typename SHAPE>
class ShapePainter : public mmc::Painter
{
private:
    using value_t = SHAPE;
    using Shapes = std::vector<value_t>;
    Shapes ivShapes;

public:
    ShapePainter(const RECT& rect) :
        ivShapes(NUMBER, value_t(rect))
    { }

    // Inherited via Painter
    LRESULT do_paint(HDC& hdc, const RECT& rect) override
    {
        auto current = getCounter() % ivShapes.size();
        auto next = (getCounter() + 1) % ivShapes.size();
        ivShapes[next].moveFrom(ivShapes[current], rect);

        for (auto& s : ivShapes) { s.draw(hdc); }

        return LRESULT();
    }
};
