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

#include "AutosearchPage.h"
#include "AutosearchDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define BUFLEN 256

PropPage::TextItem AutosearchPage::texts[] = {
	{ IDC_AUTOSEARCH_ENABLE,		ResourceManager::AUTOSEARCH_ENABLE },
	{ IDC_AUTOSEARCH_ENABLE_TIME,	ResourceManager::AUTOSEARCH_ENABLE_TIME },
	{ IDC_MINUTES,					ResourceManager::MINUTES },
	{ IDC_MINUTES2,					ResourceManager::MINUTES },
	{ IDC_ADD,						ResourceManager::ADD },
	{ IDC_CHANGE,					ResourceManager::SETTINGS_CHANGE },
	{ IDC_REMOVE,					ResourceManager::REMOVE },
	{ IDC_MOVE_DOWN,				ResourceManager::SETTINGS_BTN_MOVEDOWN },
	{ IDC_MOVE_UP,					ResourceManager::SETTINGS_BTN_MOVEUP },
	{ IDC_AUTOSEARCH_RECHECK_TEXT,	ResourceManager::AUTOSEARCH_RECHECK_TEXT },
	{ IDC_SEARH_SKIPLIST_PRESET,					ResourceManager::SETTINGS_SEARH_SKIPLIST_PRESET },
	{ IDC_ASEARCH,					ResourceManager::SETTINGS_ASEARCH },
	{ IDC_PRE1,					ResourceManager::PRESET1 },
	{ IDC_PRE2,					ResourceManager::PRESET2 },
	{ IDC_PRE3,					ResourceManager::PRESET3 },
	{ IDC_SORT_DIRS,					ResourceManager::SORT_DIRS },
	{ 0,							ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item AutosearchPage::items[] = {
	{ IDC_AUTOSEARCH_ENABLE,		SettingsManager::AUTOSEARCH_ENABLED,			PropPage::T_BOOL },
	{ IDC_AUTOSEARCH_ENABLE_TIME,	SettingsManager::AUTOSEARCH_ENABLED_TIME,	PropPage::T_BOOL },
	{ IDC_AUTOSEARCH_EVERY,			SettingsManager::AUTOSEARCH_EVERY,			PropPage::T_INT },
	{ IDC_AUTOSEARCH_RECHECK_TIME,	SettingsManager::AUTOSEARCH_RECHECK_TIME,	PropPage::T_INT },
	{ IDC_SKIPLIST_PRESET1, SettingsManager::SKIP_MSG_01, PropPage::T_STR },
	{ IDC_SKIPLIST_PRESET2, SettingsManager::SKIP_MSG_02, PropPage::T_STR },
	{ IDC_SKIPLIST_PRESET3, SettingsManager::SKIP_MSG_03, PropPage::T_STR },
	{ IDC_SORT_DIRS,		SettingsManager::SORT_DIRS,	PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

LRESULT AutosearchPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	CUpDownCtrl spin;
	spin.Attach(GetDlgItem(IDC_AUTOSEARCH_SPIN));
	spin.SetRange32(3, 360);
	spin.Detach();
	spin.Attach(GetDlgItem(IDC_AUTOSEARCH_RECHECK_SPIN));
	spin.SetRange32(1, 360);
	spin.Detach();

	CRect rc;
	ctrlAutosearch.Attach(GetDlgItem(IDC_AUTOSEARCH_ITEMS));
	ctrlAutosearch.GetClientRect(rc);
	ctrlAutosearch.InsertColumn(0, CTSTRING(SETTINGS_VALUE),		LVCFMT_LEFT, rc.Width() / 3, 0);
	ctrlAutosearch.InsertColumn(1, CTSTRING(TYPE),					LVCFMT_LEFT, rc.Width() / 8, 0);
	ctrlAutosearch.InsertColumn(2, CTSTRING(ACTION),				LVCFMT_LEFT, rc.Width() / 6, 0);
	ctrlAutosearch.SetExtendedListViewStyle(LVS_EX_CHECKBOXES | LVS_EX_INFOTIP | LVS_EX_FULLROWSELECT);

	Autosearch::List lst = AutoSearchManager::getInstance()->getAutosearch();
	for(Autosearch::List::const_iterator i = lst.begin(); i != lst.end(); ++i) {
		const Autosearch::Ptr as = *i;	
		addEntry(as, ctrlAutosearch.GetItemCount());
	}
	gotFocusOnList = false; //page created = no focus
	fixControls();
	return TRUE;
}

LRESULT AutosearchPage::onAdd(WORD , WORD , HWND , BOOL& ) {
	AutosearchPageDlg dlg;
	if(dlg.DoModal() == IDOK) {
		addEntry(AutoSearchManager::getInstance()->addAutosearch(true, Text::fromT(dlg.search), dlg.fileType, dlg.action), ctrlAutosearch.GetItemCount());
	}
	return 0;
}

LRESULT AutosearchPage::onChange(WORD , WORD , HWND , BOOL& ) {
	if(ctrlAutosearch.GetSelectedCount() == 1) {
		int sel = ctrlAutosearch.GetSelectedIndex();
		Autosearch as;
		AutoSearchManager::getInstance()->getAutosearch(sel, as);

		AutosearchPageDlg dlg;
		dlg.search = Text::toT(as.getSearchString());
		dlg.fileType = as.getFileType();
		dlg.action = as.getAction();

		if(dlg.DoModal() == IDOK) {
			as.setSearchString(Text::fromT(dlg.search));
			as.setFileType(dlg.fileType);
			as.setAction(dlg.action);

			AutoSearchManager::getInstance()->updateAutosearch(sel, as);

			ctrlAutosearch.SetCheckState(sel, as.getEnabled());
			ctrlAutosearch.SetItemText(sel, 0, dlg.search.c_str());
			ctrlAutosearch.SetItemText(sel, 1, Text::toT(getType(dlg.fileType)).c_str());
			if(dlg.action == 0){
				ctrlAutosearch.SetItemText(sel, 2, Text::toT(STRING(DOWNLOAD)).c_str());
			}else if(dlg.action == 1){
				ctrlAutosearch.SetItemText(sel, 2, Text::toT(STRING(ADD_TO_QUEUE)).c_str());
			}else if(dlg.action == 2){
				ctrlAutosearch.SetItemText(sel, 2, Text::toT(STRING(AS_REPORT)).c_str());
			}


		}
	}
	return 0;
}

LRESULT AutosearchPage::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlAutosearch.GetSelectedCount() == 1) {
		int sel = ctrlAutosearch.GetSelectedIndex();
		AutoSearchManager::getInstance()->removeAutosearch(sel);
		ctrlAutosearch.DeleteItem(sel);
	}
	return 0;
}

LRESULT AutosearchPage::onEnable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	gotFocusOnList = false;
	fixControls();
	return 0;
}

LRESULT AutosearchPage::onMoveUp(WORD , WORD , HWND , BOOL& ) {
	int i = ctrlAutosearch.GetSelectedIndex();
	if(i != -1 && i != 0) {
		//swap and reload list, not the best solution :P
		AutoSearchManager::getInstance()->moveAutosearchUp(i);
		ctrlAutosearch.SetRedraw(FALSE);
		ctrlAutosearch.DeleteAllItems();

		Autosearch::List lst = AutoSearchManager::getInstance()->getAutosearch();
		for(Autosearch::List::const_iterator j = lst.begin(); j != lst.end(); ++j) {
			const Autosearch::Ptr as = *j;	
			addEntry(as, ctrlAutosearch.GetItemCount());
		}

		ctrlAutosearch.SetItemState(i-1, LVIS_SELECTED, LVIS_SELECTED);
		ctrlAutosearch.EnsureVisible(i-1, FALSE);
		ctrlAutosearch.SetRedraw(TRUE);
		ctrlAutosearch.Invalidate();
		//selected, need to enable buttons
		::EnableWindow(GetDlgItem(IDC_MOVE_UP), true);
		::EnableWindow(GetDlgItem(IDC_MOVE_DOWN), true);
		::EnableWindow(GetDlgItem(IDC_CHANGE), true);
		::EnableWindow(GetDlgItem(IDC_REMOVE), true);
	}
	return 0;
}

LRESULT AutosearchPage::onMoveDown(WORD , WORD , HWND , BOOL& ) {
	int i = ctrlAutosearch.GetSelectedIndex();
	if(i != -1 && i != (ctrlAutosearch.GetItemCount()-1) ) {
		//swap and reload list, not the best solution :P
		ctrlAutosearch.SetRedraw(FALSE);
		AutoSearchManager::getInstance()->moveAutosearchDown(i);
		ctrlAutosearch.DeleteAllItems();

		Autosearch::List lst = AutoSearchManager::getInstance()->getAutosearch();
		for(Autosearch::List::const_iterator j = lst.begin(); j != lst.end(); ++j) {
			const Autosearch::Ptr as = *j;	
			addEntry(as, ctrlAutosearch.GetItemCount());
		}
		ctrlAutosearch.SetRedraw(TRUE);
		ctrlAutosearch.Invalidate();
		ctrlAutosearch.SetItemState(i+1, LVIS_SELECTED, LVIS_SELECTED);
		ctrlAutosearch.EnsureVisible(i+1, FALSE);
		//selected, need to enable buttons
		::EnableWindow(GetDlgItem(IDC_MOVE_UP), true);
		::EnableWindow(GetDlgItem(IDC_MOVE_DOWN), true);
		::EnableWindow(GetDlgItem(IDC_CHANGE), true);
		::EnableWindow(GetDlgItem(IDC_REMOVE), true);
	}
	return 0;
}

LRESULT AutosearchPage::onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NM_LISTVIEW* l = (NM_LISTVIEW*) pnmh;
	gotFocusOnList = (l->uNewState & LVIS_FOCUSED) || ((l->uNewState & LVIS_STATEIMAGEMASK) && ctrlAutosearch.GetSelectedIndex() != -1);

	::EnableWindow(GetDlgItem(IDC_CHANGE),		gotFocusOnList);
	::EnableWindow(GetDlgItem(IDC_REMOVE),		gotFocusOnList);
	::EnableWindow(GetDlgItem(IDC_MOVE_UP),		gotFocusOnList);
	::EnableWindow(GetDlgItem(IDC_MOVE_DOWN),	gotFocusOnList);
	return 0;		
}

void AutosearchPage::fixControls() {
	BOOL enable = IsDlgButtonChecked(IDC_AUTOSEARCH_ENABLE) == BST_CHECKED;
	BOOL time = enable && IsDlgButtonChecked(IDC_AUTOSEARCH_ENABLE_TIME) == BST_CHECKED;

	::EnableWindow(GetDlgItem(IDC_AUTOSEARCH_ITEMS),		enable);
	::EnableWindow(GetDlgItem(IDC_AUTOSEARCH_ENABLE_TIME),	enable);

	::EnableWindow(GetDlgItem(IDC_ADD),						enable);
	::EnableWindow(GetDlgItem(IDC_CHANGE),					enable && gotFocusOnList);
	::EnableWindow(GetDlgItem(IDC_REMOVE),					enable && gotFocusOnList);
	::EnableWindow(GetDlgItem(IDC_MOVE_UP),					enable && gotFocusOnList);
	::EnableWindow(GetDlgItem(IDC_MOVE_DOWN),				enable && gotFocusOnList);

	::EnableWindow(GetDlgItem(IDC_AUTOSEARCH_EVERY),		time);
	::EnableWindow(GetDlgItem(IDC_AUTOSEARCH_SPIN),			time);
	::EnableWindow(GetDlgItem(IDC_MINUTES),					time);
	::EnableWindow(GetDlgItem(IDC_MINUTES2),				time);
	::EnableWindow(GetDlgItem(IDC_AUTOSEARCH_RECHECK_TIME),	time);
	::EnableWindow(GetDlgItem(IDC_AUTOSEARCH_RECHECK_SPIN),	time);
	::EnableWindow(GetDlgItem(IDC_AUTOSEARCH_RECHECK_TEXT),	time);
}

LRESULT AutosearchPage::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		{
			try	{
				if(cd->nmcd.dwItemSpec % 2 == 0) {
					cd->clrTextBk = RGB(245, 245, 245); //second listitem color
				}
				return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
			} catch(const Exception&) {
			} catch(...) {
			}
		}
		return CDRF_NOTIFYSUBITEMDRAW;

	default:
		return CDRF_DODEFAULT;
	}
}

void AutosearchPage::write() {
	
	if(SETTING(AUTOSEARCH_EVERY) < 3)
		settings->set(SettingsManager::AUTOSEARCH_EVERY, 3);
	
	for(int i = 0; i < ctrlAutosearch.GetItemCount(); i++) {
		AutoSearchManager::getInstance()->setActiveItem(i, Util::toBool(ctrlAutosearch.GetCheckState(i)));
	}
	AutoSearchManager::getInstance()->AutosearchSave();
	PropPage::write((HWND)*this, items);
}

void AutosearchPage::addEntry(const Autosearch::Ptr as, int pos) {
	TStringList lst;
	lst.push_back(Text::toT(as->getSearchString()));
	lst.push_back(Text::toT(getType(as->getFileType())));
	if(as->getAction() == 0){
		lst.push_back(Text::toT("Download"));
	}else if(as->getAction() == 1){
		lst.push_back(Text::toT("Add to Queue"));
	}else if(as->getAction() == 2){
		lst.push_back(Text::toT("Report Hit in Mainchat"));
	}
	int i = ctrlAutosearch.insert(pos, lst, 0, (LPARAM)as->getEnabled());
	ctrlAutosearch.SetCheckState(i, as->getEnabled());
}