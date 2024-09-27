/* 
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include <windows/stdafx.h>

#include <windows/Async.h>
#include <windows/ListFilter.h>
#include <windows/WinUtil.h>

#include <airdcpp/core/timer/TimerManager.h>


namespace wingui {
ListFilter::ListFilter(size_t colCount, UpdateFunction updateF) :
	colCount(colCount),
	updateFunction(updateF),
	defMethod(StringMatch::PARTIAL),
	defMatchColumn(colCount),
	inverse(false),
	usingTypedMethod(false),
	mode(LAST)
{
	matcher.setVerbosePatternErrors(false);
}
void ListFilter::addFilterBox(HWND parent) {
	RECT rc = { 0, 0, 100, 120 };
	textEdit.Create(parent, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);
	textEdit.SetFont(WinUtil::systemFont);
	WinUtil::addCue(textEdit.m_hWnd, CTSTRING(FILTER_DOTS), TRUE);

}

void ListFilter::addColumnBox(HWND parent, vector<ColumnInfo*>& aColumns, int initialSel /*-1*/, HWND async /*NULL*/) {
	RECT rc = { 0, 0, 100, 110 };
	columnCombo.Create(parent, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL |
		WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	columnCombo.SetFont(WinUtil::systemFont);
	columns = aColumns;
	for(auto& col : columns)
		columnCombo.AddString(((*col).name).c_str());

	columnCombo.AddString(CTSTRING(ANY));
	columnCombo.SetCurSel(initialSel != -1 ? initialSel : colCount);
	callAsync(async ? async : parent, [this] { columnChanged(false); });
}


void ListFilter::addMethodBox(HWND parent){
	RECT rc = { 0, 0, 100, 110 };
	methodCombo.Create(parent, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL |
		WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	methodCombo.SetFont(WinUtil::systemFont);
	tstring methods[StringMatch::METHOD_LAST] = { TSTRING(PARTIAL_MATCH), TSTRING(REGULAR_EXPRESSION), TSTRING(WILDCARD), TSTRING(EXACT_MATCH)  };
	for(auto& str : methods) 
		methodCombo.AddString(str.c_str());

	methodCombo.SetCurSel(defMethod);
}

void ListFilter::clear() {
	if (!matcher.pattern.empty()) {
		textEdit.SetWindowText(_T(""));
		//textUpdated(Util::emptyString);
		matcher.pattern = Util::emptyString;
	}
}

void ListFilter::setInverse(bool aInverse) {
	inverse = aInverse;
	WinUtil::addCue(textEdit.m_hWnd, inverse ? CTSTRING(EXCLUDE_DOTS) : CTSTRING(FILTER_DOTS), FALSE);
}

string ListFilter::getText() const {
	string ret;
	TCHAR *buf = new TCHAR[textEdit.GetWindowTextLength() + 1];
	textEdit.GetWindowText(buf, textEdit.GetWindowTextLength() + 1);
	ret = Text::fromT(buf);
	delete [] buf;
	return ret;
}

LRESULT ListFilter::onFilterChar(WORD /*wNotifyCode*/, WORD, HWND hWndCtl, BOOL & bHandled) {
	if (hWndCtl == textEdit.m_hWnd) {
		textUpdated(false);
	}

	bHandled = FALSE;
	return 0;
}


size_t ListFilter::getMethod() const {
	if(!methodCombo.IsWindow()) 
		return defMethod;

	return methodCombo.GetCurSel();
}

size_t ListFilter::getColumn() const {
	if(!columnCombo.IsWindow()) 
		return defMatchColumn;

	return columnCombo.GetCurSel();
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
				prep.type = COLUMN_NUMERIC_OTHER;
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
	} else if (columns[prep.column]->colType == COLUMN_NUMERIC_OTHER) {
		prep.type = COLUMN_NUMERIC_OTHER;
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

bool ListFilter::Preparation::matchNumeric(int aColumn) const {
	auto toCompare = numericF(aColumn);
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

bool ListFilter::Preparation::matchText(int aColumn) const {
	return matcher.match(infoF(aColumn));
}

bool ListFilter::empty() const {
	return matcher.pattern.empty();
}

void ListFilter::textUpdated(bool alwaysUpdate) {
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
		methodCombo.SetCurSel(static_cast<int>(StringMatch::METHOD_LAST) + static_cast<int>(mode));
		filter = filter.substr(start, filter.length()-start);
		usingTypedMethod = true;
	} else if (usingTypedMethod) {
		methodCombo.SetCurSel(0);
	}

	if (alwaysUpdate || filter != matcher.pattern) {
		matcher.pattern = filter;
		updateFunction();
	}
}

void ListFilter::columnChanged(bool doFilter) {
	if(columnCombo.IsWindow() && methodCombo.IsWindow()) {
		auto n = methodCombo.GetCount();
		size_t col = getColumn();

		for (int i = StringMatch::METHOD_LAST; i < n; ++i) {
			methodCombo.DeleteString(StringMatch::METHOD_LAST);
		}

		if (getMethod() == -1) {
			methodCombo.SetCurSel(StringMatch::PARTIAL);
		}

		if (col >= colCount || columns[col]->isNumericType()) {
			methodCombo.AddString(_T("="));
			methodCombo.AddString(_T(">="));
			methodCombo.AddString(_T("<="));
			methodCombo.AddString(_T(">"));
			methodCombo.AddString(_T("<"));
			methodCombo.AddString(_T("!="));
		}

		if (doFilter)
			textUpdated(true);
		usingTypedMethod = false;
	}
}

LRESULT ListFilter::onSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled) {
	if (hWndCtl == columnCombo.m_hWnd || hWndCtl == methodCombo.m_hWnd) {
		if (hWndCtl == columnCombo.m_hWnd)
			columnChanged(true);
		if (hWndCtl == methodCombo.m_hWnd)
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

	auto ret = Util::toTimeT(matcher.pattern.substr(0, end)) * multiplier;
	return make_pair(static_cast<double>(ret > 0 ? GET_TIME() - ret : ret), hasMatch);
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
}
