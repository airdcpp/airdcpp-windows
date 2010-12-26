/* 
 * Copyright (C) 2003 Opera, opera@home.se
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

#ifndef _WEBSHORTCUTSPROPERTIES_H
#define _WEBSHORTCUTSPROPERTIES_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/WebShortcuts.h"

class WebShortcutsProperties : public CDialogImpl<WebShortcutsProperties> {
public:

	// Constructor/destructor
	WebShortcutsProperties::WebShortcutsProperties(vector<WebShortcut*>& _wslist, WebShortcut* _ws) : wslist(_wslist), ws(_ws) { };
	virtual ~WebShortcutsProperties() { };

	// Dialog unique id
	enum { IDD = IDD_WEB_SHORTCUTS };
	
	// Inline message map
	BEGIN_MSG_MAP(WebShortcutsProperties)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()
	
	// Message handlers
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	WebShortcut* ws;
	vector<WebShortcut*>& wslist;

	static const tstring badkeys;
};

#endif // _WEBSHORTCUTSPROPERTIES_H
