/*
 * Copyright (C) 2011 AirDC++ Project
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

#if !defined(PRIORITY_PAGE_H)
#define PRIORITY_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

class PriorityPage : public CPropertyPage<IDD_PRIORITYPAGE>, public PropPage
{
public:
	PriorityPage(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_DOWNLOADS) + _T('\\') + TSTRING(PRIORITIES)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	}
	~PriorityPage() { free(title);	}

	BEGIN_MSG_MAP(PriorityPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_ORDER_BALANCED, onDownloadOrder)
		COMMAND_ID_HANDLER(IDC_ORDER_PROGRESS, onDownloadOrder)
		COMMAND_ID_HANDLER(IDC_ORDER_ADDED, onDownloadOrder)
		COMMAND_ID_HANDLER(IDC_ORDER_RANDOM, onDownloadOrder)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onDownloadOrder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	virtual void write();
	
protected:

	static Item items[];
	static TextItem texts[];
	TCHAR* title;
};

#endif // !defined(PRIORITY_PAGE_H)