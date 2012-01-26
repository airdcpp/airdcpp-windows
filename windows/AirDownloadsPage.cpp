
#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"
#include "Resource.h"

#include "AirDownloadsPage.h"
#include "LineDlg.h"
#include "CommandDlg.h"

#include "WinUtil.h"
#include "PropertiesDlg.h"

PropPage::TextItem AirDownloadsPage::texts[] = {
	{ IDC_SETTINGS_KBPS5, ResourceManager::KBPS },
	{ IDC_SETTINGS_KBPS6, ResourceManager::KBPS },
	{ IDC_SETTINGS_KBPS7, ResourceManager::KBPS },
	{ IDC_SETTINGS_MINUTES, ResourceManager::SECONDS },
	{ IDC_MONITOR_SLOW_SPEED, ResourceManager::SETTINGS_AUTO_DROP_SLOW_SOURCES },
	{ IDC_CZDC_SLOW_DISCONNECT, ResourceManager::SETCZDC_SLOW_DISCONNECT },
	{ IDC_CZDC_I_DOWN_SPEED, ResourceManager::SETCZDC_I_DOWN_SPEED },
	{ IDC_CZDC_TIME_DOWN, ResourceManager::SETCZDC_TIME_DOWN },
	{ IDC_CZDC_H_DOWN_SPEED, ResourceManager::SETCZDC_H_DOWN_SPEED },
	{ IDC_DISCONNECTING_ENABLE, ResourceManager::SETCZDC_DISCONNECTING_ENABLE },
	{ IDC_CZDC_MIN_FILE_SIZE, ResourceManager::SETCZDC_MIN_FILE_SIZE },
	{ IDC_SETTINGS_MB, ResourceManager::MiB },
	{ IDC_REMOVE_IF, ResourceManager::NEW_DISCONNECT },
	{ IDC_SB_SKIPLIST_DOWNLOAD,	ResourceManager::SETTINGS_SKIPLIST_DOWNLOAD	},
	{ IDC_DL_SKIPPING_OPTIONS,	ResourceManager::SETTINGS_SKIPPING_OPTIONS	},
	{ IDC_DOWNLOAD_SKIPLIST_USE_REGEXP, ResourceManager::USE_REGEXP },
	{ IDC_OVERLAP_SLOW_SOURCES, ResourceManager::SETTINGS_OVERLAP_SLOW_SOURCES },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item AirDownloadsPage::items[] = {
	{ IDC_I_DOWN_SPEED, SettingsManager::DISCONNECT_SPEED, PropPage::T_INT },
	{ IDC_TIME_DOWN, SettingsManager::DISCONNECT_TIME, PropPage::T_INT },
	{ IDC_H_DOWN_SPEED, SettingsManager::DISCONNECT_FILE_SPEED, PropPage::T_INT },
	{ IDC_MIN_FILE_SIZE, SettingsManager::DISCONNECT_FILESIZE, PropPage::T_INT },
	{ IDC_REMOVE_IF_BELOW, SettingsManager::REMOVE_SPEED, PropPage::T_INT },
	{ IDC_SKIPLIST_DOWNLOAD, SettingsManager::SKIPLIST_DOWNLOAD, PropPage::T_STR },
	{ IDC_DOWNLOAD_SKIPLIST_USE_REGEXP, SettingsManager::DOWNLOAD_SKIPLIST_USE_REGEXP, PropPage::T_BOOL },
	{ IDC_MONITOR_SLOW_SPEED, SettingsManager::DROP_MULTISOURCE_ONLY, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

PropPage::ListItem AirDownloadsPage::optionItems[] = {
	{ SettingsManager::SKIP_ZERO_BYTE, ResourceManager::SETTINGS_SKIP_ZERO_BYTE },
	{ SettingsManager::DONT_DL_ALREADY_SHARED, ResourceManager::SETTINGS_DONT_DL_ALREADY_SHARED },
	{ SettingsManager::DONT_DL_ALREADY_QUEUED, ResourceManager::SETTING_DONT_DL_ALREADY_QUEUED },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT AirDownloadsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);
	PropPage::read((HWND)*this, items, optionItems, GetDlgItem(IDC_OTHER_SKIPPING_OPTIONS));

	CUpDownCtrl spin;
	spin.Attach(GetDlgItem(IDC_I_DOWN_SPEED_SPIN));
	spin.SetRange32(0, 99999);
	spin.Detach(); 
	spin.Attach(GetDlgItem(IDC_TIME_DOWN_SPIN));
	spin.SetRange32(1, 99999);
	spin.Detach(); 
	spin.Attach(GetDlgItem(IDC_H_DOWN_SPEED_SPIN));
	spin.SetRange32(0, 99999);
	spin.Detach(); 
	spin.Attach(GetDlgItem(IDC_MIN_FILE_SIZE_SPIN));
	spin.SetRange32(0, 99999);
	spin.Detach(); 
	spin.Attach(GetDlgItem(IDC_REMOVE_SPIN));
	spin.SetRange32(0, 99999);
	spin.Detach(); 

	checkItems();
	// Do specialized reading here
	return TRUE;
}

void AirDownloadsPage::write() {

	PropPage::write((HWND)*this, items);
	 
	//set to the defaults
	if(SETTING(SKIPLIST_DOWNLOAD).empty())
		settings->set(SettingsManager::DOWNLOAD_SKIPLIST_USE_REGEXP, false);

	TCHAR buf[256];
	GetDlgItemText(IDC_ANTIVIR_PATH, buf, 256);
	settings->set(SettingsManager::ANTIVIR_PATH, Text::fromT(buf));

	PropPage::write((HWND)*this, items, optionItems, GetDlgItem(IDC_OTHER_SKIPPING_OPTIONS));
}

LRESULT AirDownloadsPage::onTick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
	checkItems();
	return 0;
}

void AirDownloadsPage::checkItems() {
	::EnableWindow(GetDlgItem(IDC_CZDC_I_DOWN_SPEED),		IsDlgButtonChecked(IDC_MONITOR_SLOW_SPEED));
	::EnableWindow(GetDlgItem(IDC_I_DOWN_SPEED),			IsDlgButtonChecked(IDC_MONITOR_SLOW_SPEED));
	::EnableWindow(GetDlgItem(IDC_SETTINGS_KBPS5),			IsDlgButtonChecked(IDC_MONITOR_SLOW_SPEED));

	::EnableWindow(GetDlgItem(IDC_CZDC_TIME_DOWN),			IsDlgButtonChecked(IDC_MONITOR_SLOW_SPEED));
	::EnableWindow(GetDlgItem(IDC_TIME_DOWN),				IsDlgButtonChecked(IDC_MONITOR_SLOW_SPEED));
	::EnableWindow(GetDlgItem(IDC_SETTINGS_MINUTES),		IsDlgButtonChecked(IDC_MONITOR_SLOW_SPEED));

	::EnableWindow(GetDlgItem(IDC_CZDC_H_DOWN_SPEED),		IsDlgButtonChecked(IDC_MONITOR_SLOW_SPEED));
	::EnableWindow(GetDlgItem(IDC_H_DOWN_SPEED),			IsDlgButtonChecked(IDC_MONITOR_SLOW_SPEED));
	::EnableWindow(GetDlgItem(IDC_SETTINGS_KBPS6),			IsDlgButtonChecked(IDC_MONITOR_SLOW_SPEED));

	::EnableWindow(GetDlgItem(IDC_CZDC_MIN_FILE_SIZE),		IsDlgButtonChecked(IDC_MONITOR_SLOW_SPEED));
	::EnableWindow(GetDlgItem(IDC_MIN_FILE_SIZE),			IsDlgButtonChecked(IDC_MONITOR_SLOW_SPEED));
	::EnableWindow(GetDlgItem(IDC_SETTINGS_MB),				IsDlgButtonChecked(IDC_MONITOR_SLOW_SPEED));

	::EnableWindow(GetDlgItem(IDC_REMOVE_IF),				IsDlgButtonChecked(IDC_MONITOR_SLOW_SPEED));
	::EnableWindow(GetDlgItem(IDC_REMOVE_IF_BELOW),			IsDlgButtonChecked(IDC_MONITOR_SLOW_SPEED));
	::EnableWindow(GetDlgItem(IDC_SETTINGS_KBPS7),			IsDlgButtonChecked(IDC_MONITOR_SLOW_SPEED));
}



