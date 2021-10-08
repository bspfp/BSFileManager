#pragma once

#include "Layout.h"

class BookmarkManagerDlg
{
public:
	BookmarkManagerDlg();
	~BookmarkManagerDlg();

	bool show(HWND hwndParent);
	std::vector<ShortcutInfo> const & getInfo(int barIndex);

private:
	void _onInitDialog();
	void _onAdd();
	void _onEdit();
	void _onDelete();
	void _onUp();
	void _onDown();
	void _onTargetBrowse();
	void _onStartupFolderBrowse();
	void _onTreeSelectChanged(TVITEM const & item);

	void _initLayout();
	void _initTree();
	void _insertTreeItem(int barIndex, size_t itemIndex, bool bSelect);
	void _updateTreeItem();
	bool _updateData(bool bSaveValidate, ShortcutInfo& info);
	void _swapData(int barIndex, HTREEITEM hItemBar, int itemIndex1, int itemIndex2);
	void _errorBox(HWND hwnd, std::wstring const & msg);

	static INT_PTR CALLBACK _DlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	bool		_bChanged;
	HWND		_hwnd;
	HIMAGELIST	_hTreeImage;
	TLayout		_mainLayout;
	TLayout		_centerLayout;
	TLayout		_leftLayout;
	TLayout		_rightLayout;
	TLayout		_buttonLayout;
	FLayout		_closeLayout;

	int			_selectedBarIndex;
	int			_selectedItemIndex;
	HTREEITEM	_selectedItem;

	std::vector<ShortcutInfo>	_shortcutInfo[BSFM_SHORTCUT_BAR_COUNT];		// 0은 미사용
};
