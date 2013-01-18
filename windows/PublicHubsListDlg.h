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

#if !defined(PUBLIC_HUBS_LIST_DLG_H)
#define PUBLIC_HUBS_LIST_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/SettingsManager.h"
#include "../client/Text.h"
#include "../Client/FavoriteManager.h"
#include "ExListViewCtrl.h"
#include "LineDlg.h"

class PublicHubListDlg : public CDialogImpl<PublicHubListDlg> {
public:
	enum { IDD = IDD_HUB_LIST };

	PublicHubListDlg() { }
	~PublicHubListDlg() { ctrlList.Detach(); }

	BEGIN_MSG_MAP(PublicHubListDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_LIST_ADD, onAdd);
		COMMAND_ID_HANDLER(IDC_LIST_UP, onMoveUp);
		COMMAND_ID_HANDLER(IDC_LIST_DOWN, onMoveDown);
		COMMAND_ID_HANDLER(IDC_LIST_EDIT, onEdit);
		COMMAND_ID_HANDLER(IDC_LIST_REMOVE, onRemove);
		COMMAND_ID_HANDLER(IDOK, onCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, onCloseCmd)
		NOTIFY_HANDLER(IDC_LIST_LIST, LVN_ITEMCHANGED, onItemchangedDirectories)
	END_MSG_MAP();

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		// set the title, center the window
		SetWindowText(CTSTRING(CONFIGURED_HUB_LISTS));
		CenterWindow(GetParent());

		// fill in translatable strings
		SetDlgItemText(IDC_LIST_DESC, _T(""));
		SetDlgItemText(IDC_LIST_ADD, CTSTRING(ADD));
		SetDlgItemText(IDC_LIST_UP, CTSTRING(MOVE_UP));
		SetDlgItemText(IDC_LIST_DOWN, CTSTRING(MOVE_DOWN));
		SetDlgItemText(IDC_LIST_EDIT, CTSTRING(EDIT_ACCEL));
		SetDlgItemText(IDC_LIST_REMOVE, CTSTRING(REMOVE));

		// set up the list of lists
		CRect rc;
		ctrlList.Attach(GetDlgItem(IDC_LIST_LIST));
		ctrlList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
		ctrlList.GetClientRect(rc);
		ctrlList.InsertColumn(0, CTSTRING(SETTINGS_NAME), LVCFMT_LEFT, rc.Width() - 4, 0);
		StringList lists(FavoriteManager::getInstance()->getHubLists());
		for(StringList::const_iterator idx = lists.begin(); idx != lists.end(); ++idx) {
			ctrlList.insert(ctrlList.GetItemCount(), Text::toT(*idx));
		}

		// set the initial focus
		CEdit focusThis;
		focusThis.Attach(GetDlgItem(IDC_LIST_EDIT_BOX));
		focusThis.SetFocus();

		return 0;
	}

	LRESULT onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
		TCHAR buf[256];
		if(GetDlgItemText(IDC_LIST_EDIT_BOX, buf, 256)) {
			ctrlList.insert(0, buf);
			SetDlgItemText(IDC_LIST_EDIT_BOX, _T(""));
		}
		bHandled = FALSE;
		return 0;
	}

	LRESULT onMoveUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
		int j = ctrlList.GetItemCount();
		for(int i = 1; i < j; ++i) {
			if(ctrlList.GetItemState(i, LVIS_SELECTED)) {
				ctrlList.moveItem(i, i-1);
		        ctrlList.SelectItem(i-1); 
		        ctrlList.SetFocus();
			}
		}
		bHandled = FALSE;
		return 0;
	}

	LRESULT onMoveDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
		int j = ctrlList.GetItemCount() - 2;
		for(int i = j; i >= 0; --i) {
			if(ctrlList.GetItemState(i, LVIS_SELECTED)) {
				ctrlList.moveItem(i, i+1);
				ctrlList.SelectItem(i+1); 
				ctrlList.SetFocus();
			}
		}
		bHandled = FALSE;
		return 0;
	}

	LRESULT onEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
		int i = -1;
		TCHAR buf[256];
		while( (i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			LineDlg hublist;
			hublist.title = CTSTRING(HUB_LIST);
			hublist.description = CTSTRING(HUB_LIST_EDIT);
			ctrlList.GetItemText(i, 0, buf, 256);
			hublist.line = tstring(buf);
			if(hublist.DoModal(m_hWnd) == IDOK) {
				ctrlList.SetItemText(i, 0, hublist.line.c_str());
			}
		}
		bHandled = FALSE;
		return 0;
	}

	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
		int i = -1;
		while( (i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			ctrlList.DeleteItem(i);
		}
		bHandled = FALSE;
		return 0;
	}

	LRESULT onCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(wID == IDOK) {
			TCHAR buf[512];
			string tmp;
			int j = ctrlList.GetItemCount();
			for(int i = 0; i < j; ++i) {
				ctrlList.GetItemText(i, 0, buf, 512);
				tmp += Text::fromT(buf) + ';';
			}
			if(j > 0) {
				tmp.erase(tmp.size()-1);
			}
			SettingsManager::getInstance()->set(SettingsManager::HUBLIST_SERVERS, tmp);
		}
		EndDialog(wID);
		return 0;
	}

	LRESULT onItemchangedDirectories(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
			NM_LISTVIEW* lv = (NM_LISTVIEW*) pnmh;
			::EnableWindow(GetDlgItem(IDC_LIST_UP), (lv->uNewState & LVIS_FOCUSED));
			::EnableWindow(GetDlgItem(IDC_LIST_DOWN), (lv->uNewState & LVIS_FOCUSED));
			::EnableWindow(GetDlgItem(IDC_LIST_EDIT), (lv->uNewState & LVIS_FOCUSED));
			::EnableWindow(GetDlgItem(IDC_LIST_REMOVE), (lv->uNewState & LVIS_FOCUSED));
		return 0;
	}

private:
	ExListViewCtrl ctrlList;
};

#endif // !defined(PUBLIC_HUBS_LIST_DLG_H)
