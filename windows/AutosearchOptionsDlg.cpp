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

#include <commctrl.h>
#include "stdafx.h"
#include "Resource.h"
#include "AutoSearchOptionsDlg.h"

AutoSearchOptionsDlg::AutoSearchOptionsDlg(ItemSettings& aSettings) : settings(aSettings) {}

AutoSearchOptionsDlg::~AutoSearchOptionsDlg() { }

LRESULT AutoSearchOptionsDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	ATTACH(IDC_TAB1, cTab);

	cTab.InsertItem(GENERAL, TCIF_TEXT, _T("General"), -1, NULL);
	cTab.InsertItem(ADVANCED, TCIF_TEXT, _T("Advanced"), -1, NULL);
	cTab.SetCurSel(0);

	//Create the pages
	AsGeneral.reset(new AutoSearchGeneralPage(settings));
	AsGeneral->Create(cTab.m_hWnd);

	AsAdvanced.reset(new AutoSearchAdvancedPage(settings));
	AsAdvanced->Create(cTab.m_hWnd);

	CenterWindow(GetParent());
	SetWindowText(CTSTRING(AUTOSEARCH_DLG));
	showPage(GENERAL);

	return TRUE;
}

LRESULT AutoSearchOptionsDlg::onTabChanged(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
	showPage(cTab.GetCurSel());
	return 0;
}

void AutoSearchOptionsDlg::showPage(int aPage) {
	CRect rc;
	cTab.GetClientRect(rc);
	//cTab.GetItemRect(0, rc);
	cTab.AdjustRect(FALSE, rc);

	switch (aPage) {
	case GENERAL:
		AsAdvanced->ShowWindow(SW_HIDE);
		AsGeneral->MoveWindow(rc);
		AsGeneral->ShowWindow(SW_SHOW);
		break;
	case ADVANCED:
		AsGeneral->ShowWindow(SW_HIDE);
		AsAdvanced->MoveWindow(rc);
		AsAdvanced->ShowWindow(SW_SHOW);
	}

}

LRESULT AutoSearchOptionsDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled) {
	if (wID == IDOK) {
		if (!AsGeneral->write() || !AsAdvanced->write()) {
			bHandled = FALSE;
			return 0;
		}
	}
	EndDialog(wID);
	return 0;
}


