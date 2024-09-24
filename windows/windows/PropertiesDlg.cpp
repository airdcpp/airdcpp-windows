/* 
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
 *
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

#include <airdcpp/SettingsManager.h>

#include <windows/Resource.h>

#include <windows/PropertiesDlg.h>

#include <windows/GeneralPage.h>
#include <windows/DownloadPage.h>
#include <windows/SharePage.h>
#include <windows/SpeedPage.h>
#include <windows/AppearancePage.h>
#include <windows/AdvancedPage.h>
#include <windows/LogPage.h>
#include <windows/Sounds.h>
#include <windows/UCPage.h>
#include <windows/PropPageTextStyles.h>
#include <windows/AVIPreview.h>
#include <windows/OperaColorsPage.h>
#include <windows/ToolbarPage.h>
#include <windows/LocationsPage.h>
#include <windows/SDCPage.h>
#include <windows/ListViewColours.h>
#include <windows/NetworkPage.h>
#include <windows/ProxyPage.h>
#include <windows/WindowsPage.h>
#include <windows/PriorityPage.h>
#include <windows/EncryptionPage.h>
#include <windows/LimitPage.h>

#include <windows/MiscPage.h>
#include <windows/FulTabsPage.h>
#include <windows/DownloadingOptionsPage.h>
#include <windows/SharingOptionsPage.h>
#include <windows/ChatFilterPage.h>
#include <windows/Popups.h>
#include <windows/FulHighlightPage.h>
#include <windows/SearchPage.h>
#include <windows/AirAppearancePage.h>
#include <windows/SearchTypesPage.h>
#include <windows/HashingPage.h>
#include <windows/WebServerPage.h>


namespace wingui {
PropertiesDlg::PropertiesDlg(HWND parent, SettingsManager *s, uint16_t initialPage) : TreePropertySheet(CTSTRING(SETTINGS), initialPage, parent), saved(false)
{

	int n = 0;
	pages[n++] = make_unique<GeneralPage>(s);
	pages[n++] = make_unique<NetworkPage>(s);
	pages[n++] = make_unique<SpeedPage>(s);
	pages[n++] = make_unique<LimitPage>(s);
	pages[n++] = make_unique<ProxyPage>(s);
	pages[n++] = make_unique<DownloadPage>(s);
	pages[n++] = make_unique<LocationsPage>(s);
	pages[n++] = make_unique<AVIPreview>(s);
	pages[n++] = make_unique<PriorityPage>(s);
	pages[n++] = make_unique<DownloadingOptionsPage>(s);
	pages[n++] = make_unique<SharePage>(s);
	pages[n++] = make_unique<SharingOptionsPage>(s);
	pages[n++] = make_unique<HashingPage>(s);
	pages[n++] = make_unique<WebServerPage>(s);
	pages[n++] = make_unique<AppearancePage>(s);
	pages[n++] = make_unique<PropPageTextStyles>(s);
	pages[n++] = make_unique<OperaColorsPage>(s);
	pages[n++] = make_unique<ListViewColours>(s);
	pages[n++] = make_unique<Sounds>(s);
	pages[n++] = make_unique<ToolbarPage>(s);
	pages[n++] = make_unique<WindowsPage>(s);
	pages[n++] = make_unique<FulTabsPage>(s);
	pages[n++] = make_unique<Popups>(s);
	pages[n++] = make_unique<FulHighlightPage>(s);
	pages[n++] = make_unique<AirAppearancePage>(s);
	pages[n++] = make_unique<AdvancedPage>(s);
	pages[n++] = make_unique<SDCPage>(s);
	pages[n++] = make_unique<LogPage>(s);
	pages[n++] = make_unique<UCPage>(s);
	pages[n++] = make_unique<EncryptionPage>(s);
	pages[n++] = make_unique<MiscPage>(s);
	pages[n++] = make_unique<ChatFilterPage>(s);
	pages[n++] = make_unique<SearchPage>(s);
	pages[n++] = make_unique<SearchTypesPage>(s);
	
	for(int i=0; i < n; i++) {
		AddPage(pages[i]->getPSP());
	}

	// Hide "Apply" button
	m_psh.dwFlags |= PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
	m_psh.dwFlags &= ~PSH_HASHELP;

}

PropertiesDlg::~PropertiesDlg() { }

void PropertiesDlg::getTasks(PropPage::TaskList& tasks) {
	for(int i=0; i < PAGE_LAST; i++) {
		if (saved) {
			auto t = pages[i]->getThreadedTask();
			if (t) {
				tasks.emplace_back(t);
			}
		}
	}
}

void PropertiesDlg::write()
{
	for(int i=0; i < PAGE_LAST; i++)
	{
		// Check HWND of page to see if it has been created
		const HWND page = PropSheet_IndexToHwnd((HWND)*this, i);
		if(page)
			pages[i]->write();	
	}
	
}

LRESULT PropertiesDlg::onOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
	write();
	bHandled = FALSE;
	WinUtil::lastSettingPage = curPage;
	saved = true;
	//SettingsManager::getInstance()->set(SettingsManager::LAST_SETTING_PAGE, curPage);
	return TRUE;
}

LRESULT PropertiesDlg::onCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/,BOOL& bHandled) {
	WinUtil::lastSettingPage = curPage;
	SettingsManager::getInstance()->Cancel();
	bHandled = FALSE;
	return TRUE;
}
}