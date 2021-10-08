#pragma once

typedef std::vector<std::pair<PROPERTYKEY, int>> COLUMN_WIDTH_INFO;

#include "CommandLineParser.h"
#include "MainWnd.h"
#include "Settings.h"
#include "FindFileDlg.h"

class BSFMApp
{
	friend BSFMApp* getApp();

public:
	explicit BSFMApp();
	~BSFMApp();

	bool initialize(HINSTANCE hInst, int nShowCmd);
	int run();

	HINSTANCE getHandle();
	HWND getMainWindow();
	CommandLineParser const & getCommandLineParser() const;
	Settings& getSettings();
	PTP_WORK createWork(PTP_WORK_CALLBACK callback, void* param);
	void showFindFileDlg(std::wstring const & destDir);

	// resource
	std::wstring getName() const;
	HFONT getFont() const;
	int getFontHeight() const;
	HICON getIcon() const;
	HICON getAddressBarIcon(int index);

	// ∑Œƒ√∂Û¿Ã¬°
	static void initLocalize(INIFile& ini);
	std::wstring getLocalizeString(std::wstring const & group, std::wstring const & key) const;

private:
	bool _checkDuplicatedInstance();
	HFONT _createFont(int fontSizePt);
	std::wstring _getLocalizeFilePath() const;
	std::wstring _getSettingFilePath() const;
	std::wstring _getLockFilePath() const;

	HINSTANCE			_hInst;
	CommandLineParser	_commandLineArgs;
	INIFile				_localization;
	MainWnd				_mainWnd;
	Settings			_settings;
	int					_lockFile;

	HACCEL				_hAccel;
	HFONT				_defaultFont;
	HICON				_hIcon;
	HICON				_addressbarIcons[7];

	FindFileDlg			_findFileDlg;

	TP_CALLBACK_ENVIRON	_tpCallbackEnv;
	PTP_POOL			_tpHandle;
	PTP_CLEANUP_GROUP	_tpCleanupGroup;
};

BSFMApp* getApp();

void idListToString(PCIDLIST_ABSOLUTE pidl, std::wstring* parsingName, std::wstring* editName = nullptr, std::wstring* normalName = nullptr, bool* isFilesystemFolder = nullptr);
IShellItem* parsingNameToShellItem(std::wstring const & parsingName);
void getDriveStrings(std::vector<std::wstring>& out);
void getShellIcon(std::wstring const & parsingName, HICON& hIcon, std::wstring& dispName);
void getShellIcon(IShellItem* psi, HICON& hIcon, std::wstring& dispName);
void getShellIcon(PIDLIST_ABSOLUTE pidl, HICON& hIcon, std::wstring& dispName);
HBITMAP iconToBitmap(HICON hIcon, int syscolorIndex);
int addIconToImageList(HICON hIcon, HIMAGELIST hil);
int getWindowText(HWND hwnd, std::wstring& buf);
bool isFolder(std::wstring const & path);
bool isFile(std::wstring const & path);
std::wstring findPath(std::wstring const & filename);
PROPERTYKEY getPropertyKey(std::wstring const & name);
std::wstring getPropertyName(PROPERTYKEY const & pkey);
PIDLIST_ABSOLUTE browseForFolder(HWND hwnd, wchar_t (&dispName)[MAX_PATH], const wchar_t* title, const void* pInitFolder, bool bIsString);
