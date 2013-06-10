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

#include "WizardLanguage.h"
#include "WinUtil.h"

#include "../client/AirUtil.h"
#include "../client/Localization.h"


LRESULT WizardLanguage::OnInitDialog(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) { 
	ctrlLanguage.Attach(GetDlgItem(IDC_LANGUAGE));
	WinUtil::appendLanguageMenu(ctrlLanguage);

	return TRUE; 
}

WizardLanguage::WizardLanguage(SettingsManager *s, SetupWizard* aWizard) : PropPage(s), wizard(aWizard), dl(nullptr) { 
	SetHeaderTitle(CTSTRING(SETTINGS_PERSONAL_INFORMATION)); 
} 

void WizardLanguage::write() {
	//set the language
	Localization::setLanguage(ctrlLanguage.GetCurSel());
}

int WizardLanguage::OnWizardNext() {
	auto sel = ctrlLanguage.GetCurSel();
	if (sel > 0) {
		Localization::setLanguage(sel);

		dl = new LanguageDownloadDlg(wizard->m_hWnd, [this] { completeLanguageCheck(); }, wizard->isInitialRun());
		if (!dl->show()) {
			dl->close();
			dl = nullptr;
			return FALSE;
		}

		return -1;
	}

	return FALSE;
}

void WizardLanguage::completeLanguageCheck() {
	if (wizard->isInitialRun())
		Localization::loadLanguage(ctrlLanguage.GetCurSel());

	wizard->SetActivePage(SetupWizard::PAGE_GENERAL);
	dl = nullptr;
}

WizardLanguage::~WizardLanguage() { }

int WizardLanguage::OnSetActive() {
	ShowWizardButtons( PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_FINISH | PSWIZB_CANCEL, PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_CANCEL); 
	EnableWizardButtons(PSWIZB_BACK, 0);
	EnableWizardButtons(PSWIZB_NEXT, PSWIZB_NEXT);
	return 0;
}