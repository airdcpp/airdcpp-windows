/* 
 * Copyright (C) 2011-2018 AirDC++ Project
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

#ifndef SEARCH_PAGE_H
#define SEARCH_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

class SearchPage : public CPropertyPage<IDD_SEARCHPAGE>, public PropPage
{
public:
	SearchPage(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_AIRSEARCH)).c_str());

		SetTitle(title);
	};

	~SearchPage();

	BEGIN_MSG_MAP_EX(SearchPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();

	
protected:

	static Item items[];
	static TextItem texts[];
	TCHAR* title;

	CComboBox failedGroup;

};

#endif //AUTOSEARCH_PAGE_H