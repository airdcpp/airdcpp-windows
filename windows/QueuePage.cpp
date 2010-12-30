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

#include "QueuePage.h"
#include "CommandDlg.h"
#include "WinUtil.h"

PropPage::TextItem QueuePage::texts[] = {
	{ IDC_SETTINGS_AUTOPRIO, ResourceManager::SETTINGS_PRIO_AUTOPRIO },
	{ IDC_SETTINGS_PRIO_HIGHEST, ResourceManager::SETTINGS_PRIO_HIGHEST },
	{ IDC_SETTINGS_KB3, ResourceManager::KiB },
	{ IDC_SETTINGS_PRIO_HIGH, ResourceManager::SETTINGS_PRIO_HIGH },
	{ IDC_SETTINGS_KB4, ResourceManager::KiB },
	{ IDC_SETTINGS_PRIO_NORMAL, ResourceManager::SETTINGS_PRIO_NORMAL },
	{ IDC_SETTINGS_KB5, ResourceManager::KiB },
	{ IDC_SETTINGS_PRIO_LOW, ResourceManager::SETTINGS_PRIO_LOW },
	{ IDC_SETTINGS_KB6, ResourceManager::KiB },
	{ IDC_SETTINGS_SEGMENT, ResourceManager::SETTINGS_SEGMENT },
	{ IDC_AUTOSEGMENT, ResourceManager::SETTINGS_AUTO_SEARCH },
	{ IDC_DONTBEGIN, ResourceManager::DONT_ADD_SEGMENT_TEXT },
	{ IDC_MULTISOURCE, ResourceManager::ENABLE_MULTI_SOURCE },
	{ IDC_MINUTES, ResourceManager::MINUTES },
	{ IDC_KBPS, ResourceManager::KBPS },
	{ IDC_CHUNKCOUNT, ResourceManager::TEXT_MANUAL },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item QueuePage::items[] = {
	{ IDC_PRIO_HIGHEST_SIZE, SettingsManager::PRIO_HIGHEST_SIZE, PropPage::T_INT },
	{ IDC_PRIO_HIGH_SIZE, SettingsManager::PRIO_HIGH_SIZE, PropPage::T_INT },
	{ IDC_PRIO_NORMAL_SIZE, SettingsManager::PRIO_NORMAL_SIZE, PropPage::T_INT },
	{ IDC_PRIO_LOW_SIZE, SettingsManager::PRIO_LOW_SIZE, PropPage::T_INT },
	{ IDC_AUTOSEGMENT, SettingsManager::AUTO_SEARCH, PropPage::T_BOOL },
	{ IDC_DONTBEGIN, SettingsManager::DONT_BEGIN_SEGMENT, PropPage::T_BOOL },
	{ IDC_BEGIN_EDIT, SettingsManager::DONT_BEGIN_SEGMENT_SPEED, PropPage::T_INT },
	{ IDC_SEARCH_EDIT, SettingsManager::SEARCH_TIME, PropPage::T_INT },
	{ IDC_CHUNKCOUNT, SettingsManager::SEGMENTS_MANUAL, PropPage::T_BOOL },
	{ IDC_SEG_NUMBER, SettingsManager::NUMBER_OF_SEGMENTS, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

PropPage::ListItem QueuePage::optionItems[] = {
	{ SettingsManager::MULTI_CHUNK, ResourceManager::ENABLE_MULTI_SOURCE },
	{ SettingsManager::PRIO_LOWEST, ResourceManager::SETTINGS_PRIO_LOWEST },
	{ SettingsManager::AUTO_SEARCH_AUTO_MATCH, ResourceManager::SETTINGS_AUTO_SEARCH_AUTO_MATCH },
	{ SettingsManager::SKIP_ZERO_BYTE, ResourceManager::SETTINGS_SKIP_ZERO_BYTE },
	{ SettingsManager::DONT_DL_ALREADY_SHARED, ResourceManager::SETTINGS_DONT_DL_ALREADY_SHARED },
	{ SettingsManager::ANTI_FRAG, ResourceManager::SETTINGS_ANTI_FRAG },
	{ SettingsManager::AUTO_PRIORITY_DEFAULT ,ResourceManager::SETTINGS_AUTO_PRIORITY_DEFAULT },
	{ SettingsManager::OVERLAP_CHUNKS ,ResourceManager::SETTINGS_OVERLAP_CHUNKS },
	{ SettingsManager::AUTO_ADD_SOURCE ,ResourceManager::AUTO_ADD_SOURCE },
	{ SettingsManager::KEEP_FINISHED_FILES, ResourceManager::KEEP_FINISHED_FILES },
	{ SettingsManager::DONT_DL_ALREADY_QUEUED, ResourceManager::SETTING_DONT_DL_ALREADY_QUEUED },
	{ SettingsManager::DOWNLOADS_EXPAND, ResourceManager::SETTINGS_DOWNLOADS_EXPAND },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT QueuePage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items, 0, 0);
	PropPage::read((HWND)*this, items, optionItems, GetDlgItem(IDC_OTHER_QUEUE_OPTIONS));

	CUpDownCtrl spin;
	spin.Attach(GetDlgItem(IDC_SEG_NUMBER_SPIN));
	spin.SetRange32(2, 10);
	spin.Detach();

	spin.Attach(GetDlgItem(IDC_SEARCH_SPIN));
	spin.SetRange32(5, 60);
	spin.Detach();

	spin.Attach(GetDlgItem(IDC_BEGIN_SPIN));
	spin.SetRange32(2, 100000);
	spin.Detach();

	// Do specialized reading here
	return TRUE;
}

void QueuePage::write() {
	PropPage::write((HWND)*this, items, 0, 0);
	PropPage::write((HWND)*this, items, optionItems, GetDlgItem(IDC_OTHER_QUEUE_OPTIONS));
}


/**
 * @file
 * $Id: QueuePage.cpp 411 2008-07-20 22:39:42Z BigMuscle $
 */
