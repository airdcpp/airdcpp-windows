#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"
#include "../client/ShareManager.h"
#include "../client/AirUtil.h"
#include "Resource.h"

#include "SharingOptionsPage.h"
#include "LineDlg.h"
#include "CommandDlg.h"

#include "WinUtil.h"
#include "PropertiesDlg.h"


PropPage::ListItem SharingOptionsPage::listItems[] = {
	{ SettingsManager::CHECK_MISSING, ResourceManager::CHECK_MISSING },
	{ SettingsManager::CHECK_SFV, ResourceManager::CHECK_SFV },
	{ SettingsManager::CHECK_NFO, ResourceManager::CHECK_NFO },
	{ SettingsManager::CHECK_MP3_DIR, ResourceManager::CHECK_MP3_DIR },
	{ SettingsManager::CHECK_EXTRA_SFV_NFO, ResourceManager::CHECK_EXTRA_SFV_NFO },
	{ SettingsManager::CHECK_EXTRA_FILES, ResourceManager::CHECK_EXTRA_FILES },
	{ SettingsManager::CHECK_DUPES, ResourceManager::CHECK_DUPES },
	{ SettingsManager::CHECK_EMPTY_DIRS, ResourceManager::CHECK_EMPTY_DIRS },
	{ SettingsManager::CHECK_EMPTY_RELEASES, ResourceManager::CHECK_EMPTY_RELEASES },
	{ SettingsManager::CHECK_USE_SKIPLIST, ResourceManager::CHECK_USE_SKIPLIST },
	{ SettingsManager::CHECK_IGNORE_ZERO_BYTE, ResourceManager::CHECK_IGNORE_ZERO_BYTE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::TextItem SharingOptionsPage::texts[] = {
	{ IDC_ST_MINISLOTS_EXT, ResourceManager::ST_MINISLOTS_EXT },
	{ IDC_SB_MINISLOTS, ResourceManager::SB_MINISLOTS },
	{ IDC_SB_SKIPLIST_SHARE, ResourceManager::ST_SKIPLIST_SHARE_BORDER },
	{ IDC_ST_SKIPLIST_SHARE_EXT, ResourceManager::ST_SKIPLIST_SHARE },
	{ IDC_SHARE_SKIPLIST_USE_REGEXP, ResourceManager::USE_REGEXP },
	{ IDC_DONT_SHARE_BIGGER_THAN, ResourceManager::DONT_SHARE_BIGGER_THAN },
	{ IDC_SETTINGS_MB2, ResourceManager::MiB },
	{ IDC_MINUTES, ResourceManager::MINUTES },
	{ IDC_SETTINGS_SCAN_OPTIONS, ResourceManager::SETTINGS_SCAN_OPTIONS },
	{ IDC_SETTINGS_AUTO_REFRESH_TIME, ResourceManager::SETTINGS_AUTO_REFRESH_TIME },
	{ IDC_SETTINGS_INCOMING_REFRESH_TIME, ResourceManager::SETTINGS_INCOMING_REFRESH_TIME },
	{ IDC_SETTINGS_MAX_HASH_SPEED, ResourceManager::SETTINGS_MAX_HASH_SPEED },
	{ IDC_SETTINGS_MBS, ResourceManager::MBPS },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item SharingOptionsPage::items[] = {
	{ IDC_SKIPLIST_SHARE, SettingsManager::SKIPLIST_SHARE, PropPage::T_STR },
	{ IDC_MINISLOTS_EXTENSIONS, SettingsManager::FREE_SLOTS_EXTENSIONS, PropPage::T_STR },
	{ IDC_SHARE_SKIPLIST_USE_REGEXP, SettingsManager::SHARE_SKIPLIST_USE_REGEXP, PropPage::T_BOOL },
	{ IDC_DONT_SHARE_BIGGER_VALUE, SettingsManager::MAX_FILE_SIZE_SHARED, PropPage::T_INT },
	{ IDC_SHAREHIDDEN, SettingsManager::SHARE_HIDDEN, PropPage::T_BOOL },
	{ IDC_AUTO_REFRESH_TIME, SettingsManager::AUTO_REFRESH_TIME, PropPage::T_INT },
	{ IDC_INCOMING_REFRESH_TIME, SettingsManager::INCOMING_REFRESH_TIME, PropPage::T_INT },
	{ IDC_MAX_HASH_SPEED, SettingsManager::MAX_HASH_SPEED, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};



LRESULT SharingOptionsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_SCANLIST));

	setMinMax(IDC_REFRESH_SPIN, 0, 3000);
	setMinMax(IDC_INCOMING_SPIN, 0, 1000);
	setMinMax(IDC_HASH_SPIN, 0, 999);

	// Do specialized reading here
	return TRUE;
}

void SharingOptionsPage::write() {
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_SCANLIST));
	
	//set to the defaults
	if(SETTING(SKIPLIST_SHARE).empty())
		settings->set(SettingsManager::SHARE_SKIPLIST_USE_REGEXP, true);
}
 


