/* 
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
#include "../client/DCPlusPlus.h"
#include "Resource.h"
#include "WinUtil.h"
#include "AutoSearchDlg.h"

#define GET_TEXT(id, var) \
	GetDlgItemText(id, buf, 1024); \
	var = Text::fromT(buf);

#define ATTACH(id, var) var.Attach(GetDlgItem(id))

SearchPageDlg::SearchPageDlg() : fileType(0), action(0), matcherType(0), searchInterval(0), remove(false), targetType(0) {
}

SearchPageDlg::~SearchPageDlg() {
	ctrlSearch.Detach();
	ctrlFileType.Detach();
	cAction.Detach();
	ftImage.Destroy();
	ctrlTarget.Detach();
	ctrlMatch.Detach();

}

LRESULT SearchPageDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	ATTACH(IDC_AS_SEARCH_STRING, ctrlSearch);
	ATTACH(IDC_TARGET_PATH, ctrlTarget);
	ATTACH(IDC_U_MATCH, ctrlMatch);

	ctrlSearch.SetWindowText(Text::toT(searchString).c_str());
	ctrlCheatingDescription.SetWindowText(Text::toT(comment).c_str());
	ctrlTarget.SetWindowText(Text::toT(target).c_str());
	ctrlMatch.SetWindowText(Text::toT(userMatch).c_str());

	ATTACH(IDC_AS_FILETYPE, ctrlFileType);
	ftImage.CreateFromImage(IDB_SEARCH_TYPES, 16, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
    ctrlFileType.SetImageList(ftImage);	
	::SetWindowText(GetDlgItem(IDOK), (TSTRING(OK)).c_str());
	::SetWindowText(GetDlgItem(IDCANCEL), (TSTRING(CANCEL)).c_str());
	::SetWindowText(GetDlgItem(IDC_SEARCH_FAKE_DLG_SEARCH_STRING), (TSTRING(SEARCH_STRING)).c_str());
	::SetWindowText(GetDlgItem(IDC_AS_ACTION_STATIC), (TSTRING(ACTION)).c_str());
	::SetWindowText(GetDlgItem(IDC_ADD_SRCH_STR_TYPE_STATIC), (TSTRING(FILE_TYPE)).c_str());
	::SetWindowText(GetDlgItem(IDC_REMOVE_ON_HIT), (TSTRING(REMOVE_ON_HIT)).c_str());
	::SetWindowText(GetDlgItem(IDC_DL_TO), TSTRING(DOWNLOAD_TO).c_str());
	::SetWindowText(GetDlgItem(IDC_USER_MATCH_TEXT), TSTRING(AS_USER_MATCH).c_str());

	int q = 0;
	for(size_t i = 0; i < 10; i++) {
		COMBOBOXEXITEM cbitem = {CBEIF_TEXT|CBEIF_IMAGE|CBEIF_SELECTEDIMAGE};
		tstring ftString;
		switch(i) {
			case 0: q = 0; ftString = TSTRING(ANY); break;
			case 1: q = 1; ftString = TSTRING(AUDIO); break;
			case 2: q = 2; ftString = TSTRING(COMPRESSED); break;
			case 3: q = 3; ftString = TSTRING(DOCUMENT); break;
			case 4: q = 4; ftString = TSTRING(EXECUTABLE); break;
			case 5: q = 5; ftString = TSTRING(PICTURE); break;
			case 6: q = 6; ftString = TSTRING(VIDEO); break;
			case 7: q = 7; ftString = TSTRING(DIRECTORY); break;
			case 8: q = 8; ftString = _T("TTH"); break;
		}
		cbitem.pszText = const_cast<TCHAR*>(ftString.c_str());
		cbitem.iItem = i; 
		cbitem.iImage = q;
		cbitem.iSelectedImage = q;
		ctrlFileType.InsertItem(&cbitem);
	}
	ctrlFileType.SetCurSel(fileType);



	ATTACH(IDC_AS_ACTION, cAction);
	cAction.AddString(CTSTRING(DOWNLOAD));
	cAction.AddString(CTSTRING(ADD_TO_QUEUE));
	cAction.AddString(CTSTRING(AS_REPORT));
	cAction.SetCurSel(action);

	CheckDlgButton(IDC_REMOVE_ON_HIT, remove);

	CenterWindow(GetParent());
	SetWindowText(CTSTRING(AUTOSEARCH_DLG));
	return TRUE;
}

LRESULT SearchPageDlg::onBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TCHAR buf[MAX_PATH];

	GetDlgItemText(IDC_TARGET_PATH, buf, MAX_PATH);
	tstring x = buf;

	if(WinUtil::browseDirectory(x, m_hWnd) == IDOK) {
		SetDlgItemText(IDC_TARGET_PATH, x.c_str());
	}
	return 0;
}

LRESULT SearchPageDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(wID == IDOK) {
		TCHAR buf[512];
		TCHAR bufPath[MAX_PATH];
		if (ctrlSearch.GetWindowTextLength() == 0) {
			MessageBox(CTSTRING(LINE_EMPTY));
			return 0;
		}
		GetDlgItemText(IDC_AS_SEARCH_STRING, buf, 512);
		searchString = Text::fromT(buf);
		fileType = ctrlFileType.GetCurSel();
		action = cAction.GetCurSel();
		remove = IsDlgButtonChecked(IDC_REMOVE_ON_HIT) ? true : false;
		GetDlgItemText(IDC_TARGET_PATH, bufPath, MAX_PATH);
		target = Text::fromT(bufPath);
		
		GetDlgItemText(IDC_U_MATCH, buf, 512);
		userMatch = Text::fromT(buf);
	}
	EndDialog(wID);
	return 0;
}
LRESULT SearchPageDlg::onAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	return 0;
}

