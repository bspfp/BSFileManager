#pragma once

class SplitViewBase
{
public:
	struct Params
	{
		bool	bRelative;

		int		curAPos;
		int		minAFirst;
		int		minALast;

		double	curRPos;
		double	minRSize;
		double	maxRSize;

		int		splitterSize;

		Params();
		Params(int minFirst, int minLast, int curPos = -1, int splitterSize = 5);
		Params(double minFirst, double maxFirst, double curPos = 0.5, int splitterSize = 5);

		bool change(int curPos);
		bool change(double curPos);
	};

	SplitViewBase(bool bRedraw = true);
	SplitViewBase(int minFirst, int minLast, int curPos = -1, int splitterSize = 5, bool bRedraw = true);
	SplitViewBase(double minFirst, double maxFirst, double curPos = 0.5, int splitterSize = 5, bool bRedraw = true);

	bool create(HWND hwndParent);
	void show(bool bShow = true);
	HWND getWindow() const;

	void setParameters(Params const & val);
	void setDefaultParameters(Params const & val, bool bUpdateCurrent);
	void setRedrawMode(bool val);

	int getCurrentPosA() const;
	void setCurrentPosA(int val);
	double getCurrentPosR() const;
	void setCurrentPosR(double val);

	void setFirst(HWND hwnd, bool bShow);
	void setLast(HWND hwnd, bool bShow);

	HWND getFirst() const;
	HWND getLast() const;

	void showFirst(bool bShow);
	void showLast(bool bShow);

	void resize(RECT const & rectParentClient);
	void resetSplitter();

	void recalculate(bool bDrawSplitter = false);
	virtual void moveSplitter(int x, int y) = 0;
	virtual const wchar_t* getClassName() = 0;

protected:
	bool _registerClass();
	bool _createWindow(HWND hwndParent);
	void _setChild(HWND hwnd, HWND& destHwnd);
	void _refreshCursor();
	void _createSplitter();

	virtual void _onCreate() {}
	virtual HCURSOR _getSizingCursor() = 0;
	virtual void _recalculate(bool bDrawSplitter = false) = 0;
	virtual bool _preWndProc(UINT msg, WPARAM wparam, LPARAM lparam, LRESULT& result) { msg; wparam; lparam; result; return false; }

	static LRESULT CALLBACK _WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	Params	_params;
	Params	_defaultParams;
	HFONT	_hfont;
	HWND	_hwndSelf;
	HWND	_hwndFirst;
	HWND	_hwndLast;
	HWND	_hwndSplliter;
	bool	_bSplitterMoving;
	bool	_bRedraw;
	bool	_bShowFirst;
	bool	_bShowLast;
};

class VSplitView : public SplitViewBase
{
public:
	VSplitView(bool bRedraw = true);
	VSplitView(int minLeft, int minRight, int curPos = -1, int splitterSize = 5, bool bRedraw = true);
	VSplitView(double minLeft, double maxLeft, double curPos = 0.5, int splitterSize = 5, bool bRedraw = true);

	void setLeft(HWND hwnd, bool bShow = true);
	void setRight(HWND hwnd, bool bShow = true);

	HWND getLeft() const;
	HWND getRight() const;

	void showLeft(bool bShow);
	void showRight(bool bShow);

	virtual void moveSplitter(int x, int y);
	virtual const wchar_t* getClassName();

protected:
	virtual HCURSOR _getSizingCursor();
	virtual void _recalculate(bool bDrawSplitter = false);
};

class HSplitView : public SplitViewBase
{
public:
	HSplitView(bool bRedraw = true);
	HSplitView(int minTop, int minBottom, int curPos = -1, int splitterSize = 5, bool bRedraw = true);
	HSplitView(double minTop, double maxTop, double curPos = 0.5, int splitterSize = 5, bool bRedraw = true);

	void setTop(HWND hwnd, bool bShow = true);
	void setBottom(HWND hwnd, bool bShow = true);

	HWND getTop() const;
	HWND getBottom() const;

	void showTop(bool bShow);
	void showBottom(bool bShow);

	virtual void moveSplitter(int x, int y);
	virtual const wchar_t* getClassName();

protected:
	virtual HCURSOR _getSizingCursor();
	virtual void _recalculate(bool bDrawSplitter = false);
};
