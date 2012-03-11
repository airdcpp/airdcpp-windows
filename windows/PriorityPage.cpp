/*
 * Copyright (C) 2011 AirDC++ Project
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
	{ IDC_AUTOPRIO_TYPE, ResourceManager::SETTINGS_AUTOPRIO },
	{ IDC_AUTOPRIO_FILES, ResourceManager::SETTINGS_AUTOPRIO_FILES },
	{ IDC_CALC_PRIO_EVERY, ResourceManager::SETTINGS_CALCULATE_EVERY },
	
	{ IDC_SB_HIGH_PRIO_FILES, ResourceManager::SETTINGS_HIGH_PRIO_FILES	},
	{ IDC_HIGHEST_PRIORITY_USE_REGEXP, ResourceManager::USE_REGEXP },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item PriorityPage::items[] = {
	{ IDC_PRIO_HIGHEST_SIZE, SettingsManager::PRIO_HIGHEST_SIZE, PropPage::T_INT },
	{ IDC_PRIO_HIGH_SIZE, SettingsManager::PRIO_HIGH_SIZE, PropPage::T_INT },
	{ IDC_PRIO_NORMAL_SIZE, SettingsManager::PRIO_NORMAL_SIZE, PropPage::T_INT },
	{ IDC_PRIO_LOW_SIZE, SettingsManager::PRIO_LOW_SIZE, PropPage::T_INT },
	
	{ IDC_HIGH_PRIO_FILES,	SettingsManager::HIGH_PRIO_FILES, PropPage::T_STR },
	{ IDC_HIGHEST_PRIORITY_USE_REGEXP, SettingsManager::HIGHEST_PRIORITY_USE_REGEXP, PropPage::T_BOOL },

	{ IDC_AUTOPRIO_FILES, SettingsManager::QI_AUTOPRIO, PropPage::T_BOOL },
	{ IDC_AUTOPRIO_INTERVAL, SettingsManager::AUTOPRIO_INTERVAL, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

PropPage::ListItem PriorityPage::optionItems[] = {
	{ SettingsManager::PRIO_LOWEST, ResourceManager::SETTINGS_PRIO_LOWEST },
	{ SettingsManager::PRIO_LIST_HIGHEST, ResourceManager::SETTINGS_USE_HIGHEST_LIST },
	{ SettingsManager::AUTO_PRIORITY_DEFAULT ,ResourceManager::SETTINGS_AUTO_PRIORITY_DEFAULT },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT PriorityPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items, 0, 0);
	PropPage::read((HWND)*this, items, optionItems, GetDlgItem(IDC_OTHER_PRIORITY_OPTIONS));

	switch(SETTING(DOWNLOAD_ORDER)) {
		case SettingsManager::ORDER_ADDED: 
			CheckDlgButton(IDC_ORDER_ADDED, BST_CHECKED); 
			break;
		case SettingsManager::ORDER_RANDOM: 
			CheckDlgButton(IDC_ORDER_RANDOM, BST_CHECKED); 
			break;
		default: 
			CheckDlgButton(IDC_ORDER_ADDED, BST_CHECKED);
			break;
	}

	switch(SETTING(AUTOPRIO_TYPE)) {
		case SettingsManager::PRIO_BALANCED: 
			CheckDlgButton(IDC_PRIO_BALANCED, BST_CHECKED);
			CheckDlgButton(IDC_PRIO_PROGRESS, BST_UNCHECKED);
			CheckDlgButton(IDC_AUTOPRIO_ENABLED, BST_CHECKED);
			break;
		case SettingsManager::PRIO_PROGRESS: 
			CheckDlgButton(IDC_PRIO_PROGRESS, BST_CHECKED);
			CheckDlgButton(IDC_PRIO_BALANCED, BST_UNCHECKED);
			CheckDlgButton(IDC_AUTOPRIO_ENABLED, BST_CHECKED);
			break;
		case SettingsManager::PRIO_DISABLED: 
			CheckDlgButton(IDC_AUTOPRIO_ENABLED, BST_UNCHECKED);
			CheckDlgButton(IDC_PRIO_BALANCED, BST_CHECKED);
			CheckDlgButton(IDC_PRIO_PROGRESS, BST_UNCHECKED);
			break;
		default: 
			CheckDlgButton(IDC_PRIO_PROGRESS, BST_CHECKED);
			break;
	}

	CUpDownCtrl spin;
	spin.Attach(GetDlgItem(IDC_PRIO_SPIN));
	spin.SetRange32(1, 1000);
	spin.Detach();

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

	if (IsDlgButtonChecked(IDC_AUTOPRIO_ENABLED)) {
		if(IsDlgButtonChecked(IDC_ORDER_BALANCED)){
			id=1;
		} else if(IsDlgButtonChecked(IDC_ORDER_PROGRESS)) {
			id=2;
		}
	} else {
		id=0;
	}
	SettingsManager::getInstance()->set(SettingsManager::AUTOPRIO_TYPE, id);

	PropPage::write((HWND)*this, items, 0, 0);
	PropPage::write((HWND)*this, items, optionItems, GetDlgItem(IDC_OTHER_PRIORITY_OPTIONS));
}


LRESULT PriorityPage::onOrderChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(IsDlgButtonChecked(IDC_ORDER_ADDED)){
		CheckDlgButton(IDC_ORDER_RANDOM, BST_UNCHECKED);
	} else if (IsDlgButtonChecked(IDC_ORDER_RANDOM)) {
		CheckDlgButton(IDC_ORDER_ADDED, BST_UNCHECKED);
	}
	return TRUE;
}


LRESULT PriorityPage::onPrioTypeChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	::EnableWindow(GetDlgItem(IDC_PRIO_BALANCED),			IsDlgButtonChecked(IDC_AUTOPRIO_ENABLED));
	::EnableWindow(GetDlgItem(IDC_PRIO_PROGRESS),			IsDlgButtonChecked(IDC_AUTOPRIO_ENABLED));
	::EnableWindow(GetDlgItem(IDC_CALC_PRIO_EVERY),			IsDlgButtonChecked(IDC_AUTOPRIO_ENABLED));
	::EnableWindow(GetDlgItem(IDC_AUTOPRIO_INTERVAL),		IsDlgButtonChecked(IDC_AUTOPRIO_ENABLED));
	::EnableWindow(GetDlgItem(IDC_SETTINGS_SECONDS),		IsDlgButtonChecked(IDC_AUTOPRIO_ENABLED));
	::EnableWindow(GetDlgItem(IDC_AUTOPRIO_FILES),			IsDlgButtonChecked(IDC_AUTOPRIO_ENABLED));
	::EnableWindow(GetDlgItem(IDC_PRIO_SPIN),				IsDlgButtonChecked(IDC_AUTOPRIO_ENABLED));

	if(IsDlgButtonChecked(IDC_PRIO_BALANCED)){
		CheckDlgButton(IDC_PRIO_PROGRESS, BST_UNCHECKED);
	} else if(IsDlgButtonChecked(IDC_PRIO_PROGRESS)){
		CheckDlgButton(IDC_PRIO_BALANCED, BST_UNCHECKED);
	}
	return TRUE;
}