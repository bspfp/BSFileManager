#pragma once

#include "Layout.h"

class AddBookmarkDlg
{
public:
	AddBookmarkDlg(ShortcutInfo const & info);

	bool show(HWND hwndParent);
	ShortcutInfo const & getInfo();

private:
	void _onInitDialog();
	void _onStartupFolderBrowse();

	bool _updateData(bool bSaveValidate, ShortcutInfo& info);
	void _errorBox(HWND hwnd, std::wstring const & msg);

	static INT_PTR CALLBACK _DlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	ShortcutInfo	_info;
	HWND			_hwnd;
	TLayout			_mainLayout;
	TLayout			_centerLayout;
	FLayout			_okCancelLayout;
};
