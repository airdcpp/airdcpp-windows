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

#include "PriorityPage.h"
#include "CommandDlg.h"
#include "WinUtil.h"
#include "PropertiesDlg.h"

PropPage::TextItem PriorityPage::texts[] = {
	{ IDC_ORDER_BALANCED, ResourceManager::PRIOPAGE_ORDER_BALANCED },
	{ IDC_ORDER_PROGRESS, ResourceManager::PRIOPAGE_ORDER_PROGRESS },
	{ IDC_ORDER_ADDED, ResourceManager::PRIOPAGE_ORDER_ADDED },
	{ IDC_ORDER_RANDOM, ResourceManager::PRIOPAGE_ORDER_RANDOM },
	{ IDC_SETTINGS_AUTOPRIO, ResourceManager::SETTINGS_PRIO_AUTOPRIO },
	{ IDC_SETTINGS_PRIO_HIGHEST, ResourceManager::SETTINGS_PRIO_HIGHEST },
	{ IDC_SETTINGS_KB3, ResourceManager::KiB },
	{ IDC_SETTINGS_PRIO_HIGH, ResourceManager::SETTINGS_PRIO_HIGH },
	{ IDC_SETTINGS_KB4, ResourceManager::KiB },
	{ IDC_SETTINGS_PRIO_NORMAL, ResourceManager::SETTINGS_PRIO_NORMAL },
	{ IDC_SETTINGS_KB5, ResourceManager::KiB },
	{ IDC_SETTINGS_PRIO_LOW, ResourceManager::SETTINGS_PRIO_LOW },
	{ IDC_SETTINGS_KB6, ResourceManager::KiB },
	{ IDC_DOWNLOAD_ORDER, ResourceManager::DOWNLOAD_ORDER },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item PriorityPage::items[] = {
	{ IDC_PRIO_HIGHEST_SIZE, SettingsManager::PRIO_HIGHEST_SIZE, PropPage::T_INT },
	{ IDC_PRIO_HIGH_SIZE, SettingsManager::PRIO_HIGH_SIZE, PropPage::T_INT },
	{ IDC_PRIO_NORMAL_SIZE, SettingsManager::PRIO_NORMAL_SIZE, PropPage::T_INT },
	{ IDC_PRIO_LOW_SIZE, SettingsManager::PRIO_LOW_SIZE, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

LRESULT PriorityPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items, 0, 0);

	switch(SETTING(DOWNLOAD_ORDER)) {
		case SettingsManager::ORDER_BALANCED: 
			CheckDlgButton(IDC_ORDER_BALANCED, BST_CHECKED);
			break;
		case SettingsManager::ORDER_PROGRESS: 
			CheckDlgButton(IDC_ORDER_PROGRESS, BST_CHECKED);
			break;
		case SettingsManager::ORDER_ADDED: 
			CheckDlgButton(IDC_ORDER_ADDED, BST_CHECKED); 
			break;
		case SettingsManager::ORDER_RANDOM: 
			CheckDlgButton(IDC_ORDER_RANDOM, BST_CHECKED); 
			break;
		default: 
			CheckDlgButton(IDC_ORDER_BALANCED, BST_CHECKED);
			break;
	}


	/*spin.Attach(GetDlgItem(IDC_BEGIN_SPIN));
	spin.SetRange32(2, 100000);
	spin.Detach(); */

	// Do specialized reading here
	return TRUE;
}

void PriorityPage::write() {
	int id=0;
	if(IsDlgButtonChecked(IDC_ORDER_BALANCED)){
		id=0;
	} else if(IsDlgButtonChecked(IDC_ORDER_PROGRESS)) {
		id=1;
	} else if(IsDlgButtonChecked(IDC_ORDER_ADDED)){
		id=2;
	}else if(IsDlgButtonChecked(IDC_ORDER_RANDOM)){
		id=3;
	}
	SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_ORDER, id);

	PropPage::write((HWND)*this, items, 0, 0);
}


LRESULT PriorityPage::onDownloadOrder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(IsDlgButtonChecked(IDC_ORDER_BALANCED)){
		CheckDlgButton(IDC_ORDER_PROGRESS, BST_UNCHECKED);
		CheckDlgButton(IDC_ORDER_ADDED, BST_UNCHECKED);
		CheckDlgButton(IDC_ORDER_RANDOM, BST_UNCHECKED);
	} else if(IsDlgButtonChecked(IDC_ORDER_PROGRESS)){
		CheckDlgButton(IDC_ORDER_BALANCED, BST_UNCHECKED);
		CheckDlgButton(IDC_ORDER_ADDED, BST_UNCHECKED);
		CheckDlgButton(IDC_ORDER_RANDOM, BST_UNCHECKED);
	} else if(IsDlgButtonChecked(IDC_ORDER_ADDED)){
		CheckDlgButton(IDC_ORDER_PROGRESS, BST_UNCHECKED);
		CheckDlgButton(IDC_ORDER_BALANCED, BST_UNCHECKED);
		CheckDlgButton(IDC_ORDER_RANDOM, BST_UNCHECKED);
	} else {
		CheckDlgButton(IDC_ORDER_PROGRESS, BST_UNCHECKED);
		CheckDlgButton(IDC_ORDER_BALANCED, BST_UNCHECKED);
		CheckDlgButton(IDC_ORDER_ADDED, BST_UNCHECKED);
	}
	return TRUE;
}