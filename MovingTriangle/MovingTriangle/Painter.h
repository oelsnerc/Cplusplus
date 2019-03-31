#pragma once

#include "stdafx.h"
#include <memory>

namespace mmc
{
    template<typename T>
    struct Object
    {
        T ivObject;
        explicit Object(T&& obj) : ivObject(std::forward<T>(obj)) {}
        ~Object() { DeleteObject(ivObject); }

        T& getObject() { return ivObject; }
        const T& getObject() const { return ivObject; }

        Object(const Object&) = delete;
        Object& operator = (const Object&) = delete;

        Object(Object&&) = default;
        Object& operator = (Object&&) = default;
    };

    using Pen = Object<HPEN>;

    template<typename T>
    struct Selector
    {
        HDC ivHdc;
        T ivOld;
        explicit Selector(HDC hdc, const T& obj) :
            ivHdc(hdc),
            ivOld( (T)SelectObject( ivHdc, obj ) )
        {}
        ~Selector() { SelectObject(ivHdc, ivOld); }
    };

    template<typename T>
    inline Selector<T> select(HDC hdc, const T& obj)
    { return Selector<T>(hdc, obj); }

    template<typename T>
    inline Selector<T> select(HDC hdc, const Object<T>& obj)
    { return Selector<T>(hdc, obj.getObject()); }

    class Painter
    {
    private:
        size_t  ivCounter;

    protected:
        virtual LRESULT do_paint(HDC& hdc, const RECT& rect) = 0;

    public:
        Painter() : ivCounter(0) {}
        virtual ~Painter() {};

        LRESULT paint(HWND hWnd);

        size_t getCounter() const { return ivCounter; }

    public:
        using ptr = std::unique_ptr<Painter>;

        template<typename T, typename... ARGS>
        static ptr create(ARGS&&... args) { return ptr(new T(std::forward<ARGS>(args)...)); }
    };
}
