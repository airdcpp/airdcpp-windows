/*
 * Copyright (C) 2001-2021 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(GENERAL_PAGE_H)
#define GENERAL_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "ActionUtil.h"

class GeneralPage : public CPropertyPage<IDD_GENERALPAGE>, public PropPage
{
public:
	GeneralPage(SettingsManager *s) : PropPage(s), curProfile(-1) {
		SetTitle(CTSTRING(SETTINGS_GENERAL));
		m_psp.dwFlags |= PSP_RTLREADING;
	}
	~GeneralPage() { }

	BEGIN_MSG_MAP_EX(GeneralPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_HANDLER(IDC_NICK, EN_CHANGE, ActionUtil::onUserFieldChar)
		COMMAND_HANDLER(IDC_EMAIL, EN_CHANGE, ActionUtil::onUserFieldChar)
		COMMAND_HANDLER(IDC_USERDESC, EN_CHANGE, ActionUtil::onUserFieldChar)
		COMMAND_ID_HANDLER(IDC_NORMAL, onSelProfile)
		COMMAND_ID_HANDLER(IDC_RAR, onSelProfile)
		COMMAND_ID_HANDLER(IDC_LAN, onSelProfile)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onGetIP(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onClickedRadioButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSelProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	virtual void write();
	virtual Dispatcher::F getThreadedTask();
private:
	void updateCurProfile();

	static Item items[];
	static TextItem texts[];
	CComboBoxEx ctrlLanguage;

	int curProfile;
	ProfileSettingItem::List conflicts;
};

#endif // !defined(GENERAL_PAGE_H)
