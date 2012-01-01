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

#include "Resource.h"
#include "DownloadPage.h"
#include "WinUtil.h"
#include "PublicHubsListDlg.h"
#include "MainFrm.h"
#include "PropertiesDlg.h"

PropPage::TextItem DownloadPage::texts[] = {
	{ IDC_SETTINGS_DIRECTORIES, ResourceManager::SETTINGS_DIRECTORIES }, 
	{ IDC_SETTINGS_DOWNLOAD_DIRECTORY, ResourceManager::SETTINGS_DOWNLOAD_DIRECTORY },
	{ IDC_BROWSEDIR, ResourceManager::BROWSE_ACCEL },
	{ IDC_SETTINGS_UNFINISHED_DOWNLOAD_DIRECTORY, ResourceManager::SETTINGS_UNFINISHED_DOWNLOAD_DIRECTORY }, 
	{ IDC_BROWSETEMPDIR, ResourceManager::BROWSE },
	{ IDC_SETTINGS_BTN_TARGETDRIVE, ResourceManager::SETTINGS_USE_TARGETDRIVE }, 
	{ IDC_SETTINGS_TARGETDRIVE_NOTE, ResourceManager::SETTINGS_TARGETDRIVE_NOTE }, 
	{ IDC_SETTINGS_PUBLIC_HUB_LIST, ResourceManager::SETTINGS_PUBLIC_HUB_LIST },
	{ IDC_SETTINGS_PUBLIC_HUB_LIST_URL, ResourceManager::SETTINGS_PUBLIC_HUB_LIST_URL },
	{ IDC_SETTINGS_LIST_CONFIG, ResourceManager::SETTINGS_CONFIGURE_HUB_LISTS },
	{ IDC_SETTINGS_PUBLIC_HUB_LIST_HTTP_PROXY, ResourceManager::SETTINGS_PUBLIC_HUB_LIST_HTTP_PROXY },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item DownloadPage::items[] = {
	{ IDC_TEMP_DOWNLOAD_DIRECTORY, SettingsManager::TEMP_DOWNLOAD_DIRECTORY, PropPage::T_STR },
	{ IDC_DOWNLOADDIR,	SettingsManager::DOWNLOAD_DIRECTORY, PropPage::T_STR }, 
	{ IDC_PROXY, SettingsManager::HTTP_PROXY, PropPage::T_STR },
	{ 0, 0, PropPage::T_END }
};


PropPage::ListItem DownloadPage::optionItems[] = {
	{ SettingsManager::SCAN_DL_BUNDLES, ResourceManager::SETTINGS_SCAN_FINISHED_BUNDLES },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT DownloadPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);
	PropPage::read((HWND)*this, items, optionItems, GetDlgItem(IDC_DOWNLOAD_OPTIONS));
	// Do specialized reading here
	return TRUE;
}

void DownloadPage::write()
{
	PropPage::write((HWND)*this, items);
	PropPage::write((HWND)*this, items, optionItems, GetDlgItem(IDC_DOWNLOAD_OPTIONS));

	const string& s = SETTING(DOWNLOAD_DIRECTORY);
	if(s.length() > 0 && s[s.length() - 1] != '\\') {
		SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_DIRECTORY, s + '\\');
	}
	const string& t = SETTING(TEMP_DOWNLOAD_DIRECTORY);
	if(t.length() > 0 && t[t.length() - 1] != '\\') {
		SettingsManager::getInstance()->set(SettingsManager::TEMP_DOWNLOAD_DIRECTORY, t + '\\');
		
	}
	AirUtil::updateCachedSettings();
}

LRESULT DownloadPage::onClickedBrowseDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	tstring dir = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
	if(WinUtil::browseDirectory(dir, m_hWnd))
	{
		// Adjust path string
		if(dir.size() > 0 && dir[dir.size() - 1] != '\\')
			dir += '\\';
	
		SetDlgItemText(IDC_DOWNLOADDIR, dir.c_str());
	}
	return 0;
}

LRESULT DownloadPage::onClickedBrowseTempDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	tstring dir = Text::toT(SETTING(TEMP_DOWNLOAD_DIRECTORY));
	if(WinUtil::browseDirectory(dir, m_hWnd))
	{
		// Adjust path string
		if(dir.size() > 0 && dir[dir.size() - 1] != '\\')
			dir += '\\';

		SetDlgItemText(IDC_TEMP_DOWNLOAD_DIRECTORY, dir.c_str());
	}
	return 0;
}

LRESULT DownloadPage::onClickedTargetdrive(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	SetDlgItemText(IDC_TEMP_DOWNLOAD_DIRECTORY, _T("%[targetdrive]DCUnfinished"));
	return 0;
}

LRESULT DownloadPage::onClickedListConfigure(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PublicHubListDlg dlg;
	dlg.DoModal(m_hWnd);
	return 0;
}

/**
 * @file
 * $Id: DownloadPage.cpp 412 2008-07-23 22:35:40Z BigMuscle $
 */
