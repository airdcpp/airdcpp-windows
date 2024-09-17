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

#include "stdafx.h"
#include "Resource.h"
#include "ActionUtil.h"
#include "WinUtil.h"
#include "DirectoryListingDlg.h"

#include <airdcpp/SearchManager.h>
#include <airdcpp/SearchTypes.h>
#include <airdcpp/ResourceManager.h>


namespace wingui {
#define GET_TEXT(id, var) \
	GetDlgItemText(id, buf, 1024); \
	var = Text::fromT(buf);

#define ATTACH(id, var) var.Attach(GetDlgItem(id))

DirectoryListingDlg::DirectoryListingDlg(const DirectoryListingPtr& aDl) : fileTypeId(SEARCH_TYPE_ANY), dl(aDl) { }

DirectoryListingDlg::~DirectoryListingDlg() {
}

LRESULT DirectoryListingDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	ATTACH(IDC_SEARCH_STRING, ctrlSearch);
	ctrlSearch.SetFocus();
	ActionUtil::appendHistory(ctrlSearch, SettingsManager::HISTORY_SEARCH);

	ATTACH(IDC_FILETYPES, ctrlFileType);
	ATTACH(IDC_SEARCH_SIZE, ctrlSize);

	ATTACH(IDC_SIZE_MODE, ctrlSizeMode);
	ATTACH(IDC_SIZE_UNIT, ctrlSizeUnit);
	ActionUtil::appendSizeCombos(ctrlSizeUnit, ctrlSizeMode);

	::SetWindowText(GetDlgItem(IDCANCEL), CTSTRING(CANCEL));
	::SetWindowText(GetDlgItem(IDC_DL_SEARCH_DESC), CTSTRING(SEARCH_STRING));
	::SetWindowText(GetDlgItem(IDC_DL_TYPE_TEXT), CTSTRING(FILE_TYPE));
	::SetWindowText(GetDlgItem(IDC_SIZE_LABEL), CTSTRING(SIZE));

	::SetWindowText(GetDlgItem(IDC_USE_CUR_DIR), CTSTRING(SEARCH_CUR_DIR));
	if (!dl->supportsASCH() || dl->getHintedUser().user->isNMDC()) {
		::EnableWindow(GetDlgItem(IDC_USE_CUR_DIR),	FALSE);
		::ShowWindow(GetDlgItem(IDC_NO_ASCH_NOTE), SW_SHOW);
		if (dl->getHintedUser().user->isNMDC()) {
			::SetWindowText(GetDlgItem(IDC_NO_ASCH_NOTE), CTSTRING(PARTIAL_SCH_NMDC_NOTE));
		} else {
			::SetWindowText(GetDlgItem(IDC_NO_ASCH_NOTE), CTSTRING(NO_ASCH_NOTE));
		}
	}

	fileTypeId = SETTING(LAST_FL_FILETYPE);
	ctrlFileType.fillList(fileTypeId);


	CenterWindow(GetParent());
	SetWindowText(CTSTRING(SEARCH));

	return FALSE;
}

LRESULT DirectoryListingDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(wID == IDOK) {
		if (ctrlSearch.GetWindowTextLength() == 0) {
			MessageBox(CTSTRING(LINE_EMPTY));
			return 0;
		}

		sizeMode = static_cast<Search::SizeModes>(ctrlSizeMode.GetCurSel());
		searchStr = ActionUtil::addHistory(ctrlSearch, SettingsManager::HISTORY_SEARCH);
		size = WinUtil::parseSize(ctrlSize, ctrlSizeUnit);
		useCurDir = IsDlgButtonChecked(IDC_USE_CUR_DIR) == BST_CHECKED;

		auto& typeManager = SearchManager::getInstance()->getSearchTypes();
		typeManager.getSearchType(ctrlFileType.GetCurSel(), fileType, extList, fileTypeId);
		SettingsManager::getInstance()->set(SettingsManager::LAST_FL_FILETYPE, fileTypeId);
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
}