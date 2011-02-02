
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
	{ IDC_AIRDC_ANTI_VIR, ResourceManager::SETAIRDC_ANTI_VIR },
	{ IDC_ANTIVIR_BROWSE, ResourceManager::BROWSE },
	{ IDC_SETTINGS_KBPS5, ResourceManager::KBPS },
	{ IDC_SETTINGS_KBPS6, ResourceManager::KBPS },
	{ IDC_SETTINGS_KBPS7, ResourceManager::KBPS },
	{ IDC_SETTINGS_MINUTES, ResourceManager::SECONDS },
	{ IDC_CZDC_SLOW_DISCONNECT, ResourceManager::SETCZDC_SLOW_DISCONNECT },
	{ IDC_SEGMENTED_ONLY, ResourceManager::SETTINGS_AUTO_DROP_SEGMENTED_SOURCE },
	{ IDC_CZDC_I_DOWN_SPEED, ResourceManager::SETCZDC_I_DOWN_SPEED },
	{ IDC_CZDC_TIME_DOWN, ResourceManager::SETCZDC_TIME_DOWN },
	{ IDC_CZDC_H_DOWN_SPEED, ResourceManager::SETCZDC_H_DOWN_SPEED },
	{ IDC_DISCONNECTING_ENABLE, ResourceManager::SETCZDC_DISCONNECTING_ENABLE },
	{ IDC_CZDC_MIN_FILE_SIZE, ResourceManager::SETCZDC_MIN_FILE_SIZE },
	{ IDC_SETTINGS_MB, ResourceManager::MiB },
	{ IDC_REMOVE_IF, ResourceManager::NEW_DISCONNECT },
	{ IDC_SETTINGS_SEGMENTS, ResourceManager::SETTINGS_AIRDOWNLOADS_SEGMENT },
	{ IDC_MIN_SEGMENT_SIZE_LABEL, ResourceManager::SETTINGS_AIRDOWNLOADS_SEGMENT_SIZE },
	{ IDC_SB_SKIPLIST_DOWNLOAD,	ResourceManager::SETTINGS_SB_SKIPLIST_DOWNLOAD	},
	{ IDC_ST_SKIPLIST_DOWNLOAD,	ResourceManager::SETTINGS_ST_PATHS	},
	{ IDC_ST_HIGH_PRIO_FILES, ResourceManager::SETTINGS_ST_PATHS	},
	{ IDC_SB_HIGH_PRIO_FILES, ResourceManager::SETTINGS_SB_HIGH_PRIO_FILES	},

	{ IDC_HIGHEST_PRIORITY_USE_REGEXP, ResourceManager::USE_REGEXP },
	{ IDC_DOWNLOAD_SKIPLIST_USE_REGEXP, ResourceManager::USE_REGEXP },

	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item AirDownloadsPage::items[] = {

	{ IDC_I_DOWN_SPEED, SettingsManager::DISCONNECT_SPEED, PropPage::T_INT },
	{ IDC_TIME_DOWN, SettingsManager::DISCONNECT_TIME, PropPage::T_INT },
	{ IDC_H_DOWN_SPEED, SettingsManager::DISCONNECT_FILE_SPEED, PropPage::T_INT },
	{ IDC_MIN_FILE_SIZE, SettingsManager::DISCONNECT_FILESIZE, PropPage::T_INT },
	{ IDC_REMOVE_IF_BELOW, SettingsManager::REMOVE_SPEED, PropPage::T_INT },
	{ IDC_MIN_SEGMENT_SIZE, SettingsManager::MIN_SEGMENT_SIZE, PropPage::T_INT },

	{ IDC_SKIPLIST_DOWNLOAD, SettingsManager::SKIPLIST_DOWNLOAD, PropPage::T_STR },
	{ IDC_HIGH_PRIO_FILES,	SettingsManager::HIGH_PRIO_FILES, PropPage::T_STR },

	{ IDC_HIGHEST_PRIORITY_USE_REGEXP, SettingsManager::HIGHEST_PRIORITY_USE_REGEXP, PropPage::T_BOOL },
	{ IDC_DOWNLOAD_SKIPLIST_USE_REGEXP, SettingsManager::DOWNLOAD_SKIPLIST_USE_REGEXP, PropPage::T_BOOL },
	{ IDC_SEGMENTED_ONLY, SettingsManager::DROP_MULTISOURCE_ONLY, PropPage::T_BOOL },

	{ 0, 0, PropPage::T_END }
};


LRESULT AirDownloadsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{

	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

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

	if(SETTING(MIN_SEGMENT_SIZE) < 1024)
		settings->set(SettingsManager::MIN_SEGMENT_SIZE, 1024);
}

LRESULT AirDownloadsPage::onBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TCHAR buf[MAX_PATH];

	GetDlgItemText(IDC_ANTIVIR_PATH, buf, MAX_PATH);
	tstring x = buf;

	if(WinUtil::browseFile(x, m_hWnd, false) == IDOK) {
		SetDlgItemText(IDC_ANTIVIR_PATH, x.c_str());
	}
	return 0;
}



