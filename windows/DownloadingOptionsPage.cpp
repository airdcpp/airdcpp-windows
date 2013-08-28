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

#include "DownloadingOptionsPage.h"
#include "LineDlg.h"

#include "WinUtil.h"
#include "PropertiesDlg.h"

PropPage::TextItem DownloadingOptionsPage::texts[] = {
	{ IDC_SETTINGS_KBPS5, RM_KBS },
	{ IDC_SETTINGS_KBPS6, RM_KBS },
	{ IDC_SETTINGS_KBPS7, RM_KBS },
	{ IDC_SETTINGS_KIB, RM_KB },
	{ IDC_MIN_DUPE_SIZE_LBL, ResourceManager::MIN_DUPE_CHECK_SIZE },

	{ IDC_SETTINGS_MINUTES, ResourceManager::SECONDS },
	{ IDC_MONITOR_SLOW_SPEED, ResourceManager::SETTINGS_AUTO_DROP_SLOW_SOURCES },
	{ IDC_CZDC_SLOW_DISCONNECT, ResourceManager::SETCZDC_SLOW_DISCONNECT },
	{ IDC_CZDC_I_DOWN_SPEED, ResourceManager::SETCZDC_I_DOWN_SPEED },
	{ IDC_CZDC_TIME_DOWN, ResourceManager::SETCZDC_TIME_DOWN },
	{ IDC_CZDC_H_DOWN_SPEED, ResourceManager::DISCONNECT_BUNDLE_SPEED },
	{ IDC_RUNNING_DOWNLOADS_LABEL, ResourceManager::DISCONNECT_RUNNING_DOWNLOADS },
	{ IDC_DISCONNECTING_ENABLE, ResourceManager::SETCZDC_DISCONNECTING_ENABLE },
	{ IDC_CZDC_MIN_FILE_SIZE, ResourceManager::SETCZDC_MIN_FILE_SIZE },
	{ IDC_SETTINGS_MB, RM_MB },
	{ IDC_REMOVE_IF, ResourceManager::NEW_DISCONNECT },
	{ IDC_AUTO_DISCONNECT_MODE_LBL, ResourceManager::REMOVE_SOURCE_FROM },

	{ IDC_SB_SKIPLIST_DOWNLOAD,	ResourceManager::SETTINGS_SKIPLIST_DOWNLOAD	},
	{ IDC_DL_SKIPPING_OPTIONS,	ResourceManager::SETTINGS_SKIPPING_OPTIONS	},
	{ IDC_DOWNLOAD_SKIPLIST_USE_REGEXP, ResourceManager::USE_REGEXP },
	{ IDC_BUNDLE_RECENT_HOURS_LABEL, ResourceManager::SETTINGS_RECENT_HOURS },
	{ IDC_HOURS, ResourceManager::HOURS_LOWER },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item DownloadingOptionsPage::items[] = {
	{ IDC_I_DOWN_SPEED, SettingsManager::DISCONNECT_SPEED, PropPage::T_INT },
	{ IDC_TIME_DOWN, SettingsManager::DISCONNECT_TIME, PropPage::T_INT },
	{ IDC_H_DOWN_SPEED, SettingsManager::DISCONNECT_FILE_SPEED, PropPage::T_INT },
	{ IDC_MIN_FILE_SIZE, SettingsManager::DISCONNECT_FILESIZE, PropPage::T_INT },
	{ IDC_REMOVE_IF_BELOW, SettingsManager::REMOVE_SPEED, PropPage::T_INT },
	{ IDC_DISCONNECT_ONLINE_SOURCES, SettingsManager::DISCONNECT_MIN_SOURCES, PropPage::T_INT },
	{ IDC_SKIPLIST_DOWNLOAD, SettingsManager::SKIPLIST_DOWNLOAD, PropPage::T_STR },
	{ IDC_DOWNLOAD_SKIPLIST_USE_REGEXP, SettingsManager::DOWNLOAD_SKIPLIST_USE_REGEXP, PropPage::T_BOOL },
	{ IDC_USE_DISCONNECT_DEFAULT, SettingsManager::USE_SLOW_DISCONNECTING_DEFAULT, PropPage::T_BOOL },
	{ IDC_BUNDLE_RECENT_HOURS, SettingsManager::RECENT_BUNDLE_HOURS, PropPage::T_INT },
	{ IDC_MIN_DUPE_CHECK_SIZE, SettingsManager::MIN_DUPE_CHECK_SIZE, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

PropPage::ListItem DownloadingOptionsPage::optionItems[] = {
	{ SettingsManager::SKIP_ZERO_BYTE, ResourceManager::SETTINGS_SKIP_ZERO_BYTE },
	{ SettingsManager::DONT_DL_ALREADY_SHARED, ResourceManager::SETTINGS_DONT_DL_ALREADY_SHARED },
	{ SettingsManager::DONT_DL_ALREADY_QUEUED, ResourceManager::SETTING_DONT_DL_ALREADY_QUEUED },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT DownloadingOptionsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);
	PropPage::read((HWND)*this, items, optionItems, GetDlgItem(IDC_OTHER_SKIPPING_OPTIONS));

	setMinMax(IDC_I_DOWN_SPEED_SPIN, 0, 99999);
	setMinMax(IDC_TIME_DOWN_SPIN, 1, 99999);
	setMinMax(IDC_H_DOWN_SPEED_SPIN, 0, 99999);
	setMinMax(IDC_MIN_FILE_SIZE_SPIN, 0, 99999);
	setMinMax(IDC_REMOVE_SPIN, 0, 99999);
	setMinMax(IDC_SOURCES_SPIN, 1, 99999);
	setMinMax(IDC_RECENT_SPIN, 0, 99999);

	disconnectMode.Attach(GetDlgItem(IDC_AUTO_DISCONNECT_MODE));
	disconnectMode.AddString(CTSTRING(FILE));
	disconnectMode.AddString(CTSTRING(BUNDLE));
	disconnectMode.AddString(CTSTRING(WHOLE_QUEUE));
	/*disconnectMode.AddString(Text::toLower(TSTRING(FILE)).c_str());
	disconnectMode.AddString(Text::toLower(TSTRING(BUNDLE)).c_str());
	disconnectMode.AddString(Text::toLower(TSTRING(WHOLE_QUEUE)).c_str());*/
	disconnectMode.SetCurSel(SETTING(DL_AUTO_DISCONNECT_MODE));

	// Do specialized reading here
	return TRUE;
}

void DownloadingOptionsPage::write() {

	PropPage::write((HWND)*this, items);
	PropPage::write((HWND)*this, items, optionItems, GetDlgItem(IDC_OTHER_SKIPPING_OPTIONS));

	SettingsManager::getInstance()->set(SettingsManager::DL_AUTO_DISCONNECT_MODE, disconnectMode.GetCurSel());
}



