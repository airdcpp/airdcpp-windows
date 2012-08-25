/* 
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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
#include "../client/SettingsManager.h"
#include "../client/FavoriteManager.h"
#include "../client/SearchManager.h"
#include "../client/version.h"

#include "Resource.h"
#include "SearchTypesPage.h"
#include "SearchTypeDlg.h"
#include "CommandDlg.h"
#include "WinUtil.h"

PropPage::TextItem SearchTypesPage::texts[] = {
	{ IDC_ADD_MENU, ResourceManager::ADD },
	{ IDC_CHANGE_MENU, ResourceManager::SETTINGS_CHANGE },
	{ IDC_REMOVE_MENU, ResourceManager::REMOVE },
	{ IDC_RESET_DEFAULTS, ResourceManager::RESET_TO_DEFAULTS },
	{ IDC_SEARCHTYPES_NOTE, ResourceManager::SEARCH_TYPES_NOTE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item SearchTypesPage::items[] = {
	{ 0, 0, PropPage::T_END }
};

LRESULT SearchTypesPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	CRect rc;

	ctrlTypes.Attach(GetDlgItem(IDC_MENU_ITEMS));
	ctrlTypes.GetClientRect(rc);

	ctrlTypes.InsertColumn(0, CTSTRING(SETTINGS_NAME), LVCFMT_LEFT, rc.Width()/4, 0);
	ctrlTypes.InsertColumn(1, CTSTRING(SETTINGS_EXTENSIONS), LVCFMT_LEFT, rc.Width()*2 / 4, 1);
	ctrlTypes.InsertColumn(2, CTSTRING(DEFAULT), LVCFMT_LEFT, rc.Width() / 4, 2);
	ctrlTypes.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	fillList();
	
	return TRUE;
}

void SearchTypesPage::fillList() {
	ctrlTypes.DeleteAllItems();

	auto lst = SearchManager::getInstance()->getSearchTypes();

	int pos = 0;
	for(auto i = lst.begin(); i != lst.end(); ++i) {
		auto isDefault = SearchManager::isDefaultTypeStr(i->first);

		TStringList lst;
		lst.push_back(Text::toT(isDefault ? SearchManager::getTypeStr(i->first[0] - '0') : i->first));
		lst.push_back(Text::toT(Util::toString(";", i->second)));
		lst.push_back(isDefault ? _T("Yes") : _T("No"));
		ctrlTypes.insert(pos++, lst, 0);
	}
}

LRESULT SearchTypesPage::onAddMenu(WORD , WORD , HWND , BOOL& ) {
	SearchTypeDlg dlg;

	if(dlg.DoModal() == IDOK) {
		try {
			SearchManager::getInstance()->addSearchType(dlg.name, dlg.extList, true);
			fillList();
			//addRow(name, false, values);
			//types->resort();
		} catch(const SearchTypeException& e) {
			showError(e.getError());
		}
	}
	return 0;
}

LRESULT SearchTypesPage::onChangeMenu(WORD , WORD , HWND , BOOL& ) {
	if(ctrlTypes.GetSelectedCount() == 1) {
		int sel = ctrlTypes.GetSelectedIndex();
		auto lst = SearchManager::getInstance()->getSearchTypes();

		auto pos = lst.begin();
		advance(pos, sel);

		SearchTypeDlg dlg;
		auto isDefault = SearchManager::isDefaultTypeStr(pos->first);

		dlg.name = isDefault ? SearchManager::getTypeStr(pos->first[0] - '0') : pos->first;
		dlg.extList = pos->second;
		dlg.isDefault = isDefault;

		if(dlg.DoModal() == IDOK) {
			pos->second = dlg.extList;
			if (pos->first != dlg.name) {
				try {
					SearchManager::getInstance()->renameSearchType(pos->first, dlg.name);
				} catch(const SearchTypeException& e) {
					showError(e.getError());
				}
				fillList();
			} else {
				ctrlTypes.SetItemText(sel, 1, Text::toT(Util::toString(";", dlg.extList)).c_str());
			}
		}
	}
	return 0;
}

LRESULT SearchTypesPage::onRemoveMenu(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlTypes.GetSelectedCount() == 1) {
		int sel = ctrlTypes.GetNextItem(-1, LVNI_SELECTED);
		auto lst = SearchManager::getInstance()->getSearchTypes();

		auto pos = lst.begin();
		advance(pos, sel);

		if (SearchManager::isDefaultTypeStr(pos->first)) {
			return 0;
		}

		//FavoriteManager::getInstance()->removeUserCommand(ctrlTypes.GetItemData(i));
		try {
			SearchManager::getInstance()->delSearchType(pos->first);
			ctrlTypes.DeleteItem(sel);
		} catch(const SearchTypeException& e) {
			showError(e.getError());
		}
	}
	return 0;
}

LRESULT SearchTypesPage::onSelectionChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	NM_LISTVIEW* lv = (NM_LISTVIEW*) pnmh;
	if (lv->uNewState & LVIS_FOCUSED) {
		int sel = ctrlTypes.GetNextItem(-1, LVNI_SELECTED);
		auto lst = SearchManager::getInstance()->getSearchTypes();

		auto pos = lst.begin();
		advance(pos, sel);

		::EnableWindow(GetDlgItem(IDC_REMOVE_MENU), !SearchManager::isDefaultTypeStr(pos->first));
	}
	::EnableWindow(GetDlgItem(IDC_CHANGE_MENU), (lv->uNewState & LVIS_FOCUSED));
	return 0;
}

LRESULT SearchTypesPage::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	switch(kd->wVKey) {
	case VK_INSERT:
		PostMessage(WM_COMMAND, IDC_ADD_MENU, 0);
		break;
	case VK_DELETE:
		PostMessage(WM_COMMAND, IDC_REMOVE_MENU, 0);
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}

LRESULT SearchTypesPage::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

	if(item->iItem >= 0) {
		PostMessage(WM_COMMAND, IDC_CHANGE_MENU, 0);
	} else if(item->iItem == -1) {
		PostMessage(WM_COMMAND, IDC_ADD_MENU, 0);
	}

	return 0;
}

void SearchTypesPage::write() {
	PropPage::write((HWND)*this, items);
}

LRESULT SearchTypesPage::onResetDefaults(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(::MessageBox(0, CTSTRING(RESET_EXTENSIONS_CONFIRM), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
		SearchManager::getInstance()->setSearchTypeDefaults();
		fillList();
	}
	return 0;
}

void SearchTypesPage::showError(const string& e) {
	MessageBox(Text::toT(e).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONEXCLAMATION);
}
