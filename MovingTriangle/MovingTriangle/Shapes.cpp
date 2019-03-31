#include "stdafx.h"
#include "Shapes.h"
#include <cstdlib>

//-----------------------------------------------------------------------------
static INT getRandomBetween(INT min, INT max)
{
    auto r = std::rand() % (max - min + 1);
    return (r + min);
}

static shapes::Base::Point getRandomPoint(const RECT& rect)
{
    return shapes::Base::Point{ getRandomBetween(rect.left, rect.right), getRandomBetween(rect.top, rect.bottom) };
}

static int getRandomDirectionValue(int max)
{
    int result = 0;
    while (result == 0)
    {
        result = getRandomBetween(-max, max);
    }
    return result;
}

static shapes::Base::Point getRandomDirection(int max)
{
    return shapes::Base::Point{ getRandomDirectionValue(max), getRandomDirectionValue(max) };
}

static INT getRandomColorTone() { return getRandomBetween(0, 255); }
static COLORREF getRandomColor()
{
    return RGB(getRandomColorTone(), getRandomColorTone(), getRandomColorTone());
}

static HPEN getRandomPen()
{
    return CreatePen(PS_SOLID, 1, getRandomColor() /* GetSysColor(COLOR_WINDOWTEXT) */);
}

//-----------------------------------------------------------------------------
static void move(LONG& p, LONG& d, int min, int max)
{
    p += d;
    if (p < min)
    {
        d = std::abs(d);
        while (p < min) { p += d; }
    }

    if (p > max)
    {
        d = -std::abs(d);
        while (p > max) { p += d; }
    }
}

static void movePoint(shapes::Base::Point& p, shapes::Base::Point& d, const RECT& r)
{
    move(p.x, d.x, r.left, r.right);
    move(p.y, d.y, r.top, r.bottom);
}

//*****************************************************************************
shapes::Base::Base(const RECT& rect, size_t numberOfPoints) :
    ivPen(getRandomPen()),
    ivPoints(numberOfPoints),
    ivDirections(numberOfPoints)
{
    for (auto& p : ivPoints) { p = getRandomPoint(rect); }
    for (auto& d : ivDirections) { d = getRandomDirection(10); }
}

shapes::Base::Base(const shapes::Base& other) :
    ivPen(getRandomPen()),
    ivPoints(other.ivPoints),
    ivDirections(other.ivDirections)
{}

shapes::Base& shapes::Base::operator = (const shapes::Base& other)
{   // don't copy the Pen
    ivPoints = other.ivPoints;
    ivDirections = other.ivDirections;
    return *this;
}

void shapes::Base::move(const RECT& rect)
{
    for (size_t i = 0; i < ivPoints.size(); ++i)
    {
        movePoint(ivPoints[i], ivDirections[i], rect);
    }
}

void shapes::Base::moveFrom(const Base& other, const RECT& rect)
{
    *this = other;
    move(rect);
}

//-----------------------------------------------------------------------------
shapes::Triangle::Triangle(const RECT& r) : shapes::Base(r, 3)
{ }

void shapes::Triangle::draw(HDC& hdc) const
{
    auto p = mmc::select(hdc, ivPen);

    auto& last = ivPoints.back();
    MoveToEx(hdc, last.x, last.y, NULL);
    PolylineTo(hdc, ivPoints.data(), ivPoints.size());
}

//-----------------------------------------------------------------------------
shapes::Ellipse::Ellipse(const RECT& r) : shapes::Base(r, 2)
{ }

void shapes::Ellipse::draw(HDC& hdc) const
{
    auto p = mmc::select(hdc, ivPen);
    auto b = mmc::select(hdc, GetStockObject(HOLLOW_BRUSH));

    ::Ellipse(hdc, ivPoints[0].x, ivPoints[0].y, ivPoints[1].x, ivPoints[1].y);
}
