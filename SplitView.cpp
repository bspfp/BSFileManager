#include "stdafx.h"
#include "BSFMApp.h"
#include "SplitView.h"

#define CLASSNAME_SplitViewSplitter		L"SplitViewSplitter"

namespace
{
	static LRESULT CALLBACK _SplitterWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	static void _recalculate(SplitViewBase::Params const & param, RECT const & rectClient, RECT& rectLeft, RECT& rectRight)
	{
		rectLeft = rectClient;
		rectRight = rectClient;
		int space = std::max<int>(rectClient.right - rectClient.left - param.splitterSize, 0);

		if (param.bRelative)
		{
			rectLeft.right = rectLeft.left + (int)(space * param.curRPos);
		}
		else
		{
			if (param.curAPos < 0 || space < param.minAFirst + param.minALast)
			{
				rectLeft.right = rectLeft.left + space / 2;
			}
			else
			{
				int curPos = param.curAPos - rectClient.left;
				if (curPos < param.minAFirst)
				{
					rectLeft.right = rectLeft.left + param.minAFirst;
				}
				else if (space - curPos < param.minALast)
				{
					rectLeft.right = rectLeft.right - param.minALast - param.splitterSize / 2;
				}
				else
				{
					rectLeft.right = rectLeft.left + curPos;
				}
			}
		}
		rectRight.left = std::min(rectLeft.right + param.splitterSize - param.splitterSize / 2, rectRight.right);
	}
}

SplitViewBase::Params::Params()
{
	*this = Params(0.1, 0.9);
}

SplitViewBase::Params::Params(int minFirst, int minLast, int curPos, int splitterSize)
	: bRelative(false)
	, curAPos(curPos)
	, minAFirst(std::max(minFirst, 0))
	, minALast(std::max(minLast, 0))
	, curRPos(0.0)
	, minRSize(0.0)
	, maxRSize(0.0)
	, splitterSize(splitterSize)
{
}

SplitViewBase::Params::Params(double minFirst, double maxFirst, double curPos, int splitterSize)
	: bRelative(true)
	, curAPos(0)
	, minAFirst(0)
	, minALast(0)
	, curRPos(std::max(curPos, 0.0))
	, minRSize(std::max(minFirst, 0.0))
	, maxRSize(std::max(maxFirst, minFirst + 0.1))
	, splitterSize(splitterSize)
{
}

bool SplitViewBase::Params::change(int curPos)
{
	if (bRelative || curAPos == curPos)
		return false;
	curAPos = curPos;
	return true;
}

bool SplitViewBase::Params::change(double curPos)
{
	if (!bRelative)
		return false;
	curPos = std::min(std::max(curPos, minRSize), maxRSize);
	if (curRPos == curPos)
		return false;
	curRPos = curPos;
	return true;
}

SplitViewBase::SplitViewBase(bool bRedraw)
	: _hfont(nullptr)
	, _hwndSelf(nullptr)
	, _hwndFirst(nullptr)
	, _hwndLast(nullptr)
	, _hwndSplliter(nullptr)
	, _defaultParams(50, 50)
	, _bRedraw(bRedraw)
	, _bShowFirst(false)
	, _bShowLast(false)
{
	_params = _defaultParams;
}

SplitViewBase::SplitViewBase(int minFirst, int minLast, int curPos, int splitterSize, bool bRedraw)
	: _hfont(nullptr)
	, _hwndSelf(nullptr)
	, _hwndFirst(nullptr)
	, _hwndLast(nullptr)
	, _hwndSplliter(nullptr)
	, _defaultParams(minFirst, minLast, curPos, splitterSize)
	, _bRedraw(bRedraw)
	, _bShowFirst(false)
	, _bShowLast(false)
{
	_params = _defaultParams;
}

SplitViewBase::SplitViewBase(double minFirst, double maxFirst, double curPos, int splitterSize, bool bRedraw)
	: _hfont(nullptr)
	, _hwndSelf(nullptr)
	, _hwndFirst(nullptr)
	, _hwndLast(nullptr)
	, _hwndSplliter(nullptr)
	, _defaultParams(minFirst, maxFirst, curPos, splitterSize)
	, _bRedraw(bRedraw)
	, _bShowFirst(false)
	, _bShowLast(false)
{
	_params = _defaultParams;
}

bool SplitViewBase::create(HWND hwndParent)
{
	if (hwndParent == nullptr || !IsWindow(hwndParent))
		return nullptr;
	if (!_registerClass())
		return false;
	if (!_createWindow(hwndParent))
		return false;
	return true;
}

void SplitViewBase::show(bool bShow)
{
	if (bShow)
		ShowWindow(_hwndSelf, SW_SHOW);
	else
		ShowWindow(_hwndSelf, SW_HIDE);
}

HWND SplitViewBase::getWindow() const
{
	return _hwndSelf;
}

void SplitViewBase::setParameters(Params const & val)
{
	_params = val;
	recalculate();
}

void SplitViewBase::setDefaultParameters(Params const & val, bool bUpdateCurrent)
{
	_defaultParams = val;
	if (bUpdateCurrent)
		setParameters(val);
}

void SplitViewBase::setRedrawMode(bool val)
{
	_bRedraw = val;
}

int SplitViewBase::getCurrentPosA() const
{
	return _params.curAPos;
}

void SplitViewBase::setCurrentPosA(int val)
{
	_params.change(val);
}

double SplitViewBase::getCurrentPosR() const
{
	return _params.curRPos;
}

void SplitViewBase::setCurrentPosR(double val)
{
	_params.change(val);
}

void SplitViewBase::setFirst(HWND hwnd, bool bShow)
{
	if (_hwndSelf == hwnd)
		return ;
	if (_hwndFirst == hwnd)
		return ;
	if (_hwndLast != nullptr && _hwndLast == hwnd)
	{
		_hwndLast = nullptr;
		_bShowLast = false;
	}
	_setChild(hwnd, _hwndFirst);
	_bShowFirst = bShow;
}

void SplitViewBase::setLast(HWND hwnd, bool bShow)
{
	if (_hwndSelf == hwnd)
		return ;
	if (_hwndLast == hwnd)
		return ;
	if (_hwndFirst != nullptr && _hwndFirst == hwnd)
	{
		_hwndFirst = nullptr;
		_bShowFirst = false;
	}
	_setChild(hwnd, _hwndLast);
	_bShowLast = bShow;
}

HWND SplitViewBase::getFirst() const
{
	return _hwndFirst;
}

HWND SplitViewBase::getLast() const
{
	return _hwndLast;
}

void SplitViewBase::showFirst(bool bShow)
{
	if (_hwndFirst != nullptr && IsWindow(_hwndFirst))
	{
		_bShowFirst = bShow;
		ShowWindow(_hwndFirst, (bShow) ? SW_SHOW : SW_HIDE);
	}
	else
	{
		_bShowFirst = false;
	}
}

void SplitViewBase::showLast(bool bShow)
{
	if (_hwndLast != nullptr && IsWindow(_hwndLast))
	{
		_bShowLast = bShow;
		ShowWindow(_hwndLast, (bShow) ? SW_SHOW : SW_HIDE);
	}
	else
	{
		_bShowLast = false;
	}
}

void SplitViewBase::resize(RECT const & rectParentClient)
{
	MoveWindow(_hwndSelf, rectParentClient.left, rectParentClient.top, rectParentClient.right - rectParentClient.left, rectParentClient.bottom - rectParentClient.top, TRUE);
	recalculate();
}

void SplitViewBase::resetSplitter()
{
	setParameters(_defaultParams);
}

void SplitViewBase::recalculate(bool bDrawSplitter)
{
	RECT rectClient;
	GetClientRect(_hwndSelf, &rectClient);

	if (_hwndFirst == nullptr || !IsWindow(_hwndFirst) || !_bShowFirst)
	{
		if (_hwndLast == nullptr || !IsWindow(_hwndLast) || !_bShowLast)
		{
			// Empty
		}
		else
		{
			// Last Only
			MoveWindow(_hwndLast, rectClient.left, rectClient.top, rectClient.right - rectClient.left, rectClient.bottom - rectClient.top, TRUE);
		}
	}
	else if (_hwndLast == nullptr || !IsWindow(_hwndLast) || !_bShowLast)
	{
		// First Only
		MoveWindow(_hwndFirst, rectClient.left, rectClient.top, rectClient.right - rectClient.left, rectClient.bottom - rectClient.top, TRUE);
	}
	else
	{
		_recalculate(bDrawSplitter);
	}
}

bool SplitViewBase::_registerClass()
{
	WNDCLASS wc;
	if (!GetClassInfo(getApp()->getHandle(), getClassName(), &wc))
	{
		wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = SplitViewBase::_WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = getApp()->getHandle();
		wc.hIcon = nullptr;
		wc.hCursor = nullptr;
		wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = getClassName();
		if (RegisterClass(&wc) == 0)
			return false;
	}
	if (!GetClassInfo(getApp()->getHandle(), CLASSNAME_SplitViewSplitter, &wc))
	{
		wc.style = 0;
		wc.lpfnWndProc = _SplitterWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = getApp()->getHandle();
		wc.hIcon = nullptr;
		wc.hCursor = nullptr;
		wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = CLASSNAME_SplitViewSplitter;
		if (RegisterClass(&wc) == 0)
			return false;
	}
	return true;
}

bool SplitViewBase::_createWindow(HWND hwndParent)
{
	HWND ret = CreateWindowEx(0, getClassName(), L"", WS_CHILD | WS_CLIPCHILDREN, 0, 0, 0, 0, hwndParent, nullptr, getApp()->getHandle(), this);
	return (ret != nullptr);
}

void SplitViewBase::_setChild(HWND hwnd, HWND& destHwnd)
{
	if (hwnd == nullptr || !::IsWindow(hwnd))
	{
		destHwnd = nullptr;
	}
	else
	{
		destHwnd = hwnd;
		SetClassLong(hwnd, GCL_STYLE, GetClassLong(hwnd, GCL_STYLE) | CS_VREDRAW | CS_HREDRAW);
		if (::GetParent(destHwnd) != _hwndSelf)
		{
			SetParent(destHwnd, _hwndSelf);
			SendMessage(destHwnd, WM_CHANGEUISTATE, UIS_INITIALIZE, 0);
		}
		SendMessage(destHwnd, WM_SETFONT, (WPARAM)_hfont, (LPARAM)IsWindowVisible(destHwnd));
	}
	recalculate();
}

void SplitViewBase::_refreshCursor()
{
	if (_params.splitterSize > 0)
		SetClassLongPtr(_hwndSelf, GCLP_HCURSOR, (LONG_PTR)_getSizingCursor());
	else
		SetClassLongPtr(_hwndSelf, GCLP_HCURSOR, (LONG_PTR)LoadCursor(nullptr, IDC_ARROW));
}

void SplitViewBase::_createSplitter()
{
	_bSplitterMoving = false;
	_refreshCursor();
	_hwndSplliter = CreateWindowEx(WS_EX_WINDOWEDGE | WS_EX_TOPMOST, CLASSNAME_SplitViewSplitter, L"", WS_CHILD | WS_BORDER | WS_DISABLED, 0, 0, 0, 0, _hwndSelf, nullptr, getApp()->getHandle(), nullptr);
	if (!_hwndSplliter)
		_bRedraw = true;
}

LRESULT CALLBACK SplitViewBase::_WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	SplitViewBase* pThis = (SplitViewBase*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (msg)
	{
	case WM_NCCREATE:
		pThis = (SplitViewBase*)((CREATESTRUCT*)lparam)->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
		pThis->_hwndSelf = hwnd;
		return DefWindowProc(hwnd, msg, wparam, lparam);

	case WM_CREATE:
		pThis->_createSplitter();
		pThis->_onCreate();
		break;

	case WM_MOUSEMOVE:
		if (pThis->_params.splitterSize > 0)
		{
			// only LBUTTON
			if (wparam == MK_LBUTTON && pThis->_bSplitterMoving)
			{
				RECT rectClient;
				GetClientRect(pThis->_hwndSelf, &rectClient);
				if (LOWORD(lparam) > rectClient.right && HIWORD(lparam) > rectClient.bottom)
					return 0;
				pThis->moveSplitter((short)LOWORD(lparam), (short)HIWORD(lparam));
			}
		}
		break;

	case WM_LBUTTONDOWN:
		if (pThis->_params.splitterSize > 0)
		{
			pThis->_bSplitterMoving = true;
			SetCapture(pThis->_hwndSelf);
			if (!pThis->_bRedraw && pThis->_hwndSplliter != nullptr)
			{
				MoveWindow(pThis->_hwndSplliter, 0, 0, 1, 1, TRUE);
				ShowWindow(pThis->_hwndSplliter, SW_SHOW);
				pThis->moveSplitter((short)LOWORD(lparam), (short)HIWORD(lparam));
			}
		}
		break;

	case WM_LBUTTONUP:
		if (pThis->_params.splitterSize > 0)
		{
			ReleaseCapture();
			pThis->_bSplitterMoving = false;
			if (pThis->_hwndSplliter != nullptr && IsWindowVisible(pThis->_hwndSplliter))
			{
				ShowWindow(pThis->_hwndSplliter, SW_HIDE);
				pThis->recalculate();
			}
		}
		break;

	case WM_LBUTTONDBLCLK:
		if (pThis->_params.splitterSize > 0)
			pThis->resetSplitter();
		break;

	case WM_SIZE:
		pThis->recalculate();
		break;

	case WM_SETFONT:
		pThis->_hfont = (HFONT)wparam;
		if (pThis->_hwndFirst != nullptr)
			SendMessage(pThis->_hwndFirst, WM_SETFONT, wparam, lparam);
		if (pThis->_hwndLast != nullptr)
			SendMessage(pThis->_hwndLast, WM_SETFONT, wparam, lparam);
		InvalidateRect(hwnd, nullptr, TRUE);
		break;

	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}

VSplitView::VSplitView(bool bRedraw)
	: SplitViewBase(bRedraw)
{
}

VSplitView::VSplitView(int minLeft, int minRight, int curPos, int splitterSize, bool bRedraw)
	: SplitViewBase(minLeft, minRight, curPos, splitterSize, bRedraw)
{
}

VSplitView::VSplitView(double minLeft, double maxLeft, double curPos, int splitterSize, bool bRedraw)
	: SplitViewBase(minLeft, maxLeft, curPos, splitterSize, bRedraw)
{
}

void VSplitView::setLeft(HWND hwnd, bool bShow)
{
	setFirst(hwnd, bShow);
}

void VSplitView::setRight(HWND hwnd, bool bShow)
{
	setLast(hwnd, bShow);
}

HWND VSplitView::getLeft() const
{
	return getFirst();
}

HWND VSplitView::getRight() const
{
	return getLast();
}

void VSplitView::showLeft(bool bShow)
{
	showFirst(bShow);
}

void VSplitView::showRight(bool bShow)
{
	showLast(bShow);
}

void VSplitView::moveSplitter(int x, int /*y*/)
{
	if (_params.splitterSize < 1)
		return;

	RECT rectClient;
	GetClientRect(_hwndSelf, &rectClient);
	x -= rectClient.left;
	int space = std::max<int>(rectClient.right - rectClient.left - _params.splitterSize, 1);

	if (_hwndFirst != nullptr && _hwndLast != nullptr)
	{
		bool bNeedUpdate = false;
		if (_params.bRelative)
		{
			bNeedUpdate = _params.change(x * 1.0 / space);
		}
		else
		{
			if (space < _params.minAFirst + _params.minALast)
			{
				x = -1;
			}
			else
			{
				if (x < _params.minAFirst)
				{
					x = _params.minAFirst;
				}
				else if (space - x < _params.minALast)
				{
					x = space - _params.minALast - _params.splitterSize;
				}
			}
			bNeedUpdate = _params.change(x);
		}
		if (bNeedUpdate)
			recalculate((_hwndSplliter != nullptr && IsWindowVisible(_hwndSplliter)));
	}
}

const wchar_t* VSplitView::getClassName()
{
	return L"VSplitView";
}

HCURSOR VSplitView::_getSizingCursor()
{
	return LoadCursor(nullptr, IDC_SIZEWE);
}

void VSplitView::_recalculate(bool bDrawSplitter)
{
	RECT rectClient, rectLeft, rectRight;
	GetClientRect(_hwndSelf, &rectClient);
	::_recalculate(_params, rectClient, rectLeft, rectRight);
	if (bDrawSplitter)
	{
		MoveWindow(_hwndSplliter, rectLeft.right, rectLeft.top, _params.splitterSize, rectLeft.bottom - rectLeft.top, TRUE);
	}
	else
	{
		MoveWindow(_hwndFirst, rectLeft.left, rectLeft.top, rectLeft.right - rectLeft.left, rectLeft.bottom - rectLeft.top, TRUE);
		MoveWindow(_hwndLast, rectRight.left, rectRight.top, rectRight.right - rectRight.left, rectRight.bottom - rectRight.top, TRUE);
	}
}

HSplitView::HSplitView(bool bRedraw)
	: SplitViewBase(bRedraw)
{
}

HSplitView::HSplitView(int minTop, int minBottom, int curPos, int splitterSize, bool bRedraw)
	: SplitViewBase(minTop, minBottom, curPos, splitterSize, bRedraw)
{
}

HSplitView::HSplitView(double minTop, double maxTop, double curPos, int splitterSize, bool bRedraw)
	: SplitViewBase(minTop, maxTop, curPos, splitterSize, bRedraw)
{
}

void HSplitView::setTop(HWND hwnd, bool bShow)
{
	setFirst(hwnd, bShow);
}

void HSplitView::setBottom(HWND hwnd, bool bShow)
{
	setLast(hwnd, bShow);
}

HWND HSplitView::getTop() const
{
	return getFirst();
}

HWND HSplitView::getBottom() const
{
	return getLast();
}

void HSplitView::showTop(bool bShow)
{
	showFirst(bShow);
}

void HSplitView::showBottom(bool bShow)
{
	showLast(bShow);
}

void HSplitView::moveSplitter(int /*x*/, int y)
{
	if (_params.splitterSize < 1)
		return;

	RECT rectClient;
	GetClientRect(_hwndSelf, &rectClient);
	y -= rectClient.top;
	int space = std::max<int>(rectClient.bottom - rectClient.top - _params.splitterSize, 1);

	if (_hwndFirst != nullptr && _hwndLast != nullptr)
	{
		bool bNeedUpdate = false;
		if (_params.bRelative)
		{
			bNeedUpdate = _params.change(y * 1.0 / space);
		}
		else
		{
			if (space < _params.minAFirst + _params.minALast)
			{
				y = -1;
			}
			else
			{
				if (y < _params.minAFirst)
				{
					y = _params.minAFirst;
				}
				else if (space - y < _params.minALast)
				{
					y = space - _params.minALast - _params.splitterSize;
				}
			}
			bNeedUpdate = _params.change(y);
		}
		if (bNeedUpdate)
			recalculate((_hwndSplliter != nullptr && IsWindowVisible(_hwndSplliter)));
	}
}

const wchar_t* HSplitView::getClassName()
{
	return L"HSplitView";
}

HCURSOR HSplitView::_getSizingCursor()
{
	return LoadCursor(nullptr, IDC_SIZENS);
}

void HSplitView::_recalculate(bool bDrawSplitter)
{
	RECT rectClient, rectTop, rectBottom;
	GetClientRect(_hwndSelf, &rectClient);
	SetRect(&rectClient, rectClient.top, rectClient.left, rectClient.bottom, rectClient.right);
	::_recalculate(_params, rectClient, rectTop, rectBottom);
	SetRect(&rectTop, rectTop.top, rectTop.left, rectTop.bottom, rectTop.right);
	SetRect(&rectBottom, rectBottom.top, rectBottom.left, rectBottom.bottom, rectBottom.right);
	if (bDrawSplitter)
	{
		MoveWindow(_hwndSplliter, rectTop.left, rectTop.bottom, rectTop.right - rectTop.left, _params.splitterSize, TRUE);
	}
	else
	{
		MoveWindow(_hwndFirst, rectTop.left, rectTop.top, rectTop.right - rectTop.left, rectTop.bottom - rectTop.top, TRUE);
		MoveWindow(_hwndLast, rectBottom.left, rectBottom.top, rectBottom.right - rectBottom.left, rectBottom.bottom - rectBottom.top, TRUE);
	}
}