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

#include <commctrl.h>
#include <windows/stdafx.h>
#include <windows/Resource.h>
#include <windows/TabbedDialog.h>
#include <airdcpp/util/text/Text.h>

namespace wingui {
TabbedDialog::TabbedDialog(const string& aTitle) : wTitle(aTitle) { }

TabbedDialog::~TabbedDialog() { 
}

LRESULT TabbedDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	ATTACH(IDC_TAB1, cTab);
	int i = 0;
	for (auto& p : pages) {
		cTab.InsertItem(i, TCIF_TEXT, Text::toT(p->getName()).c_str(), -1, NULL);
		p->create(cTab.m_hWnd);
		i++;
	}
	cTab.SetCurSel(0);
	::SetWindowText(GetDlgItem(IDCANCEL), CTSTRING(CANCEL));

	CenterWindow(GetParent());
	SetWindowText(Text::toT(wTitle).c_str());
	showPage(0);

	return TRUE;
}

LRESULT TabbedDialog::onTabChanged(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
	showPage(cTab.GetCurSel());
	return 0;
}

void TabbedDialog::showPage(int aPage) {
	CRect rc;
	cTab.GetClientRect(rc);
	//cTab.GetItemRect(0, rc);
	cTab.AdjustRect(FALSE, rc);
	int i = 0;
	for (auto& page : pages) {
		if (i == aPage) {
			page->moveWindow(rc);
			page->showWindow(SW_SHOW);
		} else {
			page->showWindow(SW_HIDE);
			page->showWindow(SW_HIDE);
		}
		i++;
	}
	//cTab.SetWindowPos(HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE);
}

LRESULT TabbedDialog::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled) {
	if (wID == IDOK) {
		for (auto& page : pages) {
			if (!page->write()) {
				bHandled = FALSE;
				return 0;
			}
		}
	}
	EndDialog(wID);
	return 0;
}
}