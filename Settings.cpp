#include "stdafx.h"
#include "BSFMApp.h"
#include "Settings.h"

#define BSFM_DEFAULT_LANG			L"ko"
#define BSFM_DEFAULT_FONT_SIZE		9
#define BSFM_DEFAULT_FONT_SIZE_STR	L"9"
#define BSFM_DEFAULT_FONT_NAME		L"돋움체"

#define BSFM_DEFAULT_VIEW_MODE		L"details"

#define BSFM_DEFAULT_WINDOW_MAXIMIZE		L"1"
#define BSFM_DEFAULT_WINDOW_SPLIT_MODE		L"4"

namespace
{
	UINT viewModeFromString(std::wstring const & val)
	{
		if (val == L"details")			return FVM_DETAILS;
		else if (val == L"icon")		return FVM_ICON;
		else if (val == L"list")		return FVM_LIST;
		else if (val == L"content")		return FVM_CONTENT;
		else if (val == L"tile")		return FVM_TILE;
		else							return FVM_DETAILS;
	}

	const wchar_t* toShortcutBarKey(int barIndex)
	{
		static wchar_t tmp[32];
		swprintf_s(tmp, L"ShortcutBar_%d", barIndex);
		return tmp;
	}

	const wchar_t* toViewKey(int viewIndex)
	{
		static wchar_t tmp[32];
		swprintf_s(tmp, L"View_%d", viewIndex);
		return tmp;
	}

	const wchar_t* toColumnInfoKey(int viewIndex)
	{
		static wchar_t tmp[32];
		swprintf_s(tmp, L"ColumnInfo_%d", viewIndex);
		return tmp;
	}
}

void Settings::load(std::wstring const & filepath)
{
	_filepath = filepath;
	INIFile inifile;
	if (!inifile.load(filepath))
		inifile.clear();
	// 기본 설정
	setLocalizeLang(getLocalizeLang(&inifile));
	setFontSize(getFontSize(&inifile));
	setFontName(getFontName(&inifile));
	setHomeFolder(getHomeFolder(false, &inifile));
	setAllowMultiInstance(getAllowMultiInstance(&inifile));

	for (int viewIndex = 0; viewIndex < 4; ++viewIndex)
	{
		// View 설정
		setViewMode(viewIndex, getViewMode(viewIndex, &inifile));
		setCurrentFolder(viewIndex, getCurrentFolder(viewIndex, &inifile));

		// 컬럼 설정
		SORTCOLUMN sc;
		COLUMN_WIDTH_INFO cw;
		getViewColumnInfo(viewIndex, sc, cw, &inifile);
		setViewColumnInfo(viewIndex, sc, cw);
	}

	// 컬럼 설정
	int findfileCol[6];
	getFindFileColumnInfo(findfileCol, &inifile);
	setFindFileColumnInfo(findfileCol);

	// 메인 윈도우 설정
	{
		setMaximized(isMaximized(&inifile));
		setWindowRect(getWindowRect(&inifile));
		double c, u, l;
		getSplitSize(c, u, l, &inifile);
		setSplitSize(c, u, l);
		setSplitMode(getSplitMode(&inifile));
	}

	// 즐겨찾기 첫 줄은 시스템 드라이브 선택 줄이다.
	std::vector<std::wstring> drives;
	getDriveStrings(drives);
	_bShowShortcutBar[0] = true;
	for (auto iter = drives.begin(); iter != drives.end(); ++iter)
		_shortcutBarInfo[0].push_back(ShortcutInfo(L"", *iter, L"", L"", false));

	for (int i = 1; i < BSFM_SHORTCUT_BAR_COUNT; ++i)
	{
		auto group = toShortcutBarKey(i);
		auto shortcutBarInfo = inifile.get(group);

		_bShowShortcutBar[i] = false;
		_ini.set(group, L"Show", L"0");
		if (shortcutBarInfo != nullptr)
		{
			std::map<int, ShortcutInfo> shortcutData;

			for (auto iter = shortcutBarInfo->begin(); iter != shortcutBarInfo->end(); ++iter)
			{
				if (iter->first == L"Show")
				{
					_bShowShortcutBar[i] = (iter->second[0] == L"1");
					_ini.set(group, L"Show", _bShowShortcutBar[i] ? L"1" : L"0");
				}
				else
				{
					int itemIndex = _wtoi(iter->first.c_str());
					std::wstring name = (iter->second.size() > 1) ? iter->second[1] : std::wstring(L"Name");
					std::wstring targetPath = (iter->second.size() > 2) ? iter->second[2] : std::wstring(L"C:\\");
					std::wstring params = (iter->second.size() > 3) ? iter->second[3] : std::wstring(L"");
					std::wstring startupFolder = (iter->second.size() > 4) ? iter->second[4] : std::wstring(L"");
					bool checkAdmin = (iter->second.size() > 5) ? (iter->second[5] == L"1") : false;
					if (startupFolder.length() > 1 && *(startupFolder.rbegin()) != L'\\')
						startupFolder += L'\\';
					shortcutData[itemIndex] = ShortcutInfo(name, targetPath, params, startupFolder, checkAdmin);
				}
			}

			int itemIndex = 0;
			for (auto iter = shortcutData.begin(); iter != shortcutData.end(); ++iter)
			{
				_shortcutBarInfo[i].push_back(iter->second);
				std::vector<std::wstring> iniVal;
				iniVal.push_back(iter->second.name);
				iniVal.push_back(iter->second.targetPath);
				iniVal.push_back(iter->second.params);
				iniVal.push_back(iter->second.startupFolder);
				iniVal.push_back((iter->second.checkAdmin) ? L"1" : L"0");
				_ini.set(group, std::to_wstring(itemIndex), iniVal);
				++itemIndex;
			}
		}
	}
	save();
}

void Settings::save()
{
	// 바로가기 to ini
	for (int i = 1; i < BSFM_SHORTCUT_BAR_COUNT; ++i)
	{
		auto group = toShortcutBarKey(i);
		_ini.removeGroup(group);
		_ini.set(group, L"Show", (_bShowShortcutBar[i]) ? L"1" : L"0");
		int itemIndex = 0;
		for (auto iter = _shortcutBarInfo[i].begin(); iter != _shortcutBarInfo[i].end(); ++iter)
		{
			std::vector<std::wstring> iniVal;
			iniVal.push_back(iter->name);
			iniVal.push_back(iter->targetPath);
			iniVal.push_back(iter->params);
			iniVal.push_back(iter->startupFolder);
			iniVal.push_back((iter->checkAdmin) ? L"1" : L"0");
			_ini.set(group, std::to_wstring(itemIndex), iniVal);
			++itemIndex;
		}
	}

	_ini.save(_filepath);
}

std::wstring Settings::getLocalizeLang(const INIFile* ini) const
{
	if (ini == nullptr)
		ini = &_ini;
	return ini->get(L"App", L"Lang", BSFM_DEFAULT_LANG);
}

int Settings::getFontSize(const INIFile* ini) const
{
	if (ini == nullptr)
		ini = &_ini;
	int ret = _wtoi(ini->get(L"App", L"FontSize", BSFM_DEFAULT_FONT_SIZE_STR).c_str());
	return (ret > 0) ? ret : BSFM_DEFAULT_FONT_SIZE;
}

std::wstring Settings::getFontName(const INIFile* ini) const
{
	if (ini == nullptr)
		ini = &_ini;
	return ini->get(L"App", L"FontName", BSFM_DEFAULT_FONT_NAME);
}

std::wstring Settings::getHomeFolder(bool bUseDefault, const INIFile* ini) const
{
	if (ini == nullptr)
		ini = &_ini;
	std::wstring ret = ini->get(L"App", L"HomeFolder", L"");
	if (bUseDefault && ret.length() < 1)
	{
		wchar_t* name = nullptr;
		if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &name)))
		{
			ret = name;
			CoTaskMemFree(name);
		}
	}
	return ret;
}

bool Settings::getAllowMultiInstance(const INIFile* ini) const
{
	if (ini == nullptr)
		ini = &_ini;
	return (ini->get(L"App", L"AllowMultiInstance", L"0") == L"1");
}

UINT Settings::getViewMode(int viewIndex, const INIFile* ini) const
{
	if (ini == nullptr)
		ini = &_ini;
	return viewModeFromString(ini->get(toViewKey(viewIndex), L"Mode", BSFM_DEFAULT_VIEW_MODE));
}

std::wstring Settings::getCurrentFolder(int viewIndex, const INIFile* ini) const
{
	if (ini == nullptr)
		ini = &_ini;
	return ini->get(toViewKey(viewIndex), L"Folder", L"");
}

IShellItem* Settings::getCurrentFolderShellItem(int viewIndex) const
{
	IShellItem* psi = nullptr;
	std::wstring parsingName = getCurrentFolder(viewIndex);
	if (parsingName.length() > 0)
	{
		if (SUCCEEDED(SHCreateItemFromParsingName(parsingName.c_str(), nullptr, IID_PPV_ARGS(&psi))))
			return psi;
	}
	if (SUCCEEDED(SHCreateItemInKnownFolder(FOLDERID_Desktop, 0, nullptr, IID_PPV_ARGS(&psi))))
		return psi;
	return nullptr;
}

void Settings::getViewColumnInfo(int viewIndex, SORTCOLUMN& sortColumn, COLUMN_WIDTH_INFO& columnWidth, const INIFile* ini)
{
	if (ini == nullptr)
		ini = &_ini;

	const wchar_t* group = toColumnInfoKey(viewIndex);

	// 정렬
	{
		auto val = ini->get(group, L"Sort");
		if (val != nullptr && val->size() == 3)
		{
			sortColumn.propkey = getPropertyKey(val->at(1));
			sortColumn.direction = (val->at(2) != L"DESC") ? SORT_ASCENDING : SORT_DESCENDING;
		}
		else
		{
			sortColumn.propkey = PKEY_ItemNameDisplay;
			sortColumn.direction = SORT_ASCENDING;
		}
	}

	// 열 크기
	{
		columnWidth.clear();
		int count = _wtoi(ini->get(group, L"ColumnCount", L"0").c_str());
		for (int i = 0; i < count; ++i)
		{
			wchar_t key[64];
			swprintf_s(key, L"Col_%d", i);
			auto val = ini->get(group, key);
			if (val != nullptr)
			{
				PROPERTYKEY pkey = getPropertyKey(val->at(1));
				int width = _wtoi(val->at(2).c_str());
				if (width <= 0 || width >= 800)
					width = CM_WIDTH_USEDEFAULT;
				columnWidth.push_back(COLUMN_WIDTH_INFO::value_type(pkey, width));
			}
		}
	}
}

void Settings::getFindFileColumnInfo(int (&col)[6], const INIFile* ini)
{
	if (ini == nullptr)
		ini = &_ini;
	auto data = ini->get(L"FindFile", L"ColWidth");
	col[0] = (data != nullptr && data->size() > 1) ? _wtoi(data->at(1).c_str()) : 80;
	col[1] = (data != nullptr && data->size() > 2) ? _wtoi(data->at(2).c_str()) : 80;
	col[2] = (data != nullptr && data->size() > 3) ? _wtoi(data->at(3).c_str()) : 80;
	col[3] = (data != nullptr && data->size() > 4) ? _wtoi(data->at(4).c_str()) : 80;
	col[4] = (data != nullptr && data->size() > 5) ? _wtoi(data->at(5).c_str()) : 80;
	col[5] = (data != nullptr && data->size() > 6) ? _wtoi(data->at(6).c_str()) : 50;
}

bool Settings::isMaximized(const INIFile* ini) const
{
	if (ini == nullptr)
		ini = &_ini;

	return (ini->get(L"Window", L"Maximize", BSFM_DEFAULT_WINDOW_MAXIMIZE) == L"1");
}

RECT Settings::getWindowRect(const INIFile* ini) const
{
	if (ini == nullptr)
		ini = &_ini;

	RECT ret = {0, 0, 0, 0};
	auto val = ini->get(L"Window", L"Position");
	if (val != nullptr && val->size() == 5)
	{
		int x = _wtoi(val->at(1).c_str());
		int y = _wtoi(val->at(2).c_str());
		int w = _wtoi(val->at(3).c_str());
		int h = _wtoi(val->at(4).c_str());
		if (w > 0 && h > 0)
		{
			ret.left = x;
			ret.top = y;
			ret.right = x + w;
			ret.bottom = y + h;
		}
	}
	return ret;
}

void Settings::getSplitSize(double& center, double& upper, double& lower, const INIFile* ini) const
{
	if (ini == nullptr)
		ini = &_ini;

	center = upper = lower = 0.5;
	if (ini->get(L"Window", L"Split") != nullptr)
	{
		auto splitSize = ini->get(L"Window", L"Split");
		if (splitSize->size() == 4)
		{
			center = _wtof(splitSize->at(1).c_str());
			upper = _wtof(splitSize->at(2).c_str());
			lower = _wtof(splitSize->at(3).c_str());
			if (center < 0.1) center = 0.5;
			if (upper < 0.1) upper = 0.5;
			if (lower < 0.1) lower = 0.5;
		}
	}
}

int Settings::getSplitMode(const INIFile* ini) const
{
	if (ini == nullptr)
		ini = &_ini;

	int ret = _wtoi(ini->get(L"Window", L"SplitMode", BSFM_DEFAULT_WINDOW_SPLIT_MODE).c_str());
	switch (ret)
	{
	case 1:
	case 2:
	case 3:
		return ret;
	default:
		return 4;
	}
}

void Settings::setLocalizeLang(std::wstring const & val)
{
	_ini.set(L"App", L"Lang", val);
}

void Settings::setFontSize(int val)
{
	_ini.set(L"App", L"FontSize", std::to_wstring((int)((val > 0) ? val : BSFM_DEFAULT_FONT_SIZE)));
}

void Settings::setFontName(std::wstring const & val)
{
	_ini.set(L"App", L"FontName", val);
}

void Settings::setHomeFolder(std::wstring const & val)
{
	_ini.set(L"App", L"HomeFolder", val);
}

void Settings::setAllowMultiInstance(bool val)
{
	_ini.set(L"App", L"AllowMultiInstance", val ? L"1" : L"0");
}

void Settings::setViewMode(int viewIndex, UINT val)
{
	std::wstring viewMode;
	switch (val)
	{
	case FVM_DETAILS:		viewMode = L"details";		break;
	case FVM_ICON:			viewMode = L"icon";			break;
	case FVM_LIST:			viewMode = L"list";			break;
	case FVM_CONTENT:		viewMode = L"content";		break;
	case FVM_TILE:			viewMode = L"tile";			break;
	default:				viewMode = L"details";		break;
	}
	_ini.set(toViewKey(viewIndex), L"Mode", viewMode);
}

void Settings::setViewModeNext(int viewIndex)
{
	static const UINT convmap[][2] = {
		{FVM_DETAILS, FVM_ICON},
		{FVM_ICON, FVM_LIST},
		{FVM_LIST, FVM_CONTENT},
		{FVM_CONTENT, FVM_TILE},
		{FVM_TILE, FVM_DETAILS},
	};
	UINT curMode = getViewMode(viewIndex);
	UINT nextMode = FVM_DETAILS;
	for (size_t i = 0; i < _countof(convmap); ++i)
	{
		if (convmap[i][0] == curMode)
		{
			nextMode = convmap[i][1];
			break;
		}
	}
	setViewMode(viewIndex, nextMode);
}

void Settings::setCurrentFolder(int viewIndex, std::wstring const & parsingName)
{
	_ini.set(toViewKey(viewIndex), L"Folder", parsingName);
}

void Settings::setViewColumnInfo(int viewIndex, SORTCOLUMN const & sortColumn, COLUMN_WIDTH_INFO const & columnWidth)
{
	const wchar_t* group = toColumnInfoKey(viewIndex);

	_ini.removeGroup(group);

	// 정렬
	{
		std::vector<std::wstring> val;
		val.push_back(getPropertyName(sortColumn.propkey));
		val.push_back((sortColumn.direction == SORT_ASCENDING) ? L"ASC" : L"DESC");
		_ini.set(group, L"Sort", val);
	}

	// 열 크기
	{
		if (!columnWidth.empty())
		{
			int count = (int)columnWidth.size();
			_ini.set(group, L"ColumnCount", std::to_wstring(count));
			for (int i = 0; i < count; ++i)
			{
				wchar_t key[64];
				swprintf_s(key, L"Col_%d", i);
				std::vector<std::wstring> val;
				val.push_back(getPropertyName(columnWidth[i].first));
				val.push_back(std::to_wstring(columnWidth[i].second));
				_ini.set(group, key, val);
			}
		}
	}
}

void Settings::setFindFileColumnInfo(int (&col)[6])
{
	std::vector<std::wstring> data;
	data.push_back(std::to_wstring((col[0] > 30) ? col[0] : 30));
	data.push_back(std::to_wstring((col[1] > 30) ? col[1] : 30));
	data.push_back(std::to_wstring((col[2] > 30) ? col[2] : 30));
	data.push_back(std::to_wstring((col[3] > 30) ? col[3] : 30));
	data.push_back(std::to_wstring((col[4] > 30) ? col[4] : 30));
	data.push_back(std::to_wstring((col[5] > 30) ? col[5] : 30));
	_ini.set(L"FindFile", L"ColWidth", data);
}

void Settings::setMaximized(bool val)
{
	_ini.set(L"Window", L"Maximize", (val) ? L"1" : L"0");
}

void Settings::setWindowRect(RECT const & val)
{
	std::vector<std::wstring> vecVal;

	int x = val.left;
	int y = val.top;
	int w = val.right - x;
	int h = val.bottom - y;
	vecVal.push_back(std::to_wstring(x));
	vecVal.push_back(std::to_wstring(y));
	vecVal.push_back(std::to_wstring(w));
	vecVal.push_back(std::to_wstring(h));
	_ini.set(L"Window", L"Position", vecVal);
}

void Settings::setSplitSize(double center, double upper, double lower)
{
	std::vector<std::wstring> vecVal;
	wchar_t tmp[32];
	swprintf_s(tmp, L"%.2f", center);		vecVal.push_back(tmp);
	swprintf_s(tmp, L"%.2f", upper);		vecVal.push_back(tmp);
	swprintf_s(tmp, L"%.2f", lower);		vecVal.push_back(tmp);
	_ini.set(L"Window", L"Split", vecVal);
}

void Settings::setSplitMode(int mode)
{
	switch (mode)
	{
	case 1:		_ini.set(L"Window", L"SplitMode", L"1");	break;
	case 2:		_ini.set(L"Window", L"SplitMode", L"2");	break;
	case 3:		_ini.set(L"Window", L"SplitMode", L"3");	break;
	default:	_ini.set(L"Window", L"SplitMode", L"4");	break;
	}
}

std::vector<ShortcutInfo> const & Settings::getShortcutInfo(int barIndex) const
{
	return _shortcutBarInfo[barIndex];
}

bool Settings::isShowShortcutBar(int barIndex) const
{
	return _bShowShortcutBar[barIndex];
}

void Settings::setShowShortcutBar(int barIndex, bool bShow)
{
	_bShowShortcutBar[barIndex] = bShow;
}

void Settings::addShortcutInfo(int barIndex, ShortcutInfo const & info)
{
	_shortcutBarInfo[barIndex].push_back(info);
}

void Settings::removeShortcutInfo(int barIndex, ShortcutInfo const & info)
{
	for (auto iter = _shortcutBarInfo[barIndex].begin(); iter != _shortcutBarInfo[barIndex].end();)
	{
		if (*iter == info)
		{
			iter = _shortcutBarInfo[barIndex].erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

void Settings::setShortcutInfo(int barIndex, std::vector<ShortcutInfo> const & infoList)
{
	_shortcutBarInfo[barIndex].clear();
	_shortcutBarInfo[barIndex].assign(infoList.begin(), infoList.end());
}