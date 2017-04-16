#pragma once
#pragma once

/*
* Copyright (C) 2011-2017 AirDC++ Project
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

#ifndef DYNAMIC_TAB_PAGE_H
#define DYNAMIC_TAB_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <atlcrack.h>

#include <airdcpp/Util.h>


class DynamicTabPage : public CDialogImpl<DynamicTabPage> {
public:

	enum { IDD = IDD_TABPAGE };

	DynamicTabPage();
	~DynamicTabPage();

	BEGIN_MSG_MAP_EX(DynamicTabPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORDLG, onCtlColor)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
	END_MSG_MAP()

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

	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/) {
		return 0;
	}

private:
	CEdit ctrlEdit;
	bool loading;

};


#endif
