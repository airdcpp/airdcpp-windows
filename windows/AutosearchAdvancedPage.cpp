/*
* Copyright (C) 2011-2016 AirDC++ Project
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

#include <airdcpp/Search.h>

#define ATTACH(id, var) var.Attach(GetDlgItem(id))
#define setMinMax(x, y, z) \
	updown.Attach(GetDlgItem(x)); \
	updown.SetRange32(y, z); \
	updown.Detach();

AutoSearchAdvancedPage::AutoSearchAdvancedPage(AutoSearchItemSettings& aSettings) : options(aSettings), loading(true) {}

AutoSearchAdvancedPage::~AutoSearchAdvancedPage() { }

LRESULT AutoSearchAdvancedPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ATTACH(IDC_U_MATCH, ctrlUserMatch);
	ATTACH(IDC_MATCHER_PATTERN, ctrlMatcherString);

	ctrlUserMatch.SetWindowText(Text::toT(options.userMatch).c_str());

	::SetWindowText(GetDlgItem(IDC_USER_MATCH_TEXT), CTSTRING(AS_USER_MATCH));
	::SetWindowText(GetDlgItem(IDC_USE_MATCHER), CTSTRING(USE_CUSTOM_MATCHER));
	::SetWindowText(GetDlgItem(IDC_PATTERN), CTSTRING(PATTERN));
	::SetWindowText(GetDlgItem(IDC_TYPE), CTSTRING(TYPE));
	::SetWindowText(GetDlgItem(IDC_CHECK_QUEUED), CTSTRING(AUTOSEARCH_CHECK_QUEUED));
	::SetWindowText(GetDlgItem(IDC_CHECK_SHARED), CTSTRING(AUTOSEARCH_CHECK_SHARED));
	::SetWindowText(GetDlgItem(IDC_MATCH_FULL_PATH), CTSTRING(MATCH_FULL_PATH));
	::SetWindowText(GetDlgItem(IDC_AS_EXCLUDED_LABEL), CTSTRING(EXCLUDED_WORDS_DESC));
	::SetWindowText(GetDlgItem(IDC_EXACT_MATCH), CTSTRING(REQUIRE_EXACT_MATCH));
	::SetWindowText(GetDlgItem(IDC_AS_USERMATCHER_EXCLUDE), CTSTRING(EXCLUDE_MATCHES));

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

	fixControls();
	loading = false; //loading done.
	return TRUE;
}


bool AutoSearchAdvancedPage::write() {

	TCHAR buf[512];

	options.checkQueued = IsDlgButtonChecked(IDC_CHECK_QUEUED) ? true : false;
	options.checkShared = IsDlgButtonChecked(IDC_CHECK_SHARED) ? true : false;
	options.matchFullPath = IsDlgButtonChecked(IDC_MATCH_FULL_PATH) ? true : false;
	options.userMatcherExclude = IsDlgButtonChecked(IDC_AS_USERMATCHER_EXCLUDE) ? true : false;

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

	GetDlgItemText(IDC_U_MATCH, buf, 512);
	options.userMatch = Text::fromT(buf);

	return true;
}


LRESULT AutoSearchAdvancedPage::onCheckMatcher(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

LRESULT AutoSearchAdvancedPage::onExactMatch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}


void AutoSearchAdvancedPage::fixControls() {

	/* File type */
	if (options.searchType == Search::TYPE_TTH)
		CheckDlgButton(IDC_USE_MATCHER, false);

	/* Result matcher */
	bool exactMatch = IsDlgButtonChecked(IDC_EXACT_MATCH) == BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_USE_MATCHER), options.searchType != Search::TYPE_TTH && !exactMatch);
	
	/* Misc advanced */
	::EnableWindow(GetDlgItem(IDC_CHECK_QUEUED), options.searchType == Search::TYPE_DIRECTORY);
	::EnableWindow(GetDlgItem(IDC_CHECK_SHARED), options.searchType == Search::TYPE_DIRECTORY);

	BOOL matcherEnabled = (IsDlgButtonChecked(IDC_USE_MATCHER) == BST_CHECKED) && !exactMatch;
	::EnableWindow(GetDlgItem(IDC_PATTERN), matcherEnabled);
	::EnableWindow(GetDlgItem(IDC_MATCHER_PATTERN), matcherEnabled);
	::EnableWindow(GetDlgItem(IDC_TYPE), matcherEnabled);
	::EnableWindow(GetDlgItem(IDC_MATCHER_TYPE), matcherEnabled);
}

