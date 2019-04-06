#pragma once

#include "stdafx.h"
#include <vector>
#include <sstream>

namespace mmc {

	using string_t = std::vector<wchar_t>;

	extern string_t getTextFromClipBoard(HWND hWnd);
	extern void sendClipboard(HWND hWnd);

    extern void sendClipboardAsync(HWND hWnd);

}	// mmc

namespace std {

	inline std::wostream& operator << (std::wostream& str, const mmc::string_t& text)
	{
		if (not text.empty()) { str.write(text.data(), text.size() - 1); };
		return str;
	}

}	// std
