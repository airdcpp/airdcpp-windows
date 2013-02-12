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
#include "../client/AirUtil.h"
#include "../client/Socket.h"


#include "../client/Version.h"
#include "../client/UpdateManager.h"


#include "Resource.h"
#include "ProxyPage.h"
#include "WinUtil.h"

ProxyPage::ProxyPage(SettingsManager *s) : PropPage(s) {
	title = _tcsdup((TSTRING(SETTINGS_NETWORK) + _T('\\') + TSTRING(PROXIES)).c_str());
	SetTitle(title);

	m_psp.dwFlags |= PSP_RTLREADING;
}

ProxyPage::~ProxyPage() {
	free(title);

	//if(adapterInfo)
	//	HeapFree(GetProcessHeap(), 0, adapterInfo);
}

PropPage::TextItem ProxyPage::texts[] = {
	//socks
	{ IDC_DIRECT_OUT, ResourceManager::SETTINGS_DIRECT },
	{ IDC_SOCKS5, ResourceManager::SETTINGS_SOCKS5 }, 
	{ IDC_SETTINGS_SOCKS5_IP, ResourceManager::SETTINGS_SOCKS5_IP },
	{ IDC_SETTINGS_SOCKS5_PORT, ResourceManager::SETTINGS_SOCKS5_PORT },
	{ IDC_SETTINGS_SOCKS5_USERNAME, ResourceManager::SETTINGS_SOCKS5_USERNAME },
	{ IDC_SETTINGS_SOCKS5_PASSWORD, ResourceManager::PASSWORD },
	{ IDC_SOCKS_RESOLVE, ResourceManager::SETTINGS_SOCKS5_RESOLVE },
	{ IDC_SETTINGS_OUTGOING, ResourceManager::SETTINGS_OUTGOING },

	//http proxy
	{ IDC_SETTINGS_HTTP_PROXY, ResourceManager::SETTINGS_HTTP_PROXY },

	//bind address v6
	{ IDC_SETTINGS_BIND_ADDRESS, ResourceManager::SETTINGS_BIND_ADDRESS },
	{ IDC_SETTINGS_BIND_ADDRESS_HELP, ResourceManager::SETTINGS_BIND_ADDRESS_HELP },

	//conn detection v6
	{ IDC_SETTINGS_INCOMING, ResourceManager::SETTINGS_INCOMING },
	{ IDC_CONNECTION_DETECTION,			ResourceManager::CONNECTION_DETECTION		},
	{ IDC_ACTIVE, ResourceManager::SETTINGS_ACTIVE },
	{ IDC_ACTIVE_UPNP, ResourceManager::SETTINGS_ACTIVE_UPNP },
	{ IDC_PASSIVE, ResourceManager::SETTINGS_PASSIVE },

	//manual config v6
	/*{ IDC_OVERRIDE, ResourceManager::SETTINGS_OVERRIDE },
	{ IDC_SETTINGS_IP, ResourceManager::SETTINGS_EXTERNAL_IP },
	{ IDC_IPUPDATE, ResourceManager::UPDATE_IP },
	{ IDC_GETIP, ResourceManager::GET_IP },
	{ IDC_SETTINGS_MANUAL_CONFIG, ResourceManager::SETTINGS_MANUAL_CONFIG },*/
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item ProxyPage::items[] = {
	//socks
	{ IDC_SOCKS_SERVER, SettingsManager::SOCKS_SERVER,	PropPage::T_STR },
	{ IDC_SOCKS_PORT,	SettingsManager::SOCKS_PORT,	PropPage::T_INT },
	{ IDC_SOCKS_USER,	SettingsManager::SOCKS_USER,	PropPage::T_STR },
	{ IDC_SOCKS_PASSWORD, SettingsManager::SOCKS_PASSWORD, PropPage::T_STR },
	{ IDC_SOCKS_RESOLVE, SettingsManager::SOCKS_RESOLVE, PropPage::T_BOOL },

	//http proxy
	{ IDC_PROXY, SettingsManager::HTTP_PROXY, PropPage::T_STR },

	//auto detection v6
	{ IDC_CONNECTION_DETECTION,	SettingsManager::AUTO_DETECT_CONNECTION6, PropPage::T_BOOL },

	//manual config v6
	/*{ IDC_EXTERNAL_IP,	SettingsManager::EXTERNAL_IP,	PropPage::T_STR }, 
	{ IDC_OVERRIDE,		SettingsManager::NO_IP_OVERRIDE, PropPage::T_BOOL },
	{ IDC_IPUPDATE, SettingsManager::IP_UPDATE, PropPage::T_BOOL },*/
	{ 0, 0, PropPage::T_END }
};

void ProxyPage::write()
{
	TCHAR tmp[1024];
	GetDlgItemText(IDC_SOCKS_SERVER, tmp, 1024);
	tstring x = tmp;
	tstring::size_type i;

	while((i = x.find(' ')) != string::npos)
		x.erase(i, 1);
	SetDlgItemText(IDC_SOCKS_SERVER, x.c_str());
	
	PropPage::write((HWND)(*this), items);

	//socks
	int ct = SettingsManager::OUTGOING_DIRECT;
	if(IsDlgButtonChecked(IDC_SOCKS5))
		ct = SettingsManager::OUTGOING_SOCKS5;

	if(SETTING(OUTGOING_CONNECTIONS) != ct) {
		settings->set(SettingsManager::OUTGOING_CONNECTIONS, ct);
		Socket::socksUpdated();
	}


	//incoming (ipv6)
	ct = SettingsManager::INCOMING_DISABLED;
	//if (IsDlgButtonChecked(IDC_ENABLE_IPV6) == BST_CHECKED) {

	if (IsDlgButtonChecked(IDC_ENABLE_IPV6) != BST_CHECKED) {
		if(IsDlgButtonChecked(IDC_ACTIVE))
			ct = SettingsManager::INCOMING_ACTIVE;
		else if(IsDlgButtonChecked(IDC_ACTIVE_UPNP))
			ct = SettingsManager::INCOMING_ACTIVE_UPNP;
		else if(IsDlgButtonChecked(IDC_PASSIVE))
			ct = SettingsManager::INCOMING_PASSIVE;
	}

	if(SETTING(INCOMING_CONNECTIONS) != ct) {
		settings->set(SettingsManager::INCOMING_CONNECTIONS6, ct);
	}
}

LRESULT ProxyPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);

	//ipv6
	::EnableWindow(GetDlgItem(IDC_ACTIVE_UPNP), FALSE);
	switch(SETTING(INCOMING_CONNECTIONS6)) {
		case SettingsManager::INCOMING_DISABLED: CheckDlgButton(IDC_ENABLE_IPV6, BST_CHECKED); break;
		case SettingsManager::INCOMING_ACTIVE: CheckDlgButton(IDC_ACTIVE, BST_CHECKED); break;
		case SettingsManager::INCOMING_ACTIVE_UPNP: CheckDlgButton(IDC_ACTIVE_UPNP, BST_CHECKED); break;
		case SettingsManager::INCOMING_PASSIVE: CheckDlgButton(IDC_PASSIVE, BST_CHECKED); break;
		default: CheckDlgButton(IDC_ACTIVE, BST_CHECKED); break;
	}

	//socks
	switch(SETTING(OUTGOING_CONNECTIONS)) {
		case SettingsManager::OUTGOING_DIRECT: CheckDlgButton(IDC_DIRECT_OUT, BST_CHECKED); break;
		case SettingsManager::OUTGOING_SOCKS5: CheckDlgButton(IDC_SOCKS5, BST_CHECKED); break;
		default: CheckDlgButton(IDC_DIRECT_OUT, BST_CHECKED); break;
	}


	PropPage::read((HWND)(*this), items);

	fixControls();

	desc.Attach(GetDlgItem(IDC_SOCKS_SERVER));
	desc.LimitText(250);
	desc.Detach();
	desc.Attach(GetDlgItem(IDC_SOCKS_PORT));
	desc.LimitText(5);
	desc.Detach();
	desc.Attach(GetDlgItem(IDC_SOCKS_USER));
	desc.LimitText(250);
	desc.Detach();
	desc.Attach(GetDlgItem(IDC_SOCKS_PASSWORD));
	desc.LimitText(250);
	desc.Detach();
	return TRUE;
}

LRESULT ProxyPage::onClickedActive(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

void ProxyPage::fixControls() {
	//socks
	BOOL socks = IsDlgButtonChecked(IDC_SOCKS5);
	::EnableWindow(GetDlgItem(IDC_SOCKS_SERVER), socks);
	::EnableWindow(GetDlgItem(IDC_SOCKS_PORT), socks);
	::EnableWindow(GetDlgItem(IDC_SOCKS_USER), socks);
	::EnableWindow(GetDlgItem(IDC_SOCKS_PASSWORD), socks);
	::EnableWindow(GetDlgItem(IDC_SOCKS_RESOLVE), socks);

	//ipv6
	BOOL v6_enabled = IsDlgButtonChecked(IDC_ENABLE_IPV6) == BST_CHECKED;
	BOOL auto_detect = IsDlgButtonChecked(IDC_CONNECTION_DETECTION) == BST_CHECKED;
	BOOL direct = IsDlgButtonChecked(IDC_ACTIVE) == BST_CHECKED;
	BOOL upnp = IsDlgButtonChecked(IDC_ACTIVE_UPNP) == BST_CHECKED;

	::EnableWindow(GetDlgItem(IDC_ACTIVE), !auto_detect && v6_enabled);
	::EnableWindow(GetDlgItem(IDC_ACTIVE_UPNP), !auto_detect && v6_enabled);
	::EnableWindow(GetDlgItem(IDC_PASSIVE), !auto_detect && v6_enabled);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_IP), !auto_detect && v6_enabled);
	::EnableWindow(GetDlgItem(IDC_BIND_ADDRESS), !auto_detect && v6_enabled);

	::EnableWindow(GetDlgItem(IDC_EXTERNAL_IP), !auto_detect && v6_enabled);
	::EnableWindow(GetDlgItem(IDC_OVERRIDE), !auto_detect && v6_enabled);

	::EnableWindow(GetDlgItem(IDC_IPUPDATE),!auto_detect && (direct || upnp) && v6_enabled);
	::EnableWindow(GetDlgItem(IDC_GETIP),!auto_detect && ( direct || upnp) && v6_enabled);

	::EnableWindow(GetDlgItem(IDC_CONNECTION_DETECTION), v6_enabled);
}

void ProxyPage::on(UpdateManagerListener::SettingUpdated, size_t key, const string& value) noexcept {
	if (key == SettingsManager::EXTERNAL_IP) {
		if(!value.empty()) {
			SetDlgItemText(IDC_SERVER, Text::toT(value).c_str());
		} else {
			::MessageBox(m_hWnd, CTSTRING(IP_UPDATE_FAILED), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
		}
		::EnableWindow(GetDlgItem(IDC_GETIP), TRUE);
	}
}
	
LRESULT ProxyPage::onGetIP(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	::EnableWindow(GetDlgItem(IDC_GETIP), FALSE);
	UpdateManager::getInstance()->checkIP(true, true);
	return S_OK;
}