/* 
 * Copyright (C) 2012-2013 AirDC++ team
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

#if !defined(TEMPSHARE_DLG_H)
#define TEMPSHARE_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "Resource.h"
#include "WinUtil.h"
#include "ExListViewCtrl.h"
#include "../client/ShareManager.h"

class TempShareDlg : public CDialogImpl<TempShareDlg>
{

public:

	enum { IDD = IDD_TEMPSHARE_DLG };
	
	ExListViewCtrl ctrlFiles;

	BEGIN_MSG_MAP(TempShareDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_TMP_REMOVE, onRemove)
	END_MSG_MAP()
	
	TempShareDlg() { }
	~TempShareDlg() { }
	

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlFiles.Attach(GetDlgItem(IDC_TEMPSHARELST));
		ctrlFiles.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

		auto files = ShareManager::getInstance()->getTempShares();

		ctrlFiles.InsertColumn(0, CTSTRING(NAME), LVCFMT_LEFT, 180, 0);
		ctrlFiles.InsertColumn(1, CTSTRING(SIZE), LVCFMT_RIGHT, 60, 2);
		ctrlFiles.InsertColumn(2, _T("TTH"), LVCFMT_LEFT, 90, 1);
		ctrlFiles.InsertColumn(3, CTSTRING(CID), LVCFMT_LEFT, 90, 1);

		for(auto i = files.begin(); i != files.end(); ++i) {
			int item = ctrlFiles.insert(ctrlFiles.GetItemCount(), Text::toT(i->second.path));
			ctrlFiles.SetItemText(item, 1, Text::toT(Util::formatBytes(i->second.size)).c_str());
			ctrlFiles.SetItemText(item, 2, Text::toT(i->first.toBase32()).c_str());
			ctrlFiles.SetItemText(item, 3, Text::toT(i->second.key).c_str());

		}

		CenterWindow(GetParent());
		return TRUE;
	}
	
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlFiles.DeleteAllItems();
		ctrlFiles.Detach();
		EndDialog(wID);
		return 0;
	}
	
	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		TCHAR buf[MAX_PATH];
		LVITEM item;
		memzero(&item, sizeof(item));
		item.mask = LVIF_TEXT;
		item.cchTextMax = sizeof(buf);
		item.pszText = buf;

		int i = -1;
		while((i = ctrlFiles.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			item.iItem = i;

			item.iSubItem = 2;
			ctrlFiles.GetItem(&item);
			TTHValue tth = TTHValue(Text::fromT(buf));
			
			item.iSubItem = 3;
			ctrlFiles.GetItem(&item);
			string key = Text::fromT(buf);

			ShareManager::getInstance()->removeTempShare(key, tth);
			ctrlFiles.DeleteItem(i);
		}
		return 0;
	}
	
};
#endif
