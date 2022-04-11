/* 
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
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

#include <airdcpp/SettingsManager.h>

#include "Resource.h"

#include "PropertiesDlg.h"

#include "GeneralPage.h"
#include "DownloadPage.h"
#include "SharePage.h"
#include "SpeedPage.h"
#include "AppearancePage.h"
#include "AdvancedPage.h"
#include "LogPage.h"
#include "Sounds.h"
#include "UCPage.h"
#include "PropPageTextStyles.h"
#include "AVIPreview.h"
#include "OperaColorsPage.h"
#include "ToolbarPage.h"
#include "LocationsPage.h"
#include "SDCPage.h"
#include "ListViewColours.h"
#include "NetworkPage.h"
#include "ProxyPage.h"
#include "WindowsPage.h"
#include "PriorityPage.h"
#include "EncryptionPage.h"
#include "LimitPage.h"

#include "MiscPage.h"
#include "FulTabsPage.h"
#include "DownloadingOptionsPage.h"
#include "SharingOptionsPage.h"
#include "ChatFilterPage.h"
#include "Popups.h"
#include "FulHighlightPage.h"
#include "SearchPage.h"
#include "AirAppearancePage.h"
#include "SearchTypesPage.h"
#include "HashingPage.h"
#include "WebServerPage.h"


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