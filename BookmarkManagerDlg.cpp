#include "stdafx.h"
#include "BSFMApp.h"
#include "BookmarkManagerDlg.h"

BookmarkManagerDlg::BookmarkManagerDlg()
	: _bChanged(false)
	, _hwnd(nullptr)
	, _hTreeImage(nullptr)
	, _selectedBarIndex(-1)
	, _selectedItemIndex(-1)
	, _selectedItem(nullptr)
{
}

BookmarkManagerDlg::~BookmarkManagerDlg()
{
	if (_hTreeImage != nullptr)
	{
		ImageList_Destroy(_hTreeImage);
		_hTreeImage = nullptr;
	}
}

bool BookmarkManagerDlg::show(HWND hwndParent)
{
	for (int i = 1; i < BSFM_SHORTCUT_BAR_COUNT; ++i)
	{
		auto info = getApp()->getSettings().getShortcutInfo(i);
		_shortcutInfo[i].assign(info.begin(), info.end());
	}

	DialogBoxParam(getApp()->getHandle(), MAKEINTRESOURCE(IDD_BOOKMARK_MANAGER), hwndParent, BookmarkManagerDlg::_DlgProc, (LPARAM)this);
	return _bChanged;
}

std::vector<ShortcutInfo> const & BookmarkManagerDlg::getInfo(int barIndex)
{
	return _shortcutInfo[barIndex];
}

void BookmarkManagerDlg::_onInitDialog()
{
	_initLayout();
	_initTree();
}

void BookmarkManagerDlg::_onAdd()
{
	if (_selectedBarIndex < 1)
		return;
	ShortcutInfo info;
	if (!_updateData(true, info))
		return;
	_shortcutInfo[_selectedBarIndex].push_back(info);
	_insertTreeItem(_selectedBarIndex, _shortcutInfo[_selectedBarIndex].size() - 1, true);
	_bChanged = true;
}

void BookmarkManagerDlg::_onEdit()
{
	if (_selectedItemIndex < 0)
		return;

	ShortcutInfo info;
	if (!_updateData(true, info))
		return;
	_shortcutInfo[_selectedBarIndex][_selectedItemIndex] = info;
	_updateTreeItem();
	_bChanged = true;
}

void BookmarkManagerDlg::_onDelete()
{
	if (_selectedItemIndex < 0)
		return;

	auto info = _shortcutInfo[_selectedBarIndex][_selectedItemIndex];
	wchar_t msg[1024];
	swprintf_s(msg, getApp()->getLocalizeString(L"BookmarkManager", L"PromptDelete").c_str(), info.name.c_str());
	if (MessageBox(_hwnd, msg, getApp()->getName().c_str(), MB_YESNO | MB_ICONQUESTION) != IDYES)
		return;

	auto iter = _shortcutInfo[_selectedBarIndex].begin() + _selectedItemIndex;
	_shortcutInfo[_selectedBarIndex].erase(iter);

	HWND hwndTree = GetDlgItem(_hwnd, IDC_TREE);
	HTREEITEM hParent = TreeView_GetParent(hwndTree, _selectedItem);
	for (HTREEITEM hItem = TreeView_GetChild(hwndTree, hParent); hItem != nullptr; hItem = TreeView_GetNextSibling(hwndTree, hItem))
	{
		TVITEM tvi = {0};
		tvi.hItem = hItem;
		tvi.mask = TVIF_PARAM;
		tvi.lParam = -1;
		TreeView_GetItem(hwndTree, &tvi);
		if (tvi.lParam > _selectedItemIndex)
		{
			tvi.lParam -= 1;
			TreeView_SetItem(hwndTree, &tvi);
		}
	}
	TreeView_DeleteItem(hwndTree, _selectedItem);

	_bChanged = true;
}

void BookmarkManagerDlg::_onUp()
{
	if (_selectedItemIndex <= 0)
		return;

	HWND hwndTree = GetDlgItem(_hwnd, IDC_TREE);
	_swapData(_selectedBarIndex, TreeView_GetParent(hwndTree, _selectedItem), _selectedItemIndex - 1, _selectedItemIndex);
}

void BookmarkManagerDlg::_onDown()
{
	if (_selectedItemIndex < 0 || _selectedItemIndex >= (int)_shortcutInfo[_selectedBarIndex].size() - 1)
		return;

	HWND hwndTree = GetDlgItem(_hwnd, IDC_TREE);
	_swapData(_selectedBarIndex, TreeView_GetParent(hwndTree, _selectedItem), _selectedItemIndex + 1, _selectedItemIndex);
}

void BookmarkManagerDlg::_onTargetBrowse()
{
	HWND hwndTarget = GetDlgItem(_hwnd, IDC_TARGET);
	std::wstring curValue;
	getWindowText(hwndTarget, curValue);

	wchar_t path[1024] = {0};
	wcscpy_s(path, getApp()->getLocalizeString(L"BookmarkManager", L"FolderSelect").c_str());
	wcscat_s(path, L".");
	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = _hwnd;
	ofn.hInstance = getApp()->getHandle();
	ofn.lpstrFile = path;
	ofn.nMaxFile = 1024;
	ofn.lpstrTitle = getApp()->getLocalizeString(L"BookmarkManager", L"TargetBrowse").c_str();
	ofn.Flags = OFN_DONTADDTORECENT | OFN_ENABLESIZING | OFN_LONGNAMES | OFN_NONETWORKBUTTON | OFN_NOVALIDATE | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	if (curValue.length() > 0)
	{
		wchar_t D[_MAX_DRIVE] = {0};
		wchar_t d[_MAX_DIR] = {0};
		wchar_t f[_MAX_FNAME] = {0};
		wchar_t e[_MAX_EXT] = {0};
		_wsplitpath_s(curValue.c_str(), D, d, f, e);
		wchar_t p[_MAX_PATH] = {0};
		_wmakepath_s(p, D, d, nullptr, nullptr);
		if (wcslen(p) > 0)
			ofn.lpstrInitialDir = p;
	}
	if (GetOpenFileName(&ofn))
	{
		if (isFolder(path) || isFile(path))
		{
			SetWindowText(hwndTarget, path);
			SetFocus(hwndTarget);
		}
		else
		{
			path[ofn.nFileOffset] = 0;
			if (isFolder(path))
			{
				SetWindowText(hwndTarget, path);
				SetFocus(hwndTarget);
			}
		}
	}
}

void BookmarkManagerDlg::_onStartupFolderBrowse()
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

void BookmarkManagerDlg::_onTreeSelectChanged(TVITEM const & item)
{
	if (item.hItem != nullptr)
	{
		_selectedItem = item.hItem;
		if (item.lParam < 0)
		{
			// 구분용 루트 아이템이 선택됨
			_selectedBarIndex = -(int)(item.lParam);
			_selectedItemIndex = -1;
		}
		else
		{
			// 실제 즐겨찾기 아이템이 선택됨
			_selectedItemIndex = (int)(item.lParam);
			HWND hwndTree = GetDlgItem(_hwnd, IDC_TREE);
			TVITEM tvi = {0};
			tvi.hItem = TreeView_GetParent(hwndTree, item.hItem);
			tvi.mask = TVIF_PARAM;
			tvi.lParam = -1;
			TreeView_GetItem(hwndTree, &tvi);
			_selectedBarIndex = -(int)(tvi.lParam);

			_updateData(false, _shortcutInfo[_selectedBarIndex][_selectedItemIndex]);
		}
	}
	else
	{
		// 선택이 없다.
		_selectedItem = nullptr;
		_selectedBarIndex = -1;
		_selectedItemIndex = -1;
	}
}

void BookmarkManagerDlg::_initLayout()
{
	_mainLayout.setWindow(_hwnd);

	SendMessage(_hwnd, WM_SETFONT, (WPARAM)getApp()->getFont(), TRUE);
	SendMessage(_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)getApp()->getIcon());
	SendMessage(_hwnd, WM_SETICON, ICON_BIG, (LPARAM)getApp()->getIcon());
	SetClassLong(_hwnd, GCL_STYLE, GetClassLong(_hwnd, GCL_STYLE) | CS_HREDRAW | CS_VREDRAW);

	HWND hwndLabelName				= GetDlgItem(_hwnd, IDC_LABEL_NAME);
	HWND hwndName					= GetDlgItem(_hwnd, IDC_NAME);
	HWND hwndLabelTree				= GetDlgItem(_hwnd, IDC_LABEL_TREE);
	HWND hwndTree					= GetDlgItem(_hwnd, IDC_TREE);
	HWND hwndLabelTarget			= GetDlgItem(_hwnd, IDC_LABEL_TARGET);
	HWND hwndTarget					= GetDlgItem(_hwnd, IDC_TARGET);
	HWND hwndTargetBrowse			= GetDlgItem(_hwnd, IDC_TARGET_BROWSE);
	HWND hwndLabelParams			= GetDlgItem(_hwnd, IDC_LABEL_PARAMS);
	HWND hwndParams					= GetDlgItem(_hwnd, IDC_PARAMS);
	HWND hwndLabelStartupFolder		= GetDlgItem(_hwnd, IDC_LABEL_STARTUP_FOLDER);
	HWND hwndStartupFolder			= GetDlgItem(_hwnd, IDC_STARTUP_FOLDER);
	HWND hwndStartupFolderBrowse	= GetDlgItem(_hwnd, IDC_STARTUP_FOLDER_BROWSE);
	HWND hwndCheckAdmin				= GetDlgItem(_hwnd, IDC_CHECK_ADMIN);
	HWND hwndAdd					= GetDlgItem(_hwnd, IDC_ADD);
	HWND hwndEdit					= GetDlgItem(_hwnd, IDC_EDIT);
	HWND hwndDelete					= GetDlgItem(_hwnd, IDC_DELETE);
	HWND hwndUp						= GetDlgItem(_hwnd, IDC_UP);
	HWND hwndDown					= GetDlgItem(_hwnd, IDC_DOWN);
	HWND hwndClose					= GetDlgItem(_hwnd, IDCANCEL);

	SetWindowText(_hwnd, getApp()->getLocalizeString(L"BookmarkManager", L"Title").c_str());
	SetWindowText(hwndLabelTree, getApp()->getLocalizeString(L"BookmarkManager", L"Tree").c_str());
	SetWindowText(hwndLabelName, getApp()->getLocalizeString(L"BookmarkManager", L"Name").c_str());
	SetWindowText(hwndLabelTarget, getApp()->getLocalizeString(L"BookmarkManager", L"Target").c_str());
	SetWindowText(hwndLabelParams, getApp()->getLocalizeString(L"BookmarkManager", L"Params").c_str());
	SetWindowText(hwndLabelStartupFolder, getApp()->getLocalizeString(L"BookmarkManager", L"StartupFolder").c_str());
	SetWindowText(hwndCheckAdmin, getApp()->getLocalizeString(L"BookmarkManager", L"CheckAdmin").c_str());
	SetWindowText(hwndAdd, getApp()->getLocalizeString(L"BookmarkManager", L"Add").c_str());
	SetWindowText(hwndEdit, getApp()->getLocalizeString(L"BookmarkManager", L"Edit").c_str());
	SetWindowText(hwndDelete, getApp()->getLocalizeString(L"BookmarkManager", L"Delete").c_str());
	SetWindowText(hwndUp, getApp()->getLocalizeString(L"BookmarkManager", L"Up").c_str());
	SetWindowText(hwndDown, getApp()->getLocalizeString(L"BookmarkManager", L"Down").c_str());
	SetWindowText(hwndClose, getApp()->getLocalizeString(L"BookmarkManager", L"Close").c_str());

	int labelSize;
	int btSize;
	{
		SendMessage(hwndTarget, WM_SETFONT, (WPARAM)getApp()->getFont(), TRUE);
		labelSize = getApp()->getFontHeight();
		btSize = labelSize + 12;
	}

	{
		RECT rect;
		GetWindowRect(hwndClose, &rect);
		_closeLayout.add(FLayout::Control(hwndClose), FLayout::BOTTOM_RIGHT, 0, 0, rect.right - rect.left, rect.bottom - rect.top);
	}
	{
		int col[] = {
			TLayout::AUTO_SIZE, 3,		// 추가
			TLayout::AUTO_SIZE, 3,		// 수정
			TLayout::AUTO_SIZE			// 삭제
		};
		int row[] = {TLayout::AUTO_SIZE};
		_buttonLayout.resize(col, row, 0);
		_buttonLayout.add(TLayout::Control(hwndAdd), 0, 0);
		_buttonLayout.add(TLayout::Control(hwndEdit), 2, 0);
		_buttonLayout.add(TLayout::Control(hwndDelete), 4, 0);
	}
	{
		int col[] = {
			TLayout::AUTO_SIZE, 3,	// 위로
			TLayout::AUTO_SIZE		// 아래로
		};
		int row[] = {
			btSize, 3,				// 트리 레이블
			TLayout::AUTO_SIZE, 3,	// 트리
			btSize					// 위/아래 버튼
		};
		_leftLayout.resize(col, row, 0);
		_leftLayout.add(TLayout::Control(hwndLabelTree), 0, 0, 3, 1);
		_leftLayout.add(TLayout::Control(hwndTree), 0, 2, 3, 1);
		_leftLayout.add(TLayout::Control(hwndUp), 0, 4);
		_leftLayout.add(TLayout::Control(hwndDown), 2, 4);
	}
	{
		int col[] = {
			TLayout::AUTO_SIZE, 3,	// 입력
			btSize					// 찾기버튼
		};
		int row[] = {
			labelSize, btSize, 3,		// 이름
			labelSize, btSize, 3,		// 대상
			labelSize, btSize, 3,		// 명령 인수
			labelSize * 2, btSize, 3,	// 시작 위치
			btSize, 3,					// 관리자 권한 체크
			btSize,						// 추가/수정/삭제
			TLayout::AUTO_SIZE,			// 여백
			btSize						// 닫기
		};
		_rightLayout.resize(col, row, 0);
		int rIdx = 0;
		_rightLayout.add(TLayout::Control(hwndLabelName), 0, rIdx, 3);				rIdx += 1;
		_rightLayout.add(TLayout::Control(hwndName), 0, rIdx, 3);					rIdx += 2;
		_rightLayout.add(TLayout::Control(hwndLabelTarget), 0, rIdx, 3);			rIdx += 1;
		_rightLayout.add(TLayout::Control(hwndTarget), 0, rIdx);
		_rightLayout.add(TLayout::Control(hwndTargetBrowse), 2, rIdx);				rIdx += 2;
		_rightLayout.add(TLayout::Control(hwndLabelParams), 0, rIdx, 3);			rIdx += 1;
		_rightLayout.add(TLayout::Control(hwndParams), 0, rIdx, 3);					rIdx += 2;
		_rightLayout.add(TLayout::Control(hwndLabelStartupFolder), 0, rIdx, 3);		rIdx += 1;
		_rightLayout.add(TLayout::Control(hwndStartupFolder), 0, rIdx);
		_rightLayout.add(TLayout::Control(hwndStartupFolderBrowse), 2, rIdx);		rIdx += 2;
		_rightLayout.add(TLayout::Control(hwndCheckAdmin), 0, rIdx, 3);				rIdx += 2;
		_rightLayout.add(TLayout::Control(&_buttonLayout), 0, rIdx, 3);				rIdx += 2;
		_rightLayout.add(TLayout::Control(&_closeLayout), 0, rIdx, 3);				rIdx += 2;
	}
	{
		int col[] = {
			TLayout::AUTO_SIZE, 5,	// 트리
			TLayout::AUTO_SIZE		// 입력 컨트롤들
		};
		int row[] = {TLayout::AUTO_SIZE};
		_centerLayout.resize(col, row, 0);
		_centerLayout.add(TLayout::Control(&_leftLayout), 0, 0);
		_centerLayout.add(TLayout::Control(&_rightLayout), 2, 0);
	}
	{
		int col[] = {5, TLayout::AUTO_SIZE, 5};		// 좌우 여백
		int row[] = {5, TLayout::AUTO_SIZE, 5};		// 상하 여백
		_mainLayout.resize(col, row, 0);
		_mainLayout.add(TLayout::Control(&_centerLayout), 1, 1);
	}

	{
		_mainLayout.setFont(getApp()->getFont(), FALSE);
		_mainLayout.recalculate();
	}
}

void BookmarkManagerDlg::_initTree()
{
	if (_hTreeImage != nullptr)
	{
		ImageList_Destroy(_hTreeImage);
		_hTreeImage = nullptr;
	}

	_hTreeImage = ImageList_Create(16, 16, ILC_COLORDDB, 0, 1);
	int rootImageIndex = addIconToImageList(nullptr, _hTreeImage);

	HWND hwndTree = GetDlgItem(_hwnd, IDC_TREE);
	TreeView_SetImageList(hwndTree, _hTreeImage, TVSIL_NORMAL);

	HTREEITEM firstItem = nullptr;

	for (int i = 1; i < BSFM_SHORTCUT_BAR_COUNT; ++i)
	{
		wchar_t buf[1024];
		swprintf_s(buf, getApp()->getLocalizeString(L"BookmarkManager", L"BookmarkFormat").c_str(), i + 1);
		TVINSERTSTRUCT tvis = {0};
		tvis.hParent = TVI_ROOT;
		tvis.hInsertAfter = TVI_LAST;
		tvis.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_STATE | TVIF_TEXT;
		tvis.item.state = TVIS_EXPANDED;
		tvis.item.stateMask = TVIS_EXPANDED;
		tvis.item.pszText = buf;
		tvis.item.cchTextMax = int(wcslen(buf) + 1);
		tvis.item.iImage = rootImageIndex;
		tvis.item.iSelectedImage = rootImageIndex;
		tvis.item.lParam = -i;
		HTREEITEM ret = TreeView_InsertItem(hwndTree, &tvis);
		if (firstItem == nullptr)
			firstItem = ret;

		for (size_t iter = 0; iter < _shortcutInfo[i].size(); ++iter)
			_insertTreeItem(i, iter, false);
	}

	TreeView_SelectItem(hwndTree, firstItem);
}

void BookmarkManagerDlg::_insertTreeItem(int barIndex, size_t itemIndex, bool bSelect)
{
	auto info = _shortcutInfo[barIndex][itemIndex];
	HICON hIcon = nullptr;
	std::wstring name;
	getShellIcon(info.getTarget(), hIcon, name);
	int imageIndex = addIconToImageList(hIcon, _hTreeImage);
	if (hIcon != nullptr)
		DestroyIcon(hIcon);

	HWND hwndTree = GetDlgItem(_hwnd, IDC_TREE);

	for (HTREEITEM hitem = TreeView_GetRoot(hwndTree); hitem != nullptr; hitem = TreeView_GetNextSibling(hwndTree, hitem))
	{
		TVITEM item = {0};
		item.hItem = hitem;
		item.mask = TVIF_PARAM;
		item.lParam = -1;
		TreeView_GetItem(hwndTree, &item);
		if (-item.lParam == barIndex)
		{
			wchar_t buf[1024];
			wcscpy_s(buf, info.name.c_str());
			TVINSERTSTRUCT tvis = {0};
			tvis.hParent = hitem;
			tvis.hInsertAfter = TVI_LAST;
			tvis.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_TEXT;
			tvis.item.pszText = buf;
			tvis.item.cchTextMax = int(wcslen(buf) + 1);
			tvis.item.iImage = imageIndex;
			tvis.item.iSelectedImage = imageIndex;
			tvis.item.lParam = (LPARAM)itemIndex;
			HTREEITEM ret = TreeView_InsertItem(hwndTree, &tvis);
			if (bSelect)
				TreeView_SelectItem(hwndTree, ret);
			break;
		}
	}
}

void BookmarkManagerDlg::_updateTreeItem()
{
	auto info = _shortcutInfo[_selectedBarIndex][_selectedItemIndex];
	HICON hIcon = nullptr;
	std::wstring name;
	getShellIcon(info.getTarget(), hIcon, name);
	int imageIndex = addIconToImageList(hIcon, _hTreeImage);
	if (hIcon != nullptr)
		DestroyIcon(hIcon);

	HWND hwndTree = GetDlgItem(_hwnd, IDC_TREE);
	wchar_t buf[1024];
	wcscpy_s(buf, info.name.c_str());
	TVITEM item = {0};
	item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_TEXT;
	item.hItem = _selectedItem;
	item.pszText = buf;
	item.cchTextMax = int(wcslen(buf) + 1);
	item.iImage = imageIndex;
	item.iSelectedImage = imageIndex;
	item.lParam = (LPARAM)_selectedItemIndex;
	TreeView_SetItem(hwndTree, &item);
}

bool BookmarkManagerDlg::_updateData(bool bSaveValidate, ShortcutInfo& info)
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

void BookmarkManagerDlg::_swapData(int barIndex, HTREEITEM hItemBar, int itemIndex1, int itemIndex2)
{
	ShortcutInfo info1 = _shortcutInfo[barIndex][itemIndex1];
	ShortcutInfo info2 = _shortcutInfo[barIndex][itemIndex2];

	HWND hwndTree = GetDlgItem(_hwnd, IDC_TREE);
	HTREEITEM hItem1 = nullptr;
	HTREEITEM hItem2 = nullptr;
	for (HTREEITEM hItem = TreeView_GetChild(hwndTree, hItemBar); hItem != nullptr; hItem = TreeView_GetNextSibling(hwndTree, hItem))
	{
		TVITEM tvi = {0};
		tvi.hItem = hItem;
		tvi.mask = TVIF_PARAM;
		tvi.lParam = -1;
		TreeView_GetItem(hwndTree, &tvi);
		if (tvi.lParam == itemIndex1)
			hItem1 = hItem;
		else if (tvi.lParam == itemIndex2)
			hItem2 = hItem;
		if (hItem1 != nullptr && hItem2 != nullptr)
			break;
	}

	TVITEM tvi = {0};
	tvi.hItem = hItem1;
	tvi.mask = TVIF_IMAGE;
	TreeView_GetItem(hwndTree, &tvi);
	int image1 = tvi.iImage;

	tvi.hItem = hItem2;
	tvi.mask = TVIF_IMAGE;
	TreeView_GetItem(hwndTree, &tvi);
	int image2 = tvi.iImage;

	_shortcutInfo[barIndex][itemIndex1] = info2;
	_shortcutInfo[barIndex][itemIndex2] = info1;

	wchar_t buf[1024];
	wcscpy_s(buf, info1.name.c_str());
	tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_TEXT;
	tvi.hItem = hItem2;
	tvi.pszText = buf;
	tvi.cchTextMax = int(wcslen(buf) + 1);
	tvi.iImage = image1;
	tvi.iSelectedImage = image1;
	tvi.lParam = (LPARAM)itemIndex2;
	TreeView_SetItem(hwndTree, &tvi);

	wcscpy_s(buf, info2.name.c_str());
	tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_TEXT;
	tvi.hItem = hItem1;
	tvi.pszText = buf;
	tvi.cchTextMax = int(wcslen(buf) + 1);
	tvi.iImage = image2;
	tvi.iSelectedImage = image2;
	tvi.lParam = (LPARAM)itemIndex1;
	TreeView_SetItem(hwndTree, &tvi);

	TreeView_SelectItem(hwndTree, hItem1);

	_bChanged = true;
}

void BookmarkManagerDlg::_errorBox(HWND hwnd, std::wstring const & msg)
{
	MessageBox(_hwnd, getApp()->getLocalizeString(L"BookmarkManager", msg).c_str(), getApp()->getName().c_str(), MB_OK | MB_ICONERROR);
	SetFocus(hwnd);
}

INT_PTR CALLBACK BookmarkManagerDlg::_DlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	BookmarkManagerDlg* pThis = (BookmarkManagerDlg*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (msg)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lparam);
		pThis = (BookmarkManagerDlg*)lparam;
		pThis->_hwnd = hwnd;
		pThis->_onInitDialog();
		break;

	case WM_COMMAND:
		if (HIWORD(wparam) == BN_CLICKED)
		{
			switch (LOWORD(wparam))
			{
			case IDCANCEL:
				EndDialog(hwnd, IDCANCEL);
				break;

			case IDC_ADD:
				pThis->_onAdd();
				break;

			case IDC_EDIT:
				pThis->_onEdit();
				break;

			case IDC_DELETE:
				pThis->_onDelete();
				break;

			case IDC_UP:
				pThis->_onUp();
				break;

			case IDC_DOWN:
				pThis->_onDown();
				break;

			case IDC_TARGET_BROWSE:
				pThis->_onTargetBrowse();
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

	case WM_NOTIFY:
		{
			auto nmhdr = (NMHDR*)lparam;
			switch (nmhdr->idFrom)
			{
			case IDC_TREE:
				if (nmhdr->code == TVN_SELCHANGED)
				{
					auto nmtree = (NMTREEVIEW*)nmhdr;
					pThis->_onTreeSelectChanged(nmtree->itemNew);
				}
				else
				{
					return FALSE;
				}
				break;
			default:
				return FALSE;
			}
		}
		break;

	case WM_SIZE:
		if (wparam != SIZE_MINIMIZED)
			pThis->_mainLayout.recalculate();
		break;

	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lparam)->ptMinTrackSize.x = 400;
		((MINMAXINFO*)lparam)->ptMinTrackSize.y = 300;
		break;

	default:
		return FALSE;
	}
	return TRUE;
}