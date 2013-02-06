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

COptionsSheet::COptionsSheet(UINT uStartPage, HWND hWndParent, SettingsManager* s) :
  CPropertySheetImpl<COptionsSheet> ((LPCTSTR)NULL, uStartPage, hWndParent ),
  m_bCentered(false), ipv6Page(new ProtocolPage(s, true)), ipv4Page(new ProtocolPage(s, false))
{
    m_psh.dwFlags |= PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;

    AddPage(ipv4Page->getPSP());
    AddPage(ipv6Page->getPSP());
}


//////////////////////////////////////////////////////////////////////
// Message handlers

void COptionsSheet::OnShowWindow ( BOOL bShowing, int nReason )
{
    if ( bShowing && !m_bCentered )
        {
        m_bCentered = true;
        CenterWindow ( m_psh.hwndParent );
        }
}

void COptionsSheet::onInit() {
	CRect rcClient, rcTab, rcPage, rcWindow;
	CWindow tab = GetTabControl();
	CWindow page = IndexToHwnd(this->m_psh.nStartPage);

	GetClientRect(&rcClient);

	tab.GetWindowRect(&rcTab);
	page.GetClientRect(&rcPage);
	page.MapWindowPoints(m_hWnd,&rcPage);
	GetWindowRect(&rcWindow);
	::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rcTab, 2);

	//ScrollWindow(SPACE_LEFT + TREE_WIDTH + SPACE_MID-rcPage.left, SPACE_TOP-rcPage.top);
	//rcWindow.right += SPACE_LEFT + TREE_WIDTH + SPACE_MID - rcPage.left - (rcClient.Width()-rcTab.right) + SPACE_RIGHT;
	//rcWindow.bottom += SPACE_TOP - rcPage.top;

	tab.ShowWindow(SW_HIDE);

	MoveWindow(&rcWindow, TRUE);

	//tabContainer.SubclassWindow(tab.m_hWnd);
}

NetworkPage::NetworkPage(SettingsManager *s) : PropPage(s), /*protocols(0, m_hWnd, s),*/ ipv6Page(new ProtocolPage(s, true)), ipv4Page(new ProtocolPage(s, false)) {
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
	//protocols.onInit();

	CRect rcPage;
	GetClientRect(&rcPage);
	//CRect rcClient, rcTab, rcPage, rcWindow;
	//CWindow tab = GetTabControl();
	//CWindow page = IndexToHwnd(this->m_psh.nStartPage);

	//GetClientRect(&rcClient);

	//tab.GetWindowRect(&rcTab);
	//GetClientRect(&rcPage);
	//page.MapWindowPoints(m_hWnd,&rcPage);
	//GetWindowRect(&rcWindow);
	//::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rcTab, 2);

	//ScrollWindow(SPACE_LEFT + TREE_WIDTH + SPACE_MID-rcPage.left, SPACE_TOP-rcPage.top);
	//rcWindow.right += SPACE_LEFT + TREE_WIDTH + SPACE_MID - rcPage.left - (rcClient.Width()-rcTab.right) + SPACE_RIGHT;
	//rcWindow.bottom += SPACE_TOP - rcPage.top;

	//ipv4Page->ShowWindow(SW_HIDE);

	//MoveWindow(&rcWindow, TRUE);

	//ipv4Page->SubclassWindow(GetDlgItem(IDC_TREE1));
	//DialogBox(

	//DialogBox(hInst, MAKEINTRESOURCE(IDD_CHILD), hParent, Child);
	//CreateDialog();
	ipv4Page->ShowWindow(SW_SHOW);
	ipv4Page->MoveWindow(rcPage);
	return TRUE;
}




ProtocolPage::ProtocolPage(SettingsManager *s, bool v6) :  PropPage(s), v6(v6) {
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
	{ IDC_OVERRIDE,		SettingsManager::NO_IP_OVERRIDE, PropPage::T_BOOL },
	{ IDC_IPUPDATE, SettingsManager::IP_UPDATE, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

void ProtocolPage::write()
{
	PropPage::write((HWND)(*this), v6 ? items6 : items4);

	// Set connection active/passive
	int ct = SettingsManager::INCOMING_ACTIVE;

	if(IsDlgButtonChecked(IDC_ACTIVE_UPNP))
		ct = SettingsManager::INCOMING_ACTIVE_UPNP;
	else if(IsDlgButtonChecked(IDC_PASSIVE))
		ct = SettingsManager::INCOMING_PASSIVE;

	if(SETTING(INCOMING_CONNECTIONS) != ct) {
		settings->set(SettingsManager::INCOMING_CONNECTIONS, ct);
	}

	auto pos = bindAddresses.begin();
	advance(pos, BindCombo.GetCurSel());
	SettingsManager::getInstance()->set(SettingsManager::BIND_ADDRESS, pos->first);
}

LRESULT ProtocolPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	
	::EnableWindow(GetDlgItem(IDC_ACTIVE_UPNP), FALSE);
	switch(SETTING(INCOMING_CONNECTIONS)) {
		case SettingsManager::INCOMING_ACTIVE: CheckDlgButton(IDC_ACTIVE, BST_CHECKED); break;
		case SettingsManager::INCOMING_ACTIVE_UPNP: CheckDlgButton(IDC_ACTIVE_UPNP, BST_CHECKED); break;
		case SettingsManager::INCOMING_PASSIVE: CheckDlgButton(IDC_PASSIVE, BST_CHECKED); break;
		default: CheckDlgButton(IDC_ACTIVE, BST_CHECKED); break;
	}

	PropPage::read((HWND)(*this), v6 ? items6 : items4);

	fixControls();

	// Bind address
	BindCombo.Attach(GetDlgItem(IDC_BIND_ADDRESS));
	getAddresses();

	auto cur = bindAddresses.find(SETTING(BIND_ADDRESS));
	if (cur == bindAddresses.end()) {
		BindCombo.AddString(Text::toT(SETTING(BIND_ADDRESS)).c_str());
		bindAddresses.emplace(SETTING(BIND_ADDRESS), make_pair(Util::emptyString, 0));
	}
	BindCombo.SetCurSel(BindCombo.FindString(0, Text::toT(SETTING(BIND_ADDRESS)).c_str()));

	return TRUE;
}

void ProtocolPage::fixControls() {
	BOOL auto_detect = IsDlgButtonChecked(IDC_CONNECTION_DETECTION) == BST_CHECKED;
	BOOL direct = IsDlgButtonChecked(IDC_ACTIVE) == BST_CHECKED;
	BOOL upnp = IsDlgButtonChecked(IDC_ACTIVE_UPNP) == BST_CHECKED;

	::EnableWindow(GetDlgItem(IDC_ACTIVE), !auto_detect);
	::EnableWindow(GetDlgItem(IDC_ACTIVE_UPNP), !auto_detect);
	::EnableWindow(GetDlgItem(IDC_PASSIVE), !auto_detect);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_IP), !auto_detect);
	::EnableWindow(GetDlgItem(IDC_BIND_ADDRESS), !auto_detect);

	::EnableWindow(GetDlgItem(IDC_EXTERNAL_IP), !auto_detect);
	::EnableWindow(GetDlgItem(IDC_OVERRIDE), !auto_detect);

	::EnableWindow(GetDlgItem(IDC_PORT_TCP), !auto_detect && (upnp || direct));
	::EnableWindow(GetDlgItem(IDC_PORT_UDP), !auto_detect && (upnp || direct));
	::EnableWindow(GetDlgItem(IDC_PORT_TLS), !auto_detect && (upnp || direct));

	::EnableWindow(GetDlgItem(IDC_MAPPER), upnp);

	::EnableWindow(GetDlgItem(IDC_IPUPDATE),!auto_detect && (direct || upnp));
	::EnableWindow(GetDlgItem(IDC_GETIP),!auto_detect && ( direct || upnp));

}

void ProtocolPage::getAddresses() {
	AirUtil::getIpAddresses(bindAddresses, false);
	bindAddresses.emplace("0.0.0.0", make_pair("Any", 0));
	for(auto& addr: bindAddresses)
		BindCombo.AddString(Text::toT(addr.first + (!addr.second.first.empty() ? " (" + addr.second.first + ")" : Util::emptyString)).c_str());
}

LRESULT ProtocolPage::onClickedActive(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

void ProtocolPage::on(UpdateManagerListener::SettingUpdated, size_t key, const string& value) noexcept {
	if (key == SettingsManager::EXTERNAL_IP) {
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
	UpdateManager::getInstance()->checkIP(true, false);
	return S_OK;
}
