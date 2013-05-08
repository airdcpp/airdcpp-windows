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

LRESULT WizardProfile::OnInitDialog(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) { 
	//PropPage::translate((HWND)(*this), texts);
	ShowWizardButtons( PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_FINISH | PSWIZB_CANCEL, PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_CANCEL); 
	EnableWizardButtons(PSWIZB_BACK, PSWIZB_BACK);
	EnableWizardButtons(PSWIZB_NEXT, PSWIZB_NEXT);

	switch(SETTING(SETTINGS_PROFILE)) {
		case SettingsManager::PROFILE_PUBLIC: CheckDlgButton(IDC_NORMAL, BST_CHECKED); break;
		case SettingsManager::PROFILE_RAR: CheckDlgButton(IDC_RAR, BST_CHECKED); break;
		case SettingsManager::PROFILE_LAN: CheckDlgButton(IDC_LAN, BST_CHECKED); break;
		default: CheckDlgButton(IDC_NORMAL, BST_CHECKED); break;
	}

	//CenterWindow(GetParent());
	fixcontrols();

	return TRUE; 
}

WizardProfile::WizardProfile(SettingsManager *s, SetupWizard* aWizard) : PropPage(s), wizard(aWizard) { 
	SetHeaderTitle(_T("Profile"));
}

int WizardProfile::OnWizardNext() {
	return 0;
}

void WizardProfile::write() {
	auto newProfile = getCurProfile();
	if (newProfile == SettingsManager::PROFILE_LAN && IsDlgButtonChecked(IDC_DISABLE_ENCRYPTION)) {
		SettingsManager::getInstance()->set(SettingsManager::TLS_MODE, static_cast<int>(SettingsManager::TLS_DISABLED));
	} else if (newProfile == SettingsManager::PROFILE_RAR && IsDlgButtonChecked(IDC_WIZARD_SKIPLIST)) {
		SettingsManager::getInstance()->set(SettingsManager::SHARE_SKIPLIST_USE_REGEXP, true);
		SettingsManager::getInstance()->set(SettingsManager::SKIPLIST_SHARE, "(.*(\\.(scn|asd|lnk|cmd|conf|dll|url|log|crc|dat|sfk|mxm|txt|message|iso|inf|sub|exe|img|bin|aac|mrg|tmp|xml|sup|ini|db|debug|pls|ac3|ape|par2|htm(l)?|bat|idx|srt|doc(x)?|ion|b4s|bgl|cab|cat|bat)$))|((All-Files-CRC-OK|xCOMPLETEx|imdb.nfo|- Copy|(.*\\s\\(\\d\\).*)).*$)");
		ShareManager::getInstance()->setSkipList();
	}

	SettingsManager::getInstance()->set(SettingsManager::SETTINGS_PROFILE, newProfile);
	SettingsManager::getInstance()->applyProfileDefaults();

	// a custom set value that differs from the one used by the profile? don't replace those without confirmation
	vector<SettingItem> conflicts;
	for (const auto& newSetting: SettingsManager::profileSettings[newProfile]) {
		if (newSetting.isSet() && !newSetting.isProfileCurrent()) {
			conflicts.push_back(newSetting);
		}
	}

	if (!conflicts.empty()) {
		string msg;
		for (const auto& setting: conflicts) {
			msg += "Setting name: " + setting.getName() + "\r\n";
			msg += "Current value: " + setting.currentToString() + "\r\n";
			msg += "Profile value: " + setting.profileToString() + "\r\n\r\n";
		}

		CTaskDialog taskdlg;

		tstring descText = _T("There are x settings that have manually set values. Do you want to replace those with the values used by this profile?");
		taskdlg.SetContentText(descText.c_str());

		auto tmp = Text::toT(msg);
		taskdlg.SetExpandedInformationText(tmp.c_str());
		taskdlg.SetExpandedControlText(_T("Show conflicting settings"));
		TASKDIALOG_BUTTON buttons[] =
		{
			{ 0, _T("Use the profile settings"), },
			{ 1, _T("Keep my current settings"), },
		};
		taskdlg.ModifyFlags(0, TDF_USE_COMMAND_LINKS | TDF_EXPAND_FOOTER_AREA);
		taskdlg.SetWindowTitle(_T("Manually configured settings found"));
		taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
		taskdlg.SetMainIcon(TD_INFORMATION_ICON);

		taskdlg.SetButtons(buttons, _countof(buttons));

		int sel = 0;
        taskdlg.DoModal(m_hWnd, &sel, 0, 0);
		if (sel == 0) {
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
	} else if(IsDlgButtonChecked(IDC_LAN)){
		return SettingsManager::PROFILE_LAN;;
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