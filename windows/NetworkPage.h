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

#if !defined(NETWORK_PAGE_H)
#define NETWORK_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "../client/UpdateManagerListener.h"

class ProtocolPage : /*public CPropertyPage<IDD_PROTOCOLPAGE>,*/ public SettingTab, public CDialogImpl<ProtocolPage>, private UpdateManagerListener
{
public:
	ProtocolPage(SettingsManager *s, bool v6);
	~ProtocolPage();
	enum { IDD = IDD_PROTOCOLPAGE };

	BEGIN_MSG_MAP(ProtocolPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_CONNECTION_DETECTION, onClickedActive)
		COMMAND_ID_HANDLER(IDC_ACTIVE, onClickedActive)
		COMMAND_ID_HANDLER(IDC_PASSIVE, onClickedActive)
		COMMAND_ID_HANDLER(IDC_ACTIVE_UPNP, onClickedActive)
		COMMAND_ID_HANDLER(IDC_PROTOCOL_ENABLED, onClickedActive)
		COMMAND_ID_HANDLER(IDC_GETIP, onGetIP)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClickedActive(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGetIP(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */);

	//PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
private:
	static Item items4[];
	static Item items6[];
	static TextItem texts[];
	CComboBox BindCombo;

	void fixControls();
	void getAddresses();

	AirUtil::IpList bindAddresses;
	void on(UpdateManagerListener::SettingUpdated, size_t key, const string& value) noexcept;
	bool v6;
};


/*class COptionsSheet : public CPropertySheetImpl<COptionsSheet>
{
public:
    // Construction
    COptionsSheet (UINT uStartPage, HWND hWndParent, SettingsManager* s);

    // Maps
    BEGIN_MSG_MAP(COptionsSheet)
        MSG_WM_SHOWWINDOW(OnShowWindow)
        CHAIN_MSG_MAP(CPropertySheetImpl<COptionsSheet>)
    END_MSG_MAP()

    // Message handlers
    void OnShowWindow ( BOOL bShowing, int nReason );
	void onInit();

    // Property pages
   // CBackgroundOptsPage         m_pgBackground;
   // CPropertyPage<IDD_ABOUTBOX> m_pgAbout;

    // Implementation
protected:
	unique_ptr<ProtocolPage> ipv6Page;
	unique_ptr<ProtocolPage> ipv4Page;
    bool m_bCentered;
};*/


class NetworkPage : public CPropertyPage<IDD_NETWORKPAGE>, public PropPage
{
public:
	NetworkPage(SettingsManager *s);
	~NetworkPage();

	//typedef CDialogImpl<ProtocolPage> protocolBase;
	BEGIN_MSG_MAP(NetworkPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_IPV4, onClickProtocol)
		COMMAND_ID_HANDLER(IDC_IPV6, onClickProtocol)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClickProtocol(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
private:
	//COptionsSheet protocols;
	static Item items[];
	static TextItem texts[];
	CComboBox MapperCombo;

	unique_ptr<ProtocolPage> ipv6Page;
	unique_ptr<ProtocolPage> ipv4Page;

	CButton ctrlIPv4;
	CButton ctrlIPv6;

	void showProtocol(bool v6);
};

#endif // !defined(NETWORK_PAGE_H)