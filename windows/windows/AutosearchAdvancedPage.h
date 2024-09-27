
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

#ifndef AS_SEARCH_ADVANCEDPAGE_H
#define AS_SEARCH_ADVANCEDPAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <atlcrack.h>

#include <airdcpp/util/Util.h>
#include <airdcpp/modules/AutoSearchManager.h>

#include <windows/AutoSearchItemSettings.h>
#include <windows/TabbedDialog.h>

namespace wingui {
class AutoSearchAdvancedPage : public CDialogImpl<AutoSearchAdvancedPage>, public TabPage {
public:

	AutoSearchItemSettings& options;

	enum { IDD = IDD_AS_ADVANCED };

	AutoSearchAdvancedPage(AutoSearchItemSettings& aSettings, const string& aName);
	~AutoSearchAdvancedPage();

	BEGIN_MSG_MAP_EX(AutoSearchAdvancedPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_USE_MATCHER, onCheckMatcher)
		COMMAND_ID_HANDLER(IDC_EXACT_MATCH, onExactMatch)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORDLG, onCtlColor)
		END_MSG_MAP()


	bool write();
	string getName() { return name; }
	void moveWindow(CRect& rc) { this->MoveWindow(rc);  }
	void create(HWND aParent) { this->Create(aParent); }
	void showWindow(BOOL aShow) { 
		this->ShowWindow(aShow);
		if(aShow)
			fixControls();
	}

	LRESULT onCheckMatcher(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onExactMatch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onCtlColor(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		HDC hdc = (HDC)wParam;
		SetBkMode(hdc, TRANSPARENT);
		if (uMsg != WM_CTLCOLORDLG) {
			SetBkColor(hdc, ::GetSysColor(COLOR_3DFACE));
			return (LRESULT)GetStockObject(COLOR_3DFACE);
		}
		return (LRESULT)GetStockObject(NULL_BRUSH);
	}

private:

	CEdit  ctrlCheatingDescription, ctrlUserMatch, ctrlMatcherString;
	CComboBox cMatcherType;
	CComboBox cExcludedWords;

	string name;
	void fixControls();
	bool loading;

};
}


#endif
