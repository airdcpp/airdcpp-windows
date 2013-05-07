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

#include "WizardAutoConnectivity.h"

PropPage::TextItem WizardAutoConnectivity::texts[] = {
	{ IDC_SETTINGS_SHARED_DIRECTORIES, ResourceManager::SETTINGS_SHARED_DIRECTORIES },
	{ IDC_ADD_PROFILE, ResourceManager::ADD_PROFILE },
	{ IDC_ADD_PROFILE_COPY, ResourceManager::ADD_PROFILE_COPY },
	{ IDC_REMOVE_PROFILE, ResourceManager::REMOVE_PROFILE },
	{ IDC_SETTINGS_SHARE_PROFILES, ResourceManager::SHARE_PROFILES },
	{ IDC_SHARE_PROFILE_NOTE, ResourceManager::SETTINGS_SHARE_PROFILE_NOTE },
	{ IDC_RENAME_PROFILE, ResourceManager::SETTINGS_RENAME_FOLDER },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT WizardAutoConnectivity::OnInitDialog(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) { 
	log.Attach(GetDlgItem(IDC_CONNECTIVITY_LOG));
	log.Subclass();

	log.SetAutoURLDetect(false);
	log.SetEventMask( log.GetEventMask() | ENM_LINK );

	cAutoDetect.Attach(GetDlgItem(IDC_AUTO_DETECT));
	cAutoDetect.EnableWindow(ConnectivityManager::getInstance()->isRunning() ? FALSE : TRUE);

	return TRUE; 
}

WizardAutoConnectivity::WizardAutoConnectivity(SettingsManager *s, SetupWizard* aWizard) : PropPage(s), wizard(aWizard) { 
	SetHeaderTitle(_T("Connectivity detection"));
	ConnectivityManager::getInstance()->addListener(this);
} 

WizardAutoConnectivity::~WizardAutoConnectivity() {
	ConnectivityManager::getInstance()->removeListener(this);
}

void WizardAutoConnectivity::write() {

}

int WizardAutoConnectivity::OnWizardNext() {
	if (!usingManualConnectivity()) {
		wizard->SetActivePage(SetupWizard::PAGE_SHARING);
		return SetupWizard::PAGE_SHARING;
	}
	return 0;
}

bool WizardAutoConnectivity::usingManualConnectivity() {
	return IsDlgButtonChecked(IDC_MANUAL_CONFIG) > 0;
}

int WizardAutoConnectivity::OnSetActive() {
	ShowWizardButtons( PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_FINISH | PSWIZB_CANCEL, PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_CANCEL); 
	EnableWizardButtons(PSWIZB_BACK, PSWIZB_BACK);
	EnableWizardButtons(PSWIZB_NEXT, ConnectivityManager::getInstance()->isRunning() ? 0 : PSWIZB_NEXT);
	return 0;
}

LRESULT WizardAutoConnectivity::OnDetectConnection(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EnableWizardButtons(PSWIZB_NEXT, 0);
	ConnectivityManager::getInstance()->detectConnection();
	return 0;
}

void WizardAutoConnectivity::addLogLine(tstring& msg) {
	/// @todo factor out to dwt
	//log->addTextSteady(Text::toT("{\\urtf1\n") + log->rtfEscape(msg + Text::toT("\r\n")) + Text::toT("}\n"));
	//log.AppendText(msg);
	log.AppendChat(Identity(NULL, 0), _T(" -"), Util::emptyStringT, msg, WinUtil::m_ChatTextGeneral, false);
}

void WizardAutoConnectivity::on(Message, const string& message) noexcept {
	callAsync([this, message] { 
		auto msg = Text::toT(message) + _T("\n");
		addLogLine(msg); 
	});
}

void WizardAutoConnectivity::on(Started, bool /*v6*/) noexcept {
	callAsync([this] {
		cAutoDetect.EnableWindow(FALSE);
		log.SetTextEx(_T(""));
		//edit->setEnabled(false);
	});
}

void WizardAutoConnectivity::on(Finished, bool /*v6*/, bool /*failed*/) noexcept {
	callAsync([this] {
		cAutoDetect.EnableWindow(TRUE);
		//edit->setEnabled(true);
		EnableWizardButtons(PSWIZB_NEXT, PSWIZB_NEXT); 
	});
}

void WizardAutoConnectivity::on(SettingChanged) noexcept {
	//callAsync([this] { updateAuto(); });
}