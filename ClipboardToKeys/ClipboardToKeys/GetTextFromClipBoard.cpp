#include "stdafx.h"
#include "GetTextFromClipBoard.h"
#include <thread>
#include <chrono>

namespace {

	struct ClipboardPtr
	{
		bool isOpen;
		ClipboardPtr(HWND hWnd) : isOpen(OpenClipboard(hWnd)) { }
		~ClipboardPtr()	{ if (isOpen) { CloseClipboard(); }	}
		operator bool() const { return isOpen; }
	};

	static HANDLE getClipBoardData(const ClipboardPtr& clp)
	{
		if (not clp) { return nullptr; }
		return GetClipboardData(CF_TEXT);
	}

	static LPVOID getLock(HANDLE glb)
	{
		if (not glb) { return nullptr; }
		return GlobalLock(glb);
	}

	struct ClipboardLock
	{
		HANDLE data;
		LPVOID lock;

		ClipboardLock(const ClipboardPtr& clp) :
			data(getClipBoardData(clp)),
			lock(getLock(data))
		{}

		~ClipboardLock() { if (lock) { GlobalUnlock(data); } }

		const LPCCH getText() const { return static_cast<const LPCCH>(lock); }
	};

	static int transform(const LPCCH clp, wchar_t* dest = nullptr, size_t size = 0)
	{
		return MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, clp, -1, dest, size);
	}

	static size_t getClipboardSize(const LPCCH clp)
	{
		return transform(clp);
	}

	static void copyClipboard(mmc::string_t& result, const LPCCH clp)
	{
		result.resize(getClipboardSize(clp));
		transform(clp, result.data(), result.size());
	}

}	// anonymous

mmc::string_t mmc::getTextFromClipBoard(HWND hWnd)
{
	string_t result;
	if (not IsClipboardFormatAvailable(CF_TEXT))
	{ return result; }

	ClipboardPtr clp(hWnd);
	ClipboardLock lck(clp);

	auto text = lck.getText();
	if (not text) { return result; }

	copyClipboard(result, text);
	return result;
}

using input_t = std::vector<INPUT>;
static INPUT createINPUT(const wchar_t& c)
{
	INPUT ip;
	// Set up a generic keyboard event.
	ip.type = INPUT_KEYBOARD;
	ip.ki.dwFlags = KEYEVENTF_UNICODE;
	ip.ki.wVk = 0;		// the key is in wScan
	ip.ki.wScan = c;	// hardware scan code for key
	ip.ki.time = 0;
	ip.ki.dwExtraInfo = 0;
	return ip;
}

static input_t createINPUT(const mmc::string_t& str)
{
	input_t result;
	for (auto& c : str)
	{
		auto ip = createINPUT(c);
		result.emplace_back(ip);
		ip.ki.dwFlags = KEYEVENTF_KEYUP;
		result.emplace_back(std::move(ip));
	}
	return result;
}

static void sendInput(const input_t& ip)
{
	SendInput(ip.size(), (LPINPUT)ip.data(), sizeof(INPUT));
}

void mmc::sendClipboard(HWND hWnd)
{
	const auto clp = mmc::getTextFromClipBoard(hWnd);
	sendInput(createINPUT(clp));
}

static void _sendClipboardAsync(HWND hWnd)
{
    const std::chrono::milliseconds timeout(500);
    std::this_thread::sleep_for(timeout);
    mmc::sendClipboard(hWnd);
}

void mmc::sendClipboardAsync(HWND hWnd)
{
    std::thread{ _sendClipboardAsync, hWnd }.detach();
}
