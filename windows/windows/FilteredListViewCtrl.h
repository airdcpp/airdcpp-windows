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

#ifndef FILTEREDLISTVIEW_H_
#define FILTEREDLISTVIEW_H_

#include <windows/stdafx.h>

#include <windows/ListFilter.h>
#include <windows/TypedListViewCtrl.h>
#include <windows/OMenu.h>

#include <airdcpp/core/types/DupeType.h>
#include <airdcpp/settings/SettingsManager.h>

#define KEY_MESSAGE_MAP 7

#define FLV_HAS_CHECKBOXES 0x01
#define FLV_HAS_OPTIONS 0x02 
#define FLV_HAS_DUPE_OPTIONS 0x04 
#define FLV_DEFAULT FLV_HAS_CHECKBOXES | FLV_HAS_OPTIONS | FLV_HAS_DUPE_OPTIONS


namespace wingui {
template<class ContainerT, class ParentT, int ctrlId, DWORD style = FLV_DEFAULT>
class FilteredListViewCtrl : public CWindowImpl<FilteredListViewCtrl<ContainerT, ParentT, ctrlId, style>>  {

public:
	enum Settings {
		SETTING_SHARED,
		SETTING_QUEUED,
		SETTING_INVERSED,
		SETTING_TOP,
		SETTING_PARTIAL_DUPES,
		SETTING_RESET_CHANGE,
		SETTING_LAST
	};

	typedef FilteredListViewCtrl<ContainerT, ParentT, ctrlId, style> thisClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		MESSAGE_HANDLER(WM_TIMER, onTimer)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_EXITMENULOOP, onExitMenuLoop)
		MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBackground)
		if (style & FLV_HAS_CHECKBOXES) {
			COMMAND_ID_HANDLER(IDC_FILTER_QUEUED, onShow)
			COMMAND_ID_HANDLER(IDC_FILTER_SHARED, onShow)
		}
		if (style & FLV_HAS_OPTIONS) {
			COMMAND_ID_HANDLER(IDC_FILTER_OPTIONS, onClickOptions)
		}
		CHAIN_MSG_MAP_MEMBER(filter)
		FORWARD_NOTIFICATIONS()
		ALT_MSG_MAP(KEY_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CHAR, onChar)
	END_MSG_MAP();

	FilteredListViewCtrl(ParentT* aParent, size_t colCount, std::function<void ()> aUpdateF, SettingsManager::BoolSetting* aSettings, int aInitialColumn) : 
		onTop(true), resetOnChange(true), filterPartialDupes(false), filterShared(true), filterQueued(true), updateF(aUpdateF), parent(aParent), settings(aSettings),
		filter(colCount, [this] { onUpdate(); }), columnCount(colCount), initialColumn(aInitialColumn),
		filterMethodContainer(WC_COMBOBOX, this, KEY_MESSAGE_MAP),
		filterContainer(WC_EDIT, this, KEY_MESSAGE_MAP),
		filterColumnContainer(WC_COMBOBOX, this, KEY_MESSAGE_MAP),
		queuedContainer(WC_COMBOBOX, this, KEY_MESSAGE_MAP),
		sharedContainer(WC_EDIT, this, KEY_MESSAGE_MAP),
		optionsContainer(WC_COMBOBOX, this, KEY_MESSAGE_MAP)
	{
		if (style & FLV_HAS_CHECKBOXES) {
			filterShared = SettingsManager::getInstance()->get(settings[SETTING_SHARED]);
			filterQueued = SettingsManager::getInstance()->get(settings[SETTING_QUEUED]);
		}
		if (style & FLV_HAS_DUPE_OPTIONS) {
			filterPartialDupes = SettingsManager::getInstance()->get(settings[SETTING_PARTIAL_DUPES]);
		}
		onTop = SettingsManager::getInstance()->get(settings[SETTING_TOP]);
		resetOnChange = SettingsManager::getInstance()->get(settings[SETTING_RESET_CHANGE]);
		filter.setInverse(SettingsManager::getInstance()->get(settings[SETTING_INVERSED]));
	}

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		list.Create(this->m_hWnd, this->rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, ctrlId);
		list.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

		parent->createColumns();

		filter.addFilterBox(this->m_hWnd);
		filter.addColumnBox(this->m_hWnd, list.getColumnList(), initialColumn, parent->m_hWnd);
		filter.addMethodBox(this->m_hWnd);
		if (style & FLV_HAS_CHECKBOXES) {
			ctrlQueued.Create(this->m_hWnd, this->rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_FILTER_QUEUED);
			ctrlQueued.SetWindowText(CTSTRING(QUEUED));
			ctrlQueued.SetButtonStyle(BS_AUTOCHECKBOX, false);
			ctrlQueued.SetFont(WinUtil::systemFont);
			ctrlQueued.SetCheck(filterQueued ? BST_CHECKED : BST_UNCHECKED);

			ctrlShared.Create(this->m_hWnd, this->rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, IDC_FILTER_SHARED);
			ctrlShared.SetWindowText(CTSTRING(SHARED));
			ctrlShared.SetButtonStyle(BS_AUTOCHECKBOX, false);
			ctrlShared.SetFont(WinUtil::systemFont);
			ctrlShared.SetCheck(filterShared ? BST_CHECKED : BST_UNCHECKED);
			
			queuedContainer.SubclassWindow(ctrlQueued.m_hWnd);
			sharedContainer.SubclassWindow(ctrlShared.m_hWnd);

		}
		if (style & FLV_HAS_OPTIONS) {
			ctrlOptions.Create(this->m_hWnd, this->rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_PUSHBUTTON, NULL, IDC_FILTER_OPTIONS);
			ctrlOptions.SetWindowText(CTSTRING(OPTIONS_DOTS));
			ctrlOptions.SetFont(WinUtil::systemFont);
			optionsContainer.SubclassWindow(ctrlOptions.m_hWnd);
		}

		filterContainer.SubclassWindow(filter.getFilterBox().m_hWnd);
		filterMethodContainer.SubclassWindow(filter.getFilterMethodBox().m_hWnd);
		filterColumnContainer.SubclassWindow(filter.getFilterColumnBox().m_hWnd);

		bHandled = FALSE;
		return 0;
	}

	LRESULT onClickOptions(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
		CRect rect;
		ctrlOptions.GetWindowRect(rect);
		auto pt = rect.BottomRight();
		pt.x = pt.x - rect.Width();
		ctrlOptions.SetState(true);

		OMenu optionMenu;
		optionMenu.CreatePopupMenu();
		optionMenu.appendItem(TSTRING(EXCLUDE_MATCHES), [this] {
			filter.setInverse(!filter.getInverse());
			SettingsManager::getInstance()->set(settings[SETTING_INVERSED], filter.getInverse());
			updateF();
		}, filter.getInverse() ? OMenu::FLAG_CHECKED : 0);

		optionMenu.appendItem(TSTRING(SHOW_ON_TOP), [this] {
			onTop = !onTop;
			SettingsManager::getInstance()->set(settings[SETTING_TOP], onTop);
			UpdateLayout();
		}, onTop ? OMenu::FLAG_CHECKED : 0);

		//ugly hack!
		optionMenu.appendItem(settings[SETTING_RESET_CHANGE] == SettingsManager::FILTER_SEARCH_RESET_CHANGE ? TSTRING(RESET_NEW_SEARCH) : TSTRING(RESET_FOLDER_CHANGE), [this] {
			resetOnChange = !resetOnChange;
			SettingsManager::getInstance()->set(settings[SETTING_RESET_CHANGE], resetOnChange);
		}, resetOnChange ? OMenu::FLAG_CHECKED : 0);

		if (style & FLV_HAS_DUPE_OPTIONS) {
			optionMenu.appendItem(TSTRING(PARTIAL_DUPES_EQUAL), [this] {
				filterPartialDupes = !filterPartialDupes;
				SettingsManager::getInstance()->set(settings[SETTING_PARTIAL_DUPES], filterPartialDupes);
				updateF();
			}, filterPartialDupes ? OMenu::FLAG_CHECKED : 0);
		}

		optionMenu.open(this->m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERPOSANIMATION, pt);
		return 0;
	}

	LRESULT onExitMenuLoop(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/) {
		ctrlOptions.SetState(false);
		return 0;
	}

	LRESULT onEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & /*bHandled*/) {
		// draw the background
		WTL::CDCHandle dc(reinterpret_cast<HDC>(wParam));
		RECT rc;
		this->GetClientRect(&rc);
		dc.FillRect(&rc, GetSysColorBrush(COLOR_3DFACE));
		//dc.FillRect(&rc, WinUtil::bgBrush);

		// draw the left border
		HGDIOBJ oldPen = SelectObject(dc, CreatePen(PS_SOLID, 1, GetSysColor(COLOR_APPWORKSPACE)));
		MoveToEx(dc, rc.left, rc.top, (LPPOINT) NULL);
		LineTo(dc, rc.left, rc.bottom);

		// draw the right border
		MoveToEx(dc, rc.right, rc.top, (LPPOINT) NULL);
		LineTo(dc, rc.right, rc.bottom);

		if (onTop) {
			// draw the top border
			MoveToEx(dc, rc.left, rc.top, (LPPOINT) NULL);
			LineTo(dc, rc.right, rc.top);
		} else {
			// draw the bottom border
			MoveToEx(dc, rc.left, rc.bottom-1, (LPPOINT) NULL);
			LineTo(dc, rc.right, rc.bottom-1);
		}

		DeleteObject(SelectObject(dc, oldPen));
		return TRUE;
	}

	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		HWND hWnd = (HWND) lParam;
		if (hWnd == ctrlQueued.m_hWnd || hWnd == ctrlShared.m_hWnd) {
			HDC hDC = (HDC) wParam;
			::SetBkColor(hDC, ::GetSysColor(COLOR_3DFACE));
			return (LRESULT)::GetSysColorBrush(COLOR_3DFACE);
			//::SetBkColor(hDC, WinUtil::bgColor);
			//::SetTextColor(hDC, WinUtil::textColor);
			//::SetBkMode(hDC, TRANSPARENT);
			//return (LRESULT) GetStockObject(HOLLOW_BRUSH);
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
		const int sharedWidth = (style & FLV_HAS_CHECKBOXES) ?
			WinUtil::getTextWidth(TSTRING(SHARED), ctrlShared.m_hWnd) + 20 : 0;
		const int queuedWidth = (style & FLV_HAS_CHECKBOXES) ?
			WinUtil::getTextWidth(TSTRING(QUEUED), ctrlQueued.m_hWnd) + 20 : 0;
		const int optionsWidth = (style & FLV_HAS_OPTIONS) ?
			WinUtil::getTextWidth(TSTRING(SETTINGS_OPTIONS), ctrlOptions.m_hWnd) + 20 : 0;

		RECT rect;
		this->GetClientRect(&rect);

		CRect rc = rect;
		if (onTop) {
			rc.top += 40;
			list.MoveWindow(rc);

			rc.bottom = rc.top - 10;
			rc.top = rc.bottom - 20;
		} else {
			rc.bottom -= 40;
			list.MoveWindow(rc);

			rc.top = rc.bottom + 10;
			rc.bottom = rc.top + 20;
		}

		rc.left = rc.left + 5;
		rc.right = rc.right - 250 - sharedWidth - queuedWidth - optionsWidth - 10;
		filter.getFilterBox().MoveWindow(rc);

		rc.left = rc.right + lMargin;
		rc.right = rc.right + 120;
		filter.getFilterColumnBox().MoveWindow(rc);

		rc.left = rc.right + lMargin;
		rc.right = rc.right + 120;
		filter.getFilterMethodBox().MoveWindow(rc);

		if (style & FLV_HAS_CHECKBOXES) {
			rc.left = rc.right + lMargin;
			rc.right = rc.left + queuedWidth;
			ctrlQueued.MoveWindow(rc);

			rc.left = rc.right + lMargin;
			rc.right = rc.left + sharedWidth;
			ctrlShared.MoveWindow(rc);
		}
		if (style & FLV_HAS_OPTIONS) {
			rc.left = rc.right + lMargin;
			rc.right = rc.left + optionsWidth;
			ctrlOptions.MoveWindow(rc);
		}

		this->ShowScrollBar(SB_BOTH, FALSE);
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
		}
		return 0;
	}

	LRESULT onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		::KillTimer(this->m_hWnd, 1);
		updateF();
		bHandled = TRUE;
		return 0;
	}

	bool checkDupe(DupeType aDupe) {
		if (!filterQueued && (aDupe == DUPE_QUEUE_FULL || (filterPartialDupes && aDupe == DUPE_QUEUE_PARTIAL))) {
			return false;
		} else if (!filterShared && (aDupe == DUPE_SHARE_FULL || (filterPartialDupes && aDupe == DUPE_SHARE_PARTIAL))) {
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

		::SetTimer(this->m_hWnd, 1, timeOut, NULL);
	}

	void changeFilterState(bool enable) {
		filter.textEdit.EnableWindow(enable);
		filter.columnCombo.EnableWindow(enable);
		filter.methodCombo.EnableWindow(enable);
	}

	ListFilter filter;
	ContainerT list;

	void onListChanged(bool forceReset) {
		if (resetOnChange || forceReset) {
			filter.clear();
		}
	}

	LRESULT onChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		switch (wParam) {
		case VK_TAB:
			//if (uMsg == WM_KEYDOWN) {
				onTab();
			//}
			break;
		default:
			bHandled = FALSE;
		}
		return 0;
	}

	void onTab() {
		HWND wnds [] = {
			filter.getFilterBox().m_hWnd, filter.getFilterColumnBox().m_hWnd, filter.getFilterMethodBox().m_hWnd, ctrlQueued.m_hWnd, ctrlShared.m_hWnd, ctrlOptions.m_hWnd
		};

		HWND focus = GetFocus();

		static const int size = sizeof(wnds) / sizeof(wnds[0]);
		WinUtil::handleTab(focus, wnds, size);
	}
private:
	SettingsManager::BoolSetting* settings;
	ParentT* parent;
	size_t columnCount;

	std::function<void ()> updateF;

	CButton ctrlQueued;
	CButton ctrlShared;
	CButton ctrlOptions;
	bool filterQueued;
	bool filterShared;
	bool resetOnChange;
	bool filterPartialDupes;
	bool onTop;
	int initialColumn;

	CContainedWindow filterContainer;
	CContainedWindow filterMethodContainer;
	CContainedWindow filterColumnContainer;
	CContainedWindow queuedContainer;
	CContainedWindow sharedContainer;
	CContainedWindow optionsContainer;
};
}

#endif // FILTEREDLISTVIEW_H_