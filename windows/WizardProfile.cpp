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
#include "../client/SettingItem.h"

PropPage::TextItem WizardProfile::texts[] = {
	//{ IDC_PROFILE_INTRO, ResourceManager::DESCRIPTION },
	//{ IDC_NORMAL_DESC, ResourceManager::DESCRIPTION },
	//{ IDC_RAR_DESC, ResourceManager::DESCRIPTION },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

SettingItem normalSettings[] = { 
	{ SettingsManager::MULTI_CHUNK, true, ResourceManager::SEGMENTS },
	{ SettingsManager::CHECK_SFV, false, ResourceManager::CHECK_SFV },
	{ SettingsManager::CHECK_NFO, false, ResourceManager::CHECK_NFO },
	{ SettingsManager::CHECK_EXTRA_SFV_NFO, false, ResourceManager::CHECK_EXTRA_SFV_NFO },
	{ SettingsManager::CHECK_EXTRA_FILES, false, ResourceManager::CHECK_EXTRA_FILES },
	{ SettingsManager::CHECK_DUPES, false, ResourceManager::CHECK_DUPES },
	{ SettingsManager::MAX_FILE_SIZE_SHARED, 0, ResourceManager::DONT_SHARE_BIGGER_THAN },
	{ SettingsManager::SEARCH_TIME, 10, ResourceManager::MINIMUM_SEARCH_INTERVAL },
	//{ SettingsManager::AUTO_SEARCH_LIMIT, 5 },
	{ SettingsManager::AUTO_FOLLOW, true, ResourceManager::SETTINGS_AUTO_FOLLOW },
};

SettingItem rarSettings[] = {
	{ SettingsManager::MULTI_CHUNK, false, ResourceManager::SEGMENTS },
	{ SettingsManager::CHECK_SFV, true, ResourceManager::CHECK_SFV },
	{ SettingsManager::CHECK_NFO, true, ResourceManager::CHECK_NFO },
	{ SettingsManager::CHECK_EXTRA_SFV_NFO, true, ResourceManager::CHECK_EXTRA_SFV_NFO },
	{ SettingsManager::CHECK_EXTRA_FILES, true, ResourceManager::CHECK_EXTRA_FILES },
	{ SettingsManager::CHECK_DUPES, true, ResourceManager::CHECK_DUPES },
	{ SettingsManager::MAX_FILE_SIZE_SHARED, 600, ResourceManager::DONT_SHARE_BIGGER_THAN },
	{ SettingsManager::SEARCH_TIME, 5, ResourceManager::MINIMUM_SEARCH_INTERVAL },
	//{ SettingsManager::AUTO_SEARCH_LIMIT, 5 },
	{ SettingsManager::AUTO_FOLLOW, false, ResourceManager::SETTINGS_AUTO_FOLLOW },
};

SettingItem lanSettings[] = {
	{ SettingsManager::MULTI_CHUNK, true, ResourceManager::SEGMENTS },
	{ SettingsManager::CHECK_SFV, false, ResourceManager::CHECK_SFV },
	{ SettingsManager::CHECK_NFO, false, ResourceManager::CHECK_NFO },
	{ SettingsManager::CHECK_EXTRA_SFV_NFO, false, ResourceManager::CHECK_EXTRA_SFV_NFO },
	{ SettingsManager::CHECK_EXTRA_FILES, false, ResourceManager::CHECK_EXTRA_FILES },
	{ SettingsManager::CHECK_DUPES, false, ResourceManager::CHECK_DUPES },
	{ SettingsManager::MAX_FILE_SIZE_SHARED, 0, ResourceManager::DONT_SHARE_BIGGER_THAN },
	{ SettingsManager::SEARCH_TIME, 5, ResourceManager::MINIMUM_SEARCH_INTERVAL },
	//{ SettingsManager::AUTO_SEARCH_LIMIT, 5 },
	{ SettingsManager::AUTO_FOLLOW, true, ResourceManager::SETTINGS_AUTO_FOLLOW },
};

LRESULT WizardProfile::OnInitDialog(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) { 
	//PropPage::translate((HWND)(*this), texts);
	ShowWizardButtons( PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_FINISH | PSWIZB_CANCEL, PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_CANCEL); 
	EnableWizardButtons(PSWIZB_BACK, PSWIZB_BACK);
	EnableWizardButtons(PSWIZB_NEXT, PSWIZB_NEXT);

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

const SettingItem (&getArray(int newProfile))[9] {
	if (newProfile == SettingsManager::PROFILE_RAR) {
		return rarSettings;
	} else if (newProfile == SettingsManager::PROFILE_LAN) {
		return lanSettings;
	} else {
		return normalSettings;
	}
}

int WizardProfile::OnWizardNext() {
	return 0;
}

void WizardProfile::write() {
	if(IsDlgButtonChecked(IDC_NORMAL)) {
		AirUtil::setProfile(0);
	} else if(IsDlgButtonChecked(IDC_RAR)) {
		AirUtil::setProfile(1, IsDlgButtonChecked(IDC_WIZARD_SKIPLIST) ? true : false);
	} else if(IsDlgButtonChecked(IDC_PRIVATE_HUB)) {
		AirUtil::setProfile(2);
	}

	auto newProfile = getCurProfile();
	vector<SettingItem> conflicts;
	
	for (const auto& newSetting: getArray(newProfile)) {
		// a custom set value that differs from the one used by the profile? don't replace those without confirmation
		if (newSetting.isSet() && !newSetting.isProfileCurrent()) {
			conflicts.push_back(newSetting);
		} else {
			//we can set the default right away
			newSetting.setDefault(false);
		}
	}

	if (!conflicts.empty()) {
		string msg = "The following settings currently have manually set values:\r\n\r\n";
		for (const auto& setting: conflicts) {
			msg += "Setting name: " + setting.getName() + "\r\n";
			msg += "Current value: " + setting.currentToString() + "\r\n";
			msg += "Profile value: " + setting.profileToString() + "\r\n\r\n";
		}

		msg += "Do you want to use the profile values anyway?";
		if (MessageBox(Text::toT(msg).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING),  MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
			for (const auto& setting: conflicts) {
				setting.setDefault(true);
			}
		}
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

int WizardProfile::OnSetActive() {
	ShowWizardButtons( PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_FINISH | PSWIZB_CANCEL, PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_CANCEL); 
	EnableWizardButtons(PSWIZB_BACK, PSWIZB_BACK);
	EnableWizardButtons(PSWIZB_NEXT, PSWIZB_NEXT);
	return 0;
}