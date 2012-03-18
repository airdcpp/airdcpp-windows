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
#include <commctrl.h>
//#include "../client/SearchManager.h"

#define GET_TEXT(id, var) \
	GetDlgItemText(id, buf, 1024); \
	var = Text::fromT(buf);

#define ATTACH(id, var) var.Attach(GetDlgItem(id))

SearchPageDlg::SearchPageDlg() : fileType(0), action(0), matcherType(0), searchInterval(0), remove(false), targetType(0), startTime(0,0), endTime(23, 59), searchDays("1111111") {
}

SearchPageDlg::~SearchPageDlg() {
	ctrlSearch.Detach();
	ctrlFileType.Detach();
	cAction.Detach();
	ftImage.Destroy();
	ctrlTarget.Detach();
	ctrlUserMatch.Detach();
	cMatcherType.Detach();
}

LRESULT SearchPageDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	ATTACH(IDC_AS_SEARCH_STRING, ctrlSearch);
	ATTACH(IDC_TARGET_PATH, ctrlTarget);
	ATTACH(IDC_U_MATCH, ctrlUserMatch);
	ATTACH(IDC_MATCHER_PATTERN, ctrlMatcherString);

	ctrlSearch.SetWindowText(Text::toT(searchString).c_str());
	ctrlCheatingDescription.SetWindowText(Text::toT(comment).c_str());

	updateTargetText();
	ctrlUserMatch.SetWindowText(Text::toT(userMatch).c_str());

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
	::SetWindowText(GetDlgItem(IDC_SELECT_DIR), TSTRING(SELECT_DIRECTORY).c_str());

	::SetWindowText(GetDlgItem(IDC_USER_MATCH_TEXT), TSTRING(AS_USER_MATCH).c_str());

	::SetWindowText(GetDlgItem(IDC_USE_MATCHER), CTSTRING(USE_CUSTOM_MATCHER));
	::SetWindowText(GetDlgItem(IDC_PATTERN), CTSTRING(PATTERN));
	::SetWindowText(GetDlgItem(IDC_TYPE), CTSTRING(TYPE));

	::SetWindowText(GetDlgItem(IDC_USE_EXPIRY), CTSTRING(USE_EXPIRY_DAY));

	::SetWindowText(GetDlgItem(IDC_CUSTOM_SEARCH_TIMES), CTSTRING(CUSTOM_SEARCH_TIMES));
	::SetWindowText(GetDlgItem(IDC_MON), CTSTRING(MONDAY));
	::SetWindowText(GetDlgItem(IDC_TUE), CTSTRING(TUESDAY));
	::SetWindowText(GetDlgItem(IDC_WED), CTSTRING(WEDNESDAY));
	::SetWindowText(GetDlgItem(IDC_THU), CTSTRING(THURSDAY));
	::SetWindowText(GetDlgItem(IDC_FRI), CTSTRING(FRIDAY));
	::SetWindowText(GetDlgItem(IDC_SAT), CTSTRING(SATURDAY));
	::SetWindowText(GetDlgItem(IDC_SUN), CTSTRING(SUNDAY));
	::SetWindowText(GetDlgItem(IDC_SAT), CTSTRING(SATURDAY));
	::SetWindowText(GetDlgItem(IDC_SUN), CTSTRING(SUNDAY));
	::SetWindowText(GetDlgItem(IDC_START_TIME), CTSTRING(START_TIME));
	::SetWindowText(GetDlgItem(IDC_END_TIME), CTSTRING(END_TIME));

	int q = 0;
	for(size_t i = 0; i < 9; i++) {
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

	ATTACH(IDC_MATCHER_TYPE, cMatcherType);
	cMatcherType.AddString(CTSTRING(PLAIN_TEXT));
	cMatcherType.AddString(CTSTRING(REGEXP));
	cMatcherType.AddString(CTSTRING(WILDCARDS));
	cMatcherType.SetCurSel(matcherType);

	CheckDlgButton(IDC_REMOVE_ON_HIT, remove);

	if (!(matcherString == searchString && matcherType == 0)) {
		ctrlMatcherString.SetWindowText(Text::toT(matcherString).c_str());
		CheckDlgButton(IDC_USE_MATCHER, true);
	}

	CenterWindow(GetParent());
	SetWindowText(CTSTRING(AUTOSEARCH_DLG));

	ATTACH(IDC_DATETIMEPICKER, ctrlExpire);
	if (expireTime > 0) {
		SYSTEMTIME time;
		WinUtil::toSystemTime(expireTime, &time);
		ctrlExpire.SetSystemTime(0, &time);
		CheckDlgButton(IDC_USE_EXPIRY, true);
	}

	CheckDlgButton(IDC_MON, searchDays[1]);
	CheckDlgButton(IDC_TUE, searchDays[2]);
	CheckDlgButton(IDC_WED, searchDays[3]);
	CheckDlgButton(IDC_THU, searchDays[4]);
	CheckDlgButton(IDC_FRI, searchDays[5]);
	CheckDlgButton(IDC_SAT, searchDays[6]);
	CheckDlgButton(IDC_SUN, searchDays[0]);

	ATTACH(IDC_SEARCH_START, ctrlSearchStart);
	ATTACH(IDC_SEARCH_END, ctrlSearchEnd);
	ctrlSearchStart.SetFormat(_T("HH:mm"));
	ctrlSearchEnd.SetFormat(_T("HH:mm"));

	if (!((int8_t(searchDays.count()) == 7) && startTime.minute == 0 && startTime.hour == 0 && endTime.minute == 59 && endTime.hour == 23)) {
		CheckDlgButton(IDC_CUSTOM_SEARCH_TIMES, true);
	} else {
		CheckDlgButton(IDC_CUSTOM_SEARCH_TIMES, false);
	}

	SYSTEMTIME s;
	GetSystemTime(&s);
	s.wHour = startTime.hour;
	s.wMinute = startTime.minute;
	ctrlSearchStart.SetSystemTime(0, &s);

	SYSTEMTIME e;
	GetSystemTime(&e);
	e.wHour = endTime.hour;
	e.wMinute = endTime.minute;
	ctrlSearchEnd.SetSystemTime(0, &e);

	/*CButton tmp1;
	ATTACH(IDC_BROWSE, tmp1);
	tmp1.SetButtonStyle(BS_SPLITBUTTON);
	BUTTON_SPLITINFO split;
	split.uSplitStyle = BCSS_STRETCH; */
	//tmp1.SetSplitInfo(

	fixControls();
	return TRUE;
}

LRESULT SearchPageDlg::onClickLocation(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	POINT pt;
	if(GetCursorPos(&pt) != 0) {

		if (targetMenu.m_hMenu != NULL) {
			// delete target menu
			targetMenu.DestroyMenu();
			targetMenu.m_hMenu = NULL;
		}

		targetMenu.CreatePopupMenu();
		targetMenu.InsertSeparatorFirst(CTSTRING(DOWNLOAD_TO));
		WinUtil::appendDirsMenu(targetMenu);

		/*MENUINFO inf;
		inf.cbSize = sizeof(MENUINFO);
		inf.fMask = MIM_STYLE;
		inf.dwStyle = MNS_NOTIFYBYPOS;
		locations.SetMenuInfo(&inf);
		targetMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_HORPOSANIMATION | TPM_VERPOSANIMATION,
			pt.x, pt.y, m_hWnd);*/
		targetMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	}
	return 0;
}

LRESULT SearchPageDlg::onDownloadFavoriteDirs(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (WinUtil::getVirtualTarget(wID, target, targetType)) {
		updateTargetText();
	}
	return 0;
}

void SearchPageDlg::updateTargetText() {
	string targetAdd;
	if (targetType > 0) {
		targetAdd = " (";
		if (targetType == AutoSearch::TARGET_FAVORITE)
			targetAdd += "Favorite";
		else if (targetType == AutoSearch::TARGET_SHARE)
			targetAdd += "Shared";
		targetAdd += ")";
	}

	ctrlTarget.SetWindowText(Text::toT(target+targetAdd).c_str());
}

LRESULT SearchPageDlg::onBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TCHAR buf[MAX_PATH];

	GetDlgItemText(IDC_TARGET_PATH, buf, MAX_PATH);
	tstring x = buf;

	if(WinUtil::browseDirectory(x, m_hWnd) == IDOK) {
		SetDlgItemText(IDC_TARGET_PATH, x.c_str());
		targetType = 0;
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

		if (targetType == 0) {
			GetDlgItemText(IDC_TARGET_PATH, bufPath, MAX_PATH);
			target = Text::fromT(bufPath);
		}

		auto useDefaultMatcher = IsDlgButtonChecked(IDC_USE_MATCHER) != BST_CHECKED;
		if (useDefaultMatcher) {
			matcherString = Util::emptyString;
			matcherType = 0;
		} else {
			GetDlgItemText(IDC_MATCHER_PATTERN, buf, 512);
			matcherString = Text::fromT(buf);
			matcherType = cMatcherType.GetCurSel();
		}

		if (IsDlgButtonChecked(IDC_USE_EXPIRY) == BST_CHECKED) {
			SYSTEMTIME exp;
			ctrlExpire.GetSystemTime(&exp);
			expireTime = WinUtil::fromSystemTime(&exp);
		} else {
			expireTime = 0;
		}

		bool useDefaultTimes = IsDlgButtonChecked(IDC_CUSTOM_SEARCH_TIMES) != BST_CHECKED;
		searchDays[1] = (IsDlgButtonChecked(IDC_MON) == BST_CHECKED || useDefaultTimes);
		searchDays[2] = (IsDlgButtonChecked(IDC_TUE) == BST_CHECKED || useDefaultTimes);
		searchDays[3] = (IsDlgButtonChecked(IDC_WED) == BST_CHECKED || useDefaultTimes);
		searchDays[4] = (IsDlgButtonChecked(IDC_THU) == BST_CHECKED || useDefaultTimes);
		searchDays[5] = (IsDlgButtonChecked(IDC_FRI) == BST_CHECKED || useDefaultTimes);
		searchDays[6] = (IsDlgButtonChecked(IDC_SAT) == BST_CHECKED || useDefaultTimes);
		searchDays[0] = (IsDlgButtonChecked(IDC_SUN) == BST_CHECKED || useDefaultTimes);

		if (useDefaultTimes) {
			startTime.hour = 0;
			startTime.minute = 0;
			endTime.hour = 23;
			endTime.minute = 59;
		} else {
			SYSTEMTIME s;
			ctrlSearchStart.GetSystemTime(&s);
			startTime.hour = s.wHour;
			startTime.minute = s.wMinute;

			SYSTEMTIME e;
			ctrlSearchEnd.GetSystemTime(&e);
			endTime.hour = e.wHour;
			endTime.minute = e.wMinute;
		}

		GetDlgItemText(IDC_U_MATCH, buf, 512);
		userMatch = Text::fromT(buf);
	}
	EndDialog(wID);
	return 0;
}

LRESULT SearchPageDlg::onTargetChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
	if (targetType > 0) {
		//don't allow changing fav/share dir directly

		tstring path;
		path.resize(1024);
		path.resize(GetDlgItemText(wID, &path[0], 1024));

		CEdit tmp;
		tmp.Attach(hWndCtl);
		DWORD dwSel;
		if ((dwSel = tmp.GetSel()) != CB_ERR) {
			auto it = path.begin() +  HIWORD(dwSel)-1;
			path.erase(it);
			tmp.SetSel(0,-1);
			tmp.SetWindowText(path.c_str());
			tmp.SetSel(HIWORD(dwSel)-1, HIWORD(dwSel)-1);
			tmp.Detach();
		}
		return 1;
	}
	return 0;
}

LRESULT SearchPageDlg::onAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

LRESULT SearchPageDlg::onCheckMatcher(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

LRESULT SearchPageDlg::onCheckTimes(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

LRESULT SearchPageDlg::onCheckExpiry(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

void SearchPageDlg::fixControls() {
	/* File type */
	BOOL isTTH = ctrlFileType.GetCurSel() == 8;
	if (isTTH) {
		CheckDlgButton(IDC_USE_MATCHER, false);
		::EnableWindow(GetDlgItem(IDC_USE_MATCHER), false);
	} else {
		::EnableWindow(GetDlgItem(IDC_USE_MATCHER),	true);
	}

	/* Action */
	BOOL isReportOnly = cAction.GetCurSel() == AutoSearch::ACTION_REPORT;
	::EnableWindow(GetDlgItem(IDC_TARGET_PATH),				!isReportOnly);
	::EnableWindow(GetDlgItem(IDC_SELECT_DIR),				!isReportOnly);

	/* Matcher */
	BOOL enableMatcher = IsDlgButtonChecked(IDC_USE_MATCHER) == BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_PATTERN),					enableMatcher);
	::EnableWindow(GetDlgItem(IDC_MATCHER_PATTERN),			enableMatcher);
	::EnableWindow(GetDlgItem(IDC_TYPE),					enableMatcher);
	::EnableWindow(GetDlgItem(IDC_MATCHER_TYPE),			enableMatcher);

	/* Expiry date */
	::EnableWindow(GetDlgItem(IDC_DATETIMEPICKER),			IsDlgButtonChecked(IDC_USE_EXPIRY) == BST_CHECKED);

	/* Search times */
	BOOL useCustomTimes = IsDlgButtonChecked(IDC_CUSTOM_SEARCH_TIMES) == BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_MON),				useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_TUE),				useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_WED),				useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_THU),				useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_FRI),				useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_SAT),				useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_SUN),				useCustomTimes);

	::EnableWindow(GetDlgItem(IDC_SEARCH_START),	useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_SEARCH_END),		useCustomTimes);
}

