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
#include "../client/DCPlusPlus.h"
#include "../client/FavoriteManager.h"

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

	ctrlGroups.InsertColumn(0, CTSTRING(NAME), LVCFMT_LEFT, WinUtil::percent(width, 70), 0);
	ctrlGroups.InsertColumn(1, _T("Private"), LVCFMT_LEFT, WinUtil::percent(width, 15), 0);
	ctrlGroups.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	const FavHubGroups& groups = FavoriteManager::getInstance()->getFavHubGroups();
	for(FavHubGroups::const_iterator i = groups.begin(); i != groups.end(); ++i) {
		addItem(Text::toT(i->first), i->second.priv);
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
	FavHubGroupProperties group;

	for(int i = 0; i < ctrlGroups.GetItemCount(); ++i) {
		//@todo: fixme
		/*group.first = Text::fromT(getText(0, i));
		group.second.priv = getText(1, i) == _T("Yes");
		group.second.connect = getText(2, i) == _T("Yes");
		groups.insert(group);*/
		name = Text::fromT(getText(0, i));
		group.priv = getText(1, i) == TSTRING(YES);
		groups.insert(make_pair(name, group));
	}
	FavoriteManager::getInstance()->setFavHubGroups(groups);
	FavoriteManager::getInstance()->save();
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

void FavHubGroupsDlg::addItem(const tstring& name, bool priv, bool select /*= false*/) {
	int32_t item = ctrlGroups.InsertItem(ctrlGroups.GetItemCount(), name.c_str());
	ctrlGroups.SetItemText(item, 1, priv ? CTSTRING(YES) : CTSTRING(NO));
	if(select)
		ctrlGroups.SelectItem(item);
}

bool FavHubGroupsDlg::getItem(tstring& name, bool& priv, bool checkSel) {
	{
		name.resize(4096);
		CEdit wnd;
		wnd.Attach(GetDlgItem(IDC_NAME));
		name.resize(wnd.GetWindowText(&name[0], name.size()));
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
	{
		CButton wnd;
		wnd.Attach(GetDlgItem(IDC_PRIVATE));
		priv = wnd.GetCheck() == 1;
		wnd.Detach();
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
	bool priv = false;
	bool enableButtons = false;

	if(ctrlGroups.GetSelectedIndex() != -1) {
		if(forceClean == false) {
			name = getText(0);
			priv = getText(1) == TSTRING(YES);
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
	{
		CButton wnd;
		wnd.Attach(GetDlgItem(IDC_PRIVATE));
		wnd.SetCheck(priv ? 1 : 0);
		wnd.Detach();
	}
}

LRESULT FavHubGroupsDlg::onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring name;
	bool priv;
	if(getItem(name, priv, false)) {
		addItem(name, priv, true);
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
			tstring msg;
			msg += _T("The group ");
			msg += name;
			msg += _T(" contains ");
			msg += Util::toStringW(l.size());
			msg += _T(" hubs; remove all of the hubs as well?\n\n"
				_T("If you select 'Yes', all of these hubs are going to be deleted!\n\n")
				_T("If you select 'No', these hubs will simply be moved to the main default group."));
			bool remove = MessageBox(msg.c_str(), _T("Remove Group"), MB_ICONQUESTION | MB_YESNO) == IDYES;
			for(FavoriteHubEntryList::iterator i = l.begin(); i != l.end(); ++i) {
				if(remove)
					FavoriteManager::getInstance()->removeFavorite(*i);
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
		bool priv;
		if(getItem(name, priv, true)) {
			if(oldName != name) {
				FavoriteHubEntryList l = FavoriteManager::getInstance()->getFavoriteHubs(Text::fromT(oldName));
				for(FavoriteHubEntryList::iterator i = l.begin(); i != l.end(); ++i) {
					(*i)->setGroup(Text::fromT(name));
				}
			}
			ctrlGroups.DeleteItem(item);
			addItem(name, priv, true);
			updateSelectedGroup();
		}
	}
	return 0;
}
