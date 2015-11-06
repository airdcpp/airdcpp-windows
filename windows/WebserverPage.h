/*
* Copyright (C) 2011-2015 AirDC++ Project
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

#ifndef WEBSERVER_PAGE_H
#define WEBSERVER_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "Resource.h"

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

#include <web-server/WebServerManager.h>

class WebServerPage : public CPropertyPage<IDD_WEB_SERVER_PAGE>, public PropPage
{
public:
	WebServerPage(SettingsManager *s) : PropPage(s), webMgr(webserver::WebServerManager::getInstance()) {
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + _T("Web server")).c_str());

		SetTitle(title);
	};

	~WebServerPage();

	BEGIN_MSG_MAP_EX(WebServerPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_WEBSERVER_ADD_USER, onButton)
		COMMAND_ID_HANDLER(IDC_WEBSERVER_CHANGE, onButton)
		COMMAND_ID_HANDLER(IDC_WEBSERVER_REMOVE_USER, onButton)
		COMMAND_ID_HANDLER(IDC_WEBSERVER_START, onButton)
		NOTIFY_HANDLER(IDC_WEBSERVER_USERS, LVN_ITEMCHANGED, onSelChange)
		NOTIFY_HANDLER(IDC_WEBSERVER_USERS, NM_DBLCLK, onDoubleClick)
		NOTIFY_HANDLER(IDC_WEBSERVER_USERS, LVN_KEYDOWN, onKeyDown)
		END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSelChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();

	void updateListItem(int pos);
	void addListItem(const string& aUserName, const string& aPassword);

protected:

	static Item items[];
	static TextItem texts[];
	TCHAR* title;

	CButton ctrlRemove;
	CButton ctrlAdd;
	CButton ctrlChange;
	CButton ctrlStart;

	CStatic ctrlStatus;
	CEdit ctrlPort;
	CEdit ctrlTlsPort;
	ExListViewCtrl ctrlWebUsers;

	void updateStatus(const string& aError = Util::emptyString);

	webserver::WebServerManager* webMgr;
	vector<webserver::WebUserPtr> webUserList;

};

#endif