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
#include "Resource.h"

#include "ExListViewCtrl.h"

// TODO: make sure that moved items maintain their selection state
int ExListViewCtrl::moveItem(int oldPos, int newPos) {
	TCHAR buf[512];
	LVITEM lvi;
	lvi.iItem = oldPos;
	lvi.iSubItem = 0;
	lvi.mask = LVIF_IMAGE | LVIF_INDENT | LVIF_PARAM | LVIF_STATE;
	GetItem(&lvi);
	TStringList l;

	for(int j = 0; j < GetHeader().GetItemCount(); j++) {
		GetItemText(oldPos, j, buf, 512);
		l.push_back(buf);
	}

	SetRedraw(FALSE);
	
	if(oldPos < newPos) {
		lvi.iItem = newPos + 1;
	} else {
	lvi.iItem = newPos;
	}
	int i = InsertItem(&lvi);
	int m = 0;
	for(TStringIter k = l.begin(); k != l.end(); ++k, m++) {
		SetItemText(i, m, k->c_str());
	}
	EnsureVisible(i, FALSE);
	
	if(i < oldPos)
		DeleteItem(oldPos + 1);
	else
		DeleteItem(oldPos);

	SetRedraw(TRUE);

	RECT rc;
	GetItemRect(i, &rc, LVIR_BOUNDS);
	InvalidateRect(&rc);
	
	return i;
}

int ExListViewCtrl::insert(TStringList& aList, int iImage, LPARAM lParam) {

	TCHAR buf[128];
	int loc = 0;
	int count = GetItemCount();

	LVITEM a;
	a.mask = LVIF_IMAGE | LVIF_INDENT | LVIF_PARAM | LVIF_STATE | LVIF_TEXT;
	a.iItem = -1;
	a.iSubItem = sortColumn;
	a.iImage = iImage;
	a.state = 0;
	a.stateMask = 0;
	a.lParam = lParam;
	a.iIndent = 0;
	a.pszText = const_cast<TCHAR*>(sortColumn == -1 ? aList[0].c_str() : aList[sortColumn].c_str());
	a.cchTextMax = sortColumn == -1 ? aList[0].size() : aList[sortColumn].size();
	
	if(sortColumn == -1) {
		loc = count;
	} else if(count == 0) {
		loc = 0;
	} else {

		tstring& b = aList[sortColumn];
		int c = _tstoi(b.c_str());
		double f = _tstof(b.c_str());
		LPARAM data = NULL;			
		int low = 0;
		int high = count-1;
		int comp = 0;
		while(low <= high)
		{
			loc = (low + high)/2;
			
			// This is a trick, so that if fun() returns something bigger than one, use the
			// internal default sort functions
			comp = sortType;
			if(comp == SORT_FUNC) {
				data = GetItemData(loc);
				comp = fun(lParam, data, sortColumn);
			}
			
			if(comp == SORT_STRING) {
				GetItemText(loc, sortColumn, buf, 128);
				comp = compare(b, tstring(buf));
			} else if(comp == SORT_STRING_NOCASE) {
				GetItemText(loc, sortColumn, buf, 128);
				comp =  stricmp(b.c_str(), buf);
			} else if(comp == SORT_INT) {
				GetItemText(loc, sortColumn, buf, 128);
				comp = compare(c, _tstoi(buf)); 
			} else if(comp == SORT_FLOAT) {
				GetItemText(loc, sortColumn, buf, 128);
				comp = compare(f, _tstof(buf));
			}
			
			if(!ascending)
				comp = -comp;

			if(comp < 0) {
				high = loc - 1;
			} else if(comp > 0) {
				low = loc + 1;
			} else {
				break;
			} 
		}

		comp = sortType;
		if(comp == SORT_FUNC) {
			comp = fun(lParam, data, sortColumn);
		}
		if(comp == SORT_STRING) {
			comp = compare(b, tstring(buf));
		} else if(comp == SORT_STRING_NOCASE) {
			comp =  stricmp(b.c_str(), buf);
		} else if(comp == SORT_INT) {
			comp = compare(c, _tstoi(buf)); 
		} else if(comp == SORT_FLOAT) {
			comp = compare(f, _tstof(buf));
		}

		if(!ascending)
			comp = -comp;
		
		if(comp > 0)
			loc++;
	}
	dcassert(loc >= 0 && loc <= GetItemCount());
	a.iItem = loc;
	a.iSubItem = 0;
	int i = InsertItem(&a);
	int k = 0;
	for(TStringIter j = aList.begin(); j != aList.end(); ++j, k++) {
		SetItemText(i, k, j->c_str());
	}
	return loc;
}

int ExListViewCtrl::insert(int nItem, TStringList& aList, int iImage, LPARAM lParam) {

	dcassert(aList.size() > 0);

	int i = insert(nItem, aList[0], iImage, lParam);

	int k = 0;
	for(TStringIter j = aList.begin(); j != aList.end(); ++j, k++) {
		SetItemText(i, k, j->c_str());
	}
	return i;
	
}