/* 
 * Copyright (C) 2011-2013 AirDC++ Project
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

#include "ScanPage.h"

#include "WinUtil.h"
#include "PropertiesDlg.h"


PropPage::ListItem ScanPage::listItems[] = {
	{ SettingsManager::CHECK_MISSING, ResourceManager::CHECK_MISSING },
	{ SettingsManager::CHECK_SFV, ResourceManager::CHECK_SFV },
	{ SettingsManager::CHECK_NFO, ResourceManager::CHECK_NFO },
	{ SettingsManager::CHECK_MP3_DIR, ResourceManager::CHECK_MP3_DIR },
	{ SettingsManager::CHECK_EXTRA_SFV_NFO, ResourceManager::CHECK_EXTRA_SFV_NFO },
	{ SettingsManager::CHECK_EXTRA_FILES, ResourceManager::CHECK_EXTRA_FILES },
	{ SettingsManager::CHECK_DUPES, ResourceManager::CHECK_DUPES },
	{ SettingsManager::CHECK_EMPTY_DIRS, ResourceManager::CHECK_EMPTY_DIRS },
	{ SettingsManager::CHECK_EMPTY_RELEASES, ResourceManager::CHECK_EMPTY_RELEASES },
	{ SettingsManager::CHECK_USE_SKIPLIST, ResourceManager::CHECK_USE_SKIPLIST },
	{ SettingsManager::CHECK_IGNORE_ZERO_BYTE, ResourceManager::CHECK_IGNORE_ZERO_BYTE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::ListItem ScanPage::bundleItems[] = {
	{ SettingsManager::SCAN_DL_BUNDLES, ResourceManager::SETTINGS_SCAN_FINISHED_BUNDLES },
	{ SettingsManager::AUTO_COMPLETE_BUNDLES, ResourceManager::SETTINGS_AUTO_COMPLETE_BUNDLES },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::TextItem ScanPage::texts[] = {
	{ IDC_BUNDLE_OPTIONS_LBL, ResourceManager::BUNDLES },
	{ IDC_LOG_SCAN_PATH_LBL, ResourceManager::LOG_SHARE_SCAN_PATH },
	{ IDC_LOG_SCANS, ResourceManager::LOG_SHARE_SCAN },
	{ IDC_SETTINGS_LOGGING, ResourceManager::SETTINGS_LOGGING },
	{ IDC_SETTINGS_SCAN_OPTIONS, ResourceManager::SETTINGS_SCAN_OPTIONS },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item ScanPage::items[] = {
	{ IDC_SCAN_LOG_PATH, SettingsManager::LOG_SHARE_SCAN_PATH, PropPage::T_STR },
	{ IDC_LOG_SCANS, SettingsManager::LOG_SHARE_SCANS, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};



LRESULT ScanPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_SCANLIST));
	PropPage::read((HWND)*this, items, bundleItems, GetDlgItem(IDC_BUNDLE_OPTIONS));

	// Do specialized reading here

	fixControls();
	return TRUE;
}

void ScanPage::write() {
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_SCANLIST));
	PropPage::write((HWND)*this, items, bundleItems, GetDlgItem(IDC_BUNDLE_OPTIONS));
}
 
void ScanPage::fixControls() {
	BOOL log = IsDlgButtonChecked(IDC_LOG_SCANS) == BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_SCAN_LOG_PATH),			log);
	::EnableWindow(GetDlgItem(IDC_LOG_SCAN_PATH_LBL),		log);
}