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

#include "Resource.h"
#include "NetworkPage.h"

#include "../client/ConnectivityManager.h"

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