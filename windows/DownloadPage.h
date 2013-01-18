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

#if !defined(DOWNLOAD_PAGE_H)
#define DOWNLOAD_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"

class DownloadPage : public CPropertyPage<IDD_DOWNLOADPAGE>, public PropPage
{
public:
	DownloadPage(SettingsManager *s) : PropPage(s) {
		SetTitle(CTSTRING(SETTINGS_DOWNLOADS));
		m_psp.dwFlags |= PSP_RTLREADING;
	}
	~DownloadPage() { }

	BEGIN_MSG_MAP(DownloadPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_ENABLE_SEGMENTS, onTick)
		COMMAND_ID_HANDLER(IDC_AUTO_SEARCH_ALT, onTick)
		COMMAND_ID_HANDLER(IDC_CHUNKCOUNT, onTick)
		COMMAND_ID_HANDLER(IDC_DONTBEGIN, onTick)
		COMMAND_ID_HANDLER(IDC_AUTO_ADD_SOURCES, onTick)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onTick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	void checkItems();
protected:

	static Item items[];
	static TextItem texts[];
	static ListItem optionItems[];
};

#endif //  !defined(DOWNLOAD_PAGE_H)
