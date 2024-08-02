/* 
 * Copyright (C) 2011-2024 AirDC++ Project
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

#include "stdafx.h"

#include "SearchTypeCombo.h"
#include "ResourceLoader.h"

#include <airdcpp/SearchManager.h>
#include <airdcpp/SearchTypes.h>


namespace dcpp {

SearchTypeCombo::SearchTypeCombo() { }
SearchTypeCombo::~SearchTypeCombo() { }

LRESULT SearchTypeCombo::onMeasureItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	bHandled = FALSE;
	
	auto mis = (MEASUREITEMSTRUCT *)lParam;
	if(mis->CtlType == ODT_COMBOBOX) {
		bHandled = TRUE;
		mis->itemHeight = 16;
		return TRUE;
	}
	
	return FALSE;
}

LRESULT SearchTypeCombo::onDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
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

				ImageList_Draw(ResourceLoader::getSearchTypeIcons(), ii->imageIndex, dis->hDC, 
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
	auto& typeManager = SearchManager::getInstance()->getSearchTypes();
	auto types = typeManager.getSearchTypes();

	int selectionIndex = 0;
	auto addListItem = [&] (int aImagePos, const tstring& aTitle, const string& aId) -> void {
		if (aId == aSelection)
			selectionIndex = imageIndexes.size();

		auto ii = new ItemInfo(aTitle, aImagePos, aTextColor, aBgColor);
		AddString(const_cast<TCHAR*>(aTitle.c_str()));
		SetItemData(imageIndexes.size(), (DWORD_PTR)ii);

		imageIndexes.push_back(unique_ptr<ItemInfo>(ii));
	};

	addListItem(0, TSTRING(ANY), SEARCH_TYPE_ANY);
	addListItem(7, TSTRING(DIRECTORY), SEARCH_TYPE_DIRECTORY);
	addListItem(8, _T("TTH"), SEARCH_TYPE_TTH);
	addListItem(9, TSTRING(FILE), SEARCH_TYPE_FILE);

	for (auto& type: types) {
		auto name = type->getDisplayName();
		int imagePos = type->isDefault() ? type->getId()[0] - '0' : 10;
		addListItem(imagePos, Text::toT(name), type->getId());
	}

	SetCurSel(selectionIndex);
}

}