/*
 * Copyright (C) 2007-2010 adrian_007, adrian-007 on o2 point pl
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

#ifndef STRONGDCPLUSPLUS_FAV_HUB_GROUPS_DLG
#define STRONGDCPLUSPLUS_FAV_HUB_GROUPS_DLG

#include "resource.h"

class FavHubGroupsDlg : public CDialogImpl<FavHubGroupsDlg> {
public:
	enum { IDD = IDD_FAVHUBGROUPS };

	BEGIN_MSG_MAP(FavHubGroupsDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		NOTIFY_HANDLER(IDC_GROUPS, LVN_ITEMCHANGED, onItemChanged)
		COMMAND_ID_HANDLER(IDCANCEL, onClose)
		COMMAND_ID_HANDLER(IDC_ADD, onAdd)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_UPDATE, onUpdate)		
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	void addItem(const dcpp::tstring& name, bool priv, bool select = false);
	bool getItem(dcpp::tstring& name, bool& priv, bool checkSel);
	int findGroup(LPCTSTR name);
	dcpp::tstring getText(int column, int item = -1) const;
	void updateSelectedGroup(bool forceClean = false);
	void save();

	CListViewCtrl ctrlGroups;
};

#endif //STRONGDCPLUSPLUS_FAV_HUB_GROUPS_DLG
