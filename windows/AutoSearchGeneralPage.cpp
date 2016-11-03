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
#include "AutoSearchGeneralPage.h"
#include "AutosearchParams.h"

#include <airdcpp/SearchManager.h>

#define ATTACH(id, var) var.Attach(GetDlgItem(id))

AutoSearchGeneralPage::AutoSearchGeneralPage(AutoSearchItemSettings& aSettings, const string& aName) : options(aSettings), name(aName), loading(true) {}

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
	::SetWindowText(GetDlgItem(IDC_GROUP_LABEL), CTSTRING(GROUP));

	//get the search type so that we can set the initial control states correctly in fixControls
	StringList ext;
	try {
		SearchManager::getInstance()->getSearchType(options.fileTypeStr, options.searchType, ext);
	} catch (...) {
		//switch back to default
		options.searchType = Search::TYPE_ANY;
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

	cGroups.Attach(GetDlgItem(IDC_FAVGROUP_BOX));
	cGroups.AddString(_T("---"));
	cGroups.SetCurSel(0);

	auto groups = AutoSearchManager::getInstance()->getGroups();
	for (const auto& i : groups) {
		int pos = cGroups.AddString(Text::toT(i).c_str());
		if (i == options.groupName)
			cGroups.SetCurSel(pos);
	}

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
	if (dlg.DoModal() == IDOK) {

	}
	return 0;
}

LRESULT AutoSearchGeneralPage::onClickLocation(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CRect rect;
	cSelectDir.GetWindowRect(rect);
	auto pt = rect.BottomRight();
	pt.x = pt.x - rect.Width();
	cSelectDir.SetState(true);

	OMenu targetMenu;
	targetMenu.CreatePopupMenu();
	targetMenu.InsertSeparatorFirst(CTSTRING(DOWNLOAD_TO));
	//appendDownloadMenu(targetMenu, false, true);
	appendDownloadTo(targetMenu, false, true, boost::none, boost::none);

	targetMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERPOSANIMATION, pt);
	return 0;
}

void AutoSearchGeneralPage::handleDownload(const string& aTarget, Priority /*p*/, bool /*useWhole*/, TargetUtil::TargetType aTargetType, bool /*isSizeUnknown*/) {
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
	}
	else {
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
	str.resize(ctrlSearch.GetWindowTextLength() + 1);
	str.resize(GetDlgItemText(IDC_AS_SEARCH_STRING, &str[0], ctrlSearch.GetWindowTextLength() + 1 + insert.size()));

	int lSelBegin = 0, lSelEnd = 0;
	ctrlSearch.GetSel(lSelBegin, lSelEnd);

	str.insert(lSelBegin, insert);
	//str.insert()
	//str += _T("%[inc]");
	ctrlSearch.SetWindowText(str.c_str());
}

bool AutoSearchGeneralPage::write() {
	TCHAR bufPath[MAX_PATH];
	if (ctrlSearch.GetWindowTextLength() < 5) {
		MessageBox(CTSTRING(LINE_EMPTY_OR_TOO_SHORT));
		return false;
	}

	tstring str;
	str.resize(ctrlSearch.GetWindowTextLength() + 1);
	str.resize(GetDlgItemText(IDC_AS_SEARCH_STRING, &str[0], ctrlSearch.GetWindowTextLength() + 1));
	options.searchString = Text::fromT(str);

	auto lst = AutoSearchManager::getInstance()->getSearchesByString(options.searchString, options.as);
	/*
	if the dupe already exists don't confirm when edit the dupe item. 
	(we can assume it does if the search string was not changed)
	*/
	if (!lst.empty() && (!options.as || (options.searchString != options.as->getSearchString()))) { 
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
	}
	else {
		options.expireTime = 0;
	}

	if (cGroups.GetCurSel() == 0) {
		options.groupName = Util::emptyString;
	}else{
		tstring tmp;
		tmp.resize(cGroups.GetWindowTextLength());
		tmp.resize(cGroups.GetWindowText(&tmp[0], tmp.size() + 1));
		options.groupName = Text::fromT(tmp);
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
	}
	catch (...) {}
	fixControls();
	return 0;
}

void AutoSearchGeneralPage::fixControls() {

	/* Action */
	BOOL isReportOnly = cAction.GetCurSel() == AutoSearch::ACTION_REPORT;
	::EnableWindow(GetDlgItem(IDC_TARGET_PATH), !isReportOnly);
	::EnableWindow(GetDlgItem(IDC_SELECT_DIR), !isReportOnly);


	/* Param config */
	bool usingParams = IsDlgButtonChecked(IDC_USE_PARAMS) == BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_CONF_PARAMS), usingParams);

	/* Expiry date */
	::EnableWindow(GetDlgItem(IDC_DATETIMEPICKER), IsDlgButtonChecked(IDC_USE_EXPIRY) == BST_CHECKED);

}
