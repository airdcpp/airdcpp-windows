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
#include "../client/LogManager.h"
#include "../client/File.h"

#include "Resource.h"
#include "LogPage.h"
#include "WinUtil.h"


PropPage::TextItem LogPage::texts[] = {
	{ IDC_SETTINGS_LOGGING,		ResourceManager::SETTINGS_LOGGING },
	{ IDC_SETTINGS_LOG_DIR,		ResourceManager::DIRECTORY },
	{ IDC_BROWSE_LOG,			ResourceManager::BROWSE_ACCEL },
	{ IDC_SETTINGS_FORMAT,		ResourceManager::SETTINGS_FORMAT },
	{ IDC_SETTINGS_FILE_NAME,	ResourceManager::SETTINGS_FILE_NAME },
	{ 0,						ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item LogPage::items[] = {
	{ IDC_LOG_DIRECTORY, SettingsManager::LOG_DIRECTORY, PropPage::T_STR },
	{ 0, 0, PropPage::T_END }
};

PropPage::ListItem LogPage::listItems[] = {
	{ SettingsManager::LOG_MAIN_CHAT,			ResourceManager::SETTINGS_LOG_MAIN_CHAT },
	{ SettingsManager::LOG_PRIVATE_CHAT,		ResourceManager::SETTINGS_LOG_PRIVATE_CHAT },
	{ SettingsManager::LOG_DOWNLOADS,			ResourceManager::SETTINGS_LOG_DOWNLOADS }, 
	{ SettingsManager::LOG_UPLOADS,				ResourceManager::SETTINGS_LOG_UPLOADS },
	{ SettingsManager::LOG_SYSTEM,				ResourceManager::SETTINGS_LOG_SYSTEM_MESSAGES },
	{ SettingsManager::LOG_STATUS_MESSAGES,		ResourceManager::SETTINGS_LOG_STATUS_MESSAGES },
	{ SettingsManager::LOG_FILELIST_TRANSFERS, ResourceManager::SETTINGS_LOG_FILELIST_TRANSFERS },
	{ 0,										ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::ListItem LogPage::systemItems[] = {
	//ToDo add more options
	{ SettingsManager::SYSTEM_SHOW_UPLOADS,			ResourceManager::SYSTEM_SHOW_FINISHED_UPLOADS },
	{ SettingsManager::SYSTEM_SHOW_DOWNLOADS,		ResourceManager::SYSTEM_SHOW_FINISHED_DOWNLOADS },
	{ SettingsManager::REPORT_ALTERNATES,			ResourceManager::REPORT_ALTERNATES }, 
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT LogPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_LOG_OPTIONS));
	PropPage::read((HWND)*this, items, systemItems, GetDlgItem(IDC_SYSTEM_LIST));

	for(int i = 0; i < LogManager::LAST; ++i) {
		TStringPair pair;
		pair.first = Text::toT(LogManager::getInstance()->getSetting(i, LogManager::FILE));
		pair.second = Text::toT(LogManager::getInstance()->getSetting(i, LogManager::FORMAT));
		options.push_back(pair);
	}
	
	::EnableWindow(GetDlgItem(IDC_LOG_FORMAT), false);
	::EnableWindow(GetDlgItem(IDC_LOG_FILE), false);

	oldSelection = -1;
	
	// Do specialized reading here
	return TRUE;
}

LRESULT LogPage::onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
	logOptions.Attach(GetDlgItem(IDC_LOG_OPTIONS));

	
	getValues();
	
	int sel = logOptions.GetSelectedIndex();
		
	if(sel >= 0 && sel < LogManager::LAST) {
		BOOL checkState = logOptions.GetCheckState(sel) == BST_CHECKED ? TRUE : FALSE;
				
		::EnableWindow(GetDlgItem(IDC_LOG_FORMAT), checkState);
		::EnableWindow(GetDlgItem(IDC_LOG_FILE), checkState);
				
		SetDlgItemText(IDC_LOG_FILE, options[sel].first.c_str());
		SetDlgItemText(IDC_LOG_FORMAT, options[sel].second.c_str());
	
		//save the old selection so we know where to save the values
		oldSelection = sel;
	} else {
		::EnableWindow(GetDlgItem(IDC_LOG_FORMAT), FALSE);
		::EnableWindow(GetDlgItem(IDC_LOG_FILE), FALSE);

		SetDlgItemText(IDC_LOG_FILE, _T(""));
		SetDlgItemText(IDC_LOG_FORMAT, _T(""));
	}
		
	logOptions.Detach();
	return 0;
}

void LogPage::getValues() {
	if(oldSelection >= 0) {
		TCHAR buf[512];

		if(GetDlgItemText(IDC_LOG_FILE, buf, 512) > 0)
			options[oldSelection].first = buf;
		if(GetDlgItemText(IDC_LOG_FORMAT, buf, 512) > 0)
			options[oldSelection].second = buf;
	}
}

void LogPage::write()
{
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_LOG_OPTIONS));
	PropPage::write((HWND)*this, items, systemItems, GetDlgItem(IDC_SYSTEM_LIST));

	const string& s = SETTING(LOG_DIRECTORY);
	if(s.length() > 0 && s[s.length() - 1] != '\\') {
		SettingsManager::getInstance()->set(SettingsManager::LOG_DIRECTORY, s + '\\');
	}
	File::ensureDirectory(SETTING(LOG_DIRECTORY));

	//make sure we save the last edit too, the user
	//might not have changed the selection
	getValues();

	for(int i = 0; i < LogManager::LAST; ++i) {
		string tmp = Text::fromT(options[i].first);
		if(stricmp(Util::getFileExt(tmp), ".log") != 0)
			tmp += ".log";

		LogManager::getInstance()->saveSetting(i, LogManager::FILE, tmp);
		LogManager::getInstance()->saveSetting(i, LogManager::FORMAT, Text::fromT(options[i].second));
	}
}

LRESULT LogPage::onClickedBrowseDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	tstring dir = Text::toT(SETTING(LOG_DIRECTORY));
	if(WinUtil::browseDirectory(dir, m_hWnd))
	{
		// Adjust path string
		if(dir.size() > 0 && dir[dir.size() - 1] != '\\')
			dir += '\\';
		
		SetDlgItemText(IDC_LOG_DIRECTORY, dir.c_str());
	}
	return 0;
}

/**
 * @file
 * $Id: LogPage.cpp 373 2008-02-06 17:23:49Z bigmuscle $
 */
