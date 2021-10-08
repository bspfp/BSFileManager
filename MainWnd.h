#pragma once

#include "Layout.h"
#include "SplitView.h"
#include "ExplorerView.h"
#include "ShortcutBar.h"

class MainWnd
{
public:
	MainWnd();
	~MainWnd();

	bool create(int nShowCmd);
	bool preTranslateMessage(MSG* pMsg);
	HWND getWindow();

private:
	bool _registerClass();
	bool _createWindow();
	void _loadMenu();
	void _resetBookmarkMenu();
	void _onCreate();
	void _onClosed();
	void _onViewSelChanged(int viewIndex);
	void _exportLocalizeFile();
	void _setShortcutBar();
	void _refreshSplitMode();

	void _onNextView();
	void _onPrevView();
	void _onHelpAbout();
	void _onCommandForView(UINT cmd, int viewIndex);
	void _onBookmarkShow(UINT idm);
	void _onMenuCommand(HMENU menu, int pos);
	void _onSplitMode(int newMode);

	static LRESULT CALLBACK _WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	HWND			_hwnd;
	HMENU			_menu;
	ULONG			_shNotifyRegister;

	int				_btSize;
	int				_selectedViewIndex;

	TLayout					_mainLayout;
	TLayout					_shortcutBarLayout;
	HSplitView				_centerSplitView;
	VSplitView				_upperSplitView;
	VSplitView				_lowerSplitView;
	ExplorerView			_exViews[4];

	ShortcutBar::Ptr		_shortcutBars[BSFM_SHORTCUT_BAR_COUNT];
	std::list<HBITMAP>		_menuBitmap;
};
