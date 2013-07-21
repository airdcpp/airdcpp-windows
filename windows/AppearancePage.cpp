/* 
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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
#include "AppearancePage.h"
#include "WinUtil.h"
#include "PropertiesDlg.h"

PropPage::TextItem AppearancePage::texts[] = {
	{ IDC_SETTINGS_APPEARANCE_OPTIONS, ResourceManager::SETTINGS_OPTIONS },
	{ IDC_SETTINGS_BOLD_CONTENTS, ResourceManager::SETTINGS_BOLD_OPTIONS },
	{ IDC_SETTINGS_TIME_STAMPS_FORMAT, ResourceManager::SETTINGS_TIME_STAMPS_FORMAT },
	{ IDC_SETTINGS_LANGUAGE_FILE, ResourceManager::SETTINGS_LANGUAGE_FILE },
	{ IDC_BROWSE, ResourceManager::BROWSE_ACCEL },
	{ IDC_SETTINGS_REQUIRES_RESTART, ResourceManager::SETTINGS_REQUIRES_RESTART },
	{ IDC_SETTINGS_GET_USER_COUNTRY, ResourceManager::SETTINGS_GET_USER_COUNTRY }, 
	{ IDC_SETTINGS_COUNTRY_FORMAT, ResourceManager::SETTINGS_COUNTRY_FORMAT }, 
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item AppearancePage::items[] = {
	{ IDC_TIME_STAMPS_FORMAT, SettingsManager::TIME_STAMPS_FORMAT, PropPage::T_STR },
	{ IDC_COUNTRY_FORMAT, SettingsManager::COUNTRY_FORMAT, PropPage::T_STR },
	{ IDC_LANGUAGE, SettingsManager::LANGUAGE_FILE, PropPage::T_STR },
	{ 0, 0, PropPage::T_END }
};

PropPage::ListItem AppearancePage::listItems[] = {
	{ SettingsManager::EXPAND_BUNDLES, ResourceManager::SETTINGS_EXPAND_BUNDLES },
	{ SettingsManager::NAT_SORT, ResourceManager::NAT_SORT },
	{ SettingsManager::CLIENT_COMMANDS , ResourceManager::SET_CLIENT_COMMANDS },
	{ SettingsManager::SERVER_COMMANDS , ResourceManager::SETTINGS_SERVER_COMMANDS },
	{ SettingsManager::FLASH_WINDOW_ON_PM, ResourceManager::FLASH_WINDOW_ON_PM },
	{ SettingsManager::FLASH_WINDOW_ON_NEW_PM, ResourceManager::FLASH_WINDOW_ON_NEW_PM },
	{ SettingsManager::FLASH_WINDOW_ON_MYNICK, ResourceManager::FLASH_WINDOW_ON_MYNICK },
	{ SettingsManager::SHOW_QUEUE_BARS, ResourceManager::SETTINGS_SHOW_QUEUE_BARS },
	{ SettingsManager::SHOW_PROGRESS_BARS, ResourceManager::SETTINGS_SHOW_PROGRESS_BARS },
	{ SettingsManager::SHOW_INFOTIPS, ResourceManager::SETTINGS_SHOW_INFO_TIPS },
	{ SettingsManager::FILTER_MESSAGES, ResourceManager::SETTINGS_FILTER_MESSAGES },
	{ SettingsManager::MINIMIZE_TRAY, ResourceManager::SETTINGS_MINIMIZE_TRAY },
	{ SettingsManager::TIME_STAMPS, ResourceManager::SETTINGS_TIME_STAMPS },
	{ SettingsManager::STATUS_IN_CHAT, ResourceManager::SETTINGS_STATUS_IN_CHAT },
	{ SettingsManager::SHOW_JOINS, ResourceManager::SETTINGS_SHOW_JOINS },
	{ SettingsManager::FAV_SHOW_JOINS, ResourceManager::SETTINGS_FAV_SHOW_JOINS },
	{ SettingsManager::SHOW_CHAT_NOTIFY, ResourceManager::CHAT_NOTIFY },
	{ SettingsManager::EXPAND_QUEUE , ResourceManager::SETTINGS_EXPAND_QUEUE },
	{ SettingsManager::SORT_FAVUSERS_FIRST, ResourceManager::SETTINGS_SORT_FAVUSERS_FIRST },
	{ SettingsManager::USE_SYSTEM_ICONS, ResourceManager::SETTINGS_USE_SYSTEM_ICONS },
	{ SettingsManager::GET_USER_COUNTRY, ResourceManager::SETTINGS_GET_USER_COUNTRY },
	{ SettingsManager::SUPPRESS_MAIN_CHAT, ResourceManager::SETTINGS_ADVANCED_SUPPRESS_MAIN_CHAT },
	{ SettingsManager::TABS_ON_TOP, ResourceManager::TABS_ON_TOP },
	{ SettingsManager::UC_SUBMENU, ResourceManager::UC_SUBMENU },
	{ SettingsManager::USE_EXPLORER_THEME, ResourceManager::USE_EXPLORER_THEME },
	{ SettingsManager::FORMAT_RELEASE, ResourceManager::FORMAT_RELEASE },
	{ SettingsManager::SORT_DIRS, ResourceManager::SORT_DIRS },
	{ SettingsManager::HORIZONTAL_QUEUE, ResourceManager::HORIZONTAL_QUEUE },
	{ SettingsManager::SHOW_EMOTICON, ResourceManager::SHOW_EMOTICON_BUTTON },
	{ SettingsManager::SHOW_MAGNET, ResourceManager::SHOW_MAGNET_BUTTON },
	{ SettingsManager::SHOW_MULTILINE, ResourceManager::SHOW_MULTILINE_BUTTON },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};



AppearancePage::~AppearancePage(){ }

void AppearancePage::write() {
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_APPEARANCE_BOOLEANS));
}



LRESULT AppearancePage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);

	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_APPEARANCE_BOOLEANS));


	// Do specialized reading here
	return TRUE;
}

LRESULT AppearancePage::onBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TCHAR buf[MAX_PATH];
	static const TCHAR types[] = _T("Language Files\0*.xml\0All Files\0*.*\0");

	GetDlgItemText(IDC_LANGUAGE, buf, MAX_PATH);
	tstring x = buf;

	if (WinUtil::browseFile(x, m_hWnd, false, Text::toT(Util::getPath(Util::PATH_RESOURCES)), Util::emptyStringT, types) == IDOK) {
		SetDlgItemText(IDC_LANGUAGE, x.c_str());
	}
	return 0;
}

LRESULT AppearancePage::onClickedTimeHelp(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	MessageBox(CTSTRING(TIMESTAMP_HELP), CTSTRING(TIMESTAMP_HELP_DESC), MB_OK | MB_ICONINFORMATION);
	return S_OK;
}

LRESULT AppearancePage::onClickedCountryHelp(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	MessageBox(CTSTRING(SETTINGS_COUNTRY_FORMAT_HELP), CTSTRING(SETTINGS_COUNTRY_FORMAT_HELP_DESC), MB_OK | MB_ICONINFORMATION);
	return S_OK;
}