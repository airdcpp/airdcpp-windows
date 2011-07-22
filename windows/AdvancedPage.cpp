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
#include "../client/SettingsManager.h"
#include "../client/FavoriteManager.h"

#include "Resource.h"
#include "AdvancedPage.h"
#include "CommandDlg.h"
#include "WinUtil.h"
#include "PropertiesDlg.h"

PropPage::TextItem AdvancedPage::texts[] = {
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item AdvancedPage::items[] = {
	{ 0, 0, PropPage::T_END }
};

AdvancedPage::ListItem AdvancedPage::listItems[] = {
	{ SettingsManager::AUTO_AWAY, ResourceManager::SETTINGS_AUTO_AWAY },
	{ SettingsManager::AUTO_FOLLOW, ResourceManager::SETTINGS_AUTO_FOLLOW },
	{ SettingsManager::CLEAR_SEARCH, ResourceManager::SETTINGS_CLEAR_SEARCH },
	{ SettingsManager::MINIMIZE_ON_STARTUP, ResourceManager::SETTINGS_MINIMIZE_ON_STARTUP },
	{ SettingsManager::REMOVE_FORBIDDEN, ResourceManager::SETCZDC_REMOVE_FORBIDDEN },
	{ SettingsManager::URL_HANDLER, ResourceManager::SETTINGS_URL_HANDLER },
	{ SettingsManager::MAGNET_REGISTER, ResourceManager::SETCZDC_MAGNET_URI_HANDLER },
	{ SettingsManager::KEEP_LISTS, ResourceManager::SETTINGS_KEEP_LISTS },
	{ SettingsManager::AUTO_KICK, ResourceManager::SETTINGS_AUTO_KICK },
	{ SettingsManager::AUTO_KICK_NO_FAVS, ResourceManager::SETTINGS_AUTO_KICK_NO_FAVS },
	{ SettingsManager::NO_AWAYMSG_TO_BOTS, ResourceManager::SETTINGS_NO_AWAYMSG_TO_BOTS },
	{ SettingsManager::ADLS_BREAK_ON_FIRST, ResourceManager::SETTINGS_ADLS_BREAK_ON_FIRST },
	{ SettingsManager::COMPRESS_TRANSFERS, ResourceManager::SETTINGS_COMPRESS_TRANSFERS },
	{ SettingsManager::HUB_USER_COMMANDS, ResourceManager::SETTINGS_HUB_USER_COMMANDS },
	{ SettingsManager::SEARCH_PASSIVE, ResourceManager::SETCZDC_PASSIVE_SEARCH },
	{ SettingsManager::SEND_UNKNOWN_COMMANDS, ResourceManager::SETTINGS_SEND_UNKNOWN_COMMANDS },
	{ SettingsManager::ADD_FINISHED_INSTANTLY, ResourceManager::ADD_FINISHED_INSTANTLY },
	{ SettingsManager::USE_CTRL_FOR_LINE_HISTORY, ResourceManager::SETTINGS_USE_CTRL_FOR_LINE_HISTORY },
	{ SettingsManager::DONT_ANNOUNCE_NEW_VERSIONS, ResourceManager::SETTINGS_DISPLAY_DC_UPDATE },
	{ SettingsManager::FILTER_ENTER, ResourceManager::SETTINGS_FILTER_ENTER },
	{ SettingsManager::SHOW_SHELL_MENU, ResourceManager::SETTINGS_SHOW_SHELL_MENU },
	{ SettingsManager::MAGNET_ASK, ResourceManager::MAGNET_ASK },
	{ SettingsManager::OPEN_LOGS_INTERNAL, ResourceManager::OPEN_LOGS_INTERNAL },
	{ SettingsManager::CORAL, ResourceManager::CORAL },
	{ SettingsManager::FAST_HASH, ResourceManager::FAST_HASH },
	{ SettingsManager::DONT_SHARE_EMPTY_DIRS, ResourceManager::DONT_SHARE_EMPTY_DIRS }, //ApexDC
	{ SettingsManager::ONLY_SHARE_FULL_DIRS, ResourceManager::ONLY_SHARE_FULL_DIRS }, //ApexDC
	{ SettingsManager::	TESTWRITE, ResourceManager::TEST_WRITE }, 
	{ SettingsManager::USE_ADLS, ResourceManager::SETTINGS_USE_ADLS },
	{ SettingsManager::USE_ADLS_OWN_LIST, ResourceManager::SETTINGS_USE_ADLS_OWN_LIST },
	{ SettingsManager::NO_ZERO_BYTE, ResourceManager::SETTINGS_NO_ZERO_BYTE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT AdvancedPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_ADVANCED_BOOLEANS));

	// Do specialized reading here
	return TRUE;
}

void AdvancedPage::write() {
	
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_ADVANCED_BOOLEANS));
}


/**
 * @file
 * $Id: AdvancedPage.cpp 425 2008-12-24 22:17:02Z BigMuscle $
 */
