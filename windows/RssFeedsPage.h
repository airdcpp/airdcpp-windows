#pragma once
/*
* Copyright (C) 2012-2022 AirDC++ Project
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

#ifndef RSS_DLG_H
#define RSS_DLG_H

#pragma once

#include <atlcrack.h>
#include "ExListViewCtrl.h"
#include <airdcpp/modules/RSSManager.h>
#include "TabbedDialog.h"

class RssFeedsPage : public CDialogImpl<RssFeedsPage>, public TabPage {
public:

	enum { IDD = IDD_RSS_DLG };

	RssFeedsPage(const string& aName, RSSPtr aFeed);
	~RssFeedsPage();

	BEGIN_MSG_MAP_EX(RssFeedsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_RSS_ENABLE, onEnable)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORDLG, onCtlColor)
		COMMAND_HANDLER(IDC_RSS_INTERVAL, EN_KILLFOCUS, onIntervalChange)
		END_MSG_MAP()


	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onIntervalChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
	LRESULT onEnable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onCtlColor(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		HDC hdc = (HDC)wParam;
		SetBkMode(hdc, TRANSPARENT);
		if (uMsg != WM_CTLCOLORDLG) {
			SetBkColor(hdc, ::GetSysColor(COLOR_3DFACE));
			return (LRESULT)GetStockObject(COLOR_3DFACE);
		}
		return (LRESULT)GetStockObject(NULL_BRUSH);
	}


	bool write();
	string getName() { return name; }
	void moveWindow(CRect& rc) { this->MoveWindow(rc); }
	void create(HWND aParent) { this->Create(aParent); }
	void showWindow(BOOL aShow) { this->ShowWindow(aShow); }

private:

	string name;

	CEdit ctrlUrl;
	CEdit ctrlName;
	CEdit ctrlAutoSearchPattern;
	CEdit ctrlTarget;
	CEdit ctrlInterval;
	CUpDownCtrl updown;

	bool loading;
	RSSPtr feedItem;

	void fixControls();
};
#endif