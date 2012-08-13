/* 
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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

#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"
#include "../client/Socket.h"
#include "../client/AirUtil.h"

#include "Resource.h"
#include "NetworkPage.h"
#include "WinUtil.h"

#include <IPHlpApi.h>
#pragma comment(lib, "iphlpapi.lib")

PropPage::TextItem NetworkPage::texts[] = {
		{ IDC_CONNECTION_DETECTION,			ResourceManager::CONNECTION_DETECTION		},
	{ IDC_DIRECT, ResourceManager::SETTINGS_DIRECT },
	{ IDC_DIRECT_OUT, ResourceManager::SETTINGS_DIRECT },
	{ IDC_FIREWALL_UPNP, ResourceManager::SETTINGS_FIREWALL_UPNP },
	{ IDC_FIREWALL_NAT, ResourceManager::SETTINGS_FIREWALL_NAT },
	{ IDC_FIREWALL_PASSIVE, ResourceManager::SETTINGS_FIREWALL_PASSIVE },
	{ IDC_OVERRIDE, ResourceManager::SETTINGS_OVERRIDE },
	{ IDC_SOCKS5, ResourceManager::SETTINGS_SOCKS5 }, 
	{ IDC_SETTINGS_PORTS, ResourceManager::SETTINGS_PORTS },
	{ IDC_SETTINGS_IP, ResourceManager::SETTINGS_EXTERNAL_IP },
	{ IDC_SETTINGS_PORT_TCP, ResourceManager::SETTINGS_TCP_PORT },
	{ IDC_SETTINGS_PORT_UDP, ResourceManager::SETTINGS_UDP_PORT },
	{ IDC_SETTINGS_PORT_TLS, ResourceManager::SETTINGS_TLS_PORT },
	{ IDC_SETTINGS_SOCKS5_IP, ResourceManager::SETTINGS_SOCKS5_IP },
	{ IDC_SETTINGS_SOCKS5_PORT, ResourceManager::SETTINGS_SOCKS5_PORT },
	{ IDC_SETTINGS_SOCKS5_USERNAME, ResourceManager::SETTINGS_SOCKS5_USERNAME },
	{ IDC_SETTINGS_SOCKS5_PASSWORD, ResourceManager::PASSWORD },
	{ IDC_SOCKS_RESOLVE, ResourceManager::SETTINGS_SOCKS5_RESOLVE },
	{ IDC_SETTINGS_INCOMING, ResourceManager::SETTINGS_INCOMING },
	{ IDC_SETTINGS_OUTGOING, ResourceManager::SETTINGS_OUTGOING },
	{ IDC_SETTINGS_BIND_ADDRESS, ResourceManager::SETTINGS_BIND_ADDRESS },
	{ IDC_SETTINGS_BIND_ADDRESS_HELP, ResourceManager::SETTINGS_BIND_ADDRESS_HELP },
	{ IDC_IPUPDATE, ResourceManager::UPDATE_IP },
	{ IDC_GETIP, ResourceManager::GET_IP },
	{ IDC_NATT,	ResourceManager::ALLOW_NAT_TRAVERSAL },
	{ IDC_SETTINGS_HTTP_PROXY, ResourceManager::SETTINGS_HTTP_PROXY },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item NetworkPage::items[] = {
		{ IDC_CONNECTION_DETECTION,	SettingsManager::AUTO_DETECT_CONNECTION,	PropPage::T_BOOL	},
	{ IDC_EXTERNAL_IP,	SettingsManager::EXTERNAL_IP,	PropPage::T_STR }, 
	{ IDC_PORT_TCP,		SettingsManager::TCP_PORT,		PropPage::T_INT }, 
	{ IDC_PORT_UDP,		SettingsManager::UDP_PORT,		PropPage::T_INT }, 
	{ IDC_PORT_TLS,		SettingsManager::TLS_PORT,		PropPage::T_INT },
	{ IDC_OVERRIDE,		SettingsManager::NO_IP_OVERRIDE, PropPage::T_BOOL },
	{ IDC_SOCKS_SERVER, SettingsManager::SOCKS_SERVER,	PropPage::T_STR },
	{ IDC_SOCKS_PORT,	SettingsManager::SOCKS_PORT,	PropPage::T_INT },
	{ IDC_SOCKS_USER,	SettingsManager::SOCKS_USER,	PropPage::T_STR },
	{ IDC_SOCKS_PASSWORD, SettingsManager::SOCKS_PASSWORD, PropPage::T_STR },
	{ IDC_SOCKS_RESOLVE, SettingsManager::SOCKS_RESOLVE, PropPage::T_BOOL },
	{ IDC_BIND_ADDRESS, SettingsManager::BIND_ADDRESS, PropPage::T_STR },
	{ IDC_IPUPDATE, SettingsManager::IP_UPDATE, PropPage::T_BOOL },
	{ IDC_NATT,	SettingsManager::ALLOW_NAT_TRAVERSAL, PropPage::T_BOOL },
	{ IDC_PROXY, SettingsManager::HTTP_PROXY, PropPage::T_STR },
	{ 0, 0, PropPage::T_END }
};

void NetworkPage::write()
{
	TCHAR tmp[1024];
	GetDlgItemText(IDC_SOCKS_SERVER, tmp, 1024);
	tstring x = tmp;
	tstring::size_type i;

	while((i = x.find(' ')) != string::npos)
		x.erase(i, 1);
	SetDlgItemText(IDC_SOCKS_SERVER, x.c_str());

	GetDlgItemText(IDC_SERVER, tmp, 1024);
	x = tmp;

	while((i = x.find(' ')) != string::npos)
		x.erase(i, 1);

	SetDlgItemText(IDC_SERVER, x.c_str());
	
	PropPage::write((HWND)(*this), items);

	// Set connection active/passive
	int ct = SettingsManager::INCOMING_DIRECT;

	if(IsDlgButtonChecked(IDC_FIREWALL_UPNP))
		ct = SettingsManager::INCOMING_FIREWALL_UPNP;
	else if(IsDlgButtonChecked(IDC_FIREWALL_NAT))
		ct = SettingsManager::INCOMING_FIREWALL_NAT;
	else if(IsDlgButtonChecked(IDC_FIREWALL_PASSIVE))
		ct = SettingsManager::INCOMING_FIREWALL_PASSIVE;

	if(SETTING(INCOMING_CONNECTIONS) != ct) {
		settings->set(SettingsManager::INCOMING_CONNECTIONS, ct);
	}

	ct = SettingsManager::OUTGOING_DIRECT;
	
	if(IsDlgButtonChecked(IDC_SOCKS5))
		ct = SettingsManager::OUTGOING_SOCKS5;

	if(SETTING(OUTGOING_CONNECTIONS) != ct) {
		settings->set(SettingsManager::OUTGOING_CONNECTIONS, ct);
		Socket::socksUpdated();
	}
}

LRESULT NetworkPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	
	::EnableWindow(GetDlgItem(IDC_FIREWALL_UPNP), FALSE);
	switch(SETTING(INCOMING_CONNECTIONS)) {
		case SettingsManager::INCOMING_DIRECT: CheckDlgButton(IDC_DIRECT, BST_CHECKED); break;
		case SettingsManager::INCOMING_FIREWALL_UPNP: CheckDlgButton(IDC_FIREWALL_UPNP, BST_CHECKED); break;
		case SettingsManager::INCOMING_FIREWALL_NAT: CheckDlgButton(IDC_FIREWALL_NAT, BST_CHECKED); break;
		case SettingsManager::INCOMING_FIREWALL_PASSIVE: CheckDlgButton(IDC_FIREWALL_PASSIVE, BST_CHECKED); break;
		default: CheckDlgButton(IDC_DIRECT, BST_CHECKED); break;
	}

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

	BindCombo.Attach(GetDlgItem(IDC_BIND_ADDRESS));
	//BindCombo.AddString(_T("0.0.0.0"));
	getAddresses();
	BindCombo.SetCurSel(BindCombo.FindString(0, Text::toT(SETTING(BIND_ADDRESS)).c_str()));
	
	if(BindCombo.GetCurSel() == -1) {
		BindCombo.AddString(Text::toT(SETTING(BIND_ADDRESS)).c_str());
		BindCombo.SetCurSel(BindCombo.FindString(0, Text::toT(SETTING(BIND_ADDRESS)).c_str()));
	}

	return TRUE;
}

void NetworkPage::fixControls() {
	BOOL auto_detect = IsDlgButtonChecked(IDC_CONNECTION_DETECTION) == BST_CHECKED;
	BOOL direct = IsDlgButtonChecked(IDC_DIRECT) == BST_CHECKED;
	BOOL upnp = IsDlgButtonChecked(IDC_FIREWALL_UPNP) == BST_CHECKED;
	BOOL nat = IsDlgButtonChecked(IDC_FIREWALL_NAT) == BST_CHECKED;
	BOOL nat_traversal = IsDlgButtonChecked(IDC_NATT) == BST_CHECKED;

	::EnableWindow(GetDlgItem(IDC_DIRECT), !auto_detect);
	::EnableWindow(GetDlgItem(IDC_FIREWALL_UPNP), !auto_detect);
	::EnableWindow(GetDlgItem(IDC_FIREWALL_NAT), !auto_detect);
	::EnableWindow(GetDlgItem(IDC_FIREWALL_PASSIVE), !auto_detect);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_IP), !auto_detect);
	::EnableWindow(GetDlgItem(IDC_BIND_ADDRESS), !auto_detect);

	::EnableWindow(GetDlgItem(IDC_EXTERNAL_IP), !auto_detect && (direct || upnp || nat || nat_traversal));
	::EnableWindow(GetDlgItem(IDC_OVERRIDE), !auto_detect && (direct || upnp || nat || nat_traversal));

	::EnableWindow(GetDlgItem(IDC_PORT_TCP), !auto_detect && (upnp || nat));
	::EnableWindow(GetDlgItem(IDC_PORT_UDP), !auto_detect && (upnp || nat));
	::EnableWindow(GetDlgItem(IDC_PORT_TLS), !auto_detect && (upnp || nat));
	::EnableWindow(GetDlgItem(IDC_NATT), !auto_detect && !direct && !upnp && !nat); // for passive settings only



	::EnableWindow(GetDlgItem(IDC_IPUPDATE),!auto_detect && (direct || upnp || nat || nat_traversal));
	::EnableWindow(GetDlgItem(IDC_GETIP),!auto_detect && ( direct || upnp || nat || nat_traversal));

	BOOL socks = IsDlgButtonChecked(IDC_SOCKS5);
	::EnableWindow(GetDlgItem(IDC_SOCKS_SERVER), socks);
	::EnableWindow(GetDlgItem(IDC_SOCKS_PORT), socks);
	::EnableWindow(GetDlgItem(IDC_SOCKS_USER), socks);
	::EnableWindow(GetDlgItem(IDC_SOCKS_PASSWORD), socks);
	::EnableWindow(GetDlgItem(IDC_SOCKS_RESOLVE), socks);

}

void NetworkPage::getAddresses() {
	IP_ADAPTER_INFO* AdapterInfo = NULL;
	DWORD dwBufLen = NULL;

	DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
	if(dwStatus == ERROR_BUFFER_OVERFLOW) {
		AdapterInfo = (IP_ADAPTER_INFO*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBufLen);
		dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);		
	}

	if(dwStatus == ERROR_SUCCESS) {
		PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
		while (pAdapterInfo) {
			IP_ADDR_STRING* pIpList = &(pAdapterInfo->IpAddressList);
			while (pIpList) {
				BindCombo.AddString(Text::toT(pIpList->IpAddress.String).c_str());
				pIpList = pIpList->Next;
			}
			pAdapterInfo = pAdapterInfo->Next;
		}
	}
	
	if(AdapterInfo)
		HeapFree(GetProcessHeap(), 0, AdapterInfo);	
}

LRESULT NetworkPage::onClickedActive(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

void NetworkPage::completeDownload() {
	if(!c->buf.empty()) {
		SimpleXML xml;
		xml.fromXML(c->buf);
		if(xml.findChild("html")) {
			xml.stepIn();
			if(xml.findChild("body")) {
				string x = xml.getChildData().substr(20);
				if(Util::isPrivateIp(x)) {
						CheckRadioButton(IDC_DIRECT, IDC_FIREWALL_PASSIVE, IDC_FIREWALL_PASSIVE);
						fixControls();
				}
				SetDlgItemText(IDC_SERVER, Text::toT(x).c_str());
				//::MessageBox(NULL, _T("IP fetched: checkip.dyndns.org"), _T("Debug"), MB_OK);
			} else {
				if(Util::isPrivateIp(AirUtil::getLocalIp())) {
						CheckRadioButton(IDC_DIRECT, IDC_FIREWALL_PASSIVE, IDC_FIREWALL_PASSIVE);
						fixControls();
				}
				SetDlgItemText(IDC_SERVER, Text::toT(AirUtil::getLocalIp()).c_str());
			}
		}
	}
	::EnableWindow(GetDlgItem(IDC_GETIP), true);
}
	
LRESULT NetworkPage::onGetIP(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	c.reset(new HttpDownload("http://checkip.dyndns.org/",
		[this] { completeDownload(); }, false));
	return S_OK;
}
