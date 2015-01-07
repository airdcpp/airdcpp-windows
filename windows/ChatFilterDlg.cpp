/*
* Copyright (C) 2012-2014 AirDC++ Project
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
#include "ChatFilterDlg.h"

#include "../client/MessageManager.h"
#include "../client/ResourceManager.h"

#define ATTACH(id, var) var.Attach(GetDlgItem(id))

ChatFilterDlg::ChatFilterDlg() : nick(""), text(""), nickMethod(StringMatch::EXACT), textMethod(StringMatch::PARTIAL), MC(true), PM(true) {}

ChatFilterDlg::ChatFilterDlg(const string& aNickMatch, const string& aTextMatch, StringMatch::Method aNickMethod, StringMatch::Method aTextMethod, bool aMC, bool aPM) :
nick(aNickMatch), text(aTextMatch), nickMethod(aNickMethod), textMethod(aTextMethod), MC(aMC), PM(aPM)  {}

ChatFilterDlg::~ChatFilterDlg() { }

LRESULT ChatFilterDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	ATTACH(IDC_NICK_MATCH, ctrlNickMatch);
	ctrlNickMatch.SetWindowText(Text::toT(nick).c_str());
	ATTACH(IDC_NICK_MATCH_TYPE, ctrlNickMatchType);
	ctrlNickMatchType.AddString(CTSTRING(PARTIAL_MATCH));
	ctrlNickMatchType.AddString(CTSTRING(REGEXP));
	ctrlNickMatchType.AddString(CTSTRING(WILDCARDS));
	ctrlNickMatchType.AddString(CTSTRING(EXACT_MATCH));
	ctrlNickMatchType.SetCurSel(nickMethod);

	ATTACH(IDC_TEXT_MATCH, ctrlTextMatch);
	ctrlTextMatch.SetWindowText(Text::toT(text).c_str());
	ATTACH(IDC_TEXT_MATCH_TYPE, ctrlTextMatchType);
	ctrlTextMatchType.AddString(CTSTRING(PARTIAL_MATCH));
	ctrlTextMatchType.AddString(CTSTRING(REGEXP));
	ctrlTextMatchType.AddString(CTSTRING(WILDCARDS));
	ctrlTextMatchType.AddString(CTSTRING(EXACT_MATCH));
	ctrlTextMatchType.SetCurSel(textMethod);

	ATTACH(IDC_IGNORE_MAINCHAT, ctrlMainchat);
	ctrlMainchat.SetCheck(MC);

	ATTACH(IDC_IGNORE_PM, ctrlPM);
	ctrlPM.SetCheck(PM);
	
	::SetWindowText(GetDlgItem(IDCANCEL), CTSTRING(CANCEL));
	::SetWindowText(GetDlgItem(IDC_NICK_MATCH_LABEL), CTSTRING(IGNORE_NICK_MATCH));
	::SetWindowText(GetDlgItem(IDC_TEXT_MATCH_LABEL), CTSTRING(IGNORE_TEXT_MATCH));
	::SetWindowText(GetDlgItem(IDC_NICK_MATCH_TYPE_LABEL), CTSTRING(SETTINGS_ST_MATCH_TYPE));
	::SetWindowText(GetDlgItem(IDC_TEXT_MATCH_TYPE_LABEL), CTSTRING(SETTINGS_ST_MATCH_TYPE));
	::SetWindowText(GetDlgItem(IDC_IGNORE_NOTE), CTSTRING(IGNORE_HELP));
	::SetWindowText(GetDlgItem(IDC_IGNORE_PM), CTSTRING(PRIVATE_CHAT));
	::SetWindowText(GetDlgItem(IDC_IGNORE_MAINCHAT), CTSTRING(MAIN_CHAT));

	CenterWindow(GetParent());
	SetWindowText(CTSTRING(SETTINGS_CHATFILTER));
	return TRUE;
}

LRESULT ChatFilterDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (wID == IDOK) {
		if (ctrlNickMatch.GetWindowTextLength() == 0 && ctrlTextMatch.GetWindowTextLength() == 0) {
			MessageBox(CTSTRING(LINE_EMPTY));
			return 0;
		}
		PM = ctrlPM.GetCheck() ? true : false;
		MC = ctrlMainchat.GetCheck() ? true : false;

		if (!PM && !MC) {
			MessageBox(CTSTRING(IGNORE_NO_CONTEXT), CTSTRING(SETTINGS_CHATFILTER), MB_OK | MB_ICONEXCLAMATION);
			return 0;
		}

		nick = Text::fromT(WinUtil::getEditText(ctrlNickMatch));
		nickMethod = (StringMatch::Method)ctrlNickMatchType.GetCurSel();

		text = Text::fromT(WinUtil::getEditText(ctrlTextMatch));
		textMethod = (StringMatch::Method)ctrlTextMatchType.GetCurSel();

	}
	
	EndDialog(wID);
	return 0;
}