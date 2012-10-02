/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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

#ifndef Popups_H
#define Popups_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropPage.h"
#include "ExListViewCtrl.h"


class Popups : public CPropertyPage<IDD_POPUPS>, public PropPage
{
public:
	Popups(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_APPEARANCE) + _T('\\') + TSTRING(BALLOON_POPUPS)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	};

	~Popups() {
		ctrlPopupType.Detach();
		free(title);
	};

	enum { BALLOON, CUSTOM, SPLASH, WINDOW };

	BEGIN_MSG_MAP(Sounds)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_HANDLER(IDC_PREVIEW, BN_CLICKED, onPreview)
		COMMAND_ID_HANDLER(IDC_POPUP_FONT, onFont)
		COMMAND_ID_HANDLER(IDC_POPUP_TITLE_FONT, onTitleFont)
		COMMAND_ID_HANDLER(IDC_POPUP_BACKCOLOR, onBackColor)	
		COMMAND_ID_HANDLER(IDC_POPUP_TYPE, onTypeChanged)
		COMMAND_HANDLER(IDC_POPUPBROWSE, BN_CLICKED, onPopupBrowse)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onPreview(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onCtlColor(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onBackColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onFont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onTitleFont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onTypeChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onPopupBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();

protected:
	static ListItem listItems[];
	static Item items[];
	static TextItem texts[];
	TCHAR* title;

	ExListViewCtrl ctrlPopups;
	CComboBox ctrlPopupType;
	LOGFONT myFont;
};

#endif //Popups_H
