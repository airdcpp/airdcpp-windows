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
#include "DownloadPage.h"
#include "WinUtil.h"
#include "PublicHubsListDlg.h"
#include "MainFrm.h"
#include "PropertiesDlg.h"

PropPage::TextItem DownloadPage::texts[] = {
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item DownloadPage::items[] = {
	{ 0, 0, PropPage::T_END }
};


PropPage::ListItem DownloadPage::optionItems[] = {
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT DownloadPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);
	PropPage::read((HWND)*this, items, optionItems, GetDlgItem(IDC_DOWNLOAD_OPTIONS));
	// Do specialized reading here
	return TRUE;
}

void DownloadPage::write()
{
	PropPage::write((HWND)*this, items);
	PropPage::write((HWND)*this, items, optionItems, GetDlgItem(IDC_DOWNLOAD_OPTIONS));
}

/**
 * @file
 * $Id: DownloadPage.cpp 412 2008-07-23 22:35:40Z BigMuscle $
 */
