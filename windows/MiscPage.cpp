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
#include "../client/version.h"
#include "../client/SimpleXML.h"
#include "Resource.h"


#include "MiscPage.h"
#include "WinUtil.h"
#include "LineDlg.h"
#include "PropertiesDlg.h"

#include "../client/ResourceManager.h"

PropPage::TextItem MiscPage::texts[] = {
	{ IDC_CZDC_WINAMP, ResourceManager::SETCZDC_WINAMP },
	{ IDC_PLAYER_PATH, ResourceManager::SETCZDC_WINAMP_PATH },
	{ IDC_WINAMP_BROWSE, ResourceManager::BROWSEW },
	{ IDC_OPEN_FIRST, ResourceManager::OPEN_FIRST },
	{ IDC_FAV_DL, ResourceManager::FAV_DL_SPEED },
	{ IDC_PASSWD_PROTECT_STATIC, ResourceManager::PASSWD_PROTECT_STATIC },
	{ IDC_NOTE, ResourceManager::NOTE },
	{ IDC_ALLOW_CONNECTION_TO_PASSED_HUBS, ResourceManager::DISALLOW_CONNECTION_TO_PASSED_HUBS },
	{ IDC_PASSWD_PROTECT_TRAY_CHCKBOX, ResourceManager::PASSWD_PROTECT_TRAY_CHCKBOX },
	{ IDC_PASSWD_PROTECT_CHCKBOX, ResourceManager::PASSWD_PROTECT_CHCKBOX },
	{ IDC_SKIP_SUBTRACT_TEXT, ResourceManager::SKIP_SUBTRACT_TEXT },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item MiscPage::items[] = {
	{ IDC_FAV_DL_SPEED, SettingsManager::FAV_DL_SPEED, PropPage::T_INT },
	{ IDC_WINAMP, SettingsManager::WINAMP_FORMAT, PropPage::T_STR },
	{ IDC_PASSWD_PROTECT_CHCKBOX, SettingsManager::PASSWD_PROTECT, PropPage::T_BOOL },
	{ IDC_PASSWD_PROTECT_TRAY_CHCKBOX, SettingsManager::PASSWD_PROTECT_TRAY, PropPage::T_BOOL },
	{ IDC_ALLOW_CONNECTION_TO_PASSED_HUBS, SettingsManager::DISALLOW_CONNECTION_TO_PASSED_HUBS, PropPage::T_BOOL },
	{ IDC_SKIP_SUBTRACT, SettingsManager::SKIP_SUBTRACT, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

LRESULT MiscPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	CurSel = SETTING(MEDIA_PLAYER);
	
	WinampStr = Text::toT(SETTING(WINAMP_FORMAT));
	iTunesStr = Text::toT(SETTING(ITUNES_FORMAT));
	MPCStr = Text::toT(SETTING(MPLAYERC_FORMAT));
	WMPlayerStr = Text::toT(SETTING(WMP_FORMAT));
	SpotifyStr = Text::toT(SETTING(SPOTIFY_FORMAT));

	ctrlPlayer.Attach(GetDlgItem(IDC_PLAYER_COMBO));
	ctrlPlayer.AddString(_T("Winamp"));
	ctrlPlayer.AddString(_T("iTunes"));
	ctrlPlayer.AddString(_T("Media Player Classic"));
	ctrlPlayer.AddString(_T("Windows Media Player"));
	ctrlPlayer.AddString(_T("Spotify"));
	ctrlPlayer.SetCurSel(CurSel);

	if(CurSel == 0) {
		SetDlgItemText(IDC_WINAMP, WinampStr.c_str());
	
	} else if(CurSel == 1) {
		SetDlgItemText(IDC_WINAMP, iTunesStr.c_str());
	} else if(CurSel == 2) {
		SetDlgItemText(IDC_WINAMP, MPCStr.c_str());
	} else if(CurSel == 3) {
		SetDlgItemText(IDC_WINAMP, WMPlayerStr.c_str());
	} else if(CurSel == 4) {
		SetDlgItemText(IDC_WINAMP, SpotifyStr.c_str());
	} else {
		SetDlgItemText(IDC_WINAMP, CTSTRING(NO_MEDIA_SPAM));
		::EnableWindow(GetDlgItem(IDC_WINAMP), false);
		::EnableWindow(GetDlgItem(IDC_WINAMP_HELP), false);
	}
	
	SetDlgItemText(IDC_WINAMP_PATH, Text::toT(SETTING(WINAMP_PATH)).c_str());
	SetDlgItemText(IDC_ANTIVIR_PATH, Text::toT(SETTING(ANTIVIR_PATH)).c_str());
	
	fixControls();

	return TRUE;
}


void MiscPage::write() {
	PropPage::write((HWND)*this, items);

	TCHAR buf[256];
	GetDlgItemText(IDC_WINAMP_PATH, buf, 256);
	settings->set(SettingsManager::WINAMP_PATH, Text::fromT(buf));
	

	ctrlFormat.Attach(GetDlgItem(IDC_WINAMP));
	if(CurSel == 0) {
		int len = ctrlFormat.GetWindowTextLength() + 1;
		TCHAR buf[256];
		GetDlgItemText(IDC_WINAMP, buf, len);
		WinampStr = buf;
	} else if(CurSel == 1) {
		int len = ctrlFormat.GetWindowTextLength() + 1;
		TCHAR buf[256];
		GetDlgItemText(IDC_WINAMP, buf, len);
		iTunesStr = buf;
	} else if(CurSel == 2) {
		int len = ctrlFormat.GetWindowTextLength() + 1;
		TCHAR buf[256];
		GetDlgItemText(IDC_WINAMP, buf, len);
		MPCStr = buf;
	} 
	else if(CurSel == 3) {
		int len = ctrlFormat.GetWindowTextLength() + 1;
		TCHAR buf[256];
		GetDlgItemText(IDC_WINAMP, buf, len);
		WMPlayerStr = buf;
	}
	else if(CurSel == 4) {
		int len = ctrlFormat.GetWindowTextLength() + 1;
		TCHAR buf[256];
		GetDlgItemText(IDC_WINAMP, buf, len);
		SpotifyStr = buf;
	}
	ctrlFormat.Detach();

	SettingsManager::getInstance()->set(SettingsManager::MEDIA_PLAYER, ctrlPlayer.GetCurSel());
	SettingsManager::getInstance()->set(SettingsManager::WINAMP_FORMAT, Text::fromT(WinampStr).c_str());
	SettingsManager::getInstance()->set(SettingsManager::WMP_FORMAT, Text::fromT(WMPlayerStr).c_str());
	SettingsManager::getInstance()->set(SettingsManager::ITUNES_FORMAT, Text::fromT(iTunesStr).c_str());
	SettingsManager::getInstance()->set(SettingsManager::MPLAYERC_FORMAT, Text::fromT(MPCStr).c_str());
	SettingsManager::getInstance()->set(SettingsManager::SPOTIFY_FORMAT, Text::fromT(SpotifyStr).c_str());

}
LRESULT MiscPage::onClickedWinampHelp(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
		if(CurSel == 0) {
		MessageBox(CTSTRING(WINAMP_HELP), CTSTRING(WINAMP_HELP_DESC), MB_OK | MB_ICONINFORMATION);
	} else if(CurSel == 1) {
		MessageBox(CTSTRING(ITUNES_HELP), CTSTRING(ITUNES_HELP_DESC), MB_OK | MB_ICONINFORMATION);
	} else if(CurSel == 2) {
		MessageBox(CTSTRING(MPC_HELP), CTSTRING(MPC_HELP_DESC), MB_OK | MB_ICONINFORMATION);
	}
	else if(CurSel == 3) {
		MessageBox(CTSTRING(WMP_HELP), CTSTRING(WMP_HELP_DESC), MB_OK | MB_ICONINFORMATION);
	}
	else if(CurSel == 4) {
		MessageBox(CTSTRING(SPOTIFY_HELP), CTSTRING(SPOTIFY_HELP_DESC), MB_OK | MB_ICONINFORMATION);
	}
	return S_OK;
}
LRESULT MiscPage::onBrowsew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TCHAR buf[MAX_PATH];

	GetDlgItemText(IDC_WINAMP_PATH, buf, MAX_PATH);
	tstring x = buf;

	if(WinUtil::browseFile(x, m_hWnd, false) == IDOK) {
		SetDlgItemText(IDC_WINAMP_PATH, x.c_str());
	}
	return 0;
}

LRESULT MiscPage::onSelChange(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	ctrlFormat.Attach(GetDlgItem(IDC_WINAMP));
	if(CurSel == 0) {
		int len = ctrlFormat.GetWindowTextLength() + 1;
		TCHAR buf[256];
		GetDlgItemText(IDC_WINAMP, buf, len);
		WinampStr = buf;
	} else if(CurSel == 1) {
		int len = ctrlFormat.GetWindowTextLength() + 1;
		TCHAR buf[256];
		GetDlgItemText(IDC_WINAMP, buf, len);
		iTunesStr = buf;
	} else if(CurSel == 2) {
		int len = ctrlFormat.GetWindowTextLength() + 1;
		TCHAR buf[256];
		GetDlgItemText(IDC_WINAMP, buf, len);
		MPCStr = buf;
	} 
	else if(CurSel == 3) {
		int len = ctrlFormat.GetWindowTextLength() + 1;
		TCHAR buf[256];
		GetDlgItemText(IDC_WINAMP, buf, len);
		WMPlayerStr = buf;
	}
	else if(CurSel == 4) {
		int len = ctrlFormat.GetWindowTextLength() + 1;
		TCHAR buf[256];
		GetDlgItemText(IDC_WINAMP, buf, len);
		SpotifyStr = buf;
	}
	ctrlFormat.Detach();

	CurSel = ctrlPlayer.GetCurSel();

	if(CurSel == 0) {
		SetDlgItemText(IDC_WINAMP, WinampStr.c_str());
		::EnableWindow(GetDlgItem(IDC_WINAMP), true);
		::EnableWindow(GetDlgItem(IDC_WINAMP_HELP), true);
	} else if(CurSel == 1) {
		SetDlgItemText(IDC_WINAMP, iTunesStr.c_str());
		::EnableWindow(GetDlgItem(IDC_WINAMP), true);
		::EnableWindow(GetDlgItem(IDC_WINAMP_HELP), true);
	} else if(CurSel == 2) {
		SetDlgItemText(IDC_WINAMP, MPCStr.c_str());
		::EnableWindow(GetDlgItem(IDC_WINAMP), true);
		::EnableWindow(GetDlgItem(IDC_WINAMP_HELP), true);
	} 
	else if(CurSel == 3) {
		SetDlgItemText(IDC_WINAMP, WMPlayerStr.c_str());
		::EnableWindow(GetDlgItem(IDC_WINAMP), true);
		::EnableWindow(GetDlgItem(IDC_WINAMP_HELP), true);
	} 
	else if(CurSel == 4) {
		SetDlgItemText(IDC_WINAMP, SpotifyStr.c_str());
		::EnableWindow(GetDlgItem(IDC_WINAMP), true);
		::EnableWindow(GetDlgItem(IDC_WINAMP_HELP), true);
	} else {
		SetDlgItemText(IDC_WINAMP, CTSTRING(NO_MEDIA_SPAM));
		::EnableWindow(GetDlgItem(IDC_WINAMP), false);
		::EnableWindow(GetDlgItem(IDC_WINAMP_HELP), false);
	}
	return 0;
}

LRESULT MiscPage::onChangeCont(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(wID == IDC_PASSWD_PROTECT_CHCKBOX) {
		bool state = (IsDlgButtonChecked(IDC_PASSWD_PROTECT_CHCKBOX) != 0);
		if(state) {
			ChngPassDlg dlg;
			dlg.Olddescription = _T("Old:");
			dlg.Newdescription = _T("New:");
			dlg.Confirmdescription = _T("Confirm new:");
			dlg.title = _T("Setup Password");
			dlg.hideold = true;
            if(dlg.DoModal(/*m_hWnd*/) == IDOK){
				if(dlg.Newline == dlg.Confirmline) {
					settings->set(SettingsManager::PASSWORD, Util::base64_encode(reinterpret_cast<const unsigned char*>(Text::fromT(dlg.Newline).c_str()), strlen(Text::fromT(dlg.Newline).c_str())));
				} else {
					::MessageBox(m_hWnd, _T("Passwords did not match!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
					::CheckDlgButton(*this, IDC_PASSWD_PROTECT_CHCKBOX, BST_UNCHECKED);
				}
			} else {
				::CheckDlgButton(*this, IDC_PASSWD_PROTECT_CHCKBOX, BST_UNCHECKED);
			}
		} else {
			// neee
			::CheckDlgButton(*this, IDC_PASSWD_PROTECT_CHCKBOX, BST_CHECKED);
			PassDlg dlg;
			dlg.description = TSTRING(PASSWORD_DESC);
			dlg.title = TSTRING(PASSWORD_TITLE);
			dlg.ok = TSTRING(UNLOCK);
			if(dlg.DoModal(/*m_hWnd*/) == IDOK){
				tstring tmp = dlg.line;
				if (tmp == Text::toT(Util::base64_decode(SETTING(PASSWORD)))) {
					::CheckDlgButton(*this, IDC_PASSWD_PROTECT_CHCKBOX, BST_UNCHECKED);
					settings->set(SettingsManager::PASSWORD, "");
				} else {
					::MessageBox(m_hWnd, _T("Wrong password!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST);
				}
			}
		}
		fixControls();
	}
	return true;
}


LRESULT MiscPage::OnPasswordChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	if((wID == IDC_PASSWD_BUTTON) /*&& !SETTING(PASSWORD).empty()*/) {
		ChngPassDlg dlg;
		dlg.Olddescription = _T("Old:");
		dlg.Newdescription = _T("New:");
		dlg.Confirmdescription = _T("Confirm new:");
		dlg.title = _T("Change password");
			if(dlg.DoModal(/*m_hWnd*/) == IDOK){
				if(dlg.Oldline == Text::toT(Util::base64_decode(SETTING(PASSWORD)))) {
					if(dlg.Newline == dlg.Confirmline) {
						settings->set(SettingsManager::PASSWORD, Util::base64_encode(reinterpret_cast<const unsigned char*>(Text::fromT(dlg.Newline).c_str()), strlen(Text::fromT(dlg.Newline).c_str())));
					} else {
						::MessageBox(m_hWnd, _T("Passwords did not match!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
						::CheckDlgButton(*this, IDC_PASSWD_PROTECT_CHCKBOX, BST_CHECKED);
					}
				} else {
					::MessageBox(m_hWnd, _T("Wrong password!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST);
				}
		}
	}
	return true;
}

void MiscPage::fixControls() {
	bool state = (IsDlgButtonChecked(IDC_PASSWD_PROTECT_CHCKBOX) != 0);
	::EnableWindow(GetDlgItem(IDC_PASSWD_BUTTON), state);
	BOOL use = IsDlgButtonChecked(IDC_PASSWD_PROTECT_CHCKBOX) == BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_PASSWD_PROTECT_TRAY_CHCKBOX), use);
	::EnableWindow(GetDlgItem(IDC_NOTE), use);

	if(IsDlgButtonChecked(IDC_PASSWD_PROTECT_CHCKBOX) == BST_UNCHECKED)
		::CheckDlgButton(*this, IDC_PASSWD_PROTECT_TRAY_CHCKBOX, BST_UNCHECKED);
}



