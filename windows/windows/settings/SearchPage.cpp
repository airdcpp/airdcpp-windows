/* 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include <windows/stdafx.h>
#include <windows/Resource.h>

#include <windows/settings/SearchPage.h>
#include <airdcpp/modules/AutoSearchManager.h>

namespace wingui {
PropPage::TextItem SearchPage::texts[] = {
	{ IDC_SETTINGS_SECONDS,						ResourceManager::SECONDS_LOWER },
	{ IDC_INTERVAL_TEXT,						ResourceManager::MINIMUM_SEARCH_INTERVAL },
	{ IDC_SEARCHING_OPTIONS,					ResourceManager::SETTINGS_SEARCHING_OPTIONS	},
	{ IDC_EXPIRY_DAYS_LABEL,					ResourceManager::SETTINGS_EXPIRY_DAYS	},
	{ IDC_DELAY_HOURS_LABEL,					ResourceManager::SETTINGS_DELAY_HOURS	},
	{ IDC_REMOVE_EXPIRED_AS,					ResourceManager::REMOVE_EXPIRED_AS },
	{ IDC_AUTO_SEARCH,							ResourceManager::AUTO_SEARCH },
	{ IDC_AS_MIN_INTERVAL_LABEL,				ResourceManager::AUTOSEARCH_EVERY_INTERVAL },
	{ IDC_GROUP_FAILED_LABEL,					ResourceManager::AUTOSEARCH_DEFAULT_FAILED_GROUP },
	{ 0, ResourceManager::LAST }
};

PropPage::Item SearchPage::items[] = {
	{ IDC_INTERVAL, SettingsManager::MINIMUM_SEARCH_INTERVAL, PropPage::T_INT },
	{ IDC_EXPIRY_DAYS, SettingsManager::AUTOSEARCH_EXPIRE_DAYS, PropPage::T_INT }, 
	{ IDC_DELAY_HOURS, SettingsManager::AS_DELAY_HOURS, PropPage::T_INT }, 
	{ IDC_AS_MIN_INTERVAL, SettingsManager::AUTOSEARCH_EVERY, PropPage::T_INT },
	{ IDC_REMOVE_EXPIRED_AS, SettingsManager::REMOVE_EXPIRED_AS, PropPage::T_BOOL }, 
	{ 0, 0, PropPage::T_END }
};

LRESULT SearchPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);
	
	setMinMax(IDC_INTERVAL_SPIN, 5, 9999);
	setMinMax(IDC_EXPIRY_SPIN, 0, 90);
	setMinMax(IDC_DELAY_SPIN, 0, 999);
	setMinMax(IDC_AS_MIN_INTERVAL_SPIN, 1, 999);

	failedGroup.Attach(GetDlgItem(IDC_FAILED_GROUP));
	auto curSetting = SETTING(AS_FAILED_DEFAULT_GROUP);
	failedGroup.AddString(_T("---"));
	failedGroup.SetCurSel(0);
	auto groups = AutoSearchManager::getInstance()->getGroups();
	for (const auto& i : groups) {
		int pos = failedGroup.AddString(Text::toT(i).c_str());
		if (i == curSetting)
			failedGroup.SetCurSel(pos);
	}

	return TRUE;
}


void SearchPage::write() {
	PropPage::write((HWND)*this, items);

	string selectedGroup;
	tstring tmp;
	tmp.resize(failedGroup.GetWindowTextLength());
	tmp.resize(failedGroup.GetWindowText(&tmp[0], tmp.size() + 1));
	selectedGroup = Text::fromT(tmp);
	
	//Set empty group ---, settingsmanager empty is default
	SettingsManager::getInstance()->set(SettingsManager::AS_FAILED_DEFAULT_GROUP, selectedGroup);
}

SearchPage::~SearchPage() {
	free(title);
};
}