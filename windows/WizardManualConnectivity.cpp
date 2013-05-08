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

#include "WizardManualConnectivity.h"

PropPage::TextItem WizardManualConnectivity::texts[] = {
	{ IDC_MANUALCONN_INTRO, ResourceManager::WIZARD_MANUAL_CONNECTIVITY_INTRO },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT WizardManualConnectivity::OnInitDialog(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) { 
	PropPage::translate((HWND)(*this), texts);

	protocols->Create(this->m_hWnd);
	//CRect rc;
	//::GetWindowRect(GetDlgItem(IDC_SETTINGS_SHARED_DIRECTORIES), rc);
	//::AdjustWindowRect(rc, GetWindowLongPtr(GWL_STYLE), false);
	//dirPage->SetWindowPos(m_hWnd, rc.left+10, rc.top+10, 0, 0, SWP_NOSIZE);
	protocols->SetWindowPos(HWND_TOP, 10, 35, 0, 0, SWP_NOSIZE);
	protocols->ShowWindow(SW_SHOW);

	return TRUE; 
}

WizardManualConnectivity::WizardManualConnectivity(SettingsManager *s, SetupWizard* aWizard) : PropPage(s), wizard(aWizard), protocols(new ProtocolBase(s)) { 
	SetHeaderTitle(CTSTRING(MANUAL_CONNECTIVITY_SETUP));
} 

WizardManualConnectivity::~WizardManualConnectivity() {
}

void WizardManualConnectivity::write() {
	protocols->write();
}

int WizardManualConnectivity::OnWizardNext() { 
	return 0;
}

int WizardManualConnectivity::OnSetActive() {
	ShowWizardButtons( PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_FINISH | PSWIZB_CANCEL, PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_CANCEL); 
	EnableWizardButtons(PSWIZB_BACK, PSWIZB_BACK);
	EnableWizardButtons(PSWIZB_NEXT, PSWIZB_NEXT);
	return 0;
}