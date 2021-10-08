#include "stdafx.h"
#include "BSFMApp.h"
#include "ShortcutBar.h"
#include "AddBookmarkDlg.h"

#define BUTTON_ID_START					1001

#define CLASSNAME_ShortcutBar			L"ShortcutBar"
#define CLASSNAME_ShortcutButton		L"ShortcutButton"

namespace
{
	int translateKeyState(DWORD keyState)
	{
		if (keyState == MK_LBUTTON)
			return 1;
		else if (keyState == (MK_SHIFT | MK_LBUTTON) || keyState == MK_RBUTTON)
			return 2;
		else
			return 0;
	}
}

std::wstring ShortcutInfo::getTarget() const
{
	IShellItem* psi = parsingNameToShellItem(targetPath);
	if (psi != nullptr)
	{
		psi->Release();
		return targetPath;
	}
	return findPath(targetPath);
}

ShortcutButton::Ptr ShortcutButton::create(HWND hwndParent, ShortcutInfo const & info, UINT id, bool bUsingShellName, int maxWidth)
{
	Ptr pRet(new ShortcutButton());
	if (pRet->_create(hwndParent, info, id, bUsingShellName, maxWidth))
		return pRet;
	else
		return Ptr();
}

ShortcutButton::~ShortcutButton()
{
	if (_hIcon != nullptr)
	{
		DestroyIcon(_hIcon);
		_hIcon = nullptr;
	}
}

HWND ShortcutButton::getWindow() const
{
	return _hwnd;
}

void ShortcutButton::destroy()
{
	if (_hwnd != nullptr)
	{
		DestroyWindow(_hwnd);
		_hwnd = nullptr;
	}
}

int ShortcutButton::getHeight() const
{
	return _size.cy;
}

int ShortcutButton::getWidth() const
{
	return _size.cx;
}

ShortcutInfo const & ShortcutButton::getInfo() const
{
	return _info;
}

const wchar_t* ShortcutButton::getTip() const
{
	return _tip.c_str();
}

ShortcutButton::ShortcutButton()
	: _hwnd(nullptr)
	, _hIcon(nullptr)
{
	_size.cx = _size.cy = 0;
}

bool ShortcutButton::_create(HWND hwndParent, ShortcutInfo const & info, UINT id, bool bUsingShellName, int maxWidth)
{
	HICON hIcon;
	std::wstring name;
	getShellIcon(info.getTarget(), hIcon, name);
	if (!bUsingShellName)
		name = info.name;
	_hwnd = CreateWindowEx(0, WC_BUTTON, name.c_str(), WS_CHILD | BS_PUSHLIKE, 0, 0, 0, 0, hwndParent, (HMENU)(int64_t)id, getApp()->getHandle(), nullptr);
	if (_hwnd != nullptr)
	{
		SendMessage(_hwnd, WM_SETFONT, (WPARAM)getApp()->getFont(), FALSE);
		SendMessage(_hwnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		SendMessage(_hwnd, BCM_GETIDEALSIZE, 0, (LPARAM)&_size);
		_size.cx += 4;
		_size.cy += 4;
		_size.cx = std::max(_size.cx, 70L);
		_size.cx = std::min<LONG>(_size.cx, maxWidth);
		ShowWindow(_hwnd, SW_SHOW);
		_info = info;
		_hIcon = hIcon;

		if (isFolder(info.getTarget()) || isFile(info.getTarget()))
			_tip = info.getTarget();
		else
			_tip = name;
		return true;
	}
	DestroyIcon(hIcon);
	return false;
}

ShortcutBar::ShortcutBar(int index)
	: _hwnd(nullptr)
	, _index(index)
	, _pdth(nullptr)
	, _bRegisteredDrop(false)
	, _pObj(nullptr)
	, _refCount(1)
	, _lastKeyState(0)
{
	if (_index != 0)
		CoCreateInstance(CLSID_DragDropHelper, nullptr, CLSCTX_INPROC, IID_PPV_ARGS(&_pdth));
}

ShortcutBar::~ShortcutBar()
{
	if (_pdth != nullptr)
	{
		_pdth->Release();
		_pdth = nullptr;
	}
}
bool ShortcutBar::create(HWND hwndParent)
{
	if (hwndParent == nullptr || !IsWindow(hwndParent))
		return nullptr;

	WNDCLASS wc;
	if (!GetClassInfo(getApp()->getHandle(), CLASSNAME_ShortcutBar, &wc))
	{
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = ShortcutBar::_WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = getApp()->getHandle();
		wc.hIcon = nullptr;
		wc.hCursor = nullptr;
		wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = CLASSNAME_ShortcutBar;
		if (RegisterClass(&wc) == 0)
			return false;
	}

	DWORD style = WS_CHILD | WS_CLIPCHILDREN;
	if (getApp()->getSettings().isShowShortcutBar(_index))
		style |= WS_VISIBLE;
	_hwnd = CreateWindowEx(0, CLASSNAME_ShortcutBar, L"", style, 0, 0, 0, 0, hwndParent, nullptr, getApp()->getHandle(), this);
	if (_hwnd != nullptr)
	{
		if (_index != 0 && SUCCEEDED(RegisterDragDrop(_hwnd, this)))
			_bRegisteredDrop = true;
		return true;
	}
	else
	{
		return false;
	}
}

HWND ShortcutBar::getWindow() const
{
	return _hwnd;
}

void ShortcutBar::show(bool bShow)
{
	getApp()->getSettings().setShowShortcutBar(_index, bShow);
	ShowWindow(_hwnd, (bShow) ? SW_SHOW : SW_HIDE);
}

bool ShortcutBar::isShow() const
{
	return getApp()->getSettings().isShowShortcutBar(_index);
}

int ShortcutBar::getHeight() const
{
	return _buttons[0]->getHeight();
}

void ShortcutBar::refresh()
{
	auto regTooltip = [&,this](HWND hwndTool, const wchar_t* tip){
		// For _WIN32_WINNT  > 0x0501, you must set 'cbSize' to TTTOOLINFOA_V2_SIZE (instead of sizeof(TOOLTIPINFO)) or include the appropriate version of Common Controls in the manifest. Otherwise the tooltip won't be displayed.
		TOOLINFO ti = {0};
		ti.cbSize = TTTOOLINFOA_V2_SIZE;
		ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
		ti.hwnd = _hwnd;
		ti.uId = (UINT_PTR)hwndTool;
		ti.hinst = nullptr;
		ti.lpszText = (LPTSTR)tip;
		SendMessage(_hwndTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
	};
	auto unregTooltip = [&,this](HWND hwndTool){
		// For _WIN32_WINNT  > 0x0501, you must set 'cbSize' to TTTOOLINFOA_V2_SIZE (instead of sizeof(TOOLTIPINFO)) or include the appropriate version of Common Controls in the manifest. Otherwise the tooltip won't be displayed.
		TOOLINFO ti = {0};
		ti.cbSize = TTTOOLINFOA_V2_SIZE;
		ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
		ti.hwnd = _hwnd;
		ti.uId = (UINT_PTR)hwndTool;
		SendMessage(_hwndTooltip, TTM_DELTOOL, 0, (LPARAM)&ti);
	};

	// destroy prev button
	std::for_each(_buttons.begin(), _buttons.end(), [&unregTooltip](ShortcutButton::Ptr& pButton){
		if (pButton.get() != nullptr)
		{
			unregTooltip(pButton->getWindow());
			pButton->destroy();
		}
	});
	_buttons.clear();

	auto shortcutInfo = getApp()->getSettings().getShortcutInfo(_index);
	if (_index == 0)
	{
		// sort
		std::map<std::wstring, ShortcutInfo*> shortcutMap;
		for (auto iter = shortcutInfo.begin(); iter != shortcutInfo.end(); ++iter)
			shortcutMap.insert(std::pair<std::wstring, ShortcutInfo*>(iter->targetPath, &(*iter)));

		_layout.setFont(getApp()->getFont(), FALSE);
		_layout.resize((int)(shortcutMap.size() + 1), 1, 0);
		int idx = 0;
		for (auto iter = shortcutMap.begin(); iter != shortcutMap.end(); ++iter)
		{
			ShortcutButton::Ptr pButton = ShortcutButton::create(_hwnd, *(iter->second), BUTTON_ID_START + idx, true, 250);
			_buttons.push_back(pButton);
			if (pButton.get() != nullptr)
			{
				_layout.setWidth(idx, pButton->getWidth());
				_layout.add(TLayout::Control(pButton->getWindow()), idx, 0);
			}
			else
			{
				_layout.setWidth(idx, 0);
			}
			++idx;
		}
	}
	else
	{
		_layout.setFont(getApp()->getFont(), FALSE);
		_layout.resize((int)(shortcutInfo.size() + 1), 1, 0);
		int idx = 0;
		for (auto iter = shortcutInfo.begin(); iter != shortcutInfo.end(); ++iter)
		{
			ShortcutButton::Ptr pButton = ShortcutButton::create(_hwnd, *iter, BUTTON_ID_START + idx, false);
			_buttons.push_back(pButton);
			if (pButton.get() != nullptr)
			{
				_layout.setWidth(idx, pButton->getWidth());
				_layout.add(TLayout::Control(pButton->getWindow()), idx, 0);
			}
			else
			{
				_layout.setWidth(idx, 0);
			}
			++idx;
		}
	}

	std::for_each(_buttons.begin(), _buttons.end(), [&regTooltip](ShortcutButton::Ptr& pButton){
		if (pButton.get() != nullptr)
			regTooltip(pButton->getWindow(), pButton->getTip());
	});

	InvalidateRect(_hwnd, nullptr, TRUE);
	_layout.recalculate();
}

IFACEMETHODIMP ShortcutBar::QueryInterface(REFIID riid, void **ppv)
{
	if (_index == 0)
		return E_NOINTERFACE;

	static const QITAB qit[] =
	{
		QITABENT(ShortcutBar, IDropTarget),
		{ 0 },
	};
	return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) ShortcutBar::AddRef()
{
	return InterlockedIncrement(&_refCount);
}

IFACEMETHODIMP_(ULONG) ShortcutBar::Release()
{
	return InterlockedDecrement(&_refCount);
}

IFACEMETHODIMP ShortcutBar::DragEnter(IDataObject* pObj, DWORD keyState, POINTL pt, DWORD* pEffect)
{
	if (_index == 0)
		return E_NOTIMPL;

	_lastKeyState = keyState;
	if (_pdth != nullptr)
	{
		POINT tmp = {pt.x, pt.y};
		_pdth->DragEnter(_hwnd, pObj, &tmp, *pEffect);
	}
	if (_pObj)
	{
		_pObj->Release();
		_pObj = nullptr;
		_objName.clear();
	}
	pObj->QueryInterface(IID_PPV_ARGS(&_pObj));

	PIDLIST_ABSOLUTE pidl = nullptr;
	if (SUCCEEDED(SHGetIDListFromObject(pObj, &pidl)))
	{
		IShellItem* psi = nullptr;
		if (SUCCEEDED(SHCreateItemFromIDList(pidl, IID_PPV_ARGS(&psi))))
		{
			wchar_t* name = nullptr;
			if (SUCCEEDED(psi->GetDisplayName(SIGDN_NORMALDISPLAY, &name)))
			{
				_objName = name;
				CoTaskMemFree(name);
				_setDropTip(false);
			}
			psi->Release();
		}
		ILFree(pidl);
	}
	return S_OK;
}

IFACEMETHODIMP ShortcutBar::DragOver(DWORD keyState, POINTL pt, DWORD* pEffect)
{
	if (_index == 0)
		return E_NOTIMPL;

	DWORD oldKeyState = _lastKeyState;
	_lastKeyState = keyState;
	if (_pdth != nullptr)
	{
		POINT tmp = {pt.x, pt.y};
		_pdth->DragOver(&tmp, *pEffect);
	}
	if (oldKeyState != keyState)
		_setDropTip(false);
	return S_OK;
}

IFACEMETHODIMP ShortcutBar::DragLeave()
{
	if (_index == 0)
		return E_NOTIMPL;

	if (_pdth != nullptr)
		_pdth->DragLeave();
	_setDropTip(true);
	if (_pObj != nullptr)
	{
		_pObj->Release();
		_pObj = nullptr;
	}
	return S_OK;
}

IFACEMETHODIMP ShortcutBar::Drop(IDataObject* pObj, DWORD /*keyState*/, POINTL pt, DWORD* pEffect)
{
	if (_index == 0)
		return E_NOTIMPL;

	if (_pdth != nullptr)
	{
		POINT tmp = {pt.x, pt.y};
		_pdth->Drop(pObj, &tmp, *pEffect);
	}

	IShellItemArray* psia = nullptr;
	if (SUCCEEDED(SHCreateShellItemArrayFromDataObject(_pObj, IID_PPV_ARGS(&psia))))
	{
		IShellItem* psi = nullptr;
		if (SUCCEEDED(psia->GetItemAt(0, &psi)))
		{
			PIDLIST_ABSOLUTE pidl = nullptr;
			if (SUCCEEDED(SHGetIDListFromObject(psi, &pidl)))
			{
				std::wstring parsingName, editName, normalName;
				idListToString(pidl, &parsingName, &editName, &normalName);
				ILFree(pidl);

				ShortcutInfo info;
				if (isFile(parsingName))
				{
					wchar_t D[_MAX_DRIVE] = {0};
					wchar_t d[_MAX_DIR] = {0};
					wchar_t f[_MAX_FNAME] = {0};
					wchar_t e[_MAX_EXT] = {0};
					_wsplitpath_s(parsingName.c_str(), D, d, f, e);
					wchar_t p[_MAX_PATH] = {0};
					_wmakepath_s(p, D, d, nullptr, nullptr);
					info = ShortcutInfo(normalName, parsingName, L"", p, false);
				}
				else
				{
					info = ShortcutInfo(normalName, parsingName, L"", L"", false);
				}

				switch (translateKeyState(_lastKeyState))
				{
				case 1:
					getApp()->getSettings().addShortcutInfo(_index, info);
					refresh();
					PostMessage(getApp()->getMainWindow(), WM_RESET_BOOKMARK_MENU, 0, 0);
					break;
				case 2:
					{
						AddBookmarkDlg dlg(info);
						if (dlg.show(getApp()->getMainWindow()))
						{
							getApp()->getSettings().addShortcutInfo(_index, dlg.getInfo());
							refresh();
							PostMessage(getApp()->getMainWindow(), WM_RESET_BOOKMARK_MENU, 0, 0);
						}
					}
					break;
				}
			}
			psi->Release();
		}
		psia->Release();
	}
	return S_OK;
}

void ShortcutBar::_onCreate()
{
	_layout.setWindow(_hwnd);
	_hwndTooltip = CreateWindow(TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_ALWAYSTIP, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, _hwnd, nullptr, getApp()->getHandle(), nullptr);
	refresh();
}

void ShortcutBar::_setDropTip(bool bReset)
{
	static CLIPFORMAT cf = 0;
	if (cf == 0)
		cf = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DROPDESCRIPTION);

	DROPDESCRIPTION dd = {};
	if (!bReset)
	{
		switch (translateKeyState(_lastKeyState))
		{
		case 1:
			dd.type = DROPIMAGE_COPY;
			break;
		case 2:
			dd.type = DROPIMAGE_LINK;
			break;
		default:
			dd.type = DROPIMAGE_NONE;
			break;
		}
		wcscpy_s(dd.szMessage, L"%1");
		wcscpy_s(dd.szInsert, _objName.c_str());
	}
	else
	{
		dd.type = DROPIMAGE_NONE;
		wcscpy_s(dd.szMessage, L"%1");
		wcscpy_s(dd.szInsert, L"");
	}

	void* pv = GlobalAlloc(GPTR, sizeof(dd));
	if (pv != nullptr)
	{
		memcpy(pv, &dd, sizeof(dd));
		FORMATETC fmte = {cf, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		STGMEDIUM medium = {};
		medium.tymed = TYMED_HGLOBAL;
		medium.hGlobal = pv;
		if (FAILED(_pObj->SetData(&fmte, &medium, TRUE)))
			GlobalFree(pv);
	}
}

LRESULT CALLBACK ShortcutBar::_WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	ShortcutBar* pThis = (ShortcutBar*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (msg)
	{
	case WM_NCCREATE:
		pThis = (ShortcutBar*)((CREATESTRUCT*)lparam)->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
		pThis->_hwnd = hwnd;
		return DefWindowProc(hwnd, msg, wparam, lparam);

	case WM_CREATE:
		pThis->_onCreate();
		break;

	case WM_DESTROY:
		if (pThis->_bRegisteredDrop)
		{
			RevokeDragDrop(hwnd);
			pThis->_bRegisteredDrop = false;
		}
		break;

	case WM_SIZE:
		pThis->_layout.recalculate();
		break;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint(hwnd, &ps);
			RECT rect;
			GetClientRect(hwnd, &rect);
			OffsetRect(&rect, -rect.left, -rect.top);
			HBITMAP hbmp = CreateCompatibleBitmap(ps.hdc, rect.right, rect.bottom);
			HDC hdcMem = CreateCompatibleDC(ps.hdc);
			auto oldBmp = SelectObject(hdcMem, hbmp);

			FillRect(hdcMem, &rect, (HBRUSH)GetClassLongPtr(hwnd, GCLP_HBRBACKGROUND));

			if (pThis->_buttons.empty())
			{
				auto oldFont = SelectObject(hdcMem, getApp()->getFont());
				RECT rectText;
				CopyRect(&rectText, &rect);
				OffsetRect(&rectText, 3, 0);
				auto oldBkMode = SetBkMode(hdcMem, TRANSPARENT);
				auto oldTextColor = SetTextColor(hdcMem, GetSysColor(COLOR_BTNTEXT));
				DrawText(hdcMem, getApp()->getLocalizeString(L"Application", L"EmptyShortcutBar").c_str(), -1, &rectText, DT_SINGLELINE | DT_END_ELLIPSIS | DT_EXPANDTABS | DT_NOPREFIX | DT_VCENTER | DT_LEFT);
				SetTextColor(hdcMem, oldTextColor);
				SetBkMode(hdcMem, oldBkMode);
				SelectObject(hdcMem, oldFont);
			}

			BitBlt(ps.hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top, hdcMem, 0, 0, SRCCOPY);

			SelectObject(hdcMem, oldBmp);
			DeleteDC(hdcMem);
			DeleteObject(hbmp);

			EndPaint(hwnd, &ps);
		}
		break;

	case WM_ERASEBKGND:
		break;

	case WM_COMMAND:
		if (HIWORD(wparam) == BN_CLICKED)
		{
			int idx = (int)(LOWORD(wparam)) - BUTTON_ID_START;
			auto button = pThis->_buttons[idx];
			if (button.get() != nullptr)
			{
				ShortcutInfo info = button->getInfo();
				SendMessage(getApp()->getMainWindow(), WM_SHORTCUT_COMMAND, 0, (LPARAM)&info);
			}
		}
		else
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		break;

	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}