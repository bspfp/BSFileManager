#pragma once

#include <SDKDDKVer.h>
//#define _WIN32_WINNT _WIN32_WINNT_WIN10
//#define _WIN32_IE _WIN32_IE_IE70
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#pragma warning(disable: 4503)		// 데코레이팅된 이름 길이를 초과했으므로 이름이 잘립니다.

#include <algorithm>
#include <list>
#include <vector>
#include <memory>
#include <regex>
#include <tuple>

#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <sys/stat.h>

#include <Windows.h>
#include <ShlObj.h>
#include <ShObjIdl.h>
#include <Shlwapi.h>
#include <Ole2.h>
#include <ShellAPI.h>
#include <CommDlg.h>
#include <propkey.h>
#include <propsys.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Version.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "propsys.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "resource.h"

#define WM_CHANGED_FOLDER_VIEW			(WM_APP +  0)		// LPARAM: PCIDLIST_ABSOLUTE
#define WM_VIEW_SEL_CHANGED				(WM_APP +  1)		// WPARAM: view index
#define WM_VIEW_COMMAND					(WM_APP +  2)		// WPARAM: AddressBar::Command
#define WM_VIEW_SET_VIEWMODE			(WM_APP +  3)		//
#define WM_DRIVE_CHANGED				(WM_APP +  4)		// WPARAM: HANDLE		LPARAM: DWORD
#define WM_SHORTCUT_COMMAND				(WM_APP +  5)		// LPARAM: ShortcutInfo*
#define WM_RESET_BOOKMARK_MENU			(WM_APP +  6)		//
#define WM_SET_TITLE					(WM_APP +  7)		// WPARAM: const wchar_t*
#define WM_GET_SELECTED_VIEW_INDEX		(WM_APP +  8)		// RETURN: int
#define WM_ACTIVATE_BSFM				(WM_APP +  9)		//
#define WM_FIND_FILE_END				(WM_APP + 10)		// 
#define WM_FIND_FILE_NOT_FOUND			(WM_APP + 11)		// 
#define WM_FIND_FILE_RESULT				(WM_APP + 12)		// 
#define WM_FIND_FILE_GOTO				(WM_APP + 13)		// WPARAM: const wchar_t*	LPARAM: const wchar_t*

#define BSFM_SHORTCUT_BAR_COUNT			5

#include "NonCopyable.h"
#include "INIFile.h"
