#pragma once
/*
* Copyright (C) 2012-2024 AirDC++ Project
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

#ifndef RSS_FILTER_H
#define RSS_FILTER_H

#pragma once

#include <atlcrack.h>
#include "ExListViewCtrl.h"
#include <airdcpp/modules/RSSManager.h>
#include "TabbedDialog.h"
#include "DownloadBaseHandler.h"

namespace wingui {
class RssFilterPage : public CDialogImpl<RssFilterPage>, public DownloadBaseHandler<RssFilterPage>, public TabPage {
public:

	enum { IDD = IDD_RSS_FILTER_DLG };

	RssFilterPage(const string& aName, RSSPtr aFeed);
	~RssFilterPage();

	BEGIN_MSG_MAP_EX(RssFilterPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_FILTER_ADD, onAdd)
		COMMAND_ID_HANDLER(IDC_FILTER_REMOVE, onRemove)
		COMMAND_HANDLER(IDC_RSS_BROWSE, BN_CLICKED ,onBrowse)
		COMMAND_ID_HANDLER(IDC_FILTER_UPDATE, onUpdate)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORDLG, onCtlColor)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		COMMAND_HANDLER(IDC_RSS_FILTER_ACTION, CBN_SELENDOK, onAction)
		NOTIFY_HANDLER(IDC_RSS_FILTER_LIST, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_RSS_FILTER_LIST, LVN_ITEMCHANGED, onSelectionChanged)
		MESSAGE_HANDLER(WM_EXITMENULOOP, onExitMenuLoop)

		MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
		MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)

		END_MSG_MAP()


	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onIntervalChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
	LRESULT onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		fixControls();
		return 0;
	}
	LRESULT onExitMenuLoop(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		cBrowse.SetState(false);
		return 0;
	}

	LRESULT onCtlColor(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		HDC hdc = (HDC)wParam;
		SetBkMode(hdc, TRANSPARENT);
		if (uMsg != WM_CTLCOLORDLG) {
			SetBkColor(hdc, ::GetSysColor(COLOR_3DFACE));
			return (LRESULT)GetStockObject(COLOR_3DFACE);
		}
		return (LRESULT)GetStockObject(NULL_BRUSH);
	}

	LRESULT onSelectionChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
		NMLVKEYDOWN* kd = (NMLVKEYDOWN*)pnmh;
		switch (kd->wVKey) {
		case VK_DELETE:
			PostMessage(WM_COMMAND, IDC_FILTER_REMOVE, 0);
			break;
		default:
			bHandled = FALSE;
		}
		return 0;
	}

	/* DownloadBaseHandler */
	void handleDownload(const string& aTarget, Priority p, bool isWhole);

	bool write();
	string getName() { return name; }
	void moveWindow(CRect& rc) { this->MoveWindow(rc); }
	void create(HWND aParent) { this->Create(aParent); }
	void showWindow(BOOL aShow) { this->ShowWindow(aShow); }

private:

	string name;
	bool loading;

	CButton cBrowse;
	CEdit ctrlAutoSearchPattern;
	CEdit ctrlTarget;
	CEdit ctrlExpireDays;
	CComboBox cMatcherType;
	CComboBox cGroups;
	CComboBox cAction;
	CUpDownCtrl updown;


	ExListViewCtrl ctrlRssFilterList;
	vector<RSSFilter> filterList;

	RSSPtr feedItem;

	void fillList();

	void remove(int i);
	void add(const string& aPattern, const string& aTarget, int aMethod);
	bool update();

	bool validateSettings(const string& aPattern);
	void restoreSelection(const tstring& curSel);

	void handleCopyFilters(const vector<RSSFilter>& aList);
	void handlePasteFilters(const string& aClipText);

	string getClipBoardText();

	void fixControls();

};

}

#endif