#include "stdafx.h"
#include "TrianglePainter.h"
#include <cstdlib>
#include <vector>

//*****************************************************************************
static void clear(HDC& hdc, const RECT& rect)
{
    FillRect(hdc, &rect, GetSysColorBrush(COLOR_WINDOW));
}

//-----------------------------------------------------------------------------
static INT getRandomBetween(INT min, INT max)
{
    auto r = std::rand() % (max - min + 1);
    return (r + min);
}

static Shape::Point getRandomPoint(const RECT& rect)
{
    return Shape::Point{ getRandomBetween(rect.left, rect.right), getRandomBetween(rect.top, rect.bottom) };
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

static Shape::Point getRandomDirection(int max)
{
    return Shape::Point{ getRandomDirectionValue(max), getRandomDirectionValue(max) };
}

static INT getRandomColorTone() { return getRandomBetween(0, 255); }
static COLORREF getRandomColor()
{
    return RGB(getRandomColorTone(), getRandomColorTone(), getRandomColorTone() );
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

static void movePoint(Shape::Point& p, Shape::Point& d, const RECT& r)
{
    move(p.x, d.x, r.left, r.right);
    move(p.y, d.y, r.top, r.bottom);
}

static void moveShape(Shape& dest, const Shape& source, const RECT& rect)
{
    dest = source;
    dest.move(rect);
}

//*****************************************************************************
Shape::Shape(const RECT& rect, size_t numberOfPoints) :
    ivPen(getRandomPen()),
    ivPoints(numberOfPoints),
    ivDirections(numberOfPoints)
{
    for (auto& p : ivPoints) { p = getRandomPoint(rect); }
    for (auto& d : ivDirections) { d = getRandomDirection(5); }
}

Shape::Shape(const Shape& other) :
    ivPen(getRandomPen()),
    ivPoints(other.ivPoints),
    ivDirections(other.ivDirections)
{}

Shape& Shape::operator = (const Shape& other)
{   // don't copy the Pen
    ivPoints = other.ivPoints;
    ivDirections = other.ivDirections;
    return *this;
}

void Shape::draw(HDC& hdc) const
{
    auto s = mmc::select(hdc, ivPen);

    auto& last = ivPoints.back();
    MoveToEx(hdc, last.x, last.y, NULL);
    PolylineTo(hdc, ivPoints.data(), ivPoints.size());
}

void Shape::move(const RECT& rect)
{
    for (size_t i = 0; i < ivPoints.size(); ++i)
    {
        movePoint(ivPoints[i], ivDirections[i], rect);
    }
}

//*****************************************************************************
static RECT getClientRect(HWND hWnd)
{
    RECT result;
    GetClientRect(hWnd, &result);
    return result;
}

TrianglePainter::TrianglePainter(HWND hWnd) :
    ivShapes(100, Shape(getClientRect(hWnd), 3))
{
}

TrianglePainter::~TrianglePainter()
{
}

LRESULT TrianglePainter::do_paint(HDC& hdc, const RECT& rect)
{
    clear(hdc, rect);

    auto current = getCounter() % ivShapes.size();
    auto next = (getCounter()+1) % ivShapes.size();
    moveShape(ivShapes[next], ivShapes[current], rect);

    for (auto& s : ivShapes) { s.draw(hdc); }

    return LRESULT();
}
