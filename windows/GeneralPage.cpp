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
#include "../client/version.h"
#include "Resource.h"

#include "GeneralPage.h"
#include "WinUtil.h"

PropPage::TextItem GeneralPage::texts[] = {
	{ IDC_SETTINGS_PERSONAL_INFORMATION, ResourceManager::SETTINGS_PERSONAL_INFORMATION },
	{ IDC_SETTINGS_NICK, ResourceManager::NICK },
	{ IDC_SETTINGS_EMAIL, ResourceManager::EMAIL },
	{ IDC_SETTINGS_DESCRIPTION, ResourceManager::DESCRIPTION },
	{ IDC_SETTINGS_UPLOAD_LINE_SPEED, ResourceManager::SETTINGS_UPLOAD_LINE_SPEED },
	{ IDC_SETTINGS_MEBIBITS, ResourceManager::MBITSPS },
	{ IDC_ENG, ResourceManager::SETTINGS_LENG },
	{ IDC_LANG_SWE, ResourceManager::SETTINGS_LSWE },
	{ IDC_LANG_FIN, ResourceManager::SETTINGS_LFIN },
	{ IDC_LANG_ITA, ResourceManager::SETTINGS_LITA },
	{ IDC_LANG_HUN, ResourceManager::SETTINGS_LHUN },
	{ IDC_LANG_RO, ResourceManager::SETTINGS_LRO },
	{ IDC_LANG_DAN, ResourceManager::SETTINGS_LDAN },
	{ IDC_LANG_NOR, ResourceManager::SETTINGS_LNOR },
	{ IDC_LANG_POR, ResourceManager::SETTINGS_LPOR },
	{ IDC_LANG_POL, ResourceManager::SETTINGS_LPOL },
	{ IDC_LANG_FR, ResourceManager::SETTINGS_LFR },
	{ IDC_LANG_D, ResourceManager::SETTINGS_LD },
	{ IDC_LANG_RUS, ResourceManager::SETTINGS_LRUS },
	{ IDC_SETTINGS_NOMINALBW2, ResourceManager::SETTINGS_LANGUAGE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item GeneralPage::items[] = {
	{ IDC_NICK,			SettingsManager::NICK,			PropPage::T_STR }, 
	{ IDC_EMAIL,		SettingsManager::EMAIL,			PropPage::T_STR }, 
	{ IDC_DESCRIPTION,	SettingsManager::DESCRIPTION,	PropPage::T_STR }, 
	{ IDC_CONNECTION,	SettingsManager::UPLOAD_SPEED,	PropPage::T_STR },
	{ 0, 0, PropPage::T_END }
};

void GeneralPage::write()
{
	PropPage::write((HWND)(*this), items);
}

LRESULT GeneralPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	ctrlConnection.Attach(GetDlgItem(IDC_CONNECTION));

	for(StringIter i = SettingsManager::connectionSpeeds.begin(); i != SettingsManager::connectionSpeeds.end(); ++i)
		ctrlConnection.AddString(Text::toT(*i).c_str());

		if(find(SettingsManager::connectionSpeeds.begin(), SettingsManager::connectionSpeeds.end(),
			SETTING(UPLOAD_SPEED)) == SettingsManager::connectionSpeeds.end()) {
		ctrlConnection.AddString(Text::toT(SETTING(UPLOAD_SPEED)).c_str());
	}

	PropPage::read((HWND)(*this), items);

	ctrlConnection.SetCurSel(ctrlConnection.FindString(0, Text::toT(SETTING(UPLOAD_SPEED)).c_str()));

	fixControls();
	nick.Attach(GetDlgItem(IDC_NICK));
	nick.LimitText(35);
	nick.Detach();

	desc.Attach(GetDlgItem(IDC_DESCRIPTION));
	desc.LimitText(35);
	desc.Detach();
	desc.Attach(GetDlgItem(IDC_SETTINGS_EMAIL));
	desc.LimitText(35);
	desc.Detach();
	return TRUE;
}

LRESULT GeneralPage::onTextChanged(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
	TCHAR buf[SETTINGS_BUF_LEN];

	GetDlgItemText(wID, buf, SETTINGS_BUF_LEN);
	tstring old = buf;

	// Strip '$', '|', '<', '>' and ' ' from text
	TCHAR *b = buf, *f = buf, c;
	while( (c = *b++) != 0 )
	{
		if(c != '$' && c != '|' && (wID == IDC_DESCRIPTION || c != ' ') && ( (wID != IDC_NICK && wID != IDC_DESCRIPTION && wID != IDC_SETTINGS_EMAIL) || (c != '<' && c != '>') ) )
			*f++ = c;
	}

	*f = '\0';

	if(old != buf)
	{
		// Something changed; update window text without changing cursor pos
		CEdit tmp;
		tmp.Attach(hWndCtl);
		int start, end;
		tmp.GetSel(start, end);
		tmp.SetWindowText(buf);
		if(start > 0) start--;
		if(end > 0) end--;
		tmp.SetSel(start, end);
		tmp.Detach();
	}

	return TRUE;
}

void GeneralPage::fixControls() {

	if (SETTING(LANGUAGE_SWITCH) == 0) {
		CheckDlgButton(IDC_LANG_SWE, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FIN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_ITA, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_HUN, BST_UNCHECKED);
		CheckDlgButton(IDC_ENG, BST_CHECKED);
		CheckDlgButton(IDC_LANG_RO, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_DAN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_NOR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POL, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_D, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RUS, BST_UNCHECKED);
	}else if (SETTING(LANGUAGE_SWITCH) == 1) {
		CheckDlgButton(IDC_ENG, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_SWE, BST_CHECKED);
		CheckDlgButton(IDC_LANG_FIN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_ITA, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_HUN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RO, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_DAN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_NOR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POL, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_D, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RUS, BST_UNCHECKED);
	} else if (SETTING(LANGUAGE_SWITCH) == 2) {
		CheckDlgButton(IDC_ENG, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_SWE, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FIN, BST_CHECKED);
		CheckDlgButton(IDC_LANG_ITA, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_HUN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RO, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_DAN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_NOR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POL, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_D, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RUS, BST_UNCHECKED);
	} else if (SETTING(LANGUAGE_SWITCH) == 3) {
		CheckDlgButton(IDC_ENG, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_SWE, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FIN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_ITA, BST_CHECKED);
		CheckDlgButton(IDC_LANG_HUN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RO, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_DAN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_NOR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POL, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_D, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RUS, BST_UNCHECKED);
	} else if (SETTING(LANGUAGE_SWITCH) == 4) {
		CheckDlgButton(IDC_ENG, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_SWE, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FIN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_ITA, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_HUN, BST_CHECKED);
		CheckDlgButton(IDC_LANG_RO, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_DAN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_NOR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POL, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_D, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RUS, BST_UNCHECKED);
	}else if (SETTING(LANGUAGE_SWITCH) == 5) {
		CheckDlgButton(IDC_ENG, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_SWE, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FIN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_ITA, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_HUN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RO, BST_CHECKED);
		CheckDlgButton(IDC_LANG_DAN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_NOR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POL, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_D, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RUS, BST_UNCHECKED);
	}else if (SETTING(LANGUAGE_SWITCH) == 6) {
		CheckDlgButton(IDC_ENG, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_SWE, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FIN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_ITA, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_HUN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RO, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_DAN, BST_CHECKED);
		CheckDlgButton(IDC_LANG_NOR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POL, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_D, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RUS, BST_UNCHECKED);
	} else if (SETTING(LANGUAGE_SWITCH) == 7) {
		CheckDlgButton(IDC_ENG, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_SWE, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FIN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_ITA, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_HUN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RO, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_DAN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_NOR, BST_CHECKED);
		CheckDlgButton(IDC_LANG_POR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POL, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_D, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RUS, BST_UNCHECKED);
	} else if (SETTING(LANGUAGE_SWITCH) == 8) {
		CheckDlgButton(IDC_ENG, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_SWE, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FIN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_ITA, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_HUN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RO, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_DAN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_NOR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POR, BST_CHECKED);
		CheckDlgButton(IDC_LANG_POL, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_D, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RUS, BST_UNCHECKED);
	} else if (SETTING(LANGUAGE_SWITCH) == 9) {
		CheckDlgButton(IDC_ENG, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_SWE, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FIN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_ITA, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_HUN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RO, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_DAN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_NOR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POL, BST_CHECKED);
		CheckDlgButton(IDC_LANG_FR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_D, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RUS, BST_UNCHECKED);
	} else if (SETTING(LANGUAGE_SWITCH) == 10) {
		CheckDlgButton(IDC_ENG, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_SWE, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FIN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_ITA, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_HUN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RO, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_DAN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_NOR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POL, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FR, BST_CHECKED);
		CheckDlgButton(IDC_LANG_D, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RUS, BST_UNCHECKED);
	} else if (SETTING(LANGUAGE_SWITCH) == 11) {
		CheckDlgButton(IDC_ENG, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_SWE, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FIN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_ITA, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_HUN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RO, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_DAN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_NOR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POL, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RUS, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_D, BST_CHECKED);
} else if (SETTING(LANGUAGE_SWITCH) == 12) {
		CheckDlgButton(IDC_ENG, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_SWE, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FIN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_ITA, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_HUN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RO, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_DAN, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_NOR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_POL, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_FR, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_D, BST_UNCHECKED);
		CheckDlgButton(IDC_LANG_RUS, BST_CHECKED);
	}
}

LRESULT GeneralPage::onLng(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(IsDlgButtonChecked(IDC_ENG)) {
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_SWITCH, 0);
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, Util::emptyString);
	fixControls();
	MessageBox(_T("Ok!\r\nRestart the AirDC++ client!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONWARNING | MB_OK);
	}
	if(IsDlgButtonChecked(IDC_LANG_SWE)) {
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_SWITCH, 1);
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Swedish_for_AirDc.xml"));
	fixControls();
	MessageBox(_T("Ok!\r\nStarta om AirDC++ klienten!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONWARNING | MB_OK);
	}
	if(IsDlgButtonChecked(IDC_LANG_FIN)) {
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_SWITCH, 2);
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Finnish_for_AirDc.xml"));
	fixControls();
	MessageBox(_T("Ok!\r\nKäynnistä AirDC++ uudelleen!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONWARNING | MB_OK);
	}

	if(IsDlgButtonChecked(IDC_LANG_ITA)) {
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_SWITCH, 3);
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Italian_for_AirDc.xml"));
	fixControls();
	MessageBox(_T("Ok!\r\nRestart the AirDC++ client!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONWARNING | MB_OK);
	}
	if(IsDlgButtonChecked(IDC_LANG_HUN)) {
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_SWITCH, 4);
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Hungarian_for_AirDc.xml"));
	fixControls();
	MessageBox(_T("Ok!\r\nRestart the AirDC++ client!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONWARNING | MB_OK);
	}
	if(IsDlgButtonChecked(IDC_LANG_RO)) {
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_SWITCH, 5);
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Romanian_for_AirDc.xml"));
	fixControls();
	MessageBox(_T("Ok!\r\nReporneste AirDC++!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONWARNING | MB_OK);
	}
	if(IsDlgButtonChecked(IDC_LANG_DAN)) {
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_SWITCH, 6);
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Danish_for_AirDc.xml"));
	fixControls();
	MessageBox(_T("Ok!\r\nGenstart Airdc++ clienten!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONWARNING | MB_OK);
	}
	if(IsDlgButtonChecked(IDC_LANG_NOR)) {
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_SWITCH, 7);
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Norwegian_for_AirDC.xml"));
	fixControls();
	MessageBox(_T("Ok!\r\nRestart the AirDC++ client!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONWARNING | MB_OK);
	}
	if(IsDlgButtonChecked(IDC_LANG_POR)) {
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_SWITCH, 8);
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Port_Br_for_AirDC.xml"));
	fixControls();
	MessageBox(_T("Ok!\r\nRestart the AirDC++ client!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONWARNING | MB_OK);
	}
	if(IsDlgButtonChecked(IDC_LANG_POL)) {
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_SWITCH, 9);
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Polish_for_AirDc.xml"));
	fixControls();
	MessageBox(_T("Ok!\r\nZrestartuj klienta AirDC++!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONWARNING | MB_OK);
	}
	if(IsDlgButtonChecked(IDC_LANG_FR)) {
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_SWITCH, 10);
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//French_for_AirDC.xml"));
	fixControls();
	MessageBox(_T("Ok!\r\nRestart the AirDC++ client!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONWARNING | MB_OK);
	}
	if(IsDlgButtonChecked(IDC_LANG_D)) {
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_SWITCH, 11);
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Dutch_for_AirDC.xml"));
	fixControls();
	MessageBox(_T("Ok!\r\nRestart the AirDC++ client!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONWARNING | MB_OK);
	}
if(IsDlgButtonChecked(IDC_LANG_RUS)) {
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_SWITCH, 12);
	SettingsManager::getInstance()->set(SettingsManager::LANGUAGE_FILE, (Util::getPath(Util::PATH_GLOBAL_CONFIG) + "Language//Russian_for_AirDC.xml"));
	fixControls();
	MessageBox(_T("Ok!\r\nRestart the AirDC++ client!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONWARNING | MB_OK);
	}
	return 0;
}


/**
 * @file
 * $Id: GeneralPage.cpp 306 2007-07-10 19:46:55Z bigmuscle $
 */
