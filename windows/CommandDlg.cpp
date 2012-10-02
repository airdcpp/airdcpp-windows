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

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "../client/ResourceManager.h"
#include "../client/UserCommand.h"
#include "../client/NmdcHub.h"

#include "WinUtil.h"
#include "CommandDlg.h"

WinUtil::TextItem CommandDlg::texts[] = {
	{ IDOK, ResourceManager::OK },
	{ IDCANCEL, ResourceManager::CANCEL },
	{ IDC_SETTINGS_TYPE, ResourceManager::USER_CMD_TYPE },
	{ IDC_SETTINGS_SEPARATOR, ResourceManager::SEPARATOR },
	{ IDC_SETTINGS_CHAT, ResourceManager::USER_CMD_CHAT },
	{ IDC_SETTINGS_PM, ResourceManager::USER_CMD_PM },
	{ IDC_SETTINGS_CONTEXT, ResourceManager::USER_CMD_CONTEXT },
	{ IDC_SETTINGS_HUB_MENU, ResourceManager::USER_CMD_HUB_MENU },
	{ IDC_SETTINGS_USER_MENU, ResourceManager::USER_CMD_USER_MENU },
	{ IDC_SETTINGS_SEARCH_MENU, ResourceManager::USER_CMD_SEARCH_MENU },
	{ IDC_SETTINGS_FILELIST_MENU, ResourceManager::USER_CMD_FILELIST_MENU },
	{ IDC_SETTINGS_PARAMETERS, ResourceManager::USER_CMD_PARAMETERS },
	{ IDC_SETTINGS_NAME, ResourceManager::HUB_NAME },
	{ IDC_SETTINGS_COMMAND, ResourceManager::USER_CMD_COMMAND },
	{ IDC_SETTINGS_HUB, ResourceManager::USER_CMD_HUB },
	{ IDC_SETTINGS_TO, ResourceManager::USER_CMD_TO },
	{ IDC_SETTINGS_ONCE, ResourceManager::USER_CMD_ONCE },
	{ IDC_USER_CMD_EXAMPLE, ResourceManager::USER_CMD_PREVIEW },
	{ IDC_USER_CMD_DESCRIPTION, ResourceManager::DESCRIPTION },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT CommandDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// Translate
	SetWindowText(CTSTRING(USER_CMD_WINDOW));

#define ATTACH(id, var) var.Attach(GetDlgItem(id))
	ATTACH(IDC_NAME, ctrlName);
	ATTACH(IDC_HUB, ctrlHub);
	ATTACH(IDC_SETTINGS_SEPARATOR, ctrlSeparator);
	ATTACH(IDC_SETTINGS_RAW, ctrlRaw);
	ATTACH(IDC_SETTINGS_CHAT, ctrlChat);
	ATTACH(IDC_SETTINGS_PM, ctrlPM);
	ATTACH(IDC_SETTINGS_ONCE, ctrlOnce);
	ATTACH(IDC_SETTINGS_HUB_MENU, ctrlHubMenu);
	ATTACH(IDC_SETTINGS_USER_MENU, ctrlUserMenu);
	ATTACH(IDC_SETTINGS_SEARCH_MENU, ctrlSearchMenu);
	ATTACH(IDC_SETTINGS_FILELIST_MENU, ctrlFilelistMenu);
	ATTACH(IDC_NICK, ctrlNick);
	ATTACH(IDC_COMMAND, ctrlCommand);

	WinUtil::translate(*this, texts);
	SetDlgItemText(IDC_COMMAND_DESCRIPTION, CTSTRING(USER_CMD_DESCRIPTION));

	int newType = 0;
	if(type == UserCommand::TYPE_SEPARATOR) {
		ctrlSeparator.SetCheck(BST_CHECKED);
		newType = 0;
	} else {
		ctrlCommand.SetWindowText(Text::toDOS(command).c_str());
		if(type == UserCommand::TYPE_RAW || type == UserCommand::TYPE_RAW_ONCE) {
			ctrlRaw.SetCheck(BST_CHECKED);
			newType = 1;
		} else if(type == UserCommand::TYPE_CHAT || type == UserCommand::TYPE_CHAT_ONCE) {
			if(to.empty()) {
				ctrlChat.SetCheck(BST_CHECKED);
				newType = 2;
			} else {
				ctrlPM.SetCheck(BST_CHECKED);
				ctrlNick.SetWindowText(to.c_str());
				newType = 3;
			}		
		}

		if(type == UserCommand::TYPE_RAW_ONCE || type == UserCommand::TYPE_CHAT_ONCE) {
			ctrlOnce.SetCheck(BST_CHECKED);
		}
	}
	type = newType;

	ctrlHub.SetWindowText(hub.c_str());
	ctrlName.SetWindowText(name.c_str());

	if(ctx & UserCommand::CONTEXT_HUB)
		ctrlHubMenu.SetCheck(BST_CHECKED);
	if(ctx & UserCommand::CONTEXT_USER)
		ctrlUserMenu.SetCheck(BST_CHECKED);
	if(ctx & UserCommand::CONTEXT_SEARCH)
		ctrlSearchMenu.SetCheck(BST_CHECKED);
	if(ctx & UserCommand::CONTEXT_FILELIST)
		ctrlFilelistMenu.SetCheck(BST_CHECKED);
	
	updateControls();
	updateCommand();

	ctrlSeparator.SetFocus();
	
	CenterWindow(GetParent());
	return FALSE;
}

LRESULT CommandDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(wID == IDOK) {
		TCHAR buf[256];
		GetDlgItemText(IDC_NAME, buf, sizeof(buf) - 1);
		name = buf;

		if((type != 0) && ((ctrlName.GetWindowTextLength() == 0) || (ctrlCommand.GetWindowTextLength()== 0))) {
			MessageBox(_T("Name and command must not be empty"));
			return 0;
		}

		updateContext();

		switch(type) {
		case 0:
			type = UserCommand::TYPE_SEPARATOR;
			break;
		case 1:
			type = (ctrlOnce.GetCheck() == BST_CHECKED) ? UserCommand::TYPE_RAW_ONCE : UserCommand::TYPE_RAW;
			break;
		case 2:
			type = UserCommand::TYPE_CHAT;
			break;
		case 3:
			type = (ctrlOnce.GetCheck() == BST_CHECKED) ? UserCommand::TYPE_CHAT_ONCE : UserCommand::TYPE_CHAT;
			ctrlNick.GetWindowText(buf, sizeof(buf) - 1);
			to = buf;
			break;
		}
	}
	EndDialog(wID);
	return 0;
}

LRESULT CommandDlg::onChange(WORD , WORD , HWND , BOOL& ) {
	updateCommand();
	return 0;
}

LRESULT CommandDlg::onHub(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	updateHub();
	return 0;
}

LRESULT CommandDlg::onType(WORD , WORD, HWND , BOOL& ) {
	updateType();
	updateCommand();
	updateControls();
	return 0;
}

void CommandDlg::updateContext() {
	ctx = 0;
	if(ctrlHubMenu.GetCheck() & BST_CHECKED)
		ctx |= UserCommand::CONTEXT_HUB;
	if(ctrlUserMenu.GetCheck() & BST_CHECKED)
		ctx |= UserCommand::CONTEXT_USER;
	if(ctrlSearchMenu.GetCheck() & BST_CHECKED)
		ctx |= UserCommand::CONTEXT_SEARCH;
	if(ctrlFilelistMenu.GetCheck() & BST_CHECKED)
		ctx |= UserCommand::CONTEXT_FILELIST;
}


void CommandDlg::updateType() {
	if(ctrlSeparator.GetCheck() == BST_CHECKED) {
		type = 0;
	} else if(ctrlRaw.GetCheck() == BST_CHECKED) {
		type = 1;
	} else if(ctrlChat.GetCheck() == BST_CHECKED) {
		type = 2;
	} else if(ctrlPM.GetCheck() == BST_CHECKED) {
		type = 3;
	}
}

void CommandDlg::updateCommand() {
	static const size_t BUF_LEN = 1024;
	TCHAR buf[BUF_LEN];
	if(type == 0) {
		command.clear();
	} else {
		ctrlCommand.GetWindowText(buf, BUF_LEN-1);
		command = buf;
	}

	if(type == 1 && UserCommand::adc(Text::fromT(hub)) && !command.empty() && *(command.end() - 1) != '\n')
		command += '\n';
}

void CommandDlg::updateHub() {
	static const size_t BUF_LEN = 1024;
	TCHAR buf[BUF_LEN];
	ctrlHub.GetWindowText(buf, BUF_LEN-1);
	hub = buf;
	updateCommand();
}

void CommandDlg::updateControls() {
	switch(type) {
		case 0:
			ctrlName.EnableWindow(FALSE);
			ctrlCommand.EnableWindow(FALSE);
			ctrlNick.EnableWindow(FALSE);
			break;
		case 1:
		case 2:
			ctrlName.EnableWindow(TRUE);
			ctrlCommand.EnableWindow(TRUE);
			ctrlNick.EnableWindow(FALSE);
			break;
		case 3:
			ctrlName.EnableWindow(TRUE);
			ctrlCommand.EnableWindow(TRUE);
			ctrlNick.EnableWindow(TRUE);
			break;
	}
}

LRESULT CommandDlg::onHelp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	WinUtil::openLink(_T("http://dcplusplus.sourceforge.net/webhelp/dialog_user_command.html"));
	return 0;
}