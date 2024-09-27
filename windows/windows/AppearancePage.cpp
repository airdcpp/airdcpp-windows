/* 
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include <windows/stdafx.h>

#include <windows/Resource.h>
#include <windows/AppearancePage.h>
#include <windows/PropertiesDlg.h>
#include <windows/BrowseDlg.h>

#include <airdcpp/util/text/Text.h>

namespace wingui {
PropPage::TextItem AppearancePage::texts[] = {
	{ IDC_SETTINGS_APPEARANCE_OPTIONS, ResourceManager::SETTINGS_OPTIONS },
	{ IDC_SETTINGS_BOLD_CONTENTS, ResourceManager::SETTINGS_BOLD_OPTIONS },
	{ IDC_SETTINGS_TIME_STAMPS_FORMAT, ResourceManager::SETTINGS_TIME_STAMPS_FORMAT },
	{ IDC_SETTINGS_LANGUAGE_FILE, ResourceManager::SETTINGS_LANGUAGE_FILE },
	{ IDC_BROWSE, ResourceManager::BROWSE },
	{ IDC_SETTINGS_REQUIRES_RESTART, ResourceManager::SETTINGS_REQUIRES_RESTART },
	{ IDC_SETTINGS_GET_USER_COUNTRY, ResourceManager::SETTINGS_GET_USER_COUNTRY }, 
	{ IDC_SETTINGS_COUNTRY_FORMAT, ResourceManager::SETTINGS_COUNTRY_FORMAT }, 
	{ IDC_DATE_FORMAT_LBL, ResourceManager::FILE_DATE_FORMAT },
	{ 0, ResourceManager::LAST }
};

PropPage::Item AppearancePage::items[] = {
	{ IDC_TIME_STAMPS_FORMAT, SettingsManager::TIME_STAMPS_FORMAT, PropPage::T_STR },
	{ IDC_COUNTRY_FORMAT, SettingsManager::COUNTRY_FORMAT, PropPage::T_STR },
	{ IDC_LANGUAGE, SettingsManager::LANGUAGE_FILE, PropPage::T_STR },
	{ IDC_DATE_FORMAT, SettingsManager::DATE_FORMAT, PropPage::T_STR },
	{ 0, 0, PropPage::T_END }
};

PropPage::ListItem AppearancePage::listItems[] = {
	{ SettingsManager::EXPAND_BUNDLES, ResourceManager::SETTINGS_EXPAND_BUNDLES },
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
	{ SettingsManager::SINGLE_CLICK_TRAY, ResourceManager::SINGLE_CLICK_TRAY },
	{ SettingsManager::CLOSE_USE_MINIMIZE, ResourceManager::CLOSE_MINIMIZES },
	{ SettingsManager::TIME_STAMPS, ResourceManager::SETTINGS_TIME_STAMPS },
	{ SettingsManager::STATUS_IN_CHAT, ResourceManager::SETTINGS_STATUS_IN_CHAT },
	{ SettingsManager::SHOW_JOINS, ResourceManager::SETTINGS_SHOW_JOINS },
	{ SettingsManager::FAV_SHOW_JOINS, ResourceManager::SETTINGS_FAV_SHOW_JOINS },
	{ SettingsManager::SHOW_CHAT_NOTIFY, ResourceManager::CHAT_NOTIFY },
	{ SettingsManager::SORT_FAVUSERS_FIRST, ResourceManager::SETTINGS_SORT_FAVUSERS_FIRST },
	{ SettingsManager::USE_SYSTEM_ICONS, ResourceManager::SETTINGS_USE_SYSTEM_ICONS },
	{ SettingsManager::GET_USER_COUNTRY, ResourceManager::SETTINGS_GET_USER_COUNTRY },
	{ SettingsManager::SHOW_IP_COUNTRY_CHAT, ResourceManager::SETTINGS_SHOW_IP_COUNTRY_CHAT },
	{ SettingsManager::TABS_ON_TOP, ResourceManager::TABS_ON_TOP },
	{ SettingsManager::UC_SUBMENU, ResourceManager::UC_SUBMENU },
	{ SettingsManager::USE_EXPLORER_THEME, ResourceManager::USE_EXPLORER_THEME },
	{ SettingsManager::FORMAT_RELEASE, ResourceManager::FORMAT_RELEASE },
	{ SettingsManager::SORT_DIRS, ResourceManager::SORT_DIRS },
	{ SettingsManager::SHOW_EMOTICON, ResourceManager::SHOW_EMOTICON_BUTTON },
	{ SettingsManager::SHOW_MAGNET, ResourceManager::SHOW_MAGNET_BUTTON },
	{ SettingsManager::SHOW_MULTILINE, ResourceManager::SHOW_MULTILINE_BUTTON },
	{ SettingsManager::SHOW_SEND_MESSAGE, ResourceManager::SHOW_SEND_MESSAGE_BUTTON },
	{ 0, ResourceManager::LAST }
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
	const BrowseDlg::ExtensionList types[] = {
		{ _T("Language Files"), _T("*.xml") },
		{ CTSTRING(ALL_FILES), _T("*.*") }
	};

	GetDlgItemText(IDC_LANGUAGE, buf, MAX_PATH);
	tstring x = Text::toT(AppUtil::getPath(AppUtil::PATH_RESOURCES));

	BrowseDlg dlg(m_hWnd, BrowseDlg::TYPE_SETTINGS_RESOURCES, BrowseDlg::DIALOG_SELECT_FILE);
	dlg.setPath(x, true);
	dlg.setTypes(2, types);
	dlg.setOkLabel(TSTRING(SELECT));

	if (dlg.show(x)) {
		SetDlgItemText(IDC_LANGUAGE, x.c_str());
	}
	return 0;
}

LRESULT AppearancePage::onClickedTimeHelp(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	MessageBox(CTSTRING(TIMESTAMP_HELP), CTSTRING(TIMESTAMP_HELP_DESC), MB_OK | MB_ICONINFORMATION);
	return S_OK;
}

LRESULT AppearancePage::onClickedCountryHelp(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	decltype(auto) defaultFormat = SettingsManager::getInstance()->getDefault(SettingsManager::COUNTRY_FORMAT);
	MessageBox(CTSTRING_F(SETTINGS_COUNTRY_FORMAT_HELP, Text::toT(defaultFormat)), CTSTRING(SETTINGS_COUNTRY_FORMAT_HELP_DESC), MB_OK | MB_ICONINFORMATION);
	return S_OK;
}
}