
/*
* Copyright (C) 2012-2016 AirDC++ Project
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

#ifndef WEB_USER_DLG_H
#define WEB_USER_DLG_H

#include <atlcrack.h>

#include "resource.h"
#include "WinUtil.h"

class WebUserDlg : public CDialogImpl<WebUserDlg> {
public:

	enum { IDD = IDD_WEB_USER_DLG };

	BEGIN_MSG_MAP(WebUserDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	WebUserDlg() { }
	WebUserDlg(const tstring& aName, const tstring& aPassword) : name(aName), pwd(aPassword) { }

	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlName.SetFocus();
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		ctrlName.Attach(GetDlgItem(IDC_WEBUSER_NAME));
		ctrlName.SetFocus();
		ctrlName.SetWindowText(name.c_str());

		ctrlPassword.Attach(GetDlgItem(IDC_WEBUSER_PWD));
		ctrlPassword.SetWindowText(pwd.c_str());

		::SetWindowText(GetDlgItem(IDC_WEBUSER_NAME_LABEL), CTSTRING(USERNAME));
		::SetWindowText(GetDlgItem(IDC_WEBUSER_PWD_LABEL), CTSTRING(PASSWORD));

		SetWindowText(CTSTRING(USER));

		CenterWindow(GetParent());
		return FALSE;
	}

	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		name = WinUtil::getEditText(ctrlName);
		pwd = WinUtil::getEditText(ctrlPassword);
		EndDialog(wID);
		return 0;
	}

	string getUserName() const {
		return Text::fromT(name);
	}

	string getPassWord() const {
		return Text::fromT(pwd);
	}

private:

	tstring name;
	tstring pwd;

	CEdit ctrlName;
	CEdit ctrlPassword;

};
#endif
