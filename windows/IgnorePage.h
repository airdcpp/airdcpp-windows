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

#if !defined(IGNORE_PAGE_H)
#define IGNORE_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include <atlctrlx.h>

#include "ExListViewCtrl.h"

class IgnorePage : public CPropertyPage<IDD_IGNOREPAGE>, public PropPage
{
public:
	IgnorePage(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_IGNORE)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	}
	~IgnorePage() { 
		free(title);
		ignoreListCtrl.Detach();
	}

	BEGIN_MSG_MAP_EX(IgnorePage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_IGNORE_ADD, onIgnoreAdd)
		COMMAND_ID_HANDLER(IDC_IGNORE_REMOVE, onIgnoreRemove)
		COMMAND_ID_HANDLER(IDC_IGNORE_CLEAR, onIgnoreClear)
		COMMAND_CODE_HANDLER(EN_CHANGE, onEditChange)
		NOTIFY_HANDLER(IDC_IGNORELIST, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_IGNORELIST, LVN_ITEMCHANGED, onItemchangedDirectories)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onIgnoreAdd(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onIgnoreRemove(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onIgnoreClear(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onEditChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemchangedDirectories(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
		NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
		switch(kd->wVKey) {
		case VK_DELETE:
			PostMessage(WM_COMMAND, IDC_IGNORE_REMOVE, 0);
			break;
		default:
			bHandled = FALSE;
		}
		return 0;
	}

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
private:
	typedef unordered_set<tstring> TStringHash;
	typedef TStringHash::iterator TStringHashIter;

	static Item items[];
	static TextItem texts[];
	TCHAR* title;
	TStringHash ignoreList;
	ExListViewCtrl ignoreListCtrl;
};

#endif // !defined(IGNORE_PAGE_H)
