#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"
#include "Resource.h"

#include "AirSharingPage.h"
#include "LineDlg.h"
#include "CommandDlg.h"

#include "WinUtil.h"
#include "PropertiesDlg.h"

PropPage::TextItem AirSharingPage::texts[] = {
	{ IDC_ST_MINISLOTS_EXT, ResourceManager::ST_MINISLOTS_EXT },
	{ IDC_SB_MINISLOTS, ResourceManager::SB_MINISLOTS },
	{ IDC_SB_SKIPLIST_SHARE, ResourceManager::ST_SKIPLIST_SHARE_BORDER },
	{ IDC_ST_SKIPLIST_SHARE_EXT, ResourceManager::ST_SKIPLIST_SHARE },
	{ IDC_SHARE_SKIPLIST_USE_REGEXP, ResourceManager::USE_REGEXP },
	{ IDC_ST_REFRESH_DELAY, ResourceManager::ST_REFRESH_DELAY },
	{ IDC_MINUTES, ResourceManager::MINUTES },
	{ IDC_ST_AIRDC_SLOT, ResourceManager::ST_AIRDC_SLOT },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item AirSharingPage::items[] = {
	{ IDC_SKIPLIST_SHARE, SettingsManager::SKIPLIST_SHARE, PropPage::T_STR },
	{ IDC_MINISLOTS_EXTENSIONS, SettingsManager::FREE_SLOTS_EXTENSIONS, PropPage::T_STR },
	{ IDC_SHARE_SKIPLIST_USE_REGEXP, SettingsManager::SHARE_SKIPLIST_USE_REGEXP, PropPage::T_BOOL },
	{ IDC_REFRESH_DELAY, SettingsManager::REFRESH_STARTUP, PropPage::T_INT },
	{ IDC_AIRDC_SLOT, SettingsManager::EXTRA_AIRDC_SLOTS, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};



LRESULT AirSharingPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);
	
	// Do specialized reading here
	return TRUE;
}

void AirSharingPage::write() {
	
	PropPage::write((HWND)*this, items);
	

}
 


