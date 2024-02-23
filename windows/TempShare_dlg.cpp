/*
* Copyright (C) 2012-2024 AirDC++ Project
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

#include "stdafx.h"
#include "Resource.h"
#include "ExListViewCtrl.h"

#include "TempShare_dlg.h"
#include "FormatUtil.h"

#include <airdcpp/ClientManager.h>
#include <airdcpp/ShareManager.h>



LRESULT TempShareDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlFiles.Attach(GetDlgItem(IDC_TEMPSHARELST));
	ctrlFiles.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	files = ShareManager::getInstance()->getTempShares();

	ctrlFiles.InsertColumn(0, CTSTRING(NAME), LVCFMT_LEFT, 180, 0);
	ctrlFiles.InsertColumn(1, CTSTRING(SIZE), LVCFMT_RIGHT, 60, 2);
	ctrlFiles.InsertColumn(2, _T("TTH"), LVCFMT_LEFT, 90, 1);
	ctrlFiles.InsertColumn(3, CTSTRING(USER), LVCFMT_LEFT, 90, 1);

	for (const auto& info: files) {
		int item = ctrlFiles.insert(ctrlFiles.GetItemCount(), Text::toT(info.path), 0, (LPARAM)(&info));
		ctrlFiles.SetItemText(item, 1, Text::toT(Util::formatBytes(info.size)).c_str());
		ctrlFiles.SetItemText(item, 2, Text::toT(info.tth.toBase32()).c_str());
		ctrlFiles.SetItemText(item, 3, (info.user ? FormatUtil::getNicks(info.user->getCID()) : Util::emptyStringT).c_str());
	}

	CenterWindow(GetParent());
	return TRUE;
}

LRESULT TempShareDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlFiles.DeleteAllItems();
	ctrlFiles.Detach();
	EndDialog(wID);
	return 0;
}

LRESULT TempShareDlg::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	TCHAR buf[MAX_PATH];
	LVITEM item;
	memzero(&item, sizeof(item));
	item.mask = LVIF_TEXT;
	item.cchTextMax = sizeof(buf);
	item.pszText = buf;

	int i = -1;
	while ((i = ctrlFiles.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		item.iItem = i;

		auto file = (TempShareInfo*)ctrlFiles.GetItemData(i);
		if (ShareManager::getInstance()->removeTempShare(file->id)) {
			ctrlFiles.DeleteItem(i);
		}
	}
	return 0;
}

LRESULT TempShareDlg::onRemoveAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	for (const auto& file : files) {
		ShareManager::getInstance()->removeTempShare(file.id);
	}

	ctrlFiles.DeleteAllItems();
	return 0;
}