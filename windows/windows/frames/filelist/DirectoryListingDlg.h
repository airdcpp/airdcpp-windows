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

#ifndef DIRLIST_SEARCH_DLG_H
#define DIRLIST_SEARCH_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <atlcrack.h>

#include <windows/components/SearchTypeCombo.h>

#include <airdcpp/filelist/DirectoryListing.h>
#include <airdcpp/search/Search.h>

namespace wingui {
class DirectoryListingDlg : public CDialogImpl<DirectoryListingDlg> {
public:
	string searchStr;
	string fileTypeId;
	Search::TypeModes fileType = Search::TYPE_ANY;
	Search::SizeModes sizeMode = Search::SIZE_DONTCARE;
	int64_t size = 0;
	StringList extList;
	bool useCurDir = false;

	enum { IDD = IDD_DIRLIST_DLG };

	DirectoryListingDlg(const DirectoryListingPtr& aDl);
	~DirectoryListingDlg();

	BEGIN_MSG_MAP_EX(DirectoryListingDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		MESSAGE_HANDLER(WM_DRAWITEM, SearchTypeCombo::onDrawItem)
		MESSAGE_HANDLER(WM_MEASUREITEM, SearchTypeCombo::onMeasureItem)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_FILETYPES, onTypeChanged)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onTypeChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
private:
	CComboBox ctrlSearch;
	SearchTypeCombo ctrlFileType;
	CEdit ctrlSize;
	CComboBox ctrlSizeMode;
	CComboBox ctrlSizeUnit;
	DirectoryListingPtr dl;
};
}

#endif