/* 
 * Copyright (C) 2001-2021 Jacek Sieka, arnetheduck on gmail point com
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
#include "AdvancedPage.h"
#include "PropertiesDlg.h"

PropPage::TextItem AdvancedPage::texts[] = {
	{ 0, ResourceManager::LAST }
};

PropPage::Item AdvancedPage::items[] = {
	{ 0, 0, PropPage::T_END }
};

AdvancedPage::ListItem AdvancedPage::listItems[] = {
	{ SettingsManager::AUTO_FOLLOW, ResourceManager::SETTINGS_AUTO_FOLLOW },
	//{ SettingsManager::SEPARATE_NOSHARE_HUBS, ResourceManager::SEPARATE_NOSHARE_HUBS },
	{ SettingsManager::DISALLOW_CONNECTION_TO_PASSED_HUBS, ResourceManager::DISALLOW_CONNECTION_TO_PASSED_HUBS },
	{ SettingsManager::CLEAR_SEARCH, ResourceManager::SETTINGS_CLEAR_SEARCH },
	{ SettingsManager::MINIMIZE_ON_STARTUP, ResourceManager::SETTINGS_MINIMIZE_ON_STARTUP },
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
	{ SettingsManager::USE_CTRL_FOR_LINE_HISTORY, ResourceManager::SETTINGS_USE_CTRL_FOR_LINE_HISTORY },
	{ SettingsManager::MAGNET_ASK, ResourceManager::MAGNET_ASK },
	{ SettingsManager::OPEN_LOGS_INTERNAL, ResourceManager::OPEN_LOGS_INTERNAL },
	{ SettingsManager::TESTWRITE, ResourceManager::TEST_WRITE }, 
	{ SettingsManager::USE_ADLS, ResourceManager::SETTINGS_USE_ADLS },
	{ SettingsManager::USE_PARTIAL_SHARING, ResourceManager::PARTIAL_SHARING },
	{ SettingsManager::UPDATE_IP_HOURLY, ResourceManager::UPDATE_IP_EVERY },
	{ SettingsManager::SEARCH_SAVE_HUBS_STATE, ResourceManager::SAVE_HUBS_STATE },
	{ SettingsManager::FREE_SPACE_WARN, ResourceManager::SETTINGS_USE_SPACE_WARNING },
	{ SettingsManager::NMDC_MAGNET_WARN, ResourceManager::SETTINGS_NMDC_MAGNET_WARNING },
	{ SettingsManager::NFO_EXTERNAL, ResourceManager::OPEN_NFO_EXTERNAL },
	{ 0, ResourceManager::LAST }
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