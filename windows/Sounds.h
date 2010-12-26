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

#ifndef Sounds_H
#define Sounds_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropPage.h"
#include "ExListViewCtrl.h"


class Sounds : public CPropertyPage<IDD_SOUNDS>, public PropPage
{
public:
	Sounds(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_APPEARANCE) + _T('\\') + TSTRING(SETTINGS_SOUNDS)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	};

	~Sounds() {
		ctrlSounds.Detach();
		free(title);
	};

	BEGIN_MSG_MAP(Sounds)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_HANDLER(IDC_BROWSE, BN_CLICKED, onBrowse)
		COMMAND_HANDLER(IDC_PLAY, BN_CLICKED, onPlay)
		COMMAND_ID_HANDLER(IDC_NONE, onClickedNone)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onPlay(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onClickedNone(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();

protected:
	static Item items[];
	static TextItem texts[];
	TCHAR* title;

	struct snds{
		ResourceManager::Strings name;
		int setting;
		string value;
	};

	static snds sounds[];

	ExListViewCtrl ctrlSounds;
};

#endif //Sounds_H

/**
 * @file
 * $Id: Sounds.h 308 2007-07-13 18:57:02Z bigmuscle $
 */

