/* 
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdafx.h"
#include "ListFilter.h"


ListFilter::ListFilter(size_t colCount, UpdateFunction updateF) :
	colCount(colCount),
	updateFunction(updateF),
	defMethod(StringMatch::PARTIAL),
	defMatchColumn(colCount)

{
}
void ListFilter::addFilterBox(HWND parent) {
	RECT rc = { 0, 0, 100, 120 };
	text.Create(parent, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);
	text.SetFont(WinUtil::systemFont);
	WinUtil::addCue(text.m_hWnd, CTSTRING(FILTER_DOTS), TRUE);

}

void ListFilter::addColumnBox(HWND parent, vector<ColumnInfo*>& columns){
	RECT rc = { 0, 0, 100, 110 };
	column.Create(parent, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL |
		WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	column.SetFont(WinUtil::systemFont);
	for(auto& col : columns)
		column.AddString(((*col).name).c_str());

	column.AddString(_T("Any"));
	column.SetCurSel(colCount);
}


void ListFilter::addMethodBox(HWND parent){
	RECT rc = { 0, 0, 100, 110 };
	method.Create(parent, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL |
		WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	method.SetFont(WinUtil::systemFont);
	tstring methods[StringMatch::METHOD_LAST] = { TSTRING(PARTIAL_MATCH), TSTRING(REGULAR_EXPRESSION), TSTRING(WILDCARD), TSTRING(EXACT_MATCH)  };
	for(auto& str : methods) 
		method.AddString(str.c_str());

	method.SetCurSel(defMethod);

}

LRESULT ListFilter::onFilterChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	tstring filter = Util::emptyStringT;

	if(uMsg == WM_PASTE) {
		if(!IsClipboardFormatAvailable(CF_TEXT))
			return 0;
		if(!::OpenClipboard(WinUtil::mainWnd))
			return 0;
	}

	if(!SETTING(FILTER_ENTER) || (wParam == VK_RETURN)) {
		if(uMsg == WM_PASTE) {
			HGLOBAL hglb = GetClipboardData(CF_TEXT); 
			if(hglb != NULL) { 
				char* lptstr = (char*)GlobalLock(hglb); 
				if(lptstr != NULL) {
					string tmp(lptstr);
					filter = Text::toT(tmp);
					GlobalUnlock(hglb);
				}
			}
			CloseClipboard();
		}else if(uMsg == WM_CUT) {
			int begin, end;
			text.GetSel(begin, end);
			tstring buf;
			buf.resize(text.GetWindowTextLength());
			buf.resize(text.GetWindowText(&buf[0], text.GetWindowTextLength() + 1));
			buf.erase(begin, end);
			filter = buf;
		}else if(uMsg == WM_CLEAR) {
			filter.clear();
		} else {
			TCHAR *buf = new TCHAR[text.GetWindowTextLength()+1];
			text.GetWindowText(buf, text.GetWindowTextLength()+1);
			filter = buf;
			delete[] buf;
		}

		textUpdated(Text::fromT(filter));
	}

	bHandled = FALSE;
	return 0;
}


size_t ListFilter::getMethod() {
	if(!method.IsWindow()) 
		return defMethod;

	return method.GetCurSel();
}

size_t ListFilter::getColumn() {
	if(!column.IsWindow()) 
		return defMatchColumn;

	return column.GetCurSel();
}


ListFilter::Preparation ListFilter::prepare() {
	Preparation prep = Preparation();
	if(empty())
		return prep;

	prep.column = getColumn();
	prep.method = getMethod();

	if(prep.method < StringMatch::METHOD_LAST) {
		matcher.setMethod(static_cast<StringMatch::Method>(prep.method));
		matcher.prepare();
	}

	return prep;
}

bool ListFilter::match(const Preparation& prep, InfoFunction infoF) const {
	if(empty())
		return true;

	if(prep.method < StringMatch::METHOD_LAST) {
		if(prep.column >= colCount) {
			for(size_t i = 0; i < colCount; ++i) {
				if(matcher.match(infoF(i))) {
					return true;
				}
			}
		} else {
			return matcher.match(infoF(prep.column));
		}
	}
	return false;
}

bool ListFilter::empty() const {
	return matcher.pattern.empty();
}

void ListFilter::textUpdated(const string& filter) {
	if(filter != matcher.pattern) {
		matcher.pattern = filter;
		updateFunction();
	}
}

void ListFilter::columnChanged() {
	updateFunction();
}
