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
#include "ProtocolPage.h"

class NetworkPage : public CPropertyPage<IDD_NETWORKPAGE>, public PropPage
{
public:
	NetworkPage(SettingsManager *s);
	~NetworkPage();

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