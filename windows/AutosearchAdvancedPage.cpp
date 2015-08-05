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
#include "AutosearchAdvancedPage.h"
#include "../client/SearchManager.h"

#define ATTACH(id, var) var.Attach(GetDlgItem(id))
#define setMinMax(x, y, z) \
	updown.Attach(GetDlgItem(x)); \
	updown.SetRange32(y, z); \
	updown.Detach();

AutoSearchAdvancedPage::AutoSearchAdvancedPage(AutoSearchItemSettings& aSettings) : options(aSettings) {}

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
	::SetWindowText(GetDlgItem(IDC_SEARCH_INT_LABEL), CTSTRING(MINIMUM_SEARCH_INTERVAL));


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

	setMinMax(IDC_SEARCH_INT_SPIN, 60, 999);
	ATTACH(IDC_SEARCH_INT, ctrlSearchInterval);
	ctrlSearchInterval.SetWindowText(Util::toStringW(options.searchInterval).c_str());

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
	int i = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlSearchInterval)));
	options.searchInterval = i < 60 ? 60 : i;

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

LRESULT AutoSearchAdvancedPage::onTimeChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
	if (loading)
		return 0;

	int value = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlSearchInterval)));
	if (value < 60) {
		value = 60;
		ctrlSearchInterval.SetWindowText(Text::toT(Util::toString(value)).c_str());
	}
	return 0;
}


void AutoSearchAdvancedPage::fixControls() {

	/* File type */
	if (options.searchType == SearchManager::TYPE_TTH)
		CheckDlgButton(IDC_USE_MATCHER, false);

	/* Result matcher */
	bool exactMatch = IsDlgButtonChecked(IDC_EXACT_MATCH) == BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_USE_MATCHER), options.searchType != SearchManager::TYPE_TTH && !exactMatch);
	
	/* Misc advanced */
	::EnableWindow(GetDlgItem(IDC_CHECK_QUEUED), options.searchType == SearchManager::TYPE_DIRECTORY);
	::EnableWindow(GetDlgItem(IDC_CHECK_SHARED), options.searchType == SearchManager::TYPE_DIRECTORY);

	BOOL matcherEnabled = (IsDlgButtonChecked(IDC_USE_MATCHER) == BST_CHECKED) && !exactMatch;
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

