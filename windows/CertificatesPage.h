/*
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(CERTIFICATES_PAGE_H)
#define CERTIFICATES_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

class CertificatesPage : public CPropertyPage<IDD_CERTIFICATESPAGE>, public PropPage
{
public:
	CertificatesPage(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_CERTIFICATES)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	}

	~CertificatesPage() { free(title); }

	BEGIN_MSG_MAP(CertificatesPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_BROWSE_PRIVATE_KEY, onBrowsePrivateKey)
		COMMAND_ID_HANDLER(IDC_BROWSE_CERTIFICATE, onBrowseCertificate)
		COMMAND_ID_HANDLER(IDC_BROWSE_TRUSTED_PATH, onBrowseTrustedPath)
		COMMAND_ID_HANDLER(IDC_GENERATE_CERTS, onGenerateCerts)
		COMMAND_HANDLER(IDC_TRANSFER_ENCRYPTION, CBN_SELENDOK, onModeChanged)
	END_MSG_MAP()

	LRESULT onBrowsePrivateKey(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBrowseCertificate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBrowseTrustedPath(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGenerateCerts(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onModeChanged(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
protected:
	CComboBox ctrlTransferEncryption;
	static Item items[];
	static TextItem texts[];
	TCHAR* title;
	void fixControls();
};

#endif // !defined(CERTIFICATES_PAGE_H)

/**
 * @file
 * $Id: CertificatesPage.h 308 2007-07-13 18:57:02Z bigmuscle $
 */
