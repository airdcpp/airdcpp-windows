/* 
 * Copyright (C) 2011-2013 AirDC++ Project
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
#include "Resource.h"

#include "HashingPage.h"

#include "WinUtil.h"
#include "PropertiesDlg.h"
#include "MainFrm.h"

PropPage::TextItem HashingPage::texts[] = {
	//hashing
	{ IDC_HASHING_OPTIONS, ResourceManager::HASHING_OPTIONS },
	{ IDC_SETTINGS_MAX_HASH_SPEED, ResourceManager::SETTINGS_MAX_HASHER_SPEED },
	{ IDC_SETTINGS_MBS, ResourceManager::MBPS },
	{ IDC_HASHING_THREADS_LBL, ResourceManager::MAX_HASHING_THREADS },
	{ IDC_MAX_VOL_HASHERS_LBL, ResourceManager::MAX_VOL_HASHERS },
	{ IDC_REPAIR_HASHDB, ResourceManager::SETTINGS_DB_REPAIR },
	{ IDC_FILE_INDEX_SIZE_LBL, ResourceManager::FILE_INDEX_SIZE },
	{ IDC_HASH_DATA_SIZE_LBL, ResourceManager::HASH_DATA_SIZE },
	{ IDC_MAINTENANCE, ResourceManager::MAINTENANCE },
	{ IDC_OPTIMIZE_DB_FAST, ResourceManager::OPTIMIZE_DB },
	{ IDC_OPTIMIZE_DB_LBL, ResourceManager::OPTIMIZE_DB_HELP },
	{ IDC_VERIFY_DB, ResourceManager::OPTIMIZE_VERIFY_DB },
	{ IDC_VERIFY_DB_LBL, ResourceManager::OPTIMIZE_VERIFY_DB_HELP },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item HashingPage::items[] = {
	//hashing
	{ IDC_MAX_HASH_SPEED, SettingsManager::MAX_HASH_SPEED, PropPage::T_INT },
	{ IDC_HASHING_THREADS, SettingsManager::MAX_HASHING_THREADS, PropPage::T_INT },
	{ IDC_MAX_VOL_HASHERS, SettingsManager::HASHERS_PER_VOLUME, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};



LRESULT HashingPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// Do specialized reading here
	PropPage::read((HWND)*this, items);
	PropPage::translate((HWND)(*this), texts);

	//hashing
	setMinMax(IDC_HASH_SPIN, 0, 9999);
	setMinMax(IDC_VOL_HASHERS_SPIN, 1, 30);
	setMinMax(IDC_HASHING_THREADS_SPIN, 1, 50);

	CheckDlgButton(IDC_REPAIR_HASHDB, HashManager::getInstance()->isRepairScheduled());
	updateSizes();
	fixControls();

	return TRUE;
}

void HashingPage::write() {
	PropPage::write((HWND)*this, items);
	
	HashManager::getInstance()->onScheduleRepair(IsDlgButtonChecked(IDC_REPAIR_HASHDB) > 0 ? true : false);
}

void HashingPage::updateSizes() {
	int64_t fileDbSize = 0, hashDbSize = 0;
	HashManager::getInstance()->getDbSizes(fileDbSize, hashDbSize);

	::SetWindowText(GetDlgItem(IDC_FILE_INDEX_SIZE), Util::formatBytesW(fileDbSize).c_str());
	::SetWindowText(GetDlgItem(IDC_HASH_DATA_SIZE), Util::formatBytesW(hashDbSize).c_str());
}
 
LRESULT HashingPage::onOptimizeDb(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	optimizeDb(false);
	return 0;
}

LRESULT HashingPage::onVerifyDb(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	optimizeDb(true);
	return 0;
}

void HashingPage::fixControls() {
	bool enable = !HashManager::getInstance()->maintenanceRunning();
	::EnableWindow(GetDlgItem(IDC_OPTIMIZE_DB_FAST), enable);
	::EnableWindow(GetDlgItem(IDC_VERIFY_DB), enable);
}

void HashingPage::on(HashManagerListener::MaintananceFinished()) noexcept {
	updateSizes();
	fixControls();
}

void HashingPage::on(HashManagerListener::MaintananceStarted()) noexcept {
	fixControls();
}


void HashingPage::optimizeDb(bool verify) {
	if (!WinUtil::showQuestionBox(CTSTRING(OPTIMIZE_CONFIRMATION), MB_ICONEXCLAMATION)) {
		return;
	}

	MainFrame::getMainFrame()->addThreadedTask([=] { HashManager::getInstance()->startMaintenance(verify); });
	WinUtil::showMessageBox(TSTRING(MAINTENANCE_STARTED_SETTINGS), MB_ICONINFORMATION);
}