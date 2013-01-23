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

#if !defined(VIRTUAL_TREE_CTRL_H)
#define VIRTUAL_TREE_CTRL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"

enum ChildrenState {
	NO_CHILDREN,
	CHILDREN_CREATED,
	CHILDREN_PENDING
};

template<class PT, class T>
class TypedTreeCtrl : public CWindowImpl<TypedTreeCtrl<PT, T>, CTreeViewCtrl>
{

public:
	TypedTreeCtrl(PT* aParent) : parent(aParent)  { }
	~TypedTreeCtrl() {  }

	BEGIN_MSG_MAP(thisClass)
		REFLECTED_NOTIFY_CODE_HANDLER(TVN_GETINFOTIP, OnGetChildInfo)
		REFLECTED_NOTIFY_CODE_HANDLER(TVN_GETDISPINFO, OnGetItemDispInfo)
		REFLECTED_NOTIFY_CODE_HANDLER(TVN_ITEMEXPANDING, OnItemExpanding)
	END_MSG_MAP();

	LRESULT OnGetChildInfo(int /*idCtrl*/, NMHDR* pNMHDR, BOOL &bHandled) {
		NMTVDISPINFO *pDispInfo = reinterpret_cast<NMTVDISPINFO*>(pNMHDR);
		TVITEM* pItem = &(pDispInfo)->item;
    
		if ( pItem->mask & TVIF_CHILDREN ) {
			pItem->cChildren = parent->getChildrenState((T*)pItem->lParam) > 0 ? 1 : 0 ;
		}

		bHandled = FALSE; //Allow the message to be reflected again
		return 0;
	}

	LRESULT OnGetItemDispInfo(int /*idCtrl*/, NMHDR *pNMHDR, BOOL &bHandled) {
		NMTVDISPINFO *pDispInfo = reinterpret_cast<NMTVDISPINFO*>(pNMHDR);
		TVITEM* pItem = &(pDispInfo)->item;
    
		if (pItem->mask & TVIF_TEXT) {
			lstrcpyn(pItem->pszText, Text::toT(((T*)pItem->lParam)->getName()).c_str(), pItem->cchTextMax);
		}
    
		if (pItem->mask & TVIF_IMAGE) {
			pItem->iImage = parent->getIconIndex(((T*)pItem->lParam));
		}

		if (pItem->mask & TVIF_SELECTEDIMAGE) {
			pItem->iSelectedImage = parent->getIconIndex(((T*)pItem->lParam));
		}

		if ( pItem->mask & TVIF_CHILDREN) {
    		pItem->cChildren = parent->getChildrenState(((T*)pItem->lParam)) > 0 ? 1 : 0 ;
		}

		bHandled = FALSE; //Allow the message to be reflected again
		return 0;
	}

	LRESULT OnItemExpanding(int /*idCtrl*/, NMHDR *pNMHDR, BOOL &bHandled) {
		NMTREEVIEW *pNMTreeView = (NMTREEVIEW*)pNMHDR;
		if(pNMTreeView->action == TVE_COLLAPSE) {
			//Get the currently selected item
			HTREEITEM sel = GetSelectedItem();
			bool childSelected = false;
			if (sel) {
				childSelected = AirUtil::isSub(((T*)GetItemData(sel))->getPath(), ((T*)pNMTreeView->itemNew.lParam)->getPath());
			}

			Expand(pNMTreeView->itemNew.hItem, TVE_COLLAPSE);

			if (childSelected) {
				//it would be selected anyway but without any notification message
				parent->expandDir(((T*)pNMTreeView->itemNew.lParam), true);
			}
		} else if (pNMTreeView->action == TVE_EXPAND && !(pNMTreeView->itemNew.state & TVIS_EXPANDEDONCE)) {
			T* curDir = (T*)pNMTreeView->itemNew.lParam;

			ChildrenState state = parent->getChildrenState(curDir);
			/* now create the children */
			if (state == CHILDREN_CREATED) {
				insertItems(curDir, pNMTreeView->itemNew.hItem);
			} else if (state == CHILDREN_PENDING) {
				parent->expandDir(curDir, false);
			}
		}

		bHandled = FALSE; //Allow the message to be reflected again
		return 0;
	}

	void setHasChildren(HTREEITEM hItem, bool bHavePlus) {
		//Remove all the child items from the parent
		TV_ITEM tvItem;
		tvItem.hItem = hItem;
		tvItem.mask = TVIF_HANDLE | TVIF_CHILDREN;
		tvItem.cChildren = bHavePlus;
		SetItem(&tvItem);
	}

	bool hasChildren(HTREEITEM hItem) {
		TVITEM tvItem;
		tvItem.hItem = hItem;
		tvItem.mask = TVIF_HANDLE | TVIF_CHILDREN;
		return GetItem(&tvItem) && (tvItem.cChildren != 0);
	}

	void insertItems(const T* aDir, HTREEITEM aParent) {
		for (auto& d: aDir->directories | reversed) {
			TVINSERTSTRUCT tvs = {0};
 
			tvs.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_CHILDREN|TVIF_PARAM;
 
			tvs.item.pszText = LPSTR_TEXTCALLBACK ;
			tvs.item.iImage = I_IMAGECALLBACK ;
			tvs.item.iSelectedImage = I_IMAGECALLBACK ;
			tvs.item.cChildren = I_CHILDRENCALLBACK;
			tvs.item.lParam = (LPARAM)d;
	    
			tvs.hParent = aParent;
			tvs.hInsertAfter = TVI_FIRST;
    			InsertItem( &tvs ) ;
		}
	}

	HTREEITEM findItem(HTREEITEM ht, const tstring& name) {
		string::size_type i = name.find('\\');
		if(i == string::npos)
			return ht;
	
		for(HTREEITEM child = GetChildItem(ht); child != NULL; child = GetNextSiblingItem(child)) {
			T* d = (T*)GetItemData(child);
			if(Text::toT(d->getName()) == name.substr(0, i)) {
				return findItem(child, name.substr(i+1));
			}
		}

		//have we created it yet?
		if (hasChildren(ht) && !IsExpanded(ht)) {
			Expand(ht, TVE_EXPAND);
			//insertItems((T*)GetItemData(ht), ht);
			return findItem(ht, name);
		}

		return NULL;
	}

	bool IsExpanded(HTREEITEM hItem) {
		TVITEM tvItem;
		tvItem.hItem = hItem;
		tvItem.mask = TVIF_HANDLE | TVIF_STATE;
		return (GetItem(&tvItem) && (tvItem.state & TVIS_EXPANDED));
	}

private:
	PT* parent;
};

#endif // !defined(VIRTUAL_TREE_CTRL_H)