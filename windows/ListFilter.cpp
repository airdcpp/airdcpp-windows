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
	inverse(false),
	usingTypedMethod(false),
	mode(LAST)
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

	method.AddString(_T("="));
	method.AddString(_T(">="));
	method.AddString(_T("<="));
	method.AddString(_T(">"));
	method.AddString(_T("<"));
	method.AddString(_T("!="));
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

string ListFilter::getText() const {
	string ret;
	TCHAR *buf = new TCHAR[text.GetWindowTextLength() + 1];
	text.GetWindowText(buf, text.GetWindowTextLength() + 1);
	ret = Text::fromT(buf);
	delete [] buf;
	return ret;
}

LRESULT ListFilter::onFilterChar(WORD /*wNotifyCode*/, WORD, HWND hWndCtl, BOOL & bHandled) {
	if (hWndCtl == text.m_hWnd) {
		textUpdated();
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


ListFilter::Preparation ListFilter::prepare(InfoFunction aInfoF, NumericFunction aNumericF) {
	Preparation prep = Preparation(matcher);
	if(empty())
		return prep;

	prep.column = getColumn();
	prep.method = getMethod();
	prep.type = COLUMN_TEXT;

	if (prep.method < StringMatch::METHOD_LAST || prep.column >= colCount) {
		if (mode != LAST) {
			prep.type = COLUMN_TIME;
			auto ret = prepareTime();

			if (!ret.second) {
				prep.type = COLUMN_SIZE;
				ret = prepareSize();
			}

			if (!ret.second) {
				prep.type = COLUMN_SPEED;
				ret = prepareSpeed();
			}

			if (!ret.second) {
				prep.type = COLUMN_NUMERIC;
				prep.num = Util::toDouble(matcher.pattern);
			} else {
				prep.num = ret.first;
			}
		}

		matcher.setMethod(static_cast<StringMatch::Method>(prep.method));
		matcher.prepare();
	} else if (columns[prep.column]->colType == COLUMN_SIZE) {
		prep.type = COLUMN_SIZE;
		prep.num = prepareSize().first;
	} else if (columns[prep.column]->colType == COLUMN_TIME) {
		prep.type = COLUMN_TIME;
		prep.num = prepareTime().first;
	} else if (columns[prep.column]->colType == COLUMN_SPEED) {
		prep.type = COLUMN_SPEED;
		prep.num = prepareSpeed().first;
	} else if (columns[prep.column]->colType == COLUMN_NUMERIC) {
		prep.type = COLUMN_NUMERIC;
		prep.num = Util::toDouble(matcher.pattern);
	}

	prep.infoF = aInfoF;
	prep.numericF = aNumericF;
	return prep;
}

bool ListFilter::match(const Preparation& prep) const {
	if(empty())
		return true;

	bool hasMatch = false;
	if(prep.column >= colCount) {
		if (prep.method < StringMatch::METHOD_LAST) {
			for (size_t i = 0; i < colCount; ++i) {
				if (prep.matchText(i)) {
					hasMatch = true;
					break;
				}
			}
		} else {
			for (size_t i = 0; i < colCount; ++i) {
				if (prep.type == columns[i]->colType && prep.matchNumeric(i)) {
					hasMatch = true;
					break;
				}
			}
		}
	} else if (prep.method < StringMatch::METHOD_LAST || columns[prep.column]->colType == COLUMN_TEXT) {
		hasMatch = prep.matchText(prep.column);
	} else {
		hasMatch = prep.matchNumeric(prep.column);
	}
	return inverse ? !hasMatch : hasMatch;
}

bool ListFilter::Preparation::matchNumeric(int column) const {
	auto toCompare = numericF(column);
	switch (method - StringMatch::METHOD_LAST) {
		case EQUAL: return toCompare == num;
		case NOT_EQUAL: return toCompare != num;

		//inverse the match for time types (smaller number actually means bigger age)
		case GREATER_EQUAL: return type == COLUMN_TIME ? toCompare <= num : toCompare >= num;
		case LESS_EQUAL: return type == COLUMN_TIME ? toCompare >= num : toCompare <= num;
		case GREATER: return type == COLUMN_TIME ? toCompare < num : toCompare > num; break;
		case LESS: return type == COLUMN_TIME ? toCompare > num : toCompare < num; break;
	}

	dcassert(0);
	return false;
}

bool ListFilter::Preparation::matchText(int column) const {
	return matcher.match(infoF(column));
}

bool ListFilter::empty() const {
	return matcher.pattern.empty();
}

void ListFilter::textUpdated() {
	auto filter = getText();

	mode = LAST;
	auto start = string::npos;
	if (!filter.empty()) {
		if (filter.compare(0, 2, ">=") == 0) {
			mode = GREATER_EQUAL;
			start = 2;
		} else if (filter.compare(0, 2, "<=") == 0) {
			mode = LESS_EQUAL;
			start = 2;
		} else if (filter.compare(0, 2, "==") == 0) {
			mode = EQUAL;
			start = 2;
		} else if (filter.compare(0, 2, "!=") == 0) {
			mode = NOT_EQUAL;
			start = 2;
		} else if (filter[0] == _T('<')) {
			mode = LESS;
			start = 1;
		} else if (filter[0] == _T('>')) {
			mode = GREATER;
			start = 1;
		} else if (filter[0] == _T('=')) {
			mode = EQUAL;
			start = 1;
		}
	}

	if (start != string::npos) {
		method.SetCurSel(StringMatch::METHOD_LAST + mode);
		filter = filter.substr(start, filter.length()-start);
		usingTypedMethod = true;
	} else if (usingTypedMethod) {
		method.SetCurSel(0);
	}

	matcher.pattern = filter;
	updateFunction();
}

void ListFilter::columnChanged() {
	if(column.IsWindow() && method.IsWindow()) {
		auto n = method.GetCount();
		size_t col = getColumn();

		for (size_t i = StringMatch::METHOD_LAST; i < n; ++i) {
			method.DeleteString(StringMatch::METHOD_LAST);
		}

		if (getMethod() == -1) {
			method.SetCurSel(StringMatch::PARTIAL);
		}

		if (col >= colCount || columns[col]->isNumericType()) {
			method.AddString(_T("="));
			method.AddString(_T(">="));
			method.AddString(_T("<="));
			method.AddString(_T(">"));
			method.AddString(_T("<"));
			method.AddString(_T("!="));
		}

		textUpdated();
		usingTypedMethod = false;
	}
}

LRESULT ListFilter::onSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled) {
	if (hWndCtl == column.m_hWnd || hWndCtl == method.m_hWnd) {
		if (hWndCtl == column.m_hWnd)
			columnChanged();
		if (hWndCtl == method.m_hWnd)
			usingTypedMethod = false;

		updateFunction();
	}
	bHandled = FALSE;
	return 0;
}

pair<double, bool> ListFilter::prepareTime() const {
	size_t end;
	time_t multiplier;
	auto hasType = [&end, this](string&& id) {
		end = Util::findSubString(matcher.pattern, id, matcher.pattern.size() - id.size());
		return end != string::npos;
	};

	bool hasMatch = true;
	if (hasType("y")) {
		multiplier = 60 * 60 * 24 * 365;
	} else if (hasType("m")) {
		multiplier = 60 * 60 * 24 * 30;
	} else if (hasType("w")) {
		multiplier = 60 * 60 * 24 * 7;
	} else if (hasType("d")) {
		multiplier = 60 * 60 * 24;
	} else if (hasType("h")) {
		multiplier = 60 * 60;
	} else if (hasType("min")) {
		multiplier = 60;
	} else if (hasType("s")) {
		multiplier = 1;
	} else {
		hasMatch = false;
		multiplier = 1;
	}

	if (end == tstring::npos) {
		end = matcher.pattern.length();
	}

	time_t ret = Util::toInt64(matcher.pattern.substr(0, end)) * multiplier;
	return make_pair(ret > 0 ? GET_TIME() - ret : ret, hasMatch);
}

pair<double, bool> ListFilter::prepareSize() const {
	size_t end;
	int64_t multiplier;
	auto hasType = [&end, this](string && id) {
		end = Util::findSubString(matcher.pattern, id, matcher.pattern.size() - id.size());
		return end != string::npos;
	};

	if(hasType("TiB")) {
		multiplier = 1024LL * 1024LL * 1024LL * 1024LL;
	} else if(hasType("GiB")) {
		multiplier = 1024 * 1024 * 1024;
	} else if(hasType("MiB")) {
		multiplier = 1024 * 1024;
	} else if(hasType("KiB")) {
		multiplier = 1024;
	} else if(hasType("TB")) {
		multiplier = 1000LL * 1000LL * 1000LL * 1000LL;
	} else if(hasType("GB")) {
		multiplier = 1000 * 1000 * 1000;
	} else if(hasType("MB")) {
		multiplier = 1000 * 1000;
	} else if(hasType("KB")) {
		multiplier = 1000;
	} else {
		multiplier = 1;
	}

	if(end == tstring::npos) {
		end = matcher.pattern.length();
	}

	return make_pair(Util::toDouble(matcher.pattern.substr(0, end)) * multiplier, multiplier > 1);
}

pair<double, bool> ListFilter::prepareSpeed() const {
	size_t end;
	int64_t multiplier;
	auto hasType = [&end, this](string && id) {
		end = Util::findSubString(matcher.pattern, id, matcher.pattern.size() - id.size());
		return end != string::npos;
	};

	if (hasType("tbit")) {
		multiplier = 1000LL * 1000LL * 1000LL * 1000LL / 8LL;
	} else if (hasType("gbit")) {
		multiplier = 1000 * 1000 * 1000 / 8;
	} else if (hasType("mbit")) {
		multiplier = 1000 * 1000 / 8;
	} else if (hasType("kbit")) {
		multiplier = 1000 / 8;
	} else if (hasType("tibit")) {
		multiplier = 1024LL * 1024LL * 1024LL * 1024LL / 8LL;
	} else if (hasType("gibit")) {
		multiplier = 1024 * 1024 * 1024 / 8;
	} else if (hasType("mibit")) {
		multiplier = 1024 * 1024 / 8;
	} else if (hasType("kibit")) {
		multiplier = 1024 / 8;
	} else {
		multiplier = 1;
	}

	if (end == tstring::npos) {
		end = matcher.pattern.length();
	}

	return make_pair(Util::toDouble(matcher.pattern.substr(0, end)) * multiplier, multiplier > 1);
}
