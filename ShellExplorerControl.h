#pragma once

class ShellExplorerControl : public IServiceProvider, public ICommDlgBrowser, public IExplorerBrowserEvents
{
public:
	ShellExplorerControl();

	bool create(HWND hwndParent, int viewIndex);
	void destroy();

	HWND getWindow() const;
	HWND getViewWindow() const;
	HWND getTreeWindow() const;
	HWND getNamespaceTreeWindow() const;

	void setFont(HFONT hfont, BOOL bRedraw = TRUE);
	void setViewMode();

	bool preTranslateMessage(MSG* pMsg);

	bool isChild(HWND hwnd);
	int getViewIndex() const;

	void goParent();
	void editName(IShellItem* psi);
	void selectName(IShellItem* psi);
	bool browseTo(std::wstring const & path);
	bool browseTo(IShellItem* psi);
	void ensureSelectedItemVisible();
	void setFocusToShellView() const;

	bool onAppCommand(UINT cmd);

private:
	INameSpaceTreeControl* _getNamespaceTreeControl() const;
	void _restoreViewColumn(IUnknown* pTarget);
	void _saveViewColumn(IFolderView2* pfv2New = nullptr, IColumnManager* pcmNew = nullptr);
	void _setViewColumnNonFSFolder(IUnknown* pTarget, PCIDLIST_ABSOLUTE pidl);

public:
	// IUnknown
	IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv);
	IFACEMETHODIMP_(ULONG) AddRef();
	IFACEMETHODIMP_(ULONG) Release();

	// IServiceProvider
	IFACEMETHODIMP QueryService(REFGUID rguid, REFIID riid, void** ppv);

	// ICommDlgBrowser
	IFACEMETHODIMP OnDefaultCommand(IShellView*) { return E_NOTIMPL; }
	IFACEMETHODIMP OnStateChange(IShellView*, ULONG) { return E_NOTIMPL; }
	IFACEMETHODIMP IncludeObject(IShellView*, PCUITEMID_CHILD) { return E_NOTIMPL; }

	// IExplorerBrowserEvents
	IFACEMETHODIMP OnNavigationComplete(PCIDLIST_ABSOLUTE);
	IFACEMETHODIMP OnNavigationFailed(PCIDLIST_ABSOLUTE) { return S_OK; }
	IFACEMETHODIMP OnNavigationPending(PCIDLIST_ABSOLUTE) { return S_OK; }
	IFACEMETHODIMP OnViewCreated(IShellView*);

private:
	IExplorerBrowser*		_peb;
	volatile ULONG			_refCount;
	DWORD					_cookie;
	int						_viewIndex;

	bool				_isFilesystemFolder;
	SORTCOLUMN			_sortColumn;
	COLUMN_WIDTH_INFO	_columnWidth;
};
