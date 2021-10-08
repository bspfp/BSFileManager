#pragma once

#include <unordered_map>
#include <list>
#include <map>
#include <algorithm>

class FLayout;
class TLayout;
class RLayout;
class HLayout;
class VLayout;

// Layout �⺻ Ŭ����
class LayoutBase : public NonCopyable
{
	friend class FLayout;
	friend class TLayout;
	friend class RLayout;
	friend class HLayout;
	friend class VLayout;

public:
	struct Control
	{
		LayoutBase* pLayout;
		HWND		hwnd;

		Control() : pLayout(nullptr), hwnd(nullptr) {}
		Control(LayoutBase* pLayout) : pLayout(pLayout), hwnd(nullptr) {}
		Control(HWND hwnd) : pLayout(nullptr), hwnd(hwnd) {}

		bool operator==(Control const & rhs) { return (pLayout == rhs.pLayout && hwnd == rhs.hwnd); }
		bool operator!=(Control const & rhs) { return !(*this == rhs); }
		void* key() const { return (pLayout != nullptr) ? (void*)pLayout : (void*)hwnd; }
		void setFont(HFONT hfont, BOOL bRedraw = TRUE) const;
	};

	LayoutBase();

	void setWindow(HWND hwnd, bool bSetFont = false);

	void setFont(HFONT hfont, BOOL bRedraw);
	void recalculate() const;

	virtual void getAllControls(std::list<Control>& outList) const = 0;
	virtual void remove(Control const & control) = 0;
	virtual void clear() = 0;

protected:
	virtual void _recalculate(RECT const & rect) const = 0;

	HWND	_hwnd;
	HFONT	_hfont;
};

// 4�� �𼭸��� �������� ��ġ�� ũ�⸦ ���ϴ� ���̾ƿ�
class FLayout : public LayoutBase
{
public:
	enum Type
	{
		TOP_LEFT,
		TOP_RIGHT,
		BOTTOM_LEFT,
		BOTTOM_RIGHT,
	};

	bool add(Control const & control, Type type, int x, int y, int width, int height);

	bool move(Control const & control, Type type, int x, int y, int width, int height);

	virtual void remove(Control const & control);
	virtual void clear();
	virtual void getAllControls(std::list<Control>& outList) const;
	virtual void _recalculate(RECT const & rect) const;

private:
	struct FLayoutData
	{
		LayoutBase::Control		control;
		FLayout::Type			type;
		int						x, y, w, h;
	};

	std::unordered_map<void*, FLayoutData>	_controls;
};

// ���̺� ������ ���̾ƿ� (ũ�� ������ �ȼ����� ��)
// n x m�� ���̺��� ���� ��, ���� ����, ���� ������ �����ϰ�
// ��Ʈ���� (left, top) - (right, bottom)���� ��ġ�� ����
class TLayout : public LayoutBase
{
public:
	enum
	{
		AUTO_SIZE = -1,			// �� ����/�� ���̸� �ڵ����� ����� ���� ���
		NO_CHANGE = -2,			// resize���� ���� ũ�⸦ �������� �ʰ��� �� ��
	};

	TLayout();

	bool add(Control const & control, int col, int row, int width = 1, int height = 1);

	template <size_t columnCount, size_t rowCount>
	void resize(int (&columnWidth)[columnCount], int (&rowHeight)[rowCount], int padding);

	template <size_t columnCount, size_t rowCount>
	void resize(int (&columnWidth)[columnCount], int (&rowHeight)[rowCount], int paddingLeft, int paddingTop, int paddingRight, int paddingBottom);

	template <size_t columnCount, size_t rowCount>
	void resize(int (&columnWidth)[columnCount], int (&rowHeight)[rowCount], int (&padding)[4]);

	void resize(int colCount, int rowCount, int padding);
	void resize(int colCount, int rowCount, int paddingLeft, int paddingTop, int paddingRight, int paddingBottom);
	void resize(int colCount, int rowCount, int (&padding)[4]);

	int getColumnCount() const;
	int getRowCount() const;

	int getWidth(int col) const;
	int getHeight(int row) const;
	void setWidth(int col, int width, RECT* pRect = nullptr);
	void setHeight(int row, int height, RECT* pRect = nullptr);

	virtual void remove(Control const & control);
	virtual void clear();
	virtual void getAllControls(std::list<Control>& outList) const;
	virtual void _recalculate(RECT const & rect) const;

protected:
	void _resize(size_t columnCount, int* columnWidth, size_t rowCount, int* rowHeight, int paddingLeft, int paddingTop, int paddingRight, int paddingBottom);
	void _getRect(RECT& outRect, int col, int row, int w, int h, int parentW, int parentH) const;
	virtual void _getRect(RECT& outRect, int col, int row, int parentW, int parentH) const;

	struct TLayoutData
	{
		LayoutBase::Control		control;
		int						col, row, w, h;
	};

	std::vector<int>	_colWidth;
	std::vector<int>	_rowHeight;
	int					_padding[4];

	int			_fixedWidth;
	int			_autoColumnCount;
	int			_fixedHeight;
	int			_autoRowCount;

	std::unordered_map<void*, TLayoutData>	_controls;
};

// ���̺� ������ ���̾ƿ� (ũ�� ������ �ۼ�Ʈ�� ��)
// n x m�� ���̺��� ���� ��, ���� ����, ���� ������ �����ϰ�
// ��Ʈ���� (left, top) - (right, bottom)���� ��ġ�� ����
class RLayout : public TLayout
{
protected:
	virtual void _getRect(RECT& outRect, int col, int row, int parentW, int parentH) const;
};

// H/VLayout �������̽�
class FlowLayout : public LayoutBase
{
public:
	struct FlowLayoutData
	{
		LayoutBase::Control		control;
		int						w, h;
		bool					bLineBreak;
	};

	// ������: ��Ʈ�� ���� ������ �Ķ���ͷ� ���´�. �ִ� 32 pixel
	FlowLayout(int sx = 2, int sy = 2);

	bool add(Control const & control, int w = -1, int h = -1, bool bLineBreak = false);
	void setAlign(bool align);

	virtual void getAllControls(std::list<Control>& outList) const;
	virtual void remove(Control const & control);
	virtual void clear();

	virtual std::pair<int, int> calculate(int w, int h, std::list<std::pair<RECT, const FlowLayoutData*>>* outResult = nullptr) const = 0;

protected:
	int _sx;
	int _sy;
	bool _align;
	std::list<FlowLayoutData> _controls;
};

// �������� ��Ʈ���� ��ġ�ϴ� ���̾ƿ�
class HLayout : public FlowLayout
{
public:
	// ������: ��Ʈ�� ���� ������ �Ķ���ͷ� ���´�. �ִ� 32 pixel
	HLayout(int sx = 2, int sy = 2);

	virtual std::pair<int, int> calculate(int w, int h, std::list<std::pair<RECT, const FlowLayoutData*>>* outResult = nullptr) const;

protected:
	virtual void _recalculate(RECT const & rect) const;
};

// �������� ��Ʈ���� ��ġ�ϴ� ���̾ƿ�
class VLayout : public FlowLayout
{
public:
	// ������: ��Ʈ�� ���� ������ �Ķ���ͷ� ���´�. �ִ� 32 pixel
	VLayout(int sx = 2, int sy = 2);

	virtual std::pair<int, int> calculate(int w, int h, std::list<std::pair<RECT, const FlowLayoutData*>>* outResult = nullptr) const;

protected:
	virtual void _recalculate(RECT const & rect) const;
};

// ����
template <size_t columnCount, size_t rowCount>
inline void TLayout::resize(int (&columnWidth)[columnCount], int (&rowHeight)[rowCount], int padding)
{
	_resize(columnCount, columnWidth, rowCount, rowHeight, padding, padding, padding, padding);
}

template <size_t columnCount, size_t rowCount>
inline void TLayout::resize(int (&columnWidth)[columnCount], int (&rowHeight)[rowCount], int paddingLeft, int paddingTop, int paddingRight, int paddingBottom)
{
	_resize(columnCount, columnWidth, rowCount, rowHeight, paddingLeft, paddingTop, paddingRight, paddingBottom);
}

template <size_t columnCount, size_t rowCount>
inline void TLayout::resize(int (&columnWidth)[columnCount], int (&rowHeight)[rowCount], int (&padding)[4])
{
	_resize(columnCount, columnWidth, rowCount, rowHeight, padding[0], padding[1], padding[2], padding[3]);
}
