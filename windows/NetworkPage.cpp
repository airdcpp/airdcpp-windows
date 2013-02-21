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

#include "../client/SettingsManager.h"
#include "../client/Socket.h"
#include "../client/AirUtil.h"
#include "../client/UpdateManager.h"
#include "../client/ConnectivityManager.h"

#include "Resource.h"
#include "NetworkPage.h"
#include "WinUtil.h"

NetworkPage::NetworkPage(SettingsManager *s) : PropPage(s) {
	SetTitle(CTSTRING(SETTINGS_NETWORK));
	m_psp.dwFlags |= PSP_RTLREADING;
}

NetworkPage::~NetworkPage() {
}

PropPage::TextItem NetworkPage::texts[] = {
	//ports
	{ IDC_SETTINGS_PORTS, ResourceManager::SETTINGS_PORTS },
	{ IDC_SETTINGS_PORT_TCP, ResourceManager::SETTINGS_TCP_PORT },
	{ IDC_SETTINGS_PORT_UDP, ResourceManager::SETTINGS_UDP_PORT },
	{ IDC_SETTINGS_PORT_TLS, ResourceManager::SETTINGS_TLS_PORT },

	//mapper
	{ IDC_SETTINGS_MAPPER_DESC, ResourceManager::PREFERRED_MAPPER },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item NetworkPage::items[] = {
	//ports
	{ IDC_PORT_TCP,		SettingsManager::TCP_PORT,		PropPage::T_INT }, 
	{ IDC_PORT_UDP,		SettingsManager::UDP_PORT,		PropPage::T_INT }, 
	{ IDC_PORT_TLS,		SettingsManager::TLS_PORT,		PropPage::T_INT },

	//mapper
	{ IDC_MAPPER,		SettingsManager::MAPPER,		PropPage::T_STR }, 
	{ 0, 0, PropPage::T_END }
};

void NetworkPage::write()
{
	PropPage::write((HWND)(*this), items);
	if (ipv4Page)
		ipv4Page->write();
	if (ipv6Page)
		ipv6Page->write();
}

LRESULT NetworkPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)(*this), items);

	// Mapper
	MapperCombo.Attach(GetDlgItem(IDC_MAPPER));
	const auto& setting = SETTING(MAPPER);
	int sel = 0;

	auto mappers = ConnectivityManager::getInstance()->getMappers(false);
	for(const auto& name: mappers) {
		int pos = MapperCombo.AddString(Text::toT(name).c_str());
		//auto pos = mapper->addValue(Text::toT(name));
		if(!sel && name == setting)
			sel = pos;
	}

	MapperCombo.SetCurSel(sel);

	ctrlIPv4.Attach(GetDlgItem(IDC_IPV4));
	ctrlIPv6.Attach(GetDlgItem(IDC_IPV6));

	showProtocol(false);
	return TRUE;
}

LRESULT NetworkPage::onClickProtocol(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	//hide the current window
	bool v6 = wID == IDC_IPV6;
	auto& curWindow = wID == IDC_IPV6 ? ipv4Page : ipv6Page;
	if (curWindow)
		curWindow->ShowWindow(SW_HIDE);

	CheckDlgButton(!v6 ? IDC_IPV6 : IDC_IPV4, BST_UNCHECKED);

	showProtocol(wID == IDC_IPV6);
	return TRUE;
}

void NetworkPage::showProtocol(bool v6) {
	auto& shownPage = v6 ? ipv6Page : ipv4Page;
	if (!shownPage) {
		shownPage.reset(new ProtocolPage(SettingsManager::getInstance(), v6));
		shownPage->Create(this->m_hWnd);
	}

	shownPage->ShowWindow(SW_SHOW);

	ctrlIPv6.SetState(v6);
	ctrlIPv4.SetState(!v6);
}


ProtocolPage::ProtocolPage(SettingsManager *s, bool v6) :  SettingTab(s), v6(v6) {
	UpdateManager::getInstance()->addListener(this);
}

ProtocolPage::~ProtocolPage() {
	UpdateManager::getInstance()->removeListener(this);
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
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
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
		settings->set(bind, bindAddresses[sel].ip);
}

LRESULT ProtocolPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	SettingTab::translate((HWND)(*this), texts);
	
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
	//get the addresses and sort them
	AirUtil::getIpAddresses(bindAddresses, v6);
	sort(bindAddresses.begin(), bindAddresses.end(), [](const AirUtil::AddressInfo& lhs, const AirUtil::AddressInfo& rhs) { 
		/*return compare(lhs.ip, rhs.ip) < 0;*/ 
		return stricmp(lhs.adapterName, rhs.adapterName) < 0; 
	});

	//fill the combo
	bindAddresses.emplace(bindAddresses.begin(), "Any", v6 ? "::" : "0.0.0.0", 0);
	for(auto& addr: bindAddresses)
		//BindCombo.AddString(Text::toT(addr.ip + (!addr.adapterName.empty() ? " (" + addr.adapterName + ")" : Util::emptyString)).c_str());
		BindCombo.AddString(Text::toT(addr.adapterName + " (" + addr.ip + ")").c_str());


	//select the address
	const auto& setting = v6 ? SETTING(BIND_ADDRESS6) : SETTING(BIND_ADDRESS);
	auto cur = boost::find_if(bindAddresses, [&setting](const AirUtil::AddressInfo& aInfo) { return aInfo.ip == setting; });
	if (cur == bindAddresses.end()) {
		BindCombo.AddString(Text::toT(setting).c_str());
		bindAddresses.emplace_back(STRING(UNKNOWN), setting, 0);
		cur = bindAddresses.end()-1;
	}
	BindCombo.SetCurSel(distance(bindAddresses.begin(), cur));
}

LRESULT ProtocolPage::onClickedActive(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

void ProtocolPage::on(UpdateManagerListener::SettingUpdated, size_t key, const string& value) noexcept {
	if ((key == SettingsManager::EXTERNAL_IP && !v6) || (key == SettingsManager::EXTERNAL_IP6 && v6)) {
		if(!value.empty()) {
			SetDlgItemText(IDC_SERVER, Text::toT(value).c_str());
		} else {
			::MessageBox(m_hWnd, CTSTRING(IP_UPDATE_FAILED), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
		}
		::EnableWindow(GetDlgItem(IDC_GETIP), TRUE);
	}
}
	
LRESULT ProtocolPage::onGetIP(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	::EnableWindow(GetDlgItem(IDC_GETIP), FALSE);
	UpdateManager::getInstance()->checkIP(true, v6);
	return S_OK;
}
