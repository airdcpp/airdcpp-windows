/* 
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
#include "../client/SettingsManager.h"
#include "Resource.h"

#include "SearchPage.h"
#include "WebShortcutsProperties.h"

PropPage::TextItem SearchPage::texts[] = {
	{ IDC_SEARH_SKIPLIST_PRESET,				ResourceManager::SETTINGS_SEARH_SKIPLIST_PRESET },
	{ IDC_PRE1,									ResourceManager::PRESET1 },
	{ IDC_PRE2,									ResourceManager::PRESET2 },
	{ IDC_PRE3,									ResourceManager::PRESET3 },
	{ IDC_WEB_SHORTCUTS_ADD,					ResourceManager::ADD						},
	{ IDC_WEB_SHORTCUTS_REMOVE,					ResourceManager::REMOVE						},
	{ IDC_WEB_SHORTCUTS_PROPERTIES,				ResourceManager::PROPERTIES					},
	{ IDC_SB_WEB_SHORTCUTS,						ResourceManager::SETTINGS_SB_WEB_SHORTCUTS	},
	{ IDC_INTERVAL_TEXT,						ResourceManager::MINIMUM_SEARCH_INTERVAL },
	{ IDC_SETTINGS_SEARCH_HISTORY,				ResourceManager::SETTINGS_SEARCH_HISTORY },
	{ IDC_SEARCHING_OPTIONS,					ResourceManager::SETTINGS_SEARCHING_OPTIONS	},
	{ IDC_EXPIRY_DAYS_LABEL,					ResourceManager::SETTINGS_EXPIRY_DAYS	},
	{ IDC_AUTO_SEARCH,							ResourceManager::AUTO_SEARCH },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item SearchPage::items[] = {
	{ IDC_SKIPLIST_PRESET1, SettingsManager::SKIP_MSG_01, PropPage::T_STR },
	{ IDC_SKIPLIST_PRESET2, SettingsManager::SKIP_MSG_02, PropPage::T_STR },
	{ IDC_SKIPLIST_PRESET3, SettingsManager::SKIP_MSG_03, PropPage::T_STR },
	{ IDC_INTERVAL, SettingsManager::MINIMUM_SEARCH_INTERVAL, PropPage::T_INT },
	{ IDC_SEARCH_HISTORY, SettingsManager::SEARCH_HISTORY, PropPage::T_INT }, 
	{ IDC_EXPIRY_DAYS, SettingsManager::AUTOSEARCH_EXPIRE_DAYS, PropPage::T_INT }, 
	{ 0, 0, PropPage::T_END }
};

#define setMinMax(x, y, z) \
	updown.Attach(GetDlgItem(x)); \
	updown.SetRange32(y, z); \
	updown.Detach();

LRESULT SearchPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);
	
	CUpDownCtrl updown;
	setMinMax(IDC_SEARCH_HISTORY_SPIN, 1, 100);
	setMinMax(IDC_INTERVAL_SPIN, 5, 9999);
	setMinMax(IDC_EXPIRY_SPIN, 0, 90);

	wsList = WebShortcuts::getInstance()->copyList();

	CRect rc;
	ctrlWebShortcuts.Attach(GetDlgItem(IDC_WEB_SHORTCUTS_LIST));
	ctrlWebShortcuts.GetClientRect(rc);
	rc.right -= GetSystemMetrics(SM_CXVSCROLL);
	ctrlWebShortcuts.InsertColumn(0, CTSTRING(SETTINGS_NAME), LVCFMT_LEFT, rc.Width() / 5, 0);
	ctrlWebShortcuts.InsertColumn(1, _T("Key"), LVCFMT_LEFT, rc.Width() / 5, 1);
	ctrlWebShortcuts.InsertColumn(2, _T("Url"), LVCFMT_LEFT, rc.Width() * 3 / 5, 2);
	ctrlWebShortcuts.InsertColumn(3, _T("Clean"), LVCFMT_LEFT, rc.Width() / 5, 3);
	ctrlWebShortcuts.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	for (WebShortcut::Iter i = wsList.begin(); i != wsList.end(); ++i) {
		WebShortcut* ws = *i;
		addListItem(ws);
	}


	return TRUE;
}
LRESULT SearchPage::onClickedShortcuts(WORD /* wNotifyCode */, WORD wID, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	if (wID == IDC_WEB_SHORTCUTS_ADD) {
		WebShortcut* ws;
		ws = new WebShortcut();
		WebShortcutsProperties wsp(wsList, ws);
		if (wsp.DoModal() == IDOK) {
			wsList.push_back(ws);
			addListItem(ws);
		} else {
			delete ws;
		}
	} else if (wID == IDC_WEB_SHORTCUTS_PROPERTIES) {
		if (ctrlWebShortcuts.GetSelectedCount() == 1) {
			int sel = ctrlWebShortcuts.GetSelectedIndex();
			WebShortcut* ws = wsList[sel];
			WebShortcutsProperties wsp(wsList, ws);
			if (wsp.DoModal() == IDOK) {
				updateListItem(sel);
			}
		}
	} else if (wID == IDC_WEB_SHORTCUTS_REMOVE) {
		if (ctrlWebShortcuts.GetSelectedCount() == 1) {
			int sel = ctrlWebShortcuts.GetSelectedIndex();
			dcassert(sel >= 0 && sel < (int)wsList.size());

			wsList.erase(find(wsList.begin(), wsList.end(), wsList[sel]));
			ctrlWebShortcuts.DeleteItem(sel);
		}
	}
	return S_OK;
}
LRESULT SearchPage::onSelChangeShortcuts(WORD /* wNotifyCode */, WORD /* wID */, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	CButton cbnProp, cbnRemove;
	cbnProp.Attach(GetDlgItem(IDC_WEB_SHORTCUTS_PROPERTIES));
	cbnRemove.Attach(GetDlgItem(IDC_WEB_SHORTCUTS_REMOVE));

	dcassert(ctrlWebShortcuts.IsWindow());
	if (ctrlWebShortcuts.GetSelectedCount() == 1) {
		cbnProp.EnableWindow(TRUE);
		cbnRemove.EnableWindow(TRUE);
	} else {
		cbnProp.EnableWindow(FALSE);
		cbnRemove.EnableWindow(FALSE);
	}
	return S_OK;
}
LRESULT SearchPage::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	switch(kd->wVKey) {
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

LRESULT SearchPage::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

	if(item->iItem >= 0) {
		PostMessage(WM_COMMAND, IDC_WEB_SHORTCUTS_PROPERTIES, 0);
	} else if(item->iItem == -1) {
		PostMessage(WM_COMMAND, IDC_WEB_SHORTCUTS_ADD, 0);
	}

	return 0;
}


void SearchPage::write() {
	WebShortcuts::getInstance()->replaceList(wsList);

	PropPage::write((HWND)*this, items);
}
