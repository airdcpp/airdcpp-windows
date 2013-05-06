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

#include "WizardProfile.h"

#include "../client/AirUtil.h"

PropPage::TextItem WizardProfile::texts[] = {
	//{ IDC_PROFILE_INTRO, ResourceManager::DESCRIPTION },
	//{ IDC_NORMAL_DESC, ResourceManager::DESCRIPTION },
	//{ IDC_RAR_DESC, ResourceManager::DESCRIPTION },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT WizardProfile::OnInitDialog(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) { 
	ShowWizardButtons( PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_FINISH | PSWIZB_CANCEL, PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_CANCEL); 

	//PropPage::translate((HWND)(*this), texts);

	switch(SETTING(SETTINGS_PROFILE)) {
		case SettingsManager::PROFILE_PUBLIC: CheckDlgButton(IDC_NORMAL, BST_CHECKED); break;
		case SettingsManager::PROFILE_RAR: CheckDlgButton(IDC_RAR, BST_CHECKED); break;
		case SettingsManager::PROFILE_PRIVATE: CheckDlgButton(IDC_PRIVATE_HUB, BST_CHECKED); break;
		default: CheckDlgButton(IDC_NORMAL, BST_CHECKED); break;
	}

	//CenterWindow(GetParent());
	fixcontrols();

	return TRUE; 
}

WizardProfile::WizardProfile(SettingsManager *s, SetupWizard* aWizard) : PropPage(s), wizard(aWizard) { 
	SetHeaderTitle(_T("Profile"));
} 

void WizardProfile::write() {
	if(IsDlgButtonChecked(IDC_NORMAL)){
		AirUtil::setProfile(0);
	} else if(IsDlgButtonChecked(IDC_RAR)) {
		AirUtil::setProfile(1, IsDlgButtonChecked(IDC_WIZARD_SKIPLIST) ? true : false);
	} else if(IsDlgButtonChecked(IDC_PRIVATE_HUB)){
		AirUtil::setProfile(2);
	}
}

int WizardProfile::getCurProfile() {
	if(IsDlgButtonChecked(IDC_NORMAL)){
		return SettingsManager::PROFILE_PUBLIC;
	} else if(IsDlgButtonChecked(IDC_RAR)) {
		return SettingsManager::PROFILE_RAR;
	} else if(IsDlgButtonChecked(IDC_PRIVATE_HUB)){
		return SettingsManager::PROFILE_PRIVATE;;
	}

	return SettingsManager::PROFILE_PUBLIC;
}

LRESULT WizardProfile::OnSelProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixcontrols();
	return 0;
}

void WizardProfile::fixcontrols() {
	::EnableWindow(GetDlgItem(IDC_WIZARD_SKIPLIST),	IsDlgButtonChecked(IDC_RAR));
	::EnableWindow(GetDlgItem(IDC_DISABLE_ENCRYPTION),	IsDlgButtonChecked(IDC_LAN));
}