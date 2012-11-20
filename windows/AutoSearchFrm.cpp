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

int AutoSearchFrame::columnIndexes[] = { COLUMN_VALUE, COLUMN_TYPE, COLUMN_ACTION, COLUMN_PATH, COLUMN_REMOVE, COLUMN_MATCH, COLUMN_LASTSEARCH };
int AutoSearchFrame::columnSizes[] = { 450, 100, 125, 400, 100, 200, 200 };
static ResourceManager::Strings columnNames[] = { ResourceManager::SETTINGS_VALUE, ResourceManager::TYPE, 
ResourceManager::ACTION, ResourceManager::PATH, ResourceManager::REMOVE_ON_HIT, ResourceManager::USER_MATCH, ResourceManager::LAST_SEARCH };

LRESULT AutoSearchFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	
	ctrlAutoSearch.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, IDC_AUTOSEARCH);
	ctrlAutoSearch.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);	
	ctrlAutoSearch.SetBkColor(WinUtil::bgColor);
	ctrlAutoSearch.SetTextBkColor(WinUtil::bgColor);
	ctrlAutoSearch.SetTextColor(WinUtil::textColor);
	
	// Insert columns
	WinUtil::splitTokens(columnIndexes, SETTING(AUTOSEARCHFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(AUTOSEARCHFRAME_WIDTHS), COLUMN_LAST);
	
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = LVCFMT_LEFT;
		ctrlAutoSearch.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlAutoSearch.SetColumnOrderArray(COLUMN_LAST, columnIndexes);

	/*AutoSearch every time */
	ctrlAsTime.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | ES_RIGHT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL | ES_NUMBER, WS_EX_CLIENTEDGE,IDC_AUTOSEARCH_ENABLE_TIME );
	ctrlAsTime.SetFont(WinUtil::systemFont);

	Timespin.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_NOTHOUSANDS | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	Timespin.SetRange(1, 999);
	ctrlAsTimeLabel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | SS_RIGHT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlAsTimeLabel.SetFont(WinUtil::systemFont, FALSE);
	ctrlAsTimeLabel.SetWindowText(CTSTRING(AUTOSEARCH_ENABLE_TIME));
	ctrlAsTime.SetWindowText(Text::toT(Util::toString(SETTING(AUTOSEARCH_EVERY))).c_str());
	
	/*AutoSearch reched items time */
	ctrlAsRTime.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | ES_RIGHT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
	ES_AUTOHSCROLL | ES_NUMBER, WS_EX_CLIENTEDGE,IDC_AUTOSEARCH_RECHECK_TIME );
	ctrlAsRTime.SetFont(WinUtil::systemFont);

	RTimespin.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_AUTOBUDDY | UDS_ARROWKEYS | UDS_NOTHOUSANDS | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	RTimespin.SetRange(30, 999);
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

	WinUtil::SetIcon(m_hWnd, IDI_AUTOSEARCH);
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
	ctrlAutoSearch.MoveWindow(rc);

	rc = rect;

	const int button_width = 80;
	const int textbox_width = 30;
	const int middle_margin = 32;

	const long bottom = rc.bottom - 2;
	const long top =  rc.bottom - 54;

	/*AutoSearch time settings*/
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

	/*AutoSearch recheck time settings*/
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

	if(reinterpret_cast<HWND>(wParam) == ctrlAutoSearch) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		
		asMenu.CreatePopupMenu();

		CRect rc;

		ctrlAutoSearch.GetHeader().GetWindowRect(&rc);
		if (PtInRect(&rc, pt)) {
			return 0;
		}

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlAutoSearch, pt);
		}

		int enable = ctrlAutoSearch.GetSelectedCount() == 1 ? MFS_ENABLED : MFS_DISABLED;

		if(ctrlAutoSearch.GetSelectedCount() > 1) {
			asMenu.AppendMenu(MF_STRING, IDC_ENABLE, CTSTRING(ENABLE_AUTOSEARCH));
			asMenu.AppendMenu(MF_STRING, IDC_DISABLE, CTSTRING(DISABLE_AUTOSEARCH));
			asMenu.AppendMenu(MF_SEPARATOR);
			//only remove and add is enabled
		} else if(ctrlAutoSearch.GetSelectedCount() == 1) {
			asMenu.AppendMenu(MF_STRING, IDC_SEARCH, CTSTRING(SEARCH));
			asMenu.AppendMenu(MF_SEPARATOR);
			
			if(ctrlAutoSearch.GetCheckState(ctrlAutoSearch.GetSelectedIndex()) == 1) {
				asMenu.AppendMenu(MF_STRING, IDC_DISABLE, CTSTRING(DISABLE_AUTOSEARCH));
			} else {
				asMenu.AppendMenu(MF_STRING, IDC_ENABLE, CTSTRING(ENABLE_AUTOSEARCH));
			}
			asMenu.AppendMenu(MF_SEPARATOR);
		}
		
		asMenu.AppendMenu(MF_STRING, IDC_ADD, CTSTRING(ADD));
		asMenu.AppendMenu(MF_STRING, IDC_CHANGE, CTSTRING(SETTINGS_CHANGE));
		asMenu.AppendMenu(MF_STRING, IDC_MOVE_UP, CTSTRING(SETTINGS_BTN_MOVEUP));
		asMenu.AppendMenu(MF_STRING, IDC_MOVE_DOWN, CTSTRING(SETTINGS_BTN_MOVEDOWN));
		asMenu.AppendMenu(MF_SEPARATOR);
		asMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));

		asMenu.EnableMenuItem(IDC_CHANGE, enable);
		asMenu.EnableMenuItem(IDC_MOVE_UP, enable);
		asMenu.EnableMenuItem(IDC_MOVE_DOWN, enable);
		
		asMenu.SetMenuDefaultItem(IDC_CHANGE);
		//make a menu title from the search string, its probobly too long to fit but atleast it shows something.
		tstring title;
		if (ctrlAutoSearch.GetSelectedCount() == 1) {
			StringPairList spl;
			auto as = AutoSearchManager::getInstance()->getSearchByIndex(ctrlAutoSearch.GetSelectedIndex());
			title = Text::toT(as->getSearchString());

			AutoSearchManager::getInstance()->getBundleInfo(as, spl);

			if (!spl.empty()) {
				auto bundleMenu = asMenu.createSubMenu(CTSTRING(REMOVE_BUNDLE), true);
				for(auto j=spl.begin(); j != spl.end(); j++) {
					string token = j->first;
					bundleMenu->appendItem(Text::toT(j->second), [=] { WinUtil::removeBundle(token); });
				}
			}
		}

		if (!title.empty())
			asMenu.InsertSeparatorFirst(title);
		
		asMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);

		if (!title.empty())
			asMenu.RemoveFirstItem();

		return TRUE; 
	}
	
	bHandled = FALSE;
	return FALSE; 
}


LRESULT AutoSearchFrame::onAdd(WORD , WORD , HWND , BOOL& ) {
	AutoSearchDlg dlg;
	dlg.expireTime = SETTING(AUTOSEARCH_EXPIRE_DAYS) > 0 ? GET_TIME() + (SETTING(AUTOSEARCH_EXPIRE_DAYS)*24*60*60) : 0;
	if(dlg.DoModal() == IDOK) {
		string search = dlg.searchString + "\r\n";
		string::size_type j = 0;
		string::size_type i = 0;
	
		while((i = search.find("\r\n", j)) != string::npos) {
			string str = search.substr(j, i-j);
			j = i +2;
			if(str.size() >= 5) { //dont accept shorter search strings than 5 chars
				AutoSearchPtr as = new AutoSearch(true, str, dlg.fileTypeStr, (AutoSearch::ActionType)dlg.action, dlg.remove, 
					dlg.target, dlg.targetType, (StringMatch::Method)dlg.matcherType, dlg.matcherString, dlg.userMatch, dlg.searchInterval, dlg.expireTime, dlg.checkQueued, dlg.checkShared);
				as->startTime = dlg.startTime;
				as->endTime = dlg.endTime;
				as->searchDays = dlg.searchDays;
				AutoSearchManager::getInstance()->addAutoSearch(as);
			} else if(search.size() < 5) { // dont report if empty line between/end when adding multiple
				//MessageBox(_T("Not adding the auto search: ") + Text::toT(str).c_str());
				MessageBox(CTSTRING(LINE_EMPTY_OR_TOO_SHORT));
			}
		}
	}
	return 0;
}

LRESULT AutoSearchFrame::onChange(WORD , WORD , HWND , BOOL& ) {
	if(ctrlAutoSearch.GetSelectedCount() == 1) {
		int sel = ctrlAutoSearch.GetSelectedIndex();
		AutoSearchPtr as = AutoSearchManager::getInstance()->getSearchByIndex(sel);

		AutoSearchDlg dlg;
		dlg.searchString = as->getSearchString();
		dlg.fileTypeStr = as->getFileType();
		dlg.action = as->getAction();
		dlg.remove = as->getRemove();
		dlg.target = as->getTarget();
		dlg.userMatch = as->getNickPattern();
		dlg.matcherString = as->pattern;
		dlg.matcherType = as->getMethod();
		dlg.expireTime = as->getExpireTime();
		dlg.searchDays = as->searchDays;
		dlg.startTime = as->startTime;
		dlg.endTime = as->endTime;
		dlg.targetType = as->getTargetType();
		dlg.checkQueued = as->getCheckAlreadyQueued();
		dlg.checkShared = as->getCheckAlreadyShared();

		if(dlg.DoModal() == IDOK) {
			AutoSearchPtr asNew = AutoSearchPtr(new AutoSearch(as->getEnabled(), dlg.searchString, dlg.fileTypeStr, (AutoSearch::ActionType)dlg.action, 
				dlg.remove, dlg.target, (TargetUtil::TargetType)dlg.targetType, (StringMatch::Method)dlg.matcherType, dlg.matcherString, dlg.userMatch, dlg.searchInterval, 
				dlg.expireTime, dlg.checkQueued, dlg.checkShared));
			asNew->startTime = dlg.startTime;
			asNew->endTime = dlg.endTime;
			asNew->searchDays = dlg.searchDays;
			asNew->setLastSearch(as->getLastSearch());

			if (AutoSearchManager::getInstance()->updateAutoSearch(sel, asNew)) {
				ctrlAutoSearch.DeleteItem(sel);
				addEntry(asNew, sel);
				ctrlAutoSearch.SelectItem(sel);
			}
		}
	}
	return 0;
}

LRESULT AutoSearchFrame::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	//use a removelist, the selection position will change when removing multiple.
	std::vector<AutoSearchPtr> removelist;

	int i = -1;
	while( (i = ctrlAutoSearch.GetNextItem(i, LVNI_SELECTED)) != -1) {
		removelist.push_back((AutoSearch*)ctrlAutoSearch.GetItemData(i));
	}

	if(!BOOLSETTING(CONFIRM_AS_REMOVE) || (MessageBox(CTSTRING(REALLY_REMOVE), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)) {
		for(auto a = removelist.begin(); a !=removelist.end(); ++a )
			AutoSearchManager::getInstance()->removeAutoSearch(*a);
	}
	return 0;
}

LRESULT AutoSearchFrame::onMoveUp(WORD , WORD , HWND , BOOL& ) {
	int i = ctrlAutoSearch.GetSelectedIndex();
	if(i != -1 && i != 0) {
		//swap and reload list, not the best solution :P
		AutoSearchManager::getInstance()->moveAutoSearchUp(i);
		ctrlAutoSearch.SetRedraw(FALSE);
		ctrlAutoSearch.DeleteAllItems();

		AutoSearchList lst = AutoSearchManager::getInstance()->getSearchItems();
		for(auto j = lst.begin(); j != lst.end(); ++j) {
			const AutoSearchPtr as = *j;	
			addEntry(as, ctrlAutoSearch.GetItemCount());
		}

		ctrlAutoSearch.SetItemState(i-1, LVIS_SELECTED, LVIS_SELECTED);
		ctrlAutoSearch.EnsureVisible(i-1, FALSE);
		ctrlAutoSearch.SetRedraw(TRUE);
		ctrlAutoSearch.Invalidate();
	}
	return 0;
}

LRESULT AutoSearchFrame::onMoveDown(WORD , WORD , HWND , BOOL& ) {
	int i = ctrlAutoSearch.GetSelectedIndex();
	if(i != -1 && i != (ctrlAutoSearch.GetItemCount()-1) ) {
		//swap and reload list, not the best solution :P
		ctrlAutoSearch.SetRedraw(FALSE);
		AutoSearchManager::getInstance()->moveAutoSearchDown(i);
		ctrlAutoSearch.DeleteAllItems();

		AutoSearchList lst = AutoSearchManager::getInstance()->getSearchItems();
		for(auto j = lst.begin(); j != lst.end(); ++j) {
			const AutoSearchPtr as = *j;	
			addEntry(as, ctrlAutoSearch.GetItemCount());
		}
		ctrlAutoSearch.SetRedraw(TRUE);
		ctrlAutoSearch.Invalidate();
		ctrlAutoSearch.SetItemState(i+1, LVIS_SELECTED, LVIS_SELECTED);
		ctrlAutoSearch.EnsureVisible(i+1, FALSE);

	}
	return 0;
}
LRESULT AutoSearchFrame::onSearchAs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlAutoSearch.GetSelectedCount() == 1) {
		int sel = ctrlAutoSearch.GetSelectedIndex();
		AutoSearchPtr as = AutoSearchManager::getInstance()->getSearchByIndex(sel);
		if(as) {
			as->setLastSearch(GET_TIME());
			updateItem(as, sel);
			AutoSearchManager::getInstance()->manualSearch(as);
		}
	}
	return 0;
}

LRESULT AutoSearchFrame::onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* l = (NMITEMACTIVATE*)pnmh;
	
	::EnableWindow(GetDlgItem(IDC_REMOVE), (ctrlAutoSearch.GetSelectedCount() >= 1));
	::EnableWindow(GetDlgItem(IDC_CHANGE), (ctrlAutoSearch.GetSelectedCount() == 1));
	::EnableWindow(GetDlgItem(IDC_MOVE_UP), (ctrlAutoSearch.GetSelectedCount() == 1));
	::EnableWindow(GetDlgItem(IDC_MOVE_DOWN), (ctrlAutoSearch.GetSelectedCount() == 1));
	
	if(!loading && l->iItem != -1 && ((l->uNewState & LVIS_STATEIMAGEMASK) != (l->uOldState & LVIS_STATEIMAGEMASK))) {
		AutoSearchManager::getInstance()->setActiveItem(l->iItem, Util::toBool(ctrlAutoSearch.GetCheckState(l->iItem)));	
	}
	return 0;		
}

LRESULT AutoSearchFrame::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	switch(kd->wVKey) {
		case VK_DELETE:
			if(ctrlAutoSearch.GetSelectedCount() >= 1) {
				PostMessage(WM_COMMAND, IDC_REMOVE, 0);
			}
			break;
		case VK_RETURN:
			if( WinUtil::isShift() || WinUtil::isCtrl() || WinUtil::isAlt() ) {
			} else {
				if(ctrlAutoSearch.GetSelectedCount() == 1) {
					PostMessage(WM_COMMAND, IDC_CHANGE, 0);
				}
			}
			break;
		default:
			break;
	}
	return 0;
}


void AutoSearchFrame::addEntry(const AutoSearchPtr as, int pos) {
	if(as == NULL)
		return;
		
	if(pos < 0)
		pos = 0;


	TStringList lst;

	lst.push_back(Text::toT(as->getSearchString()));
	auto ft = SearchManager::isDefaultTypeStr(as->getFileType()) ? SearchManager::getTypeStr(as->getFileType()[0]-'0') : as->getFileType();
	lst.push_back(Text::toT(ft));
		
	if(as->getAction() == 0){
		lst.push_back(CTSTRING(DOWNLOAD));
	}else if(as->getAction() == 1){
		lst.push_back(CTSTRING(ADD_TO_QUEUE));
	}else if(as->getAction() == 2){
		lst.push_back(CTSTRING(AS_REPORT));
	}
		
	lst.push_back(Text::toT(as->getTarget()));
	lst.push_back(Text::toT(as->getRemove()? "Yes" : "No"));
	lst.push_back(Text::toT(as->getNickPattern()));
	lst.push_back((as->getLastSearch() > 0 ? formatSearchDate(as->getLastSearch()).c_str() : _T("Unknown")));

	bool b = as->getEnabled();
	int i = ctrlAutoSearch.insert(pos, lst, 0, (LPARAM)as.get());
	ctrlAutoSearch.SetCheckState(i, b);
}

void AutoSearchFrame::updateItem(const AutoSearchPtr as, int pos) {
	ctrlAutoSearch.SetItemText(pos, COLUMN_LASTSEARCH, (as->getLastSearch() > 0 ? formatSearchDate(as->getLastSearch()).c_str() : _T("Unknown")));
	ctrlAutoSearch.SetItemText(pos, COLUMN_TYPE, Text::toT(as->getDisplayType()).c_str());
}

tstring AutoSearchFrame::formatSearchDate(const time_t aTime) {
	char buf[20];
	if (strftime(buf, 20, "%x %X", localtime(&aTime))) {
		return Text::toT(string(buf));
	} else {
		return _T("-");
	}
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
			ctrlAutoSearch, 
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