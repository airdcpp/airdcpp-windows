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

#ifndef HASHINGPAGE_H
#define HASHINGPAGE_H

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

#include "../client/HashManager.h"

class HashingPage : public CPropertyPage<IDD_HASHINGPAGE>, public PropPage
{
public:
	HashingPage(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_SHARINGPAGE) + _T('\\') + TSTRING(HASHING)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	}

	~HashingPage() {
		free(title);
	}

	BEGIN_MSG_MAP(HashingPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_OPTIMIZE_DB_FAST, onOptimizeDb)
		COMMAND_ID_HANDLER(IDC_VERIFY_DB, onVerifyDb)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onOptimizeDb(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onVerifyDb(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
protected:
	void updateSizes();

	static Item items[];
	static TextItem texts[];
	TCHAR* title;

	void fixControls();
	void optimizeDb(bool verify);

	void on(HashManagerListener::MaintananceStarted()) noexcept;
	void on(HashManagerListener::MaintananceFinished()) noexcept;
};

#endif // !defined(HASHINGPAGE_H)


