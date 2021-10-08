#include "stdafx.h"
#include "BSFMApp.h"
#include "BookmarkManagerDlg.h"

#define CLASSNAME_MainWnd		L"MainWnd"
#define MAINWND_MIN_WIDTH		640
#define MAINWND_MIN_HEIGHT		480

namespace
{
	static const wchar_t* aboutTextFmt =
		L"%s\r\n"
		L"Version %s\r\n"
		L"%s\r\n"
		L"%s"
		;
	INT_PTR CALLBACK _AboutDlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		hwnd;msg;wparam;lparam;
		switch (msg)
		{
		case WM_INITDIALOG:
			{
				wchar_t aboutText[4096] = L"";
				wchar_t* productName = L"BSFM";
				wchar_t* version = L"(unknown)";
				wchar_t* copyright = L"BSPFP";
				wchar_t* description = L"BSPFP";

				std::wstring exeFilePath = getApp()->getCommandLineParser().getExeFilePath();
				DWORD dummy = 0;
				DWORD lenvi = GetFileVersionInfoSize(exeFilePath.c_str(), &dummy);
				if (lenvi > 0)
				{
					uint8_t* vidata = new uint8_t[lenvi];
					if (GetFileVersionInfo(exeFilePath.c_str(), 0, lenvi, vidata))
					{
						UINT len = 0;

						VerQueryValue(vidata, L"\\StringFileInfo\\041204b0\\ProductName", (void**)&productName, &len);
						VerQueryValue(vidata, L"\\StringFileInfo\\041204b0\\ProductVersion", (void**)&version, &len);
						VerQueryValue(vidata, L"\\StringFileInfo\\041204b0\\LegalCopyright", (void**)&copyright, &len);
						VerQueryValue(vidata, L"\\StringFileInfo\\041204b0\\FileDescription", (void**)&description, &len);
					}
					swprintf_s(aboutText, aboutTextFmt, productName, version, copyright, description);
					delete [] vidata;
				}
				else
					swprintf_s(aboutText, aboutTextFmt, productName, version, copyright, description);
				wcscat_s(aboutText, L"\r\n\r\nOptions:\r\n\t/exportLocalize\r\n\t\tExport localization template file.");
				SetDlgItemText(hwnd, IDC_ABOUT, aboutText);
				SetDlgItemText(hwnd, IDCANCEL, getApp()->getLocalizeString(L"About", L"OK").c_str());
				SetDlgItemText(hwnd, IDC_HOMEPAGE, getApp()->getLocalizeString(L"About", L"Homepage").c_str());
			}
			break;
		case WM_COMMAND:
			if (HIWORD(wparam) == BN_CLICKED)
			{
				switch (LOWORD(wparam))
				{
				case IDCANCEL:
					EndDialog(hwnd, IDCANCEL);
					break;
				case IDC_HOMEPAGE:
					ShellExecute(getApp()->getMainWindow(), nullptr, L"http://bspfp.pe.kr/431", L"", L"", SW_SHOWDEFAULT);
					break;
				default:
					return FALSE;
				}
				return TRUE;
			}
			return FALSE;
		default:
			return FALSE;
		}
		return TRUE;
	}
}

MainWnd::MainWnd()
	: _hwnd(nullptr)
	, _menu(nullptr)
	, _mainLayout()
	, _centerSplitView(0.1, 0.9)
	, _upperSplitView(0.1, 0.9)
	, _lowerSplitView(0.1, 0.9)
	, _btSize(1)
	, _shNotifyRegister(0)
	, _selectedViewIndex(0)
{
	static_assert(sizeof(_exViews) / sizeof(_exViews[0]) == 4, "Ooooops");
}

MainWnd::~MainWnd()
{
	_menu = nullptr;
	std::for_each(_menuBitmap.begin(), _menuBitmap.end(), [](HBITMAP& hbmp){
		DeleteObject(hbmp);
	});
	_menuBitmap.clear();
}

bool MainWnd::create(int nShowCmd)
{
	if (!_registerClass())
		return false;
	if (!_createWindow())
		return false;
	SendMessage(_hwnd, WM_SETFONT, (WPARAM)getApp()->getFont(), 0);
	if (nShowCmd == SW_SHOWMINIMIZED)
		ShowWindow(_hwnd, nShowCmd);
	else if (getApp()->getSettings().isMaximized())
		ShowWindow(_hwnd, SW_SHOWMAXIMIZED);
	else
		ShowWindow(_hwnd, SW_SHOW);

	_exViews[0].setFocusToShellView();
	_onViewSelChanged(0);

	PIDLIST_ABSOLUTE pidl = nullptr;
	SHChangeNotifyEntry scne[1];
	if (SUCCEEDED(SHGetKnownFolderIDList(FOLDERID_ComputerFolder, 0, nullptr, &pidl)))
	{
		scne[0].pidl = pidl;
		scne[0].fRecursive = FALSE;
		_shNotifyRegister = SHChangeNotifyRegister(
			_hwnd,
			SHCNRF_ShellLevel | SHCNRF_NewDelivery,
			SHCNE_DRIVEADD | SHCNE_DRIVEREMOVED | SHCNE_MEDIAINSERTED | SHCNE_MEDIAREMOVED | SHCNE_RENAMEFOLDER,
			WM_DRIVE_CHANGED,
			1, scne);
		ILFree(pidl);
	}
	return true;
}

bool MainWnd::preTranslateMessage(MSG* pMsg)
{
	if (_exViews[_selectedViewIndex].preTranslateMessage(pMsg))
		return true;
	switch (pMsg->message)
	{
	case WM_SYSCHAR:			// 이 메시지들은 무조건 메인 윈도우에 전달하게 했다
	case WM_SYSCOLORCHANGE:		// 무슨 문제가 있으려나?
	case WM_SYSDEADCHAR:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		pMsg->hwnd = _hwnd;
		break;
	default:
		break;
	}
	return false;
}

HWND MainWnd::getWindow()
{
	return _hwnd;
}

bool MainWnd::_registerClass()
{
	WNDCLASS wc;
	if (GetClassInfo(getApp()->getHandle(), CLASSNAME_MainWnd, &wc))
		return true;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWnd::_WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = getApp()->getHandle();
	wc.hIcon = getApp()->getIcon();
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);
	wc.lpszClassName = CLASSNAME_MainWnd;
	return (RegisterClass(&wc) != 0);
}

bool MainWnd::_createWindow()
{
	int x = CW_USEDEFAULT;
	int y = 0;
	int width = 0;
	int height = 0;

	RECT rt = getApp()->getSettings().getWindowRect();
	if (rt.right > rt.left && rt.bottom > rt.top)
	{
		x = rt.left;
		y = rt.top;
		width = rt.right - x;
		height = rt.bottom - y;
	}

	width = std::max<int>(width, MAINWND_MIN_WIDTH);
	height = std::max<int>(height, MAINWND_MIN_HEIGHT);
	HWND ret = CreateWindowEx(
		WS_EX_APPWINDOW | WS_EX_OVERLAPPEDWINDOW
		, CLASSNAME_MainWnd
		, getApp()->getName().c_str()
		, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN
		, x, y, width, height
		, nullptr
		, nullptr
		, getApp()->getHandle()
		, this
		);
	if (ret == nullptr)
	{
		DWORD err = GetLastError();
		err = err;
	}
	return (ret != nullptr);
}

void MainWnd::_loadMenu()
{
	_menu = GetMenu(_hwnd);
	if (_menu == nullptr)
		return;

	MENUINFO mi = {0};
	mi.cbSize = sizeof(mi);
	mi.fMask = MIM_STYLE;
	GetMenuInfo(_menu, &mi);
	mi.dwStyle |= MNS_NOTIFYBYPOS;
	SetMenuInfo(_menu, &mi);

	if (getApp()->getCommandLineParser().hasOption(L"exportLocalize"))
	{
		// Export Menu ini
		_exportLocalizeFile();
	}
	else
	{
		// Localize
		std::list<HMENU> menuList;
		menuList.push_back(_menu);
		while (!menuList.empty())
		{
			HMENU menu = menuList.front();
			menuList.pop_front();
			int menucount = GetMenuItemCount(menu);
			for (int i = 0; i < menucount; ++i)
			{
				int len = GetMenuString(menu, i, nullptr, 0, MF_BYPOSITION);
				if (len > 0)
				{
					wchar_t* buf = new wchar_t[len + 1];
					GetMenuString(menu, i, buf, len + 1, MF_BYPOSITION);
					MENUITEMINFO mii;
					memset(&mii, 0, sizeof(mii));
					mii.cbSize = sizeof(mii);
					mii.fMask = MIIM_SUBMENU | MIIM_ID;
					if (GetMenuItemInfo(menu, i, MF_BYPOSITION, &mii))
					{
						if (mii.hSubMenu != nullptr)
						{
							menuList.push_back(mii.hSubMenu);
							ModifyMenu(menu, i, MF_BYPOSITION | MF_STRING, (UINT_PTR)mii.hSubMenu, getApp()->getLocalizeString(L"Menu", buf).c_str());
						}
						else
						{
							ModifyMenu(menu, i, MF_BYPOSITION | MF_STRING, (UINT_PTR)mii.wID, getApp()->getLocalizeString(L"Menu", buf).c_str());
						}
					}
					delete [] buf;
				}
			}
		}
	}
}

void MainWnd::_resetBookmarkMenu()
{
	HMENU menuBookmarkTop = GetSubMenu(_menu, 3);
	HMENU menuBookmark[BSFM_SHORTCUT_BAR_COUNT] = {0};
	for (int i = 0; i < BSFM_SHORTCUT_BAR_COUNT; ++i)
	{
		menuBookmark[i] = GetSubMenu(menuBookmarkTop, 2 + i);
		while (GetMenuItemCount(menuBookmark[i]) > 2)
			DeleteMenu(menuBookmark[i], 2, MF_BYPOSITION);
	}
	std::for_each(_menuBitmap.begin(), _menuBitmap.end(), [](HBITMAP& hbmp){
		DeleteObject(hbmp);
	});
	_menuBitmap.clear();

	for (int i = 0; i < BSFM_SHORTCUT_BAR_COUNT; ++i)
	{
		static const wchar_t* keys[10] = {
			L"\t&1", L"\t&2", L"\t&3", L"\t&4", L"\t&5", L"\t&6", L"\t&7", L"\t&8", L"\t&9", L"\t&0"
		};
		auto infoVector = getApp()->getSettings().getShortcutInfo(i);
		for (size_t iter = 0; iter < infoVector.size(); ++iter)
		{
			auto& info = infoVector[iter];
			HICON hIcon = nullptr;
			std::wstring dispName;
			getShellIcon(info.getTarget(), hIcon, dispName);
			wchar_t buf[1024];
			if (i == 0)
				wcscpy_s(buf, dispName.c_str());
			else
				wcscpy_s(buf, info.name.c_str());
			if (iter < 10)
				wcscat_s(buf, keys[iter]);
			HBITMAP hbmp = iconToBitmap(hIcon, COLOR_MENU);
			if (hIcon != nullptr)
				DestroyIcon(hIcon);
			if (hbmp != nullptr)
				_menuBitmap.push_back(hbmp);
			MENUITEMINFO mii = {0};
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_BITMAP | MIIM_STRING | MIIM_DATA;
			mii.fType = MFT_BITMAP | MFT_STRING;
			mii.dwItemData = MAKELONG((WORD)iter, (WORD)i);
			mii.dwTypeData = buf;
			mii.cch = (UINT)(dispName.length());
			mii.hbmpItem = hbmp;
			InsertMenuItem(menuBookmark[i], GetMenuItemCount(menuBookmark[i]), TRUE, &mii);
		}
	}
}

void MainWnd::_onCreate()
{
	_mainLayout.setWindow(_hwnd);

	_loadMenu();
	for (;;)
	{
		if (!_centerSplitView.create(_hwnd))
			break;
		if (!_upperSplitView.create(_centerSplitView.getWindow()))
			break;
		if (!_lowerSplitView.create(_centerSplitView.getWindow()))
			break;
		double centerPos, upperPos, lowerPos;
		centerPos = upperPos = lowerPos = 0.5;
		getApp()->getSettings().getSplitSize(centerPos, upperPos, lowerPos);
		_centerSplitView.setCurrentPosR(centerPos);
		_upperSplitView.setCurrentPosR(upperPos);
		_lowerSplitView.setCurrentPosR(lowerPos);
		for (int i = 0; i < 4; ++i)
		{
			if (!_exViews[i].create(getWindow(), i))
			{
				PostQuitMessage(E_UNEXPECTED);
				return;
			}
		}

		_upperSplitView.setLeft(_exViews[0].getWindow(), false);
		_upperSplitView.setRight(_exViews[1].getWindow(), false);
		_lowerSplitView.setLeft(_exViews[2].getWindow(), false);
		_lowerSplitView.setRight(_exViews[3].getWindow(), false);

		_centerSplitView.setTop(_upperSplitView.getWindow(), false);
		_centerSplitView.setBottom(_lowerSplitView.getWindow(), false);

		_btSize = _exViews[0].getAddressBarHeight();
		int col[1] = { TLayout::AUTO_SIZE };
		int row[2] = {_btSize, TLayout::AUTO_SIZE };
		_mainLayout.resize(col, row, 0);

		_mainLayout.add(TLayout::Control(&_shortcutBarLayout), 0, 0);
		_mainLayout.add(TLayout::Control(_centerSplitView.getWindow()), 0, 1);

		_upperSplitView.show();
		_lowerSplitView.show();
		_centerSplitView.show();

		bool bTestShorcutBarCreation = true;
		for (int i = 0; i < BSFM_SHORTCUT_BAR_COUNT; ++i)
		{
			ShortcutBar::Ptr pBar(new ShortcutBar(i));
			if (pBar->create(_hwnd))
				_shortcutBars[i] = pBar;
			else
				bTestShorcutBarCreation = false;
		}

		if (bTestShorcutBarCreation)
			_setShortcutBar();

		_refreshSplitMode();

		return;
	}
	PostQuitMessage(E_UNEXPECTED);
}

void MainWnd::_onClosed()
{
	if (_shNotifyRegister != 0)
	{
		SHChangeNotifyDeregister(_shNotifyRegister);
		_shNotifyRegister = 0;
	}

	// 분할 상태 저장
	{
		getApp()->getSettings().setSplitSize(_centerSplitView.getCurrentPosR(), _upperSplitView.getCurrentPosR(), _lowerSplitView.getCurrentPosR());
	}
	// 마지막 위치 저장
	{
		for (int viewIndex = 0; viewIndex < 4; ++viewIndex)
		{
			getApp()->getSettings().setCurrentFolder(viewIndex, _exViews[viewIndex].getParsingName());
		}
	}
	// 윈도우 위치 및 상태 저장
	{
		WINDOWPLACEMENT wp = {0};
		wp.length = sizeof(wp);
		GetWindowPlacement(_hwnd, &wp);

		bool bMaximized = (wp.showCmd == SW_MAXIMIZE || (wp.showCmd == SW_MINIMIZE && (wp.flags & WPF_RESTORETOMAXIMIZED) != 0));
		RECT rc;
		if (bMaximized)
			rc = wp.rcNormalPosition;
		else
			GetWindowRect(_hwnd, &rc);
		getApp()->getSettings().setMaximized(bMaximized);
		getApp()->getSettings().setWindowRect(rc);
	}
	DestroyWindow(_hwnd);
}

void MainWnd::_onViewSelChanged(int viewIndex)
{
	for (int i = 0; i < 4; ++i)
		_exViews[i].setSel(i == viewIndex);
	_selectedViewIndex = viewIndex;
}

void MainWnd::_exportLocalizeFile()
{
	INIFile iniFile;

	// Menu
	{
		std::list<HMENU> menuList;
		menuList.push_back(_menu);
		while (!menuList.empty())
		{
			HMENU menu = menuList.front();
			menuList.pop_front();
			int menucount = GetMenuItemCount(menu);
			for (int i = 0; i < menucount; ++i)
			{
				int len = GetMenuString(menu, i, nullptr, 0, MF_BYPOSITION);
				if (len > 0)
				{
					wchar_t* buf = new wchar_t[len + 1];
					GetMenuString(menu, i, buf, len + 1, MF_BYPOSITION);
					iniFile.set(L"Menu", buf, buf);
					delete [] buf;
				}
				MENUITEMINFO mii;
				memset(&mii, 0, sizeof(mii));
				mii.cbSize = sizeof(mii);
				mii.fMask = MIIM_SUBMENU;
				if (GetMenuItemInfo(menu, i, MF_BYPOSITION, &mii))
				{
					if (mii.hSubMenu != nullptr)
						menuList.push_back(mii.hSubMenu);
				}
			}
		}
	}

	BSFMApp::initLocalize(iniFile);

	wchar_t filepath[1024] = L"localize_xx.ini";
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = _hwnd;
	ofn.hInstance = getApp()->getHandle();
	ofn.lpstrFilter = L"INI Files\0*.ini\0All Files\0*.*\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = filepath;
	ofn.nMaxFile = 1024;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_ENABLESIZING | OFN_EXPLORER | OFN_FORCESHOWHIDDEN | OFN_LONGNAMES | OFN_NONETWORKBUTTON | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = L"ini";
	if (GetSaveFileName(&ofn))
	{
		if (!iniFile.save(ofn.lpstrFile))
			MessageBox(_hwnd, L"Cannot save file", getApp()->getName().c_str(), MB_OK | MB_ICONERROR);
	}
	PostQuitMessage(S_OK);
}

void MainWnd::_setShortcutBar()
{
	MENUITEMINFO mii = {0};
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	int visibleRowCount = 0;

	_shortcutBarLayout.clear();
	_shortcutBarLayout.resize(1, BSFM_SHORTCUT_BAR_COUNT, 0);

	if (GetMenuItemInfo(_menu, IDM_BOOKMARK_1_SHOW, MF_BYCOMMAND, &mii))
	{
		mii.fState |= MFS_CHECKED | MFS_GRAYED;
		mii.fState &= ~MFS_ENABLED;
		SetMenuItemInfo(_menu, IDM_BOOKMARK_1_SHOW, MF_BYCOMMAND, &mii);
	}
	for (int i = 0; i < BSFM_SHORTCUT_BAR_COUNT; ++i)
	{
		bool bShow = _shortcutBars[i]->isShow();
		if (i > 0)
		{
			static_assert(IDM_BOOKMARK_1_SHOW + 1 == IDM_BOOKMARK_2_SHOW, "Ooops");
			static_assert(IDM_BOOKMARK_2_SHOW + 1 == IDM_BOOKMARK_3_SHOW, "Ooops");
			static_assert(IDM_BOOKMARK_3_SHOW + 1 == IDM_BOOKMARK_4_SHOW, "Ooops");
			static_assert(IDM_BOOKMARK_4_SHOW + 1 == IDM_BOOKMARK_5_SHOW, "Ooops");
			if (GetMenuItemInfo(_menu, IDM_BOOKMARK_1_SHOW + i, MF_BYCOMMAND, &mii))
			{
				if (bShow)
					mii.fState |= MFS_CHECKED;
				else
					mii.fState &= ~MFS_CHECKED;
				SetMenuItemInfo(_menu, IDM_BOOKMARK_1_SHOW + i, MF_BYCOMMAND, &mii);
			}
		}
		if (bShow)
		{
			_shortcutBarLayout.setHeight(i, _shortcutBars[0]->getHeight());
			++visibleRowCount;
		}
		else
		{
			_shortcutBarLayout.setHeight(i, 0);
		}
		_shortcutBarLayout.add(TLayout::Control(_shortcutBars[i]->getWindow()), 0, i);
	}
	RECT rect;
	GetClientRect(_hwnd, &rect);
	_mainLayout.setHeight(0, _shortcutBars[0]->getHeight() * (visibleRowCount), &rect);
	_resetBookmarkMenu();
}

void MainWnd::_refreshSplitMode()
{
	switch (getApp()->getSettings().getSplitMode())
	{
	case 1:
		_upperSplitView.showLeft(true);
		_upperSplitView.showRight(false);
		_lowerSplitView.showLeft(false);
		_lowerSplitView.showRight(false);
		_centerSplitView.showTop(true);
		_centerSplitView.showBottom(false);
		CheckMenuItem(_menu, IDM_VIEW_SPLIT_1, MF_BYCOMMAND | MF_CHECKED);
		CheckMenuItem(_menu, IDM_VIEW_SPLIT_2, MF_BYCOMMAND | MF_UNCHECKED);
		CheckMenuItem(_menu, IDM_VIEW_SPLIT_3, MF_BYCOMMAND | MF_UNCHECKED);
		CheckMenuItem(_menu, IDM_VIEW_SPLIT_4, MF_BYCOMMAND | MF_UNCHECKED);
		break;
	case 2:
		_upperSplitView.showLeft(true);
		_upperSplitView.showRight(true);
		_lowerSplitView.showLeft(false);
		_lowerSplitView.showRight(false);
		_centerSplitView.showTop(true);
		_centerSplitView.showBottom(false);
		CheckMenuItem(_menu, IDM_VIEW_SPLIT_1, MF_BYCOMMAND | MF_UNCHECKED);
		CheckMenuItem(_menu, IDM_VIEW_SPLIT_2, MF_BYCOMMAND | MF_CHECKED);
		CheckMenuItem(_menu, IDM_VIEW_SPLIT_3, MF_BYCOMMAND | MF_UNCHECKED);
		CheckMenuItem(_menu, IDM_VIEW_SPLIT_4, MF_BYCOMMAND | MF_UNCHECKED);
		break;
	case 3:
		_upperSplitView.showLeft(true);
		_upperSplitView.showRight(false);
		_lowerSplitView.showLeft(true);
		_lowerSplitView.showRight(false);
		_centerSplitView.showTop(true);
		_centerSplitView.showBottom(true);
		CheckMenuItem(_menu, IDM_VIEW_SPLIT_1, MF_BYCOMMAND | MF_UNCHECKED);
		CheckMenuItem(_menu, IDM_VIEW_SPLIT_2, MF_BYCOMMAND | MF_UNCHECKED);
		CheckMenuItem(_menu, IDM_VIEW_SPLIT_3, MF_BYCOMMAND | MF_CHECKED);
		CheckMenuItem(_menu, IDM_VIEW_SPLIT_4, MF_BYCOMMAND | MF_UNCHECKED);
		break;
	case 4:
		_upperSplitView.showLeft(true);
		_upperSplitView.showRight(true);
		_lowerSplitView.showLeft(true);
		_lowerSplitView.showRight(true);
		_centerSplitView.showTop(true);
		_centerSplitView.showBottom(true);
		CheckMenuItem(_menu, IDM_VIEW_SPLIT_1, MF_BYCOMMAND | MF_UNCHECKED);
		CheckMenuItem(_menu, IDM_VIEW_SPLIT_2, MF_BYCOMMAND | MF_UNCHECKED);
		CheckMenuItem(_menu, IDM_VIEW_SPLIT_3, MF_BYCOMMAND | MF_UNCHECKED);
		CheckMenuItem(_menu, IDM_VIEW_SPLIT_4, MF_BYCOMMAND | MF_CHECKED);
		break;
	default:
		MessageBox(_hwnd, L"Error", getApp()->getName().c_str(), MB_OK | MB_ICONERROR);
		break;
	}
	_upperSplitView.recalculate();
	_lowerSplitView.recalculate();
	_centerSplitView.recalculate();
	switch (getApp()->getSettings().getSplitMode())
	{
	case 1:
		_exViews[0].ensureSelectedItemVisible();
		break;
	case 2:
		_exViews[0].ensureSelectedItemVisible();
		_exViews[1].ensureSelectedItemVisible();
		break;
	case 3:
		_exViews[0].ensureSelectedItemVisible();
		_exViews[2].ensureSelectedItemVisible();
		break;
	case 4:
		_exViews[0].ensureSelectedItemVisible();
		_exViews[1].ensureSelectedItemVisible();
		_exViews[2].ensureSelectedItemVisible();
		_exViews[3].ensureSelectedItemVisible();
		break;
	}
}

void MainWnd::_onNextView()
{
	if (_exViews[_selectedViewIndex].getTreeViewWindow() == GetFocus())
	{
		_exViews[_selectedViewIndex].setFocusToShellView();
	}
	else
	{
		int idx;
		switch (getApp()->getSettings().getSplitMode())
		{
		case 1:
			return;
		case 2:
			idx = (_selectedViewIndex == 1) ? 0 : 1;
			break;
		case 3:
			idx = (_selectedViewIndex == 2) ? 0 : 1;
			break;
		case 4:
		default:
			idx = (_selectedViewIndex + 1) % 4;
			break;
		}
		_exViews[idx].setFocusToShellView();
		_onViewSelChanged(idx);
	}
}

void MainWnd::_onPrevView()
{
	if (_exViews[_selectedViewIndex].getTreeViewWindow() == GetFocus())
	{
		_exViews[_selectedViewIndex].setFocusToShellView();
	}
	else
	{
		int idx;
		switch (getApp()->getSettings().getSplitMode())
		{
		case 1:
			return;
		case 2:
			idx = (_selectedViewIndex == 1) ? 0 : 1;
			break;
		case 3:
			idx = (_selectedViewIndex == 2) ? 0 : 1;
			break;
		case 4:
		default:
			idx = (_selectedViewIndex + 3) % 4;
			break;
		}
		_exViews[idx].setFocusToShellView();
		_onViewSelChanged(idx);
	}
}

void MainWnd::_onHelpAbout()
{
	DialogBox(getApp()->getHandle(), MAKEINTRESOURCE(IDD_ABOUT), _hwnd, _AboutDlgProc);
}

void MainWnd::_onCommandForView(UINT cmd, int viewIndex)
{
	static const wchar_t driveChars[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	switch (cmd)
	{
	case IDM_MOVE_BACK:
		SendMessage(_exViews[viewIndex].getWindow(), WM_VIEW_COMMAND, AddressBar::ABCMD_BACK, 0);
		break;
	case IDM_MOVE_FORWARD:
		SendMessage(_exViews[viewIndex].getWindow(), WM_VIEW_COMMAND, AddressBar::ABCMD_FORWARD, 0);
		break;
	case IDM_MOVE_HOME:
		SendMessage(_exViews[viewIndex].getWindow(), WM_VIEW_COMMAND, AddressBar::ABCMD_HOME, 0);
		break;
	case IDM_MOVE_UP:
		SendMessage(_exViews[viewIndex].getWindow(), WM_VIEW_COMMAND, AddressBar::ABCMD_UP, 0);
		break;
	case IDM_FILE_NEW_FOLDER:
		SendMessage(_exViews[viewIndex].getWindow(), WM_VIEW_COMMAND, AddressBar::ABCMD_NEW_FOLDER, 0);
		break;
	case IDM_VIEW_FORMAT_DETAILS:
		getApp()->getSettings().setViewMode(viewIndex, FVM_DETAILS);
		SendMessage(_exViews[viewIndex].getWindow(), WM_VIEW_SET_VIEWMODE, 0, 0);
		break;
	case IDM_VIEW_FORMAT_ICON:
		getApp()->getSettings().setViewMode(viewIndex, FVM_ICON);
		SendMessage(_exViews[viewIndex].getWindow(), WM_VIEW_SET_VIEWMODE, 0, 0);
		break;
	case IDM_VIEW_FORMAT_LIST:
		getApp()->getSettings().setViewMode(viewIndex, FVM_LIST);
		SendMessage(_exViews[viewIndex].getWindow(), WM_VIEW_SET_VIEWMODE, 0, 0);
		break;
	case IDM_VIEW_FORMAT_CONTENT:
		getApp()->getSettings().setViewMode(viewIndex, FVM_CONTENT);
		SendMessage(_exViews[viewIndex].getWindow(), WM_VIEW_SET_VIEWMODE, 0, 0);
		break;
	case IDM_VIEW_FORMAT_TILE:
		getApp()->getSettings().setViewMode(viewIndex, FVM_TILE);
		SendMessage(_exViews[viewIndex].getWindow(), WM_VIEW_SET_VIEWMODE, 0, 0);
		break;
	case IDM_VIEW_ADDRESS:
		SendMessage(_exViews[viewIndex].getWindow(), WM_VIEW_COMMAND, AddressBar::ABCMD_ADDRESS, 0);
		break;
	case IDM_GO_DRIVE_A: case IDM_GO_DRIVE_B: case IDM_GO_DRIVE_C: case IDM_GO_DRIVE_D:
	case IDM_GO_DRIVE_E: case IDM_GO_DRIVE_F: case IDM_GO_DRIVE_G: case IDM_GO_DRIVE_H:
	case IDM_GO_DRIVE_I: case IDM_GO_DRIVE_J: case IDM_GO_DRIVE_K: case IDM_GO_DRIVE_L:
	case IDM_GO_DRIVE_M: case IDM_GO_DRIVE_N: case IDM_GO_DRIVE_O: case IDM_GO_DRIVE_P:
	case IDM_GO_DRIVE_Q: case IDM_GO_DRIVE_R: case IDM_GO_DRIVE_S: case IDM_GO_DRIVE_T:
	case IDM_GO_DRIVE_U: case IDM_GO_DRIVE_V: case IDM_GO_DRIVE_W: case IDM_GO_DRIVE_X:
	case IDM_GO_DRIVE_Y: case IDM_GO_DRIVE_Z:
		{
			auto cur = getApp()->getSettings().getShortcutInfo(0);
			for (auto iter = cur.begin(); iter != cur.end(); ++iter)
			{
				if (iter->targetPath.at(0) == driveChars[cmd - IDM_GO_DRIVE_A])
				{
					SendMessage(_exViews[viewIndex].getWindow(), WM_SHORTCUT_COMMAND, 0, (LPARAM)&(*iter));
					break;
				}
			}
		}
		break;

	case IDM_FILE_NEW_FILE:
		_exViews[viewIndex].onNewFile();
		break;
	}
}

void MainWnd::_onBookmarkShow(UINT idm)
{
	int height = _mainLayout.getHeight(0);
	int barHeight = _shortcutBars[0]->getHeight();
	MENUITEMINFO mii = {0};
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE;
	if (GetMenuItemInfo(_menu, idm, MF_BYCOMMAND, &mii))
	{
		if ((mii.fState & MFS_CHECKED) != 0)
		{
			_shortcutBarLayout.setHeight(idm - IDM_BOOKMARK_1_SHOW, 0);
			height -= barHeight;
			_shortcutBars[idm - IDM_BOOKMARK_1_SHOW]->show(false);
		}
		else
		{
			_shortcutBarLayout.setHeight(idm - IDM_BOOKMARK_1_SHOW, barHeight);
			height += barHeight;
			_shortcutBars[idm - IDM_BOOKMARK_1_SHOW]->show(true);
		}
		mii.fState ^= MFS_CHECKED;
		SetMenuItemInfo(_menu, idm, MF_BYCOMMAND, &mii);
	}
	RECT rect;
	GetClientRect(_hwnd, &rect);
	_mainLayout.setHeight(0, height, &rect);
}

void MainWnd::_onMenuCommand(HMENU menu, int pos)
{
	MENUITEMINFO mii = {0};
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_DATA;
	GetMenuItemInfo(menu, pos, TRUE, &mii);
	auto v = getApp()->getSettings().getShortcutInfo(HIWORD(mii.dwItemData));
	SendMessage(getApp()->getMainWindow(), WM_SHORTCUT_COMMAND, 0, (LPARAM)&(v[LOWORD(mii.dwItemData)]));
}

void MainWnd::_onSplitMode(int newMode)
{
	int oldMode = getApp()->getSettings().getSplitMode();
	if (oldMode != newMode)
	{
		getApp()->getSettings().setSplitMode(newMode);
		_refreshSplitMode();
		_exViews[0].setFocusToShellView();
		_onViewSelChanged(0);
	}
}

LRESULT CALLBACK MainWnd::_WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	MainWnd* pThis = (MainWnd*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (msg)
	{
	case WM_NCCREATE:
		pThis = (MainWnd*)((CREATESTRUCT*)lparam)->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
		pThis->_hwnd = hwnd;
		return DefWindowProc(hwnd, msg, wparam, lparam);

	case WM_CREATE:
		pThis->_onCreate();
		break;

	case WM_CLOSE:
		pThis->_onClosed();
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lparam)->ptMinTrackSize.x = MAINWND_MIN_WIDTH;
		((MINMAXINFO*)lparam)->ptMinTrackSize.y = MAINWND_MIN_HEIGHT;
		break;

	case WM_SETFONT:
		pThis->_mainLayout.setFont((HFONT)wparam, (BOOL)lparam);
		if (lparam != 0)
			InvalidateRect(hwnd, nullptr, TRUE);
		break;

	case WM_SIZE:
		if (wparam != SIZE_MINIMIZED)
		{
			pThis->_mainLayout.recalculate();
			pThis->_exViews[pThis->_selectedViewIndex].setFocusToShellView();
			pThis->_onViewSelChanged(pThis->_selectedViewIndex);
		}
		break;

	case WM_APPCOMMAND:
		for (int i = 0; i < 4; ++i)
		{
			if (pThis->_exViews[i].onAppCommand((HWND)wparam, GET_APPCOMMAND_LPARAM(lparam)))
				return 0;
		}
		return DefWindowProc(hwnd, msg, wparam, lparam);

	case WM_MENUCOMMAND:
		{
			auto pos = (int)wparam;
			auto menu = (HMENU)lparam;
			UINT idm = GetMenuItemID(menu, pos);
			if (idm > 0)
				SendMessage(hwnd, WM_COMMAND, idm, 0);
			else
				pThis->_onMenuCommand(menu, pos);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wparam))
		{
		case IDM_NEXT_VIEW:
			pThis->_onNextView();
			break;
		case IDM_PREV_VIEW:
			pThis->_onPrevView();
			break;

		case IDM_FILE_EXIT:
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;

		case IDM_HELP_ABOUT:
			pThis->_onHelpAbout();
			break;

		case IDM_MOVE_BACK:
		case IDM_MOVE_FORWARD:
		case IDM_MOVE_HOME:
		case IDM_MOVE_UP:
		case IDM_FILE_NEW_FOLDER:
		case IDM_FILE_NEW_FILE:
		case IDM_VIEW_FORMAT_DETAILS:
		case IDM_VIEW_FORMAT_ICON:
		case IDM_VIEW_FORMAT_LIST:
		case IDM_VIEW_FORMAT_CONTENT:
		case IDM_VIEW_FORMAT_TILE:
		case IDM_VIEW_ADDRESS:
			pThis->_onCommandForView(LOWORD(wparam), pThis->_selectedViewIndex);
			break;

		case IDM_BOOKMARK_MANAGER:
			{
				BookmarkManagerDlg dlg;
				if (dlg.show(hwnd))
				{
					for (int i = 1; i < BSFM_SHORTCUT_BAR_COUNT; ++i)
					{
						getApp()->getSettings().setShortcutInfo(i, dlg.getInfo(i));
						pThis->_shortcutBars[i]->refresh();
						pThis->_resetBookmarkMenu();
					}
				}
			}
			break;

		case IDM_BOOKMARK_2_SHOW:
		case IDM_BOOKMARK_3_SHOW:
		case IDM_BOOKMARK_4_SHOW:
		case IDM_BOOKMARK_5_SHOW:
			pThis->_onBookmarkShow(LOWORD(wparam));
			break;

		case IDM_GO_DRIVE_A: case IDM_GO_DRIVE_B: case IDM_GO_DRIVE_C: case IDM_GO_DRIVE_D:
		case IDM_GO_DRIVE_E: case IDM_GO_DRIVE_F: case IDM_GO_DRIVE_G: case IDM_GO_DRIVE_H:
		case IDM_GO_DRIVE_I: case IDM_GO_DRIVE_J: case IDM_GO_DRIVE_K: case IDM_GO_DRIVE_L:
		case IDM_GO_DRIVE_M: case IDM_GO_DRIVE_N: case IDM_GO_DRIVE_O: case IDM_GO_DRIVE_P:
		case IDM_GO_DRIVE_Q: case IDM_GO_DRIVE_R: case IDM_GO_DRIVE_S: case IDM_GO_DRIVE_T:
		case IDM_GO_DRIVE_U: case IDM_GO_DRIVE_V: case IDM_GO_DRIVE_W: case IDM_GO_DRIVE_X:
		case IDM_GO_DRIVE_Y: case IDM_GO_DRIVE_Z:
			static_assert(IDM_GO_DRIVE_A + 1 == IDM_GO_DRIVE_B, "Oooops"); static_assert(IDM_GO_DRIVE_B + 1 == IDM_GO_DRIVE_C, "Oooops");
			static_assert(IDM_GO_DRIVE_C + 1 == IDM_GO_DRIVE_D, "Oooops"); static_assert(IDM_GO_DRIVE_D + 1 == IDM_GO_DRIVE_E, "Oooops");
			static_assert(IDM_GO_DRIVE_E + 1 == IDM_GO_DRIVE_F, "Oooops"); static_assert(IDM_GO_DRIVE_F + 1 == IDM_GO_DRIVE_G, "Oooops");
			static_assert(IDM_GO_DRIVE_G + 1 == IDM_GO_DRIVE_H, "Oooops"); static_assert(IDM_GO_DRIVE_H + 1 == IDM_GO_DRIVE_I, "Oooops");
			static_assert(IDM_GO_DRIVE_I + 1 == IDM_GO_DRIVE_J, "Oooops"); static_assert(IDM_GO_DRIVE_J + 1 == IDM_GO_DRIVE_K, "Oooops");
			static_assert(IDM_GO_DRIVE_K + 1 == IDM_GO_DRIVE_L, "Oooops"); static_assert(IDM_GO_DRIVE_L + 1 == IDM_GO_DRIVE_M, "Oooops");
			static_assert(IDM_GO_DRIVE_M + 1 == IDM_GO_DRIVE_N, "Oooops"); static_assert(IDM_GO_DRIVE_N + 1 == IDM_GO_DRIVE_O, "Oooops");
			static_assert(IDM_GO_DRIVE_O + 1 == IDM_GO_DRIVE_P, "Oooops"); static_assert(IDM_GO_DRIVE_P + 1 == IDM_GO_DRIVE_Q, "Oooops");
			static_assert(IDM_GO_DRIVE_Q + 1 == IDM_GO_DRIVE_R, "Oooops"); static_assert(IDM_GO_DRIVE_R + 1 == IDM_GO_DRIVE_S, "Oooops");
			static_assert(IDM_GO_DRIVE_S + 1 == IDM_GO_DRIVE_T, "Oooops"); static_assert(IDM_GO_DRIVE_T + 1 == IDM_GO_DRIVE_U, "Oooops");
			static_assert(IDM_GO_DRIVE_U + 1 == IDM_GO_DRIVE_V, "Oooops"); static_assert(IDM_GO_DRIVE_V + 1 == IDM_GO_DRIVE_W, "Oooops");
			static_assert(IDM_GO_DRIVE_W + 1 == IDM_GO_DRIVE_X, "Oooops"); static_assert(IDM_GO_DRIVE_X + 1 == IDM_GO_DRIVE_Y, "Oooops");
			static_assert(IDM_GO_DRIVE_Y + 1 == IDM_GO_DRIVE_Z, "Oooops");
			pThis->_onCommandForView(LOWORD(wparam), pThis->_selectedViewIndex);
			break;

		case IDM_SET_HOME:
			{
				std::wstring const & normalName = pThis->_exViews[pThis->_selectedViewIndex].getNormalName();
				std::wstring const & parsingName = pThis->_exViews[pThis->_selectedViewIndex].getParsingName();
				wchar_t prompt[1024];
				swprintf_s(prompt, getApp()->getLocalizeString(L"Application", L"SetHomePrompt").c_str(), normalName.c_str());
				if (MessageBox(hwnd, prompt, getApp()->getName().c_str(), MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					getApp()->getSettings().setHomeFolder(parsingName);
				}
			}
			break;

		case IDM_VIEW_SPLIT_1:
			pThis->_onSplitMode(1);
			break;

		case IDM_VIEW_SPLIT_2:
			pThis->_onSplitMode(2);
			break;

		case IDM_VIEW_SPLIT_3:
			pThis->_onSplitMode(3);
			break;

		case IDM_VIEW_SPLIT_4:
			pThis->_onSplitMode(4);
			break;

		case IDM_FINDFILE:
			{
				std::wstring destDir = pThis->_exViews[pThis->_selectedViewIndex].getParsingName();
				if (isFolder(destDir))
					getApp()->showFindFileDlg(destDir);
				else
					getApp()->showFindFileDlg(L"C:\\");
			}
			break;

		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		break;

	case WM_VIEW_SEL_CHANGED:
		pThis->_exViews[(int)wparam].setFocusToShellView();
		pThis->_onViewSelChanged((int)wparam);
		break;

	case WM_SHORTCUT_COMMAND:
		SendMessage(pThis->_exViews[pThis->_selectedViewIndex].getWindow(), WM_SHORTCUT_COMMAND, wparam, lparam);
		break;

	case WM_DRIVE_CHANGED:
		{
			PIDLIST_ABSOLUTE* ppidl = nullptr;
			LONG event = 0;
			HANDLE hlock = SHChangeNotification_Lock((HANDLE)wparam, (DWORD)lparam, &ppidl, &event);
			if (hlock != nullptr)
			{
				IShellItem* psi = nullptr;
				if (SUCCEEDED(SHCreateItemFromIDList(ppidl[0], IID_PPV_ARGS(&psi))))
				{
					wchar_t* name = nullptr;
					if (SUCCEEDED(psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &name)))
					{
						if (event == SHCNE_DRIVEADD)
						{
							getApp()->getSettings().addShortcutInfo(0, ShortcutInfo(L"", name, L"", L"", false));
						}
						else if (event == SHCNE_DRIVEREMOVED)
						{
							getApp()->getSettings().removeShortcutInfo(0, ShortcutInfo(L"", name, L"", L"", false));
						}
						CoTaskMemFree(name);
						pThis->_shortcutBars[0]->refresh();
						pThis->_resetBookmarkMenu();
					}
					psi->Release();
				}
				SHChangeNotification_Unlock(hlock);
			}
			for (int i = 0; i < 4; ++i)
				pThis->_exViews[i].validateBrowsingPath();
		}
		break;

	case WM_RESET_BOOKMARK_MENU:
		pThis->_resetBookmarkMenu();
		break;

	case WM_SETFOCUS:
		pThis->_exViews[pThis->_selectedViewIndex].setFocusToShellView();
		break;

	case WM_SET_TITLE:
		{
			const wchar_t* name = (const wchar_t*)wparam;
			if (name == nullptr || wcslen(name) < 0)
			{
				SetWindowText(pThis->_hwnd, getApp()->getName().c_str());
			}
			else
			{
				wchar_t title[1024];
				swprintf_s(title, L"%s - %s", name, getApp()->getName().c_str());
				SetWindowText(pThis->_hwnd, title);
			}
		}
		break;

	case WM_GET_SELECTED_VIEW_INDEX:
		return pThis->_selectedViewIndex;

	case WM_ACTIVATE_BSFM:
		{
			WINDOWPLACEMENT wp = {0};
			wp.length = sizeof(wp);
			GetWindowPlacement(hwnd, &wp);
			switch (wp.showCmd)
			{
			case SW_HIDE:
				ShowWindow(hwnd, SW_SHOW);
				break;
			case SW_MINIMIZE:
			case SW_SHOWMINIMIZED:
			case SW_SHOWMINNOACTIVE:
				ShowWindow(hwnd, SW_RESTORE);
				break;
			default:
				break;
			}
			SetForegroundWindow(hwnd);
		}
		break;

	case WM_FIND_FILE_GOTO:
		{
			auto folder = (const wchar_t*)wparam;
			auto filename = (const wchar_t*)lparam;
			pThis->_exViews[pThis->_selectedViewIndex].go(folder, filename);
		}
		break;

	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}