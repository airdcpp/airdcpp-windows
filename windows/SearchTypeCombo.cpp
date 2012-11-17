/* 
 * Copyright (C) 2003-2004 "Opera", <opera at home dot se>
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

#include "WinUtil.h"
#include "SearchTypeCombo.h"
#include "ResourceLoader.h"
#include "../client/SearchManager.h"


namespace dcpp {

SearchTypeCombo::SearchTypeCombo() { }
SearchTypeCombo::~SearchTypeCombo() { }

LRESULT SearchTypeCombo::onMeasureItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	bHandled = FALSE;
	
	auto mis = (MEASUREITEMSTRUCT *)lParam;
	if(mis->CtlType == ODT_COMBOBOX) {
		bHandled = TRUE;
		mis->itemHeight = 16;
		return TRUE;
	}
	
	return FALSE;
}

LRESULT SearchTypeCombo::onDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
	bHandled = FALSE;
	
	if(dis->CtlType == ODT_COMBOBOX) {
		bHandled = TRUE;
		//TCHAR szText[MAX_PATH+1];
	
		switch(dis->itemAction) {
			case ODA_FOCUS:
				if(!(dis->itemState & 0x0200))
					DrawFocusRect(dis->hDC, &dis->rcItem);
				break;

			case ODA_SELECT:
			case ODA_DRAWENTIRE:
				auto ii = (ItemInfo*)dis->itemData;
				if(dis->itemState & ODS_SELECTED) {
					SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
					SetBkColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHT));
				} else {			
					SetTextColor(dis->hDC, ii->textColor);
					SetBkColor(dis->hDC, ii->bgColor);
				}

				ExtTextOut(dis->hDC, dis->rcItem.left+22, dis->rcItem.top+1, ETO_OPAQUE, &dis->rcItem, ii->title.c_str(), ii->title.length(), 0);
				if(dis->itemState & ODS_FOCUS) {
					if(!(dis->itemState &  0x0200 ))
						DrawFocusRect(dis->hDC, &dis->rcItem);
				}

				ImageList_Draw(ResourceLoader::searchImages, ii->imageIndex, dis->hDC, 
					dis->rcItem.left + 2, 
					dis->rcItem.top, 
					ILD_TRANSPARENT);

				break;
		}
		return TRUE;
	}
	
	return FALSE;
}

void SearchTypeCombo::fillList(const string& aSelection, COLORREF aTextColor, COLORREF aBgColor) {
	auto types = SearchManager::getInstance()->getSearchTypes();

	int selection=0;
	auto addListItem = [&] (int imagePos, const tstring& title, const string& nameStr) -> void {
		if (nameStr == aSelection)
			selection = imageIndexes.size()+1;

		auto ii = new ItemInfo(title, imagePos, aTextColor, aBgColor);
		AddString(const_cast<TCHAR*>(title.c_str()));
		SetItemData(imageIndexes.size(), (DWORD_PTR)ii);

		imageIndexes.push_back(unique_ptr<ItemInfo>(ii));
	};

	addListItem(0, TSTRING(ANY), SEARCH_TYPE_ANY);
	addListItem(7, TSTRING(DIRECTORY), SEARCH_TYPE_DIRECTORY);
	addListItem(8, _T("TTH"), SEARCH_TYPE_TTH);

	for(auto i = types.begin(); i != types.end(); i++) {
		string name = i->first;
		int imagePos = 0;
		if(name.size() == 1 && name[0] >= '1' && name[0] <= '6') {
			imagePos = name[0] - '0';
			name = SearchManager::getTypeStr(name[0] - '0');
		} else {
			imagePos = 9;
		}

		addListItem(imagePos, Text::toT(name), i->first);
	}

	SetCurSel(selection);
}

}