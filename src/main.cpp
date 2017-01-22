#include "exceptions.h"
#include "utils.h"
#include "resource.h"
#include "animation.h"
#include "utils.h"
#include <commctrl.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <cderr.h>
#include <vector>
#include <set>
using namespace std;

struct FILEINFO
{
	wstring		filename;
	wstring		compacted;
	int			width;
	wstring		size;
	Animation*	animation;
};

typedef vector<FILEINFO> FileList;

struct APPLICATION_INFO
{
	HINSTANCE	hInstance;
	HWND		hWnd;

	FileList	files;

	APPLICATION_INFO(HINSTANCE _hInstance)
	{
		hInstance = _hInstance;
		hWnd      = NULL;
	}

	~APPLICATION_INFO()
	{
		for (FileList::const_iterator i = files.begin(); i != files.end(); i++)
		{
			delete i->animation;
		}
		DestroyWindow(hWnd);
	}
};

static void CheckConvert(APPLICATION_INFO* info)
{
	wstring path = GetDlgItemStr(info->hWnd, IDC_EDIT1);
	bool valid = PathIsDirectory(path.c_str()) && ListView_GetItemCount(GetDlgItem(info->hWnd, IDC_LIST1)) > 0;
	EnableWindow(GetDlgItem(info->hWnd, IDC_BUTTON4), valid);
	EnableWindow(GetDlgItem(info->hWnd, IDC_BUTTON5), valid);
	EnableWindow(GetDlgItem(info->hWnd, IDC_BUTTON6), valid);
}

static void DoAddFiles(APPLICATION_INFO* info)
{
	static const int BUFFER_SIZE = 2*1048576;

    wstring filter = LoadString(IDS_FILES_ALA) + wstring(L" (*.ala)\0*.ALA\0", 15)
                   + LoadString(IDS_FILES_ALL) + wstring(L" (*.*)\0*.*\0", 11);

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize  = sizeof(OPENFILENAME);
	ofn.hwndOwner    = info->hWnd;
	ofn.hInstance    = info->hInstance;
    ofn.lpstrFilter  = filter.c_str();
	ofn.nFilterIndex = 0;
	ofn.lpstrFile    = new TCHAR[BUFFER_SIZE]; ofn.lpstrFile[0] = L'\0';
	ofn.nMaxFile     = BUFFER_SIZE - 1;
	ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
	if (GetOpenFileName( &ofn ) == 0)
	{
		if (CommDlgExtendedError() == FNERR_BUFFERTOOSMALL)
		{
			MessageBox(NULL, LoadString(IDS_ERROR_FILE_COUNT).c_str(), NULL, MB_OK | MB_ICONERROR );
		}
		delete[] ofn.lpstrFile;
		return;
	}

	wstring directory = ofn.lpstrFile;
	vector<wstring> filenames;

	TCHAR* str = ofn.lpstrFile + wcslen(ofn.lpstrFile) + 1;
	while (*str != '\0')
	{
		filenames.push_back( directory + L"\\" + str );
		str += wcslen(str) + 1;
	}
	delete[] ofn.lpstrFile;

	if (filenames.size() == 0)
	{
		// User selected only one file, we interpreted it as the directory
		filenames.push_back( directory );
	}

	vector<pair<Animation*,LONGLONG> > animations(filenames.size(), pair<Animation*,LONGLONG>(NULL, 0));
	HWND hList = GetDlgItem(info->hWnd, IDC_LIST1);
	int  count = ListView_GetItemCount(hList);
	try
	{
		size_t i;
		for (i = 0; i < filenames.size(); i++)
		{
			HANDLE hFile = CreateFile(filenames[i].c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				wstring message = LoadString(IDS_ERROR_FILE_OPEN, filenames[i].c_str());
				if (MessageBox(NULL, message.c_str(), NULL, MB_YESNO | MB_ICONWARNING) == IDNO)
				{
					break;
				}
			}

			try
			{
				animations[i].first  = new Animation(hFile);
			}
			catch (wruntime_error&)
			{
				wstring message = LoadString(IDS_ERROR_FILE_FORMAT, filenames[i].c_str());
				MessageBox(NULL, message.c_str(), NULL, MB_OK | MB_ICONWARNING);
				animations[i].first = NULL;
			}

			LARGE_INTEGER size;
			size.LowPart = GetFileSize(hFile, (DWORD*)&size.HighPart);
			animations[i].second = size.QuadPart;

			CloseHandle(hFile);
		}

		if (i == filenames.size())
		{
			for (i = 0; i < filenames.size(); i++)
			{
				if (animations[i].first != NULL)
				{
					// Find right position to insert
					int low = 0, high = (int)info->files.size() - 1;
					while (high >= low)
					{
						int mid = (low + high) / 2;
						int cmp = filenames[i].compare(info->files[mid].filename);
						if (cmp == 0) break;
						if (cmp < 0) high = mid - 1;
						else		 low  = mid + 1;
					}

					if (high < low)
					{
						// Move everything over one position
						info->files.resize(info->files.size() + 1);
						size_t j;
						for (j = info->files.size() - 1; j > (size_t)low; j--)
						{
							info->files[j] = info->files[j-1];
						}

						// Insert it
						FILEINFO& fi = info->files[j];
						fi.filename  = filenames[i];
						fi.animation = animations[i].first;
						fi.width     = 0;

						wchar_t sizebuf[128];
						StrFormatByteSize(animations[i].second, sizebuf, 128);
						fi.size = sizebuf;

						// The file didn't yet exist in the list
						count++;
					}
					else
					{
						// The file already exists in the list. Ignore it
						delete animations[i].first;
					}
					animations[i].first = NULL;
				}
			}
		}
	}
	catch (...)
	{
		for (size_t i = 0; i < animations.size(); i++)
		{
			delete animations[i].first;
		}
		throw;
	}
    ListView_SetItemCountEx(hList, count, 0);
	CheckConvert(info);
}

static void DoRemoveFiles(APPLICATION_INFO* info)
{
	set<int> selected;
	int i = -1;
	HWND hList = GetDlgItem(info->hWnd, IDC_LIST1);
	while ((i = ListView_GetNextItem(hList, i, LVNI_SELECTED)) != -1)
	{
		selected.insert(i);
	}

	int count = 0;
	for (set<int>::const_iterator i = selected.begin(); i != selected.end(); i++, count++)
	{
		delete info->files[*i - count].animation;
		for (size_t j = *i - count; j < info->files.size() - 1; j++)
		{
			info->files[j] = info->files[j+1];
		}
		info->files.resize(info->files.size() - 1);
	}
	ListView_SetItemCount(hList, ListView_GetItemCount(hList) - count);
	CheckConvert(info);
}

static void DoBrowseFolder(APPLICATION_INFO* info)
{
    const wstring title = LoadString(IDS_QUERY_TARGET_FOLDER);

	BROWSEINFO bi;
	bi.hwndOwner      = info->hWnd;
	bi.pidlRoot       = NULL;
	bi.pszDisplayName = NULL;
	bi.lpszTitle      = title.c_str();
	bi.ulFlags		  = BIF_RETURNONLYFSDIRS;
	bi.lpfn           = NULL;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if (pidl != NULL)
	{
		TCHAR path[MAX_PATH];
		if (SHGetPathFromIDList(pidl, path))
		{
			SetWindowText(GetDlgItem(info->hWnd, IDC_EDIT1), path);
		}
	}
}

static void DoConvertFiles(APPLICATION_INFO* info, const Animation::Type type)
{
	wchar_t pathbuf[MAX_PATH];
	wstring path = GetDlgItemStr(info->hWnd, IDC_EDIT1);
	wcsncpy_s(pathbuf, MAX_PATH, path.c_str(), MAX_PATH);

	HWND hButton4  = GetDlgItem(info->hWnd, IDC_BUTTON4);
	HWND hButton5  = GetDlgItem(info->hWnd, IDC_BUTTON5);
	HWND hButton6  = GetDlgItem(info->hWnd, IDC_BUTTON6);
	HWND hProgress = GetDlgItem(info->hWnd, IDC_PROGRESS1);

	static const int NUM_CONTROLS = 9;
	int  Controls[NUM_CONTROLS] = {IDC_LIST1, IDC_BUTTON1, IDC_BUTTON2, IDC_BUTTON3, IDC_BUTTON4, IDC_BUTTON5, IDC_BUTTON6, IDCANCEL, IDC_EDIT1};
	BOOL Enabled [NUM_CONTROLS];

	ShowWindow(hButton4,  SW_HIDE);
	ShowWindow(hButton5,  SW_HIDE);
	ShowWindow(hButton6,  SW_HIDE);
	ShowWindow(hProgress, SW_SHOW);
	for (int i = 0; i < NUM_CONTROLS; i++)
	{
		HWND hControl = GetDlgItem(info->hWnd, Controls[i]);
		Enabled[i] = IsWindowEnabled(hControl);
		EnableWindow(hControl, FALSE);
	}

	HANDLE hFile = NULL;
	try
	{
		size_t count = info->files.size();

		SendMessage(hProgress, PBM_SETRANGE32, 0, count - 1);
		SendMessage(hProgress, PBM_SETPOS,  0, 0);
		SendMessage(hProgress, PBM_SETSTEP, 1, 0);

		for (size_t i = 0; i < count; i++)
		{
			// Convert
			if (!PathAppend(pathbuf, PathFindFileName(info->files[i].filename.c_str())))
			{
				throw wruntime_error(LoadString(IDS_ERROR_PATH_COMPOSE));
			}

			// Determine destination type
			Animation::Type destType = type;
			if (type == Animation::ANIM_NONE)
			{
				destType = (info->files[i].animation->getType() == Animation::ANIM_EAW) ? Animation::ANIM_FOC : Animation::ANIM_EAW;
			}

			hFile = CreateFile(pathbuf, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				throw wruntime_error(LoadString(IDS_ERROR_FILE_CREATE, pathbuf));
			}

			info->files[i].animation->write(hFile, destType);

			CloseHandle(hFile);
			hFile = NULL;

			// Restore path
			PathRemoveFileSpec(pathbuf);

			// Update UI
			SendMessage(hProgress, PBM_STEPIT, 0, 0);
			UpdateWindow(hProgress);
		}
	}
	catch (wruntime_error& e)
	{
		CloseHandle(hFile);
		wstring message = LoadString(IDS_ERROR_CONVERSION, e.what());
		MessageBox(NULL, message.c_str(), NULL, MB_OK | MB_ICONHAND);
	}

	ShowWindow(hProgress, SW_HIDE);
	ShowWindow(hButton4,  SW_SHOW);
	ShowWindow(hButton5,  SW_SHOW);
	ShowWindow(hButton6,  SW_SHOW);
	for (int i = 0; i < NUM_CONTROLS; i++)
	{
		EnableWindow(GetDlgItem(info->hWnd, Controls[i]), Enabled[i]);
	}
	SetFocus(info->hWnd);
}

INT_PTR CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	APPLICATION_INFO* info = (APPLICATION_INFO*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			info = (APPLICATION_INFO*)lParam;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)info);

            wstring title;

			// Initialize list box
			HWND hList = GetDlgItem(hWnd, IDC_LIST1);
			LVCOLUMN column;
			column.mask    = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
            title          = LoadString(IDS_FILENAME);
			column.pszText = (LPWSTR)title.c_str();
			column.cx      = 425;
			ListView_InsertColumn(hList, 0, &column);

            title          = LoadString(IDS_SIZE);
			column.pszText = (LPWSTR)title.c_str();
			column.cx      = 75;
			column.fmt     = LVCFMT_RIGHT;
			ListView_InsertColumn(hList, 1, &column);

            title          = LoadString(IDS_TYPE);
			column.pszText = (LPWSTR)title.c_str();
			column.cx      = 50;
			column.fmt     = LVCFMT_CENTER;
			ListView_InsertColumn(hList, 2, &column);

			ListView_SetExtendedListViewStyle(hList, ListView_GetExtendedListViewStyle(hList) | LVS_EX_FULLROWSELECT);

			return TRUE;
		}

		case WM_CLOSE:
			PostQuitMessage(0);
			break;

		case WM_NOTIFY:
		{
			NMHDR* hdr = (NMHDR*)lParam;
			switch (hdr->code)
			{
				case LVN_GETDISPINFO:
				{
					LV_DISPINFO* di = (LV_DISPINFO*)lParam;
					FILEINFO& fi = info->files[di->item.iItem];

					if (di->item.mask & LVIF_TEXT)
					switch (di->item.iSubItem)
					{
						case 0:
						{
							LVCOLUMN column;
							column.mask = LVCF_WIDTH;
							ListView_GetColumn(di->hdr.hwndFrom, di->item.iSubItem, &column);

							if (fi.width != column.cx)
							{
								fi.compacted = fi.filename;
								HDC hDC = GetDC(di->hdr.hwndFrom);
								SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
								PathCompactPath(hDC, (LPWSTR)fi.compacted.c_str(), column.cx - 20);
								ReleaseDC(di->hdr.hwndFrom, hDC);
								fi.width = column.cx;
							}
							di->item.pszText = (LPWSTR)fi.compacted.c_str();
							break;
						}

						case 1:
							di->item.pszText = (LPWSTR)fi.size.c_str();
							break;

						case 2:
							di->item.pszText = (fi.animation->getType() == Animation::ANIM_EAW) ? L"EaW" : L"FoC";
							break;
					}
					break;
				}

				case LVN_ITEMCHANGED:
				{
					NMLISTVIEW* nmv = (NMLISTVIEW*)lParam; 
					if (nmv->uChanged & LVIF_STATE)
					{
						EnableWindow(GetDlgItem(hWnd, IDC_BUTTON2), ListView_GetSelectedCount(GetDlgItem(hWnd, IDC_LIST1)) > 0);
					}
					break;
				}
			}
			break;
		}

		case WM_COMMAND:
			switch (HIWORD(wParam))
			{
				case BN_CLICKED:
				{
					switch (LOWORD(wParam))
					{
						case IDC_BUTTON1:	DoAddFiles(info);		 break;
						case IDC_BUTTON2:	DoRemoveFiles(info);	 break;
						case IDC_BUTTON3:	DoBrowseFolder(info);	 break;
						case IDC_BUTTON4:	DoConvertFiles(info, Animation::ANIM_NONE); break;
						case IDC_BUTTON5:	DoConvertFiles(info, Animation::ANIM_EAW);  break;
						case IDC_BUTTON6:	DoConvertFiles(info, Animation::ANIM_FOC);  break;
						case IDCANCEL:		PostQuitMessage(0);		 break;
					}
					break;
				}

				case EN_CHANGE:
					CheckConvert(info);
					break;
			}
			break;
	}
	return FALSE;
}

int main(APPLICATION_INFO* info)
{
	ShowWindow(info->hWnd, SW_SHOW);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!IsDialogMessage(info->hWnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int)msg.wParam;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	APPLICATION_INFO info(hInstance);

	int result = -1;
	try
	{
		InitCommonControls();

		// Create window
        if ((info.hWnd = CreateDialogParam(info.hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, MainWindowProc, (LPARAM)&info)) == NULL)
        {
			throw wruntime_error(LoadString(IDS_ERROR_UI_INITIALIZATION));
        }
		result = main(&info);
	}
	catch (wexception& e)
	{
		MessageBox(NULL, e.what(), NULL, MB_OK | MB_ICONERROR );
	}
	catch (exception& e)
	{
		MessageBoxA(NULL, e.what(), NULL, MB_OK | MB_ICONERROR );
	}
	return result;
}