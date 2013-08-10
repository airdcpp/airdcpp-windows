/*
* Copyright (C) 2011-2013 AirDC++ Project
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

#ifndef FILTEREDLISTVIEW_H_
#define FILTEREDLISTVIEW_H_

#include "stdafx.h"

#include "ListFilter.h"
#include "TypedListViewCtrl.h"

#define FILTER_MESSAGE_MAP 8

template<class ParentT, class T, int ctrlId>
class FilteredListViewCtrl : public CWindowImpl<FilteredListViewCtrl<ParentT, T, ctrlId>>  {

public:
	typedef FilteredListViewCtrl<ParentT, T, ctrlId> thisClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		MESSAGE_HANDLER(WM_TIMER, onTimer)
		FORWARD_NOTIFICATIONS()
	ALT_MSG_MAP(FILTER_MESSAGE_MAP)
		CHAIN_MSG_MAP_MEMBER(filter)
		//MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, onCtlColor)
		//MESSAGE_HANDLER(WM_CHAR, onFilterChar)
		//MESSAGE_HANDLER(WM_KEYUP, onFilterChar)
	END_MSG_MAP();

	FilteredListViewCtrl(ParentT* aParent, size_t colCount, std::function<void ()> aUpdateF) : updateF(aUpdateF), parent(aParent), filter(colCount, [this] { onUpdate(); }), columnCount(colCount), /*, listContainer(WC_LISTVIEW, this, CONTROL_MESSAGE_MAP),*/
		ctrlFilterContainer(WC_EDIT, this, FILTER_MESSAGE_MAP),
		ctrlFilterSelContainer(WC_COMBOBOX, this, FILTER_MESSAGE_MAP),
		ctrlFilterMethodContainer(WC_COMBOBOX, this, FILTER_MESSAGE_MAP)
	{}

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		list.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, ctrlId);
		//listContainer.SubclassWindow(list);
		list.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

		parent->createColumns();

		list.SetBkColor(WinUtil::bgColor);
		list.SetTextBkColor(WinUtil::bgColor);
		list.SetTextColor(WinUtil::textColor);

		filter.addFilterBox(m_hWnd);
		filter.addColumnBox(m_hWnd, list.getColumnList());
		filter.addMethodBox(m_hWnd);

		ctrlFilterContainer.SubclassWindow(filter.getFilterBox().m_hWnd);
		ctrlFilterSelContainer.SubclassWindow(filter.getFilterColumnBox().m_hWnd);
		ctrlFilterMethodContainer.SubclassWindow(filter.getFilterMethodBox().m_hWnd);

		bHandled = FALSE;
		return 0;
	}

	LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		UpdateLayout();
		bHandled = FALSE;
		return 0;
	}

	void UpdateLayout(BOOL /*bResizeBars*/ = TRUE) {
		const int lMargin = 4;

		RECT rect;
		GetClientRect(&rect);

		CRect rc = rect;
		rc.bottom -= 26;
		list.MoveWindow(rc);

		// "Search filter"
		rc.top = rc.bottom + 2;
		rc.bottom = rc.top + 22;

		rc.left = rc.left;
		rc.right = rc.right - 250;
		filter.getFilterBox().MoveWindow(rc);

		rc.left = rc.right + lMargin;
		rc.right = rc.right + 120;
		filter.getFilterColumnBox().MoveWindow(rc);

		rc.left = rc.right + lMargin;
		rc.right = rc.right + 120;
		filter.getFilterMethodBox().MoveWindow(rc);
	}

	LRESULT onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		::KillTimer(m_hWnd, 1);
		updateF();
		bHandled = TRUE;
		return 0;
	}

	void onUpdate() {
		int timeOut = 0;
		int items = list.GetItemCount();
		if (items < 1000) {
			timeOut = 100;
		} else if (items < 10000) {
			timeOut = 300;
		} else if (items < 20000) {
			timeOut = 500;
		} else if (items < 30000) {
			timeOut = 700;
		} else {
			timeOut = 900;
		}

		::SetTimer(m_hWnd, 1, timeOut, NULL);
	}

	ListFilter filter;
	TypedListViewCtrl<T, ctrlId> list;
private:
	ParentT* parent;
	size_t columnCount;

	CContainedWindow listContainer;

	std::function<void ()> updateF;

	CContainedWindow ctrlFilterContainer;
	CContainedWindow ctrlFilterSelContainer;
	CContainedWindow ctrlFilterMethodContainer;
};

#endif // FILTEREDLISTVIEW_H_