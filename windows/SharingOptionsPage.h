/* 
 * Copyright (C) 2011-2013 AirDC++ Project
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

#if !defined(SHARINGOPTIONS_PAGE_H)
#define SHARINGOPTIONS_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"


class SharingOptionsPage : public CPropertyPage<IDD_SHARING_OPTIONS>, public PropPage
{
public:
	SharingOptionsPage(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_SHARINGPAGE) + _T('\\') + TSTRING(SETTINGS_SHARING_OPTIONS)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	}

	~SharingOptionsPage() {
		free(title);
	}

	BEGIN_MSG_MAP(SharingOptionsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);


	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
protected:
	CComboBox ctrlThreadedRefresh;

	static Item items[];
	static TextItem texts[];
	static ListItem listItems[];
	TCHAR* title;

};

#endif // !defined(SHARINGOPTIONS_PAGE_H)


