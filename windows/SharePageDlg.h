/* 
 * Copyright (C) 2011-2013 AirDC++ Project
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

#if !defined(SHAREPAGE_DLG_H)
#define SHAREPAGE_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "../client/ResourceManager.h"
#include "../client/Text.h"

class SharePageDlg : public CDialogImpl<SharePageDlg>
{
	CEdit ctrlLine;
	CStatic ctrlLineDescription;
	CStatic ctrlListDescription;
public:
	tstring line;
	bool rename;
	//tstring description;
	//tstring title;

	ProfileTokenList selectedProfiles;

	CListViewCtrl ctrlList;


	ShareProfile::list profiles;

	enum { IDD = IDD_SHAREPAGE_DLG };
	
	BEGIN_MSG_MAP(SharePageDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_SELECT_ALL, onSelectAll)
	END_MSG_MAP()
	
	SharePageDlg() { }
	
	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlLine.SetFocus();
		return FALSE;
	}

	LRESULT onSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		int pos = 0;
		while(pos < ctrlList.GetItemCount()) {
			ctrlList.SetCheckState(pos++, true);
		}
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		ctrlLine.Attach(GetDlgItem(IDC_LINE));
		ctrlLine.SetFocus();
		ctrlLine.SetWindowText(line.c_str());
		ctrlLine.SetSelAll(TRUE);

		ctrlLineDescription.Attach(GetDlgItem(IDC_DESCRIPTION));
		ctrlLineDescription.SetWindowText(CTSTRING(VIRTUAL_NAME_LONG));

		ctrlListDescription.Attach(GetDlgItem(IDC_PROFILES_DESC));
		ctrlListDescription.SetWindowText(rename ? CTSTRING(RENAME_DLG_DESC) : CTSTRING(ADD_DLG_DESC));

		::SetWindowText(GetDlgItem(IDC_SELECT_ALL), CTSTRING(SELECT_ALL));
		
		SetWindowText(CTSTRING(VIRTUAL_NAME));

		ctrlList.Attach(GetDlgItem(IDC_PROFILES));
		CRect rc;
		ctrlList.GetClientRect(rc);
		ctrlList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
		ctrlList.InsertColumn(0, _T("Dummy"), LVCFMT_LEFT, rc.Width(), 0);

		LVITEM lvi;
		lvi.mask = LVIF_TEXT;
		lvi.iSubItem = 0;

		int counter = 0;
		for(auto i = profiles.begin(); i != profiles.end(); i++) {
			ctrlList.InsertItem(counter++, Text::toT((*i)->getDisplayName()).c_str());
			/*lvi.iItem = counter++;
			auto text = const_cast<TCHAR*>(Text::toT((*i)->getDisplayName()).c_str());
			auto text2 = Text::toT((*i)->getDisplayName()).c_str();
			auto text3 = (*i)->getDisplayName();
			auto text4 = Text::toT((*i)->getDisplayName());
			lvi.pszText = const_cast<TCHAR*>(Text::toT((*i)->getDisplayName()).c_str());
			ctrlList.InsertItem(&lvi);*/
			//ctrl.SetCheckState(i, false);
		}
		ctrlList.SetColumnWidth(0, LVSCW_AUTOSIZE);
		//ctrlList.Detach();

		CenterWindow(GetParent());
		return FALSE;
	}
	
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(wID == IDOK) {
			int pos = 0;
			while(pos < ctrlList.GetItemCount()) {
				if (ctrlList.GetCheckState(pos++)) {
					selectedProfiles.push_back(profiles[pos-1]->getToken());
				}
			}
			line.resize(ctrlLine.GetWindowTextLength() + 1);
			line.resize(GetDlgItemText(IDC_LINE, &line[0], line.size()));
		}
		EndDialog(wID);
		return 0;
	}
	
};

#endif // !defined(SHAREPAGE_DLG_H)