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

#include "stdafx.h"

#include "../client/CryptoManager.h"

#include "Resource.h"
#include "EncryptionPage.h"
#include "WinUtil.h"

PropPage::TextItem EncryptionPage::texts[] = {
	{ IDC_STATIC1, ResourceManager::PRIVATE_KEY_FILE },
	{ IDC_STATIC2, ResourceManager::OWN_CERTIFICATE_FILE },
	{ IDC_STATIC3, ResourceManager::TRUSTED_CERTIFICATES_PATH },
	{ IDC_GENERATE_CERTS, ResourceManager::GENERATE_CERTIFICATES },
	{ IDC_ALLOW_UNTRUSTED_HUBS, ResourceManager::SETTINGS_ALLOW_UNTRUSTED_HUBS },
	{ IDC_ALLOW_UNTRUSTED_CLIENTS, ResourceManager::SETTINGS_ALLOW_UNTRUSTED_CLIENTS },

	{ IDC_TRANSFER_ENCRYPTION_LBL, ResourceManager::TRANSFER_ENCRYPTION },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item EncryptionPage::items[] = {
	{ IDC_TLS_CERTIFICATE_FILE, SettingsManager::TLS_CERTIFICATE_FILE, PropPage::T_STR },
	{ IDC_TLS_PRIVATE_KEY_FILE, SettingsManager::TLS_PRIVATE_KEY_FILE, PropPage::T_STR },
	{ IDC_TLS_TRUSTED_CERTIFICATES_PATH, SettingsManager::TLS_TRUSTED_CERTIFICATES_PATH, PropPage::T_STR },
	{ IDC_ALLOW_UNTRUSTED_HUBS, SettingsManager::ALLOW_UNTRUSTED_HUBS, PropPage::T_BOOL },
	{ IDC_ALLOW_UNTRUSTED_CLIENTS, SettingsManager::ALLOW_UNTRUSTED_CLIENTS, PropPage::T_BOOL },


	{ 0, 0, PropPage::T_END }
};

LRESULT EncryptionPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);

	// Do specialized reading here
	PropPage::read((HWND)*this, items);

	ctrlTransferEncryption.Attach(GetDlgItem(IDC_TRANSFER_ENCRYPTION));
	ctrlTransferEncryption.AddString(CTSTRING(DISABLED));
	ctrlTransferEncryption.AddString(CTSTRING(ENABLED));
	ctrlTransferEncryption.AddString(CTSTRING(ENCRYPTION_FORCED));
	ctrlTransferEncryption.SetCurSel(SETTING(TLS_MODE));
	return TRUE;
}

void EncryptionPage::write() {
	PropPage::write((HWND)*this, items);

	SettingsManager::getInstance()->set(SettingsManager::TLS_MODE, ctrlTransferEncryption.GetCurSel());
}

LRESULT EncryptionPage::onBrowsePrivateKey(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring target = Text::toT(SETTING(TLS_PRIVATE_KEY_FILE));
	CEdit edt(GetDlgItem(IDC_TLS_PRIVATE_KEY_FILE));

	if(WinUtil::browseFile(target, m_hWnd, false, target)) {
		edt.SetWindowText(&target[0]);
	}
	return 0;
}

LRESULT EncryptionPage::onBrowseCertificate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring target = Text::toT(SETTING(TLS_CERTIFICATE_FILE));
	CEdit edt(GetDlgItem(IDC_TLS_CERTIFICATE_FILE));

	if(WinUtil::browseFile(target, m_hWnd, false, target)) {
		edt.SetWindowText(&target[0]);
	}
	return 0;
}

LRESULT EncryptionPage::onBrowseTrustedPath(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring target = Text::toT(SETTING(TLS_TRUSTED_CERTIFICATES_PATH));
	CEdit edt(GetDlgItem(IDC_TLS_TRUSTED_CERTIFICATES_PATH));

	if(WinUtil::browseDirectory(target, m_hWnd)) {
		edt.SetWindowText(&target[0]);
	}
	return 0;
}

LRESULT EncryptionPage::onGenerateCerts(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	try {
		CryptoManager::getInstance()->generateCertificate();
	} catch(const CryptoException& e) {
		MessageBox(Text::toT(e.getError()).c_str(), L"Error generating certificate");
	}
	return 0;
}