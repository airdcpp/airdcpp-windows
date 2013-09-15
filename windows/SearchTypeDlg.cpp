/* 
 * Copyright (C) 2012-2013 AirDC++ Project
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

#include "stdafx.h"
#include "Resource.h"
#include "WinUtil.h"
#include "SearchTypeDlg.h"

#include "../client/StringTokenizer.h"
#include "../client/SearchManager.h"
#include "../client/ResourceManager.h"

#define GET_TEXT(id, var) \
	GetDlgItemText(id, buf, 1024); \
	var = Text::fromT(buf);

#define ATTACH(id, var) var.Attach(GetDlgItem(id))

SearchTypeDlg::SearchTypeDlg() : isDefault(false) { }

SearchTypeDlg::~SearchTypeDlg() {
	//ctrlSearch.Detach();
	//ctrlFileType.Detach();
}

LRESULT SearchTypeDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	ATTACH(IDC_TYPE_NAME, ctrlTypeName);
	ctrlTypeName.SetWindowText(Text::toT(name).c_str());
	ctrlTypeName.EnableWindow(!isDefault);

	ATTACH(IDC_EXTENSION_NAME, ctrlExtensionName);

	ATTACH(IDC_EXTENSIONS, ctrlExtensions);

	CRect rc;
	ctrlExtensions.GetClientRect(rc);
	ctrlExtensions.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT);
	ctrlExtensions.InsertColumn(0, _T("Dummy"), LVCFMT_LEFT, (rc.Width() - 17), 0);
	//ctrlExtensions.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	::SetWindowText(GetDlgItem(IDCANCEL), CTSTRING(CANCEL));
	::SetWindowText(GetDlgItem(IDC_NAME_LABEL), CTSTRING(NAME));
	::SetWindowText(GetDlgItem(IDC_REMOVE), CTSTRING(REMOVE));
	::SetWindowText(GetDlgItem(IDC_ADD), CTSTRING(ADD));
	::SetWindowText(GetDlgItem(IDC_EXTENSIONS_LABEL), CTSTRING(SETTINGS_EXTENSIONS));

	::EnableWindow(GetDlgItem(IDC_REMOVE), false);

	fillList();

	CenterWindow(GetParent());
	SetWindowText(CTSTRING(ADLS_TYPE));
	return TRUE;
}

LRESULT SearchTypeDlg::onSelectionChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	NM_LISTVIEW* lv = (NM_LISTVIEW*) pnmh;
	::EnableWindow(GetDlgItem(IDC_REMOVE), (lv->uNewState & LVIS_FOCUSED));
	return 0;
}

LRESULT SearchTypeDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(wID == IDOK) {
		if (!isDefault) {
			if (ctrlTypeName.GetWindowTextLength() == 0) {
				MessageBox(CTSTRING(LINE_EMPTY));
				return 0;
			}

			if (extList.empty()) {
				MessageBox(CTSTRING(EXTENSIONS_EMPTY));
				return 0;
			}

			TCHAR buf[512];
			ctrlTypeName.GetWindowText(buf, 512);
			name = Text::fromT(buf);

			try {
				SearchManager::getInstance()->validateSearchTypeName(name);
			} catch(const SearchTypeException& e) {
				MessageBox(Text::toT(e.getError()).c_str());
				return 0;
				//showError(e.getError());
			}
		}
	}
	EndDialog(wID);
	return 0;
}

LRESULT SearchTypeDlg::onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TCHAR buf[512];
	if (ctrlExtensionName.GetWindowTextLength() == 0) {
		MessageBox(CTSTRING(LINE_EMPTY));
		return 0;
	}

	ctrlExtensionName.GetWindowText(buf, 512);

	StringTokenizer<tstring> t(buf, ';');
	for (auto i = t.getTokens().begin(); i != t.getTokens().end(); ++i) {
		if(!i->empty()) {
			if(Util::checkExtension(Text::fromT(*i))) {
				extList.push_back(Text::fromT(buf));
			} else {
				MessageBox(CTSTRING_F(INVALID_EXTENSION, *i));
			}
		}
	}

	fillList();
	ctrlExtensionName.SetWindowText(_T(""));

	return 0;
}

LRESULT SearchTypeDlg::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	//int sel = ctrlTypes.GetSelectedIndex();
	int i = -1;
	while((i = ctrlExtensions.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ctrlExtensions.DeleteItem(i);
		extList.erase(extList.begin()+i);
		i--;
	}

	/*int i = ctrlExtensions.GetCurSel();
	if (i > 0) {
		ctrlExtensions.DeleteString(i);

		auto pos = extList.begin();
		advance(pos, i);
		extList.erase(pos);
	}*/
	return 0;
}

void SearchTypeDlg::fillList() {
	sort(extList.begin(), extList.end());
	ctrlExtensions.DeleteAllItems();
	int pos = 0;
	for (auto i = extList.rbegin(); i != extList.rend(); ++i) {
		ctrlExtensions.InsertItem(pos++, Text::toT(*i).c_str());
	}

	ctrlExtensions.RedrawWindow();
	//RedrawWindow();
}