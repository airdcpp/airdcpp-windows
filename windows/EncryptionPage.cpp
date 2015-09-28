/*
 * Copyright (C) 2001-2015 Jacek Sieka, arnetheduck on gmail point com
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

#include <airdcpp/CryptoManager.h>

#include "Resource.h"
#include "EncryptionPage.h"
#include "WinUtil.h"
#include "BrowseDlg.h"

PropPage::TextItem EncryptionPage::texts[] = {
	{ IDC_CERT_USE_DEFAULT_PATHS, ResourceManager::USE_DEFAULT_CERT_PATHS },
	{ IDC_PRIV_KEY_LBL, ResourceManager::PRIVATE_KEY_FILE },
	{ IDC_OWN_CERT_LBL, ResourceManager::OWN_CERTIFICATE_FILE },
	{ IDC_OWN_CERT, ResourceManager::OWN_CERTIFICATE },
	{ IDC_GENERATE_CERTS, ResourceManager::GENERATE_CERTIFICATES },

	{ IDC_TRUSTED_PATH_LBL, ResourceManager::TRUSTED_CERTIFICATES_PATH },
	{ IDC_TRANSFER_ENCRYPTION_LBL, ResourceManager::TRANSFER_ENCRYPTION },
	{ IDC_TRUSTED_CERT_NOTE, ResourceManager::TRUSTED_CERT_NOTE },
	{ IDC_TRUSTED_CERTS, ResourceManager::TRUSTED_CERTIFICATES },
	{ IDC_GENERATE_CERTS_NOTE, ResourceManager::GENERATE_CERTS_NOTE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item EncryptionPage::items[] = {
	{ IDC_CERT_USE_DEFAULT_PATHS, SettingsManager::USE_DEFAULT_CERT_PATHS, PropPage::T_BOOL },
	{ IDC_TLS_CERTIFICATE_FILE, SettingsManager::TLS_CERTIFICATE_FILE, PropPage::T_STR },
	{ IDC_TLS_PRIVATE_KEY_FILE, SettingsManager::TLS_PRIVATE_KEY_FILE, PropPage::T_STR },

	{ IDC_TLS_TRUSTED_CERTIFICATES_PATH, SettingsManager::TLS_TRUSTED_CERTIFICATES_PATH, PropPage::T_STR },
	{ 0, 0, PropPage::T_END }
};

EncryptionPage::ListItem EncryptionPage::listItems[] = {
	{ SettingsManager::ALLOW_UNTRUSTED_HUBS, ResourceManager::SETTINGS_ALLOW_UNTRUSTED_HUBS },
	{ SettingsManager::ALLOW_UNTRUSTED_CLIENTS, ResourceManager::SETTINGS_ALLOW_UNTRUSTED_CLIENTS },
	{ SettingsManager::ALWAYS_CCPM, ResourceManager::ALWAYS_CCPM },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};


LRESULT EncryptionPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);

	// Do specialized reading here
	PropPage::read((HWND)*this, items);
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_ENCRYPTION_LIST));

	ctrlTransferEncryption.Attach(GetDlgItem(IDC_TRANSFER_ENCRYPTION));
	ctrlTransferEncryption.AddString(CTSTRING(DISABLED));
	ctrlTransferEncryption.AddString(CTSTRING(ENABLED));
	ctrlTransferEncryption.AddString(CTSTRING(ENCRYPTION_FORCED));
	ctrlTransferEncryption.SetCurSel(SETTING(TLS_MODE));

	fixControls();
	return TRUE;
}

void EncryptionPage::write() {
	PropPage::write((HWND)*this, items);
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_ENCRYPTION_LIST));

	SettingsManager::getInstance()->set(SettingsManager::TLS_MODE, ctrlTransferEncryption.GetCurSel());
	CryptoManager::setCertPaths();
	AirUtil::updateCachedSettings();
}

LRESULT EncryptionPage::onBrowsePrivateKey(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring target = Text::toT(SETTING(TLS_PRIVATE_KEY_FILE));
	CEdit edt(GetDlgItem(IDC_TLS_PRIVATE_KEY_FILE));

	BrowseDlg dlg(m_hWnd, BrowseDlg::TYPE_NOSAVE, BrowseDlg::DIALOG_SELECT_FILE);
	dlg.setPath(target, true);
	if (dlg.show(target)) {
		edt.SetWindowText(&target[0]);
	}
	return 0;
}

LRESULT EncryptionPage::onUseDefaults(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

void EncryptionPage::fixControls() {
	BOOL manualPaths = IsDlgButtonChecked(IDC_CERT_USE_DEFAULT_PATHS) != BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_PRIV_KEY_LBL),			manualPaths);
	::EnableWindow(GetDlgItem(IDC_OWN_CERT_LBL),			manualPaths);
	::EnableWindow(GetDlgItem(IDC_TLS_CERTIFICATE_FILE),	manualPaths);
	::EnableWindow(GetDlgItem(IDC_TLS_PRIVATE_KEY_FILE),	manualPaths);
	::EnableWindow(GetDlgItem(IDC_BROWSE_PRIVATE_KEY),		manualPaths);
	::EnableWindow(GetDlgItem(IDC_BROWSE_CERTIFICATE),		manualPaths);
}

LRESULT EncryptionPage::onBrowseCertificate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring target = Text::toT(SETTING(TLS_CERTIFICATE_FILE));
	CEdit edt(GetDlgItem(IDC_TLS_CERTIFICATE_FILE));

	BrowseDlg dlg(m_hWnd, BrowseDlg::TYPE_NOSAVE, BrowseDlg::DIALOG_SELECT_FILE);
	dlg.setPath(target, true);
	if (dlg.show(target)) {
		edt.SetWindowText(&target[0]);
	}

	return 0;
}

LRESULT EncryptionPage::onBrowseTrustedPath(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring target = Text::toT(SETTING(TLS_TRUSTED_CERTIFICATES_PATH));
	CEdit edt(GetDlgItem(IDC_TLS_TRUSTED_CERTIFICATES_PATH));

	BrowseDlg dlg(m_hWnd, BrowseDlg::TYPE_NOSAVE, BrowseDlg::DIALOG_SELECT_FOLDER);
	dlg.setPath(target, true);
	if (dlg.show(target)) {
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