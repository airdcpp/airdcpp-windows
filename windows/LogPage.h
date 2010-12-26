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

#if !defined(LOG_PAGE_H)
#define LOG_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

class LogPage : public CPropertyPage<IDD_LOGPAGE>, public PropPage
{
public:
	LogPage(SettingsManager *s) : PropPage(s), oldSelection(-1) {
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_LOGS)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	}

	~LogPage() { free(title); }

	BEGIN_MSG_MAP_EX(LogPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_BROWSE_LOG, onClickedBrowseDir)
		NOTIFY_HANDLER(IDC_LOG_OPTIONS, LVN_ITEMCHANGED, onItemChanged)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClickedBrowseDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();

protected:
	static Item items[];
	static TextItem texts[];
	static ListItem listItems[];
	TCHAR* title;

	ExListViewCtrl logOptions;
	
	int oldSelection;
	
	//store all log options here so we can discard them
	//if the user cancels the dialog.
	//.first is filename and .second is format
	TStringPairList options;

	void getValues();
};

#endif // !defined(LOG_PAGE_H)

/**
 * @file
 * $Id: LogPage.h 308 2007-07-13 18:57:02Z bigmuscle $
 */
