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
#include "WinUtil.h"
#include "AutoSearchDlg.h"
#include "AutosearchParams.h"

#define GET_TEXT(id, var) \
	GetDlgItemText(id, buf, 1024); \
	var = Text::fromT(buf);

AutoSearchGeneralPage::AutoSearchGeneralPage(ItemSettings& aSettings) : options(aSettings) {}

AutoSearchGeneralPage::~AutoSearchGeneralPage() { }

LRESULT AutoSearchGeneralPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ATTACH(IDC_AS_SEARCH_STRING, ctrlSearch);
	ATTACH(IDC_TARGET_PATH, ctrlTarget);
	ATTACH(IDC_TARGET_TYPE, cTargetType);

	ctrlSearch.SetWindowText(Text::toT(options.searchString).c_str());

	ctrlTarget.SetWindowText(Text::toT(options.target).c_str());
	updateTargetTypeText();

	ATTACH(IDC_AS_FILETYPE, ctrlFileType);	
	::SetWindowText(GetDlgItem(IDC_SEARCH_FAKE_DLG_SEARCH_STRING), CTSTRING(SEARCH_STRING));
	::SetWindowText(GetDlgItem(IDC_AS_ACTION_STATIC), CTSTRING(ACTION));
	::SetWindowText(GetDlgItem(IDC_ADD_SRCH_STR_TYPE_STATIC), CTSTRING(FILE_TYPE));
	::SetWindowText(GetDlgItem(IDC_REMOVE_AFTER_COMPLETED), CTSTRING(REMOVE_AFTER_COMPLETED));

	::SetWindowText(GetDlgItem(IDC_DL_TO), CTSTRING(DOWNLOAD_TO));
	::SetWindowText(GetDlgItem(IDC_SELECT_DIR), CTSTRING(SELECT_DIRECTORY));

	::SetWindowText(GetDlgItem(IDC_USE_EXPIRY), CTSTRING(USE_EXPIRY_DAY));

	::SetWindowText(GetDlgItem(IDC_CONF_PARAMS), Text::toT(STRING(CONFIGURE) + "...").c_str());
	::SetWindowText(GetDlgItem(IDC_USE_PARAMS), CTSTRING(ENABLE_PARAMETERS));

	//get the search type so that we can set the initial control states correctly in fixControls
	StringList ext;
	try {
		SearchManager::getInstance()->getSearchType(options.fileTypeStr, options.searchType, ext);
	} catch (...) {
		//switch back to default
		options.searchType = SearchManager::TYPE_ANY;
	}

	ctrlFileType.fillList(options.fileTypeStr);

	ATTACH(IDC_AS_ACTION, cAction);
	cAction.AddString(CTSTRING(DOWNLOAD));
	cAction.AddString(CTSTRING(ADD_TO_QUEUE));
	cAction.AddString(CTSTRING(AS_REPORT));
	cAction.SetCurSel(options.action);

	CheckDlgButton(IDC_REMOVE_AFTER_COMPLETED, options.remove);

	CheckDlgButton(IDC_USE_PARAMS, options.useParams);

	ATTACH(IDC_DATETIMEPICKER, ctrlExpire);
	if (options.expireTime > 0) {
		SYSTEMTIME time;
		WinUtil::toSystemTime(options.expireTime, &time);
		ctrlExpire.SetSystemTime(0, &time);
		CheckDlgButton(IDC_USE_EXPIRY, true);
	}

	ATTACH(IDC_SELECT_DIR, cSelectDir);

	fixControls();
	loading = false; //loading done.
	return TRUE;
}

LRESULT AutoSearchGeneralPage::onExitMenuLoop(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	cSelectDir.SetState(false);
	return 0;
}


LRESULT AutoSearchGeneralPage::onConfigParams(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	AutoSearchParams dlg(this);
	if(dlg.DoModal() == IDOK) {

	}
	return 0;
}

LRESULT AutoSearchGeneralPage::onClickLocation(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CRect rect;
	cSelectDir.GetWindowRect(rect);
	auto pt = rect.BottomRight();
	pt.x = pt.x-rect.Width();
	cSelectDir.SetState(true);

	OMenu targetMenu;
	targetMenu.CreatePopupMenu();
	targetMenu.InsertSeparatorFirst(CTSTRING(DOWNLOAD_TO));
	//appendDownloadMenu(targetMenu, false, true);
	appendDownloadTo(targetMenu, false, true, boost::none, boost::none);

	targetMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERPOSANIMATION, pt);
	return 0;
}

void AutoSearchGeneralPage::handleDownload(const string& aTarget, QueueItemBase::Priority /*p*/, bool /*useWhole*/, TargetUtil::TargetType aTargetType, bool /*isSizeUnknown*/) {
	options.target = aTarget;
	ctrlTarget.SetWindowTextW(Text::toT(options.target).c_str());
	//update the type only after setting the text
	options.targetType = aTargetType;
	updateTargetTypeText();
}

void AutoSearchGeneralPage::updateTargetTypeText() {
	tstring targetText = TSTRING(LOCATION_TYPE) + _T(": ");

	TCHAR bufPath[MAX_PATH];
	GetDlgItemText(IDC_TARGET_PATH, bufPath, MAX_PATH);

	if (Text::fromT(bufPath).empty()) {
		targetText += TSTRING(SETTINGS_DOWNLOAD_DIRECTORY);
	} else {
		if (options.targetType == TargetUtil::TARGET_PATH)
			targetText += TSTRING(TYPE_TARGET_PATH);
		else if (options.targetType == TargetUtil::TARGET_FAVORITE)
			targetText += TSTRING(TYPE_TARGET_FAVORITE);
		else if (options.targetType == TargetUtil::TARGET_SHARE)
			targetText += TSTRING(TYPE_TARGET_SHARE);
	}

	cTargetType.SetWindowText(targetText.c_str());
}


void AutoSearchGeneralPage::insertNumber() {
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

bool AutoSearchGeneralPage::write() {
		TCHAR bufPath[MAX_PATH];
		if (ctrlSearch.GetWindowTextLength() <= 5) {
			MessageBox(CTSTRING(LINE_EMPTY_OR_TOO_SHORT));
			return false;
		}
		
		tstring str;
		str.resize(ctrlSearch.GetWindowTextLength()+1);
		str.resize(GetDlgItemText(IDC_AS_SEARCH_STRING, &str[0], ctrlSearch.GetWindowTextLength()+1));
		options.searchString = Text::fromT(str);

		auto lst = AutoSearchManager::getInstance()->getSearchesByString(options.searchString, options.as);
		if (!lst.empty()) {
			auto msg = str + _T(": ") + TSTRING(ITEM_NAME_EXISTS) + _T("\r\n\r\n") + TSTRING(AS_ADD_DUPE_CONFIRM);
			if (!WinUtil::showQuestionBox(msg, MB_ICONQUESTION))
				return false;
		}

		options.action = cAction.GetCurSel();
		options.remove = IsDlgButtonChecked(IDC_REMOVE_AFTER_COMPLETED) ? true : false;

		options.useParams = IsDlgButtonChecked(IDC_USE_PARAMS) ? true : false;

		if (options.targetType == 0) {
			GetDlgItemText(IDC_TARGET_PATH, bufPath, MAX_PATH);
			options.target = Text::fromT(bufPath);
		}


		if (IsDlgButtonChecked(IDC_USE_EXPIRY) == BST_CHECKED) {
			SYSTEMTIME exp;
			ctrlExpire.GetSystemTime(&exp);
			options.expireTime = WinUtil::fromSystemTime(&exp);
		} else {
			options.expireTime = 0;
		}
		return true;
}

LRESULT AutoSearchGeneralPage::onTargetChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (!loading) {
		options.targetType = TargetUtil::TARGET_PATH;
		updateTargetTypeText();
	}
	return 0;
}

LRESULT AutoSearchGeneralPage::onAction(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

LRESULT AutoSearchGeneralPage::onCheckExpiry(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

LRESULT AutoSearchGeneralPage::onCheckParams(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

LRESULT AutoSearchGeneralPage::onTypeChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	StringList extensions;
	try {
		SearchManager::getInstance()->getSearchType(ctrlFileType.GetCurSel(), options.searchType, extensions, options.fileTypeStr);
	} catch (...) { }
	fixControls();
	return 0;
}

void AutoSearchGeneralPage::fixControls() {

	/* Action */
	BOOL isReportOnly = cAction.GetCurSel() == AutoSearch::ACTION_REPORT;
	::EnableWindow(GetDlgItem(IDC_TARGET_PATH),				!isReportOnly);
	::EnableWindow(GetDlgItem(IDC_SELECT_DIR),				!isReportOnly);


	/* Param config */
	bool usingParams = IsDlgButtonChecked(IDC_USE_PARAMS) == BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_CONF_PARAMS), usingParams);

	/* Result matcher 
	bool exactMatch = IsDlgButtonChecked(IDC_EXACT_MATCH) == BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_USE_MATCHER), searchType != SearchManager::TYPE_TTH && !exactMatch);
	*/

	/* Expiry date */
	::EnableWindow(GetDlgItem(IDC_DATETIMEPICKER),	IsDlgButtonChecked(IDC_USE_EXPIRY) == BST_CHECKED);

}

AutoSearchAdvancedPage::AutoSearchAdvancedPage(ItemSettings& aSettings) : options(aSettings) {}

AutoSearchAdvancedPage::~AutoSearchAdvancedPage() { }

LRESULT AutoSearchAdvancedPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ATTACH(IDC_U_MATCH, ctrlUserMatch);
	ATTACH(IDC_MATCHER_PATTERN, ctrlMatcherString);

	ctrlUserMatch.SetWindowText(Text::toT(options.userMatch).c_str());

	::SetWindowText(GetDlgItem(IDC_USER_MATCH_TEXT), CTSTRING(AS_USER_MATCH));

	::SetWindowText(GetDlgItem(IDC_USE_MATCHER), CTSTRING(USE_CUSTOM_MATCHER));
	::SetWindowText(GetDlgItem(IDC_PATTERN), CTSTRING(PATTERN));
	::SetWindowText(GetDlgItem(IDC_TYPE), CTSTRING(TYPE));

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

	::SetWindowText(GetDlgItem(IDC_EXACT_MATCH), CTSTRING(REQUIRE_EXACT_MATCH));

	ATTACH(IDC_MATCHER_TYPE, cMatcherType);
	cMatcherType.AddString(CTSTRING(PLAIN_TEXT));
	cMatcherType.AddString(CTSTRING(REGEXP));
	cMatcherType.AddString(CTSTRING(WILDCARDS));


	if (options.matcherType == StringMatch::EXACT) {
		CheckDlgButton(IDC_EXACT_MATCH, true);
		cMatcherType.SetCurSel(0);
	}
	else {
		cMatcherType.SetCurSel(options.matcherType);
	}

	ATTACH(IDC_AS_EXCLUDED, cExcludedWords);
	WinUtil::appendHistory(cExcludedWords, SettingsManager::HISTORY_EXCLUDE);
	cExcludedWords.SetWindowText(Text::toT(options.excludedWords).c_str());

	CheckDlgButton(IDC_CHECK_QUEUED, options.checkQueued);
	CheckDlgButton(IDC_CHECK_SHARED, options.checkShared);
	CheckDlgButton(IDC_MATCH_FULL_PATH, options.matchFullPath);

	if (options.matcherString != options.searchString) {
		ctrlMatcherString.SetWindowText(Text::toT(options.matcherString).c_str());
		CheckDlgButton(IDC_USE_MATCHER, true);
	}

	CheckDlgButton(IDC_MON, options.searchDays[1]);
	CheckDlgButton(IDC_TUE, options.searchDays[2]);
	CheckDlgButton(IDC_WED, options.searchDays[3]);
	CheckDlgButton(IDC_THU, options.searchDays[4]);
	CheckDlgButton(IDC_FRI, options.searchDays[5]);
	CheckDlgButton(IDC_SAT, options.searchDays[6]);
	CheckDlgButton(IDC_SUN, options.searchDays[0]);

	ATTACH(IDC_SEARCH_START, ctrlSearchStart);
	ATTACH(IDC_SEARCH_END, ctrlSearchEnd);
	ctrlSearchStart.SetFormat(_T("HH:mm"));
	ctrlSearchEnd.SetFormat(_T("HH:mm"));

	if (!((int8_t(options.searchDays.count()) == 7) && options.startTime.minute == 0 && options.startTime.hour == 0 && options.endTime.minute == 59 && options.endTime.hour == 23)) {
		CheckDlgButton(IDC_CUSTOM_SEARCH_TIMES, true);
	}
	else {
		CheckDlgButton(IDC_CUSTOM_SEARCH_TIMES, false);
	}

	SYSTEMTIME s;
	GetSystemTime(&s);
	s.wHour = options.startTime.hour;
	s.wMinute = options.startTime.minute;
	ctrlSearchStart.SetSystemTime(0, &s);

	SYSTEMTIME e;
	GetSystemTime(&e);
	e.wHour = options.endTime.hour;
	e.wMinute = options.endTime.minute;
	ctrlSearchEnd.SetSystemTime(0, &e);

	fixControls();
	loading = false; //loading done.
	return TRUE;
}


bool AutoSearchAdvancedPage::write() {

	TCHAR buf[512];

	options.checkQueued = IsDlgButtonChecked(IDC_CHECK_QUEUED) ? true : false;
	options.checkShared = IsDlgButtonChecked(IDC_CHECK_SHARED) ? true : false;
	options.matchFullPath = IsDlgButtonChecked(IDC_MATCH_FULL_PATH) ? true : false;

	auto useDefaultMatcher = IsDlgButtonChecked(IDC_USE_MATCHER) != BST_CHECKED;
	auto exactMatch = IsDlgButtonChecked(IDC_EXACT_MATCH) == BST_CHECKED;

	if (useDefaultMatcher || exactMatch) {
		options.matcherString = Util::emptyString;
		options.matcherType = exactMatch ? StringMatch::EXACT : StringMatch::PARTIAL;
	}
	else {
		GetDlgItemText(IDC_MATCHER_PATTERN, buf, 512);
		options.matcherString = Text::fromT(buf);
		options.matcherType = cMatcherType.GetCurSel();
	}

	options.excludedWords = WinUtil::addHistory(cExcludedWords, SettingsManager::HISTORY_EXCLUDE);

	bool useDefaultTimes = IsDlgButtonChecked(IDC_CUSTOM_SEARCH_TIMES) != BST_CHECKED;
	options.searchDays[1] = (IsDlgButtonChecked(IDC_MON) == BST_CHECKED || useDefaultTimes);
	options.searchDays[2] = (IsDlgButtonChecked(IDC_TUE) == BST_CHECKED || useDefaultTimes);
	options.searchDays[3] = (IsDlgButtonChecked(IDC_WED) == BST_CHECKED || useDefaultTimes);
	options.searchDays[4] = (IsDlgButtonChecked(IDC_THU) == BST_CHECKED || useDefaultTimes);
	options.searchDays[5] = (IsDlgButtonChecked(IDC_FRI) == BST_CHECKED || useDefaultTimes);
	options.searchDays[6] = (IsDlgButtonChecked(IDC_SAT) == BST_CHECKED || useDefaultTimes);
	options.searchDays[0] = (IsDlgButtonChecked(IDC_SUN) == BST_CHECKED || useDefaultTimes);

	if (useDefaultTimes) {
		options.startTime.hour = 0;
		options.startTime.minute = 0;
		options.endTime.hour = 23;
		options.endTime.minute = 59;
	}
	else {
		SYSTEMTIME s;
		ctrlSearchStart.GetSystemTime(&s);
		options.startTime.hour = s.wHour;
		options.startTime.minute = s.wMinute;

		SYSTEMTIME e;
		ctrlSearchEnd.GetSystemTime(&e);
		options.endTime.hour = e.wHour;
		options.endTime.minute = e.wMinute;

		if (options.endTime.hour < options.startTime.hour || (options.endTime.hour == options.startTime.hour && options.endTime.minute <= options.startTime.minute)) {
			MessageBox(CTSTRING(AS_END_GREATER));
			return false;
		}
	}

	GetDlgItemText(IDC_U_MATCH, buf, 512);
	options.userMatch = Text::fromT(buf);

	return true;
}


LRESULT AutoSearchAdvancedPage::onCheckMatcher(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

LRESULT AutoSearchAdvancedPage::onCheckTimes(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

LRESULT AutoSearchAdvancedPage::onExactMatch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}


void AutoSearchAdvancedPage::fixControls() {

	/* File type */
	if (options.searchType == SearchManager::TYPE_TTH)
		CheckDlgButton(IDC_USE_MATCHER, false);

	/* Misc advanced */
	::EnableWindow(GetDlgItem(IDC_CHECK_QUEUED), options.searchType == SearchManager::TYPE_DIRECTORY);
	::EnableWindow(GetDlgItem(IDC_CHECK_SHARED), options.searchType == SearchManager::TYPE_DIRECTORY);

	BOOL matcherEnabled = (IsDlgButtonChecked(IDC_USE_MATCHER) == BST_CHECKED);
	::EnableWindow(GetDlgItem(IDC_PATTERN), matcherEnabled);
	::EnableWindow(GetDlgItem(IDC_MATCHER_PATTERN), matcherEnabled);
	::EnableWindow(GetDlgItem(IDC_TYPE), matcherEnabled);
	::EnableWindow(GetDlgItem(IDC_MATCHER_TYPE), matcherEnabled);

	BOOL useCustomTimes = (IsDlgButtonChecked(IDC_CUSTOM_SEARCH_TIMES) == BST_CHECKED);
	::EnableWindow(GetDlgItem(IDC_MON), useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_TUE), useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_WED), useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_THU), useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_FRI), useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_SAT), useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_SUN), useCustomTimes);

	::EnableWindow(GetDlgItem(IDC_START_TIME), useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_SEARCH_START), useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_END_TIME), useCustomTimes);
	::EnableWindow(GetDlgItem(IDC_SEARCH_END), useCustomTimes);
}

