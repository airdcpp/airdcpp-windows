/* 
 * Copyright (C) 2001-2019 Jacek Sieka, arnetheduck on gmail point com
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

#include <airdcpp/SettingsManager.h>
#include <airdcpp/Socket.h>
#include <airdcpp/AirUtil.h>
#include <airdcpp/UpdateManager.h>
#include <airdcpp/ConnectivityManager.h>
#include <airdcpp/version.h>

#include "Resource.h"
#include "ProtocolPage.h"
#include "WinUtil.h"


ProtocolBase::ProtocolBase(SettingsManager *s) : SettingTab(s) {
	//SetTitle(CTSTRING(SETTINGS_NETWORK));
	//m_psp.dwFlags |= PSP_RTLREADING;
}

ProtocolBase::~ProtocolBase() {
}

PropPage::TextItem ProtocolBase::texts[] = {
	//ports
	{ IDC_SETTINGS_PORTS, ResourceManager::SETTINGS_PORTS },
	{ IDC_SETTINGS_PORT_TCP, ResourceManager::SETTINGS_TCP_PORT },
	{ IDC_SETTINGS_PORT_UDP, ResourceManager::SETTINGS_UDP_PORT },
	{ IDC_SETTINGS_PORT_TLS, ResourceManager::SETTINGS_TLS_PORT },
	{ IDC_PROTOCOL_OPTIONS, ResourceManager::PROTOCOL_OPTIONS },

	//mapper
	//{ IDC_SETTINGS_MAPPER_DESC, ResourceManager::PREFERRED_MAPPER },
	{ 0, ResourceManager::LAST }
};

PropPage::Item ProtocolBase::items[] = {
	//ports
	{ IDC_PORT_TCP,		SettingsManager::TCP_PORT,		PropPage::T_INT }, 
	{ IDC_PORT_UDP,		SettingsManager::UDP_PORT,		PropPage::T_INT }, 
	{ IDC_PORT_TLS,		SettingsManager::TLS_PORT,		PropPage::T_INT },

	//mapper
	//{ IDC_MAPPER,		SettingsManager::MAPPER,		PropPage::T_STR }, 
	{ 0, 0, PropPage::T_END }
};

void ProtocolBase::write()
{
	if (!validatePorts())
		GetDlgItem(IDC_PORT_TLS).SetWindowText(_T(""));

	SettingTab::write((HWND)(*this), items);
	if (ipv4Page)
		ipv4Page->write();
	if (ipv6Page)
		ipv6Page->write();
}

LRESULT ProtocolBase::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{

	SettingTab::translate((HWND)(*this), texts);
	SettingTab::read((HWND)(*this), items);

	tabs.Attach(GetDlgItem(IDC_TAB1));
	tabs.InsertItem(IPV4, TCIF_TEXT, _T("IPv4"), -1, NULL);
	tabs.InsertItem(IPV6, TCIF_TEXT, _T("IPv6"), -1, NULL);
	tabs.SetCurSel(0);
	showProtocol(false);

	created = true;

	return TRUE;
}

LRESULT ProtocolBase::onProtocolChanged(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {

	switch (tabs.GetCurSel()) {
	case IPV4:
		if (ipv6Page)
			ipv6Page->ShowWindow(SW_HIDE);

		showProtocol(false);
		break;

	case IPV6:
		if (ipv4Page)
			ipv4Page->ShowWindow(SW_HIDE);

		showProtocol(true);
		break;
	default:
		break;
	}
	return 0;
}

void ProtocolBase::showProtocol(bool v6) {

	auto& shownPage = v6 ? ipv6Page : ipv4Page;
	if (!shownPage) {
		shownPage.reset(new ProtocolPage(SettingsManager::getInstance(), v6));
		shownPage->Create(m_hWnd);
		CRect rc;
		tabs.GetItemRect(0,rc);
		shownPage->SetWindowPos(HWND_BOTTOM, rc.left, rc.bottom+1, 0, 0, SWP_NOSIZE);
	}
	shownPage->ShowWindow(SW_SHOW);
	//tab control doesn't seem to stay at the bottom of Z-Order 
	tabs.SetWindowPos(HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE);
}

LRESULT ProtocolBase::onPortsUpdated(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWnd, BOOL& bHandled) {

	if (created && (hWnd == GetDlgItem(IDC_PORT_TLS).m_hWnd || hWnd == GetDlgItem(IDC_PORT_TCP).m_hWnd)) {
		if (!validatePorts()) {
			WinUtil::showMessageBox(TSTRING(INVALID_PORTS), MB_ICONERROR);
		}
	}

	bHandled = FALSE;
	return 0;
}

bool ProtocolBase::validatePorts() {
	CEdit tmp;
	tmp.Attach(GetDlgItem(IDC_PORT_TLS));
	tstring tls = WinUtil::getEditText(tmp);
	tmp.Detach();

	tmp.Attach(GetDlgItem(IDC_PORT_TCP));
	tstring tcp = WinUtil::getEditText(tmp);
	tmp.Detach();

	if (tls == tcp) {
		return false;
	}

	return true;
}


ProtocolPage::ProtocolPage(SettingsManager *s, bool v6) :  SettingTab(s), v6(v6) {
	UpdateManager::getInstance()->addListener(this);
	ConnectivityManager::getInstance()->addListener(this);
}

ProtocolPage::~ProtocolPage() {
	UpdateManager::getInstance()->removeListener(this);
	ConnectivityManager::getInstance()->removeListener(this);
}


PropPage::TextItem ProtocolPage::texts[] = {
	//bind address
	{ IDC_SETTINGS_BIND_ADDRESS, ResourceManager::SETTINGS_BIND_ADDRESS },
	{ IDC_SETTINGS_BIND_ADDRESS_HELP, ResourceManager::SETTINGS_BIND_ADDRESS_HELP },

	//conn detection
	{ IDC_SETTINGS_INCOMING, ResourceManager::SETTINGS_INCOMING },
	{ IDC_CONNECTION_DETECTION,			ResourceManager::CONNECTION_DETECTION		},
	{ IDC_ACTIVE, ResourceManager::SETTINGS_ACTIVE },
	{ IDC_ACTIVE_UPNP, ResourceManager::SETTINGS_ACTIVE_UPNP },
	{ IDC_PASSIVE, ResourceManager::SETTINGS_PASSIVE },

	//manual config
	{ IDC_OVERRIDE, ResourceManager::SETTINGS_OVERRIDE },
	{ IDC_SETTINGS_IP, ResourceManager::SETTINGS_EXTERNAL_IP },
	{ IDC_IPUPDATE, ResourceManager::UPDATE_IP },
	{ IDC_GETIP, ResourceManager::GET_IP },
	{ IDC_SETTINGS_MANUAL_CONFIG, ResourceManager::SETTINGS_MANUAL_CONFIG },
	{ 0, ResourceManager::LAST }
};

PropPage::Item ProtocolPage::items4[] = {
	//auto detection
	{ IDC_CONNECTION_DETECTION,	SettingsManager::AUTO_DETECT_CONNECTION,	PropPage::T_BOOL	},

	//manual config
	{ IDC_EXTERNAL_IP,	SettingsManager::EXTERNAL_IP,	PropPage::T_STR }, 
	{ IDC_OVERRIDE,		SettingsManager::NO_IP_OVERRIDE, PropPage::T_BOOL },
	{ IDC_IPUPDATE, SettingsManager::IP_UPDATE, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

PropPage::Item ProtocolPage::items6[] = {
	//auto detection
	{ IDC_CONNECTION_DETECTION,	SettingsManager::AUTO_DETECT_CONNECTION6,	PropPage::T_BOOL	},

	//manual config
	{ IDC_EXTERNAL_IP,	SettingsManager::EXTERNAL_IP6,	PropPage::T_STR }, 
	{ IDC_OVERRIDE,		SettingsManager::NO_IP_OVERRIDE6, PropPage::T_BOOL },
	{ IDC_IPUPDATE, SettingsManager::IP_UPDATE6, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

void ProtocolPage::write()
{
	SettingTab::write((HWND)(*this), v6 ? items6 : items4);

	// Set connection active/passive
	int ct = SettingsManager::INCOMING_DISABLED;
	if (IsDlgButtonChecked(IDC_PROTOCOL_ENABLED) == BST_CHECKED) {
		if(IsDlgButtonChecked(IDC_ACTIVE))
			ct = SettingsManager::INCOMING_ACTIVE;
		else if(IsDlgButtonChecked(IDC_ACTIVE_UPNP))
			ct = SettingsManager::INCOMING_ACTIVE_UPNP;
		else if(IsDlgButtonChecked(IDC_PASSIVE))
			ct = SettingsManager::INCOMING_PASSIVE;
	}


	const auto mode = v6 ? SettingsManager::INCOMING_CONNECTIONS6 : SettingsManager::INCOMING_CONNECTIONS;
	if(ct != settings->getDefault(mode) || !settings->isDefault(mode)) {
		settings->set(mode, ct);
	}

	auto sel = BindCombo.GetCurSel();
	const auto bind = v6 ? SettingsManager::BIND_ADDRESS6 : SettingsManager::BIND_ADDRESS;
	if (sel > 0 || !settings->isDefault(bind))
		settings->set(bind, bindAdapters[sel].ip);
}

LRESULT ProtocolPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	SettingTab::translate((HWND)(*this), texts);
	
	cAutoDetect.Attach(GetDlgItem(IDC_CONNECTION_DETECTION));
	SetDlgItemText(IDC_PROTOCOL_ENABLED, CTSTRING_F(ENABLE_CONNECTIVITY, (v6 ? "IPv6" : "IPv4")));

	auto mode = v6 ? SETTING(INCOMING_CONNECTIONS6) : SETTING(INCOMING_CONNECTIONS);
	if (mode == SettingsManager::INCOMING_DISABLED) {
		CheckDlgButton(IDC_PROTOCOL_ENABLED, BST_UNCHECKED);
		CheckDlgButton(IDC_ACTIVE, BST_CHECKED);
	} else {
		CheckDlgButton(IDC_PROTOCOL_ENABLED, BST_CHECKED);
		switch(mode) {
		case SettingsManager::INCOMING_ACTIVE: CheckDlgButton(IDC_ACTIVE, BST_CHECKED); break;
		case SettingsManager::INCOMING_ACTIVE_UPNP: CheckDlgButton(IDC_ACTIVE_UPNP, BST_CHECKED); break;
		case SettingsManager::INCOMING_PASSIVE: CheckDlgButton(IDC_PASSIVE, BST_CHECKED); break;
		default: CheckDlgButton(IDC_ACTIVE, BST_CHECKED); break;
		}
	}

	SettingTab::read((HWND)(*this), v6 ? items6 : items4);

	fixControls();

	// Bind address
	BindCombo.Attach(GetDlgItem(IDC_BIND_ADDRESS));
	getAddresses();

	return TRUE;
}

void ProtocolPage::fixControls() {
	BOOL enabled = IsDlgButtonChecked(IDC_PROTOCOL_ENABLED) == BST_CHECKED;
	BOOL auto_detect = IsDlgButtonChecked(IDC_CONNECTION_DETECTION) == BST_CHECKED;
	BOOL direct = IsDlgButtonChecked(IDC_ACTIVE) == BST_CHECKED;
	BOOL upnp = IsDlgButtonChecked(IDC_ACTIVE_UPNP) == BST_CHECKED;

	::EnableWindow(GetDlgItem(IDC_CONNECTION_DETECTION), enabled);
	::EnableWindow(GetDlgItem(IDC_ACTIVE), !auto_detect && enabled);
	::EnableWindow(GetDlgItem(IDC_ACTIVE_UPNP), !auto_detect && enabled);
	::EnableWindow(GetDlgItem(IDC_PASSIVE), !auto_detect && enabled);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_IP), !auto_detect && enabled);
	::EnableWindow(GetDlgItem(IDC_BIND_ADDRESS), !auto_detect && enabled);

	::EnableWindow(GetDlgItem(IDC_EXTERNAL_IP), !auto_detect && enabled);
	::EnableWindow(GetDlgItem(IDC_OVERRIDE), !auto_detect && enabled);

	::EnableWindow(GetDlgItem(IDC_PORT_TCP), !auto_detect && (upnp || direct) && enabled);
	::EnableWindow(GetDlgItem(IDC_PORT_UDP), !auto_detect && (upnp || direct) && enabled);
	::EnableWindow(GetDlgItem(IDC_PORT_TLS), !auto_detect && (upnp || direct) && enabled);

	::EnableWindow(GetDlgItem(IDC_MAPPER), upnp && enabled);

	::EnableWindow(GetDlgItem(IDC_IPUPDATE),!auto_detect && (direct || upnp) && enabled);
	::EnableWindow(GetDlgItem(IDC_GETIP),!auto_detect && ( direct || upnp) && enabled);

}

void ProtocolPage::getAddresses() {
	// fill the combo
	bindAdapters = AirUtil::getCoreBindAdapters(v6);

	const auto& curSetting = v6 ? SETTING(BIND_ADDRESS6) : SETTING(BIND_ADDRESS);
	WinUtil::insertBindAddresses(bindAdapters, BindCombo, curSetting);
}

LRESULT ProtocolPage::onClickedActive(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

LRESULT ProtocolPage::onClickedAutoDetect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	// apply immediately so that ConnectivityManager updates.
	SettingsManager::getInstance()->set(v6 ? SettingsManager::AUTO_DETECT_CONNECTION6 : SettingsManager::AUTO_DETECT_CONNECTION, cAutoDetect.GetCheck() > 0);
	ConnectivityManager::getInstance()->fire(ConnectivityManagerListener::SettingChanged());
	fixControls();
	return 0;
}

void ProtocolPage::on(UpdateManagerListener::SettingUpdated, size_t key, const string& value) noexcept {
	if ((key == SettingsManager::EXTERNAL_IP && !v6) || (key == SettingsManager::EXTERNAL_IP6 && v6)) {
		if(!value.empty()) {
			SetDlgItemText(IDC_SERVER, Text::toT(value).c_str());
		} else {
			::MessageBox(m_hWnd, CTSTRING(IP_UPDATE_FAILED), Text::toT(shortVersionString).c_str(), MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
		}
		::EnableWindow(GetDlgItem(IDC_GETIP), TRUE);
	}
}
	
LRESULT ProtocolPage::onGetIP(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	::EnableWindow(GetDlgItem(IDC_GETIP), FALSE);
	UpdateManager::getInstance()->checkIP(true, v6);
	return S_OK;
}

void ProtocolPage::on(SettingChanged) noexcept {
	callAsync([this] {
		cAutoDetect.SetCheck((v6 ? SETTING(AUTO_DETECT_CONNECTION6) : SETTING(AUTO_DETECT_CONNECTION)) ? TRUE : FALSE);

		fixControls();

		// reload settings in case they have been changed (eg by the "Edit detected settings" feature).

		/*active->setChecked(false);
		upnp->setChecked(false);
		passive->setChecked(false);

		mapper->clear();*/

		//read();
	});
}