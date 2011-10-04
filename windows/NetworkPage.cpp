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
	{ IDC_NATT,							ResourceManager::ALLOW_NAT_TRAVERSAL		},
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
	//{ IDC_BIND_ADDRESS, SettingsManager::BIND_INTERFACE, PropPage::T_STR },
	{ IDC_IPUPDATE, SettingsManager::IP_UPDATE, PropPage::T_BOOL },
	{ IDC_NATT,				SettingsManager::ALLOW_NAT_TRAVERSAL,	PropPage::T_BOOL	},
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

	PIP_ADAPTER_ADDRESSES adapter = (PIP_ADAPTER_ADDRESSES)BindCombo.GetItemDataPtr(BindCombo.GetCurSel());
	if(adapter)
		settings->set(SettingsManager::BIND_INTERFACE, adapter->AdapterName);
	else
		settings->unset(SettingsManager::BIND_INTERFACE);
}

LRESULT NetworkPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	
	if(!(WinUtil::getOsMajor() >= 5 && WinUtil::getOsMinor() >= 1  //WinXP & WinSvr2003
		|| WinUtil::getOsMajor() >= 6 )) //Vista
	{
		::EnableWindow(GetDlgItem(IDC_FIREWALL_UPNP), FALSE);
	}
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
	BindCombo.AddString((TSTRING(ANY) + (Socket::getFamily() == AF_INET6 ? _T(" (IPv4 & IPv6)") : _T(" (IPv4 only)"))).c_str());
	getAddresses();

	if(BindCombo.GetCurSel() == -1)
		BindCombo.SetCurSel(0);

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

	// trunk with win32 solution only... multiplatform solution in wx build
	ULONG len =	8192; // begin with 8 kB, it should be enough in most of cases
	for(int i = 0; i < 3; ++i)
	{
		PIP_ADAPTER_ADDRESSES adapterInfo = (PIP_ADAPTER_ADDRESSES)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
		ULONG ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, NULL, adapterInfo, &len);
		bool freeObject = true;

		if(ret == ERROR_SUCCESS)
		{
			for(PIP_ADAPTER_ADDRESSES pAdapterInfo = adapterInfo; pAdapterInfo != NULL; pAdapterInfo = pAdapterInfo->Next)
			{
				// we want only enabled ethernet interfaces
				if(pAdapterInfo->FirstUnicastAddress && pAdapterInfo->OperStatus == IfOperStatusUp && (pAdapterInfo->IfType == IF_TYPE_ETHERNET_CSMACD || pAdapterInfo->IfType == IF_TYPE_IEEE80211))
				{
					int n = BindCombo.AddString((tstring(pAdapterInfo->FriendlyName) + _T(" (IPv4 only)")).c_str());
					BindCombo.SetItemDataPtr(n, pAdapterInfo);
					freeObject = false;

					if(SETTING(BIND_INTERFACE) == pAdapterInfo->AdapterName)
						BindCombo.SetCurSel(n);
				}
			}
		}

		if(freeObject)
			HeapFree(GetProcessHeap(), 0, adapterInfo);

		if(ret != ERROR_BUFFER_OVERFLOW)
			break;
	}
}

LRESULT NetworkPage::onClickedActive(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}
void NetworkPage::on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const uint8_t* buf, size_t len) noexcept {
		downBuf = string((const char*)buf, len);
	}

void NetworkPage::on(HttpConnectionListener::Complete, HttpConnection* conn, string const& /*aLine*/, bool /*fromCoral*/) noexcept {
		conn->removeListener(this);
		if(!downBuf.empty()) {
			SimpleXML xml;
			xml.fromXML(downBuf);
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

void NetworkPage::on(HttpConnectionListener::Failed, HttpConnection* conn, const string& /*aLine*/) noexcept {
		conn->removeListener(this);
		{
			if(Util::isPrivateIp(AirUtil::getLocalIp())) {
					CheckRadioButton(IDC_DIRECT, IDC_FIREWALL_PASSIVE, IDC_FIREWALL_PASSIVE);
					fixControls();
			}
			SetDlgItemText(IDC_SERVER, Text::toT(AirUtil::getLocalIp()).c_str());	
		}
		::EnableWindow(GetDlgItem(IDC_GETIP), true);
	}
	
LRESULT NetworkPage::onGetIP(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	c = new HttpConnection();
	c->addListener(this);
	c->setCoralizeState(HttpConnection::CST_NOCORALIZE);
	c->downloadFile("http://checkip.dyndns.org/");
	return S_OK;
}
