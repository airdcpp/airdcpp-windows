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
#include "Resource.h"

#include "AutoSearchFrm.h"
#include "../client/SettingsManager.h"
#include "../client/StringTokenizer.h"
#include "../client/AutoSearchManager.h"

int AutoSearchFrame::columnIndexes[] = { COLUMN_VALUE, COLUMN_TYPE, COLUMN_ACTION, COLUMN_PATH, COLUMN_REMOVE};
int AutoSearchFrame::columnSizes[] = { 450, 100, 125, 400, 100 };
static ResourceManager::Strings columnNames[] = { ResourceManager::SETTINGS_VALUE, ResourceManager::TYPE, 
ResourceManager::ACTION, ResourceManager::PATH, ResourceManager::REMOVE_ON_HIT };

LRESULT AutoSearchFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	
	ctrlAutosearch.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, IDC_AUTOSEARCH);
	ctrlAutosearch.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);	
	ctrlAutosearch.SetBkColor(WinUtil::bgColor);
	ctrlAutosearch.SetTextBkColor(WinUtil::bgColor);
	ctrlAutosearch.SetTextColor(WinUtil::textColor);
	
	// Insert columns
	WinUtil::splitTokens(columnIndexes, SETTING(AUTOSEARCHFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(AUTOSEARCHFRAME_WIDTHS), COLUMN_LAST);
	
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = LVCFMT_LEFT;
		ctrlAutosearch.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlAutosearch.SetColumnOrderArray(COLUMN_LAST, columnIndexes);

	/*Autosearch every time */
	ctrlAsTime.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | ES_RIGHT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL | ES_NUMBER, WS_EX_CLIENTEDGE,IDC_AUTOSEARCH_ENABLE_TIME );
	ctrlAsTime.SetFont(WinUtil::systemFont);

	Timespin.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_NOTHOUSANDS | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	Timespin.SetRange(0, 999);
	ctrlAsTimeLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | SS_RIGHT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlAsTimeLabel.SetFont(WinUtil::systemFont, FALSE);
	ctrlAsTimeLabel.SetWindowText(CTSTRING(AUTOSEARCH_ENABLE_TIME));
	ctrlAsTime.SetWindowText(Text::toT(Util::toString(SETTING(AUTOSEARCH_EVERY))).c_str());
	
	/*Autosearch reched items time */
	ctrlAsRTime.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | ES_RIGHT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
	ES_AUTOHSCROLL | ES_NUMBER, WS_EX_CLIENTEDGE,IDC_AUTOSEARCH_RECHECK_TIME );
	ctrlAsRTime.SetFont(WinUtil::systemFont);

	RTimespin.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_NOTHOUSANDS | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	RTimespin.SetRange(0, 999);
	ctrlAsRTimeLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | SS_RIGHT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlAsRTimeLabel.SetFont(WinUtil::systemFont, FALSE);
	ctrlAsRTimeLabel.SetWindowText(CTSTRING(AUTOSEARCH_RECHECK_TEXT));
	ctrlAsRTime.SetWindowText(Text::toT(Util::toString(SETTING(AUTOSEARCH_RECHECK_TIME))).c_str());
	

	//create buttons
	ctrlAdd.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_ADD);
	ctrlAdd.SetWindowText(CTSTRING(ADD));
	ctrlAdd.SetFont(WinUtil::systemFont);

	ctrlRemove.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_REMOVE);
	ctrlRemove.SetWindowText(CTSTRING(REMOVE));
	ctrlRemove.SetFont(WinUtil::systemFont);

	ctrlChange.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_CHANGE);
	ctrlChange.SetWindowText(CTSTRING(SETTINGS_CHANGE));
	ctrlChange.SetFont(WinUtil::systemFont);

	ctrlDown.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_MOVE_DOWN);
	ctrlDown.SetWindowText(CTSTRING(SETTINGS_BTN_MOVEDOWN ));
	ctrlDown.SetFont(WinUtil::systemFont);

	ctrlUp.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_MOVE_UP);
	ctrlUp.SetWindowText(CTSTRING(SETTINGS_BTN_MOVEUP));
	ctrlUp.SetFont(WinUtil::systemFont);

	AutoSearchManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);



	//fill the list
	updateList();

	WinUtil::SetIcon(m_hWnd, _T("search.ico"));
	loading = false;
	bHandled = FALSE;
	return TRUE;

}

void AutoSearchFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	/*
	Counting Text lengths for the items according to Font,
	so translations should fit nicely Automatically 
	*/
	
	RECT rect;
	GetClientRect(&rect);
	UpdateBarsPosition(rect, bResizeBars);

	CRect rc = rect;
	rc.bottom -=60;
	ctrlAutosearch.MoveWindow(rc);

	rc = rect;

	const int button_width = 80;
	const int textbox_width = 30;
	const int middle_margin = 32;

	const long bottom = rc.bottom - 2;
	const long top =  rc.bottom - 54;

	/*Autosearch time settings*/
	//text
	rc.bottom = bottom - middle_margin -2;
	rc.top = rc.bottom - WinUtil::getTextHeight(m_hWnd, WinUtil::systemFont) - 2;
	rc.left = 2;
	rc.right = rc.left + (ctrlAsTimeLabel.GetWindowTextLength() * WinUtil::getTextWidth(m_hWnd, WinUtil::systemFont)) +2;
	ctrlAsTimeLabel.MoveWindow(rc);
	//setting box
	rc.top = top;
	rc.bottom = bottom - middle_margin;
	rc.left = rc.right +4;
	rc.right = rc.left + textbox_width;
	ctrlAsTime.MoveWindow(rc);
	//the spin
	rc.left = rc.right;
	rc.right = rc.left + 20;
	Timespin.MoveWindow(rc);

	/*Autosearch recheck time settings*/
	rc.bottom = bottom - middle_margin -2;
	rc.top = rc.bottom - WinUtil::getTextHeight(m_hWnd, WinUtil::systemFont) - 2;
	rc.left = rc.right + 5;
	rc.right = rc.left + (ctrlAsRTimeLabel.GetWindowTextLength() * WinUtil::getTextWidth(m_hWnd, WinUtil::systemFont)) +2;
	ctrlAsRTimeLabel.MoveWindow(rc);
	//setting box
	rc.top = top;
	rc.bottom = bottom - middle_margin;
	rc.left = rc.right +4;
	rc.right = rc.left + textbox_width;
	ctrlAsRTime.MoveWindow(rc);
	//the spin
	rc.left = rc.right;
	rc.right = rc.left + 20;
	RTimespin.MoveWindow(rc);

	//buttons
	rc.bottom = bottom;
	rc.top = bottom - 22;
	rc.left = 2;
	rc.right = rc.left + button_width;
	ctrlAdd.MoveWindow(rc);

	rc.OffsetRect(button_width+2, 0);
	ctrlRemove.MoveWindow(rc);

	rc.OffsetRect(button_width+2, 0);
	ctrlChange.MoveWindow(rc);
	
	//add a small space between these buttons
	rc.OffsetRect(10 + button_width +2, 0);
	ctrlUp.MoveWindow(rc);

	rc.OffsetRect(button_width+2, 0);
	ctrlDown.MoveWindow(rc);

}

LRESULT AutoSearchFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	if(reinterpret_cast<HWND>(wParam) == ctrlAutosearch) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		
		asMenu.CreatePopupMenu();

		CRect rc;

		ctrlAutosearch.GetHeader().GetWindowRect(&rc);
		if (PtInRect(&rc, pt)) {
			return 0;
		}

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlAutosearch, pt);
		}

		int enable = ctrlAutosearch.GetSelectedCount() == 1 ? MFS_ENABLED : MFS_DISABLED;

		if(ctrlAutosearch.GetSelectedCount() > 1) {
			asMenu.AppendMenu(MF_STRING, IDC_ENABLE, CTSTRING(ENABLE_AUTOSEARCH));
			asMenu.AppendMenu(MF_STRING, IDC_DISABLE, CTSTRING(DISABLE_AUTOSEARCH));
			asMenu.AppendMenu(MF_SEPARATOR);
			//only remove and add is enabled
		} else if(ctrlAutosearch.GetSelectedCount() == 1) {
			if(ctrlAutosearch.GetCheckState(ctrlAutosearch.GetSelectedIndex()) == 1)
				asMenu.AppendMenu(MF_STRING, IDC_DISABLE, CTSTRING(DISABLE_AUTOSEARCH));
			else
				asMenu.AppendMenu(MF_STRING, IDC_ENABLE, CTSTRING(ENABLE_AUTOSEARCH));

				asMenu.AppendMenu(MF_SEPARATOR);
				//all menu items enabled
		}
		

		asMenu.AppendMenu(MF_STRING, IDC_ADD, CTSTRING(ADD));
		asMenu.AppendMenu(MF_STRING, IDC_CHANGE, CTSTRING(SETTINGS_CHANGE));
		asMenu.AppendMenu(MF_STRING, IDC_MOVE_UP, CTSTRING(SETTINGS_BTN_MOVEUP));
		asMenu.AppendMenu(MF_STRING, IDC_MOVE_DOWN, CTSTRING(SETTINGS_BTN_MOVEDOWN));
		asMenu.AppendMenu(MF_SEPARATOR);
		asMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));

		//asMenu.EnableMenuItem(IDC_REMOVE, enable);
		asMenu.EnableMenuItem(IDC_CHANGE, enable);
		asMenu.EnableMenuItem(IDC_MOVE_UP, enable);
		asMenu.EnableMenuItem(IDC_MOVE_DOWN, enable);
		
		//make a menu title from the search string, its probobly too long to fit but atleast it shows something.
		tstring title;
		if (ctrlAutosearch.GetSelectedCount() == 1) {
			AutosearchPtr as = (AutosearchPtr)ctrlAutosearch.GetItemData(WinUtil::getFirstSelectedIndex(ctrlAutosearch));
			title = Text::toT(as->getSearchString());
		} else {
			title = _T("");
		}
		if (!title.empty())
			asMenu.InsertSeparatorFirst(title);

		
		asMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

		if (!title.empty())
			asMenu.RemoveFirstItem();

		return TRUE; 
	}
	
	bHandled = FALSE;
	return FALSE; 
}


LRESULT AutoSearchFrame::onAdd(WORD , WORD , HWND , BOOL& ) {
	AutosearchPageDlg dlg;
	if(dlg.DoModal() == IDOK) {
		string search = Text::fromT(dlg.search + _T("\r\n"));
		string::size_type j = 0;
		string::size_type i = 0;
	
		while((i = search.find("\r\n", j)) != string::npos) {
			string str = search.substr(j, i-j);
			j = i +2;
			if(str.size() >= 5) //dont accept shorter search strings than 5 chars
				AutoSearchManager::getInstance()->addAutosearch(true, str, dlg.fileType, dlg.action, dlg.remove, Text::fromT(dlg.target));
		}
	}
	return 0;
}

LRESULT AutoSearchFrame::onChange(WORD , WORD , HWND , BOOL& ) {
	if(ctrlAutosearch.GetSelectedCount() == 1) {
		int sel = ctrlAutosearch.GetSelectedIndex();
		Autosearch as;
		AutoSearchManager::getInstance()->getAutosearch(sel, as);

		AutosearchPageDlg dlg;
		dlg.search = Text::toT(as.getSearchString());
		dlg.fileType = as.getFileType();
		dlg.action = as.getAction();
		dlg.remove = as.getRemove();
		dlg.target = Text::toT(as.getTarget());

		if(dlg.DoModal() == IDOK) {
			as.setSearchString(Text::fromT(dlg.search));
			as.setFileType(dlg.fileType);
			as.setAction(dlg.action);
			as.setRemove(dlg.remove);
			as.setTarget(Text::fromT(dlg.target));

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
			ctrlAutosearch.SetItemText(sel, 3, dlg.target.c_str());

		}
	}
	return 0;
}

LRESULT AutoSearchFrame::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	//use a removelist, the selection position will change when removing multiple.
	std::vector<AutosearchPtr> removelist;

	int i = -1;
	while( (i = ctrlAutosearch.GetNextItem(i, LVNI_SELECTED)) != -1) {
		removelist.push_back((AutosearchPtr)ctrlAutosearch.GetItemData(i));
	}

	for(std::vector<AutosearchPtr>::const_iterator a = removelist.begin(); a !=removelist.end(); ++a )
		AutoSearchManager::getInstance()->removeAutosearch(*a);

	return 0;
}
LRESULT AutoSearchFrame::onMoveUp(WORD , WORD , HWND , BOOL& ) {
	int i = ctrlAutosearch.GetSelectedIndex();
	if(i != -1 && i != 0) {
		//swap and reload list, not the best solution :P
		AutoSearchManager::getInstance()->moveAutosearchUp(i);
		ctrlAutosearch.SetRedraw(FALSE);
		ctrlAutosearch.DeleteAllItems();

		Autosearch::List lst = AutoSearchManager::getInstance()->getAutosearch();
		for(Autosearch::List::const_iterator j = lst.begin(); j != lst.end(); ++j) {
			const AutosearchPtr as = *j;	
			addEntry(as, ctrlAutosearch.GetItemCount());
		}

		ctrlAutosearch.SetItemState(i-1, LVIS_SELECTED, LVIS_SELECTED);
		ctrlAutosearch.EnsureVisible(i-1, FALSE);
		ctrlAutosearch.SetRedraw(TRUE);
		ctrlAutosearch.Invalidate();
	}
	return 0;
}

LRESULT AutoSearchFrame::onMoveDown(WORD , WORD , HWND , BOOL& ) {
	int i = ctrlAutosearch.GetSelectedIndex();
	if(i != -1 && i != (ctrlAutosearch.GetItemCount()-1) ) {
		//swap and reload list, not the best solution :P
		ctrlAutosearch.SetRedraw(FALSE);
		AutoSearchManager::getInstance()->moveAutosearchDown(i);
		ctrlAutosearch.DeleteAllItems();

		Autosearch::List lst = AutoSearchManager::getInstance()->getAutosearch();
		for(Autosearch::List::const_iterator j = lst.begin(); j != lst.end(); ++j) {
			const AutosearchPtr as = *j;	
			addEntry(as, ctrlAutosearch.GetItemCount());
		}
		ctrlAutosearch.SetRedraw(TRUE);
		ctrlAutosearch.Invalidate();
		ctrlAutosearch.SetItemState(i+1, LVIS_SELECTED, LVIS_SELECTED);
		ctrlAutosearch.EnsureVisible(i+1, FALSE);

	}
	return 0;
}

LRESULT AutoSearchFrame::onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* l = (NMITEMACTIVATE*)pnmh;
	
	::EnableWindow(GetDlgItem(IDC_REMOVE), ctrlAutosearch.GetItemState(l->iItem, LVIS_SELECTED));
	::EnableWindow(GetDlgItem(IDC_CHANGE), ctrlAutosearch.GetItemState(l->iItem, LVIS_SELECTED));
	::EnableWindow(GetDlgItem(IDC_MOVE_UP), ctrlAutosearch.GetItemState(l->iItem, LVIS_SELECTED));
	::EnableWindow(GetDlgItem(IDC_MOVE_DOWN), ctrlAutosearch.GetItemState(l->iItem, LVIS_SELECTED));
	
	if(!loading && l->iItem != -1 && ((l->uNewState & LVIS_STATEIMAGEMASK) != (l->uOldState & LVIS_STATEIMAGEMASK))) {
		AutoSearchManager::getInstance()->setActiveItem(l->iItem, Util::toBool(ctrlAutosearch.GetCheckState(l->iItem)));	
	}
	return 0;		
}

void AutoSearchFrame::addEntry(const AutosearchPtr as, int pos) {
	if(as == NULL)
		return;
		
	if(pos < 0)
		pos = 0;

	TStringList lst;
	lst.push_back(Text::toT(as->getSearchString()));
	lst.push_back(Text::toT(getType(as->getFileType())));
		
		if(as->getAction() == 0){
			lst.push_back(Text::toT(STRING(DOWNLOAD)));
		}else if(as->getAction() == 1){
			lst.push_back(Text::toT(STRING(ADD_TO_QUEUE)));
		}else if(as->getAction() == 2){
			lst.push_back(Text::toT(STRING(AS_REPORT)));
		}
		
	lst.push_back(Text::toT(as->getTarget()));
	lst.push_back(Text::toT(as->getRemove()? "Yes" : "No"));
		
	bool b = as->getEnabled();
	int i = ctrlAutosearch.insert(pos, lst, 0, (LPARAM)as);
	ctrlAutosearch.SetCheckState(i, b);

}


LRESULT AutoSearchFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {

	if(!closed) {
		SettingsManager::getInstance()->removeListener(this);
		AutoSearchManager::getInstance()->removeListener(this);
		WinUtil::setButtonPressed(IDC_AUTOSEARCH, false);
		closed = true;
		save();
		PostMessage(WM_CLOSE);
		return 0;
	}else {
		WinUtil::saveHeaderOrder(
			ctrlAutosearch, 
			SettingsManager::AUTOSEARCHFRAME_ORDER, 
			SettingsManager::AUTOSEARCHFRAME_WIDTHS, 
			COLUMN_LAST, 
			columnIndexes, 
			columnSizes
			);


		bHandled = FALSE;
		return 0;
	}
}	