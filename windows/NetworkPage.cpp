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
#include "WinUtil.h"

#include "../client/ConnectivityManager.h"

NetworkPage::NetworkPage(SettingsManager *s) : PropPage(s), protocols(new ProtocolBase(s)) {
	SetTitle(CTSTRING(SETTINGS_NETWORK));
	m_psp.dwFlags |= PSP_RTLREADING;
}

NetworkPage::~NetworkPage() {
}

PropPage::TextItem NetworkPage::texts[] = {
	//mapper
	{ IDC_SETTINGS_MAPPER_DESC, ResourceManager::PREFERRED_MAPPER },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item NetworkPage::items[] = {
	//mapper
	{ IDC_MAPPER,		SettingsManager::MAPPER,		PropPage::T_STR }, 
	{ 0, 0, PropPage::T_END }
};

void NetworkPage::write()
{
	PropPage::write((HWND)(*this), items);
	protocols->write();
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

	protocols->Create(this->m_hWnd);
	//CRect rc;
	//::GetWindowRect(GetDlgItem(IDC_SETTINGS_SHARED_DIRECTORIES), rc);
	//::AdjustWindowRect(rc, GetWindowLongPtr(GWL_STYLE), false);
	//dirPage->SetWindowPos(m_hWnd, rc.left+10, rc.top+10, 0, 0, SWP_NOSIZE);

	auto factor = WinUtil::getFontFactor();
	protocols->SetWindowPos(HWND_TOP, 5*factor, 0, 0, 0, SWP_NOSIZE);
	protocols->ShowWindow(SW_SHOW);
	
	return TRUE;
}