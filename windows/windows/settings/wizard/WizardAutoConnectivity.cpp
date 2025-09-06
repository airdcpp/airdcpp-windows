/*
 * Copyright (C) 2012-2024 AirDC++ Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include <windows/stdafx.h>

#include <windows/settings/wizard/WizardAutoConnectivity.h>

#include <airdcpp/connectivity/ConnectivityManager.h>
#include <airdcpp/message/Message.h>

namespace wingui {
PropPage::TextItem WizardAutoConnectivity::texts[] = {
	{ IDC_IPV4_AUTODETECT, ResourceManager::ALLOW_AUTO_DETECT_V4 },
	{ IDC_IPV6_AUTODETECT, ResourceManager::ALLOW_AUTO_DETECT_V6 },
	{ IDC_AUTOCONN_INTRO, ResourceManager::WIZARD_AUTO_CONNECTIVITY_INTRO },
	{ IDC_AUTO_DETECT, ResourceManager::DETECT_NOW },
	{ IDC_MANUAL_CONFIG, ResourceManager::CONN_USE_MANUAL_CONFIG },
	{ 0, ResourceManager::LAST }
};

PropPage::Item WizardAutoConnectivity::items[] = {
	{ IDC_IPV4_AUTODETECT,		SettingsManager::AUTO_DETECT_CONNECTION,		PropPage::T_BOOL }, 
	{ IDC_IPV6_AUTODETECT,		SettingsManager::AUTO_DETECT_CONNECTION6,		PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

LRESULT WizardAutoConnectivity::OnInitDialog(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) { 
	PropPage::translate((HWND)(*this), texts);

	log.Attach(GetDlgItem(IDC_CONNECTIVITY_LOG));
	log.Subclass();

	log.SetAutoURLDetect(false);
	log.SetEventMask( log.GetEventMask() | ENM_LINK );

	cAutoDetect.Attach(GetDlgItem(IDC_AUTO_DETECT));
	cAutoDetect.EnableWindow(ConnectivityManager::getInstance()->isRunning() ? FALSE : TRUE);

	cDetectIPv4.Attach(GetDlgItem(IDC_IPV4_AUTODETECT));
	cDetectIPv6.Attach(GetDlgItem(IDC_IPV6_AUTODETECT));

	cManualDetect.Attach(GetDlgItem(IDC_MANUAL_CONFIG));

	updateAuto();
	if (SETTING(NICK).empty() && SETTING(AUTO_DETECT_CONNECTION) && SETTING(AUTO_DETECT_CONNECTION)) {
		//initial run
		detectConnection();
	}

	return TRUE; 
}

LRESULT WizardAutoConnectivity::OnTickAutoDetect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	// apply immediately so that ConnectivityManager updates.
	if ((cDetectIPv4.GetCheck() > 0 ? true : false) != SETTING(AUTO_DETECT_CONNECTION))
		SettingsManager::getInstance()->set(SettingsManager::AUTO_DETECT_CONNECTION, cDetectIPv4.GetCheck() > 0);
	if ((cDetectIPv6.GetCheck() > 0 ? true : false) != SETTING(AUTO_DETECT_CONNECTION6))
		SettingsManager::getInstance()->set(SettingsManager::AUTO_DETECT_CONNECTION6, cDetectIPv6.GetCheck() > 0);

	ConnectivityManager::getInstance()->fire(ConnectivityManagerListener::SettingChanged());
	return 0;
}

void WizardAutoConnectivity::updateAuto() {
	PropPage::read((HWND)*this, items);
	if (cDetectIPv4.GetCheck() == 0 && cDetectIPv6.GetCheck() == 0) {
		cAutoDetect.EnableWindow(FALSE);
	} else if (v6State != STATE_DETECTING && v4State != STATE_DETECTING) {
		cAutoDetect.EnableWindow(TRUE);
	}
}

WizardAutoConnectivity::WizardAutoConnectivity(SettingsManager *s, SetupWizard* aWizard) : PropPage(s), wizard(aWizard), v4State(STATE_UNKNOWN), v6State(STATE_UNKNOWN) { 
	SetHeaderTitle(CTSTRING(AUTO_CONNECTIVITY_DETECTION));
} 

WizardAutoConnectivity::~WizardAutoConnectivity() {

}

void WizardAutoConnectivity::write() {
	PropPage::write((HWND)(*this), items);
}

int WizardAutoConnectivity::OnWizardNext() {
	if (!usingManualConnectivity()) {
		wizard->SetActivePage(SetupWizard::PAGE_SHARING);
		return SetupWizard::PAGE_SHARING;
	}

	return 0;
}

bool WizardAutoConnectivity::usingManualConnectivity() {
	return cManualDetect.GetCheck() > 0;
}

int WizardAutoConnectivity::OnSetActive() {
	ConnectivityManager::getInstance()->addListener(this);

	ShowWizardButtons( PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_FINISH | PSWIZB_CANCEL, PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_CANCEL); 
	EnableWizardButtons(PSWIZB_BACK, PSWIZB_BACK);
	EnableWizardButtons(PSWIZB_NEXT, ConnectivityManager::getInstance()->isRunning() ? 0 : PSWIZB_NEXT);
	return 0;
}

int WizardAutoConnectivity::OnKillActive() {
	ConnectivityManager::getInstance()->removeListener(this);
	return 0;
}

LRESULT WizardAutoConnectivity::OnDetectConnection(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	detectConnection();
	return 0;
}

void WizardAutoConnectivity::detectConnection() {
	EnableWizardButtons(PSWIZB_NEXT, 0);
	ConnectivityManager::getInstance()->detectConnection();
}

void WizardAutoConnectivity::addLogLine(const tstring& aMessage, CHARFORMAT2W& cf /*WinUtil::m_ChatTextGeneral*/) {
	log.AppendMessage(dcpp::Message::fromText(Text::fromT(aMessage), LogMessage::InitFlags::INIT_DISABLE_TIMESTAMP), cf, false);
}

void WizardAutoConnectivity::on(ConnectivityManagerListener::Message, const string& aMessage, LogMessage::Severity /*aSeverity*/) noexcept {
	wizard->callAsync([this, aMessage] {
		auto msg = Text::toT(aMessage) /* + _T("\n")*/;
		addLogLine(msg); 
	});
}

void WizardAutoConnectivity::on(ConnectivityManagerListener::Started, bool v6) noexcept {
	(v6 ? v6State :v4State) = STATE_DETECTING;
	wizard->callAsync([this] {
		changeControlState(false);
		log.SetTextEx(_T(""));
	});
}

void WizardAutoConnectivity::on(ConnectivityManagerListener::Finished, bool v6, bool failed) noexcept {
	if (v6){
		v6State = failed ? STATE_FAILED : STATE_SUCCEED;
	} else {
		v4State = failed ? STATE_FAILED : STATE_SUCCEED;
	}

	/*if (failed) {
		string msg = "Setting up " + v6 ? "IPv6" : "IPv4" + "failed; it's recommended to set up the connectivity manually";
	}*/

	wizard->callAsync([this] {
		if (v6State != STATE_DETECTING && v4State != STATE_DETECTING) {
			CHARFORMAT2 cf;
			memzero(&cf, sizeof(CHARFORMAT2));
			cf.cbSize = sizeof(cf);
			cf.dwReserved = 0;
			cf.dwMask = CFM_COLOR | CFM_BOLD | CFM_ITALIC;
			cf.dwEffects = 0;
			cf.crTextColor = RGB(3, 93, 35);

			tstring msg = TSTRING(CONN_DETECT_FINISHED);
			addLogLine(msg, cf);

			changeControlState(true);
			EnableWizardButtons(PSWIZB_NEXT, PSWIZB_NEXT);
		}
	});
}

void WizardAutoConnectivity::changeControlState(bool enable) {
	cAutoDetect.EnableWindow(enable ? TRUE : false);
	cDetectIPv4.EnableWindow(enable ? TRUE : false);
	cDetectIPv6.EnableWindow(enable ? TRUE : false);
}

void WizardAutoConnectivity::on(ConnectivityManagerListener::SettingChanged) noexcept {
	wizard->callAsync([this] { updateAuto(); });
}
}