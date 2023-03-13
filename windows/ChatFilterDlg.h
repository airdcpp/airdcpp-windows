/*
* Copyright (C) 2012-2023 AirDC++ Project
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

#ifndef IGNORE_DLG_H
#define IGNORE_DLG_H

#pragma once

#include <atlcrack.h>

#include <airdcpp/StringMatch.h>

class ChatFilterDlg : public CDialogImpl<ChatFilterDlg> {
public:

	enum { IDD = IDD_CHATFILTERITEM_DLG };

	ChatFilterDlg(const string& aNickMatch, const string& aTextMatch, StringMatch::Method aNickMethod, StringMatch::Method aTextMethod, bool aMC, bool aPM);
	ChatFilterDlg();
	~ChatFilterDlg();

	BEGIN_MSG_MAP_EX(ChatFilterDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()


		LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
			ctrlNickMatch.SetFocus();
			return FALSE;
		}

		LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
		LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

		string nick;
		string text;
		StringMatch::Method nickMethod;
		StringMatch::Method textMethod;
		bool PM;
		bool MC;

private:
	CEdit ctrlNickMatch;
	CEdit ctrlTextMatch;

	CComboBox ctrlNickMatchType;
	CComboBox ctrlTextMatchType;

	CButton ctrlMainchat;
	CButton ctrlPM;

};
#endif
