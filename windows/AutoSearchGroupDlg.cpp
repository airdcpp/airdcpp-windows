
/*
* Copyright (C) 2011-2015 AirDC++ Project
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
#include "../client/AutoSearchManager.h"

#include "WinUtil.h"
#include "AutoSearchGroupDlg.h"

LRESULT AsGroupsDlg::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
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

	ctrlGroups.InsertColumn(0, CTSTRING(NAME), LVCFMT_LEFT, WinUtil::percent(width, 100), 0);
	ctrlGroups.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	const auto groups = AutoSearchManager::getInstance()->getGroups();
	for (auto i : groups) {
		addItem(Text::toT(i));
	}
	updateSelectedGroup(true);
	return 0;
}

LRESULT AsGroupsDlg::onClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	save();
	EndDialog(FALSE);
	return 0;
}

LRESULT AsGroupsDlg::onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
	updateSelectedGroup();
	return 0;
}

void AsGroupsDlg::save() {
	vector<string> groups;

	for (int i = 0; i < ctrlGroups.GetItemCount(); ++i) {
		groups.emplace_back(Text::fromT(getText(0, i)));
	}
	AutoSearchManager::getInstance()->setGroups(groups);
	AutoSearchManager::getInstance()->AutoSearchSave();
}

int AsGroupsDlg::findGroup(LPCTSTR name) {
	TCHAR buf[4096] = { 0 };
	for (int i = 0; i < ctrlGroups.GetItemCount(); ++i) {
		ctrlGroups.GetItemText(i, 0, buf, 4096);
		if (wcscmp(buf, name) == 0) {
			return i;
		}
	}
	return -1;
}

void AsGroupsDlg::addItem(const tstring& name, bool select /*= false*/) {
	int32_t item = ctrlGroups.InsertItem(ctrlGroups.GetItemCount(), name.c_str());
	if (select)
		ctrlGroups.SelectItem(item);
}

bool AsGroupsDlg::getItem(tstring& name, bool checkSel) {
	{
		CEdit wnd;
		wnd.Attach(GetDlgItem(IDC_NAME));
		name = WinUtil::getEditText(wnd);
		wnd.Detach();
		if (name.empty()) {
			MessageBox(_T("You must enter a group name!"), CTSTRING(MANAGE_GROUPS), MB_ICONERROR);
			return false;
		}
		else {
			int32_t pos = findGroup(name.c_str());
			if (pos != -1 && (checkSel == false || pos != ctrlGroups.GetSelectedIndex())) {
				MessageBox(_T("Item already exists!"), CTSTRING(MANAGE_GROUPS), MB_ICONERROR);
				return false;
			}
		}
	}
	return true;
}

tstring AsGroupsDlg::getText(int column, int item /*= -1*/) const {
	tstring buf;
	int32_t selection = item == -1 ? ctrlGroups.GetSelectedIndex() : item;
	if (selection >= 0) {
		buf.resize(4096);
		buf.resize(ctrlGroups.GetItemText(selection, column, &buf[0], buf.size()));
	}
	return buf;
}

void AsGroupsDlg::updateSelectedGroup(bool forceClean /*= false*/) {
	tstring name;
	bool enableButtons = false;

	if (ctrlGroups.GetSelectedIndex() != -1) {
		if (forceClean == false) {
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

LRESULT AsGroupsDlg::onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring name;
	if (getItem(name, false)) {
		addItem(name, true);
		updateSelectedGroup(true);
	}
	return 0;
}

LRESULT AsGroupsDlg::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int32_t pos = ctrlGroups.GetSelectedIndex();
	if (pos < 0)
		return 0;

	AutoSearchList removeLst;
	{
		tstring name = getText(0, pos);
		WLock l(AutoSearchManager::getInstance()->getCS());
		auto lst = AutoSearchManager::getInstance()->getSearchItems();
		tstring msg = _T("Remove all items in the group aswell?");
		bool remove = MessageBox(msg.c_str(), _T("Remove Group"), MB_ICONQUESTION | MB_YESNO) == IDYES;

		for (auto as : lst | map_values) {
			if (as->getGroup() != Text::fromT(name))
				continue;
			if (remove)
				removeLst.push_back(as);
			else
				as->setGroup(Util::emptyString);
		}
		ctrlGroups.DeleteItem(pos);
		updateSelectedGroup(true);
	}

	for_each(removeLst, [&](AutoSearchPtr a) { AutoSearchManager::getInstance()->removeAutoSearch(a); });
	return 0;
}

LRESULT AsGroupsDlg::onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int32_t item = ctrlGroups.GetSelectedIndex();
	if (item >= 0) {
		tstring name;
		tstring oldName = getText(0);
		if (getItem(name, true)) {
			if (oldName == name)
				return 0;
			{
				WLock l(AutoSearchManager::getInstance()->getCS());
				auto lst = AutoSearchManager::getInstance()->getSearchItems();

				for (auto as : lst | map_values) {
					if (as->getGroup() == Text::fromT(oldName))
						as->setGroup(Text::fromT(name));
				}
			}
			ctrlGroups.DeleteItem(item);
			addItem(name, true);
			updateSelectedGroup();
		}
	}
	return 0;
}