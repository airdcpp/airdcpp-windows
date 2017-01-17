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

#include "stdafx.h"
#include <airdcpp/FavoriteManager.h>

#include "FavHubGroupsDlg.h"
#include "WinUtil.h"

LRESULT FavHubGroupsDlg::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlGroups.Attach(GetDlgItem(IDC_GROUPS));

	uint32_t width;
	{
		CRect rc;
		ctrlGroups.GetClientRect(rc);
		width = rc.Width() - 20; // for scroll
	}

	// Translate dialog
	SetWindowText(CTSTRING(MANAGE_GROUPS));
	SetDlgItemText(IDC_ADD, CTSTRING(ADD));
	SetDlgItemText(IDC_REMOVE, CTSTRING(REMOVE));
	SetDlgItemText(IDC_UPDATE, CTSTRING(UPDATE));
	SetDlgItemText(IDCANCEL, CTSTRING(CLOSE));
	SetDlgItemText(IDC_NAME_STATIC, CTSTRING(NAME));

	GetDlgItem(IDC_MOVE_DOWN).ShowWindow(FALSE);
	GetDlgItem(IDC_MOVE_UP).ShowWindow(FALSE);

	ctrlGroups.InsertColumn(0, CTSTRING(NAME), LVCFMT_LEFT, WinUtil::percent(width, 100), 0);
	ctrlGroups.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	const FavHubGroups& groups = FavoriteManager::getInstance()->getFavHubGroups();
	for(FavHubGroups::const_iterator i = groups.begin(); i != groups.end(); ++i) {
		addItem(Text::toT(i->first));
	}
	updateSelectedGroup(true);
	return 0;
}

LRESULT FavHubGroupsDlg::onClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	save();
	EndDialog(FALSE);
	return 0;
}

LRESULT FavHubGroupsDlg::onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
	updateSelectedGroup();
	return 0;
}

void FavHubGroupsDlg::save() {
	FavHubGroups groups;
	string name;
	//FavHubGroupProperties group;

	HubSettings settings;
	for(int i = 0; i < ctrlGroups.GetItemCount(); ++i) {
		name = Text::fromT(getText(0, i));
		groups.emplace(name, settings);
	}
	FavoriteManager::getInstance()->setFavHubGroups(groups);
	FavoriteManager::getInstance()->setDirty();
}

int FavHubGroupsDlg::findGroup(LPCTSTR name) {
	TCHAR buf[4096] = { 0 };
	for(int i = 0; i < ctrlGroups.GetItemCount(); ++i) {
		ctrlGroups.GetItemText(i, 0, buf, 4096);
		if(wcscmp(buf, name) == 0) {
			return i;
		}
	}
	return -1;
}

void FavHubGroupsDlg::addItem(const tstring& name, bool select /*= false*/) {
	int32_t item = ctrlGroups.InsertItem(ctrlGroups.GetItemCount(), name.c_str());
	if(select)
		ctrlGroups.SelectItem(item);
}

bool FavHubGroupsDlg::getItem(tstring& name, bool checkSel) {
	{
		CEdit wnd;
		wnd.Attach(GetDlgItem(IDC_NAME));
		name = WinUtil::getEditText(wnd);
		wnd.Detach();
		if(name.empty()) {
			MessageBox(_T("You must enter a group name!"), CTSTRING(MANAGE_GROUPS), MB_ICONERROR);
			return false;
		} else {
			int32_t pos = findGroup(name.c_str());
			if(pos != -1 && (checkSel == false || pos != ctrlGroups.GetSelectedIndex())) {
				MessageBox(_T("Item already exists!"), CTSTRING(MANAGE_GROUPS), MB_ICONERROR);
				return false;
			}
		}
	}
	return true;
}

tstring FavHubGroupsDlg::getText(int column, int item /*= -1*/) const {
	tstring buf;
	int32_t selection = item == -1 ? ctrlGroups.GetSelectedIndex() : item;
	if(selection >= 0) {
		buf.resize(4096);
		buf.resize(ctrlGroups.GetItemText(selection, column, &buf[0], buf.size()));
	}
	return buf;
}

void FavHubGroupsDlg::updateSelectedGroup(bool forceClean /*= false*/) {
	tstring name;
	bool enableButtons = false;

	if(ctrlGroups.GetSelectedIndex() != -1) {
		if(forceClean == false) {
			name = getText(0);
		}
		enableButtons = true;
	}

	{
		CWindow wnd;
		wnd.Attach(GetDlgItem(IDC_REMOVE));
		wnd.EnableWindow(enableButtons);
		wnd.Detach();

		wnd.Attach(GetDlgItem(IDC_UPDATE));
		wnd.EnableWindow(enableButtons);
		wnd.Detach();
	}
	{
		CEdit wnd;
		wnd.Attach(GetDlgItem(IDC_NAME));
		wnd.SetWindowText(name.c_str());
		wnd.Detach();
	}
}

LRESULT FavHubGroupsDlg::onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring name;
	if(getItem(name, false)) {
		addItem(name, true);
		updateSelectedGroup(true);
	}
	return 0;
}

LRESULT FavHubGroupsDlg::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int32_t pos = ctrlGroups.GetSelectedIndex();
	if(pos >= 0) {
		tstring name = getText(0, pos);
		FavoriteHubEntryList l = FavoriteManager::getInstance()->getFavoriteHubs(Text::fromT(name));
		if(l.size() > 0) {
			bool remove = MessageBox(CTSTRING_F(CONFIRM_REMOVE_HUBS_FAV_GROUP, name % l.size()), CTSTRING(REMOVE), MB_ICONQUESTION | MB_YESNO) == IDYES;
			for(FavoriteHubEntryList::iterator i = l.begin(); i != l.end(); ++i) {
				if(remove)
					FavoriteManager::getInstance()->removeFavoriteHub((*i)->getToken());
				else
					(*i)->setGroup(Util::emptyString);
			}
		}
		ctrlGroups.DeleteItem(pos);
		updateSelectedGroup(true);
	}
	return 0;
}

LRESULT FavHubGroupsDlg::onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int32_t item = ctrlGroups.GetSelectedIndex();
	if(item >= 0) {
		tstring name;
		tstring oldName = getText(0);
		if(getItem(name, true)) {
			if(oldName != name) {
				FavoriteHubEntryList l = FavoriteManager::getInstance()->getFavoriteHubs(Text::fromT(oldName));
				for(FavoriteHubEntryList::iterator i = l.begin(); i != l.end(); ++i) {
					(*i)->setGroup(Text::fromT(name));
				}
			}
			ctrlGroups.DeleteItem(item);
			addItem(name, true);
			updateSelectedGroup();
		}
	}
	return 0;
}
