#include "stdafx.h"
#include "BSFMApp.h"
#include "ExplorerView.h"

#define CLASSNAME_ExplorerView		L"ExplorerView"

ExplorerView::ExplorerView()
	: _hwnd(nullptr)
	, _bFileSystemFolder(false)
{
}

bool ExplorerView::create(HWND hwndParent, int viewIndex)
{
	if (hwndParent == nullptr || !IsWindow(hwndParent))
		return nullptr;

	WNDCLASS wc;
	if (!GetClassInfo(getApp()->getHandle(), CLASSNAME_ExplorerView, &wc))
	{
		wc.style = CS_VREDRAW | CS_HREDRAW;
		wc.lpfnWndProc = ExplorerView::_WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = getApp()->getHandle();
		wc.hIcon = nullptr;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = CLASSNAME_ExplorerView;
		if (RegisterClass(&wc) == 0)
			return false;
	}

	_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, CLASSNAME_ExplorerView, L"", WS_CHILD | WS_CLIPCHILDREN, 0, 0, 0, 0, hwndParent, nullptr, getApp()->getHandle(), this);
	if (_hwnd == nullptr)
		return false;

	_layout.setWindow(_hwnd);

	if (!_bar.create(_hwnd))
		return false;

	if (!_view.create(_hwnd, viewIndex))
		return false;

	int col[] = {2, TLayout::AUTO_SIZE, 2};
	int row[] = {2, _bar.getHeight(), 2, TLayout::AUTO_SIZE, 2};
	_layout.resize(col, row, 0);

	_layout.add(TLayout::Control(_bar.getWindow()), 1, 1);
	_layout.add(TLayout::Control(_view.getWindow()), 1, 3);

	SendMessage(_hwnd, WM_SETFONT, (WPARAM)getApp()->getFont(), 0);
	ShowWindow(_hwnd, SW_SHOW);
	return true;
}

bool ExplorerView::preTranslateMessage(MSG* pMsg)
{
	if (WM_KEYFIRST <= pMsg->message && pMsg->message <= WM_KEYLAST)
	{
		if (_view.isChild(pMsg->hwnd) && _view.preTranslateMessage(pMsg))
		{
			return true;
		}
		else if (_bar.preTranslateMessage(pMsg))
		{
			return true;
		}
	}

	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE && IsChild(_bar.getWindow(), pMsg->hwnd))
	{
		setFocusToShellView();
		return true;
	}

	return false;
}

void ExplorerView::ensureSelectedItemVisible()
{
	_view.ensureSelectedItemVisible();
}

void ExplorerView::setFocusToShellView() const
{
	_view.setFocusToShellView();
}

void ExplorerView::setSel(bool bSel)
{
	RECT rect;
	GetClientRect(_hwnd, &rect);

	HBRUSH hbr;
	if (bSel)
	{
		hbr = GetSysColorBrush(COLOR_HIGHLIGHT);
		SendMessage(getApp()->getMainWindow(), WM_SET_TITLE, (WPARAM)_normalName.c_str(), 0);
	}
	else
	{
		hbr = GetSysColorBrush(COLOR_3DFACE);
	}
	HDC hdc = GetDC(_hwnd);
	if (hdc != nullptr)
	{
		FillRect(hdc, &rect, hbr);
		ReleaseDC(_hwnd, hdc);
	}
}

void ExplorerView::validateBrowsingPath()
{
	IShellItem* psi = parsingNameToShellItem(_parsingName);
	if (psi != nullptr)
	{
		psi->Release();
		return;
	}
	wchar_t* name = nullptr;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &name)))
	{
		_view.browseTo(name);
		CoTaskMemFree(name);
	}
}

void ExplorerView::go(const wchar_t* folder, const wchar_t* filename)
{
	if (_view.browseTo(folder))
	{
		std::wstring fullpath = folder;
		fullpath.append(filename);
		SetFocus(_view.getViewWindow());
		IShellItem* psi = parsingNameToShellItem(fullpath);
		if (psi != nullptr)
		{
			_view.selectName(psi);
			psi->Release();
			_forSelectPath = fullpath;
		}
	}
	else
	{
		_bar.setAddress(_editName);
	}
}

void ExplorerView::onNewFile()
{
	if (_bFileSystemFolder)
	{
		wchar_t path[MAX_PATH];
		int i = 1;
		swprintf_s(path, L"%s\\%s.txt", _parsingName.c_str(), getApp()->getLocalizeString(L"Application", L"NewFileName").c_str());
		for (;;)
		{
			int file = 0;
			int ret = _wsopen_s(&file, path, _O_BINARY | _O_CREAT | _O_EXCL | _O_RDWR, _SH_DENYRW, _S_IREAD | _S_IWRITE);
			if (ret == 0)
			{
				_close(file);
				break;
			}
			else if (ret == EEXIST)
			{
				swprintf_s(path, L"%s\\%s (%d).txt", _parsingName.c_str(), getApp()->getLocalizeString(L"Application", L"NewFileName").c_str(), i++);
			}
			else
			{
				wchar_t msg[4096];
				swprintf_s(msg, getApp()->getLocalizeString(L"Application", L"Error_NewFile").c_str(), path, errno);
				MessageBox(getApp()->getMainWindow(), msg, getApp()->getName().c_str(), MB_OK | MB_ICONERROR);
				return;
			}
		}
		IShellItem* psi = parsingNameToShellItem(path);
		if (psi != nullptr)
		{
			_view.editName(psi);
			psi->Release();
		}
	}
}

bool ExplorerView::onAppCommand(HWND hwnd, UINT cmd)
{
	if (IsChild(_hwnd, hwnd))
		return _view.onAppCommand(cmd);
	else
		return false;
}

HWND ExplorerView::getWindow() const
{
	return _hwnd;
}

HWND ExplorerView::getTreeViewWindow() const
{
	return _view.getTreeWindow();
}

int ExplorerView::getAddressBarHeight() const
{
	return _bar.getHeight();
}

std::wstring const & ExplorerView::getParsingName() const
{
	return _parsingName;
}

std::wstring const & ExplorerView::getNormalName() const
{
	return _normalName;
}

void ExplorerView::_onChangedFolderView(PCIDLIST_ABSOLUTE pidl)
{
	idListToString(pidl, &_parsingName, &_editName, &_normalName, &_bFileSystemFolder);
	_bar.setAddress(_editName);
	if (_view.getViewIndex() == SendMessage(getApp()->getMainWindow(), WM_GET_SELECTED_VIEW_INDEX, 0, 0))
		SendMessage(getApp()->getMainWindow(), WM_SET_TITLE, (WPARAM)_normalName.c_str(), 0);
	IShellItem* psi = parsingNameToShellItem(_forSelectPath);
	if (psi != nullptr)
	{
		_view.selectName(psi);
		psi->Release();
	}
	_forSelectPath.clear();
}

LRESULT CALLBACK ExplorerView::_WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	ExplorerView* pThis = (ExplorerView*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (msg)
	{
	case WM_NCCREATE:
		pThis = (ExplorerView*)((CREATESTRUCT*)lparam)->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
		pThis->_hwnd = hwnd;
		return DefWindowProc(hwnd, msg, wparam, lparam);

	case WM_DESTROY:
		pThis->_view.destroy();
		break;

	case WM_SETFONT:
		pThis->_layout.setFont((HFONT)wparam, (BOOL)lparam);
		if (lparam != 0)
			InvalidateRect(hwnd, nullptr, TRUE);
		break;

	case WM_SIZE:
		if (wparam != SIZE_MINIMIZED)
			pThis->_layout.recalculate();
		break;

	case WM_ERASEBKGND:
		break;

	case WM_PARENTNOTIFY:
		switch (LOWORD(wparam))
		{
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_XBUTTONDOWN:
			SendMessage(getApp()->getMainWindow(), WM_VIEW_SEL_CHANGED, (WPARAM)pThis->_view.getViewIndex(), 0);
			break;
		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		break;

	case WM_VIEW_COMMAND:
		switch (wparam)
		{
		case AddressBar::ABCMD_BACK:
			pThis->_view.onAppCommand(APPCOMMAND_BROWSER_BACKWARD);
			SetFocus(pThis->_view.getViewWindow());
			break;
		case AddressBar::ABCMD_FORWARD:
			pThis->_view.onAppCommand(APPCOMMAND_BROWSER_FORWARD);
			SetFocus(pThis->_view.getViewWindow());
			break;
		case AddressBar::ABCMD_HOME:
			pThis->_view.onAppCommand(APPCOMMAND_BROWSER_HOME);
			SetFocus(pThis->_view.getViewWindow());
			break;
		case AddressBar::ABCMD_UP:
			pThis->_view.goParent();
			SetFocus(pThis->_view.getViewWindow());
			break;
		case AddressBar::ABCMD_NEW_FOLDER:
			if (pThis->_bFileSystemFolder)
			{
				wchar_t path[MAX_PATH];
				wchar_t base[MAX_PATH];
				int i = 1;
				swprintf_s(base, L"%s\\%s", pThis->_parsingName.c_str(), getApp()->getLocalizeString(L"Application", L"NewFolderName").c_str());
				wcscpy_s(path, base);
				for (;;)
				{
					if (_wmkdir(path) == 0)
					{
						break;
					}
					else if (errno == EEXIST)
					{
						swprintf_s(path, L"%s (%d)", base, i++);
					}
					else
					{
						wchar_t msgboxMessage[4096] = {};
						swprintf_s(msgboxMessage, getApp()->getLocalizeString(L"Application", L"Error_NewFolder").c_str(), path, errno);
						MessageBox(getApp()->getMainWindow(), msgboxMessage, getApp()->getName().c_str(), MB_OK | MB_ICONERROR);
						return 0;
					}
				}
				IShellItem* psi = parsingNameToShellItem(path);
				if (psi != nullptr)
				{
					pThis->_view.editName(psi);
					psi->Release();
				}
			}
			break;
		case AddressBar::ABCMD_GO:
			{
				std::wstring path;
				if (pThis->_bar.getAddress(path))
				{
					if (!pThis->_view.browseTo(path))
						pThis->_bar.setAddress(pThis->_editName);
					SetFocus(pThis->_view.getViewWindow());
				}
			}
			break;
		case AddressBar::ABCMD_VIEW_FORMAT:
			getApp()->getSettings().setViewModeNext(pThis->_view.getViewIndex());
			pThis->_view.setViewMode();
			SetFocus(pThis->_view.getViewWindow());
			break;
		case AddressBar::ABCMD_ADDRESS:
			pThis->_bar.setFocusToAddress();
			break;
		}
		break;

	case WM_VIEW_SET_VIEWMODE:
		pThis->_view.setViewMode();
		break;

	case WM_CHANGED_FOLDER_VIEW:
		pThis->_onChangedFolderView((PCIDLIST_ABSOLUTE)lparam);
		break;

	case WM_SHORTCUT_COMMAND:
		{
			auto info = (ShortcutInfo*)lparam;
			if (isFile(info->getTarget()))
			{
				std::wstring startupFolder;
				if (info->startupFolder.length() > 0 && isFolder(info->startupFolder))
					startupFolder = info->startupFolder.c_str();
				else if (pThis->_parsingName.length() > 0 && pThis->_bFileSystemFolder)
					startupFolder = pThis->_parsingName.c_str();
				else
				{
					wchar_t* name = nullptr;
					if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &name))
						|| SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &name)))
					{
						startupFolder = name;
						CoTaskMemFree(name);
					}
				}
				SetFocus(pThis->_view.getViewWindow());
				if (info->checkAdmin)
				{
					wchar_t tmp[4096];
					swprintf_s(tmp, L"/c start \"%s\" /D \"%s\" \"%s\" %s", getApp()->getName().c_str(), startupFolder.c_str(), info->getTarget().c_str(), info->params.c_str());
					ShellExecute(getApp()->getMainWindow(), L"runas", L"cmd.exe", tmp, nullptr, SW_HIDE);
				}
				else
				{
					ShellExecute(getApp()->getMainWindow(), nullptr, info->getTarget().c_str(), info->params.c_str(), startupFolder.c_str(), SW_SHOWDEFAULT);
				}
			}
			else
			{
				IShellItem* psi = parsingNameToShellItem(info->getTarget());
				if (psi != nullptr)
				{
					psi->Release();
					pThis->_view.browseTo(info->getTarget());
					SetFocus(pThis->_view.getViewWindow());
				}
			}
		}
		break;

	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	return 0;
}