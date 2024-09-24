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

#ifndef LISTFILTER_H
#define LISTFILTER_H

#include <windows/stdafx.h>
#include <windows/TypedListViewCtrl.h>
#include <airdcpp/StringMatch.h>

namespace wingui {
/*
a helper class for adding and handling list filters, any of the objects can be added to any parent 
and moved to proper placement as individuals.
The messages in this class can be chained by member to the parents message map with CHAIN_MSG_MAP_MEMBER(<FilterObjectName>)
*/

class ListFilter : boost::noncopyable {
	typedef std::function<void ()> UpdateFunction;
	typedef std::function<string (size_t)> InfoFunction;
	typedef std::function<double(size_t)> NumericFunction;

	struct Preparation {
		Preparation(const StringMatch& aMatch) : matcher(aMatch) { }

		size_t column;
		size_t method;
		double num;
		ColumnType type;
		NumericFunction numericF;
		InfoFunction infoF;
		const StringMatch& matcher;

		bool matchNumeric(int aColumn) const;
		bool matchText(int aColumn) const;
	};


public:
	ListFilter(size_t colCount, UpdateFunction updateF);
	virtual ~ListFilter() {}

	BEGIN_MSG_MAP(ListFilter)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, onSelChange)
		COMMAND_CODE_HANDLER(EN_CHANGE, onFilterChar)
	END_MSG_MAP()

	LRESULT onFilterChar(WORD /*wNotifyCode*/, WORD, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
	LRESULT onSelChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled);

	void addFilterBox(HWND parent);
	void addColumnBox(HWND parent, vector<ColumnInfo*>& columns, int initialSel = -1, HWND async = NULL);
	void addMethodBox(HWND parent);

	CComboBox& getFilterColumnBox() { return columnCombo; } 
	CComboBox& getFilterMethodBox() { return methodCombo; } 
	CEdit& getFilterBox() { return textEdit; } 

	Preparation prepare(InfoFunction infoF, NumericFunction numericF);
	bool match(const Preparation& prep) const;

	bool empty() const;

	CEdit textEdit;
	CComboBox columnCombo;
	CComboBox methodCombo;

	void SetDefaultMatchColumn(int i) { defMatchColumn = i; } //for setting the match column without column box
	void clear();

	void setInverse(bool aInverse);
	bool getInverse() const { return inverse;  }
private:
	vector<ColumnInfo*> columns;

	void textUpdated(bool alwaysUpdate);
	void columnChanged(bool doFilter);

	pair<double, bool> prepareSize() const;
	pair<double, bool> prepareTime() const;
	pair<double, bool> prepareSpeed() const;

	StringMatch::Method defMethod;
	int defMatchColumn;


	size_t getMethod() const;
	size_t getColumn() const;
	string getText() const;
	
	const size_t colCount;
	const UpdateFunction updateFunction;

	StringMatch matcher;

	// Hide matching items
	bool inverse;

	// Filtering mode was typed into filtering expression
	bool usingTypedMethod;

	enum FilterMode {
		EQUAL,
		GREATER_EQUAL,
		LESS_EQUAL,
		GREATER,
		LESS,
		NOT_EQUAL,
		LAST
	};

	FilterMode mode;
};
}
#endif