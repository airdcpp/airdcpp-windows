/*
 * Copyright (C) 2012-2013 AirDC++ Project
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

#include "WizardGeneral.h"
#include "WinUtil.h"

#include "../client/AirUtil.h"
#include "../client/Localization.h"

PropPage::TextItem WizardGeneral::texts[] = {
	{ IDC_SETTINGS_SHARED_DIRECTORIES, ResourceManager::SETTINGS_SHARED_DIRECTORIES },
	{ IDC_ADD_PROFILE, ResourceManager::ADD_PROFILE },
	{ IDC_ADD_PROFILE_COPY, ResourceManager::ADD_PROFILE_COPY },
	{ IDC_REMOVE_PROFILE, ResourceManager::REMOVE_PROFILE },
	{ IDC_SETTINGS_SHARE_PROFILES, ResourceManager::SHARE_PROFILES },
	{ IDC_SHARE_PROFILE_NOTE, ResourceManager::SETTINGS_SHARE_PROFILE_NOTE },
	{ IDC_RENAME_PROFILE, ResourceManager::SETTINGS_RENAME_FOLDER },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item WizardGeneral::items[] = {
	//limiter
	{ IDC_MX_UP_SP_LMT_NORMAL, SettingsManager::MAX_UPLOAD_SPEED_MAIN, PropPage::T_INT },
	{ IDC_MX_DW_SP_LMT_NORMAL, SettingsManager::MAX_DOWNLOAD_SPEED_MAIN, PropPage::T_INT },
	{ IDC_TIME_LIMITING, SettingsManager::TIME_DEPENDENT_THROTTLE, PropPage::T_BOOL },
	{ IDC_MX_UP_SP_LMT_TIME, SettingsManager::MAX_UPLOAD_SPEED_ALTERNATE, PropPage::T_INT },
	{ IDC_MX_DW_SP_LMT_TIME, SettingsManager::MAX_DOWNLOAD_SPEED_ALTERNATE, PropPage::T_INT },
	{ IDC_BW_START_TIME, SettingsManager::BANDWIDTH_LIMIT_START, PropPage::T_INT },
	{ IDC_BW_END_TIME, SettingsManager::BANDWIDTH_LIMIT_END, PropPage::T_INT },
	{ IDC_AUTO_DETECTION_USE_LIMITED, SettingsManager::AUTO_DETECTION_USE_LIMITED, PropPage::T_BOOL },

	//uploads
	{ IDC_EXTRA_SLOTS, SettingsManager::EXTRA_SLOTS, PropPage::T_INT },
	{ IDC_SMALL_FILE_SIZE, SettingsManager::SET_MINISLOT_SIZE, PropPage::T_INT },
	{ IDC_EXTRA_SLOTS2, SettingsManager::HUB_SLOTS, PropPage::T_INT },
	{ IDC_PARTIAL_SLOTS, SettingsManager::EXTRA_PARTIAL_SLOTS, PropPage::T_INT  },

	//minislots
	{ IDC_MINISLOTS_EXTENSIONS, SettingsManager::FREE_SLOTS_EXTENSIONS, PropPage::T_STR },
	{ 0, 0, PropPage::T_END }
};


LRESULT WizardGeneral::OnInitDialog(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) { 
	ShowWizardButtons( PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_FINISH | PSWIZB_CANCEL, PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_CANCEL); 
	//EnableWizardButtons(PSWIZB_BACK, 0); 
	//SetButtonText(PSWIZB_NEXT, L"&Next"); 

	//Set current nick setting
	nickline.Attach(GetDlgItem(IDC_NICK));
	SetDlgItemText(IDC_NICK, Text::toT(SETTING(NICK)).c_str());

	ctrlLanguage.Attach(GetDlgItem(IDC_LANGUAGE));


	WinUtil::appendLanguageMenu(ctrlLanguage);

	switch(SETTING(SETTINGS_PROFILE)) {
		case SettingsManager::PROFILE_PUBLIC: CheckDlgButton(IDC_PUBLIC, BST_CHECKED); break;
		case SettingsManager::PROFILE_RAR: CheckDlgButton(IDC_RAR, BST_CHECKED); break;
		case SettingsManager::PROFILE_PRIVATE: CheckDlgButton(IDC_PRIVATE_HUB, BST_CHECKED); break;
		default: CheckDlgButton(IDC_PUBLIC, BST_CHECKED); break;
	}

	CenterWindow(GetParent());
	fixcontrols();

	return TRUE; 
}

WizardGeneral::WizardGeneral(SettingsManager *s) : PropPage(s) { 
	SetHeaderTitle(_T("General")); 
} 

void WizardGeneral::write() {
	//set the language
	Localization::setLanguage(ctrlLanguage.GetCurSel());

	//for Nick
	TCHAR buf[64];
	GetDlgItemText(IDC_NICK, buf, sizeof(buf) + 1);
	string nick = Text::fromT(buf);
	if(nick != Util::emptyString)
		SettingsManager::getInstance()->set(SettingsManager::NICK, nick );


	if(IsDlgButtonChecked(IDC_PUBLIC)){
		AirUtil::setProfile(0);
	} else if(IsDlgButtonChecked(IDC_RAR)) {
		AirUtil::setProfile(1, IsDlgButtonChecked(IDC_WIZARD_SKIPLIST) ? true : false);
	} else if(IsDlgButtonChecked(IDC_PRIVATE_HUB)){
		AirUtil::setProfile(2);
	}
}

LRESULT WizardGeneral::OnSelProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixcontrols();
	return 0;
}

void WizardGeneral::fixcontrols() {
	if(IsDlgButtonChecked(IDC_PUBLIC)){
		CheckDlgButton(IDC_RAR, BST_UNCHECKED);
		CheckDlgButton(IDC_PRIVATE_HUB, BST_UNCHECKED);
		::EnableWindow(GetDlgItem(IDC_WIZARD_SKIPLIST),	0);
	}

	if(IsDlgButtonChecked(IDC_RAR)){
		CheckDlgButton(IDC_PRIVATE_HUB, BST_UNCHECKED);
		CheckDlgButton(IDC_PUBLIC, BST_UNCHECKED);
		::EnableWindow(GetDlgItem(IDC_WIZARD_SKIPLIST),	1);
	}

	if(IsDlgButtonChecked(IDC_PRIVATE_HUB)){
		CheckDlgButton(IDC_RAR, BST_UNCHECKED);
		CheckDlgButton(IDC_PUBLIC, BST_UNCHECKED);
		::EnableWindow(GetDlgItem(IDC_WIZARD_SKIPLIST),	0);
	}
}