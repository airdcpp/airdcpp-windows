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

#include "../client/AirUtil.h"
#include "../client/Localization.h"
#include "Resource.h"

#include "GeneralPage.h"
#include "WinUtil.h"

PropPage::TextItem GeneralPage::texts[] = {
	{ IDC_SETTINGS_PERSONAL_INFORMATION, ResourceManager::SETTINGS_PERSONAL_INFORMATION },
	{ IDC_SETTINGS_NICK, ResourceManager::NICK },
	{ IDC_SETTINGS_EMAIL, ResourceManager::EMAIL },
	{ IDC_SETTINGS_DESCRIPTION, ResourceManager::DESCRIPTION },
	{ IDC_SETTINGS_PROFILE, ResourceManager::SETTINGS_PROFILE },
	{ IDC_LANGUAGE_CAPTION, ResourceManager::SETTINGS_LANGUAGE },
	{ IDC_LANGUAGE_NOTE, ResourceManager::LANGUAGE_NOTE },
	{ IDC_AUTO_AWAY, ResourceManager::SETTINGS_AUTO_AWAY },
	{ IDC_SETTINGS_DEFAULT_AWAY_MSG, ResourceManager::SETTINGS_DEFAULT_AWAY_MSG },
	{ IDC_AWAY_MODE, ResourceManager::AWAY_MODE },
	{ IDC_AWAY_IDLE_TEXT_BEGIN, ResourceManager::AWAY_IDLE_TIME_BEGIN },
	{ IDC_AWAY_IDLE_TEXT_END, ResourceManager::AWAY_IDLE_TIME_END },
	{ IDC_NORMAL, ResourceManager::NORMAL },
	{ IDC_RAR, ResourceManager::RAR_HUBS },
	{ IDC_LAN, ResourceManager::LAN_HUBS },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item GeneralPage::items[] = {
	{ IDC_NICK,			SettingsManager::NICK,			PropPage::T_STR }, 
	{ IDC_EMAIL,		SettingsManager::EMAIL,			PropPage::T_STR }, 
	{ IDC_USERDESC,		SettingsManager::DESCRIPTION,	PropPage::T_STR },
	{ IDC_AUTO_AWAY,	SettingsManager::AUTO_AWAY,	    PropPage::T_BOOL },
 	{ IDC_DEFAULT_AWAY_MESSAGE, SettingsManager::DEFAULT_AWAY_MESSAGE, PropPage::T_STR },
	{ IDC_AWAY_IDLE_TIME, SettingsManager::AWAY_IDLE_TIME, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

void GeneralPage::write()
{
	Localization::setLanguage(ctrlLanguage.GetCurSel());
	PropPage::write((HWND)(*this), items);
}

void GeneralPage::updateCurProfile() {
	if(IsDlgButtonChecked(IDC_NORMAL)){
		curProfile = SettingsManager::PROFILE_NORMAL;
	} else if(IsDlgButtonChecked(IDC_RAR)) {
		curProfile = SettingsManager::PROFILE_RAR;
	} else if(IsDlgButtonChecked(IDC_LAN)){
		curProfile = SettingsManager::PROFILE_LAN;;
	}
}

// make sure that this is set after the other settings
Dispatcher::F GeneralPage::getThreadedTask() {
	if (curProfile > 0 && (curProfile != SETTING(SETTINGS_PROFILE) || !conflicts.empty())) {
		return Dispatcher::F([=] { 
			SettingsManager::getInstance()->setProfile(curProfile, conflicts);
		});
	}

	return nullptr;
}

LRESULT GeneralPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)(*this), items);

	switch(SETTING(SETTINGS_PROFILE)) {
		case SettingsManager::PROFILE_NORMAL: CheckDlgButton(IDC_NORMAL, BST_CHECKED); break;
		case SettingsManager::PROFILE_RAR: CheckDlgButton(IDC_RAR, BST_CHECKED); break;
		case SettingsManager::PROFILE_LAN: CheckDlgButton(IDC_LAN, BST_CHECKED); break;
		default: CheckDlgButton(IDC_NORMAL, BST_CHECKED); break;
	}

	setMinMax(IDC_AWAY_SPIN, 0, 60);

	ctrlLanguage.Attach(GetDlgItem(IDC_LANGUAGE));

	::SetWindowText(GetDlgItem(IDC_CURRENT_PROFILE_NAME), Text::toT(SettingsManager::getInstance()->getProfileName(SETTING(SETTINGS_PROFILE))).c_str());

	WinUtil::appendLanguageMenu(ctrlLanguage);

	WinUtil::setUserFieldLimits(m_hWnd);
	updateCurProfile();
	return TRUE;
}

LRESULT GeneralPage::onSelProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto lastProfile = curProfile;
	updateCurProfile();
	if (curProfile != lastProfile) {
		WinUtil::getProfileConflicts(m_hWnd, curProfile, conflicts);
	}

	return TRUE;
}