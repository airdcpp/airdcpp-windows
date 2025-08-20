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
#include <windows/util/WinUtil.h>
#include <windows/MainFrm.h>

#include <windows/frames/favorites/FavHubGeneralPage.h>
#include <windows/settings/base/PropertiesDlg.h>

#include <airdcpp/util/LinkUtil.h>
#include <airdcpp/favorites/FavoriteManager.h>
#include <airdcpp/core/localization/ResourceManager.h>
#include <airdcpp/core/classes/tribool.h>
#include <airdcpp/share/ShareManager.h>
#include <airdcpp/share/profiles/ShareProfileManager.h>


namespace wingui {
FavHubGeneralPage::FavHubGeneralPage(const FavoriteHubEntryPtr& _entry, const string& aName) : entry(_entry), name(aName) { }

LRESULT FavHubGeneralPage::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
	// Translate dialog
	SetWindowText(CTSTRING(FAVORITE_HUB_PROPERTIES));
	SetDlgItemText(IDCANCEL, CTSTRING(CANCEL));
	SetDlgItemText(IDC_FH_HUB, CTSTRING(HUB));
	SetDlgItemText(IDC_FH_IDENT, CTSTRING(FAVORITE_HUB_IDENTITY));
	SetDlgItemText(IDC_FH_NAME, CTSTRING(NAME));
	SetDlgItemText(IDC_EMAIL_LBL, CTSTRING(EMAIL));
	SetDlgItemText(IDC_FH_ADDRESS, CTSTRING(HUB_ADDRESS));
	SetDlgItemText(IDC_FH_HUB_DESC, CTSTRING(DESCRIPTION));
	SetDlgItemText(IDC_FH_NICK, CTSTRING(NICK));
	SetDlgItemText(IDC_FH_PASSWORD, CTSTRING(PASSWORD));
	SetDlgItemText(IDC_FH_USER_DESC, CTSTRING(DESCRIPTION));

	SetDlgItemText(IDC_FAVGROUP, CTSTRING(GROUP));

	SetDlgItemText(IDC_ENCODINGTEXT, CTSTRING(NMDC_ENCODING));

	SetDlgItemText(IDC_HIDE_SHARE, CTSTRING(HIDE_SHARE));
	SetDlgItemText(IDC_FAV_SHAREPROFILE_CAPTION, CTSTRING(SHARE_PROFILE));
	SetDlgItemText(IDC_EDIT_PROFILES, CTSTRING(EDIT_PROFILES));
	SetDlgItemText(IDC_PROFILES_NOTE, CTSTRING(PROFILES_NOTE));

	// Fill in values
	SetDlgItemText(IDC_HUBNAME, Text::toT(entry->getName()).c_str());
	SetDlgItemText(IDC_HUBDESCR, Text::toT(entry->getDescription()).c_str());
	SetDlgItemText(IDC_HUBADDR, Text::toT(entry->getServer()).c_str());
	SetDlgItemText(IDC_NICK, Text::toT(entry->get(HubSettings::Nick)).c_str());
	SetDlgItemText(IDC_HUBPASS, Text::toT(entry->getPassword()).c_str());
	SetDlgItemText(IDC_USERDESC, Text::toT(entry->get(HubSettings::Description)).c_str());
	SetDlgItemText(IDC_EMAIL, Text::toT(entry->get(HubSettings::Email)).c_str());

	bool isAdcHub = entry->isAdcHub();

	CComboBox tmpCombo;
	tmpCombo.Attach(GetDlgItem(IDC_FAVGROUP_BOX));
	tmpCombo.AddString(_T("---"));
	tmpCombo.SetCurSel(0);

	{
		RLock l(FavoriteManager::getInstance()->getCS());
		const auto& favHubGroups = FavoriteManager::getInstance()->getFavHubGroupsUnsafe();
		for (const auto& n : favHubGroups | views::keys) {
			int pos = tmpCombo.AddString(Text::toT(n).c_str());

			if (n == entry->getGroup())
				tmpCombo.SetCurSel(pos);
		}
	}

	tmpCombo.Detach();

	// TODO: add more encoding into wxWidgets version, this is enough now
	// FIXME: following names are Windows only!
	ctrlEncoding.Attach(GetDlgItem(IDC_ENCODING));
	ctrlEncoding.AddString(CTSTRING(SYSTEM_DEFAULT));
	ctrlEncoding.AddString(_T("English_United Kingdom.1252"));
	ctrlEncoding.AddString(_T("Czech_Czech Republic.1250"));
	ctrlEncoding.AddString(_T("Russian_Russia.1251"));
	ctrlEncoding.AddString(Text::toT(Text::utf8).c_str());

	ctrlProfile.Attach(GetDlgItem(IDC_FAV_SHAREPROFILE));
	appendProfiles();
	hideShare = entry->get(HubSettings::ShareProfile) == SP_HIDDEN;
	CheckDlgButton(IDC_HIDE_SHARE, hideShare ? BST_CHECKED : BST_UNCHECKED);


	if (isAdcHub) {
		ctrlEncoding.SetCurSel(4); // select UTF-8 for ADC hubs
		ctrlEncoding.EnableWindow(false);
		if (hideShare)
			ctrlProfile.EnableWindow(false);
	} else {
		ctrlProfile.EnableWindow(false);
		if (entry->get(HubSettings::NmdcEncoding).empty() || entry->get(HubSettings::NmdcEncoding) == "System default") {
			ctrlEncoding.SetCurSel(0);
		} else {
			ctrlEncoding.SetWindowText(Text::toT(entry->get(HubSettings::NmdcEncoding)).c_str());
		}
	}

	CEdit tmp;
	tmp.Attach(GetDlgItem(IDC_HUBNAME));
	tmp.SetFocus();
	tmp.SetSel(0, -1);
	tmp.Detach();
	tmp.Attach(GetDlgItem(IDC_NICK));
	tmp.LimitText(35);
	tmp.Detach();
	tmp.Attach(GetDlgItem(IDC_USERDESC));
	tmp.LimitText(50);
	tmp.Detach();
	tmp.Attach(GetDlgItem(IDC_EMAIL));
	tmp.LimitText(50);
	tmp.Detach();
	tmp.Attach(GetDlgItem(IDC_HUBPASS));
	tmp.SetPasswordChar('*');
	tmp.Detach();

	loading = false;
	return FALSE;
}
LRESULT FavHubGeneralPage::onClickedHideShare(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (IsDlgButtonChecked(IDC_HIDE_SHARE)) {
		ctrlProfile.EnableWindow(false);
		hideShare = true;
	}
	else {
		hideShare = false;
		TCHAR buf[512];
		GetDlgItemText(IDC_HUBADDR, buf, 256);
		if (LinkUtil::isAdcHub(Text::fromT(buf)))
			ctrlProfile.EnableWindow(true);
	}
	return FALSE;
}

void FavHubGeneralPage::appendProfiles() {
	auto profiles = ShareManager::getInstance()->getProfiles();

	ctrlProfile.InsertString(0, CTSTRING(USE_DEFAULT));
	ctrlProfile.SetCurSel(0);

	int counter = 1;
	for (auto j = profiles.begin(); j != profiles.end() - 1; j++) {
		ctrlProfile.InsertString(counter, Text::toT((*j)->getDisplayName()).c_str());
		if (*j == entry->get(HubSettings::ShareProfile))
			ctrlProfile.SetCurSel(counter);
		counter++;
	}
}

LRESULT FavHubGeneralPage::OnEditProfiles(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	MainFrame::getMainFrame()->openSettings(PropertiesDlg::PAGE_SHARING);
	while (ctrlProfile.GetCount()) {
		ctrlProfile.DeleteString(0);
	}
	appendProfiles();
	return FALSE;
}

LRESULT FavHubGeneralPage::OnTextChanged(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (loading)
		return 0;

	if (wID == IDC_HUBADDR) {
		CComboBox combo;
		combo.Attach(GetDlgItem(IDC_ENCODING));
		tstring address;
		address.resize(1024);
		address.resize(GetDlgItemText(IDC_HUBADDR, &address[0], 1024));

		if (LinkUtil::isAdcHub(Text::fromT(address))) {
			if (!hideShare)
				ctrlProfile.EnableWindow(true);
			combo.SetCurSel(4); // select UTF-8 for ADC hubs
			combo.EnableWindow(false);
		}
		else {
			ctrlProfile.EnableWindow(false);
			combo.EnableWindow(true);
		}
		combo.Detach();
	}

	return TRUE;
}

bool FavHubGeneralPage::write() {
	TCHAR buf[1024];
	GetDlgItemText(IDC_HUBADDR, buf, 256);
	if (buf[0] == '\0') {
		WinUtil::showMessageBox(TSTRING(INCOMPLETE_FAV_HUB), MB_ICONWARNING);
		return false;
	}

	//check the primary address for dupes
	string addresses = Text::fromT(buf);
	size_t pos = addresses.find(";");

	if (!FavoriteManager::getInstance()->isUnique(pos != string::npos ? addresses.substr(0, pos) : addresses, entry->getToken())) {
		WinUtil::showMessageBox(TSTRING(FAVORITE_HUB_ALREADY_EXISTS), MB_ICONWARNING);
		return false;
	}

	//validate the encoding
	GetDlgItemText(IDC_ENCODING, buf, 512);
	if (_tcschr(buf, _T('.')) == NULL && _tcscmp(buf, Text::toT(Text::utf8).c_str()) != 0 && ctrlEncoding.GetCurSel() != 0)
	{
		WinUtil::showMessageBox(TSTRING_F(INVALID_ENCODING, buf), MB_ICONWARNING);
		return false;
	}

	//set the values
	entry->get(HubSettings::NmdcEncoding) = ctrlEncoding.GetCurSel() != 0 ? Text::fromT(buf) : Util::emptyString;
	entry->setServer(addresses);

	GetDlgItemText(IDC_HUBNAME, buf, 256);
	entry->setName(Text::fromT(buf));
	GetDlgItemText(IDC_HUBDESCR, buf, 256);
	entry->setDescription(Text::fromT(buf));
	GetDlgItemText(IDC_HUBPASS, buf, 256);
	entry->setPassword(Text::fromT(buf));

	//Hub settings
	GetDlgItemText(IDC_NICK, buf, 256);
	entry->get(HubSettings::Nick) = Text::fromT(buf);

	GetDlgItemText(IDC_USERDESC, buf, 256);
	entry->get(HubSettings::Description) = Text::fromT(buf);

	GetDlgItemText(IDC_EMAIL, buf, 256);
	entry->get(HubSettings::Email) = Text::fromT(buf);

	CComboBox combo;
	combo.Attach(GetDlgItem(IDC_FAV_DLG_GROUP));

	if (combo.GetCurSel() == 0)
	{
		entry->setGroup(Util::emptyString);
	}
	else
	{
		tstring text(combo.GetWindowTextLength() + 1, _T('\0'));
		combo.GetWindowText(&text[0], text.size());
		text.resize(text.size() - 1);
		entry->setGroup(Text::fromT(text));
	}
	combo.Detach();

	auto p = ShareManager::getInstance()->getProfiles();

	ProfileToken token = HUB_SETTING_DEFAULT_INT;
	if (hideShare) {
		token = SP_HIDDEN;
	}
	else if (entry->isAdcHub() && ctrlProfile.GetCurSel() != 0) {
		token = p[ctrlProfile.GetCurSel() - 1]->getToken();
	}

	entry->get(HubSettings::ShareProfile) = token;

	//FavoriteManager::getInstance()->save();

	return true;
}
}
