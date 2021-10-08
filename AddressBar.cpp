#include "stdafx.h"
#include "BSFMApp.h"
#include "AddressBar.h"

#define CLASSNAME_AddressBar	L"AddressBar"
#define CONTROL_ID_START		1001

AddressBar::AddressBar()
	: _hwnd(nullptr)
	, _hwndAddress(0)
	, _btSize(0)
{
}

bool AddressBar::create(HWND hwndParent)
{
	WNDCLASS wc;
	if (!GetClassInfo(getApp()->getHandle(), CLASSNAME_AddressBar, &wc))
	{
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = AddressBar::_WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = getApp()->getHandle();
		wc.hIcon = nullptr;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = CLASSNAME_AddressBar;
		if (RegisterClass(&wc) == 0)
			return false;
	}

	_layout.setFont(getApp()->getFont(), false);

	_hwnd = CreateWindow(CLASSNAME_AddressBar, L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, 0, 0, hwndParent, nullptr, getApp()->getHandle(), this);
	if (_hwnd == nullptr)
		return false;

	return true;
}

bool AddressBar::preTranslateMessage(MSG* pMsg)
{
	if (pMsg->hwnd == _hwndAddress && pMsg->message == WM_CHAR && pMsg->wParam == 0x0d)
	{
		SendMessage(GetParent(_hwnd), WM_VIEW_COMMAND, AddressBar::ABCMD_GO, 0);
		return true;
	}
	return false;
}

HWND AddressBar::getWindow() const
{
	return _hwnd;
}

void AddressBar::setAddress(std::wstring const & path)
{
	SendMessage(_hwndAddress, WM_SETTEXT, 0, (LPARAM)path.c_str());
	SendMessage(_hwndAddress, EM_SETSEL, 0, (LPARAM)-1);
}

bool AddressBar::getAddress(std::wstring& buf) const
{
	return (getWindowText(_hwndAddress, buf) > 0);
}

int AddressBar::getHeight() const
{
	return _btSize;
}

void AddressBar::setFocusToAddress() const
{
	SetFocus(_hwndAddress);
}

void AddressBar::_onCreate()
{
	_layout.setWindow(_hwnd);

	HINSTANCE hInst = getApp()->getHandle();
	HWND hwndBack = CreateWindow(WC_BUTTON, nullptr, WS_VISIBLE | WS_CHILD | BS_ICON | BS_PUSHLIKE, 0, 0, 0, 0, _hwnd, (HMENU)(CONTROL_ID_START + ABCMD_BACK), hInst, nullptr);
	HWND hwndForw = CreateWindow(WC_BUTTON, nullptr, WS_VISIBLE | WS_CHILD | BS_ICON | BS_PUSHLIKE, 0, 0, 0, 0, _hwnd, (HMENU)(CONTROL_ID_START + ABCMD_FORWARD), hInst, nullptr);
	HWND hwndToHome = CreateWindow(WC_BUTTON, nullptr, WS_VISIBLE | WS_CHILD | BS_ICON | BS_PUSHLIKE, 0, 0, 0, 0, _hwnd, (HMENU)(CONTROL_ID_START + ABCMD_HOME), hInst, nullptr);
	HWND hwndToUp = CreateWindow(WC_BUTTON, nullptr, WS_VISIBLE | WS_CHILD | BS_ICON | BS_PUSHLIKE, 0, 0, 0, 0, _hwnd, (HMENU)(CONTROL_ID_START + ABCMD_UP), hInst, nullptr);
	HWND hwndNewFolder = CreateWindow(WC_BUTTON, nullptr, WS_VISIBLE | WS_CHILD | BS_ICON | BS_PUSHLIKE, 0, 0, 0, 0, _hwnd, (HMENU)(CONTROL_ID_START + ABCMD_NEW_FOLDER), hInst, nullptr);
	_hwndAddress = CreateWindow(WC_EDIT, L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | WS_BORDER | ES_LEFT, 0, 0, 0, 0, _hwnd, (HMENU)(CONTROL_ID_START + ABCMD_ADDRESS), hInst, nullptr);
	HWND hwndGo = CreateWindow(WC_BUTTON, nullptr, WS_VISIBLE | WS_CHILD | BS_ICON | BS_PUSHLIKE, 0, 0, 0, 0, _hwnd, (HMENU)(CONTROL_ID_START + ABCMD_GO), hInst, nullptr);
	HWND hwndViewFormat = CreateWindow(WC_BUTTON, nullptr, WS_VISIBLE | WS_CHILD | BS_ICON | BS_PUSHLIKE, 0, 0, 0, 0, _hwnd, (HMENU)(CONTROL_ID_START + ABCMD_VIEW_FORMAT), hInst, nullptr);

	SendMessage(hwndBack, BM_SETIMAGE, IMAGE_ICON, (LPARAM)getApp()->getAddressBarIcon(0));
	SendMessage(hwndForw, BM_SETIMAGE, IMAGE_ICON, (LPARAM)getApp()->getAddressBarIcon(1));
	SendMessage(hwndToHome, BM_SETIMAGE, IMAGE_ICON, (LPARAM)getApp()->getAddressBarIcon(2));
	SendMessage(hwndToUp, BM_SETIMAGE, IMAGE_ICON, (LPARAM)getApp()->getAddressBarIcon(3));
	SendMessage(hwndNewFolder, BM_SETIMAGE, IMAGE_ICON, (LPARAM)getApp()->getAddressBarIcon(4));
	SendMessage(hwndGo, BM_SETIMAGE, IMAGE_ICON, (LPARAM)getApp()->getAddressBarIcon(5));
	SendMessage(hwndViewFormat, BM_SETIMAGE, IMAGE_ICON, (LPARAM)getApp()->getAddressBarIcon(6));

	HWND hwndTooltip = CreateWindow(TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_ALWAYSTIP, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, _hwnd, nullptr, getApp()->getHandle(), nullptr);
	auto regTooltip = [&,this](HWND hwndTool, std::wstring const & tooltip){
		// For _WIN32_WINNT  > 0x0501, you must set 'cbSize' to TTTOOLINFOA_V2_SIZE (instead of sizeof(TOOLTIPINFO)) or include the appropriate version of Common Controls in the manifest. Otherwise the tooltip won't be displayed.
		TOOLINFO ti = {0};
		ti.cbSize = TTTOOLINFOA_V2_SIZE;
		ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
		ti.hwnd = _hwnd;
		ti.uId = (UINT_PTR)hwndTool;
		ti.hinst = nullptr;
		ti.lpszText = (LPTSTR)tooltip.c_str();
		SendMessage(hwndTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
	};

	_tooltipPrev = getApp()->getLocalizeString(L"Tooltips", L"Prev");
	_tooltipNext = getApp()->getLocalizeString(L"Tooltips", L"Next");
	_tooltipToHome = getApp()->getLocalizeString(L"Tooltips", L"ToHome");
	_tooltipToUp = getApp()->getLocalizeString(L"Tooltips", L"ToUp");
	_tooltipNewFolder = getApp()->getLocalizeString(L"Tooltips", L"NewFolder");
	_tooltipGo = getApp()->getLocalizeString(L"Tooltips", L"Go");
	_tooltipViewFormat = getApp()->getLocalizeString(L"Tooltips", L"ViewFormat");
	regTooltip(hwndBack, _tooltipPrev);
	regTooltip(hwndForw, _tooltipNext);
	regTooltip(hwndToHome, _tooltipToHome);
	regTooltip(hwndToUp, _tooltipToUp);
	regTooltip(hwndNewFolder, _tooltipNewFolder);
	regTooltip(hwndGo, _tooltipGo);
	regTooltip(hwndViewFormat, _tooltipViewFormat);

	int fontHeight = getApp()->getFontHeight();
	int colAddress[] = {TLayout::AUTO_SIZE};
	int rowAddress[] = {TLayout::AUTO_SIZE, fontHeight + 4, TLayout::AUTO_SIZE};
	_layoutAddress.resize(colAddress, rowAddress, 0);
	_layoutAddress.add(TLayout::Control(_hwndAddress), 0, 1);

	_btSize = fontHeight + 4 + 4;
	int col[] = {_btSize, 2, _btSize, 2, _btSize, 2, _btSize, 2, _btSize, 2, TLayout::AUTO_SIZE, 2, _btSize, 2, _btSize};	// 이전(0), 다음(2), 홈(4), 위로(6), 새폴더(8), 주소(10), 이동(12), 보기변경(14)
	int row[] = {_btSize, TLayout::AUTO_SIZE};
	_layout.resize(col, row, 0);

	_layout.add(TLayout::Control(hwndBack), 0, 0);
	_layout.add(TLayout::Control(hwndForw), 2, 0);
	_layout.add(TLayout::Control(hwndToHome), 4, 0);
	_layout.add(TLayout::Control(hwndToUp), 6, 0);
	_layout.add(TLayout::Control(hwndNewFolder), 8, 0);
	_layout.add(TLayout::Control(&_layoutAddress), 10, 0);
	_layout.add(TLayout::Control(hwndGo), 12, 0);
	_layout.add(TLayout::Control(hwndViewFormat), 14, 0);
}

LRESULT CALLBACK AddressBar::_WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	AddressBar* pThis = (AddressBar*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (msg)
	{
	case WM_NCCREATE:
		pThis = (AddressBar*)((CREATESTRUCT*)lparam)->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
		pThis->_hwnd = hwnd;
		return DefWindowProc(hwnd, msg, wparam, lparam);

	case WM_CREATE:
		pThis->_onCreate();
		break;

	case WM_SIZE:
		{
			pThis->_layout.recalculate();
			DWORD start, end;
			HWND hwndAddress = GetDlgItem(hwnd, CONTROL_ID_START + ABCMD_ADDRESS);
			{
				SendMessage(hwndAddress, EM_GETSEL,	(WPARAM)&start, (LPARAM)&end);
				SendMessage(hwndAddress, EM_SETSEL, 0, 0);
				SendMessage(hwndAddress, EM_SETSEL, (WPARAM)start, (LPARAM)end);
			}
		}
		break;

	case WM_COMMAND:
		switch (HIWORD(wparam))
		{
		case BN_CLICKED:
			switch (LOWORD(wparam) - CONTROL_ID_START)
			{
			case ABCMD_BACK:
			case ABCMD_FORWARD:
			case ABCMD_HOME:
			case ABCMD_UP:
			case ABCMD_NEW_FOLDER:
			case ABCMD_GO:
			case ABCMD_VIEW_FORMAT:
				SendMessage(GetParent(hwnd), WM_VIEW_COMMAND, LOWORD(wparam) - CONTROL_ID_START, 0);
				break;
			default:
				return DefWindowProc(hwnd, msg, wparam, lparam);
			}
			break;
		case EN_SETFOCUS:
			if (LOWORD(wparam) - CONTROL_ID_START == ABCMD_ADDRESS)
			{
				PostMessage((HWND)lparam, EM_SETSEL, 0, -1);
			}
			else
			{
				return DefWindowProc(hwnd, msg, wparam, lparam);
			}
			break;
		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		break;

	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}