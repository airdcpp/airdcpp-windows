/*
 * Copyright (C) 2012-2021 AirDC++ Project
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

#ifndef DCPLUSPLUS_PROTOCOL_PAGE
#define DCPLUSPLUS_PROTOCOL_PAGE

#include <atlcrack.h>

#include "Async.h"
#include "PropPage.h"

#include <airdcpp/ConnectivityManager.h>
#include <airdcpp/UpdateManagerListener.h>

class ProtocolPage : public SettingTab, public CDialogImpl<ProtocolPage>, private UpdateManagerListener, private ConnectivityManagerListener, private Async<ProtocolPage>
{
public:
	ProtocolPage(SettingsManager *s, bool v6);
	~ProtocolPage();
	enum { IDD = IDD_PROTOCOLPAGE };

	BEGIN_MSG_MAP(ProtocolPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker) 
		COMMAND_ID_HANDLER(IDC_CONNECTION_DETECTION, onClickedAutoDetect)
		COMMAND_ID_HANDLER(IDC_ACTIVE, onClickedActive)
		COMMAND_ID_HANDLER(IDC_PASSIVE, onClickedActive)
		COMMAND_ID_HANDLER(IDC_ACTIVE_UPNP, onClickedActive)
		COMMAND_ID_HANDLER(IDC_PROTOCOL_ENABLED, onClickedActive)
		COMMAND_ID_HANDLER(IDC_GETIP, onGetIP)

	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClickedActive(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedAutoDetect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGetIP(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */);


	void write();
private:
	static Item items4[];
	static Item items6[];
	static TextItem texts[];
	CComboBox BindCombo;
	CButton cAutoDetect;

	void fixControls();
	void getAddresses();



	AdapterInfoList bindAdapters;
	bool v6;

	// UpdateManagerListener
	void on(UpdateManagerListener::SettingUpdated, size_t key, const string& value) noexcept;

	// ConnectivityManagerListener
	void on(SettingChanged) noexcept;

};

class ProtocolBase : public CDialogImpl<ProtocolBase>, public SettingTab
{
public:
	ProtocolBase(SettingsManager *s);
	~ProtocolBase();
	enum { IDD = IDD_PROTOCOLBASE };

	enum Page{
		IPV4 = 0,
		IPV6 = 1
	};

	BEGIN_MSG_MAP(ProtocolBase)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		NOTIFY_HANDLER(IDC_TAB1, TCN_SELCHANGE, onProtocolChanged)
		COMMAND_CODE_HANDLER(EN_CHANGE, onPortsUpdated)
		//COMMAND_HANDLER(IDC_PORT_TLS, EN_CHANGE, onPortsUpdated)
		//COMMAND_HANDLER(IDC_PORT_TCP, EN_CHANGE, onPortsUpdated)

	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onProtocolChanged(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/);
	LRESULT onPortsUpdated(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// Common PropPage interface
	void write();
	bool validatePorts();

private:
	static Item items[];
	static TextItem texts[];
	//CComboBox MapperCombo;

	unique_ptr<ProtocolPage> ipv6Page;
	unique_ptr<ProtocolPage> ipv4Page;
	CTabCtrl tabs;

	void showProtocol(bool v6);
	bool created = false;
};

#endif // !defined(NETWORK_PAGE_H)