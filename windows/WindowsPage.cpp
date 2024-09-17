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

#include "stdafx.h"

#include "Resource.h"
#include "WindowsPage.h"

namespace wingui {
PropPage::Item WindowsPage::items[] = { { 0, 0, PropPage::T_END } };

PropPage::TextItem WindowsPage::textItem[] = {
	{ IDC_SETTINGS_AUTO_OPEN, ResourceManager::SETTINGS_AUTO_OPEN },
	{ IDC_SETTINGS_WINDOWS_OPTIONS, ResourceManager::SETTINGS_WINDOWS_OPTIONS },
	{ IDC_SETTINGS_CONFIRM_OPTIONS, ResourceManager::SETTINGS_CONFIRM_DIALOG_OPTIONS },
	{ 0, ResourceManager::LAST }
};

WindowsPage::ListItem WindowsPage::listItems[] = {
	{ SettingsManager::OPEN_AUTOSEARCH, ResourceManager::AUTO_SEARCH },
	{ SettingsManager::OPEN_QUEUE, ResourceManager::DOWNLOAD_QUEUE },
	{ SettingsManager::OPEN_FAVORITE_HUBS, ResourceManager::FAVORITE_HUBS },
	{ SettingsManager::OPEN_FAVORITE_USERS, ResourceManager::USERS },	
	{ SettingsManager::OPEN_NOTEPAD, ResourceManager::NOTEPAD },
	{ SettingsManager::OPEN_SYSTEM_LOG, ResourceManager::SYSTEM_LOG },
	{ SettingsManager::OPEN_WAITING_USERS, ResourceManager::UPLOAD_QUEUE },
	{ SettingsManager::OPEN_FINISHED_UPLOADS, ResourceManager::FINISHED_UPLOADS },
	{ SettingsManager::OPEN_PUBLIC, ResourceManager::PUBLIC_HUBS },
	{ SettingsManager::OPEN_SEARCH_SPY, ResourceManager::SEARCH_SPY },
	{ 0, ResourceManager::LAST }
};

WindowsPage::ListItem WindowsPage::optionItems[] = {
	{ SettingsManager::POPUP_HUB_PMS, ResourceManager::SETTINGS_POPUP_HUB_PMS },
	{ SettingsManager::POPUP_BOT_PMS, ResourceManager::SETTINGS_POPUP_BOT_PMS },
	{ SettingsManager::POPUNDER_FILELIST, ResourceManager::SETTINGS_POPUNDER_FULL_LIST },
	{ SettingsManager::POPUNDER_PARTIAL_LIST, ResourceManager::SETTINGS_POPUNDER_PARTIAL_LIST },
	{ SettingsManager::POPUNDER_PM, ResourceManager::SETTINGS_POPUNDER_PM },
	{ SettingsManager::POPUNDER_TEXT, ResourceManager::SETTINGS_POPUNDER_TEXT },
	{ SettingsManager::TOGGLE_ACTIVE_WINDOW, ResourceManager::SETTINGS_TOGGLE_ACTIVE_WINDOW },
	{ SettingsManager::PROMPT_PASSWORD, ResourceManager::SETTINGS_PROMPT_PASSWORD },
	{ 0, ResourceManager::LAST }
};

WindowsPage::ListItem WindowsPage::confirmItems[] = {
	{ SettingsManager::CONFIRM_EXIT, ResourceManager::SETTINGS_CONFIRM_EXIT },
	{ SettingsManager::CONFIRM_HUB_REMOVAL, ResourceManager::SETTINGS_CONFIRM_HUB_REMOVAL },
	{ SettingsManager::CONFIRM_QUEUE_REMOVAL, ResourceManager::SETTINGS_CONFIRM_ITEM_REMOVAL },
	{ SettingsManager::CONFIRM_HUB_CLOSING, ResourceManager::SETTINGS_CONFIRM_HUB_CLOSE },
	{ SettingsManager::CONFIRM_AS_REMOVAL, ResourceManager::SETTINGS_CONFIRM_AS_REMOVE },
	{ SettingsManager::CONFIRM_FILE_DELETIONS, ResourceManager::CONFIRM_FILE_DELETIONS },
	{ 0, ResourceManager::LAST }
};

LRESULT WindowsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), textItem);
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_WINDOWS_STARTUP));
	PropPage::read((HWND)*this, items, optionItems, GetDlgItem(IDC_WINDOWS_OPTIONS));
	PropPage::read((HWND)*this, items, confirmItems, GetDlgItem(IDC_CONFIRM_OPTIONS));

	::SetWindowText(GetDlgItem(IDC_SAVE_LAST_STATE), CTSTRING(SAVE_LAST_STATE));
	CheckDlgButton(IDC_SAVE_LAST_STATE, SETTING(SAVE_LAST_STATE));
	fixControls();

	// Do specialized reading here
	return TRUE;
}
void WindowsPage::fixControls() {
	::EnableWindow(GetDlgItem(IDC_WINDOWS_STARTUP), IsDlgButtonChecked(IDC_SAVE_LAST_STATE) != BST_CHECKED);
}

void WindowsPage::write() {
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_WINDOWS_STARTUP));
	PropPage::write((HWND)*this, items, optionItems, GetDlgItem(IDC_WINDOWS_OPTIONS));
	PropPage::write((HWND)*this, items, confirmItems, GetDlgItem(IDC_CONFIRM_OPTIONS));
	SettingsManager::getInstance()->set(SettingsManager::SAVE_LAST_STATE, IsDlgButtonChecked(IDC_SAVE_LAST_STATE) == BST_CHECKED);

}
}