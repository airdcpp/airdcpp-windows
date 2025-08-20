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
#include <windows/Resource.h>

#include <windows/frames/favorites/FavHubOptionsPage.h>
#include <windows/util/WinUtil.h>

#include <airdcpp/favorites/FavoriteManager.h>
#include <airdcpp/core/localization/ResourceManager.h>
#include <airdcpp/settings/SettingsManager.h>
#include <airdcpp/core/classes/tribool.h>


namespace wingui {
FavHubOptionsPage::FavHubOptionsPage(const FavoriteHubEntryPtr& _entry, const string& aName) : entry(_entry), name(aName) { }

LRESULT FavHubOptionsPage::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{

	SetDlgItemText(IDC_SHOW_JOIN, CTSTRING(FAV_SHOW_JOIN));
	SetDlgItemText(IDC_SHOW_JOIN_FAV, CTSTRING(SETTINGS_FAV_SHOW_JOINS));
	SetDlgItemText(IDC_FAV_SEARCH_INTERVAL, CTSTRING(MINIMUM_SEARCH_INTERVAL));
	SetDlgItemText(IDC_SETTINGS_SECONDS, CTSTRING(SECONDS_LOWER));
	SetDlgItemText(IDC_SEARCH_INTERVAL_DEFAULT, CTSTRING(USE_DEFAULT));
	SetDlgItemText(IDC_LOGMAINCHAT, CTSTRING(FAV_LOG_CHAT));
	SetDlgItemText(IDC_CHAT_NOTIFY, CTSTRING(CHAT_NOTIFY));
	SetDlgItemText(IDC_LOGMAINCHAT, CTSTRING(FAV_LOG_CHAT));
	SetDlgItemText(IDC_HUBSETTINGS, CTSTRING(GLOBAL_SETTING_OVERRIDES));

	SetDlgItemText(IDC_AWAY_MSG_LBL, CTSTRING(CUSTOM_AWAY_MESSAGE));
	SetDlgItemText(IDC_FH_CONN, CTSTRING(CONNECTION));


	SetDlgItemText(IDC_AWAY_MSG, Text::toT(entry->get(HubSettings::AwayMsg)).c_str());
	
	CheckDlgButton(IDC_SHOW_JOIN, toInt(entry->get(HubSettings::ShowJoins)));
	CheckDlgButton(IDC_SHOW_JOIN_FAV, toInt(entry->get(HubSettings::FavShowJoins)));
	CheckDlgButton(IDC_LOGMAINCHAT, toInt(entry->get(HubSettings::LogMainChat)));
	CheckDlgButton(IDC_CHAT_NOTIFY, toInt(entry->get(HubSettings::ChatNotify)));

	auto searchInterval = entry->get(HubSettings::SearchInterval);
	CheckDlgButton(IDC_SEARCH_INTERVAL_DEFAULT, searchInterval == HUB_SETTING_DEFAULT_INT ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemText(IDC_FAV_SEARCH_INTERVAL_BOX, WinUtil::toStringW(searchInterval).c_str());
	
	// connection modes
	auto appendCombo = [](CComboBox& combo, int curMode) {
	combo.InsertString(0, CTSTRING(DEFAULT));
	combo.InsertString(1, CTSTRING(DISABLED));
	combo.InsertString(2, CTSTRING(ACTIVE_MODE));
	combo.InsertString(3, CTSTRING(PASSIVE_MODE));

	if (curMode == HUB_SETTING_DEFAULT_INT)
	combo.SetCurSel(0);
	else if (curMode == SettingsManager::INCOMING_DISABLED)
	combo.SetCurSel(1);
	else if (curMode == SettingsManager::INCOMING_ACTIVE)
	combo.SetCurSel(2);
	else if (curMode == SettingsManager::INCOMING_PASSIVE)
	combo.SetCurSel(3);
	};

	modeCombo4.Attach(GetDlgItem(IDC_MODE4));
	modeCombo6.Attach(GetDlgItem(IDC_MODE6));

	appendCombo(modeCombo4, entry->get(HubSettings::Connection));
	appendCombo(modeCombo6, entry->get(HubSettings::Connection6));

	//external ips
	SetDlgItemText(IDC_SERVER4, Text::toT(entry->get(HubSettings::UserIp)).c_str());
	SetDlgItemText(IDC_SERVER6, Text::toT(entry->get(HubSettings::UserIp6)).c_str());
	
	fixControls();

	CUpDownCtrl updown;
	updown.Attach(GetDlgItem(IDC_FAV_SEARCH_INTERVAL_SPIN));
	updown.SetRange32(5, 9999);
	updown.Detach();
	
	return FALSE;
}

LRESULT FavHubOptionsPage::fixControls(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return FALSE;
}

void FavHubOptionsPage::fixControls() {
	auto usingDefaultInterval = IsDlgButtonChecked(IDC_SEARCH_INTERVAL_DEFAULT);
	::EnableWindow(GetDlgItem(IDC_FAV_SEARCH_INTERVAL_BOX), !usingDefaultInterval);
	::EnableWindow(GetDlgItem(IDC_FAV_SEARCH_INTERVAL_SPIN), !usingDefaultInterval);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_SECONDS), !usingDefaultInterval);
	if (usingDefaultInterval)
		SetDlgItemText(IDC_FAV_SEARCH_INTERVAL_BOX, WinUtil::toStringW(SettingsManager::getInstance()->get(SettingsManager::MINIMUM_SEARCH_INTERVAL)).c_str());
}


bool FavHubOptionsPage::write() {
	TCHAR buf[1024];
	GetDlgItemText(IDC_AWAY_MSG, buf, 1024);
	entry->get(HubSettings::AwayMsg) = Text::fromT(buf);

	entry->get(HubSettings::ShowJoins) = to3bool(IsDlgButtonChecked(IDC_SHOW_JOIN));
	entry->get(HubSettings::FavShowJoins) = to3bool(IsDlgButtonChecked(IDC_SHOW_JOIN_FAV));
	entry->get(HubSettings::LogMainChat) = to3bool(IsDlgButtonChecked(IDC_LOGMAINCHAT));
	entry->get(HubSettings::ChatNotify) = to3bool(IsDlgButtonChecked(IDC_CHAT_NOTIFY));

	auto val = HUB_SETTING_DEFAULT_INT;
	if (!IsDlgButtonChecked(IDC_SEARCH_INTERVAL_DEFAULT)) {
		GetDlgItemText(IDC_FAV_SEARCH_INTERVAL_BOX, buf, 512);
		val = Util::toInt(Text::fromT(buf));
	}
	entry->get(HubSettings::SearchInterval) = val;

	// connection modes
	auto getConnMode = [](const CComboBox& combo) -> int {
		if (combo.GetCurSel() == 1)
			return SettingsManager::INCOMING_DISABLED;
		else if (combo.GetCurSel() == 2)
			return SettingsManager::INCOMING_ACTIVE;
		else if (combo.GetCurSel() == 3)
			return SettingsManager::INCOMING_PASSIVE;

		return HUB_SETTING_DEFAULT_INT;
	};

	entry->get(HubSettings::Connection) = getConnMode(modeCombo4);
	entry->get(HubSettings::Connection6) = getConnMode(modeCombo6);

	//external ip addresses
	GetDlgItemText(IDC_SERVER4, buf, 512);
	entry->get(HubSettings::UserIp) = Text::fromT(buf);

	GetDlgItemText(IDC_SERVER6, buf, 512);
	entry->get(HubSettings::UserIp6) = Text::fromT(buf);
	
	FavoriteManager::getInstance()->setDirty();

	return true;
}
}