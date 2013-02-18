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

#ifndef LISTFILTER_H
#define LISTFILTER_H

#include "stdafx.h"
#include "TypedListViewCtrl.h"
#include "../client/StringMatch.h"

/*
a helper class for adding and handling list filters, any of the objects can be added to any parent 
and moved to proper placement as individuals.
The messages in this class can be chained by member to the parents message map with CHAIN_MSG_MAP_MEMBER(<FilterObjectName>)
TODO: numerical columns matching
*/

class ListFilter : boost::noncopyable {
	typedef std::function<void ()> UpdateFunction;
	typedef std::function<string (size_t)> InfoFunction;

	struct Preparation {
		size_t column;
		size_t method;
		double size;
	};


public:
	ListFilter(size_t colCount, UpdateFunction updateF);
	virtual ~ListFilter() {}

	BEGIN_MSG_MAP(ListFilter)
		MESSAGE_HANDLER(WM_CHAR, onFilterChar)
		MESSAGE_HANDLER(WM_CLEAR, onFilterChar)
		MESSAGE_HANDLER(WM_KEYUP, onFilterChar)
		MESSAGE_HANDLER(WM_CUT, onFilterChar)
		MESSAGE_HANDLER(WM_PASTE, onFilterChar)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, onSelChange)
	END_MSG_MAP()

	LRESULT onFilterChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);

	
	LRESULT onSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
		columnChanged();
		bHandled = FALSE;
		return 0;
	}

	void addFilterBox(HWND parent);
	void addColumnBox(HWND parent, vector<ColumnInfo*>& columns);
	void addMethodBox(HWND parent);

	CComboBox& getFilterColumnBox() { return column; } 
	CComboBox& getFilterMethodBox() { return method; } 
	CEdit& getFilterBox() { return text; } 

	Preparation prepare();
	bool match(const Preparation& prep, InfoFunction infoF) const;

	bool empty() const;

	CEdit text;
	CComboBox column;
	CComboBox method;

	void SetDefaultMatchColumn(int i) { defMatchColumn = i; } //for setting the match column without column box

private:

	enum {
		EQUAL,
		GREATER_EQUAL,
		LESS_EQUAL,
		GREATER,
		LESS,
		NOT_EQUAL
	};

	vector<ColumnInfo*> columns;

	void textUpdated(const string& filter);
	void columnChanged();
	double prepareSize() const;

	StringMatch::Method defMethod;
	int defMatchColumn;


	size_t getMethod();
	size_t getColumn();
	
	const size_t colCount;
	const UpdateFunction updateFunction;

	StringMatch matcher;
};
#endif