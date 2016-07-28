#pragma once
/*
* Copyright (C) 2012-2016 AirDC++ Project
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

#ifndef RSS_FILTER_H
#define RSS_FILTER_H

#pragma once

#include <atlcrack.h>
#include "ExListViewCtrl.h"
#include <airdcpp/RSSManager.h>
#include "TabbedDialog.h"

class RssFilterPage : public CDialogImpl<RssFilterPage>, public TabPage {
public:

	enum { IDD = IDD_RSS_FILTER_DLG };

	RssFilterPage(const string& aName);
	~RssFilterPage();

	BEGIN_MSG_MAP_EX(RssFilterPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_RSS_ADD, onAdd)
		COMMAND_ID_HANDLER(IDC_RSS_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_RSS_BROWSE, onBrowse)
		COMMAND_ID_HANDLER(IDC_RSS_UPDATE, onUpdate)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORDLG, onCtlColor)
		NOTIFY_HANDLER(IDC_RSS_FILTER_LIST, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_RSS_FILTER_LIST, LVN_ITEMCHANGED, onSelectionChanged)
		END_MSG_MAP()


	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onIntervalChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
	LRESULT onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

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
			PostMessage(WM_COMMAND, IDC_RSS_REMOVE, 0);
			break;
		default:
			bHandled = FALSE;
		}
		return 0;
	}

	bool write();
	string getName() { return name; }
	void moveWindow(CRect& rc) { this->MoveWindow(rc); }
	void create(HWND aParent) { this->Create(aParent); }
	void showWindow(BOOL aShow) { this->ShowWindow(aShow); }

private:

	string name;
	bool loading;

	CEdit ctrlAutoSearchPattern;
	CEdit ctrlTarget;
	ExListViewCtrl ctrlRssFilterList;

	vector<RSSFilter> filterList;

	void fillList();

	void remove(int i);
	void add(const string& aPattern, const string& aTarget);
	bool update();

	bool validateSettings(const string& aPattern);
	void restoreSelection(const tstring& curSel);


};
#endif