#pragma once
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

#ifndef AS_SEARCH_DLG_H
#define AS_SEARCH__DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <atlcrack.h>
#include "../client/AutoSearchManager.h"
#include "AutosearchGeneralPage.h"
#include "AutosearchAdvancedPage.h"
#include "AutoSearchItemSettings.h"

#define ATTACH(id, var) var.Attach(GetDlgItem(id))

class AutoSearchOptionsDlg : public CDialogImpl<AutoSearchOptionsDlg> {
public:

	enum {
		IDD = IDD_AS_DIALOG
	};

	enum Page {
		GENERAL = 0,
		ADVANCED = 1
	};

	AutoSearchOptionsDlg(const AutoSearchPtr& as);
	AutoSearchOptionsDlg();
	~AutoSearchOptionsDlg();

	BEGIN_MSG_MAP_EX(AutoSearchOptionsDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		NOTIFY_HANDLER(IDC_TAB1, TCN_SELCHANGE, onTabChanged)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		END_MSG_MAP()


	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled);
	LRESULT onTabChanged(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/);

	AutoSearchItemSettings options;

private:

	unique_ptr<AutoSearchGeneralPage> AsGeneral;
	unique_ptr<AutoSearchAdvancedPage> AsAdvanced;

	void showPage(int aPage);
	CTabCtrl cTab;

};

#endif
