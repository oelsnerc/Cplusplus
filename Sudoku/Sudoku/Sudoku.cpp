// Sudoku.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "sudoku.h"
#include "SudokuPainter.h"
#include <stack>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// actual Sudoku Variables
constexpr size_t dimension = 3;
static SudokuCells cells(dimension);
static SudokuPainter painter(cells);
static SudokuStack cellStack;

constexpr UINT IDM_NUMBERS_0 = 1000;
constexpr UINT IDM_NUMBERS_END = IDM_NUMBERS_0 + dimension*dimension +1;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SUDOKU, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SUDOKU));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SUDOKU));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SUDOKU);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   // Initialize the Sudoku to random Values
   //cells.for_each([](size_t maxValue, SudokuCell & cell, size_t x, size_t y)
   //    {
   //        auto v = std::rand() % (maxValue + 1);
   //        cell.setValue(static_cast<SudokuCell::value_t>(v));
   //    }, cells.getLength());

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

static const SudokuCell* getCell(HWND hWnd, LONG x, LONG y)
{
    RECT rect;
    GetClientRect(hWnd, &rect);

    return painter.getCellAt(x, y, rect);
}

static const SudokuCell* showPopUpMenu(HWND hWnd, LONG x, LONG y)
{
    auto* cell = getCell(hWnd, x, y);
    if (cell == nullptr) { return cell; }

    mmc::Object<HMENU> hMenuPopup(CreatePopupMenu());
    for (SudokuCell::value_t i = 1; i < cells.getLength() + 1; ++i)
    {
        if (cell->isPossible(i))
        {
            AppendMenu(hMenuPopup, MF_STRING, IDM_NUMBERS_0 + i, toString(i));
        }
    }
    AppendMenu(hMenuPopup, MF_STRING, IDM_FILE_UNDO, L"undo");

    for (size_t i = 1; i < cells.getLength() + 1; ++i)
    {
        UINT state = cell->isPossible( static_cast<SudokuCell::value_t>(i)) ? MF_ENABLED : MF_GRAYED;
        EnableMenuItem(hMenuPopup, IDM_NUMBERS_0 + i, state);
    }

    POINT pt{ x, y };
    ClientToScreen(hWnd, &pt);
    TrackPopupMenu(hMenuPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
    return cell;
}

static bool isMenuItemChecked(HWND hWnd, UINT item)
{
    MENUITEMINFO info;
    info.cbSize = sizeof(MENUITEMINFO);
    info.fMask = MIIM_STATE;

    auto hMenu = GetMenu(hWnd);
    GetMenuItemInfo(hMenu, item, FALSE, &info);
    return ( (info.fState & MFS_CHECKED) != 0);
}

static void toggleMenuItem(HWND hWnd, UINT item)
{
    MENUITEMINFO info;
    info.cbSize = sizeof(MENUITEMINFO);
    info.fMask = MIIM_STATE;

    auto hMenu = GetMenu(hWnd);
    GetMenuItemInfo(hMenu, item, FALSE, &info);
    if (info.fState & MFS_CHECKED)
    { info.fState &= ~MFS_CHECKED; }
    else
    { info.fState |= MFS_CHECKED; }

    SetMenuItemInfo(hMenu, item, FALSE, &info);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static const SudokuCell* cell = nullptr;

    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);

            if ((wmId > IDM_NUMBERS_0) && (wmId < IDM_NUMBERS_END))
            {
                cellStack.push(cells);

                cells.resetCellValue(cell);

                auto value = static_cast<SudokuCell::value_t>(wmId - IDM_NUMBERS_0);
                cells.setCellValue(cell, value);

                if (isMenuItemChecked(hWnd, ID_EDIT_SOLVEWHILEENTER))
                { cells.solve(); }

                InvalidateRect(hWnd, NULL, FALSE);
                break;
            }

            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDM_FILE_UNDO:
                if (not cellStack.empty())
                {
                    cells = cellStack.top();
                    cellStack.pop();
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                break;
            case ID_FILE_NEW:
                cells = SudokuCells(dimension);
                cellStack = SudokuStack();
                InvalidateRect(hWnd, NULL, FALSE);
                break;
            case ID_EDIT_SOLVE:
                cells.solve();
                InvalidateRect(hWnd, NULL, FALSE);
                break;
            case ID_EDIT_SOLVEWHILEENTER:
                toggleMenuItem(hWnd, ID_EDIT_SOLVEWHILEENTER);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        painter.paint(hWnd);
        break;
    case WM_LBUTTONDOWN:
        cell = showPopUpMenu(hWnd, LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
