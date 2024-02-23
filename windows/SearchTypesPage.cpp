/* 
 * Copyright (C) 2012-2024 AirDC++ Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include <airdcpp/SearchManager.h>

#include "Resource.h"
#include "SearchTypesPage.h"
#include "SearchTypeDlg.h"
#include "WinUtil.h"

PropPage::TextItem SearchTypesPage::texts[] = {
	{ IDC_ADD_MENU, ResourceManager::ADD },
	{ IDC_CHANGE_MENU, ResourceManager::SETTINGS_CHANGE },
	{ IDC_REMOVE_MENU, ResourceManager::REMOVE },
	{ IDC_RESET_DEFAULTS, ResourceManager::RESET_TO_DEFAULTS },
	{ IDC_SEARCHTYPES_NOTE, ResourceManager::SEARCH_TYPES_NOTE },
	{ 0, ResourceManager::LAST }
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

	ctrlTypes.InsertColumn(0, CTSTRING(NAME), LVCFMT_LEFT, rc.Width()/4, 0);
	ctrlTypes.InsertColumn(1, CTSTRING(SETTINGS_EXTENSIONS), LVCFMT_LEFT, rc.Width()*2 / 4, 1);
	ctrlTypes.InsertColumn(2, CTSTRING(DEFAULT), LVCFMT_LEFT, rc.Width() / 4, 2);
	ctrlTypes.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	fillList();
	
	return TRUE;
}

void SearchTypesPage::fillList() {
	ctrlTypes.DeleteAllItems();

	auto typeList = SearchManager::getInstance()->getSearchTypes();

	int pos = 0;
	for(const auto& t: typeList) {
		TStringList lst;
		lst.push_back(Text::toT(t->getDisplayName()));
		lst.push_back(Text::toT(Util::toString(";", t->getExtensions())));
		lst.push_back(t->isDefault() ? CTSTRING(YES) : CTSTRING(NO));
		ctrlTypes.insert(pos++, lst, 0);
	}
}

LRESULT SearchTypesPage::onAddMenu(WORD , WORD , HWND , BOOL& ) {
	SearchTypeDlg dlg;

	if(dlg.DoModal() == IDOK) {
		try {
			SearchManager::getInstance()->addSearchType(dlg.name, dlg.extList);
			fillList();
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
		auto type = *pos;

		SearchTypeDlg dlg;

		dlg.name = type->getDisplayName();
		dlg.extList = type->getExtensions();
		dlg.isDefault = type->isDefault();

		if(dlg.DoModal() == IDOK) {
			try {
				SearchManager::getInstance()->modSearchType(
					type->getId(), 
					!dlg.isDefault && type->getId() != dlg.name ? optional(dlg.name) : nullopt,
					dlg.extList
				);
			} catch(const SearchTypeException& e) {
				showError(e.getError());
			}

			fillList();
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

		auto type = *pos;
		if (type->isDefault()) {
			return 0;
		}

		try {
			SearchManager::getInstance()->delSearchType(type->getId());
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
		if (sel != -1) {
			auto lst = SearchManager::getInstance()->getSearchTypes();

			auto pos = lst.begin();
			advance(pos, sel);

			::EnableWindow(GetDlgItem(IDC_REMOVE_MENU), !(*pos)->isDefault());
		}
	} else {
		::EnableWindow(GetDlgItem(IDC_REMOVE_MENU), FALSE);
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
	if(WinUtil::showQuestionBox(TSTRING(RESET_EXTENSIONS_CONFIRM), MB_ICONQUESTION)) {
		SearchManager::getInstance()->setSearchTypeDefaults();
		fillList();
	}
	return 0;
}

void SearchTypesPage::showError(const string& e) {
	WinUtil::showMessageBox(Text::toT(e), MB_ICONEXCLAMATION);
}
