#pragma once

#include "Layout.h"
#include "Lock.h"

struct FindFileParam
{
	std::wstring	destDir;
	std::wstring	pattern;
	bool			bUseRegEx;
	HWND			hwndDlg;
	volatile bool	stopFlag;

	LockCS			lockobj;
	std::list<std::tuple<std::wstring, unsigned int, __time64_t, long long>>	result;		// fullpath, attr, write time, size
};

class FindFileDlg
{
public:
	FindFileDlg();
	~FindFileDlg();

	void show(std::wstring const & destDir);
	bool isDialogMsg(MSG* pMsg);

private:
	void _startFind();
	void _stopFinding();
	void _stopAndWaitFinding();
	void _enableActionButtons(BOOL bEnable, bool bMultiSelect = false);

	void _onInitDialog();
	void _onFindFileResult();
	void _onClose();
	void _onResultItemChanged();
	void _onGoto();
	void _onDestBrowse();
	void _onOpen();

	static INT_PTR CALLBACK _DlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	static VOID CALLBACK _FindCallback(PTP_CALLBACK_INSTANCE inst, PVOID context, PTP_WORK);

	HWND			_hwnd;
	HIMAGELIST		_hImageList;
	PTP_WORK		_work;
	FindFileParam	_param;

	TLayout			_mainLayout;
	TLayout			_leftLayout;
	FLayout			_rightLayout;
};
