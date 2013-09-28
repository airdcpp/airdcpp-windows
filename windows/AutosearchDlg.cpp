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
#include "AutoSearchDlg.h"
#include <commctrl.h>
#include "AutosearchParams.h"

//#include "../client/SearchManager.h"

#define GET_TEXT(id, var) \
	GetDlgItemText(id, buf, 1024); \
	var = Text::fromT(buf);

AutoSearchDlg::AutoSearchDlg(const AutoSearchPtr& aAutoSearch) : as(aAutoSearch), fileTypeStr(SEARCH_TYPE_ANY), action(0), matcherType(0), remove(false), targetType(TargetUtil::TARGET_PATH), startTime(0, 0),
	endTime(23, 59), searchDays("1111111"), loading(true), checkQueued(true), checkShared(true), searchType(0), advanced(true), matchFullPath(false), curNumber(1), maxNumber(0),
	numberLen(2), useParams(false) { }

AutoSearchDlg::~AutoSearchDlg() { }

LRESULT AutoSearchDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ATTACH(IDC_AS_SEARCH_STRING, ctrlSearch);
	ATTACH(IDC_TARGET_PATH, ctrlTarget);
	ATTACH(IDC_TARGET_TYPE, cTargetType);
	ATTACH(IDC_U_MATCH, ctrlUserMatch);
	ATTACH(IDC_MATCHER_PATTERN, ctrlMatcherString);
	ATTACH(IDC_ADVANCED, cAdvanced);

	ctrlSearch.SetWindowText(Text::toT(searchString).c_str());

	ctrlTarget.SetWindowText(Text::toT(target).c_str());
	updateTargetTypeText();
	ctrlUserMatch.SetWindowText(Text::toT(userMatch).c_str());

	ATTACH(IDC_AS_FILETYPE, ctrlFileType);	
	::SetWindowText(GetDlgItem(IDCANCEL), (TSTRING(CANCEL)).c_str());
	::SetWindowText(GetDlgItem(IDC_SEARCH_FAKE_DLG_SEARCH_STRING), (TSTRING(SEARCH_STRING)).c_str());
	::SetWindowText(GetDlgItem(IDC_AS_ACTION_STATIC), (TSTRING(ACTION)).c_str());
	::SetWindowText(GetDlgItem(IDC_ADD_SRCH_STR_TYPE_STATIC), (TSTRING(FILE_TYPE)).c_str());
	::SetWindowText(GetDlgItem(IDC_REMOVE_AFTER_COMPLETED), (TSTRING(REMOVE_AFTER_COMPLETED)).c_str());

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
	::SetWindowText(GetDlgItem(IDC_CHECK_QUEUED), CTSTRING(AUTOSEARCH_CHECK_QUEUED));
	::SetWindowText(GetDlgItem(IDC_CHECK_SHARED), CTSTRING(AUTOSEARCH_CHECK_SHARED));
	::SetWindowText(GetDlgItem(IDC_MATCH_FULL_PATH), CTSTRING(MATCH_FULL_PATH));
	::SetWindowText(GetDlgItem(IDC_AS_EXCLUDED_LABEL), CTSTRING(EXCLUDED_WORDS_DESC));

	::SetWindowText(GetDlgItem(IDC_SEARCH_TIMES_LABEL), CTSTRING(SEARCH_TIMES));
	::SetWindowText(GetDlgItem(IDC_ADVANCED_LABEL), CTSTRING(SETTINGS_ADVANCED));
	::SetWindowText(GetDlgItem(IDC_EXACT_MATCH), CTSTRING(REQUIRE_EXACT_MATCH));

	::SetWindowText(GetDlgItem(IDC_CONF_PARAMS), Text::toT(STRING(CONFIGURE) + "...").c_str());
	::SetWindowText(GetDlgItem(IDC_USE_PARAMS), CTSTRING(ENABLE_PARAMETERS));

	//get the search type so that we can set the initial control states correctly in fixControls
	StringList ext;
	try {
		SearchManager::getInstance()->getSearchType(fileTypeStr, searchType, ext);
	} catch (...) {
		//switch back to default
		searchType = SearchManager::TYPE_ANY;
	}

	ctrlFileType.fillList(fileTypeStr);

	ATTACH(IDC_AS_ACTION, cAction);
	cAction.AddString(CTSTRING(DOWNLOAD));
	cAction.AddString(CTSTRING(ADD_TO_QUEUE));
	cAction.AddString(CTSTRING(AS_REPORT));
	cAction.SetCurSel(action);

	ATTACH(IDC_MATCHER_TYPE, cMatcherType);
	cMatcherType.AddString(CTSTRING(PLAIN_TEXT));
	cMatcherType.AddString(CTSTRING(REGEXP));
	cMatcherType.AddString(CTSTRING(WILDCARDS));


	if (matcherType == StringMatch::EXACT) {
		CheckDlgButton(IDC_EXACT_MATCH, true);
		cMatcherType.SetCurSel(0);
	} else {
		cMatcherType.SetCurSel(matcherType);
	}

	ATTACH(IDC_AS_EXCLUDED, cExcludedWords);
	WinUtil::appendHistory(cExcludedWords, SettingsManager::HISTORY_EXCLUDE);
	cExcludedWords.SetWindowText(Text::toT(excludedWords).c_str());


	CheckDlgButton(IDC_REMOVE_AFTER_COMPLETED, remove);
	CheckDlgButton(IDC_CHECK_QUEUED, checkQueued);
	CheckDlgButton(IDC_CHECK_SHARED, checkShared);
	CheckDlgButton(IDC_MATCH_FULL_PATH, matchFullPath);

	CheckDlgButton(IDC_USE_PARAMS, useParams);

	if (matcherString != searchString) {
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

	ATTACH(IDC_SELECT_DIR, cSelectDir);

	fixControls();
	loading = false; //loading done.
	switchMode();
	return TRUE;
}

LRESULT AutoSearchDlg::onExitMenuLoop(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	cSelectDir.SetState(false);
	return 0;
}

LRESULT AutoSearchDlg::onClickAdvanced(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	switchMode();
	return 0;
}

LRESULT AutoSearchDlg::onConfigParams(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	AutoSearchParams dlg(this);
	if(dlg.DoModal() == IDOK) {

	}
	return 0;
}

void AutoSearchDlg::switchMode() {
	advanced = !advanced;

	//users shouldn't be able to change the hidden options with the keyboard command
	fixControls();

	auto adjustWindowSize = [](HWND m_hWnd, int exceptedCurX, int exceptedCurY, int& newX, int& newY) -> void {
		//get the border widths so it's being sized correctly on different operating systems
		CRect rc;
		DWORD dwStyle = ::GetWindowLongPtr(m_hWnd, GWL_STYLE);
		AdjustWindowRect(rc, dwStyle, FALSE);

		//get the current window rect (it varies depending on the font size)
		CRect rcCur;
		::GetClientRect(m_hWnd, &rcCur);

		//get the conversion factors
		auto dpiFactorX = static_cast<float>(rcCur.right) / exceptedCurX;
		auto dpiFactorY = static_cast<float>(rcCur.bottom) / exceptedCurY;

		//calculate the new size
		newX = (newX * dpiFactorX) + abs(rc.left) + abs(rc.right);
		newY = (newY * dpiFactorY) + abs(rc.top) + abs(rc.bottom);
	};

	int newX = 584;
	int newY = (advanced ? 489.0 : 250.0);
	adjustWindowSize(m_hWnd, 584, (!advanced ? 489.0 : 250.0), newX, newY);

	SetWindowPos(m_hWnd,0, 0, newX, newY, SWP_NOZORDER|SWP_NOMOVE);
	cAdvanced.SetWindowText(Text::toT(STRING(SETTINGS_ADVANCED) + (advanced ? " <<" : " >>")).c_str());
}

LRESULT AutoSearchDlg::onClickLocation(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CRect rect;
	cSelectDir.GetWindowRect(rect);
	auto pt = rect.BottomRight();
	pt.x = pt.x-rect.Width();
	cSelectDir.SetState(true);

	OMenu targetMenu;
	targetMenu.CreatePopupMenu();
	targetMenu.InsertSeparatorFirst(CTSTRING(DOWNLOAD_TO));
	//appendDownloadMenu(targetMenu, false, true);
	appendDownloadTo(targetMenu, false, true, nullptr, nullptr);

	targetMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERPOSANIMATION, pt);
	return 0;
}

void AutoSearchDlg::handleDownload(const string& aTarget, QueueItemBase::Priority /*p*/, bool /*useWhole*/, TargetUtil::TargetType aTargetType, bool /*isSizeUnknown*/) {
	target = aTarget;
	ctrlTarget.SetWindowTextW(Text::toT(target).c_str());
	//update the type only after setting the text
	targetType = aTargetType;
	updateTargetTypeText();
}

void AutoSearchDlg::updateTargetTypeText() {
	tstring targetText = TSTRING(LOCATION_TYPE) + _T(": ");

	TCHAR bufPath[MAX_PATH];
	GetDlgItemText(IDC_TARGET_PATH, bufPath, MAX_PATH);

	if (Text::fromT(bufPath).empty()) {
		targetText += TSTRING(SETTINGS_DOWNLOAD_DIRECTORY);
	} else {
		if (targetType == TargetUtil::TARGET_PATH)
			targetText += TSTRING(TYPE_TARGET_PATH);
		else if (targetType == TargetUtil::TARGET_FAVORITE)
			targetText += TSTRING(TYPE_TARGET_FAVORITE);
		else if (targetType == TargetUtil::TARGET_SHARE)
			targetText += TSTRING(TYPE_TARGET_SHARE);
	}

	cTargetType.SetWindowText(targetText.c_str());
}


void AutoSearchDlg::insertNumber() {
	tstring str;
	tstring insert = _T("%[inc]");
	str.resize(ctrlSearch.GetWindowTextLength()+1);
	str.resize(GetDlgItemText(IDC_AS_SEARCH_STRING, &str[0], ctrlSearch.GetWindowTextLength()+1+insert.size()));

	int lSelBegin = 0, lSelEnd = 0;
	ctrlSearch.GetSel(lSelBegin, lSelEnd);

	str.insert(lSelBegin, insert);
	//str.insert()
	//str += _T("%[inc]");
	ctrlSearch.SetWindowText(str.c_str());
}

LRESULT AutoSearchDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(wID == IDOK) {
		TCHAR buf[512];
		TCHAR bufPath[MAX_PATH];
		if (ctrlSearch.GetWindowTextLength() == 0) {
			MessageBox(CTSTRING(LINE_EMPTY));
			return 0;
		}
		
		tstring str;
		str.resize(ctrlSearch.GetWindowTextLength()+1);
		str.resize(GetDlgItemText(IDC_AS_SEARCH_STRING, &str[0], ctrlSearch.GetWindowTextLength()+1));
		searchString = Text::fromT(str);

		if (AutoSearchManager::getInstance()->hasNameDupe(searchString, false, as)) {
			auto msg = str + _T(": ") + TSTRING(ITEM_NAME_EXISTS) + _T("\r\n\r\n") + TSTRING(AS_ADD_DUPE_CONFIRM);
			if (MessageBox(msg.c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES)
				return 0;
		}

		action = cAction.GetCurSel();
		remove = IsDlgButtonChecked(IDC_REMOVE_AFTER_COMPLETED) ? true : false;
		checkQueued = IsDlgButtonChecked(IDC_CHECK_QUEUED) ? true : false;
		checkShared = IsDlgButtonChecked(IDC_CHECK_SHARED) ? true : false;
		matchFullPath = IsDlgButtonChecked(IDC_MATCH_FULL_PATH) ? true : false;

		useParams = IsDlgButtonChecked(IDC_USE_PARAMS) ? true : false;

		if (targetType == 0) {
			GetDlgItemText(IDC_TARGET_PATH, bufPath, MAX_PATH);
			target = Text::fromT(bufPath);
		}

		auto useDefaultMatcher = IsDlgButtonChecked(IDC_USE_MATCHER) != BST_CHECKED;
		auto exactMatch = IsDlgButtonChecked(IDC_EXACT_MATCH) == BST_CHECKED;

		if (useDefaultMatcher || exactMatch) {
			matcherString = Util::emptyString;
			matcherType = exactMatch ? StringMatch::EXACT : StringMatch::PARTIAL;
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

		excludedWords = WinUtil::addHistory(cExcludedWords, SettingsManager::HISTORY_EXCLUDE);

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

			if (endTime.hour < startTime.hour || (endTime.hour == startTime.hour && endTime.minute <= startTime.minute)) {
				MessageBox(_T("End time must be greater than the start time!"));
				return 0;
			}
		}

		GetDlgItemText(IDC_U_MATCH, buf, 512);
		userMatch = Text::fromT(buf);
	}
	EndDialog(wID);
	return 0;
}

LRESULT AutoSearchDlg::onTargetChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (!loading) {
		targetType = TargetUtil::TARGET_PATH;
		updateTargetTypeText();
	}
	return 0;
}

LRESULT AutoSearchDlg::onAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

LRESULT AutoSearchDlg::onCheckMatcher(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

LRESULT AutoSearchDlg::onCheckTimes(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

LRESULT AutoSearchDlg::onCheckExpiry(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

LRESULT AutoSearchDlg::onCheckParams(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

LRESULT AutoSearchDlg::onExactMatch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

LRESULT AutoSearchDlg::onTypeChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	StringList extensions;
	try {
		SearchManager::getInstance()->getSearchType(ctrlFileType.GetCurSel(), searchType, extensions, fileTypeStr);
	} catch (...) { }
	fixControls();
	return 0;
}

void AutoSearchDlg::fixControls() {
	/* File type */
	if (searchType == SearchManager::TYPE_TTH)
		CheckDlgButton(IDC_USE_MATCHER, false);


	/* Action */
	BOOL isReportOnly = cAction.GetCurSel() == AutoSearch::ACTION_REPORT;
	::EnableWindow(GetDlgItem(IDC_TARGET_PATH),				!isReportOnly);
	::EnableWindow(GetDlgItem(IDC_SELECT_DIR),				!isReportOnly);


	/* Misc advanced */
	::EnableWindow(GetDlgItem(IDC_U_MATCH), advanced);
	::EnableWindow(GetDlgItem(IDC_CHECK_QUEUED), searchType == SearchManager::TYPE_DIRECTORY && advanced);
	::EnableWindow(GetDlgItem(IDC_CHECK_SHARED), searchType == SearchManager::TYPE_DIRECTORY && advanced);

	/* Param config */
	bool usingParams = IsDlgButtonChecked(IDC_USE_PARAMS) == BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_CONF_PARAMS), usingParams);

	/* Result matcher */
	bool exactMatch = IsDlgButtonChecked(IDC_EXACT_MATCH) == BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_USE_MATCHER), searchType != SearchManager::TYPE_TTH && advanced && !exactMatch);

	BOOL matcherEnabled = (IsDlgButtonChecked(IDC_USE_MATCHER) == BST_CHECKED && advanced && !exactMatch);
	::EnableWindow(GetDlgItem(IDC_PATTERN),					matcherEnabled);
	::EnableWindow(GetDlgItem(IDC_MATCHER_PATTERN),			matcherEnabled);
	::EnableWindow(GetDlgItem(IDC_TYPE),					matcherEnabled);
	::EnableWindow(GetDlgItem(IDC_MATCHER_TYPE),			matcherEnabled);

	/* Expiry date */
	::EnableWindow(GetDlgItem(IDC_DATETIMEPICKER),			IsDlgButtonChecked(IDC_USE_EXPIRY) == BST_CHECKED);

	/* Search times */
	::EnableWindow(GetDlgItem(IDC_CUSTOM_SEARCH_TIMES),			advanced);

	BOOL useCustomTimes = (IsDlgButtonChecked(IDC_CUSTOM_SEARCH_TIMES) == BST_CHECKED && advanced);
	::EnableWindow(GetDlgItem(IDC_MON),				useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_TUE),				useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_WED),				useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_THU),				useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_FRI),				useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_SAT),				useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_SUN),				useCustomTimes);

	::EnableWindow(GetDlgItem(IDC_START_TIME),		useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_SEARCH_START),	useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_END_TIME),		useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_SEARCH_END),		useCustomTimes);
}

