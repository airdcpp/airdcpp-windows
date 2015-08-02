
/*
* Copyright (C) 2011-2015 AirDC++ Project
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

#ifndef AS_SEARCH_ADVANCEDPAGE_H
#define AS_SEARCH_ADVANCEDPAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <atlcrack.h>
#include "../client/Util.h"
#include "../client/AutoSearchManager.h"
#include "AutoSearchItemSettings.h"


class AutoSearchAdvancedPage : public CDialogImpl<AutoSearchAdvancedPage> {
public:

	AutoSearchItemSettings& options;

	enum { IDD = IDD_AS_ADVANCED };

	AutoSearchAdvancedPage(AutoSearchItemSettings& aSettings);
	~AutoSearchAdvancedPage();

	BEGIN_MSG_MAP_EX(AutoSearchAdvancedPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_USE_MATCHER, onCheckMatcher)
		COMMAND_ID_HANDLER(IDC_CUSTOM_SEARCH_TIMES, onCheckTimes)
		COMMAND_ID_HANDLER(IDC_EXACT_MATCH, onExactMatch)
		COMMAND_HANDLER(IDC_SEARCH_INT, EN_KILLFOCUS, onTimeChange)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORDLG, onCtlColor)
		END_MSG_MAP()


	bool write();
	LRESULT onCheckMatcher(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCheckTimes(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onExactMatch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onTimeChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/);

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onCtlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
		HDC hdc = (HDC)wParam;
		SetBkMode(hdc, TRANSPARENT);
		if (uMsg != WM_CTLCOLORDLG) {
			SetBkColor(hdc, ::GetSysColor(COLOR_3DFACE));
			return (LRESULT)GetStockObject(COLOR_3DFACE);
		}
		return (LRESULT)GetStockObject(NULL_BRUSH);
	}

private:

	CEdit  ctrlCheatingDescription, ctrlUserMatch, ctrlMatcherString, ctrlSearchInterval;
	CComboBox cMatcherType;
	CComboBox cExcludedWords;
	CUpDownCtrl updown;

	
	CDateTimePickerCtrl ctrlSearchStart, ctrlSearchEnd;

	void fixControls();
	bool loading;

};


#endif
