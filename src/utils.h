#ifndef UTILS_H
#define UTILS_H

#include "types.h"
#include <string>

std::wstring GetWindowStr(HWND hWnd);
std::wstring GetDlgItemStr(HWND hWnd, int idItem);
std::wstring FormatString(const wchar_t* format, ...);
std::wstring LoadString(UINT id, ...);

#endif