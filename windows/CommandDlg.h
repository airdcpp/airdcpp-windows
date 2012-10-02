/*
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(COMMAND_DLG_H)
#define COMMAND_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WinUtil.h"

class CommandDlg : public CDialogImpl<CommandDlg>
{
	CEdit ctrlName;
	CEdit ctrlCommand;
	CEdit ctrlHub;
	CEdit ctrlNick;
	CButton ctrlSeparator;
	CButton ctrlRaw;
	CButton ctrlChat;
	CButton ctrlPM;
	CButton ctrlHubMenu;
	CButton ctrlUserMenu;
	CButton ctrlSearchMenu;
	CButton ctrlFilelistMenu;
	CButton ctrlOnce;

public:
	int type;
	int ctx;
	tstring name;
	tstring command;
	tstring to;
	tstring hub;

	enum { IDD = IDD_USER_COMMAND };

	BEGIN_MSG_MAP(CommandDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		MESSAGE_HANDLER(WM_HELP, onHelp)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_SETTINGS_SEPARATOR, onType)
		COMMAND_ID_HANDLER(IDC_SETTINGS_RAW, onType)
		COMMAND_ID_HANDLER(IDC_SETTINGS_CHAT, onType)
		COMMAND_ID_HANDLER(IDC_SETTINGS_PM, onType)
		COMMAND_HANDLER(IDC_COMMAND, EN_CHANGE, onChange)
		COMMAND_HANDLER(IDC_NICK, EN_CHANGE, onChange)
		COMMAND_HANDLER(IDC_HUB, EN_CHANGE, onHub);
	END_MSG_MAP()

	CommandDlg() : type(0), ctx(0) { }

	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlName.SetFocus();
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onType(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onHelp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);	
	LRESULT onChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onHub(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
private:
	static WinUtil::TextItem texts[];

	void updateType();
	void updateCommand();
	void updateHub();
	void updateControls();
	void updateContext();
};

#endif // !defined(COMMAND_DLG_H)