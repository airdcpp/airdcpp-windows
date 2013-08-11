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

#include "WinUtil.h"


ListFilter::ListFilter(size_t colCount, UpdateFunction updateF) :
	colCount(colCount),
	updateFunction(updateF),
	defMethod(StringMatch::PARTIAL),
	defMatchColumn(colCount),
	inverse(false)
{
}
void ListFilter::addFilterBox(HWND parent) {
	RECT rc = { 0, 0, 100, 120 };
	text.Create(parent, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);
	text.SetFont(WinUtil::systemFont);
	WinUtil::addCue(text.m_hWnd, CTSTRING(FILTER_DOTS), TRUE);

}

void ListFilter::addColumnBox(HWND parent, vector<ColumnInfo*>& aColumns){
	RECT rc = { 0, 0, 100, 110 };
	column.Create(parent, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL |
		WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	column.SetFont(WinUtil::systemFont);
	columns = aColumns;
	for(auto& col : columns)
		column.AddString(((*col).name).c_str());

	column.AddString(CTSTRING(ANY));
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

void ListFilter::clear() {
	text.SetWindowText(_T(""));
	//textUpdated(Util::emptyString);
	matcher.pattern = Util::emptyString;
}

void ListFilter::setInverse(bool aInverse) {
	inverse = aInverse;
	WinUtil::addCue(text.m_hWnd, inverse ? CTSTRING(EXCLUDE_DOTS) : CTSTRING(FILTER_DOTS), FALSE);
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
		} else if(uMsg == WM_CUT || uMsg == WM_CLEAR) {
			int begin, end;
			text.GetSel(begin, end);
			tstring buf;
			buf.resize(text.GetWindowTextLength());
			buf.resize(text.GetWindowText(&buf[0], text.GetWindowTextLength() + 1));
			buf.erase(begin, end);
			filter = buf;
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


size_t ListFilter::getMethod() const {
	if(!method.IsWindow()) 
		return defMethod;

	return method.GetCurSel();
}

size_t ListFilter::getColumn() const {
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
	} else {
		prep.size = prepareSize();
	}

	return prep;
}

bool ListFilter::match(const Preparation& prep, InfoFunction infoF) const {
	if(empty())
		return true;

	bool hasMatch = false;
	if(prep.method < StringMatch::METHOD_LAST) {
		if(prep.column >= colCount) {
			for(size_t i = 0; i < colCount; ++i) {
				if(matcher.match(infoF(i))) {
					hasMatch = true;
					break;
				}
			}
		} else {
			hasMatch = matcher.match(infoF(prep.column));
		}
	} else {
		auto size = Util::toDouble(infoF(prep.column));
		switch(prep.method - StringMatch::METHOD_LAST) {
		case EQUAL: hasMatch = size == prep.size; break;
		case GREATER_EQUAL: hasMatch = size >= prep.size; break;
		case LESS_EQUAL: hasMatch = size <= prep.size; break;
		case GREATER: hasMatch = size > prep.size; break;
		case LESS: hasMatch = size < prep.size; break;
		case NOT_EQUAL: hasMatch = size != prep.size; break;
		}
	}
	return inverse ? !hasMatch : hasMatch;
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
	if(column.IsWindow() && method.IsWindow()) {
		auto n = getMethod();
		size_t col = getColumn();

		if(col < colCount && columns[col]->colType == COLUMN_NUMERIC) {
			if(n <= StringMatch::METHOD_LAST) {
				method.AddString(_T("="));
				method.AddString(_T(">="));
				method.AddString(_T("<="));
				method.AddString(_T(">"));
				method.AddString(_T("<"));
				method.AddString(_T("!="));
			}

		} else if(n > StringMatch::METHOD_LAST) {
			for(size_t i = StringMatch::METHOD_LAST; i < n; ++i) {
				method.DeleteString(StringMatch::METHOD_LAST);
			}
			if(getMethod() == -1) {
				method.SetCurSel(StringMatch::PARTIAL);
			}
		}
	}

	updateFunction();
}
double ListFilter::prepareSize() const {
	size_t end;
	int64_t multiplier;

	if((end = Util::findSubString(matcher.pattern, ("TiB"))) != string::npos) {
		multiplier = 1024LL * 1024LL * 1024LL * 1024LL;
	} else if((end = Util::findSubString(matcher.pattern, ("GiB"))) != string::npos) {
		multiplier = 1024 * 1024 * 1024;
	} else if((end = Util::findSubString(matcher.pattern, ("MiB"))) != string::npos) {
		multiplier = 1024 * 1024;
	} else if((end = Util::findSubString(matcher.pattern, ("KiB"))) != string::npos) {
		multiplier = 1024;
	} else if((end = Util::findSubString(matcher.pattern, ("TB"))) != string::npos) {
		multiplier = 1000LL * 1000LL * 1000LL * 1000LL;
	} else if((end = Util::findSubString(matcher.pattern, ("GB"))) != string::npos) {
		multiplier = 1000 * 1000 * 1000;
	} else if((end = Util::findSubString(matcher.pattern, ("MB"))) != string::npos) {
		multiplier = 1000 * 1000;
	} else if((end = Util::findSubString(matcher.pattern, ("KB"))) != string::npos) {
		multiplier = 1000;
	} else {
		multiplier = 1;
	}

	if(end == tstring::npos) {
		end = matcher.pattern.length();
	}

	return Util::toDouble(matcher.pattern.substr(0, end)) * multiplier;
}
