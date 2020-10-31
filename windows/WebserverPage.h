/*
* Copyright (C) 2011-2021 AirDC++ Project
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

#pragma once

#include "stdafx.h"
#include "Async.h"
#include "Resource.h"

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

#include <web-server/WebServerManager.h>

class WebServerPage : public CPropertyPage<IDD_WEB_SERVER_PAGE>, public PropPage, private webserver::WebServerManagerListener, private Async<WebServerPage>
{
public:
	WebServerPage(SettingsManager *s);
	~WebServerPage();

	BEGIN_MSG_MAP_EX(WebServerPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		COMMAND_ID_HANDLER(IDC_WEBSERVER_ADD_USER, onAddUser)
		COMMAND_ID_HANDLER(IDC_WEBSERVER_CHANGE, onChangeUser)
		COMMAND_ID_HANDLER(IDC_WEBSERVER_REMOVE_USER, onRemoveUser)
		COMMAND_ID_HANDLER(IDC_WEBSERVER_START, onServerState)
		NOTIFY_HANDLER(IDC_WEBSERVER_USERS, LVN_ITEMCHANGED, onSelChange)
		NOTIFY_HANDLER(IDC_WEBSERVER_USERS, NM_DBLCLK, onDoubleClick)
		NOTIFY_HANDLER(IDC_WEBSERVER_USERS, LVN_KEYDOWN, onKeyDown)
		END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onServerState(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onChangeUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAddUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSelChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
protected:

	enum ServerState {
		STATE_STARTED,
		STATE_STOPPING,
		STATE_STOPPED
	};

	static Item items[];
	static TextItem texts[];

	tstring lastError;

	CButton ctrlRemove;
	CButton ctrlAdd;
	CButton ctrlChange;
	CButton ctrlStart;
	CComboBox ctrlBindHttp;
	CComboBox ctrlBindHttps;

	CStatic ctrlStatus;
	CEdit ctrlPort;
	CEdit ctrlTlsPort;
	ExListViewCtrl ctrlWebUsers;

	CHyperLink url;

	AdapterInfoList bindAdapters;
	void initBindAddresses() noexcept;

	void applySettings() noexcept;

	void setLastError(const string& aError) noexcept;
	void updateState(ServerState aNewState) noexcept;
	ServerState currentState;

	webserver::WebServerManager* webMgr;
	webserver::WebUserList webUserList;

	void updateListItem(int pos);
	void addListItem(const webserver::WebUserPtr& aUser) noexcept;
	webserver::WebUserList::iterator getListUser(int aPos) noexcept;

	void on(webserver::WebServerManagerListener::Started) noexcept;
	void on(webserver::WebServerManagerListener::Stopped) noexcept;
	void on(webserver::WebServerManagerListener::Stopping) noexcept;
};

#endif