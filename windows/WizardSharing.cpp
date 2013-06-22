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

#include "WizardSharing.h"

PropPage::TextItem WizardSharing::texts[] = {
	{ IDC_SHARING_INTRO, ResourceManager::WIZARD_SHARING_INTRO },
	{ IDC_SHARING_SP_NOTE, ResourceManager::WIZARD_SHARING_SP_NOTE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT WizardSharing::OnInitDialog(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) { 
	PropPage::translate((HWND)(*this), texts);

	dirPage->Create(this->m_hWnd);
	//dirPage->MoveWindow(10, 10, 0, 0);

	auto factor = WinUtil::getFontFactor();
	dirPage->SetWindowPos(HWND_TOP, 10*factor, 65*factor, 0, 0, SWP_NOSIZE);
	dirPage->ShowWindow(SW_SHOW);
	return TRUE; 
}

WizardSharing::WizardSharing(SettingsManager *s, SetupWizard* aWizard) : PropPage(s), dirPage(unique_ptr<ShareDirectories>(new ShareDirectories(this, s))), wizard(aWizard) { 
	SetHeaderTitle(CTSTRING(SETTINGS_SHARED_DIRECTORIES));
} 

void WizardSharing::write() {
	//dirPage->applyChanges(true);
}

int WizardSharing::OnWizardFinish() { 
	return wizard->OnWizardFinish();
}

Dispatcher::F WizardSharing::getThreadedTask() {
	return dirPage->getThreadedTask();
}

int WizardSharing::OnSetActive() {
	if (wizard->isInitialRun()) {
		ShowWizardButtons( PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_FINISH | PSWIZB_CANCEL, PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_CANCEL); 
		EnableWizardButtons(PSWIZB_NEXT, PSWIZB_NEXT);
	} else {
		ShowWizardButtons( PSWIZB_BACK  | PSWIZB_FINISH | PSWIZB_CANCEL, PSWIZB_BACK | PSWIZB_FINISH | PSWIZB_CANCEL);
	}

	EnableWizardButtons(PSWIZB_BACK, PSWIZB_BACK);
	return 0;
}

int WizardSharing::OnWizardBack() {
	auto p = wizard->getPage<WizardAutoConnectivity>(SetupWizard::PAGE_AUTOCONN);
	if (p && !p->usingManualConnectivity()) {
		wizard->SetActivePage(SetupWizard::PAGE_AUTOCONN);
		return SetupWizard::PAGE_AUTOCONN;
	}

	return 0;
}