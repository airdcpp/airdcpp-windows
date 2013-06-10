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

#include "../client/AirUtil.h"
#include "../client/Localization.h"

PropPage::TextItem WizardGeneral::texts[] = {
	{ IDC_SETTINGS_NICK, ResourceManager::NICK },
	{ IDC_SETTINGS_EMAIL, ResourceManager::EMAIL },
	{ IDC_SETTINGS_DESCRIPTION, ResourceManager::DESCRIPTION },
	{ IDC_GENERAL_INTRO, ResourceManager::WIZARD_GENERAL_INTRO },
	{ IDC_SETTINGS_PERSONAL_INFORMATION, ResourceManager::SETTINGS_PERSONAL_INFORMATION },
	{ IDC_REQUIRED,		ResourceManager::REQUIRED_BRACKETS },
	{ IDC_OPTIONAL,		ResourceManager::OPTIONAL_BRACKETS },
	{ IDC_OPTIONAL2,	ResourceManager::OPTIONAL_BRACKETS },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item WizardGeneral::items[] = {
	{ IDC_NICK,			SettingsManager::NICK,				PropPage::T_STR }, 
	{ IDC_EMAIL,		SettingsManager::EMAIL,				PropPage::T_STR }, 
	{ IDC_USERDESC,		SettingsManager::DESCRIPTION,		PropPage::T_STR },
	{ 0, 0, PropPage::T_END }
};


LRESULT WizardGeneral::OnInitDialog(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) { 
	PropPage::read((HWND)*this, items);
	PropPage::translate((HWND)(*this), texts);

	/*ctrlLanguage.Attach(GetDlgItem(IDC_LANGUAGE));
	WinUtil::appendLanguageMenu(ctrlLanguage);*/

	WinUtil::setUserFieldLimits(m_hWnd);

	return TRUE; 
}

WizardGeneral::WizardGeneral(SettingsManager *s, SetupWizard* aWizard) : PropPage(s), wizard(aWizard) { 
	SetHeaderTitle(CTSTRING(SETTINGS_PERSONAL_INFORMATION)); 
} 

void WizardGeneral::write() {
	PropPage::write((HWND)(*this), items);

	//set the language
	//Localization::setLanguage(ctrlLanguage.GetCurSel());
}

int WizardGeneral::OnWizardNext() {
	/*if (wizard->isInitialRun()) {
		Localization::loadLanguage(ctrlLanguage.GetCurSel());
	}*/

	return FALSE;
}

int WizardGeneral::OnSetActive() {
	ShowWizardButtons( PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_FINISH | PSWIZB_CANCEL, PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_CANCEL);
	EnableWizardButtons(PSWIZB_BACK, PSWIZB_BACK);
	EnableWizardButtons(PSWIZB_NEXT, PSWIZB_NEXT);
	return 0;
}