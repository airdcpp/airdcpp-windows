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

#ifndef FAV_OPTIONS_DLG_H
#define FAV_OPTIONS_DLG_H

#include <windows/stdafx.h>
#include <atlcrack.h>

#include <airdcpp/favorites/HubEntry.h>
#include <windows/components/TabbedDialog.h>


namespace wingui {
class FavHubOptionsPage : public CDialogImpl<FavHubOptionsPage>, public TabPage {
public:

	enum { IDD = IDD_FAV_OPTIONS_DLG };

	FavHubOptionsPage(const FavoriteHubEntryPtr& entry, const string& aName);
	~FavHubOptionsPage() { }

	BEGIN_MSG_MAP_EX(FavHubOptionsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORDLG, onCtlColor)
		COMMAND_ID_HANDLER(IDC_SEARCH_INTERVAL_DEFAULT, fixControls)
	END_MSG_MAP()


	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT fixControls(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

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

	void fixControls();

	CComboBox modeCombo4;
	CComboBox modeCombo6;

	FavoriteHubEntryPtr entry;
};
}

#endif