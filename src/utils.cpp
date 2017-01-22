#include "utils.h"
using namespace std;

wstring GetWindowStr(HWND hWnd)
{
	int len = GetWindowTextLength(hWnd);
	wchar_t* buf = new wchar_t[len+1]; buf[len] = L'\0';
	GetWindowText(hWnd, buf, len + 1);
	wstring value = buf;
	delete[] buf;
	return value;
}

wstring GetDlgItemStr(HWND hWnd, int idItem)
{
	return GetWindowStr(GetDlgItem(hWnd, idItem));
}

static wstring FormatString(const wchar_t* format, va_list args)
{
    int      n   = _vscwprintf(format, args);
    wchar_t* buf = new wchar_t[n + 1];
    try
    {
        vswprintf(buf, n + 1, format, args);
        wstring str = buf;
        delete[] buf;
        return str;
    }
    catch (...)
    {
        delete[] buf;
        throw;
    }
}

wstring FormatString(const wchar_t* format, ...)
{
    va_list args;
    va_start(args, format);
    wstring str = FormatString(format, args);
    va_end(args);
    return str;
}

wstring LoadString(UINT id, ...)
{
    int len = 256;
    TCHAR* buf = new TCHAR[len];
    try
    {
        while (::LoadString(NULL, id, buf, len) >= len - 1)
        {
            delete[] buf;
            buf = NULL;
            buf = new TCHAR[len *= 2];
        }
        va_list args;
        va_start(args, id);
        wstring str = FormatString(buf, args);
        va_end(args);
        delete[] buf;
        return str;
    }
    catch (...)
    {
        delete[] buf;
        throw;
    }
}