#include "stdafx.h"
#include "BSFMApp.h"
#include "AddBookmarkDlg.h"

AddBookmarkDlg::AddBookmarkDlg(ShortcutInfo const & info)
	: _info(info)
	, _hwnd(nullptr)
{
}

bool AddBookmarkDlg::show(HWND hwndParent)
{
	return (IDOK == DialogBoxParam(getApp()->getHandle(), MAKEINTRESOURCE(IDD_ADD_BOOKMARK), hwndParent, AddBookmarkDlg::_DlgProc, (LPARAM)this));
}

ShortcutInfo const & AddBookmarkDlg::getInfo()
{
	return _info;
}

void AddBookmarkDlg::_onInitDialog()
{
	_mainLayout.setWindow(_hwnd);

	SendMessage(_hwnd, WM_SETFONT, (WPARAM)getApp()->getFont(), TRUE);
	SendMessage(_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)getApp()->getIcon());
	SendMessage(_hwnd, WM_SETICON, ICON_BIG, (LPARAM)getApp()->getIcon());
	SetClassLong(_hwnd, GCL_STYLE, GetClassLong(_hwnd, GCL_STYLE) | CS_HREDRAW | CS_VREDRAW);

	HWND hwndLabelName				= GetDlgItem(_hwnd, IDC_LABEL_NAME);
	HWND hwndName					= GetDlgItem(_hwnd, IDC_NAME);
	HWND hwndLabelTarget			= GetDlgItem(_hwnd, IDC_LABEL_TARGET);
	HWND hwndTarget					= GetDlgItem(_hwnd, IDC_TARGET);
	HWND hwndLabelParams			= GetDlgItem(_hwnd, IDC_LABEL_PARAMS);
	HWND hwndParams					= GetDlgItem(_hwnd, IDC_PARAMS);
	HWND hwndLabelStartupFolder		= GetDlgItem(_hwnd, IDC_LABEL_STARTUP_FOLDER);
	HWND hwndStartupFolder			= GetDlgItem(_hwnd, IDC_STARTUP_FOLDER);
	HWND hwndStartupFolderBrowse	= GetDlgItem(_hwnd, IDC_STARTUP_FOLDER_BROWSE);
	HWND hwndCheckAdmin				= GetDlgItem(_hwnd, IDC_CHECK_ADMIN);
	HWND hwndOK						= GetDlgItem(_hwnd, IDOK);
	HWND hwndCancel					= GetDlgItem(_hwnd, IDCANCEL);

	SetWindowText(_hwnd, getApp()->getLocalizeString(L"AddBookmark", L"Title").c_str());
	SetWindowText(hwndLabelName, getApp()->getLocalizeString(L"BookmarkManager", L"Name").c_str());
	SetWindowText(hwndLabelTarget, getApp()->getLocalizeString(L"BookmarkManager", L"Target").c_str());
	SetWindowText(hwndLabelParams, getApp()->getLocalizeString(L"BookmarkManager", L"Params").c_str());
	SetWindowText(hwndLabelStartupFolder, getApp()->getLocalizeString(L"BookmarkManager", L"StartupFolder").c_str());
	SetWindowText(hwndCheckAdmin, getApp()->getLocalizeString(L"BookmarkManager", L"CheckAdmin").c_str());
	SetWindowText(hwndOK, getApp()->getLocalizeString(L"AddBookmark", L"OK").c_str());
	SetWindowText(hwndCancel, getApp()->getLocalizeString(L"AddBookmark", L"Cancel").c_str());

	int labelSize;
	int btSize;
	{
		SendMessage(hwndTarget, WM_SETFONT, (WPARAM)getApp()->getFont(), TRUE);
		labelSize = getApp()->getFontHeight();
		btSize = labelSize + 12;
	}

	{
		RECT rectCancel;
		GetWindowRect(hwndCancel, &rectCancel);
		int widthCancel = rectCancel.right - rectCancel.left;
		int heightCancel = rectCancel.bottom - rectCancel.top;
		_okCancelLayout.add(FLayout::Control(hwndCancel), FLayout::BOTTOM_RIGHT, 0, 0, widthCancel, heightCancel);
		RECT rectOK;
		GetWindowRect(hwndOK, &rectOK);
		int widthOK = rectOK.right - rectOK.left;
		int heightOK = rectOK.bottom - rectOK.top;
		_okCancelLayout.add(FLayout::Control(hwndOK), FLayout::BOTTOM_RIGHT, widthCancel + 5, 0, widthOK, heightOK);
	}

	{
		int col[] = {TLayout::AUTO_SIZE, 3, btSize};
		int row[] = {
			labelSize, btSize, 3,		// 이름
			labelSize, btSize, 3,		// 대상
			labelSize, btSize, 3,		// 명령 인수
			labelSize * 2, btSize, 3,	// 시작 위치
			btSize,						// 관리자 권한 체크
			TLayout::AUTO_SIZE,			// 여백
			btSize						// 확인/취소
		};
		_centerLayout.resize(col, row, 0);
		int rIdx = 0;
		_centerLayout.add(TLayout::Control(hwndLabelName), 0, rIdx, 3);				rIdx += 1;
		_centerLayout.add(TLayout::Control(hwndName), 0, rIdx, 3);					rIdx += 2;
		_centerLayout.add(TLayout::Control(hwndLabelTarget), 0, rIdx, 3);			rIdx += 1;
		_centerLayout.add(TLayout::Control(hwndTarget), 0, rIdx, 3);				rIdx += 2;
		_centerLayout.add(TLayout::Control(hwndLabelParams), 0, rIdx, 3);			rIdx += 1;
		_centerLayout.add(TLayout::Control(hwndParams), 0, rIdx, 3);				rIdx += 2;
		_centerLayout.add(TLayout::Control(hwndLabelStartupFolder), 0, rIdx, 3);	rIdx += 1;
		_centerLayout.add(TLayout::Control(hwndStartupFolder), 0, rIdx);
		_centerLayout.add(TLayout::Control(hwndStartupFolderBrowse), 2, rIdx);		rIdx += 2;
		_centerLayout.add(TLayout::Control(hwndCheckAdmin), 0, rIdx, 3);			rIdx += 2;
		_centerLayout.add(TLayout::Control(&_okCancelLayout), 0, rIdx, 3);			rIdx += 1;
	}

	{
		int col[] = {5, TLayout::AUTO_SIZE, 5};		// 좌우 여백
		int row[] = {5, TLayout::AUTO_SIZE, 5};		// 상하 여백
		_mainLayout.resize(col, row, 0);
		_mainLayout.add(TLayout::Control(&_centerLayout), 1, 1);
	}

	_mainLayout.setFont(getApp()->getFont(), FALSE);
	_mainLayout.recalculate();
	_updateData(false, _info);
}

void AddBookmarkDlg::_onStartupFolderBrowse()
{
	wchar_t dispName[MAX_PATH];
	std::wstring title(getApp()->getLocalizeString(L"BookmarkManager", L"StartupFolderBrowse"));
	HWND hwndStartupFolder = GetDlgItem(_hwnd, IDC_STARTUP_FOLDER);
	std::wstring curVal;
	getWindowText(hwndStartupFolder, curVal);
	PIDLIST_ABSOLUTE pidl = browseForFolder(_hwnd, dispName, title.c_str(), curVal.c_str(), true);
	if (pidl != nullptr)
	{
		std::wstring parsingName;
		idListToString(pidl, &parsingName);
		ILFree(pidl);

		SetWindowText(hwndStartupFolder, parsingName.c_str());
		SetFocus(hwndStartupFolder);
	}
}

bool AddBookmarkDlg::_updateData(bool bSaveValidate, ShortcutInfo& info)
{
	HWND hwndName					= GetDlgItem(_hwnd, IDC_NAME);
	HWND hwndTarget					= GetDlgItem(_hwnd, IDC_TARGET);
	HWND hwndParams					= GetDlgItem(_hwnd, IDC_PARAMS);
	HWND hwndStartupFolder			= GetDlgItem(_hwnd, IDC_STARTUP_FOLDER);
	HWND hwndCheckAdmin				= GetDlgItem(_hwnd, IDC_CHECK_ADMIN);

	if (!bSaveValidate)
	{
		SetWindowText(hwndName, info.name.c_str());
		SetWindowText(hwndTarget, info.targetPath.c_str());
		SetWindowText(hwndParams, info.params.c_str());
		SetWindowText(hwndStartupFolder, info.startupFolder.c_str());
		SendMessage(hwndCheckAdmin, BM_SETCHECK, (info.checkAdmin) ? BST_CHECKED : BST_UNCHECKED, 0);
		return true;
	}
	else
	{
		bool bValid = true;
		std::wstring name, target, params, startupfolder;
		bool checkAdmin;
		getWindowText(hwndName, name);
		getWindowText(hwndTarget, target);
		getWindowText(hwndParams, params);
		getWindowText(hwndStartupFolder, startupfolder);
		checkAdmin = (SendMessage(hwndCheckAdmin, BM_GETCHECK, 0, 0) == BST_CHECKED);
		if (target.length() > 0)
		{
			if (isFile(target) || isFile(findPath(target)))
			{
				if (startupfolder.length() > 0)
				{
					if (!isFolder(startupfolder))
					{
						_errorBox(hwndStartupFolder, L"Error_StartupFolder");
						bValid = false;
					}
					if (*(startupfolder.rbegin()) != L'\\')
					{
						startupfolder += L'\\';
						SetWindowText(hwndStartupFolder, startupfolder.c_str());
					}
				}
			}
			else
			{
				bool bExist = false;
				IShellItem* psi = parsingNameToShellItem(target);
				if (psi != nullptr)
				{
					bExist = true;
					psi->Release();
				}
				else
				{
					psi = parsingNameToShellItem(findPath(target));
					if (psi != nullptr)
					{
						bExist = true;
						psi->Release();
					}
				}
				if (bExist)
				{
					params.clear();
					startupfolder.clear();
					checkAdmin = false;

					SetWindowText(hwndParams, params.c_str());
					SetWindowText(hwndStartupFolder, startupfolder.c_str());
					SendMessage(hwndCheckAdmin, BM_SETCHECK, (checkAdmin) ? BST_CHECKED : BST_UNCHECKED, 0);
				}
				else
				{
					_errorBox(hwndStartupFolder, L"Error_InvalidTarget");
					bValid = false;
				}
			}

			if (bValid && name.length() < 1)
			{
				HICON tmp = nullptr;
				if (isFolder(target) || isFile(target))
					getShellIcon(target, tmp, name);
				else
					getShellIcon(findPath(target), tmp, name);
				if (tmp != nullptr)
					DestroyIcon(tmp);
				SetWindowText(hwndName, name.c_str());
			}
		}
		else
		{
			_errorBox(hwndStartupFolder, L"Error_EmptyTarget");
			bValid = false;
		}
		if (bValid)
		{
			info.name = name;
			info.targetPath = target;
			info.params = params;
			info.startupFolder = startupfolder;
			info.checkAdmin = checkAdmin;
		}
		return bValid;
	}
}

void AddBookmarkDlg::_errorBox(HWND hwnd, std::wstring const & msg)
{
	MessageBox(_hwnd, getApp()->getLocalizeString(L"BookmarkManager", msg).c_str(), getApp()->getName().c_str(), MB_OK | MB_ICONERROR);
	SetFocus(hwnd);
}

INT_PTR CALLBACK AddBookmarkDlg::_DlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	AddBookmarkDlg* pThis = (AddBookmarkDlg*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (msg)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lparam);
		pThis = (AddBookmarkDlg*)lparam;
		pThis->_hwnd = hwnd;
		pThis->_onInitDialog();
		break;

	case WM_COMMAND:
		if (HIWORD(wparam) == BN_CLICKED)
		{
			switch (LOWORD(wparam))
			{
			case IDOK:
				{
					ShortcutInfo info;
					if (pThis->_updateData(true, info))
					{
						pThis->_info = info;
						EndDialog(hwnd, IDOK);
					}
				}
				break;

			case IDCANCEL:
				EndDialog(hwnd, IDCANCEL);
				break;

			case IDC_STARTUP_FOLDER_BROWSE:
				pThis->_onStartupFolderBrowse();
				break;

			default:
				return FALSE;
			}
		}
		else if (HIWORD(wparam) == EN_SETFOCUS)
		{
			switch (LOWORD(wparam))
			{
			case IDC_NAME:
			case IDC_TARGET:
			case IDC_PARAMS:
			case IDC_STARTUP_FOLDER:
				PostMessage((HWND)lparam, EM_SETSEL, 0, -1);
				break;
			default:
				return FALSE;
			}
		}
		else
		{
			return FALSE;
		}
		break;

	case WM_SIZE:
		if (wparam != SIZE_MINIMIZED)
			pThis->_mainLayout.recalculate();
		break;

	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lparam)->ptMinTrackSize.x = 400;
		((MINMAXINFO*)lparam)->ptMinTrackSize.y = 270;
		break;

	default:
		return FALSE;
	}
	return TRUE;
}