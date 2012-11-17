/* 
 * Copyright (C) 2012 AirDC++ Project 
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

#ifndef SEARCHTYPECOMBO_H
#define SEARCHTYPECOMBO_H

#pragma once

#include "../client/typedefs.h"
#include <atlctrls.h>

namespace dcpp {

class SearchTypeCombo : public CComboBox {
public:
	SearchTypeCombo();
	~SearchTypeCombo();

	static LRESULT onMeasureItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	static LRESULT onDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	void fillList(const string& aSelection, COLORREF aTextColor = GetSysColor(COLOR_WINDOWTEXT), COLORREF aBgColor = GetSysColor(COLOR_WINDOW));
private:
	struct ItemInfo {
		ItemInfo(const tstring& aTitle, int aImageIndex, COLORREF aTextColor, COLORREF aBgColor) : title(aTitle), imageIndex(aImageIndex), bgColor(aBgColor), 
			textColor(aTextColor) { }

		COLORREF bgColor;
		COLORREF textColor;
		int imageIndex;
		tstring title;
	};

	vector<unique_ptr<ItemInfo>> imageIndexes;
};

}

#endif
