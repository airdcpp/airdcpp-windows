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

#if !defined(PROPERTIES_DLG_H)
#define PROPERTIES_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropPage.h"
#include "TreePropertySheet.h"

class PropertiesDlg : public TreePropertySheet
{


public:
	enum { numPages = 29 };

	BEGIN_MSG_MAP(PropertiesDlg)
		COMMAND_ID_HANDLER(IDOK, onOK)
		CHAIN_MSG_MAP(TreePropertySheet)
	ALT_MSG_MAP(TreePropertySheet::TAB_MESSAGE_MAP)
		MESSAGE_HANDLER(TCM_SETCURSEL, TreePropertySheet::onSetCurSel)
	END_MSG_MAP()

	PropertiesDlg(HWND parent, SettingsManager *s, uint16_t initialPage);
	~PropertiesDlg();
	static bool needUpdate;

	LRESULT onOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
protected:
	void write();
	

	PropPage *pages[numPages];
};

#endif // !defined(PROPERTIES_DLG_H)

/**
 * @file
 * $Id: PropertiesDlg.h 395 2008-06-30 12:11:32Z BigMuscle $
 */
