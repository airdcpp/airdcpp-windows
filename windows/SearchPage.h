/* 
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

#ifndef AUTOSEARCH_PAGE_H
#define AUTOSEARCH_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/WebShortcuts.h"

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

class AutosearchPage : public CPropertyPage<IDD_AUTOSEARCH>, public PropPage
{
public:
	AutosearchPage(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_AIRSEARCH)).c_str());

		SetTitle(title);
	};

	~AutosearchPage() {
		for (WebShortcut::Iter i = wsList.begin(); i != wsList.end(); ++i)
			delete *i;
		
		ctrlWebShortcuts.Detach();
		free(title);
	};

	BEGIN_MSG_MAP_EX(AutosearchPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_HANDLER(IDC_WEB_SHORTCUTS_ADD, BN_CLICKED, onClickedShortcuts)
		COMMAND_HANDLER(IDC_WEB_SHORTCUTS_PROPERTIES, BN_CLICKED, onClickedShortcuts)
		COMMAND_HANDLER(IDC_WEB_SHORTCUTS_REMOVE, BN_CLICKED, onClickedShortcuts)
		COMMAND_HANDLER(IDC_WEB_SHORTCUTS_LIST, LVN_ITEMCHANGED, onSelChangeShortcuts)
		COMMAND_HANDLER(IDC_WEB_SHORTCUTS_LIST, LVN_ITEMCHANGING, onSelChangeShortcuts)
		COMMAND_HANDLER(IDC_WEB_SHORTCUTS_LIST, LVN_ITEMACTIVATE, onSelChangeShortcuts)
		NOTIFY_HANDLER(IDC_WEB_SHORTCUTS_LIST, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_WEB_SHORTCUTS_LIST, NM_DBLCLK, onDoubleClick)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onClickedShortcuts(WORD /* wNotifyCode */, WORD wID, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onSelChangeShortcuts(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();

	void updateListItem(int pos) {
		dcassert(pos >= 0 && (unsigned int)pos < wsList.size());
		ctrlWebShortcuts.SetItemText(pos, 0, wsList[pos]->name.c_str());
		ctrlWebShortcuts.SetItemText(pos, 1, wsList[pos]->key.c_str());
		ctrlWebShortcuts.SetItemText(pos, 2, wsList[pos]->url.c_str());
	}

	void addListItem(WebShortcut* ws) {
		TStringList cols;
		cols.push_back(ws->name);
		cols.push_back(ws->key);
		cols.push_back(ws->url);
		cols.push_back(ws->clean? _T("Yes") : _T("No"));
		ctrlWebShortcuts.insert(cols);
		cols.clear();
	}
	
protected:

	static Item items[];
	static TextItem texts[];
	TCHAR* title;

	ExListViewCtrl ctrlWebShortcuts;
	WebShortcut::List wsList;

};

#endif //AUTOSEARCH_PAGE_H