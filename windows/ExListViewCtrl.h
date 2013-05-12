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

#if !defined(EX_LIST_VIEW_CTRL_H)
#define EX_LIST_VIEW_CTRL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ListViewArrows.h"
#include "WinUtil.h"

class ExListViewCtrl : public CWindowImpl<ExListViewCtrl, CListViewCtrl, CControlWinTraits>,
	public ListViewArrows<ExListViewCtrl>
{
	int sortColumn;
	int sortType;
	bool ascending;
	int (*fun)(LPARAM, LPARAM, int);

public:
	enum {	
		SORT_FUNC = 2,
 		SORT_STRING,
		SORT_STRING_NOCASE,
		SORT_INT,
		SORT_FLOAT,
		SORT_BYTES
	};

	typedef ListViewArrows<ExListViewCtrl> arrowBase;

	BEGIN_MSG_MAP(ExListViewCtrl)
		MESSAGE_HANDLER(WM_CHAR, onChar)
		CHAIN_MSG_MAP(arrowBase)
	END_MSG_MAP()

	void setSort(int aColumn, int aType, bool aAscending = true, int (*aFun)(LPARAM, LPARAM, int) = NULL) {
		bool doUpdateArrow = (aColumn != sortColumn || aAscending != ascending);
		
		sortColumn = aColumn;
		sortType = aType;
		ascending = aAscending;
		fun = aFun;
		resort();
		if (doUpdateArrow)
			updateArrow();
	}

	void resort() {
		if(sortColumn != -1) {
			SortItemsEx(&CompareFunc, (LPARAM)this);
		}
	}

	bool isAscending() const { return ascending; }
	int getSortColumn() const { return sortColumn; }
	int getSortType() const { return sortType; }

	int insert(int nItem, TStringList& aList, int iImage = 0, LPARAM lParam = NULL);
	int insert(TStringList& aList, int iImage = 0, LPARAM lParam = NULL);
	int insert(int nItem, const tstring& aString, int iImage = 0, LPARAM lParam = NULL) {
		return InsertItem(LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE, nItem, aString.c_str(), 0, 0, iImage, lParam);
	}

	int getItemImage(int aItem) const {
		LVITEM lvi;
		lvi.iItem = aItem;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_IMAGE;
		GetItem(&lvi);
		return lvi.iImage;
	}
	
	int find(LPARAM lParam, int aStart = -1) const {
		LV_FINDINFO fi;
		fi.flags = LVFI_PARAM;
		fi.lParam = lParam;
		return FindItem(&fi, aStart);
	}

	int find(const tstring& aText, int aStart = -1, bool aPartial = false) const {
		LV_FINDINFO fi;
		fi.flags = aPartial ? LVFI_PARTIAL : LVFI_STRING;
		fi.psz = aText.c_str();
		return FindItem(&fi, aStart);
	}
	void deleteItem(const tstring& aItem, int col = 0) {
		for(int i = 0; i < GetItemCount(); i++) {
			TCHAR buf[256];
			GetItemText(i, col, buf, 256);
			if(aItem == buf) {
				DeleteItem(i);
				break;
			}
		}
	}
	int moveItem(int oldPos, int newPos);
	void setSortDirection(bool aAscending) { setSort(sortColumn, sortType, aAscending, fun); }

	static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
		ExListViewCtrl* p = (ExListViewCtrl*) lParamSort;
		TCHAR buf[128];
		TCHAR buf2[128];

		int na = (int)lParam1;
		int nb = (int)lParam2;
		// This is a trick, so that if fun() returns something bigger than one, use the
		// internal default sort functions
		int result = p->sortType;
		if(result == SORT_FUNC) {
			result = p->fun(p->GetItemData(na), p->GetItemData(nb), p->sortColumn);
		}

		if(result == SORT_STRING) {
			p->GetItemText(na, p->sortColumn, buf, 128);
			p->GetItemText(nb, p->sortColumn, buf2, 128);
			result = Util::DefaultSort(buf, buf2, false);			
		} else if(result == SORT_STRING_NOCASE) {
			p->GetItemText(na, p->sortColumn, buf, 128);
			p->GetItemText(nb, p->sortColumn, buf2, 128);
			result = Util::DefaultSort(buf, buf2, true);			
		} else if(result == SORT_INT) {
			p->GetItemText(na, p->sortColumn, buf, 128);
			p->GetItemText(nb, p->sortColumn, buf2, 128);
			result = compare(_tstoi(buf), _tstoi(buf2));
		} else if(result == SORT_FLOAT) {
			p->GetItemText(na, p->sortColumn, buf, 128);
			p->GetItemText(nb, p->sortColumn, buf2, 128);
			result = compare(_tstof(buf), _tstof(buf2));
		} else if(result == SORT_BYTES) {
			p->GetItemText(na, p->sortColumn, buf, 128);
			p->GetItemText(nb, p->sortColumn, buf2, 128);
			result = compare(WinUtil::toBytes(buf), WinUtil::toBytes(buf2));
		}
		if(!p->ascending)
			result = -result;

		return result;
	}
	
	LRESULT onChar(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		if((GetKeyState(VkKeyScan('A') & 0xFF) & 0xFF00) > 0 && (GetKeyState(VK_CONTROL) & 0xFF00) > 0){
			int count = GetItemCount();
			for(int i = 0; i < count; ++i)
				ListView_SetItemState(m_hWnd, i, LVIS_SELECTED, LVIS_SELECTED);

			return 0;
		}

		bHandled = FALSE;
		return 1;
	}

	template<class T> static int compare(const T& a, const T& b) {
		return (a < b) ? -1 : ( (a == b) ? 0 : 1);
	}

	ExListViewCtrl() : sortType(SORT_STRING), ascending(true), sortColumn(-1) { }

	~ExListViewCtrl() { }
};

#endif // !defined(EX_LIST_VIEW_CTRL_H)