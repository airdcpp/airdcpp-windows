/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(PROPERTIES_DLG_H)
#define PROPERTIES_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropPage.h"
#include "TreePropertySheet.h"
#include "Dispatchers.h"


class PropertiesDlg : public TreePropertySheet
{


public:
	enum Pages { 
		PAGE_GENERAL,
		PAGE_CONNECTION,
		PAGE_SPEED_SLOTS,
		PAGE_LIMITS,
		PAGE_PROXIES,
		PAGE_DOWNLOADS,
		PAGE_DL_LOCATIONS,
		PAGE_PREVIEW,
		PAGE_PRIORITIES,
		PAGE_DL_OPTIONS,
		PAGE_SHARING,
		PAGE_SHARING_OPTIONS,
		PAGE_APPEARANCE,
		PAGE_COLORS,
		PAGE_PROGRESSBAR,
		PAGE_USERLIST,
		PAGE_SOUNDS,
		PAGE_TOOLBAR,
		PAGE_WINDOWS,
		PAGE_TABS,
		PAGE_POPUPS,
		PAGE_HIGHLIGHT,
		PAGE_AIRAPPEARANCE,
		PAGE_ADVANCED,
		PAGE_EXPERTS,
		PAGE_LOGS,
		PAGE_USERCOMMANDS,
		PAGE_ENCRYPTION,
		PAGE_MISCELLANEOUS,
		PAGE_IGNORE,
		PAGE_SEARCH,
		PAGE_SEARCHTYPES,
		PAGE_SCAN,
		PAGE_LAST
	};

	BEGIN_MSG_MAP(PropertiesDlg)
		COMMAND_ID_HANDLER(IDOK, onOK)
		COMMAND_ID_HANDLER(IDCANCEL, onCancel)
		CHAIN_MSG_MAP(TreePropertySheet)
	ALT_MSG_MAP(TreePropertySheet::TAB_MESSAGE_MAP)
		MESSAGE_HANDLER(TCM_SETCURSEL, TreePropertySheet::onSetCurSel)
	END_MSG_MAP()

	PropertiesDlg(HWND parent, SettingsManager *s, uint16_t initialPage);
	~PropertiesDlg();

	void deletePages(PropPage::TaskList& tasks);
	LRESULT onOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
protected:
	void write();
	
	bool saved;
	PropPage *pages[PAGE_LAST];
};

#endif // !defined(PROPERTIES_DLG_H)