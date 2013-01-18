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

#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"
#include "../client/AirUtil.h"
#include "../client/Socket.h"

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
}

PropPage::TextItem ProxyPage::texts[] = {
	{ IDC_DIRECT_OUT, ResourceManager::SETTINGS_DIRECT },
	{ IDC_SOCKS5, ResourceManager::SETTINGS_SOCKS5 }, 
	{ IDC_SETTINGS_SOCKS5_IP, ResourceManager::SETTINGS_SOCKS5_IP },
	{ IDC_SETTINGS_SOCKS5_PORT, ResourceManager::SETTINGS_SOCKS5_PORT },
	{ IDC_SETTINGS_SOCKS5_USERNAME, ResourceManager::SETTINGS_SOCKS5_USERNAME },
	{ IDC_SETTINGS_SOCKS5_PASSWORD, ResourceManager::PASSWORD },
	{ IDC_SOCKS_RESOLVE, ResourceManager::SETTINGS_SOCKS5_RESOLVE },
	{ IDC_SETTINGS_OUTGOING, ResourceManager::SETTINGS_OUTGOING },
	{ IDC_SETTINGS_HTTP_PROXY, ResourceManager::SETTINGS_HTTP_PROXY },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item ProxyPage::items[] = {
	{ IDC_SOCKS_SERVER, SettingsManager::SOCKS_SERVER,	PropPage::T_STR },
	{ IDC_SOCKS_PORT,	SettingsManager::SOCKS_PORT,	PropPage::T_INT },
	{ IDC_SOCKS_USER,	SettingsManager::SOCKS_USER,	PropPage::T_STR },
	{ IDC_SOCKS_PASSWORD, SettingsManager::SOCKS_PASSWORD, PropPage::T_STR },
	{ IDC_SOCKS_RESOLVE, SettingsManager::SOCKS_RESOLVE, PropPage::T_BOOL },
	{ IDC_PROXY, SettingsManager::HTTP_PROXY, PropPage::T_STR },
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


	int ct = SettingsManager::OUTGOING_DIRECT;
	if(IsDlgButtonChecked(IDC_SOCKS5))
		ct = SettingsManager::OUTGOING_SOCKS5;

	if(SETTING(OUTGOING_CONNECTIONS) != ct) {
		settings->set(SettingsManager::OUTGOING_CONNECTIONS, ct);
		Socket::socksUpdated();
	}
}

LRESULT ProxyPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);

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
	BOOL socks = IsDlgButtonChecked(IDC_SOCKS5);
	::EnableWindow(GetDlgItem(IDC_SOCKS_SERVER), socks);
	::EnableWindow(GetDlgItem(IDC_SOCKS_PORT), socks);
	::EnableWindow(GetDlgItem(IDC_SOCKS_USER), socks);
	::EnableWindow(GetDlgItem(IDC_SOCKS_PASSWORD), socks);
	::EnableWindow(GetDlgItem(IDC_SOCKS_RESOLVE), socks);

}