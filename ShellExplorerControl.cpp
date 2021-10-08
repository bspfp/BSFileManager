#include "stdafx.h"
#include "BSFMApp.h"
#include "ShellExplorerControl.h"

namespace
{
	bool isFilesystemFolder(IShellItem* psi)
	{
		SFGAOF flag = 0;
		if (SUCCEEDED(psi->GetAttributes(SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_STREAM, &flag)))
			return (flag == (SFGAO_FILESYSTEM | SFGAO_FOLDER));
		return false;
	}

	std::wstring const & getName_RecycleBin()
	{
		static std::wstring ret;
		if (ret.empty())
		{
			PIDLIST_ABSOLUTE pidl;
			if (SUCCEEDED(SHGetKnownFolderIDList(FOLDERID_RecycleBinFolder, 0, nullptr, &pidl)))
			{
				wchar_t* p;
				if (SUCCEEDED(SHGetNameFromIDList(pidl, SIGDN_DESKTOPABSOLUTEPARSING, &p)))
				{
					ret = p;
					CoTaskMemFree(p);
				}
				ILFree(pidl);
			}
		}
		return ret;
	}
}

ShellExplorerControl::ShellExplorerControl()
	: _peb(nullptr)
	, _refCount(1)
	, _cookie(0)
	, _viewIndex(-1)
	, _isFilesystemFolder(false)
{
	memset(&_sortColumn, 0, sizeof(_sortColumn));
}

bool ShellExplorerControl::create(HWND hwndParent, int viewIndex)
{
	_viewIndex = viewIndex;

	getApp()->getSettings().getViewColumnInfo(_viewIndex, _sortColumn, _columnWidth);

	RECT rect;
	SetRectEmpty(&rect);
	if (SUCCEEDED(CoCreateInstance(CLSID_ExplorerBrowser, nullptr, CLSCTX_INPROC, IID_PPV_ARGS(&_peb))))
	{
		IUnknown_SetSite(_peb, (IServiceProvider*)this);
		if (SUCCEEDED(_peb->Advise((IExplorerBrowserEvents*)this, &_cookie)))
		{
			FOLDERSETTINGS fs = {getApp()->getSettings().getViewMode(viewIndex), FWF_AUTOARRANGE | FWF_SHOWSELALWAYS | FWF_FULLROWSELECT};
			if (SUCCEEDED(_peb->Initialize(hwndParent, &rect, &fs)))
			{
				if (SUCCEEDED(_peb->SetOptions(EBO_ALWAYSNAVIGATE | EBO_SHOWFRAMES)))
				{
					IShellItem* psi = getApp()->getSettings().getCurrentFolderShellItem(_viewIndex);
					_peb->BrowseToObject(psi, 0);
					if (psi != nullptr)
						psi->Release();
					return true;
				}
				_peb->Destroy();
			}
			_peb->Unadvise(_cookie);
			_cookie = 0;
		}
		_peb->Release();
		_peb = nullptr;
	}

	return false;
}

void ShellExplorerControl::destroy()
{
	_saveViewColumn();
	getApp()->getSettings().setViewColumnInfo(_viewIndex, _sortColumn, _columnWidth);

	if (_peb != nullptr)
	{
		_peb->Destroy();
		if (_cookie != 0)
		{
			_peb->Unadvise(_cookie);
			_cookie = 0;
		}
		_peb->Release();
		_peb = nullptr;
	}
}

HWND ShellExplorerControl::getWindow() const
{
	HWND ret = nullptr;
	if (_peb != nullptr)
	{
		IOleWindow* pio;
		if (SUCCEEDED(_peb->QueryInterface(IID_PPV_ARGS(&pio))))
		{
			HWND hwnd;
			if (SUCCEEDED(pio->GetWindow(&hwnd)))
				ret = hwnd;
			pio->Release();
		}
	}
	return ret;
}

HWND ShellExplorerControl::getViewWindow() const
{
	if (_peb != nullptr)
	{
		IShellView* psv;
		if (SUCCEEDED(_peb->GetCurrentView(IID_PPV_ARGS(&psv))))
		{
			HWND hwnd = nullptr;
			psv->GetWindow(&hwnd);
			psv->Release();
			return hwnd;
		}
	}
	return nullptr;
}

HWND ShellExplorerControl::getTreeWindow() const
{
	HWND hwndNSTC = getNamespaceTreeWindow();
	if (hwndNSTC != nullptr)
		return GetDlgItem(hwndNSTC, 0x64);
	else
		return nullptr;
}

HWND ShellExplorerControl::getNamespaceTreeWindow() const
{
	HWND ret = nullptr;
	INameSpaceTreeControl* pnstc = _getNamespaceTreeControl();
	if (pnstc != nullptr)
	{
		IOleWindow* pio = nullptr;
		if (SUCCEEDED(pnstc->QueryInterface(IID_PPV_ARGS(&pio))))
		{
			HWND hwnd = nullptr;
			if (SUCCEEDED(pio->GetWindow(&hwnd)))
				ret = hwnd;
			pio->Release();
		}
		pnstc->Release();
	}
	return ret;
}

void ShellExplorerControl::setFont(HFONT hfont, BOOL bRedraw)
{
	HWND hwnd = getWindow();
	if (hwnd != nullptr)
		SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont, (LPARAM)bRedraw);
	hwnd = getViewWindow();
	if (hwnd != nullptr)
		SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont, (LPARAM)bRedraw);
}

void ShellExplorerControl::setViewMode()
{
	FOLDERSETTINGS fs = {getApp()->getSettings().getViewMode(_viewIndex), FWF_AUTOARRANGE | FWF_SHOWSELALWAYS | FWF_FULLROWSELECT};
	_peb->SetFolderSettings(&fs);
}

bool ShellExplorerControl::preTranslateMessage(MSG* pMsg)
{
	bool bret = false;
	if (_peb != nullptr)
	{
		IInputObject* pIO = nullptr;
		if (SUCCEEDED(_peb->QueryInterface(IID_PPV_ARGS(&pIO))))
		{
			if (pIO->HasFocusIO() == S_OK)
				bret = (pIO->TranslateAcceleratorIO(pMsg) == S_OK);
			pIO->Release();
		}
	}
	return bret;
}

bool ShellExplorerControl::isChild(HWND hwnd)
{
	HWND hwndThis = getWindow();
	return (IsChild(hwndThis, hwnd)) ? true : false;
}

int ShellExplorerControl::getViewIndex() const
{
	return _viewIndex;
}

void ShellExplorerControl::goParent()
{
	_peb->BrowseToIDList(nullptr, SBSP_PARENT);
}

void ShellExplorerControl::editName(IShellItem* psi)
{
	if (_peb != nullptr)
	{
		IShellView* psv;
		if (SUCCEEDED(_peb->GetCurrentView(IID_PPV_ARGS(&psv))))
		{
			PIDLIST_ABSOLUTE pidl = nullptr;
			if (SUCCEEDED(SHGetIDListFromObject(psi, &pidl)))
			{
				IShellFolder* isf = nullptr;
				PCUITEMID_CHILD pidlchild = nullptr;
				if (SUCCEEDED(SHBindToParent(pidl, IID_PPV_ARGS(&isf), &pidlchild)))
				{
					psv->Refresh();
					psv->SelectItem(pidlchild, SVSI_EDIT | SVSI_DESELECTOTHERS | SVSI_ENSUREVISIBLE | SVSI_FOCUSED);
					isf->Release();
				}
				ILFree(pidl);
			}
			psv->Release();
		}
	}
}

void ShellExplorerControl::selectName(IShellItem* psi)
{
	if (_peb != nullptr)
	{
		IShellView* psv;
		if (SUCCEEDED(_peb->GetCurrentView(IID_PPV_ARGS(&psv))))
		{
			PIDLIST_ABSOLUTE pidl = nullptr;
			if (SUCCEEDED(SHGetIDListFromObject(psi, &pidl)))
			{
				IShellFolder* isf = nullptr;
				PCUITEMID_CHILD pidlchild = nullptr;
				if (SUCCEEDED(SHBindToParent(pidl, IID_PPV_ARGS(&isf), &pidlchild)))
				{
					psv->SelectItem(pidlchild, SVSI_SELECT | SVSI_DESELECTOTHERS | SVSI_ENSUREVISIBLE | SVSI_FOCUSED);
					isf->Release();
				}
				ILFree(pidl);
			}
			psv->Release();
		}
	}
}

bool ShellExplorerControl::browseTo(std::wstring const & path)
{
	IShellItem* psi = nullptr;
	if (SUCCEEDED(SHCreateItemFromParsingName(path.c_str(), nullptr, IID_PPV_ARGS(&psi))))
	{
		bool ret = browseTo(psi);
		psi->Release();
		return ret;
	}
	else
	{
		return false;
	}
}

bool ShellExplorerControl::browseTo(IShellItem* psi)
{
	return (SUCCEEDED(_peb->BrowseToObject(psi, 0))) ? true : false;
}

void ShellExplorerControl::ensureSelectedItemVisible()
{
	HWND hwndTree = getTreeWindow();
	if (hwndTree != nullptr)
	{
		HTREEITEM ti = TreeView_GetNextSelected(hwndTree, nullptr);
		if (ti != nullptr)
			TreeView_EnsureVisible(hwndTree, ti);
	}
	if (_peb != nullptr)
	{
		IShellView* psv;
		if (SUCCEEDED(_peb->GetCurrentView(IID_PPV_ARGS(&psv))))
		{
			IShellItemArray* psia;
			if (SUCCEEDED(psv->GetItemObject(SVGIO_SELECTION, IID_PPV_ARGS(&psia))))
			{
				IShellItem* psi;
				if (SUCCEEDED(psia->GetItemAt(0, &psi)))
				{
					PIDLIST_ABSOLUTE pidl;
					if (SUCCEEDED(SHGetIDListFromObject(psi, &pidl)))
					{
						IShellFolder* psf;
						PCITEMID_CHILD pidlChild;
						if (SUCCEEDED(SHBindToParent(pidl, IID_PPV_ARGS(&psf), &pidlChild)))
						{
							psv->SelectItem(pidlChild, SVSI_ENSUREVISIBLE | SVSI_SELECT | SVSI_FOCUSED);
							psf->Release();
						}
						ILFree(pidl);
					}
					psi->Release();
				}
				psia->Release();
			}
			psv->Release();
		}
	}
}

void ShellExplorerControl::setFocusToShellView() const
{
	if (_peb != nullptr)
	{
		IShellView* psv;
		if (SUCCEEDED(_peb->GetCurrentView(IID_PPV_ARGS(&psv))))
		{
			psv->UIActivate(SVUIA_ACTIVATE_FOCUS);
			psv->Release();
		}
	}
}

bool ShellExplorerControl::onAppCommand(UINT cmd)
{
	switch (cmd)
	{
	case APPCOMMAND_BROWSER_BACKWARD:
		_peb->BrowseToIDList(nullptr, SBSP_NAVIGATEBACK);
		return true;
	case APPCOMMAND_BROWSER_FORWARD:
		_peb->BrowseToIDList(nullptr, SBSP_NAVIGATEFORWARD);
		return true;
	case APPCOMMAND_BROWSER_HOME:
		{
			IShellItem* psi = parsingNameToShellItem(getApp()->getSettings().getHomeFolder(true));
			if (psi != nullptr)
			{
				_peb->BrowseToObject(psi, 0);
				psi->Release();
			}
			else
			{
				PIDLIST_ABSOLUTE pidl = nullptr;
				if (SUCCEEDED(SHGetKnownFolderIDList(FOLDERID_Desktop, 0, nullptr, &pidl)))
				{
					_peb->BrowseToIDList(pidl, SBSP_ABSOLUTE);
					ILFree(pidl);
				}
			}
		}
		return true;
	case APPCOMMAND_BROWSER_REFRESH:
		{
			IShellView* pShellView = nullptr;
			if (SUCCEEDED(_peb->GetCurrentView(IID_PPV_ARGS(&pShellView))))
			{
				pShellView->Refresh();
				pShellView->Release();
			}
		}
		return true;
	default:
		break;
	}
	return false;
}

INameSpaceTreeControl* ShellExplorerControl::_getNamespaceTreeControl() const
{
	INameSpaceTreeControl* ret = nullptr;
	if (_peb != nullptr)
	{
		IServiceProvider* psp = nullptr;
		if (SUCCEEDED(_peb->QueryInterface(IID_PPV_ARGS(&psp))))
		{
			INameSpaceTreeControl* pnstc = nullptr;
			if (SUCCEEDED(psp->QueryService(SID_SNavigationPane, IID_PPV_ARGS(&pnstc))))
				ret = pnstc;
			psp->Release();
		}
	}
	return ret;
}

void ShellExplorerControl::_restoreViewColumn(IUnknown* pTarget)
{
	IFolderView2* pfv2;
	if (SUCCEEDED(pTarget->QueryInterface(IID_PPV_ARGS(&pfv2))))
	{
		pfv2->SetSortColumns(&_sortColumn, 1);
		IColumnManager* pcm;
		if (SUCCEEDED(pfv2->QueryInterface(IID_PPV_ARGS(&pcm))))
		{
			std::for_each(_columnWidth.begin(), _columnWidth.end(), [&,this](COLUMN_WIDTH_INFO::value_type& val) {
				CM_COLUMNINFO ci = {0};
				ci.cbSize = sizeof(ci);
				ci.dwMask = CM_MASK_WIDTH;
				ci.uWidth = val.second;
				pcm->SetColumnInfo(val.first, &ci);
			});
			pcm->Release();
		}
		pfv2->Release();
	}
}

void ShellExplorerControl::_saveViewColumn(IFolderView2* pfv2New, IColumnManager* pcmNew)
{
	if (!_isFilesystemFolder)
		return;
	if (_peb != nullptr)
	{
		IFolderView2* pfv2;
		if (SUCCEEDED(_peb->GetCurrentView(IID_PPV_ARGS(&pfv2))))
		{
			SORTCOLUMN sc = {0};
			if (SUCCEEDED(pfv2->GetSortColumns(&sc, 1)))
			{
				_sortColumn = sc;
				if (pfv2New != nullptr)
					pfv2New->SetSortColumns(&sc, 1);
			}
			IColumnManager* pcm;
			if (SUCCEEDED(pfv2->QueryInterface(IID_PPV_ARGS(&pcm))))
			{
				UINT colCount = 0;
				if (SUCCEEDED(pcm->GetColumnCount(CM_ENUM_VISIBLE, &colCount)))
				{
					PROPERTYKEY* keys = new PROPERTYKEY[colCount];
					if (SUCCEEDED(pcm->GetColumns(CM_ENUM_VISIBLE, keys, colCount)))
					{
						_columnWidth.clear();
						for (UINT i = 0; i < colCount; ++i)
						{
							CM_COLUMNINFO ci = {0};
							ci.cbSize = sizeof(ci);
							ci.dwMask = CM_MASK_WIDTH;
							if (SUCCEEDED(pcm->GetColumnInfo(keys[i], &ci)))
							{
								_columnWidth.push_back(COLUMN_WIDTH_INFO::value_type(keys[i], ci.uWidth));
								if (pcmNew != nullptr)
									pcmNew->SetColumnInfo(keys[i], &ci);
							}
						}
					}
					delete [] keys;
				}
				pcm->Release();
			}
			pfv2->Release();
		}
	}
}

void ShellExplorerControl::_setViewColumnNonFSFolder(IUnknown* pTarget, PCIDLIST_ABSOLUTE pidl)
{
	IFolderView2* pfv2;
	if (SUCCEEDED(pTarget->QueryInterface(IID_PPV_ARGS(&pfv2))))
	{
		std::vector<std::pair<PROPERTYKEY, CM_COLUMNINFO>> colInfo;

		IColumnManager* pcm;
		if (SUCCEEDED(pfv2->QueryInterface(IID_PPV_ARGS(&pcm))))
		{
			UINT colCount = 0;
			if (SUCCEEDED(pcm->GetColumnCount(CM_ENUM_VISIBLE, &colCount)))
			{
				PROPERTYKEY* keys = new PROPERTYKEY[colCount];
				if (SUCCEEDED(pcm->GetColumns(CM_ENUM_VISIBLE, keys, colCount)))
				{
					for (UINT i = 0; i < colCount; ++i)
					{
						CM_COLUMNINFO ci = {0};
						ci.cbSize = sizeof(ci);
						ci.dwMask = CM_MASK_WIDTH | CM_MASK_DEFAULTWIDTH | CM_MASK_IDEALWIDTH;
#ifdef _DEBUG
						ci.dwMask |= CM_MASK_NAME;
#endif
						if (SUCCEEDED(pcm->GetColumnInfo(keys[i], &ci)))
							colInfo.push_back(std::pair<PROPERTYKEY, CM_COLUMNINFO>(keys[i], ci));
					}
				}
				delete [] keys;
			}
			pcm->Release();
		}

		bool bSetSort = false;
		SORTCOLUMN sc = {0};
		bool bSetColWidth = false;
		COLUMN_WIDTH_INFO columnWidth;

		wchar_t* name = nullptr;
		if (SUCCEEDED(SHGetNameFromIDList(pidl, SIGDN_DESKTOPABSOLUTEPARSING, &name)))
		{
			if (getName_RecycleBin() == name)
			{
				sc.propkey = getPropertyKey(L"System.Recycle.DateDeleted");
				sc.direction = SORT_DESCENDING;
				bSetSort = true;

				std::for_each(colInfo.begin(), colInfo.end(), [&,this](std::pair<PROPERTYKEY, CM_COLUMNINFO>& val) {
					columnWidth.push_back(COLUMN_WIDTH_INFO::value_type(val.first, val.second.uIdealWidth));
				});
				bSetColWidth = true;
			}
			CoTaskMemFree(name);
		}

		if (bSetSort)
			pfv2->SetSortColumns(&sc, 1);

		if (bSetColWidth)
		{
			if (SUCCEEDED(pfv2->QueryInterface(IID_PPV_ARGS(&pcm))))
			{
				std::for_each(columnWidth.begin(), columnWidth.end(), [&,this](COLUMN_WIDTH_INFO::value_type& val) {
					CM_COLUMNINFO ci = {0};
					ci.cbSize = sizeof(ci);
					ci.dwMask = CM_MASK_WIDTH;
					ci.uWidth = val.second;
					pcm->SetColumnInfo(val.first, &ci);
				});
				pcm->Release();
			}
		}

		pfv2->Release();
	}
}

// IUnknown
IFACEMETHODIMP ShellExplorerControl::QueryInterface(REFIID riid, void** ppv)
{
	static const QITAB qit[] = {
		QITABENT(ShellExplorerControl, IServiceProvider),
		QITABENT(ShellExplorerControl, ICommDlgBrowser),
		{ 0 },
	};
	return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) ShellExplorerControl::AddRef()
{
	return InterlockedIncrement(&_refCount);
}

IFACEMETHODIMP_(ULONG) ShellExplorerControl::Release()
{
	ULONG ret = InterlockedDecrement(&_refCount);
	// Not delete this
	return ret;
}

// IServiceProvider
IFACEMETHODIMP ShellExplorerControl::QueryService(REFGUID rguid, REFIID riid, void** ppv)
{
	HRESULT hr = E_NOINTERFACE;
	*ppv = nullptr;
	if (rguid == SID_SExplorerBrowserFrame)
	{
		// responding to this SID allows us to hook up our ICommDlgBrowser
		// implementation so we get selection change events from the view
		hr = QueryInterface(riid, ppv);
	}
	return hr;
}

// IExplorerBrowserEvents
IFACEMETHODIMP ShellExplorerControl::OnNavigationComplete(PCIDLIST_ABSOLUTE pidl)
{
	HWND hwndParent = GetParent(getWindow());
	if (hwndParent != nullptr)
	{
		IShellItem* psi;
		if (SUCCEEDED(SHCreateItemFromIDList(pidl, IID_PPV_ARGS(&psi))))
		{
			_isFilesystemFolder = isFilesystemFolder(psi);
			psi->Release();
		}
		else
		{
			_isFilesystemFolder = false;
		}
		SendMessage(hwndParent, WM_CHANGED_FOLDER_VIEW, 0, (LPARAM)pidl);
		return S_OK;
	}
	return E_NOTIMPL;
}

IFACEMETHODIMP ShellExplorerControl::OnViewCreated(IShellView* psv)
{
	IFolderView* pfvNew;
	if (FAILED(psv->QueryInterface(IID_PPV_ARGS(&pfvNew))))
		pfvNew = nullptr;
	IFolderView2* pfv2New;
	if (FAILED(psv->QueryInterface(IID_PPV_ARGS(&pfv2New))))
		pfv2New = nullptr;
	IColumnManager* pcmNew;
	if (FAILED(pfv2New->QueryInterface(IID_PPV_ARGS(&pcmNew))))
		pcmNew = nullptr;
	bool bisfilesystemfolder = false;
	PIDLIST_ABSOLUTE pidlNew = nullptr;
	if (pfvNew != nullptr)
	{
		IShellItem* psiNew;
		if (SUCCEEDED(pfvNew->GetFolder(IID_PPV_ARGS(&psiNew))))
		{
			bisfilesystemfolder = isFilesystemFolder(psiNew);
			SHGetIDListFromObject(psiNew, &pidlNew);
			psiNew->Release();
		}
	}

	if (_isFilesystemFolder)
	{
		if (bisfilesystemfolder)
		{
			_saveViewColumn(pfv2New, pcmNew);
		}
		else
		{
			_saveViewColumn();
			_setViewColumnNonFSFolder(psv, pidlNew);
		}
	}
	else
	{
		if (bisfilesystemfolder)
			_restoreViewColumn(psv);
		else
			_setViewColumnNonFSFolder(psv, pidlNew);
	}

	if (pidlNew != nullptr)
		ILFree(pidlNew);
	if (pcmNew != nullptr)
		pcmNew->Release();
	if (pfv2New != nullptr)
		pfv2New->Release();
	if (pfvNew != nullptr)
		pfvNew->Release();
	return S_OK;
}