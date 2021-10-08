#include "stdafx.h"
#include "BSFMApp.h"

static BSFMApp* g_TheApp = nullptr;

namespace
{
	enum BSFMAPP_STATE
	{
		NOT_INITIALIZED		= -1,
		END_RUN				= 0,
		INITIALIZED			= 1,
		RUNNING				= 2,
	};

	struct BFFParam
	{
		const void* pData;
		bool isString;
	};

	int CALLBACK _BFFProc(HWND hwnd, UINT msg, LPARAM lparam, LPARAM data)
	{
		lparam;
		if (msg == BFFM_INITIALIZED)
		{
			auto pData = (BFFParam*)data;
			if (pData != nullptr)
				SendMessage(hwnd, BFFM_SETSELECTION, (pData->isString) ? TRUE : FALSE, (LPARAM)(pData->pData));
		}
		return 0;
	}
}

BSFMApp* getApp()
{
	if (g_TheApp != nullptr)
		return g_TheApp;
	return nullptr;
}

BSFMApp::BSFMApp()
	: _hInst(nullptr)
	, _defaultFont(nullptr)
	, _hAccel(nullptr)
	, _hIcon(nullptr)
	, _lockFile(-1)
	, _tpHandle(nullptr)
	, _tpCleanupGroup(nullptr)
{
	memset(_addressbarIcons, 0, sizeof(_addressbarIcons));
	InitializeThreadpoolEnvironment(&_tpCallbackEnv);
}

BSFMApp::~BSFMApp()
{
	if (_tpCleanupGroup != nullptr)
	{
		CloseThreadpoolCleanupGroupMembers(_tpCleanupGroup, FALSE, nullptr);
		CloseThreadpoolCleanupGroup(_tpCleanupGroup);
		_tpCleanupGroup = nullptr;
	}
	if (_tpHandle != nullptr)
	{
		CloseThreadpool(_tpHandle);
		_tpHandle = nullptr;
	}

	_settings.save();

	if (_hAccel != nullptr)
	{
		DestroyAcceleratorTable(_hAccel);
		_hAccel = nullptr;
	}
	if (_defaultFont != nullptr)
	{
		DeleteObject(_defaultFont);
		_defaultFont = nullptr;
	}
	for (size_t i = 0; i < _countof(_addressbarIcons); ++i)
	{
		if (_addressbarIcons[i] != nullptr)
		{
			DeleteObject(_addressbarIcons[i]);
			_addressbarIcons[i] = nullptr;
		}
	}
	if (_hIcon != nullptr)
	{
		DeleteObject(_hIcon);
		_hIcon = nullptr;
	}
	g_TheApp = nullptr;
	OleUninitialize();
}

bool BSFMApp::initialize(HINSTANCE hInst, int nShowCmd)
{
	_hInst = hInst;
	g_TheApp = this;

	if (FAILED(OleInitialize(nullptr)))
	{
		MessageBox(nullptr, L"Error: cannot initialize OLE", L"BS File Manager", MB_OK | MB_ICONERROR);
		return false;
	}

	_tpHandle = CreateThreadpool(nullptr);
	if (_tpHandle == nullptr)
	{
		MessageBox(nullptr, L"Error: cannot create thread pool", L"BS File Manager", MB_OK | MB_ICONERROR);
		return false;
	}

	SetThreadpoolThreadMaximum(_tpHandle, 1);
	if (!SetThreadpoolThreadMinimum(_tpHandle, 1))
	{
		MessageBox(nullptr, L"Error: cannot create thread pool", L"BS File Manager", MB_OK | MB_ICONERROR);
		return false;
	}

	_tpCleanupGroup = CreateThreadpoolCleanupGroup();
	if (_tpCleanupGroup == nullptr)
	{
		MessageBox(nullptr, L"Error: cannot create thread pool", L"BS File Manager", MB_OK | MB_ICONERROR);
		return false;
	}
	SetThreadpoolCallbackPool(&_tpCallbackEnv, _tpHandle);
	SetThreadpoolCallbackCleanupGroup(&_tpCallbackEnv, _tpCleanupGroup, nullptr);

	if (!_commandLineArgs.parse())
	{
		MessageBox(nullptr, L"Error: cannot parsing command line", L"BS File Manager", MB_OK | MB_ICONERROR);
		return false;
	}

	_settings.load(_getSettingFilePath());

	if (_checkDuplicatedInstance())
		return false;

	_localization.load(_getLocalizeFilePath());
	initLocalize(_localization);

	_defaultFont = _createFont(_settings.getFontSize());
	_hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_MAIN));
	_addressbarIcons[0] = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_PREV), IMAGE_ICON, 16, 16, 0);
	_addressbarIcons[1] = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_NEXT), IMAGE_ICON, 16, 16, 0);
	_addressbarIcons[2] = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_HOME), IMAGE_ICON, 16, 16, 0);
	_addressbarIcons[3] = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_UP), IMAGE_ICON, 16, 16, 0);
	_addressbarIcons[4] = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_NEW_FOLDER), IMAGE_ICON, 16, 16, 0);
	_addressbarIcons[5] = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_GO), IMAGE_ICON, 16, 16, 0);
	_addressbarIcons[6] = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_VIEW_FORMAT), IMAGE_ICON, 16, 16, 0);

	_hAccel = LoadAccelerators(hInst, MAKEINTRESOURCE(IDR_ACCELERATOR));

	if (!_mainWnd.create(nShowCmd))
	{
		MessageBox(nullptr, L"Error: cannot create window", L"BS File Manager", MB_OK | MB_ICONERROR);
		return false;
	}

	if (_lockFile != -1)
	{
		HWND hwndCur = getMainWindow();
		_write(_lockFile, &hwndCur, sizeof(HWND));
		_flushall();
	}

	return true;
}

int BSFMApp::run()
{
	MSG		msg;
	BOOL	ret;

	while ((ret = GetMessage(&msg, nullptr, 0, 0)) != 0)
	{
		if (ret == -1)
		{
			return -1;
		}
		else
		{
			if (_findFileDlg.isDialogMsg(&msg))
				continue;

			// 액셀러레이트 처리
			if (_hAccel != nullptr)
			{
				if (TranslateAccelerator(_mainWnd.getWindow(), _hAccel, &msg))
					continue;
			}

			// 키보드 마우스 입력을 우선 Shell에 먼저 보낸다.
			if (_mainWnd.preTranslateMessage(&msg))
				continue;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	if (_lockFile != -1)
	{
		_close(_lockFile);
		_lockFile = -1;
		_wunlink(_getLockFilePath().c_str());
	}

	return (int)msg.wParam;
}

HINSTANCE BSFMApp::getHandle()
{
	return _hInst;
}

HWND BSFMApp::getMainWindow()
{
	return _mainWnd.getWindow();
}

CommandLineParser const & BSFMApp::getCommandLineParser() const
{
	return _commandLineArgs;
}

Settings& BSFMApp::getSettings()
{
	return _settings;
}

PTP_WORK BSFMApp::createWork(PTP_WORK_CALLBACK callback, void* param)
{
	return CreateThreadpoolWork(callback, param, &_tpCallbackEnv);
}

void BSFMApp::showFindFileDlg(std::wstring const & destDir)
{
	_findFileDlg.show(destDir);
}

std::wstring BSFMApp::getName() const
{
	return getLocalizeString(L"Application", L"Name");
}

HFONT BSFMApp::getFont() const
{
	return _defaultFont;
}

int BSFMApp::getFontHeight() const
{
	HDC hdc = GetDC(nullptr);
	HGDIOBJ oldFont = SelectObject(hdc, getFont());
	TEXTMETRIC tm = {0};
	GetTextMetrics(hdc, &tm);
	SelectObject(hdc, oldFont);
	ReleaseDC(nullptr, hdc);
	return tm.tmHeight;
}

HICON BSFMApp::getIcon() const
{
	return _hIcon;
}

HICON BSFMApp::getAddressBarIcon(int index)
{
	return _addressbarIcons[index];
}

void BSFMApp::initLocalize(INIFile& ini)
{
	auto setfunc = [&](std::wstring const & group, std::wstring const & key, std::wstring const & val){
		if (ini.get(group, key) == nullptr)
			ini.set(group, key, val);
	};

	setfunc(L"Application", L"Name", L"BS File Manager");
	setfunc(L"Application", L"NewFileName", L"새 파일");
	setfunc(L"Application", L"NewFolderName", L"새 폴더");
	setfunc(L"Application", L"EmptyShortcutBar", L"Drag & Drop 또는 즐겨찾기 관리자를 통해서 등록하세요.");
	setfunc(L"Application", L"SetHomePrompt", L"%s\r\n이곳을 홈으로 설정하시겠습니까?");
	setfunc(L"Application", L"Error_NewFolder", L"다음 폴더를 생성할 수 없습니다.\r\n%s\r\nError Code: %d");
	setfunc(L"Application", L"Error_NewFile", L"다음 파일을 생성할 수 없습니다.\r\n%s\r\nError Code: %d");

	setfunc(L"About", L"OK", L"확인");
	setfunc(L"About", L"Homepage", L"홈페이지");

	setfunc(L"Tooltips", L"Prev", L"뒤로(Alt + Left)");
	setfunc(L"Tooltips", L"Next", L"앞으로(Alt + Right)");
	setfunc(L"Tooltips", L"ToHome", L"홈으로(Alt + Home)");
	setfunc(L"Tooltips", L"ToUp", L"상위 폴더로(Alt + Up)");
	setfunc(L"Tooltips", L"NewFolder", L"새 폴더(Ctrl + Shift + N)");
	setfunc(L"Tooltips", L"Go", L"이동");
	setfunc(L"Tooltips", L"ViewFormat", L"보기 변경");

	setfunc(L"BookmarkManager", L"Title", L"즐겨찾기 관리자");
	setfunc(L"BookmarkManager", L"Tree", L"즐겨찾기(&B)");
	setfunc(L"BookmarkManager", L"Name", L"이름(&N)");
	setfunc(L"BookmarkManager", L"Target", L"대상(&T)");
	setfunc(L"BookmarkManager", L"Params", L"명령 인수(&P)");
	setfunc(L"BookmarkManager", L"StartupFolder", L"시작 위치(&F)\r\n비워두면 현재 탐색중인 경로(또는 사용자 폴더/바탕화면)");
	setfunc(L"BookmarkManager", L"CheckAdmin", L"관리자 권한으로 실행(&A)");
	setfunc(L"BookmarkManager", L"Add", L"추가(&I)");
	setfunc(L"BookmarkManager", L"Edit", L"수정(&E)");
	setfunc(L"BookmarkManager", L"Delete", L"삭제(&R)");
	setfunc(L"BookmarkManager", L"Up", L"위로(&U)");
	setfunc(L"BookmarkManager", L"Down", L"아래로(&D)");
	setfunc(L"BookmarkManager", L"Close", L"닫기(&C)");
	setfunc(L"BookmarkManager", L"BookmarkFormat", L"즐겨찾기 %d");
	setfunc(L"BookmarkManager", L"PromptDelete", L"%s 항목을 삭제하시겠습니까?");
	setfunc(L"BookmarkManager", L"FolderSelect", L"폴더 선택");
	setfunc(L"BookmarkManager", L"TargetBrowse", L"대상 선택");
	setfunc(L"BookmarkManager", L"StartupFolderBrowse", L"시작 위치 선택");
	setfunc(L"BookmarkManager", L"Error_StartupFolder", L"시작 위치가 잘못되었습니다.");
	setfunc(L"BookmarkManager", L"Error_InvalidTarget", L"대상 경로가 잘못되었습니다.");
	setfunc(L"BookmarkManager", L"Error_EmptyTarget", L"대상을 입력하세요.");

	setfunc(L"AddBookmark", L"Title", L"즐겨찾기 추가");
	setfunc(L"AddBookmark", L"OK", L"확인(&O)");
	setfunc(L"AddBookmark", L"Cancel", L"취소(&C)");

	setfunc(L"FindFile", L"Pattern", L"패턴(&P):");
	setfunc(L"FindFile", L"UseRegEx", L"정규직 사용(&U)");
	setfunc(L"FindFile", L"Dest", L"대상 폴더(&T):");
	setfunc(L"FindFile", L"Find", L"찾기(&F)");
	setfunc(L"FindFile", L"Stop", L"찾기 중지(&F)");
	setfunc(L"FindFile", L"Goto", L"항목보기(&G)");
	setfunc(L"FindFile", L"Open", L"열기(&O)");
	setfunc(L"FindFile", L"CopyPath", L"경로복사(&Y)");
	setfunc(L"FindFile", L"Delete", L"삭제(&D)");
	setfunc(L"FindFile", L"Copy", L"복사(&C)");
	setfunc(L"FindFile", L"Close", L"닫기");
	setfunc(L"FindFile", L"Result", L"결과(&R):");
	setfunc(L"FindFile", L"Error_InvalidDestFolder", L"잘못된 경로를 입력하였습니다.");
	setfunc(L"FindFile", L"Error_Failed", L"지정된 폴더의 탐색이 실패하였습니다.");
	setfunc(L"FindFile", L"Error_NotFound", L"대상 파일을 찾을 수 없습니다.");
	setfunc(L"FindFile", L"Col_Name", L"파일명");
	setfunc(L"FindFile", L"Col_Folder", L"폴더");
	setfunc(L"FindFile", L"Col_Type", L"종류");
	setfunc(L"FindFile", L"Col_Time", L"수정 날짜");
	setfunc(L"FindFile", L"Col_Attr", L"속성");
	setfunc(L"FindFile", L"Col_Size", L"크기");
	setfunc(L"FindFile", L"DestFolderBrowse", L"대상 위치 선택");
}

std::wstring BSFMApp::getLocalizeString(std::wstring const & group, std::wstring const & key) const
{
	return _localization.get(group, key, key);
}

bool BSFMApp::_checkDuplicatedInstance()
{
	if (_settings.getAllowMultiInstance())
		return false;
	std::wstring lockFilePath = _getLockFilePath();
	if (_wsopen_s(&_lockFile, lockFilePath.c_str(), _O_CREAT | _O_TRUNC | _O_WRONLY | _O_BINARY, _SH_DENYWR, _S_IWRITE | _S_IREAD) == 0)
	{
		return false;
	}
	else
	{
		int tryCount = 5;
		while (tryCount-- > 0)
		{
			if (_wsopen_s(&_lockFile, lockFilePath.c_str(), _O_RDONLY | _O_BINARY, _SH_DENYNO, S_IREAD) == 0)
			{
				HWND hwndPrev = nullptr;
				if (_read(_lockFile, &hwndPrev, sizeof(HWND)) == sizeof(HWND))
				{
					PostMessage(hwndPrev, WM_ACTIVATE_BSFM, 0, 0);
					_close(_lockFile);
					_lockFile = -1;
					return true;
				}
				else
				{
					_close(_lockFile);
					_lockFile = -1;
				}
			}
			Sleep(1000);
		}
	}
	return true;
}

HFONT BSFMApp::_createFont(int fontSizePt)
{
	HDC hdc = GetDC(nullptr);
	HFONT ret = CreateFont(
		-MulDiv(fontSizePt, GetDeviceCaps(hdc, LOGPIXELSY), 72)
		, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE
		, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH
		, _settings.getFontName().c_str()
		);
	ReleaseDC(nullptr, hdc);
	return ret;
}

std::wstring BSFMApp::_getLocalizeFilePath() const
{
	wchar_t inifile[_MAX_PATH] = L"";
	swprintf_s(inifile, L"%slocalize_%s.ini", _commandLineArgs.getExeFileFolder().c_str(), _settings.getLocalizeLang().c_str());
	return std::wstring(inifile);
}

std::wstring BSFMApp::_getSettingFilePath() const
{
	wchar_t inifile[_MAX_PATH] = L"";
	swprintf_s(inifile, L"%ssettings.ini", _commandLineArgs.getExeFileFolder().c_str());
	return std::wstring(inifile);
}

std::wstring BSFMApp::_getLockFilePath() const
{
	wchar_t lockFilePath[_MAX_PATH] = L"";
	swprintf_s(lockFilePath, L"%sBSFM.lock", _commandLineArgs.getExeFileFolder().c_str());
	return std::wstring(lockFilePath);
}

void idListToString(PCIDLIST_ABSOLUTE pidl, std::wstring* parsingName, std::wstring* editName, std::wstring* normalName, bool* isFilesystemFolder)
{
	if (parsingName != nullptr)
		*parsingName = L"";
	if (editName != nullptr)
		*editName = L"";
	if (normalName != nullptr)
		*normalName = L"";
	if (isFilesystemFolder != nullptr)
		*isFilesystemFolder = false;

	IShellItem* psi = nullptr;
	if (SUCCEEDED(SHCreateItemFromIDList(pidl, IID_PPV_ARGS(&psi))))
	{
		wchar_t* name = nullptr;
		if (parsingName != nullptr)
		{
			if (SUCCEEDED(psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &name)))
			{
				*parsingName = name;
				CoTaskMemFree(name);
			}
		}
		if (editName != nullptr)
		{
			if (SUCCEEDED(psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEEDITING, &name)))
			{
				*editName = name;
				CoTaskMemFree(name);
			}
		}
		if (normalName != nullptr)
		{
			if (SUCCEEDED(psi->GetDisplayName(SIGDN_NORMALDISPLAY, &name)))
			{
				*normalName = name;
				CoTaskMemFree(name);
			}
		}
		if (isFilesystemFolder != nullptr)
		{
			SFGAOF flag = 0;
			if (SUCCEEDED(psi->GetAttributes(SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_STREAM, &flag)))
			{
				*isFilesystemFolder = ((flag & (SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_STREAM)) == (SFGAO_FILESYSTEM | SFGAO_FOLDER));
			}
		}
		psi->Release();
	}
}

IShellItem* parsingNameToShellItem(std::wstring const & parsingName)
{
	IShellItem* psi = nullptr;
	if (SUCCEEDED(SHCreateItemFromParsingName(parsingName.c_str(), nullptr, IID_PPV_ARGS(&psi))))
		return psi;
	else
		return nullptr;
}

void getDriveStrings(std::vector<std::wstring>& out)
{
	out.clear();

	DWORD needLen = GetLogicalDriveStrings(0, nullptr);
	wchar_t* buf = new wchar_t[needLen + 1];
	GetLogicalDriveStrings(needLen, buf);
	wchar_t* p = buf;
	wchar_t* pEnd = buf + needLen;
	while (p < pEnd && *p != 0)
	{
		out.push_back(p);
		p += wcslen(p) + 1;
	}
	delete [] buf;
}

void getShellIcon(std::wstring const & parsingName, HICON& hIcon, std::wstring& dispName)
{
	IShellItem* psi = nullptr;
	if (SUCCEEDED(SHCreateItemFromParsingName(parsingName.c_str(), nullptr, IID_PPV_ARGS(&psi))))
	{
		getShellIcon(psi, hIcon, dispName);
		psi->Release();
	}
	else
	{
		hIcon = nullptr;
		dispName = L"(unknown)";
	}
}

void getShellIcon(IShellItem* psi, HICON& hIcon, std::wstring& dispName)
{
	PIDLIST_ABSOLUTE pidl = nullptr;
	if (SUCCEEDED(SHGetIDListFromObject(psi, &pidl)))
	{
		getShellIcon(pidl, hIcon, dispName);
		ILFree(pidl);
	}
	else
	{
		hIcon = nullptr;
		dispName = L"(unknown)";
	}
}

void getShellIcon(PIDLIST_ABSOLUTE pidl, HICON& hIcon, std::wstring& dispName)
{
	SHFILEINFO sfi = {0};
	UINT flag = SHGFI_ICON | SHGFI_PIDL | SHGFI_SMALLICON | SHGFI_DISPLAYNAME;
	if (SHGetFileInfo((LPCWSTR)pidl, 0, &sfi, sizeof(sfi), flag))
	{
		hIcon = sfi.hIcon;
		dispName = sfi.szDisplayName;
	}
	else
	{
		SHSTOCKICONINFO ssii = {0};
		ssii.cbSize = sizeof(ssii);
		if (SUCCEEDED(SHGetStockIconInfo(SIID_DOCNOASSOC, SHGSI_ICON | SHGSI_SMALLICON, &ssii)))
		{
			hIcon = ssii.hIcon;
			dispName = L"(unknown)";
		}
		else
		{
			hIcon = nullptr;
			dispName = L"(unknown)";
		}
	}
}

HBITMAP iconToBitmap(HICON hIcon, int syscolorIndex)
{
	HBITMAP ret = nullptr;
	if (hIcon != nullptr)
	{
		HDC hdcDisp = GetDC(nullptr);
		if (hdcDisp != nullptr)
		{
			HDC hdcMem = CreateCompatibleDC(hdcDisp);
			if (hdcMem != nullptr)
			{
				HBITMAP hBmp = CreateCompatibleBitmap(hdcDisp, 16, 16);
				if (hBmp != nullptr)
				{
					HBITMAP oldBmp = (HBITMAP)SelectObject(hdcMem, hBmp);
					DrawIconEx(hdcMem, 0, 0, hIcon, 16, 16, 0, GetSysColorBrush(syscolorIndex), DI_NORMAL);
					SelectObject(hdcMem, oldBmp);
					ret = hBmp;
				}
				DeleteDC(hdcMem);
			}
			ReleaseDC(nullptr, hdcDisp);
		}
	}
	return ret;
}

int addIconToImageList(HICON hIcon, HIMAGELIST hil)
{
	int ret = 0;
	HBITMAP hbmp = nullptr;
	if (hIcon == nullptr)
	{
		SHSTOCKICONINFO ssii = {0};
		ssii.cbSize = sizeof(ssii);
		if (SUCCEEDED(SHGetStockIconInfo(SIID_FOLDER, SHGSI_ICON | SHGSI_SMALLICON, &ssii)))
		{
			hbmp = iconToBitmap(ssii.hIcon, COLOR_WINDOW);
			DestroyIcon(ssii.hIcon);
		}
	}
	else
	{
		hbmp = iconToBitmap(hIcon, COLOR_WINDOW);
	}
	if (hbmp != nullptr)
	{
		ret = ImageList_Add(hil, hbmp, nullptr);
		DeleteObject(hbmp);
	}
	return ret;
}

int getWindowText(HWND hwnd, std::wstring& buf)
{
	int len = (int)SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);
	if (len > 0)
	{
		wchar_t* tmp = new wchar_t[len + 1];
		SendMessage(hwnd, WM_GETTEXT, (WPARAM)(len + 1), (LPARAM)tmp);
		buf = tmp;
		delete [] tmp;
		return len;
	}
	else
	{
		buf.clear();
		return 0;
	}
}

bool isFolder(std::wstring const & path)
{
	bool bRet = false;
	if (path.length() > 0)
	{
		IShellItem* psi = parsingNameToShellItem(path.c_str());
		if (psi != nullptr)
		{
			SFGAOF flag = 0;
			if (SUCCEEDED(psi->GetAttributes(SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_STREAM, &flag)))
			{
				if ((flag & (SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_STREAM)) == (SFGAO_FOLDER | SFGAO_FILESYSTEM))
					bRet = true;
			}
			psi->Release();
		}
	}
	return bRet;
}

bool isFile(std::wstring const & path)
{
	bool bRet = false;
	if (path.length() > 0)
	{
		IShellItem* psi = parsingNameToShellItem(path.c_str());
		if (psi != nullptr)
		{
			SFGAOF flag = 0;
			if (SUCCEEDED(psi->GetAttributes(SFGAO_FILESYSTEM | SFGAO_STREAM, &flag)))
			{
				if ((flag & (SFGAO_STREAM | SFGAO_FILESYSTEM)) == (SFGAO_STREAM | SFGAO_FILESYSTEM))
					bRet = true;
			}
			psi->Release();
		}
	}
	return bRet;
}

std::wstring findPath(std::wstring const & filename)
{
	wchar_t ret[1024];
	if (_wsearchenv_s(filename.c_str(), L"PATH", ret) == 0)
		return std::wstring(ret);
	else if (_wsearchenv_s(filename.c_str(), L"USERPROFILE", ret) == 0)
		return std::wstring(ret);
	else
		return std::wstring(L"");
}

PROPERTYKEY getPropertyKey(std::wstring const & name)
{
	PROPERTYKEY pkey;
	if (SUCCEEDED(PSGetPropertyKeyFromName(name.c_str(), &pkey)))
		return pkey;
	return PKEY_ItemNameDisplay;
}

std::wstring getPropertyName(PROPERTYKEY const & pkey)
{
	wchar_t* keyName = nullptr;
	if (SUCCEEDED(PSGetNameFromPropertyKey(pkey, &keyName))
		|| SUCCEEDED(PSGetNameFromPropertyKey(PKEY_ItemNameDisplay, &keyName)))
	{
		std::wstring ret(keyName);
		CoTaskMemFree(keyName);
		return ret;
	}
	return std::wstring();
}

PIDLIST_ABSOLUTE browseForFolder(HWND hwnd, wchar_t (&dispName)[MAX_PATH], const wchar_t* title, const void* pInitFolder, bool bIsString)
{
	BFFParam param;
	param.pData = pInitFolder;
	param.isString = bIsString;

	BROWSEINFO bi = {0};
	bi.hwndOwner = hwnd;
	bi.pszDisplayName = dispName;
	bi.lpszTitle = title;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_DONTGOBELOWDOMAIN | BIF_RETURNFSANCESTORS | BIF_NONEWFOLDERBUTTON;
	bi.lParam = (LPARAM)&param;
	bi.lpfn = _BFFProc;
	return SHBrowseForFolder(&bi);
}
