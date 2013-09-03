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

#include "stdafx.h"
#include "../client/ResourceManager.h"
#include "../client/WebShortcuts.h"
#include "Resource.h"

#include "WebShortcutsProperties.h"

// Initialize dialog
LRESULT WebShortcutsProperties::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {

	SetWindowText(CTSTRING(WEBSHORTCUTS));
	SetDlgItemText(IDC_WEB_SHORTCUTS_NAME_DESC, CTSTRING(SETTINGS_NAME));
	SetDlgItemText(IDC_WEB_SHORTCUTS_HOWTO, CTSTRING(SETTINGS_WS_HOWTO));
	SetDlgItemText(IDC_WEB_SHORTCUTS_DESC, CTSTRING(SETTINGS_WS_DESCR));
	SetDlgItemText(IDC_WEB_SHORTCUTS_CLEAN, CTSTRING(SETTINGS_WS_CLEAN));

	SetDlgItemText(IDC_WEB_SHORTCUT_NAME,	Text::toT(ws->name).c_str());
	SetDlgItemText(IDC_WEB_SHORTCUT_KEY,	Text::toT(ws->key).c_str());
	SetDlgItemText(IDC_WEB_SHORTCUT_URL,	Text::toT(ws->url).c_str());
	CheckDlgButton(IDC_WEB_SHORTCUTS_CLEAN, ws->clean ? BST_CHECKED : BST_UNCHECKED);

	::SetFocus(GetDlgItem(IDC_WEB_SHORTCUT_NAME));

	return FALSE;
}

// Exit dialog
LRESULT WebShortcutsProperties::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(wID == IDOK) {
		// Update search
		TCHAR buf[2048];

		GetDlgItemText(IDC_WEB_SHORTCUT_NAME, buf, 2048);
		tstring sName = buf;
		if (sName.empty()) {
			MessageBox(CTSTRING(NAME_REQUIRED), sName.c_str(), MB_OK | MB_ICONEXCLAMATION);
			return 0;
		}

		auto _ws = WebShortcuts::getShortcutByName(wslist, Text::fromT(sName));
		if ( _ws != NULL && _ws != ws ) {
			MessageBox(CTSTRING(NAME_IN_USE), sName.c_str(), MB_OK | MB_ICONEXCLAMATION);
			return 0;
		}

		GetDlgItemText(IDC_WEB_SHORTCUT_KEY, buf, 2048);
		tstring sKey = buf;
		// Check if key is busy
		if (!sKey.empty()) {
			_ws = WebShortcuts::getShortcutByKey(wslist, Text::fromT(sKey));
			if ( _ws != NULL && _ws != ws ) {
				MessageBox(CTSTRING(KEY_IN_USE), (sName + _T(" (") + sKey + _T(")")).c_str(), MB_OK | MB_ICONEXCLAMATION);
				return 0;
			}
		}

		GetDlgItemText(IDC_WEB_SHORTCUT_URL, buf, 2048);
		tstring sUrl = buf;

		bool bClean = (IsDlgButtonChecked(IDC_WEB_SHORTCUTS_CLEAN) == BST_CHECKED);

		ws->name = Text::fromT(sName);
		ws->key = Text::fromT(sKey);
		ws->url = Text::fromT(sUrl);
		ws->clean = bClean;
	}

	EndDialog(wID);
	return 0;
}
