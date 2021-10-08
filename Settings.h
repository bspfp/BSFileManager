#pragma once

#include "INIFile.h"

static_assert(BSFM_SHORTCUT_BAR_COUNT > 0, "Must > 0");

class Settings
{
public:
	void load(std::wstring const & filepath);
	void save();

	// get

	// 기본 설정
	std::wstring getLocalizeLang(const INIFile* ini = nullptr) const;
	int getFontSize(const INIFile* ini = nullptr) const;
	std::wstring getFontName(const INIFile* ini = nullptr) const;
	std::wstring getHomeFolder(bool bUseDefault, const INIFile* ini = nullptr) const;
	bool getAllowMultiInstance(const INIFile* ini = nullptr) const;

	// View 설정
	UINT getViewMode(int viewIndex, const INIFile* ini = nullptr) const;
	std::wstring getCurrentFolder(int viewIndex, const INIFile* ini = nullptr) const;
	IShellItem* getCurrentFolderShellItem(int viewIndex) const;

	// 컬럼 설정
	void getViewColumnInfo(int viewIndex, SORTCOLUMN& sortColumn, COLUMN_WIDTH_INFO& columnWidth, const INIFile* ini = nullptr);
	void getFindFileColumnInfo(int (&col)[6], const INIFile* ini = nullptr);

	// 메인 윈도우 설정
	bool isMaximized(const INIFile* ini = nullptr) const;
	RECT getWindowRect(const INIFile* ini = nullptr) const;
	void getSplitSize(double& center, double& upper, double& lower, const INIFile* ini = nullptr) const;
	int getSplitMode(const INIFile* ini = nullptr) const;

	// set

	// 기본 설정
	void setLocalizeLang(std::wstring const & val);
	void setFontSize(int val);
	void setFontName(std::wstring const & val);
	void setHomeFolder(std::wstring const & val);
	void setAllowMultiInstance(bool val);

	// View 설정
	void setViewMode(int viewIndex, UINT val);
	void setViewModeNext(int viewIndex);
	void setCurrentFolder(int viewIndex, std::wstring const & parsingName);

	// 컬럼 설정
	void setViewColumnInfo(int viewIndex, SORTCOLUMN const & sortColumn, COLUMN_WIDTH_INFO const & columnWidth);
	void setFindFileColumnInfo(int (&col)[6]);

	// 메인 윈도우 설정
	void setMaximized(bool val);
	void setWindowRect(RECT const & val);
	void setSplitSize(double center, double upper, double lower);
	void setSplitMode(int mode);

	// 바로가기 설정
	std::vector<ShortcutInfo> const & getShortcutInfo(int barIndex) const;
	bool isShowShortcutBar(int barIndex) const;
	void setShowShortcutBar(int barIndex, bool bShow);
	void addShortcutInfo(int barIndex, ShortcutInfo const & info);
	void removeShortcutInfo(int barIndex, ShortcutInfo const & info);
	void setShortcutInfo(int barIndex, std::vector<ShortcutInfo> const & infoList);

private:
	std::wstring	_filepath;
	INIFile			_ini;
	std::vector<ShortcutInfo>	_shortcutBarInfo[BSFM_SHORTCUT_BAR_COUNT];
	bool						_bShowShortcutBar[BSFM_SHORTCUT_BAR_COUNT];
};
