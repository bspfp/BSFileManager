#include "stdafx.h"
#include "BSFMApp.h"
#include "FindFileDlg.h"

FindFileDlg::FindFileDlg()
	: _hwnd(nullptr)
	, _hImageList(nullptr)
	, _work(nullptr)
{
}

FindFileDlg::~FindFileDlg()
{
	_stopAndWaitFinding();
	if (_hwnd != nullptr)
	{
		DestroyWindow(_hwnd);
		_hwnd = nullptr;
	}
	if (_hImageList != nullptr)
	{
		ImageList_Destroy(_hImageList);
		_hImageList = nullptr;
	}
}

void FindFileDlg::show(std::wstring const & destDir)
{
	if (_hwnd != nullptr)
	{
		WINDOWPLACEMENT wp = {0};
		wp.length = sizeof(wp);
		GetWindowPlacement(_hwnd, &wp);
		switch (wp.showCmd)
		{
		case SW_HIDE:
			ShowWindow(_hwnd, SW_SHOW);
			break;
		case SW_MINIMIZE:
		case SW_SHOWMINIMIZED:
		case SW_SHOWMINNOACTIVE:
			ShowWindow(_hwnd, SW_RESTORE);
			break;
		default:
			break;
		}
		SetForegroundWindow(_hwnd);
		if (_work != nullptr)
			return;

		_param.destDir = destDir;
		SetDlgItemText(_hwnd, IDC_DEST, _param.destDir.c_str());
		SetFocus(GetDlgItem(_hwnd, IDC_PATTERN));
		return;
	}
	_param.destDir = destDir;
	CreateDialogParam(getApp()->getHandle(), MAKEINTRESOURCE(IDD_FINDFILE), nullptr, _DlgProc, (LPARAM)this);
	ShowWindow(_hwnd, SW_SHOW);
}

bool FindFileDlg::isDialogMsg(MSG* pMsg)
{
	if (_hwnd == nullptr)
		return false;
	return (IsDialogMessage(_hwnd, pMsg)) ? true : false;
}

void FindFileDlg::_startFind()
{
	SetDlgItemText(_hwnd, IDC_FIND, getApp()->getLocalizeString(L"FindFile", L"Stop").c_str());
	if (_work == nullptr)
	{
		std::wstring destDir;
		getWindowText(GetDlgItem(_hwnd, IDC_DEST), destDir);
		if (isFolder(destDir))
		{
			_param.destDir = destDir;
			getWindowText(GetDlgItem(_hwnd, IDC_PATTERN), _param.pattern);
			_param.bUseRegEx = (SendDlgItemMessage(_hwnd, IDC_USE_REGEX, BM_GETCHECK, 0, 0) == BST_CHECKED);
			_param.hwndDlg = _hwnd;
			_param.stopFlag = false;
			{
				SYNC(_param.lockobj);
				_param.result.clear();
			}
			_work = getApp()->createWork(_FindCallback, &_param);
			if (_work != nullptr)
			{
				while (SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_GETITEMCOUNT, 0, 0) > 0)
					SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_DELETEITEM, 0, 0);
				if (_hImageList != nullptr)
				{
					ImageList_Destroy(_hImageList);
					_hImageList = nullptr;
				}

				_hImageList = ImageList_Create(16, 16, ILC_COLORDDB, 0, 1);
				SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)_hImageList);
				SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)_hImageList);
				SubmitThreadpoolWork(_work);
			}
			else
			{
				MessageBox(_hwnd, getApp()->getLocalizeString(L"FindFile", L"Error_Failed").c_str(), getApp()->getName().c_str(), MB_OK | MB_ICONERROR);
				SetDlgItemText(_hwnd, IDC_FIND, getApp()->getLocalizeString(L"FindFile", L"Find").c_str());
				return;
			}
		}
		else
		{
			MessageBox(_hwnd, getApp()->getLocalizeString(L"FindFile", L"Error_InvalidDestFolder").c_str(), getApp()->getName().c_str(), MB_OK | MB_ICONERROR);
			SetDlgItemText(_hwnd, IDC_FIND, getApp()->getLocalizeString(L"FindFile", L"Find").c_str());
			SetFocus(GetDlgItem(_hwnd, IDC_DEST));
			return;
		}
	}
}

void FindFileDlg::_stopFinding()
{
	_stopAndWaitFinding();
	SetDlgItemText(_hwnd, IDC_FIND, getApp()->getLocalizeString(L"FindFile", L"Find").c_str());
}

void FindFileDlg::_stopAndWaitFinding()
{
	if (_work != nullptr)
	{
		_param.stopFlag = true;
		WaitForThreadpoolWorkCallbacks(_work, TRUE);
		_work = nullptr;
	}
}

void FindFileDlg::_enableActionButtons(BOOL bEnable, bool bMultiSelect)
{
	EnableWindow(GetDlgItem(_hwnd, IDC_GOTO), (bEnable && !bMultiSelect) ? TRUE : FALSE);
	EnableWindow(GetDlgItem(_hwnd, IDC_OPEN), bEnable);
	EnableWindow(GetDlgItem(_hwnd, IDC_COPY_PATH), bEnable);
	EnableWindow(GetDlgItem(_hwnd, IDC_DELETE), bEnable);
	EnableWindow(GetDlgItem(_hwnd, IDC_COPY), bEnable);
}

void FindFileDlg::_onInitDialog()
{
	SendMessage(_hwnd, WM_SETFONT, (WPARAM)getApp()->getFont(), TRUE);

	SetDlgItemText(_hwnd, IDC_LABEL_PATTERN, getApp()->getLocalizeString(L"FindFile", L"Pattern").c_str());
	SetDlgItemText(_hwnd, IDC_USE_REGEX, getApp()->getLocalizeString(L"FindFile", L"UseRegEx").c_str());
	SetDlgItemText(_hwnd, IDC_LABEL_DEST, getApp()->getLocalizeString(L"FindFile", L"Dest").c_str());
	SetDlgItemText(_hwnd, IDC_DEST, _param.destDir.c_str());
	SetDlgItemText(_hwnd, IDC_FIND, getApp()->getLocalizeString(L"FindFile", L"Find").c_str());
	SetDlgItemText(_hwnd, IDC_GOTO, getApp()->getLocalizeString(L"FindFile", L"Goto").c_str());
	SetDlgItemText(_hwnd, IDC_OPEN, getApp()->getLocalizeString(L"FindFile", L"Open").c_str());
	SetDlgItemText(_hwnd, IDC_COPY_PATH, getApp()->getLocalizeString(L"FindFile", L"CopyPath").c_str());
	SetDlgItemText(_hwnd, IDC_DELETE, getApp()->getLocalizeString(L"FindFile", L"Delete").c_str());
	SetDlgItemText(_hwnd, IDC_COPY, getApp()->getLocalizeString(L"FindFile", L"Copy").c_str());
	SetDlgItemText(_hwnd, IDCANCEL, getApp()->getLocalizeString(L"FindFile", L"Close").c_str());
	SetDlgItemText(_hwnd, IDC_LABEL_RESULT, getApp()->getLocalizeString(L"FindFile", L"Result").c_str());

	SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	_enableActionButtons(FALSE);

	LVCOLUMN lvc;
	memset(&lvc, 0, sizeof(lvc));
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_MINWIDTH;
	lvc.fmt = LVCFMT_LEFT;
	lvc.cxMin = 30;

	auto insertCol = [&,this](int idx, const wchar_t* colName){
		int col[6];
		getApp()->getSettings().getFindFileColumnInfo(col);
		std::wstring str = getApp()->getLocalizeString(L"FindFile", colName);
		lvc.cx = col[idx];
		lvc.pszText = (wchar_t*)(str.c_str());
		lvc.iSubItem = idx;
		SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_INSERTCOLUMN, lvc.iSubItem, (LPARAM)&lvc);
	};

	insertCol(0, L"Col_Name");
	insertCol(1, L"Col_Folder");
	insertCol(2, L"Col_Type");
	insertCol(3, L"Col_Time");
	insertCol(4, L"Col_Attr");
	insertCol(5, L"Col_Size");

	SendDlgItemMessage(_hwnd, IDC_USE_REGEX, WM_SETFONT, (WPARAM)getApp()->getFont(), TRUE);
	SIZE sizeUseRegEx = {0};
	if (!SendDlgItemMessage(_hwnd, IDC_USE_REGEX, BCM_GETIDEALSIZE, 0, (LPARAM)&sizeUseRegEx))
	{
		RECT rect;
		GetWindowRect(GetDlgItem(_hwnd, IDC_USE_REGEX), &rect);
		sizeUseRegEx.cx = rect.right - rect.left;
		sizeUseRegEx.cy = rect.bottom - rect.top;
	}
	SendDlgItemMessage(_hwnd, IDC_DEST_BROWSE, WM_SETFONT, (WPARAM)getApp()->getFont(), TRUE);
	SIZE sizeBrowse = {0};
	if (!SendDlgItemMessage(_hwnd, IDC_DEST_BROWSE, BCM_GETIDEALSIZE, 0, (LPARAM)&sizeBrowse))
	{
		RECT rect;
		GetWindowRect(GetDlgItem(_hwnd, IDC_DEST_BROWSE), &rect);
		sizeBrowse.cx = rect.right - rect.left;
		sizeBrowse.cy = rect.bottom - rect.top;
	}
	SIZE sizeFind = {0};
	{
		RECT rect;
		GetWindowRect(GetDlgItem(_hwnd, IDC_FIND), &rect);
		sizeFind.cx = rect.right - rect.left;
		sizeFind.cy = rect.bottom - rect.top;
	}

	int labelHeight = getApp()->getFontHeight();

	{
		int col[] = {TLayout::AUTO_SIZE, 3, sizeUseRegEx.cx - sizeBrowse.cx - 3, 3, sizeBrowse.cx};
		int row[] = {labelHeight, sizeBrowse.cy, 3,
			labelHeight, sizeBrowse.cy, 3,
			labelHeight, TLayout::AUTO_SIZE
		};
		_leftLayout.resize(col, row, 0);
		_leftLayout.add(TLayout::Control(GetDlgItem(_hwnd, IDC_LABEL_PATTERN)), 0, 0, 5);
		_leftLayout.add(TLayout::Control(GetDlgItem(_hwnd, IDC_PATTERN)), 0, 1, 1);
		_leftLayout.add(TLayout::Control(GetDlgItem(_hwnd, IDC_USE_REGEX)), 2, 1, 3);
		_leftLayout.add(TLayout::Control(GetDlgItem(_hwnd, IDC_LABEL_DEST)), 0, 3, 5);
		_leftLayout.add(TLayout::Control(GetDlgItem(_hwnd, IDC_DEST)), 0, 4, 3);
		_leftLayout.add(TLayout::Control(GetDlgItem(_hwnd, IDC_DEST_BROWSE)), 4, 4, 1);
		_leftLayout.add(TLayout::Control(GetDlgItem(_hwnd, IDC_LABEL_RESULT)), 0, 6, 5);
		_leftLayout.add(TLayout::Control(GetDlgItem(_hwnd, IDC_RESULT)), 0, 7, 5);
	}

	{
		int gotoY = labelHeight * 3 + sizeBrowse.cy * 2 + 6;
		_rightLayout.add(FLayout::Control(GetDlgItem(_hwnd, IDC_FIND)), FLayout::TOP_RIGHT, 0, 0, sizeFind.cx, sizeFind.cy);
		_rightLayout.add(FLayout::Control(GetDlgItem(_hwnd, IDC_GOTO)), FLayout::TOP_RIGHT, 0, gotoY, sizeFind.cx, sizeFind.cy);
		_rightLayout.add(FLayout::Control(GetDlgItem(_hwnd, IDC_OPEN)), FLayout::TOP_RIGHT, 0, gotoY + (3 + sizeFind.cy) * 1, sizeFind.cx, sizeFind.cy);
		_rightLayout.add(FLayout::Control(GetDlgItem(_hwnd, IDC_COPY_PATH)), FLayout::TOP_RIGHT, 0, gotoY + (3 + sizeFind.cy) * 2, sizeFind.cx, sizeFind.cy);
		_rightLayout.add(FLayout::Control(GetDlgItem(_hwnd, IDC_DELETE)), FLayout::TOP_RIGHT, 0, gotoY + (3 + sizeFind.cy) * 3, sizeFind.cx, sizeFind.cy);
		_rightLayout.add(FLayout::Control(GetDlgItem(_hwnd, IDC_COPY)), FLayout::TOP_RIGHT, 0, gotoY + (3 + sizeFind.cy) * 4, sizeFind.cx, sizeFind.cy);
		_rightLayout.add(FLayout::Control(GetDlgItem(_hwnd, IDCANCEL)), FLayout::BOTTOM_RIGHT, 0, 0, sizeFind.cx, sizeFind.cy);
	}

	{
		int col[] = {5, TLayout::AUTO_SIZE, 10, sizeFind.cx, 5};
		int row[] = {5, TLayout::AUTO_SIZE, 5};
		_mainLayout.resize(col, row, 0);
		_mainLayout.add(TLayout::Control(&_leftLayout), 1, 1);
		_mainLayout.add(TLayout::Control(&_rightLayout), 3, 1);
		_mainLayout.setWindow(_hwnd, true);
		_mainLayout.recalculate();
	}

	SetWindowPos(_hwnd, nullptr, 0, 0, 670, 450, SWP_NOZORDER | SWP_NOMOVE);
}

void FindFileDlg::_onFindFileResult()
{
	SYNC(_param.lockobj);
	while (!_param.result.empty())
	{
		auto iter		= _param.result.begin();
		auto fullpath	= std::get<0>(*iter);
		auto attr		= std::get<1>(*iter);
		auto writeTime	= std::get<2>(*iter);
		auto size		= std::get<3>(*iter);

		wchar_t D[_MAX_DRIVE] = {0};
		wchar_t d[_MAX_DIR] = {0};
		wchar_t f[_MAX_FNAME] = {0};
		wchar_t e[_MAX_EXT] = {0};
		_wsplitpath_s(fullpath.c_str(), D, d, f, e);

		wchar_t folder[_MAX_PATH] = {0};
		_wmakepath_s(folder, D, d, nullptr, nullptr);

		wchar_t filename[_MAX_PATH] = {0};
		_wmakepath_s(filename, nullptr, nullptr, f, e);

		SHFILEINFO shfi = {0};
		SHGetFileInfo(fullpath.c_str(), 0, &shfi, sizeof(shfi), SHGFI_TYPENAME | SHGFI_ICON | SHGFI_SMALLICON);
		int imageIndex = addIconToImageList(shfi.hIcon, _hImageList);
		if (shfi.hIcon != nullptr)
			DestroyIcon(shfi.hIcon);

		tm tmWrite = {0};
		_localtime64_s(&tmWrite, &writeTime);
		wchar_t timestring[32] = {0};
		swprintf_s(timestring, L"%04d-%02d-%02d %02d:%02d:%02d", tmWrite.tm_year + 1900, tmWrite.tm_mon + 1, tmWrite.tm_mday, tmWrite.tm_hour, tmWrite.tm_min, tmWrite.tm_sec);

		wchar_t attrstring[6] = {0};
		if ((attr & _A_ARCH) != 0)
			attrstring[0] = L'A';
		else
			attrstring[0] = L'_';
		if ((attr & _A_RDONLY) != 0)
			attrstring[1] = L'R';
		else
			attrstring[1] = L'_';
		if ((attr & _A_HIDDEN) != 0)
			attrstring[2] = L'H';
		else
			attrstring[2] = L'_';
		if ((attr & _A_SYSTEM) != 0)
			attrstring[3] = L'S';
		else
			attrstring[3] = L'_';

		wchar_t sizestring[32] = {0};
		long long nSize = size;
		int sizeUnit = 0; // 0: byte, 1: KiB, 2: MiB, 3: GiB, 4: TiB
		while (nSize > 1024 && sizeUnit < 4)
		{
			++sizeUnit;
			nSize >>= 10;
		}
		swprintf_s(sizestring, L"%lld", nSize);
		switch (sizeUnit)
		{
		case 1:		wcscat_s(sizestring, L" KiB");		break;
		case 2:		wcscat_s(sizestring, L" MiB");		break;
		case 3:		wcscat_s(sizestring, L" GiB");		break;
		case 4:		wcscat_s(sizestring, L" TiB");		break;
		}

		int itemIndex = -1;
		{
			LVITEM lvi = {0};
			lvi.mask = LVIF_IMAGE | LVIF_TEXT;
			lvi.iItem = INT_MAX;
			lvi.pszText = filename;
			lvi.iImage = imageIndex;
			itemIndex = (int)SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_INSERTITEM, 0, (LPARAM)&lvi);
		}
		{
			LVITEM lvi = {0};
			lvi.iSubItem = 1;
			lvi.pszText = folder;
			SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_SETITEMTEXT, itemIndex, (LPARAM)&lvi);
		}
		{
			LVITEM lvi = {0};
			lvi.iSubItem = 2;
			lvi.pszText = shfi.szTypeName;
			SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_SETITEMTEXT, itemIndex, (LPARAM)&lvi);
		}
		{
			LVITEM lvi = {0};
			lvi.iSubItem = 3;
			lvi.pszText = timestring;
			SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_SETITEMTEXT, itemIndex, (LPARAM)&lvi);
		}
		{
			LVITEM lvi = {0};
			lvi.iSubItem = 4;
			lvi.pszText = attrstring;
			SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_SETITEMTEXT, itemIndex, (LPARAM)&lvi);
		}
		{
			LVITEM lvi = {0};
			lvi.iSubItem = 5;
			lvi.pszText = sizestring;
			SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_SETITEMTEXT, itemIndex, (LPARAM)&lvi);
		}

		_param.result.pop_front();
	}
}

void FindFileDlg::_onClose()
{
	int col[6];
	for (int i = 0; i < _countof(col); ++i)
		col[i] = (int)SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_GETCOLUMNWIDTH, i, 0);
	getApp()->getSettings().setFindFileColumnInfo(col);
	DestroyWindow(_hwnd);
	_hwnd = nullptr;
}

void FindFileDlg::_onResultItemChanged()
{
	LRESULT ret = SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_GETSELECTEDCOUNT, 0, 0);
	_enableActionButtons((ret > 0) ? TRUE : FALSE, (ret > 1) ? true : false);
}

void FindFileDlg::_onGoto()
{
	LRESULT ret = SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_GETNEXTITEM, (WPARAM)-1, (LPARAM)LVNI_SELECTED);
	if (ret >= 0)
	{
		wchar_t txtFile[1024] = {0};
		wchar_t txtFolder[1024] = {0};
		LVITEM lvi1;
		LVITEM lvi2;
		lvi1.iSubItem = 0;
		lvi1.pszText = txtFile;
		lvi1.cchTextMax = 1024;
		lvi2.iSubItem = 1;
		lvi2.pszText = txtFolder;
		lvi2.cchTextMax = 1024;
		SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_GETITEMTEXT, ret, (LPARAM)&lvi1);
		SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_GETITEMTEXT, ret, (LPARAM)&lvi2);
		SendMessage(getApp()->getMainWindow(), WM_FIND_FILE_GOTO, (WPARAM)txtFolder, (LPARAM)txtFile);
	}
}

void FindFileDlg::_onDestBrowse()
{
	wchar_t dispName[MAX_PATH];
	std::wstring title(getApp()->getLocalizeString(L"FindFile", L"DestFolderBrowse"));
	HWND hwnd = GetDlgItem(_hwnd, IDC_DEST);
	std::wstring curVal;
	getWindowText(hwnd, curVal);
	PIDLIST_ABSOLUTE pidl = browseForFolder(_hwnd, dispName, title.c_str(), curVal.c_str(), true);
	if (pidl != nullptr)
	{
		std::wstring parsingName;
		idListToString(pidl, &parsingName);
		ILFree(pidl);

		SetWindowText(hwnd, parsingName.c_str());
		SetFocus(hwnd);
	}
}

void FindFileDlg::_onOpen()
{
	int idx = -1;
	for (;;)
	{
		idx = (int)SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_GETNEXTITEM, (WPARAM)idx, (LPARAM)LVNI_SELECTED);
		if (idx >= 0)
		{
			wchar_t txtFile[1024] = {0};
			wchar_t txtFolder[1024] = {0};
			LVITEM lvi1;
			LVITEM lvi2;
			lvi1.iSubItem = 0;
			lvi1.pszText = txtFile;
			lvi1.cchTextMax = 1024;
			lvi2.iSubItem = 1;
			lvi2.pszText = txtFolder;
			lvi2.cchTextMax = 1024;
			SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_GETITEMTEXT, (WPARAM)idx, (LPARAM)&lvi1);
			SendDlgItemMessage(_hwnd, IDC_RESULT, LVM_GETITEMTEXT, (WPARAM)idx, (LPARAM)&lvi2);
			std::wstring target(txtFolder);
			target.append(txtFile);
			if (0 != ShellExecute(getApp()->getMainWindow(), nullptr, target.c_str(), L"", txtFolder, SW_SHOWDEFAULT))
			{
				// TODO : 에러를 문자열로 변환해서 출력하기
			}
		}
		else
		{
			break;
		}
	}
}

INT_PTR FindFileDlg::_DlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	auto pThis = (FindFileDlg*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (msg)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lparam);
		pThis = (FindFileDlg*)lparam;
		pThis->_hwnd = hwnd;
		pThis->_onInitDialog();
		break;
	case WM_SIZE:
		if (wparam != SIZE_MINIMIZED)
			pThis->_mainLayout.recalculate();
		break;
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lparam)->ptMinTrackSize.x = 670;
		((MINMAXINFO*)lparam)->ptMinTrackSize.y = 450;
		break;
	case WM_CLOSE:
		pThis->_onClose();
		break;
	case WM_COMMAND:
		if (HIWORD(wparam) == BN_CLICKED)
		{
			switch (LOWORD(wparam))
			{
			case IDC_DEST_BROWSE:
				pThis->_onDestBrowse();
				break;
			case IDC_FIND:
				if (pThis->_work == nullptr)
					pThis->_startFind();
				else
					pThis->_stopFinding();
				break;
			case IDC_GOTO:
				pThis->_onGoto();
				break;
			case IDC_OPEN:
				pThis->_onOpen();
				break;
			case IDC_COPY_PATH:
				// TODO : 
				break;
			case IDC_DELETE:
				// TODO : 
				break;
			case IDC_COPY:
				// TODO : 
				break;
			case IDCANCEL:
				pThis->_onClose();
				break;
			default:
				return FALSE;
			}
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	case WM_NOTIFY:
		{
			auto nmhdr = (NMHDR*)lparam;
			switch (nmhdr->code)
			{
			case LVN_ITEMCHANGED:
				if (nmhdr->idFrom == IDC_RESULT)
				{
					pThis->_onResultItemChanged();
				}
				else
				{
					return FALSE;
				}
				break;
			default:
				return FALSE;
			}
			return TRUE;
		}

	case WM_FIND_FILE_END:
		pThis->_stopFinding();
		break;

	case WM_FIND_FILE_NOT_FOUND:
		MessageBox(hwnd, getApp()->getLocalizeString(L"FindFile", L"Error_NotFound").c_str(), getApp()->getName().c_str(), MB_OK | MB_ICONINFORMATION);
		pThis->_stopFinding();
		break;

	case WM_FIND_FILE_RESULT:
		pThis->_onFindFileResult();
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

VOID CALLBACK FindFileDlg::_FindCallback(PTP_CALLBACK_INSTANCE inst, PVOID context, PTP_WORK)
{
	CallbackMayRunLong(inst);
	auto param = (FindFileParam*)context;

	std::wregex re;
	re.imbue(std::locale(std::locale::classic(), ".OCP", std::locale::ctype | std::locale::collate));
	if (param->bUseRegEx)
	{
		re.assign(param->pattern);
	}
	else
	{
		std::wstring modPattern;
		for (auto iter = param->pattern.begin(); iter != param->pattern.end(); ++iter)
		{
			// 정규식으로 변환
			// 예외 처리: (){}[]\^$|+.
			// 와일드 카드: *?
			switch (*iter)
			{
			case L'(':
			case L')':
			case L'{':
			case L'}':
			case L'[':
			case L']':
			case L'\\':
			case L'^':
			case L'$':
			case L'|':
			case L'+':
			case L'.':
				modPattern += L'\\';
				modPattern += *iter;
				break;

			case L'*':
				modPattern += L".*";
				break;
			case L'?':
				modPattern += L'.';
				break;

			default:
				modPattern += *iter;
				break;
			}
		}
		re.assign(modPattern);
	}

	bool bFound = false;
	std::list<std::wstring> findTargets;
	findTargets.push_back(param->destDir);
	while (!param->stopFlag && !findTargets.empty())
	{
		std::wstring folder = *(findTargets.begin());
		findTargets.pop_front();

		if (folder.rbegin() != folder.rend() && *(folder.rbegin()) != L'\\')
			folder.append(L"\\");
		std::wstring findname = folder;
		findname.append(L"*.*");
		_wfinddata64_t fi = {0};
		intptr_t fh = _wfindfirst64(findname.c_str(), &fi);
		if (fh == -1)
			continue;

		auto waitForUI = [&](){
			while (!param->stopFlag)
			{
				param->lockobj.lock();
				if (param->result.empty())
				{
					param->lockobj.unlock();
					break;
				}
				param->lockobj.unlock();
				::Sleep(0);
			}
		};

		while (!param->stopFlag)
		{
			if (wcscmp(fi.name, L".") != 0 && wcscmp(fi.name, L"..") != 0)
			{
				if (std::regex_search(fi.name, re, std::regex_constants::match_any))
				{
					std::wstring fullpath = folder + fi.name;
					SYNC(param->lockobj);
					param->result.push_back(std::make_tuple(fullpath, fi.attrib, fi.time_write, fi.size));
					PostMessage(param->hwndDlg, WM_FIND_FILE_RESULT, 0, 0);
					bFound = true;
				}
				if ((fi.attrib & _A_SUBDIR) != 0)
					findTargets.push_back(folder + fi.name);
			}
			if (_wfindnext64(fh, &fi) != 0)
			{
				_findclose(fh);
				break;
			}
			param->lockobj.lock();
			if (param->result.size() >10)
			{
				param->lockobj.unlock();
				waitForUI();
			}
			else
			{
				param->lockobj.unlock();
			}
		}
		waitForUI();
	}
	if (bFound)
		PostMessage(param->hwndDlg, WM_FIND_FILE_END, 0, 0);
	else
		PostMessage(param->hwndDlg, WM_FIND_FILE_NOT_FOUND, 0, 0);
}
