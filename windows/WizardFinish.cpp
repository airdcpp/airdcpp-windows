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

#include "WizardFinish.h"

PropPage::TextItem WizardFinish::texts[] = {
	{ IDC_FINISH_INTRO, ResourceManager::WIZARD_FINISHED_INTRO },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT WizardFinish::OnInitDialog(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) { 
	PropPage::translate((HWND)(*this), texts);
	return TRUE; 
}

WizardFinish::WizardFinish(SettingsManager *s, SetupWizard* aWizard) : PropPage(s), wizard(aWizard) { 
	SetHeaderTitle(CTSTRING(WIZARD_FINISHED_TITLE));
} 

void WizardFinish::write() {
	//..
}

int WizardFinish::OnWizardFinish() { 
	return wizard->OnWizardFinish();
}

int WizardFinish::OnSetActive() {
	ShowWizardButtons( PSWIZB_BACK  | PSWIZB_FINISH | PSWIZB_CANCEL, PSWIZB_BACK | PSWIZB_FINISH | PSWIZB_CANCEL);
	EnableWizardButtons(PSWIZB_BACK, PSWIZB_BACK);
	return 0;
}

int WizardFinish::OnWizardBack() {
	return 0;
}