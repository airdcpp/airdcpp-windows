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

#include "stdafx.h"
#include "Resource.h"

#include "IgnorePage.h"
#include "../client/version.h"
#include "WinUtil.h"
#include "IgnoreDlg.h"

PropPage::TextItem IgnorePage::texts[] = {
	{ IDC_IGNORE_ADD, ResourceManager::ADD },
	{ IDC_IGNORE_REMOVE, ResourceManager::REMOVE },
	{ IDC_IGNORE_EDIT, ResourceManager::SETTINGS_CHANGE },
	{ IDC_IGNORE_CLEAR, ResourceManager::CLEAR },
	{ IDC_MISC_IGNORE, ResourceManager::IGNORED_USERS },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT IgnorePage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);

	CRect rc;
	
	ignoreListCtrl.Attach(GetDlgItem(IDC_IGNORELIST));
	ignoreListCtrl.GetClientRect(rc);
	ignoreListCtrl.InsertColumn(0, CTSTRING(IGNORE_NICK_MATCH), LVCFMT_LEFT, rc.Width() / 3, 1);
	ignoreListCtrl.InsertColumn(1, CTSTRING(IGNORE_TEXT_MATCH), LVCFMT_LEFT, rc.Width() / 3, 1);
	ignoreListCtrl.InsertColumn(2, CTSTRING(MAIN_CHAT), LVCFMT_LEFT, rc.Width() / 3 / 2, 1);
	ignoreListCtrl.InsertColumn(3, CTSTRING(PRIVATE_CHAT), LVCFMT_LEFT, rc.Width() / 3 / 2, 1);
	ignoreListCtrl.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	ignoreItems = IgnoreManager::getInstance()->getIgnoreList();
	for(auto& def: ignoreItems) {
		int p = ignoreListCtrl.insert(ignoreListCtrl.GetItemCount(), Text::toT(def.getNickPattern()));
		ignoreListCtrl.SetItemText(p, 1, Text::toT(def.getTextPattern()).c_str());
		ignoreListCtrl.SetItemText(p, 2, def.matchMainchat ? CTSTRING(YES) : CTSTRING(NO));
		ignoreListCtrl.SetItemText(p, 3, def.matchPM ? CTSTRING(YES) : CTSTRING(NO));
	}

	return TRUE;
}

LRESULT IgnorePage::onItemchangedDirectories(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
	bool hasSel = ignoreListCtrl.GetSelectedCount() > 0;
	::EnableWindow(GetDlgItem(IDC_IGNORE_EDIT), hasSel);
	::EnableWindow(GetDlgItem(IDC_IGNORE_REMOVE), hasSel);
	return 0;
}

LRESULT IgnorePage::onIgnoreAdd(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	IgnoreDlg dlg;
	if(dlg.DoModal() == IDOK) {
		auto k = addIgnore(dlg.nick, dlg.text, dlg.nickMethod, dlg.textMethod, dlg.MC, dlg.PM);
		if(k) {
			int p = ignoreListCtrl.insert(ignoreListCtrl.GetItemCount(), Text::toT(dlg.nick));
			ignoreListCtrl.SetItemText(p, 1, Text::toT(dlg.text).c_str());
			ignoreListCtrl.SetItemText(p, 2, dlg.MC ? CTSTRING(YES) : CTSTRING(NO));
			ignoreListCtrl.SetItemText(p, 3, dlg.PM ? CTSTRING(YES) : CTSTRING(NO));
		} else {
			WinUtil::showMessageBox(TSTRING(ALREADY_IGNORED), MB_OK);
		}
	}
	return 0;
}
LRESULT IgnorePage::onIgnoreEdit(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	int sel = ignoreListCtrl.GetSelectedIndex();
	if (sel == -1)
		return 0;

	auto olditem = getIgnore(sel);
	IgnoreDlg dlg(olditem.getNickPattern(), olditem.getTextPattern(), olditem.getNickMethod(), olditem.getTextMethod(), olditem.matchMainchat, olditem.matchPM);
	if (dlg.DoModal() == IDOK) {
		auto newitem = IgnoreItem(dlg.nick, dlg.text, dlg.nickMethod, dlg.textMethod, dlg.MC, dlg.PM);
		updateIgnore(newitem, sel);
		ignoreListCtrl.SetItemText(sel, 0, Text::toT(dlg.nick).c_str());
		ignoreListCtrl.SetItemText(sel, 1, Text::toT(dlg.text).c_str());
		ignoreListCtrl.SetItemText(sel, 2, dlg.MC ? CTSTRING(YES) : CTSTRING(NO));
		ignoreListCtrl.SetItemText(sel, 3, dlg.PM ? CTSTRING(YES) : CTSTRING(NO));
	}

	return 0;
}

LRESULT IgnorePage::onIgnoreRemove(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	int i = -1;
	
	TCHAR buf[256];

	while((i = ignoreListCtrl.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		ignoreListCtrl.GetItemText(i, 0, buf, 256);

		removeIgnore(i);
		ignoreListCtrl.DeleteItem(i);
	}

	return 0;
}

LRESULT IgnorePage::onIgnoreClear(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	ignoreListCtrl.DeleteAllItems();
	clearIgnores();
	return 0;
}

bool IgnorePage::addIgnore(const string& aNick, const string& aText, StringMatch::Method aNickMethod, StringMatch::Method aTextMethod, bool aMainChat, bool aPM) {
	auto i = find_if(ignoreItems.begin(), ignoreItems.end(), [aNick, aText](const IgnoreItem& s) {
		return ((s.getNickPattern().empty() || compare(s.getNickPattern(), aNick) == 0) && (s.getTextPattern().empty() || compare(s.getTextPattern(), aText) == 0));
	});
	if (i == ignoreItems.end()){
		ignoreItems.push_back(IgnoreItem(aNick, aText, aNickMethod, aTextMethod, aMainChat, aPM));
		return true;
	}
	return false;
}

void IgnorePage::removeIgnore(int pos) {
	ignoreItems.erase(ignoreItems.begin() + pos);
}

void IgnorePage::write() {
	IgnoreManager::getInstance()->replaceList(ignoreItems);
}