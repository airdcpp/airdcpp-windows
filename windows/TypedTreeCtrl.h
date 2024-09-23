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

#if !defined(VIRTUAL_TREE_CTRL_H)
#define VIRTUAL_TREE_CTRL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"

namespace wingui {
template<class T>
class TypedTreeCtrl : public CTreeViewCtrl {
public:
	bool IsExpanded(HTREEITEM hItem) const noexcept {
		TVITEM tvItem;
		tvItem.hItem = hItem;
		tvItem.mask = TVIF_HANDLE | TVIF_STATE;
		return (GetItem(&tvItem) && (tvItem.state & TVIS_EXPANDED));
	}
};

template<class PT, class T>
class TypedVirtualTreeCtrl : public TypedTreeCtrl<T>
{

public:
	enum ChildrenState {
		NO_CHILDREN,
		CHILDREN_CREATED,
		CHILDREN_ALL_PENDING,
		CHILDREN_PART_PENDING,
		CHILDREN_LOADING
	};

	TypedVirtualTreeCtrl(PT* aParent) : parent(aParent) {}
	~TypedVirtualTreeCtrl() {}

	/*BEGIN_MSG_MAP(thisClass)
		NOTIFY(TVN_GETINFOTIP, OnGetChildInfo)
		REFLECTED_NOTIFY_CODE_HANDLER(TVN_GETDISPINFO, OnGetItemDispInfo)
		REFLECTED_NOTIFY_CODE_HANDLER(TVN_ITEMEXPANDING, OnItemExpanding)
	END_MSG_MAP();*/

	LRESULT OnGetChildInfo(int /*idCtrl*/, NMHDR* pNMHDR, BOOL &bHandled) const noexcept {
		auto pDispInfo = reinterpret_cast<NMTVDISPINFO*>(pNMHDR);
		TVITEM* pItem = &pDispInfo->item;
    
		if ( pItem->mask & TVIF_CHILDREN ) {
			pItem->cChildren = getChildrenState(pItem) > 0 ? 1 : 0 ;
		}

		bHandled = FALSE; //Allow the message to be reflected again
		return 0;
	}

	const T* getSelectedItemData() const noexcept {
		auto t = this->GetSelectedItem();
		if (!t) {
			return nullptr;
		}

		return (T*)this->GetItemData(t);
	}

	LRESULT OnGetItemDispInfo(int /*idCtrl*/, NMHDR *pNMHDR, BOOL &bHandled) const noexcept {
		auto pDispInfo = reinterpret_cast<NMTVDISPINFO*>(pNMHDR);
		TVITEM* pItem = &pDispInfo->item;
    
		if (pItem->mask & TVIF_TEXT) {
			//pItem->mask |= TVIF_DI_SETITEM;
			lstrcpyn(pItem->pszText, ((T*)pItem->lParam)->getNameW().c_str(), pItem->cchTextMax);
		}
    
		if (pItem->mask & TVIF_IMAGE) {
			pItem->iImage = parent->getIconIndex(((T*)pItem->lParam));
		}

		if (pItem->mask & TVIF_SELECTEDIMAGE) {
			pItem->iSelectedImage = parent->getIconIndex((T*)pItem->lParam);
		}

		if ( pItem->mask & TVIF_CHILDREN) {
    		pItem->cChildren = getChildrenState(pItem) > 0 ? 1 : 0 ;
		}

		bHandled = FALSE; //Allow the message to be reflected again
		return 0;
	}

	LRESULT OnItemExpanding(int /*idCtrl*/, NMHDR *pNMHDR, BOOL &bHandled) noexcept {
		auto pNMTreeView = (NMTREEVIEW*)pNMHDR;
		if (pNMTreeView->action == TVE_COLLAPSE || pNMTreeView->action == TVE_COLLAPSERESET) {
			//Get the currently selected item
			auto selectedItemData = getSelectedItemData();
			auto childSelected = false;
			if (selectedItemData) {
				childSelected = PathUtil::isSubAdc(selectedItemData->getAdcPath(), ((T*)pNMTreeView->itemNew.lParam)->getAdcPath());
				if (childSelected) {
					//it would be selected anyway but without any notification message
					this->SelectItem(pNMTreeView->itemNew.hItem);
				}
			}

			auto collapse = childSelected && getChildrenState(&pNMTreeView->itemNew) == CHILDREN_PART_PENDING;
			this->Expand(pNMTreeView->itemNew.hItem, collapse ? TVE_COLLAPSE | TVE_COLLAPSERESET : pNMTreeView->action);
		} else if (pNMTreeView->action == TVE_EXPAND && !(pNMTreeView->itemNew.state & TVIS_EXPANDEDONCE)) {
			auto curDir = (T*)pNMTreeView->itemNew.lParam;

			ChildrenState state = parent->getChildrenState(curDir);
			/* now create the children */
			if (state == CHILDREN_CREATED || state == CHILDREN_PART_PENDING) {
				parent->insertTreeItems(curDir->getAdcPath(), pNMTreeView->itemNew.hItem);
			} else if (state == CHILDREN_ALL_PENDING) {
				//items aren't ready, tell the parent to load them
				parent->expandDir(curDir, false);
				bHandled = TRUE;
				return 1;
			}
		}

		bHandled = FALSE; //Allow the message to be reflected again
		return 0;
	}

	void setHasChildren(HTREEITEM hItem, bool bHavePlus) noexcept {
		TV_ITEM tvItem;
		tvItem.hItem = hItem;
		tvItem.mask = TVIF_HANDLE | TVIF_CHILDREN;
		tvItem.cChildren = bHavePlus;
		this->SetItem(&tvItem);
	}

	void updateChildren(HTREEITEM hItem) noexcept {
		TV_ITEM tvItem;
		tvItem.hItem = hItem;
		tvItem.mask = TVIF_HANDLE | TVIF_CHILDREN;
		tvItem.cChildren = I_CHILDRENCALLBACK;
		this->SetItem(&tvItem);
	}


	bool hasChildren(HTREEITEM hItem) const noexcept {
		TVITEM tvItem;
		tvItem.hItem = hItem;
		tvItem.mask = TVIF_HANDLE | TVIF_CHILDREN;
		return this->GetItem(&tvItem) && (tvItem.cChildren != 0);
	}

	void insertItem(const T* aDir, HTREEITEM aParent, bool aBold) noexcept {
		TVINSERTSTRUCT tvs = {0};
 
		tvs.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_CHILDREN|TVIF_PARAM;
		tvs.item.pszText = LPSTR_TEXTCALLBACK ;
		tvs.item.iImage = I_IMAGECALLBACK ;
		tvs.item.iSelectedImage = I_IMAGECALLBACK ;
		tvs.item.cChildren = I_CHILDRENCALLBACK;
		tvs.item.lParam = (LPARAM)aDir;
		if (aBold) {
			tvs.item.mask |= TVIF_STATE;
			tvs.item.state = TVIS_BOLD;
			tvs.item.stateMask = TVIS_BOLD;
		}
	    
		tvs.hParent = aParent;
		tvs.hInsertAfter = TVI_FIRST;
		this->InsertItem(&tvs) ;
	}

	void updateItemImage(const T* item) noexcept { 
		HTREEITEM ht = findItem(this->GetRootItem(), item->getAdcPath());
		if (ht) {
			updateItemImage(ht);
		}
	}

	void updateItemImage(HTREEITEM ht) noexcept {
		this->SetItemImage(ht, I_IMAGECALLBACK, I_IMAGECALLBACK);
		this->RedrawWindow();
	}

	HTREEITEM findItemByPath(HTREEITEM ht, const tstring& aPath) noexcept {
		return findItem(ht, aPath, true);
	}

	HTREEITEM findItemByName(HTREEITEM ht, const tstring& aPath) noexcept {
		return findItem(ht, _T(ADC_SEPARATOR) + aPath + _T(ADC_SEPARATOR), true);
	}

	StringList getTreeItemPaths(HTREEITEM ht) noexcept {
		StringList paths;
		getTreeItemPaths(ht, paths);
		return paths;
	}

	// Reset the children
	void refreshItem(HTREEITEM ht, bool aUpdateCurrentChildren, bool aForceExpand) {
		bool shouldExpand = aForceExpand || this->IsExpanded(ht);

		// An existing directory with children won't be collapsed if the new one doesn't have any (the children callback will be called)
		// Make sure that it gets collapsed (we need to update the children data later anyway if the children were removed)
		if (aUpdateCurrentChildren) {
			this->setHasChildren(ht, true);
		}

		// make sure that all tree subitems are removed and expand again if needed
		this->Expand(ht, TVE_COLLAPSE | TVE_COLLAPSERESET);
		dcassert(!this->IsExpanded(ht));

		if (shouldExpand) {
			this->Expand(ht);
			dcassert(
				this->IsExpanded(ht) || 
				parent->getChildrenState((T*)this->GetItemData(ht)) == CHILDREN_ALL_PENDING ||
				parent->getChildrenState((T*)this->GetItemData(ht)) == NO_CHILDREN
			);
		}

		if (aUpdateCurrentChildren) {
			this->updateChildren(ht);
		}
	}
private:
	ChildrenState getChildrenState(TVITEM* pItem) const noexcept {
		return parent->getChildrenState(((T*)pItem->lParam));
	}

	void getTreeItemPaths(HTREEITEM ht, StringList& paths_) const noexcept {
		for (HTREEITEM child = this->GetChildItem(ht); child != NULL; child = this->GetNextSiblingItem(child)) {
			auto d = (T*)this->GetItemData(child);
			auto path = d->getAdcPath();
			paths_.push_back(path);

			getTreeItemPaths(child, paths_);
		}
	}

	HTREEITEM findItem(HTREEITEM ht, const tstring& aPath, bool aIsFirst) noexcept {
		dcassert(!aPath.empty() && aPath.front() == ADC_SEPARATOR);
		dcassert(aPath.back() == ADC_SEPARATOR);

		auto i = aPath.find(ADC_SEPARATOR, 1);
		if (i == string::npos) {
			return ht;
		}

		for (HTREEITEM child = this->GetChildItem(ht); child != NULL; child = this->GetNextSiblingItem(child)) {
			auto d = (T*)this->GetItemData(child);
			if (compare(d->getNameW(), aPath.substr(1, i - 1)) == 0) {
				return findItem(child, aPath.substr(i), true);
			}
		}

		//have we created it yet?
		if (aIsFirst && hasChildren(ht)) {
			bool expanded = this->IsExpanded(ht);
			if (expanded) {
				// refresh the content
				this->Expand(ht, TVE_COLLAPSE | TVE_COLLAPSERESET);
			}

			this->Expand(ht, TVE_EXPAND);

			auto ret = findItem(ht, aPath, false);

			if (!expanded) {
				//leave it as it was...
				this->Expand(ht, TVE_COLLAPSE);
			}
			return ret;
		}

		return nullptr;
	}

	PT* parent;
};

}

#endif // !defined(VIRTUAL_TREE_CTRL_H)