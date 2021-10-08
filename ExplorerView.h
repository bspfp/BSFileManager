#pragma once

#include "ShellExplorerControl.h"
#include "Layout.h"
#include "AddressBar.h"

class ExplorerView
{
public:
	ExplorerView();

	bool create(HWND hwndParent, int viewIndex);
	bool preTranslateMessage(MSG* pMsg);

	void setSel(bool bSel);
	void validateBrowsingPath();
	void ensureSelectedItemVisible();
	void setFocusToShellView() const;
	void go(const wchar_t* folder, const wchar_t* filename);

	void onNewFile();
	bool onAppCommand(HWND hwnd, UINT cmd);

	HWND getWindow() const;
	HWND getTreeViewWindow() const;
	int getAddressBarHeight() const;

	std::wstring const & getParsingName() const;
	std::wstring const & getNormalName() const;

private:
	void _onChangedFolderView(PCIDLIST_ABSOLUTE pidl);

	static LRESULT CALLBACK _WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	HWND					_hwnd;
	ShellExplorerControl	_view;
	AddressBar				_bar;
	TLayout					_layout;

	std::wstring			_parsingName;
	std::wstring			_editName;
	std::wstring			_normalName;
	std::wstring			_forSelectPath;
	bool					_bFileSystemFolder;
};
