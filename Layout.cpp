#include "stdafx.h"
#include "Layout.h"

void LayoutBase::Control::setFont(HFONT hfont, BOOL bRedraw) const
{
	if (pLayout != nullptr)
	{
		pLayout->setFont(hfont, bRedraw);
	}
	else if (hwnd != nullptr)
	{
		SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont, bRedraw);
	}
}

LayoutBase::LayoutBase()
	: _hwnd(nullptr)
	, _hfont(nullptr)
{
}

void LayoutBase::setWindow(HWND hwnd, bool bSetFont)
{
	_hwnd = hwnd;
	SetClassLong(_hwnd, GCL_STYLE, GetClassLong(_hwnd, GCL_STYLE) | CS_VREDRAW | CS_HREDRAW);
	if (bSetFont)
		setFont((HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0), TRUE);
}

void LayoutBase::setFont(HFONT hfont, BOOL bRedraw)
{
	_hfont = hfont;
	std::list<Control> controls;
	getAllControls(controls);
	for (auto iter = controls.begin(); iter != controls.end(); ++iter)
		iter->setFont(hfont, bRedraw);
}

void LayoutBase::recalculate() const
{
	if (_hwnd != nullptr)
	{
		RECT rect;
		GetClientRect(_hwnd, &rect);
		_recalculate(rect);
	}
}

bool FLayout::add(Control const & control, Type type, int x, int y, int width, int height)
{
	if (control.key() == nullptr)
		return false;
	if (this == control.pLayout)
		return false;

	FLayoutData data;
	data.control = control;
	data.type = type;
	data.x = x;
	data.y = y;
	data.w = width;
	data.h = height;
	if (!_controls.insert(std::make_pair(control.key(), data)).second)
		return false;
	control.setFont(_hfont);
	return true;
}

bool FLayout::move(Control const & control, Type type, int x, int y, int width, int height)
{
	if (control.key() == nullptr)
		return false;
	auto iter = _controls.find(control.key());
	if (iter == _controls.end())
		return false;
	iter->second.type = type;
	iter->second.x = x;
	iter->second.y = y;
	iter->second.w = width;
	iter->second.h = height;
	return true;
}

void FLayout::remove(Control const & control)
{
	if (control.key() == nullptr)
		return;
	_controls.erase(control.key());
}

void FLayout::clear()
{
	_controls.clear();
}

void FLayout::getAllControls(std::list<Control>& outList) const
{
	for (auto iter = _controls.begin(); iter != _controls.end(); ++iter)
		outList.push_back(iter->second.control);
}

void FLayout::_recalculate(RECT const & rect) const
{
	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top;
	for (auto iter = _controls.begin(); iter != _controls.end(); ++iter)
	{
		int destX, destY;
		FLayoutData const & data = iter->second;
		switch (data.type)
		{
		case TOP_LEFT:
			destX = data.x;
			destY = data.y;
			break;
		case TOP_RIGHT:
			destX = w - data.x - data.w;
			destY = data.y;
			break;
		case BOTTOM_LEFT:
			destX = data.x;
			destY = h - data.y - data.h;
			break;
		case BOTTOM_RIGHT:
			destX = w - data.x - data.w;
			destY = h - data.y - data.h;
			break;
		default:
			continue;
		}

		if (data.control.hwnd != nullptr)
		{
			MoveWindow(data.control.hwnd, destX + rect.left, destY + rect.top, data.w, data.h, TRUE);
		}
		else
		{
			RECT rectResult;
			SetRect(&rectResult, destX + rect.left, destY + rect.top, destX + rect.left + data.w, destY + rect.top + data.h);
			data.control.pLayout->_recalculate(rectResult);
		}
	}
}

TLayout::TLayout()
	: _fixedWidth(0)
	, _autoColumnCount(0)
	, _fixedHeight(0)
	, _autoRowCount(0)
{
	memset(_padding, 0, sizeof(_padding));
}

bool TLayout::add(Control const & control, int col, int row, int width, int height)
{
	if (control.key() == nullptr)
		return false;
	if (this == control.pLayout)
		return false;

	if (_colWidth.empty() || _rowHeight.empty())
		return false;

	TLayoutData data;
	data.control = control;
	data.col = col;
	data.row = row;
	data.w = width;
	data.h = height;
	if (!_controls.insert(std::make_pair(control.key(), data)).second)
		return false;
	control.setFont(_hfont);
	return true;
}

void TLayout::resize(int colCount, int rowCount, int padding)
{
	int* col = new int[colCount];
	int* row = new int[rowCount];
	for (int i = 0; i < colCount; ++i)
		col[i] = TLayout::AUTO_SIZE;
	for (int i = 0; i < rowCount; ++i)
		row[i] = TLayout::AUTO_SIZE;
	_resize(colCount, col, rowCount, row, padding, padding, padding, padding);
	delete [] row;
	delete [] col;
}

void TLayout::resize(int colCount, int rowCount, int paddingLeft, int paddingTop, int paddingRight, int paddingBottom)
{
	int* col = new int[colCount];
	int* row = new int[rowCount];
	for (int i = 0; i < colCount; ++i)
		col[i] = TLayout::AUTO_SIZE;
	for (int i = 0; i < rowCount; ++i)
		row[i] = TLayout::AUTO_SIZE;
	_resize(colCount, col, rowCount, row, paddingLeft, paddingTop, paddingRight, paddingBottom);
	delete [] row;
	delete [] col;
}

void TLayout::resize(int colCount, int rowCount, int (&padding)[4])
{
	int* col = new int[colCount];
	int* row = new int[rowCount];
	for (int i = 0; i < colCount; ++i)
		col[i] = TLayout::AUTO_SIZE;
	for (int i = 0; i < rowCount; ++i)
		row[i] = TLayout::AUTO_SIZE;
	_resize(colCount, col, rowCount, row, padding[0], padding[1], padding[2], padding[3]);
	delete [] row;
	delete [] col;
}

int TLayout::getColumnCount() const
{
	return (int)_colWidth.size();
}

int TLayout::getRowCount() const
{
	return (int)_rowHeight.size();
}

int TLayout::getWidth(int col) const
{
	return _colWidth[col];
}

int TLayout::getHeight(int row) const
{
	return _rowHeight[row];
}

void TLayout::setWidth(int col, int width, RECT* pRect)
{
	if (col >= 0 && col < getColumnCount())
	{
		int old = _colWidth[col];
		if (old != width)
		{
			_colWidth[col] = width;
			if (old == TLayout::AUTO_SIZE)
			{
				--_autoColumnCount;
				_fixedWidth += width;
			}
			else if (width == TLayout::AUTO_SIZE)
			{
				++_autoColumnCount;
				_fixedWidth -= old;
			}
			else
			{
				_fixedWidth += width - old;
			}
			if (pRect != nullptr)
				_recalculate(*pRect);
		}
	}
}

void TLayout::setHeight(int row, int height, RECT* pRect)
{
	if (row >=0 && row < getRowCount())
	{
		int old = _rowHeight[row];
		if (old != height)
		{
			_rowHeight[row] = height;
			if (old == TLayout::AUTO_SIZE)
			{
				--_autoRowCount;
				_fixedHeight += height;
			}
			else if (height == TLayout::AUTO_SIZE)
			{
				++_autoRowCount;
				_fixedHeight -= old;
			}
			else
			{
				_fixedHeight += height - old;
			}
			if (pRect != nullptr)
				_recalculate(*pRect);
		}
	}
}

void TLayout::remove(Control const & control)
{
	if (control.key() == nullptr)
		return;
	_controls.erase(control.key());
}

void TLayout::clear()
{
	_controls.clear();
}

void TLayout::getAllControls(std::list<Control>& outList) const
{
	for (auto iter = _controls.begin(); iter != _controls.end(); ++iter)
		outList.push_back(iter->second.control);
}

void TLayout::_recalculate(RECT const & rect) const
{
	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top;

	for (auto iter = _controls.begin(); iter != _controls.end(); ++iter)
	{
		TLayoutData const & data = iter->second;

		RECT rectResult;
		_getRect(rectResult, data.col, data.row, data.w, data.h, w, h);
		OffsetRect(&rectResult, rect.left, rect.top);

		rectResult.left = std::min(rectResult.left + _padding[0], rectResult.right);
		rectResult.top = std::min(rectResult.top + _padding[1], rectResult.bottom);
		rectResult.right = std::max(rectResult.right - _padding[2], rectResult.left);
		rectResult.bottom = std::max(rectResult.bottom - _padding[3], rectResult.top);

		if (data.control.hwnd != nullptr)
		{
			MoveWindow(data.control.hwnd, rectResult.left, rectResult.top, rectResult.right - rectResult.left, rectResult.bottom - rectResult.top, TRUE);
		}
		else
		{
			data.control.pLayout->_recalculate(rectResult);
		}
	}
}

void TLayout::_resize(size_t columnCount, int* columnWidth, size_t rowCount, int* rowHeight, int paddingLeft, int paddingTop, int paddingRight, int paddingBottom)
{
	_colWidth.clear();
	_rowHeight.clear();
	_fixedWidth = 0;
	_autoColumnCount = 0;
	_fixedHeight = 0;
	_autoRowCount = 0;
	for (size_t i = 0; i < columnCount; ++i)
	{
		_colWidth.push_back(columnWidth[i]);
		if (columnWidth[i] == AUTO_SIZE)
		{
			++_autoColumnCount;
		}
		else if (columnWidth[i] >= 0)
		{
			_fixedWidth += columnWidth[i];
		}
		else
		{
			throw 0;
		}
	}
	for (size_t i = 0; i < rowCount; ++i)
	{
		_rowHeight.push_back(rowHeight[i]);
		if (rowHeight[i] == AUTO_SIZE)
		{
			++_autoRowCount;
		}
		else if (rowHeight[i] >= 0)
		{
			_fixedHeight += rowHeight[i];
		}
		else
		{
			throw 0;
		}
	}
	_padding[0] = paddingLeft;
	_padding[1] = paddingTop;
	_padding[2] = paddingRight;
	_padding[3] = paddingBottom;
}

void TLayout::_getRect(RECT& outRect, int col, int row, int w, int h, int parentW, int parentH) const
{
	col = std::min(col, (int)_colWidth.size() - 1);
	row = std::min(row, (int)_rowHeight.size() - 1);
	w = std::min(w + col, (int)_colWidth.size()) - col;
	h = std::min(h + row, (int)_rowHeight.size()) - row;

	RECT rectCur;
	SetRectEmpty(&outRect);
	for (int iCol = 0; iCol < w; ++iCol)
	{
		for (int iRow = 0; iRow < h; ++iRow)
		{
			_getRect(rectCur, col + iCol, row + iRow, parentW, parentH);
			UnionRect(&outRect, &outRect, &rectCur);
		}
	}
}

void TLayout::_getRect(RECT& outRect, int col, int row, int parentW, int parentH) const
{
	double rw = parentW * 1.0 / std::max(_fixedWidth, 1);
	double rh = parentH * 1.0 / std::max(_fixedHeight, 1);
	int aw = (_autoColumnCount < 1 || parentW <= _fixedWidth) ? 0 : (int)((parentW - _fixedWidth) * 1.0 / _autoColumnCount);
	int ah = (_autoRowCount < 1 || parentH <= _fixedHeight) ? 0 : (int)((parentH - _fixedHeight) * 1.0 / _autoRowCount);

	auto func = [&,this](int& result, int val, int autoSize, double ratio){
		if (val == AUTO_SIZE)
		{
			result += autoSize;
		}
		else if (val >= 0)
		{
			if (ratio >= 1.0)
				result += val;
			else
				result += (int)(val * ratio);
		}
		else
		{
			throw 0;
		}
	};

	int left = 0;
	for (int i = 0; i < col; ++i)
		func(left, _colWidth[i], aw, rw);

	int top = 0;
	for (int i = 0; i < row; ++i)
		func(top, _rowHeight[i], ah, rh);

	int right = left;
	if (col == (int)(_colWidth.size() - 1))
		right = parentW;
	else
		func(right, _colWidth[col], aw, rw);

	int bottom = top;
	if (row == (int)(_rowHeight.size() - 1))
		bottom = parentH;
	else
		func(bottom, _rowHeight[row], ah, rh);

	SetRect(&outRect, left, top, right, bottom);
}

void RLayout::_getRect(RECT& outRect, int col, int row, int parentW, int parentH) const
{
	int aw = (_autoColumnCount < 1 || _fixedWidth >= 100) ? 0 : (int)(parentW * (100.0 - _fixedWidth) / 100.0 / _autoColumnCount);
	int ah = (_autoRowCount < 1 || _fixedHeight >= 100) ? 0 : (int)(parentH * (100.0 - _fixedHeight) / 100.0 / _autoRowCount);

	auto func = [&,this](int& result, int val, int autoSize, int fixed, int parent){
		if (val == AUTO_SIZE)
		{
			result += autoSize;
		}
		else if (val >= 0)
		{
			if (fixed < 100)
				result += (int)(parent * 1.0 * val / 100);
			else
				result += (int)(parent * 1.0 * val / fixed);
		}
		else
		{
			throw 0;
		}
	};

	int left = 0;
	for (int i = 0; i < col; ++i)
		func(left, _colWidth[i], aw, _fixedWidth, parentW);

	int top = 0;
	for (int i = 0; i < row; ++i)
		func(top, _rowHeight[i], ah, _fixedHeight, parentH);

	int right = left;
	if (col == (int)(_colWidth.size() - 1))
		right = parentW;
	else
		func(right, _colWidth[col], aw, _fixedWidth, parentW);

	int bottom = top;
	if (row == (int)(_rowHeight.size() - 1))
		bottom = parentH;
	else
		func(bottom, _rowHeight[row], ah, _fixedHeight, parentH);

	SetRect(&outRect, left, top, right, bottom);
}

FlowLayout::FlowLayout(int sx, int sy)
	: _sx(std::min(sx, 32))
	, _sy(std::min(sy, 32))
	, _align(false)
{
}

bool FlowLayout::add(Control const & control, int w, int h, bool bLineBreak)
{
	if (control.key() == nullptr)
		return false;
	if (this == control.pLayout)
		return false;

	if (control.hwnd != nullptr)
	{
		RECT rc;
		GetWindowRect(control.hwnd, &rc);
		if (w < 0)
			w = rc.right - rc.left;
		if (h < 0)
			h = rc.bottom - rc.top;
	}
	else
	{
		if (w < 0)
			w = std::max(w, _sx);
		if (h < 0)
			h = std::max(w, _sy);
	}

	FlowLayoutData data;
	data.control = control;
	data.w = w;
	data.h = h;
	data.bLineBreak = bLineBreak;

	for (auto it = _controls.begin(), itEnd = _controls.end(); it != itEnd; ++it)
	{
		if (it->control.key() == control.key())
			return false;
	}
	_controls.push_back(data);
	control.setFont(_hfont);
	return true;
}

void FlowLayout::setAlign(bool align)
{
	_align = align;
}

void FlowLayout::getAllControls(std::list<Control>& outList) const
{
	for (auto iter = _controls.begin(); iter != _controls.end(); ++iter)
		outList.push_back(iter->control);
}

void FlowLayout::remove(Control const & control)
{
	if (control.key() == nullptr)
		return;
	for (auto it = _controls.begin(), itEnd = _controls.end(); it != itEnd;)
	{
		if (it->control.key() == control.key())
			it = _controls.erase(it);
		else
			++it;
	}
}

void FlowLayout::clear()
{
	_controls.clear();
}

HLayout::HLayout(int sx, int sy)
	: FlowLayout(sx, sy)
{
}

std::pair<int, int> HLayout::calculate(int w, int /*h*/, std::list<std::pair<RECT, const FlowLayoutData*>>* outResult) const
{
	int tx = 0;
	int ty = 0;
	int maxH = 0;
	int lastX = 0;
	int lastY = 0;
	if (outResult != nullptr)
		outResult->clear();
	for (auto iter = _controls.begin(); iter != _controls.end(); ++iter)
	{
		FlowLayoutData const & data = *iter;

		int destX, destY;
		if (data.bLineBreak || (tx > 0 && tx + _sx + data.w > w))
		{
			tx = 0;
			ty += _sy + maxH;
			maxH = 0;
		}
		destX = tx + _sx;
		destY = ty + _sy;
		tx += _sx + data.w;
		maxH = std::max(maxH, data.h);
		lastX = std::max(lastX, destX + data.w);
		lastY = std::max(lastY, destY + data.h);

		RECT rt;
		SetRect(&rt, destX, destY, destX + data.w, destY + data.h);
		if (outResult != nullptr)
			outResult->push_back(std::make_pair(rt, &data));
	}
	return std::make_pair(lastX + _sx, lastY + _sy);
}

void HLayout::_recalculate(RECT const & rect) const
{
	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top;
	std::list<std::pair<RECT, const FlowLayoutData*>> result;
	calculate(w, h, &result);
	std::map<int, int> sizeMap;
	for (auto it = result.begin(), itEnd = result.end(); it != itEnd; ++it)
	{
		auto ib = sizeMap.insert(std::make_pair(it->first.top, it->second->h));
		if (!ib.second)
			ib.first->second = std::max(ib.first->second, it->second->h);
	}
	for (auto it = result.begin(), itEnd = result.end(); it != itEnd; ++it)
	{
		FlowLayoutData const & data = *(it->second);
		RECT rt = it->first;
		int cx = data.w;
		int cy = (_align) ? sizeMap[rt.top] : data.h;
		if (data.control.hwnd != nullptr)
		{
			MoveWindow(data.control.hwnd, rt.left + rect.left, rt.top + rect.top, cx, cy, TRUE);
		}
		else
		{
			RECT rectResult;
			SetRect(&rectResult, rt.left, rt.top, rt.left + cx, rt.top + cy);
			OffsetRect(&rectResult, rect.left, rect.top);
			data.control.pLayout->_recalculate(rectResult);
		}
	}
}

VLayout::VLayout(int sx, int sy)
	: FlowLayout(sx, sy)
{
}

std::pair<int, int> VLayout::calculate(int /*w*/, int h, std::list<std::pair<RECT, const FlowLayoutData*>>* outResult) const
{
	int tx = 0;
	int ty = 0;
	int maxW = 0;
	int lastX = 0;
	int lastY = 0;
	if (outResult != nullptr)
		outResult->clear();
	for (auto iter = _controls.begin(); iter != _controls.end(); ++iter)
	{
		FlowLayoutData const & data = *iter;

		int destX, destY;
		if (data.bLineBreak || (ty > 0 && ty + _sy + data.h > h))
		{
			tx += _sx + maxW;
			ty = 0;
			maxW = 0;
		}
		destX = tx + _sx;
		destY = ty + _sy;
		ty += _sy + data.h;
		maxW = std::max(maxW, data.w);
		lastX = std::max(lastX, destX + data.w);
		lastY = std::max(lastY, destY + data.h);

		RECT rt;
		SetRect(&rt, destX, destY, destX + data.w, destY + data.h);
		if (outResult != nullptr)
			outResult->push_back(std::make_pair(rt, &data));
	}
	return std::make_pair(lastX + _sx, lastY + _sy);
}

void VLayout::_recalculate(RECT const & rect) const
{
	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top;
	std::list<std::pair<RECT, const FlowLayoutData*>> result;
	calculate(w, h, &result);
	std::map<int, int> sizeMap;
	for (auto it = result.begin(), itEnd = result.end(); it != itEnd; ++it)
	{
		auto ib = sizeMap.insert(std::make_pair(it->first.left, it->second->w));
		if (!ib.second)
			ib.first->second = std::max(ib.first->second, it->second->w);
	}
	for (auto it = result.begin(), itEnd = result.end(); it != itEnd; ++it)
	{
		FlowLayoutData const & data = *(it->second);
		RECT rt = it->first;
		int cx = (_align) ? sizeMap[rt.left] : data.w;
		int cy = data.h;
		if (data.control.hwnd != nullptr)
		{
			MoveWindow(data.control.hwnd, rt.left + rect.left, rt.top + rect.top, cx, cy, TRUE);
		}
		else
		{
			RECT rectResult;
			SetRect(&rectResult, rt.left, rt.top, rt.left + cx, rt.top + cy);
			OffsetRect(&rectResult, rect.left, rect.top);
			data.control.pLayout->_recalculate(rectResult);
		}
	}
}