#pragma once

struct ShortcutInfo
{
	std::wstring	name;
	std::wstring	targetPath;
	std::wstring	params;
	std::wstring	startupFolder;
	bool			checkAdmin;

	ShortcutInfo() : checkAdmin(false) {}
	ShortcutInfo(std::wstring const & name, std::wstring const & targetPath, std::wstring const & params, std::wstring const & startupFolder, bool checkAdmin) : name(name), targetPath(targetPath), params(params), startupFolder(startupFolder), checkAdmin(checkAdmin) {}

	bool operator==(ShortcutInfo const & rhs) const
	{
		return (name == rhs.name && targetPath == rhs.targetPath && params == rhs.params && startupFolder == rhs.startupFolder && checkAdmin == rhs.checkAdmin);
	}

	std::wstring getTarget() const;
};

class ShortcutButton
{
public:
	typedef std::shared_ptr<ShortcutButton>	Ptr;

	static Ptr create(HWND hwndParent, ShortcutInfo const & info, UINT id, bool bUsingShellName, int maxWidth = INT_MAX);

	~ShortcutButton();
	HWND getWindow() const;
	void destroy();
	int getHeight() const;
	int getWidth() const;
	ShortcutInfo const & getInfo() const;
	const wchar_t* getTip() const;

private:
	ShortcutButton();

	bool _create(HWND hwndParent, ShortcutInfo const & info, UINT id, bool bUsingShellName, int maxWidth);

	HWND			_hwnd;
	HICON			_hIcon;
	SIZE			_size;
	ShortcutInfo	_info;
	std::wstring	_tip;
};

class ShortcutBar : public IDropTarget
{
public:
	typedef std::shared_ptr<ShortcutBar>	Ptr;

	ShortcutBar(int index);
	~ShortcutBar();

	bool create(HWND hwndParent);
	HWND getWindow() const;
	void show(bool bShow);
	bool isShow() const;
	int getHeight() const;

	void refresh();

	// IUnknown
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
	IFACEMETHODIMP_(ULONG) AddRef();
	IFACEMETHODIMP_(ULONG) Release();

	// IDropTarget
	IFACEMETHODIMP DragEnter(IDataObject* pObj, DWORD keyState, POINTL pt, DWORD* pEffect);
	IFACEMETHODIMP DragOver(DWORD keyState, POINTL pt, DWORD* pEffect);
	IFACEMETHODIMP DragLeave();
	IFACEMETHODIMP Drop(IDataObject* pObj, DWORD keyState, POINTL pt, DWORD* pEffect);

private:
	void _onCreate();

	void _setDropTip(bool bReset);

	static LRESULT CALLBACK _WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	HWND	_hwnd;
	HWND	_hwndTooltip;
	int		_index;
	TLayout	_layout;
	std::vector<ShortcutButton::Ptr>	_buttons;

	IDropTargetHelper*	_pdth;
	IDataObject*		_pObj;
	bool				_bRegisteredDrop;
	DWORD				_lastKeyState;
	std::wstring		_objName;
	volatile ULONG		_refCount;
};
