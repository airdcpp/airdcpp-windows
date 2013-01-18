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

#if !defined(AIRDC_PAGE_H)
#define AIRDC_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"
#include "../client/File.h"

class MiscPage : public CPropertyPage<IDD_MISC_PAGE>, public PropPage
{
public:
	MiscPage(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_MISC)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	}

	~MiscPage() { free(title); }
	

	BEGIN_MSG_MAP_EX(MiscPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_HANDLER(IDC_WINAMP_HELP, BN_CLICKED, onClickedWinampHelp)
		COMMAND_HANDLER(IDC_PLAYER_COMBO, CBN_SELCHANGE, onSelChange)
		COMMAND_HANDLER(IDC_WINAMP_BROWSE, BN_CLICKED, onBrowsew)
		COMMAND_ID_HANDLER(IDC_PASSWD_PROTECT_CHCKBOX, onChangeCont)
		COMMAND_ID_HANDLER(IDC_PASSWD_BUTTON, OnPasswordChange)
	END_MSG_MAP()


	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClickedWinampHelp(WORD /* wNotifyCode */, WORD wID, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onSelChange(WORD /* wNotifyCode */, WORD wID, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onBrowsew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onChangeCont(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnPasswordChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
private:
	void fixControls();
	static Item items[];
	static TextItem texts[];

	CComboBox ctrlPlayer;
	CEdit ctrlFormat;

	tstring WinampStr;
	tstring iTunesStr;
	tstring MPCStr;
	tstring WMPlayerStr;
	tstring SpotifyStr;
	int CurSel;

	TCHAR* title;

};

#endif // !defined(AIRDC_PAGE_H)
