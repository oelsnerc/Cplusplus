#include "stdafx.h"
#include "TrianglePainter.h"
#include <cstdlib>
#include <vector>

using namespace Gdiplus;

//*****************************************************************************
static void clear(Graphics& g, const Rect& rect)
{
    static const SolidBrush brush(Color::White);
    g.FillRectangle(&brush, rect);
}

//-----------------------------------------------------------------------------
static INT getRandomBetween(INT min, INT max)
{
    auto r = std::rand() % (max - min + 1);
    return (r + min);
}

static Point getRandomPoint(const RECT& rect)
{
    return Point(getRandomBetween(rect.left, rect.right), getRandomBetween(rect.top, rect.bottom));
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

static Point getRandomDirection(int max)
{ return Point(getRandomDirectionValue(max), getRandomDirectionValue(max)); }

//-----------------------------------------------------------------------------
static void move(int& p, int& d, int min, int max)
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

static void movePoint(Point& p, Point& d, const Rect& r)
{
    move(p.X, d.X, r.GetLeft(), r.GetRight());
    move(p.Y, d.Y, r.GetTop(), r.GetBottom());
}

//*****************************************************************************
Shape::Shape(const RECT& rect, size_t numberOfPoints) :
    ivPoints(numberOfPoints),
    ivDirections(numberOfPoints)
{
    for (auto& p : ivPoints) { p = getRandomPoint(rect); }
    for (auto& d : ivDirections) { d = getRandomDirection(5); }
}

void Shape::draw(Gdiplus::Graphics & g) const
{
    static const Pen pen(Color::Blue);
    g.DrawLines(&pen, ivPoints.data(), ivPoints.size());
    g.DrawLine(&pen, ivPoints.back(), ivPoints.front());
}

void Shape::move(const Gdiplus::Rect & rect)
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

static Rect getRect(const RECT& rect)
{
    return Rect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
}

TrianglePainter::TrianglePainter(HWND hWnd) :
    ivShapes(50, Shape(getClientRect(hWnd), 3))
{
}

TrianglePainter::~TrianglePainter()
{
}

LRESULT TrianglePainter::do_paint(Gdiplus::Graphics & g, const Gdiplus::Rect & rect)
{
    clear(g, rect);

    auto current = getCounter() % ivShapes.size();
    auto next = (getCounter()+1) % ivShapes.size();

    ivShapes[next] = ivShapes[current];
    ivShapes[next].move(rect);

    for (auto& s : ivShapes) { s.draw(g); }

    return LRESULT();
}
