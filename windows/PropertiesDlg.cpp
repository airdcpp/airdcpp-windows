/* 
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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

#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"

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
#include "UserListColours.h"
#include "NetworkPage.h"
#include "WindowsPage.h"
#include "PriorityPage.h"
#include "CertificatesPage.h"
#include "LimitPage.h"

#include "MiscPage.h"
#include "FulTabsPage.h"
#include "DownloadingOptionsPage.h"
#include "SharingOptionsPage.h"
#include "IgnorePage.h"
#include "Popups.h"
#include "FulHighlightPage.h"
#include "SearchPage.h"
#include "AirAppearancePage.h"
#include "SearchTypesPage.h"


PropertiesDlg::PropertiesDlg(HWND parent, SettingsManager *s, uint16_t initialPage) : TreePropertySheet(CTSTRING(SETTINGS), initialPage, parent)
{

	int n = 0;
	pages[n++] = new GeneralPage(s);
	pages[n++] = new NetworkPage(s);
	pages[n++] = new SpeedPage(s);
	pages[n++] = new LimitPage(s);
	pages[n++] = new DownloadPage(s);
	pages[n++] = new LocationsPage(s);
	pages[n++] = new AVIPreview(s);	
	pages[n++] = new PriorityPage(s);
	pages[n++] = new DownloadingOptionsPage(s);
	pages[n++] = new SharePage(s);
	pages[n++] = new SharingOptionsPage(s);
	pages[n++] = new AppearancePage(s);
	pages[n++] = new PropPageTextStyles(s);
	pages[n++] = new OperaColorsPage(s);
	pages[n++] = new UserListColours(s);
	pages[n++] = new Sounds(s);
	pages[n++] = new ToolbarPage(s);
	pages[n++] = new WindowsPage(s);
	pages[n++] = new FulTabsPage(s);
	pages[n++] = new Popups(s);
	pages[n++] = new FulHighlightPage(s);
	pages[n++] = new AirAppearancePage(s);
	pages[n++] = new AdvancedPage(s);
	pages[n++] = new SDCPage(s);
	pages[n++] = new LogPage(s);
	pages[n++] = new UCPage(s);	
	pages[n++] = new CertificatesPage(s);
	pages[n++] = new MiscPage(s);
	pages[n++] = new IgnorePage(s);
	pages[n++] = new SearchPage(s);
	pages[n++] = new SearchTypesPage(s);
	
	for(int i=0; i < n; i++) {
		AddPage(pages[i]->getPSP());
	}

	// Hide "Apply" button
	m_psh.dwFlags |= PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
	m_psh.dwFlags &= ~PSH_HASHELP;

}

PropertiesDlg::~PropertiesDlg()
{
	for(int i=0; i<numPages; i++) {
		delete pages[i];
	}
}

void PropertiesDlg::write()
{
	for(int i=0; i<numPages; i++)
	{
		// Check HWND of page to see if it has been created
		const HWND page = PropSheet_IndexToHwnd((HWND)*this, i);

		if(page != NULL)
			pages[i]->write();	
	}
	
}

LRESULT PropertiesDlg::onOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
	write();
	bHandled = FALSE;
	WinUtil::lastSettingPage = curPage;
	//SettingsManager::getInstance()->set(SettingsManager::LAST_SETTING_PAGE, curPage);
	return TRUE;
}