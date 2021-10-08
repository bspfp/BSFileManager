#pragma once

#include "Layout.h"

class AddressBar
{
public:
	enum Command
	{
		ABCMD_BACK,
		ABCMD_FORWARD,
		ABCMD_HOME,
		ABCMD_UP,
		ABCMD_NEW_FOLDER,
		ABCMD_ADDRESS,
		ABCMD_GO,
		ABCMD_VIEW_FORMAT,
	};

	AddressBar();

	bool create(HWND hwndParent);
	bool preTranslateMessage(MSG* pMsg);
	HWND getWindow() const;
	bool getAddress(std::wstring& buf) const;
	void setAddress(std::wstring const & path);
	int getHeight() const;
	void setFocusToAddress() const;

private:
	void _onCreate();

	static LRESULT CALLBACK _WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	HWND	_hwnd;
	HWND	_hwndAddress;
	TLayout	_layout;
	TLayout	_layoutAddress;
	int		_btSize;

	std::wstring	_tooltipPrev;
	std::wstring	_tooltipNext;
	std::wstring	_tooltipToHome;
	std::wstring	_tooltipToUp;
	std::wstring	_tooltipNewFolder;
	std::wstring	_tooltipGo;
	std::wstring	_tooltipViewFormat;
};
