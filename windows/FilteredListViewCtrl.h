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

#include "../client/AirUtil.h"
#include "../client/SettingsManager.h"

#define FILTER_MESSAGE_MAP 8

template<class ContainerT, class ParentT, int ctrlId>
class FilteredListViewCtrl : public CWindowImpl<FilteredListViewCtrl<ContainerT, ParentT, ctrlId>>  {

public:
	enum Settings {
		SETTING_SHARED,
		SETTING_QUEUED,
		SETTING_INVERSED,
		SETTING_LAST
	};

	typedef FilteredListViewCtrl<ContainerT, ParentT, ctrlId> thisClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		MESSAGE_HANDLER(WM_TIMER, onTimer)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)

		COMMAND_ID_HANDLER(IDC_FILTER_QUEUED, onShow)
		COMMAND_ID_HANDLER(IDC_FILTER_SHARED, onShow)
		COMMAND_ID_HANDLER(IDC_FILTER_INVERSE, onShow)
		FORWARD_NOTIFICATIONS()
	ALT_MSG_MAP(FILTER_MESSAGE_MAP)
		CHAIN_MSG_MAP_MEMBER(filter)
		//MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, onCtlColor)
		//MESSAGE_HANDLER(WM_CHAR, onFilterChar)
		//MESSAGE_HANDLER(WM_KEYUP, onFilterChar)
	END_MSG_MAP();

	FilteredListViewCtrl(ParentT* aParent, size_t colCount, std::function<void ()> aUpdateF, SettingsManager::BoolSetting* aSettings) : filterShared(true), filterQueued(true), updateF(aUpdateF), parent(aParent), settings(aSettings),
		filter(colCount, [this] { onUpdate(); }), columnCount(colCount), /*, listContainer(WC_LISTVIEW, this, CONTROL_MESSAGE_MAP),*/
		ctrlFilterContainer(WC_EDIT, this, FILTER_MESSAGE_MAP),
		ctrlFilterSelContainer(WC_COMBOBOX, this, FILTER_MESSAGE_MAP),
		ctrlFilterMethodContainer(WC_COMBOBOX, this, FILTER_MESSAGE_MAP)
	{
		filterShared = SettingsManager::getInstance()->get(settings[SETTING_SHARED]);
		filterQueued = SettingsManager::getInstance()->get(settings[SETTING_QUEUED]);
		filter.setInverse(SettingsManager::getInstance()->get(settings[SETTING_INVERSED]));
	}

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		list.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, ctrlId);
		//listContainer.SubclassWindow(list);
		list.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

		parent->createColumns();

		filter.addFilterBox(m_hWnd);
		filter.addColumnBox(m_hWnd, list.getColumnList());
		filter.addMethodBox(m_hWnd);

		ctrlFilterContainer.SubclassWindow(filter.getFilterBox().m_hWnd);
		ctrlFilterSelContainer.SubclassWindow(filter.getFilterColumnBox().m_hWnd);
		ctrlFilterMethodContainer.SubclassWindow(filter.getFilterMethodBox().m_hWnd);


		ctrlQueued.Create(m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_FILTER_QUEUED);
		ctrlQueued.SetWindowText(CTSTRING(QUEUED));
		ctrlQueued.SetButtonStyle(BS_AUTOCHECKBOX, false);
		ctrlQueued.SetFont(WinUtil::systemFont);
		ctrlQueued.SetCheck(filterQueued ? BST_CHECKED : BST_UNCHECKED);

		ctrlShared.Create(m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_FILTER_SHARED);
		ctrlShared.SetWindowText(CTSTRING(SHARED));
		ctrlShared.SetButtonStyle(BS_AUTOCHECKBOX, false);
		ctrlShared.SetFont(WinUtil::systemFont);
		ctrlShared.SetCheck(filterShared ? BST_CHECKED : BST_UNCHECKED);

		ctrlInverse.Create(m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_FILTER_INVERSE);
		ctrlInverse.SetWindowText(CTSTRING(HIDE_MATCHES));
		ctrlInverse.SetButtonStyle(BS_AUTOCHECKBOX, false);
		ctrlInverse.SetFont(WinUtil::systemFont);
		ctrlInverse.SetCheck(filter.getInverse() ? BST_CHECKED : BST_UNCHECKED);

		bHandled = FALSE;
		return 0;
	}

	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		HWND hWnd = (HWND) lParam;
		HDC hDC = (HDC) wParam;
		if (hWnd == ctrlQueued.m_hWnd || hWnd == ctrlShared.m_hWnd ||hWnd == ctrlInverse.m_hWnd) {
			::SetBkColor(hDC, WinUtil::bgColor);
			::SetTextColor(hDC, WinUtil::textColor);
			return (LRESULT) WinUtil::bgBrush;
		}

		bHandled = FALSE;
		return FALSE;
	}

	LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		UpdateLayout();
		bHandled = FALSE;
		return 0;
	}

	void UpdateLayout(BOOL /*bResizeBars*/ = TRUE) {
		const int lMargin = 4;
		const int sharedWidth = WinUtil::getTextWidth(TSTRING(SHARED), ctrlShared.m_hWnd) + 20;
		const int queuedWidth = WinUtil::getTextWidth(TSTRING(QUEUED), ctrlQueued.m_hWnd) + 20;
		const int inverseWidth = WinUtil::getTextWidth(TSTRING(HIDE_MATCHES), ctrlInverse.m_hWnd) + 20;

		RECT rect;
		GetClientRect(&rect);

		CRect rc = rect;
		rc.bottom -= 26;
		list.MoveWindow(rc);

		// "Search filter"
		rc.top = rc.bottom + 2;
		rc.bottom = rc.top + 22;

		rc.left = rc.left;
		rc.right = rc.right - 250 - sharedWidth - queuedWidth - inverseWidth - 10;
		filter.getFilterBox().MoveWindow(rc);

		rc.left = rc.right + lMargin;
		rc.right = rc.right + 120;
		filter.getFilterColumnBox().MoveWindow(rc);

		rc.left = rc.right + lMargin;
		rc.right = rc.right + 120;
		filter.getFilterMethodBox().MoveWindow(rc);

		rc.left = rc.right + lMargin;
		rc.right = rc.left + queuedWidth;
		ctrlQueued.MoveWindow(rc);

		rc.left = rc.right + lMargin;
		rc.right = rc.left + sharedWidth;
		ctrlShared.MoveWindow(rc);

		rc.left = rc.right + lMargin;
		rc.right = rc.left + inverseWidth;
		ctrlInverse.MoveWindow(rc);

		//sr.left = sr.right + 8;
		//sr.right = sr.left + ctrlShowOnline.GetWindowTextLength() * WinUtil::getTextWidth(ctrlShowOnline.m_hWnd, WinUtil::systemFont) + 24;
		//ctrlShowOnline.MoveWindow(sr);
	}

	LRESULT onShow(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
		switch (wID) {
			case IDC_FILTER_QUEUED: {
				filterQueued = !filterQueued;
				SettingsManager::getInstance()->set(settings[SETTING_QUEUED], filterQueued);
				updateF();
				break;
			}
			case IDC_FILTER_SHARED: {
				filterShared = !filterShared;
				SettingsManager::getInstance()->set(settings[SETTING_SHARED], filterShared);
				updateF();
				break;
			}
			case IDC_FILTER_INVERSE: {
				filter.setInverse(!filter.getInverse());
				SettingsManager::getInstance()->set(settings[SETTING_INVERSED], filter.getInverse());
				updateF();
				break;
			}
		}
		return 0;
	}

	LRESULT onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		::KillTimer(m_hWnd, 1);
		updateF();
		bHandled = TRUE;
		return 0;
	}

	bool checkDupe(DupeType aDupe) {
		if (!filterQueued && aDupe == QUEUE_DUPE) {
			return false;
		} else if (!filterShared && aDupe == SHARE_DUPE) {
			return false;
		}

		return true;
	}

	void onUpdate() {
		int timeOut = 0;
		auto items = parent->getTotalListItemCount();
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

	void changeFilterState(bool enable) {
		filter.text.EnableWindow(enable);
		filter.column.EnableWindow(enable);
		filter.method.EnableWindow(enable);
	}

	ListFilter filter;
	ContainerT list;
private:
	SettingsManager::BoolSetting* settings;
	ParentT* parent;
	size_t columnCount;

	CContainedWindow listContainer;

	std::function<void ()> updateF;

	CContainedWindow ctrlFilterContainer;
	CContainedWindow ctrlFilterSelContainer;
	CContainedWindow ctrlFilterMethodContainer;

	CButton ctrlQueued;
	CButton ctrlShared;
	CButton ctrlInverse;
	bool filterQueued;
	bool filterShared;
	//CButton ctrlShowOnline;
};

#endif // FILTEREDLISTVIEW_H_