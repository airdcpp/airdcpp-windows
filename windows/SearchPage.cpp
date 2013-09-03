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
#include "Resource.h"

#include "SearchPage.h"
#include "WebShortcutsProperties.h"

PropPage::TextItem SearchPage::texts[] = {
	{ IDC_WEB_SHORTCUTS_ADD,					ResourceManager::ADD						},
	{ IDC_WEB_SHORTCUTS_REMOVE,					ResourceManager::REMOVE						},
	{ IDC_WEB_SHORTCUTS_PROPERTIES,				ResourceManager::PROPERTIES					},
	{ IDC_SB_WEB_SHORTCUTS,						ResourceManager::SETTINGS_SB_WEB_SHORTCUTS	},
	{ IDC_INTERVAL_TEXT,						ResourceManager::MINIMUM_SEARCH_INTERVAL },
	{ IDC_SEARCHING_OPTIONS,					ResourceManager::SETTINGS_SEARCHING_OPTIONS	},
	{ IDC_EXPIRY_DAYS_LABEL,					ResourceManager::SETTINGS_EXPIRY_DAYS	},
	{ IDC_DELAY_HOURS_LABEL,					ResourceManager::SETTINGS_DELAY_HOURS	},
	{ IDC_REMOVE_EXPIRED_AS,					ResourceManager::REMOVE_EXPIRED_AS },
	{ IDC_AUTO_SEARCH,							ResourceManager::AUTO_SEARCH },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item SearchPage::items[] = {
	{ IDC_INTERVAL, SettingsManager::MINIMUM_SEARCH_INTERVAL, PropPage::T_INT },
	{ IDC_EXPIRY_DAYS, SettingsManager::AUTOSEARCH_EXPIRE_DAYS, PropPage::T_INT }, 
	{ IDC_DELAY_HOURS, SettingsManager::AS_DELAY_HOURS, PropPage::T_INT }, 
	{ IDC_REMOVE_EXPIRED_AS, SettingsManager::REMOVE_EXPIRED_AS, PropPage::T_BOOL }, 
	{ 0, 0, PropPage::T_END }
};

LRESULT SearchPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);
	
	setMinMax(IDC_INTERVAL_SPIN, 5, 9999);
	setMinMax(IDC_EXPIRY_SPIN, 0, 90);
	setMinMax(IDC_DELAY_SPIN, 0, 999);

	wsList = WebShortcuts::getInstance()->copyList();

	CRect rc;
	ctrlWebShortcuts.Attach(GetDlgItem(IDC_WEB_SHORTCUTS_LIST));
	ctrlWebShortcuts.GetClientRect(rc);
	rc.right -= GetSystemMetrics(SM_CXVSCROLL);
	ctrlWebShortcuts.InsertColumn(0, CTSTRING(SETTINGS_NAME), LVCFMT_LEFT, rc.Width() / 5, 0);
	ctrlWebShortcuts.InsertColumn(1, CTSTRING(KEY), LVCFMT_LEFT, rc.Width() / 5, 1);
	ctrlWebShortcuts.InsertColumn(2, CTSTRING(PROPPAGE_URL), LVCFMT_LEFT, rc.Width() * 3 / 5, 2);
	ctrlWebShortcuts.InsertColumn(3, CTSTRING(CLEAN), LVCFMT_LEFT, rc.Width() / 5, 3);
	ctrlWebShortcuts.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	for (auto ws: wsList) {
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

void SearchPage::updateListItem(int pos) {
	dcassert(pos >= 0 && (unsigned int) pos < wsList.size());
	ctrlWebShortcuts.SetItemText(pos, 0, Text::toT(wsList[pos]->name).c_str());
	ctrlWebShortcuts.SetItemText(pos, 1, Text::toT(wsList[pos]->key).c_str());
	ctrlWebShortcuts.SetItemText(pos, 2, Text::toT(wsList[pos]->url).c_str());
}

void SearchPage::addListItem(WebShortcut* ws) {
	TStringList cols;
	cols.push_back(Text::toT(ws->name));
	cols.push_back(Text::toT(ws->key));
	cols.push_back(Text::toT(ws->url));
	cols.push_back(ws->clean ? TSTRING(YES) : TSTRING(NO));
	ctrlWebShortcuts.insert(cols);
	cols.clear();
}

SearchPage::~SearchPage() {
	for (auto i : wsList)
		delete i;

	ctrlWebShortcuts.Detach();
	free(title);
};
