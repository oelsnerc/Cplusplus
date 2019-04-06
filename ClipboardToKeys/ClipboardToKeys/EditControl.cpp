#include "stdafx.h"
#include "GetTextFromClipBoard.h"

mmc::string_t getWindowTitle(HWND hWnd)
{
	size_t size = GetWindowTextLength(hWnd);
	mmc::string_t result(size);
	GetWindowText(hWnd, result.data(), result.size());
	return result;
}

void setText(HWND hWnd, const std::wstringstream& str)
{
	SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)str.str().data());
}

static void setTextFromClipBoard(HWND hWnd)
{
	std::wstringstream str;
	str << L"Clipboard is [";
	str << mmc::getTextFromClipBoard(hWnd);
	str << L"]";
	setText(hWnd, str);
}

static void handleKillFocus(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	auto newWindow = (HWND)wParam;
	std::wstringstream str;
	str << L"LostFocus to ";
	if (newWindow)
	{
		str << getWindowTitle(newWindow);
	}
	else
	{
		str << L"unknown!";
	}
	setText(hWnd, str);
}

//*****************************************************************************
constexpr auto ID_EDITCHILD = 100;
WNDPROC wpOrigEditProc;

LRESULT CALLBACK EditWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SETFOCUS:
		setTextFromClipBoard(hWnd);
		break;

	case WM_KILLFOCUS:
		mmc::sendClipboardAsync(hWnd);
		// handleKillFocus(hWnd, wParam, lParam);
		break;

	default:
		return CallWindowProc(wpOrigEditProc, hWnd, message, wParam, lParam);
	}
	return 0;
}

HWND createEditWindow(HWND parent)
{
	auto child = CreateWindowEx(
		0, L"EDIT",   // predefined class 
		NULL,         // no window title 
		WS_CHILD | WS_VISIBLE | WS_VSCROLL |
		ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
		0, 0, 0, 0,   // set size in WM_SIZE message 
		parent,         // parent window 
		(HMENU)ID_EDITCHILD,   // edit control ID 
		(HINSTANCE)GetWindowLong(parent, GWL_HINSTANCE),
		NULL);        // pointer not needed 

	wpOrigEditProc = (WNDPROC)SetWindowLong(child, GWL_WNDPROC, (LONG)EditWndProc);	// set up the event handler
	return child;
}
