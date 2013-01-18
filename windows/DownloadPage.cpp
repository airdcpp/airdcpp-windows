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
#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"

#include "Resource.h"

#include "DownloadPage.h"
#include "CommandDlg.h"
#include "WinUtil.h"
#include "PropertiesDlg.h"

PropPage::TextItem DownloadPage::texts[] = {
	{ IDC_AUTO_SEARCH_ALT, ResourceManager::SETTINGS_AUTO_BUNDLE_SEARCH },
	{ IDC_DONTBEGIN, ResourceManager::DONT_ADD_SEGMENT_TEXT },
	{ IDC_ENABLE_SEGMENTS, ResourceManager::ENABLE_MULTI_SOURCE },
	{ IDC_MINUTES, ResourceManager::MINUTES },
	{ IDC_KBPS, ResourceManager::KBPS },
	{ IDC_CHUNKCOUNT, ResourceManager::TEXT_MANUAL },
	{ IDC_SETTINGS_SEGMENTS, ResourceManager::SETTINGS_SEGMENTED_DOWNLOADS },
	{ IDC_MIN_SEGMENT_SIZE_LABEL, ResourceManager::SETTINGS_AIRDOWNLOADS_SEGMENT_SIZE },
	{ IDC_SETTINGS_KIB, ResourceManager::KiB },
	{ IDC_SETTINGS_AUTO_SEARCH_LIMIT, ResourceManager::SETTINGS_AUTO_SEARCH_LIMIT },
	{ IDC_MATCH_QUEUE_TEXT, ResourceManager::SETTINGS_SB_MAX_SOURCES },
	{ IDC_SETTINGS_SEARCH_MATCHING, ResourceManager::SETTINGS_SEARCH_MATCHING },
	{ IDC_INTERVAL_TEXT, ResourceManager::MINIMUM_SEARCH_INTERVAL },
	{ IDC_AUTO_ADD_SOURCES, ResourceManager::AUTO_ADD_SOURCE },
	{ IDC_ALLOW_MATCH_FULL, ResourceManager::SETTINGS_ALLOW_MATCH_FULL_LIST },
	{ IDC_OTHER_QUEUE_OPTIONS, ResourceManager::FINISHED_DOWNLOADS },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item DownloadPage::items[] = {
	{ IDC_AUTO_SEARCH_ALT, SettingsManager::AUTO_SEARCH, PropPage::T_BOOL },
	{ IDC_DONTBEGIN, SettingsManager::DONT_BEGIN_SEGMENT, PropPage::T_BOOL },
	{ IDC_DONTBEGIN_EDIT, SettingsManager::DONT_BEGIN_SEGMENT_SPEED, PropPage::T_INT },
	{ IDC_SEARCH_INTERVAL, SettingsManager::SEARCH_TIME, PropPage::T_INT },
	{ IDC_CHUNKCOUNT, SettingsManager::SEGMENTS_MANUAL, PropPage::T_BOOL },
	{ IDC_SEG_NUMBER, SettingsManager::NUMBER_OF_SEGMENTS, PropPage::T_INT },
	{ IDC_MIN_SEGMENT_SIZE, SettingsManager::MIN_SEGMENT_SIZE, PropPage::T_INT },
	{ IDC_ENABLE_SEGMENTS, SettingsManager::MULTI_CHUNK, PropPage::T_BOOL },
	{ IDC_MATCH, SettingsManager::MAX_AUTO_MATCH_SOURCES, PropPage::T_INT },
	{ IDC_AUTO_SEARCH_LIMIT, SettingsManager::AUTO_SEARCH_LIMIT, PropPage::T_INT },
	{ IDC_AUTO_ADD_SOURCES, SettingsManager::AUTO_ADD_SOURCE, PropPage::T_BOOL },
	{ IDC_ALLOW_MATCH_FULL, SettingsManager::ALLOW_MATCH_FULL_LIST, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

PropPage::ListItem DownloadPage::optionItems[] = {
	{ SettingsManager::ADD_FINISHED_INSTANTLY, ResourceManager::ADD_FINISHED_INSTANTLY },
	{ SettingsManager::SCAN_DL_BUNDLES, ResourceManager::SETTINGS_SCAN_FINISHED_BUNDLES },
	{ SettingsManager::OVERLAP_SLOW_SOURCES, ResourceManager::SETTINGS_OVERLAP_SLOW_SOURCES },
	{ SettingsManager::AUTO_COMPLETE_BUNDLES, ResourceManager::SETTINGS_AUTO_COMPLETE_BUNDLES },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT DownloadPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items, 0, 0);
	PropPage::read((HWND)*this, items, optionItems, GetDlgItem(IDC_OTHER_QUEUE_OPTIONS));

	setMinMax(IDC_INTERVAL_SPIN, 5, 9999);
	setMinMax(IDC_MATCH_SPIN, 1, 999);
	setMinMax(IDC_AUTO_SEARCH_LIMIT_SPIN, 1, 999);

	setMinMax(IDC_SEG_NUMBER_SPIN, 1, 10);
	setMinMax(IDC_SEARCH_SPIN, 5, 60);
	setMinMax(IDC_BEGIN_SPIN, 2, 100000);

	checkItems();

	// Do specialized reading here
	return TRUE;
}

void DownloadPage::write() {

	if(SETTING(MIN_SEGMENT_SIZE) < 1024)
		settings->set(SettingsManager::MIN_SEGMENT_SIZE, 1024);

	PropPage::write((HWND)*this, items, 0, 0);
	PropPage::write((HWND)*this, items, optionItems, GetDlgItem(IDC_OTHER_QUEUE_OPTIONS));
}

LRESULT DownloadPage::onTick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	checkItems();
	return 0;
}

void DownloadPage::checkItems() {
	/* Segments */
	::EnableWindow(GetDlgItem(IDC_MIN_SEGMENT_SIZE_LABEL),	IsDlgButtonChecked(IDC_ENABLE_SEGMENTS));
	::EnableWindow(GetDlgItem(IDC_MIN_SEGMENT_SIZE),		IsDlgButtonChecked(IDC_ENABLE_SEGMENTS));
	::EnableWindow(GetDlgItem(IDC_SETTINGS_KIB),			IsDlgButtonChecked(IDC_ENABLE_SEGMENTS));

	::EnableWindow(GetDlgItem(IDC_SEG_NUMBER_EDIT),			IsDlgButtonChecked(IDC_CHUNKCOUNT) && IsDlgButtonChecked(IDC_ENABLE_SEGMENTS));
	::EnableWindow(GetDlgItem(IDC_CHUNKCOUNT),				IsDlgButtonChecked(IDC_ENABLE_SEGMENTS));

	::EnableWindow(GetDlgItem(IDC_DONTBEGIN_EDIT),			IsDlgButtonChecked(IDC_DONTBEGIN) && IsDlgButtonChecked(IDC_ENABLE_SEGMENTS));
	::EnableWindow(GetDlgItem(IDC_KBPS),					IsDlgButtonChecked(IDC_DONTBEGIN) && IsDlgButtonChecked(IDC_ENABLE_SEGMENTS));
	::EnableWindow(GetDlgItem(IDC_DONTBEGIN),				IsDlgButtonChecked(IDC_ENABLE_SEGMENTS));

	/* Searching */
	::EnableWindow(GetDlgItem(IDC_MATCH),					IsDlgButtonChecked(IDC_AUTO_ADD_SOURCES));
	::EnableWindow(GetDlgItem(IDC_MATCH_QUEUE_TEXT),		IsDlgButtonChecked(IDC_AUTO_ADD_SOURCES));

	::EnableWindow(GetDlgItem(IDC_ALLOW_MATCH_FULL),		IsDlgButtonChecked(IDC_AUTO_ADD_SOURCES));
	::EnableWindow(GetDlgItem(IDC_AUTO_SEARCH_ALT),			IsDlgButtonChecked(IDC_AUTO_ADD_SOURCES));

	::EnableWindow(GetDlgItem(IDC_INTERVAL_TEXT),			IsDlgButtonChecked(IDC_AUTO_SEARCH_ALT) && IsDlgButtonChecked(IDC_AUTO_ADD_SOURCES));
	::EnableWindow(GetDlgItem(IDC_SEARCH_INTERVAL),			IsDlgButtonChecked(IDC_AUTO_SEARCH_ALT) && IsDlgButtonChecked(IDC_AUTO_ADD_SOURCES));
	::EnableWindow(GetDlgItem(IDC_MINUTES),					IsDlgButtonChecked(IDC_AUTO_SEARCH_ALT) && IsDlgButtonChecked(IDC_AUTO_ADD_SOURCES));

	::EnableWindow(GetDlgItem(IDC_AUTO_SEARCH_LIMIT),			IsDlgButtonChecked(IDC_AUTO_SEARCH_ALT) && IsDlgButtonChecked(IDC_AUTO_ADD_SOURCES));
	::EnableWindow(GetDlgItem(IDC_SETTINGS_AUTO_SEARCH_LIMIT),	IsDlgButtonChecked(IDC_AUTO_SEARCH_ALT) && IsDlgButtonChecked(IDC_AUTO_ADD_SOURCES));
}

