/* 
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
#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"
#include "Resource.h"

#include "SearchPage.h"

PropPage::TextItem AutosearchPage::texts[] = {
	{ IDC_SEARH_SKIPLIST_PRESET,				ResourceManager::SETTINGS_SEARH_SKIPLIST_PRESET },
	{ IDC_PRE1,									ResourceManager::PRESET1 },
	{ IDC_PRE2,									ResourceManager::PRESET2 },
	{ IDC_PRE3,									ResourceManager::PRESET3 },
	{ 0,										ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item AutosearchPage::items[] = {
	{ IDC_SKIPLIST_PRESET1, SettingsManager::SKIP_MSG_01, PropPage::T_STR },
	{ IDC_SKIPLIST_PRESET2, SettingsManager::SKIP_MSG_02, PropPage::T_STR },
	{ IDC_SKIPLIST_PRESET3, SettingsManager::SKIP_MSG_03, PropPage::T_STR },

	{ 0, 0, PropPage::T_END }
};

LRESULT AutosearchPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	return TRUE;
}

void AutosearchPage::write() {
	
	PropPage::write((HWND)*this, items);
}
