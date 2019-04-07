#pragma once

#include <Windows.h>
#include <string>

namespace dialog
{
    extern std::wstring getOpenFileName(HWND hWnd);
    extern std::wstring getSaveFileName(HWND hWnd);
}   // end of namespace dialog
