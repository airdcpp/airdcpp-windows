/*
* Copyright (C) 2011-2019 AirDC++ Project
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

#include "WebShortCutsPage.h"
#include "WebShortcutsProperties.h"

PropPage::TextItem WebShortCutsPage::texts[] = {
	{ IDC_WEB_SHORTCUTS_ADD,					ResourceManager::ADD },
	{ IDC_WEB_SHORTCUTS_REMOVE,					ResourceManager::REMOVE },
	{ IDC_WEB_SHORTCUTS_PROPERTIES,				ResourceManager::PROPERTIES },
	{ IDC_SB_WEB_SHORTCUTS,						ResourceManager::SETTINGS_SB_WEB_SHORTCUTS },
	{ 0, ResourceManager::LAST }
};

PropPage::Item WebShortCutsPage::items[] = {
	{ 0, 0, PropPage::T_END }
};

LRESULT WebShortCutsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	wsList = WebShortcuts::getInstance()->copyList();

	CRect rc;
	ctrlWebShortcuts.Attach(GetDlgItem(IDC_WEB_SHORTCUTS_LIST));
	ctrlWebShortcuts.GetClientRect(rc);
	ctrlWebShortcuts.InsertColumn(0, CTSTRING(NAME), LVCFMT_LEFT, rc.Width() / 10 *2, 0);
	ctrlWebShortcuts.InsertColumn(1, CTSTRING(KEY), LVCFMT_LEFT, rc.Width() / 10*2, 1);
	ctrlWebShortcuts.InsertColumn(2, CTSTRING(PROPPAGE_URL), LVCFMT_LEFT, rc.Width() / 10 *5, 2);
	ctrlWebShortcuts.InsertColumn(3, CTSTRING(CLEAN), LVCFMT_LEFT, rc.Width() / 10, 3);
	ctrlWebShortcuts.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	for (auto ws : wsList) {
		addListItem(ws);
	}


	return TRUE;
}
LRESULT WebShortCutsPage::onClickedShortcuts(WORD /* wNotifyCode */, WORD wID, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	if (wID == IDC_WEB_SHORTCUTS_ADD) {
		WebShortcut* ws;
		ws = new WebShortcut();
		WebShortcutsProperties wsp(wsList, ws);
		if (wsp.DoModal() == IDOK) {
			wsList.push_back(ws);
			addListItem(ws);
		}
		else {
			delete ws;
		}
	}
	else if (wID == IDC_WEB_SHORTCUTS_PROPERTIES) {
		if (ctrlWebShortcuts.GetSelectedCount() == 1) {
			int sel = ctrlWebShortcuts.GetSelectedIndex();
			WebShortcut* ws = wsList[sel];
			WebShortcutsProperties wsp(wsList, ws);
			if (wsp.DoModal() == IDOK) {
				updateListItem(sel);
			}
		}
	}
	else if (wID == IDC_WEB_SHORTCUTS_REMOVE) {
		if (ctrlWebShortcuts.GetSelectedCount() == 1) {
			int sel = ctrlWebShortcuts.GetSelectedIndex();
			dcassert(sel >= 0 && sel < (int)wsList.size());

			wsList.erase(find(wsList.begin(), wsList.end(), wsList[sel]));
			ctrlWebShortcuts.DeleteItem(sel);
		}
	}
	return S_OK;
}
LRESULT WebShortCutsPage::onSelChangeShortcuts(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	CButton cbnProp, cbnRemove;
	cbnProp.Attach(GetDlgItem(IDC_WEB_SHORTCUTS_PROPERTIES));
	cbnRemove.Attach(GetDlgItem(IDC_WEB_SHORTCUTS_REMOVE));

	dcassert(ctrlWebShortcuts.IsWindow());
	if (ctrlWebShortcuts.GetSelectedCount() == 1) {
		cbnProp.EnableWindow(TRUE);
		cbnRemove.EnableWindow(TRUE);
	}
	else {
		cbnProp.EnableWindow(FALSE);
		cbnRemove.EnableWindow(FALSE);
	}
	return S_OK;
}
LRESULT WebShortCutsPage::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*)pnmh;
	switch (kd->wVKey) {
	case VK_INSERT:
		PostMessage(WM_COMMAND, IDC_WEB_SHORTCUTS_ADD, 0);
		break;
	case VK_DELETE:
		PostMessage(WM_COMMAND, IDC_WEB_SHORTCUTS_REMOVE, 0);
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}

LRESULT WebShortCutsPage::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

	if (item->iItem >= 0) {
		PostMessage(WM_COMMAND, IDC_WEB_SHORTCUTS_PROPERTIES, 0);
	}
	else if (item->iItem == -1) {
		PostMessage(WM_COMMAND, IDC_WEB_SHORTCUTS_ADD, 0);
	}

	return 0;
}


void WebShortCutsPage::write() {
	WebShortcuts::getInstance()->replaceList(wsList);

	PropPage::write((HWND)*this, items);
}

void WebShortCutsPage::updateListItem(int pos) {
	dcassert(pos >= 0 && (unsigned int)pos < wsList.size());
	ctrlWebShortcuts.SetItemText(pos, 0, Text::toT(wsList[pos]->name).c_str());
	ctrlWebShortcuts.SetItemText(pos, 1, Text::toT(wsList[pos]->key).c_str());
	ctrlWebShortcuts.SetItemText(pos, 2, Text::toT(wsList[pos]->url).c_str());
}

void WebShortCutsPage::addListItem(WebShortcut* ws) {
	TStringList cols;
	cols.push_back(Text::toT(ws->name));
	cols.push_back(Text::toT(ws->key));
	cols.push_back(Text::toT(ws->url));
	cols.push_back(ws->clean ? TSTRING(YES) : TSTRING(NO));
	ctrlWebShortcuts.insert(cols);
	cols.clear();
}

WebShortCutsPage::~WebShortCutsPage() {
	for (auto i : wsList)
		delete i;

	ctrlWebShortcuts.Detach();
	free(title);
};
