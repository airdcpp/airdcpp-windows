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

#include "stdafx.h"
#include "Resource.h"
#include "WinUtil.h"
#include "DirectoryListingDlg.h"

#include "../client/SearchManager.h"
#include "../client/ResourceManager.h"

#define GET_TEXT(id, var) \
	GetDlgItemText(id, buf, 1024); \
	var = Text::fromT(buf);

#define ATTACH(id, var) var.Attach(GetDlgItem(id))

DirectoryListingDlg::DirectoryListingDlg(bool aSupportsASCH) : fileTypeStr(SEARCH_TYPE_ANY), fileType(0), size(0), sizeMode(0), useCurDir(false), supportsASCH(aSupportsASCH) { }

DirectoryListingDlg::~DirectoryListingDlg() {
	//ctrlSearch.Detach();
	//ctrlFileType.Detach();
}

LRESULT DirectoryListingDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	ATTACH(IDC_SEARCH_STRING, ctrlSearch);
	ctrlSearch.SetFocus();
	//ctrlSearch.SetWindowText(Text::toT(searchStr).c_str());
	WinUtil::appendHistory(ctrlSearch, SettingsManager::HISTORY_SEARCH);

	//ctrlSearch.SetSelAll(TRUE);

	ATTACH(IDC_FILETYPES, ctrlFileType);
	ATTACH(IDC_SEARCH_SIZE, ctrlSize);

	ATTACH(IDC_SIZE_MODE, ctrlSizeMode);
	ctrlSizeMode.AddString(CTSTRING(NORMAL));
	ctrlSizeMode.AddString(CTSTRING(AT_LEAST));
	ctrlSizeMode.AddString(CTSTRING(AT_MOST));
	ctrlSizeMode.AddString(CTSTRING(EXACT_SIZE));
	ctrlSizeMode.SetCurSel(1);

	ATTACH(IDC_SIZE_UNIT, ctrlSizeUnit);
	ctrlSizeUnit.AddString(CTSTRING(B));
	ctrlSizeUnit.AddString(CTSTRING(KiB));
	ctrlSizeUnit.AddString(CTSTRING(MiB));
	ctrlSizeUnit.AddString(CTSTRING(GiB));
	ctrlSizeUnit.SetCurSel(0);

	::SetWindowText(GetDlgItem(IDOK), (CTSTRING(OK)));
	::SetWindowText(GetDlgItem(IDCANCEL), CTSTRING(CANCEL));
	::SetWindowText(GetDlgItem(IDC_DL_SEARCH_DESC), CTSTRING(SEARCH_STRING));
	::SetWindowText(GetDlgItem(IDC_DL_TYPE_TEXT), CTSTRING(FILE_TYPE));
	::SetWindowText(GetDlgItem(IDC_SIZE_LABEL), CTSTRING(SIZE));

	::SetWindowText(GetDlgItem(IDC_USE_CUR_DIR), CTSTRING(SEARCH_CUR_DIR));
	if (!supportsASCH)
		::EnableWindow(GetDlgItem(IDC_USE_CUR_DIR),	FALSE);

	fileTypeStr = SETTING(LAST_FL_FILETYPE);
	ctrlFileType.fillList(fileTypeStr);


	CenterWindow(GetParent());
	SetWindowText(CTSTRING(SEARCH));

	return FALSE;
}

LRESULT DirectoryListingDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(wID == IDOK) {
		TCHAR buf[512];
		if (ctrlSearch.GetWindowTextLength() == 0) {
			MessageBox(CTSTRING(LINE_EMPTY));
			return 0;
		}

		sizeMode = ctrlSizeMode.GetCurSel();

		searchStr = WinUtil::addHistory(ctrlSearch, SettingsManager::HISTORY_SEARCH);

		ctrlSize.GetWindowText(buf, 512);
		double lsize = Util::toDouble(Text::fromT(buf));
		switch(ctrlSizeUnit.GetCurSel()) {
			case 1:
				lsize*=1024.0; break;
			case 2:
				lsize*=1024.0*1024.0; break;
			case 3:
				lsize*=1024.0*1024.0*1024.0; break;
		}

		useCurDir = IsDlgButtonChecked(IDC_USE_CUR_DIR) == BST_CHECKED;

		size = lsize;

		SearchManager::getInstance()->getSearchType(ctrlFileType.GetCurSel(), fileType, extList, fileTypeStr);
		SettingsManager::getInstance()->set(SettingsManager::LAST_FL_FILETYPE, fileTypeStr);
	}
	EndDialog(wID);
	return 0;
}

LRESULT DirectoryListingDlg::onTypeChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	//extList.clear();
	//SearchManager::getInstance()->getSearchType(ctrlFileType.GetCurSel(), fileType, extList, fileTypeStr);
	//fixControls();
	return 0;
}

LRESULT DirectoryListingDlg::onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlSearch.SetFocus();
	return FALSE;
}