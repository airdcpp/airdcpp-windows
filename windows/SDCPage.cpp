/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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
#include "../client/SettingsManager.h"

#include "Resource.h"

#include "SDCPage.h"
#include "WinUtil.h"
#include "PropertiesDlg.h"

PropPage::TextItem SDCPage::texts[] = {
	{ IDC_SETTINGS_WRITE_BUFFER, ResourceManager::SETTINGS_WRITE_BUFFER },
	{ IDC_SETTINGS_KB, ResourceManager::KiB },
	{ IDC_STATIC1, ResourceManager::PORT },
	{ IDC_STATIC2, ResourceManager::USER },
	{ IDC_STATIC3, ResourceManager::PASSWORD },
	{ IDC_SETTINGS_ODC_SHUTDOWNTIMEOUT, ResourceManager::SETTINGS_ODC_SHUTDOWNTIMEOUT },
	{ IDC_MAXCOMPRESS, ResourceManager::SETTINGS_MAX_COMPRESS },
	{ IDC_USERLISTDBLCLICKACTION, ResourceManager::USERLISTDBLCLICKACTION },
	{ IDC_TRANSFERLISTDBLCLICKACTION, ResourceManager::TRANSFERLISTDBLCLICKACTION },
	{ IDC_CHATDBLCLICKACTION, ResourceManager::CHATDBLCLICKACTION },
	{ IDC_SHUTDOWNACTION, ResourceManager::SHUTDOWN_ACTION },
	{ IDC_SETTINGS_DOWNCONN, ResourceManager::SETTINGS_DOWNCONN },
	{ IDC_SETTINGS_SOCKET_IN_BUFFER, ResourceManager::SETTINGS_SOCKET_IN_BUFFER },
	{ IDC_SETTINGS_SOCKET_OUT_BUFFER, ResourceManager::SETTINGS_SOCKET_OUT_BUFFER },
	{ IDC_LOG_LINESTEXT, ResourceManager::MAX_LOG_LINES },
	{ IDC_DECREASE_RAM, ResourceManager::DECREASE_RAM },
	{ IDC_BLOOM_MODE_LBL, ResourceManager::BLOOM_MODE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item SDCPage::items[] = {
	{ IDC_BUFFERSIZE, SettingsManager::BUFFER_SIZE, PropPage::T_INT },
	{ IDC_SOCKET_IN_BUFFER, SettingsManager::SOCKET_IN_BUFFER, PropPage::T_INT },
	{ IDC_SOCKET_OUT_BUFFER, SettingsManager::SOCKET_OUT_BUFFER, PropPage::T_INT },
	{ IDC_SHUTDOWNTIMEOUT, SettingsManager::SHUTDOWN_TIMEOUT, PropPage::T_INT },
	{ IDC_MAX_COMPRESSION, SettingsManager::MAX_COMPRESSION, PropPage::T_INT },
	{ IDC_DOWNCONN, SettingsManager::DOWNCONN_PER_SEC, PropPage::T_INT },
	{ IDC_LOG_LINES, SettingsManager::LOG_LINES, PropPage::T_INT },
	{ IDC_DECREASE_RAM, SettingsManager::DECREASE_RAM, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

LRESULT SDCPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	setMinMax(IDC_BUFFER_SPIN, 0, 4096);
	setMinMax(IDC_READ_SPIN, 1024, 1024*1024);
	setMinMax(IDC_WRITE_SPIN, 1024, 1024*1024);

	setMinMax(IDC_DOWNCONN_SPIN, 0, 5);
	setMinMax(IDC_MAX_COMP_SPIN, 0, 9);

	setMinMax(IDC_SHUTDOWN_SPIN , 1, 3600);
	setMinMax(IDC_LOG_LINES_SPIN, 0, 1000);

	ctrlBloom.Attach(GetDlgItem(IDC_BLOOM_MODE));
	ctrlBloom.AddString(CTSTRING(DISABLED));
	ctrlBloom.AddString(CTSTRING(ENABLED));
	ctrlBloom.SetCurSel(SETTING(SEND_BLOOM));

	ctrlShutdownAction.Attach(GetDlgItem(IDC_COMBO1));
	ctrlShutdownAction.AddString(CTSTRING(POWER_OFF));
	ctrlShutdownAction.AddString(CTSTRING(LOG_OFF));
	ctrlShutdownAction.AddString(CTSTRING(REBOOT));
	ctrlShutdownAction.AddString(CTSTRING(SUSPEND));
	ctrlShutdownAction.AddString(CTSTRING(HIBERNATE));

	ctrlShutdownAction.SetCurSel(SETTING(SHUTDOWN_ACTION));

	// Do specialized reading here
	userlistaction.Attach(GetDlgItem(IDC_USERLIST_DBLCLICK));
	transferlistaction.Attach(GetDlgItem(IDC_TRANSFERLIST_DBLCLICK));
	chataction.Attach(GetDlgItem(IDC_CHAT_DBLCLICK));

    userlistaction.AddString(CTSTRING(GET_FILE_LIST));
    userlistaction.AddString(CTSTRING(SEND_PUBLIC_MESSAGE));
    userlistaction.AddString(CTSTRING(SEND_PRIVATE_MESSAGE));
    userlistaction.AddString(CTSTRING(MATCH_QUEUE));
    userlistaction.AddString(CTSTRING(GRANT_EXTRA_SLOT));
	userlistaction.AddString(CTSTRING(ADD_TO_FAVORITES));
	userlistaction.AddString(CTSTRING(BROWSE_FILE_LIST));
	transferlistaction.AddString(CTSTRING(SEND_PRIVATE_MESSAGE));
	transferlistaction.AddString(CTSTRING(GET_FILE_LIST));
	transferlistaction.AddString(CTSTRING(MATCH_QUEUE));
	transferlistaction.AddString(CTSTRING(GRANT_EXTRA_SLOT));
	transferlistaction.AddString(CTSTRING(ADD_TO_FAVORITES));
	transferlistaction.AddString(CTSTRING(BROWSE_FILE_LIST));
	chataction.AddString(CTSTRING(SELECT_USER_LIST));
    chataction.AddString(CTSTRING(SEND_PUBLIC_MESSAGE));
    chataction.AddString(CTSTRING(SEND_PRIVATE_MESSAGE));
    chataction.AddString(CTSTRING(GET_FILE_LIST));
    chataction.AddString(CTSTRING(MATCH_QUEUE));
    chataction.AddString(CTSTRING(GRANT_EXTRA_SLOT));
	chataction.AddString(CTSTRING(ADD_TO_FAVORITES));

	userlistaction.SetCurSel(SETTING(USERLIST_DBLCLICK));
	transferlistaction.SetCurSel(SETTING(TRANSFERLIST_DBLCLICK));
	chataction.SetCurSel(SETTING(CHAT_DBLCLICK));

	userlistaction.Detach();
	transferlistaction.Detach();
	chataction.Detach();
	
	return TRUE;
}

void SDCPage::write()
{

	PropPage::write((HWND)*this, items);
	SettingsManager::getInstance()->set(SettingsManager::SHUTDOWN_ACTION, ctrlShutdownAction.GetCurSel());

	userlistaction.Attach(GetDlgItem(IDC_USERLIST_DBLCLICK));
	transferlistaction.Attach(GetDlgItem(IDC_TRANSFERLIST_DBLCLICK));
	chataction.Attach(GetDlgItem(IDC_CHAT_DBLCLICK));
	settings->set(SettingsManager::USERLIST_DBLCLICK, userlistaction.GetCurSel());
	settings->set(SettingsManager::TRANSFERLIST_DBLCLICK, transferlistaction.GetCurSel());
	settings->set(SettingsManager::CHAT_DBLCLICK, chataction.GetCurSel());
	settings->set(SettingsManager::SEND_BLOOM, ctrlBloom.GetCurSel());
	userlistaction.Detach();
	transferlistaction.Detach(); 
	chataction.Detach(); 

	if(SETTING(DOWNCONN_PER_SEC) > 5) 
		settings->set(SettingsManager::DOWNCONN_PER_SEC, 5);

	if(SETTING(AUTO_SEARCH_LIMIT) < 1)
		settings->set(SettingsManager::AUTO_SEARCH_LIMIT, 1);	
}