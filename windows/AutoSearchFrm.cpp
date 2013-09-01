/*
 * Copyright (C) 2012-2013 AirDC++ Project
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
#include "MainFrm.h"

#include "AutoSearchFrm.h"
#include "ResourceLoader.h"
#include "SearchFrm.h"

#include "../client/SettingsManager.h"

int AutoSearchFrame::columnIndexes[] = { COLUMN_VALUE, COLUMN_TYPE, COLUMN_SEARCH_STATUS, COLUMN_LASTSEARCH, COLUMN_BUNDLES, COLUMN_ACTION, COLUMN_EXPIRATION,
	COLUMN_PATH, COLUMN_REMOVE, COLUMN_USERMATCH, COLUMN_ERROR };

int AutoSearchFrame::columnSizes[] = { 300, 125, 150, 125, 500, 100, 100, 300, 100, 200, 300 };
static ResourceManager::Strings columnNames[] = { ResourceManager::SETTINGS_VALUE, ResourceManager::TYPE, ResourceManager::SEARCHING_STATUS, ResourceManager::LAST_SEARCH, 
ResourceManager::BUNDLES, ResourceManager::ACTION, ResourceManager::EXPIRATION, ResourceManager::PATH, ResourceManager::AUTO_REMOVE, ResourceManager::USER_MATCH, ResourceManager::LAST_ERROR };

LRESULT AutoSearchFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	
	ctrlAutoSearch.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_AUTOSEARCH);
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
	ctrlAutoSearch.SetImageList(ResourceLoader::getAutoSearchStatuses(), LVSIL_SMALL);


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

	ctrlDuplicate.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_DUPLICATE);
	ctrlDuplicate.SetWindowText(CTSTRING(DUPLICATE));
	ctrlDuplicate.SetFont(WinUtil::systemFont);

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

	rc.OffsetRect(button_width+2, 0);
	ctrlDuplicate.MoveWindow(rc);
	
	//add a small space between these buttons
	rc.OffsetRect(10 + button_width +2, 0);
	ctrlUp.MoveWindow(rc);

	rc.OffsetRect(button_width+2, 0);
	ctrlDown.MoveWindow(rc);

}

LRESULT AutoSearchFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)pnmh;

	switch(cd->nmcd.dwDrawStage) {
		case CDDS_PREPAINT:
			return CDRF_NOTIFYITEMDRAW;

		case CDDS_ITEMPREPAINT:
			{
				auto status = ((AutoSearch*)cd->nmcd.lItemlParam)->getStatus();
				if(status == AutoSearch::STATUS_FAILED_EXTRAS || status == AutoSearch::STATUS_FAILED_MISSING || !((AutoSearch*)cd->nmcd.lItemlParam)->getLastError().empty()) {
					cd->clrText = SETTING(ERROR_COLOR);
					return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
				}		
			}
			return CDRF_NOTIFYSUBITEMDRAW;
		default:
			return CDRF_DODEFAULT;
	}
}	

LRESULT AutoSearchFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	if(reinterpret_cast<HWND>(wParam) == ctrlAutoSearch) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		
		OMenu asMenu;
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

		auto index = WinUtil::getFirstSelectedIndex(ctrlAutoSearch);
		if(ctrlAutoSearch.GetSelectedCount() > 1) {
			asMenu.appendItem(TSTRING(ENABLE_AUTOSEARCH), [=] { handleState(false); });
			asMenu.appendItem(TSTRING(DISABLE_AUTOSEARCH), [=] { handleState(true); });
			asMenu.appendSeparator();
			//only remove and add is enabled
		} else if(ctrlAutoSearch.GetSelectedCount() == 1) {
			asMenu.appendItem(TSTRING(SEARCH), [=] { handleSearch(true); });
			asMenu.appendItem(TSTRING(SEARCH_FOREGROUND), [=] { handleSearch(false); });
			asMenu.appendSeparator();
			
			if(ctrlAutoSearch.GetCheckState(index) == 1) {
				asMenu.appendItem(TSTRING(DISABLE_AUTOSEARCH), [=] { handleState(true); });
			} else {
				asMenu.appendItem(TSTRING(ENABLE_AUTOSEARCH), [=] { handleState(false); });
			}
			asMenu.appendSeparator();
		}
		
		asMenu.AppendMenu(MF_STRING, IDC_ADD, CTSTRING(ADD));
		asMenu.AppendMenu(MF_STRING, IDC_CHANGE, CTSTRING(SETTINGS_CHANGE));
		asMenu.AppendMenu(MF_STRING, IDC_DUPLICATE, CTSTRING(DUPLICATE));
		asMenu.AppendMenu(MF_STRING, IDC_MOVE_UP, CTSTRING(SETTINGS_BTN_MOVEUP));
		asMenu.AppendMenu(MF_STRING, IDC_MOVE_DOWN, CTSTRING(SETTINGS_BTN_MOVEDOWN));

		tstring title;
		if (ctrlAutoSearch.GetSelectedCount() == 1) {
			BundleList bundles;
			AutoSearch::FinishedPathMap fpl;
			auto as = AutoSearchManager::getInstance()->getSearchByIndex(index);
			title = Text::toT(as->getDisplayName());


			if (as->usingIncrementation()) {
				asMenu.appendSeparator();
				asMenu.appendItem(TSTRING(INCREASE_NUM), [=] { 
					AutoSearchManager::getInstance()->changeNumber(as, true);
				}, (as->getMaxNumber() > 0 && as->getCurNumber() >= as->getMaxNumber()) ? OMenu::FLAG_DISABLED : 0);
				asMenu.appendItem(TSTRING(DECREASE_NUM), [=] { 
					AutoSearchManager::getInstance()->changeNumber(as, false);
				}, as->getCurNumber() == 0 ? OMenu::FLAG_DISABLED : 0);
			}

			AutoSearchManager::getInstance()->getMenuInfo(as, bundles, fpl);
			if (!bundles.empty() || !fpl.empty()) {
				asMenu.appendSeparator();
				if (!bundles.empty()) {
					auto bundleMenu = asMenu.createSubMenu(CTSTRING(REMOVE_BUNDLE), true);
					for(auto& b: bundles) {
						string token = b->getToken();
						bundleMenu->appendItem(Text::toT(b->getName()), [=] { WinUtil::removeBundle(token); });
					}
				}

				auto pathMenu = asMenu.createSubMenu(CTSTRING(OPEN_FOLDER), false);
				if (!bundles.empty()) {
					pathMenu->InsertSeparatorFirst(CTSTRING(QUEUED_BUNDLES));
					for(auto& b: bundles) {
						string path = b->getTarget();
						pathMenu->appendItem(Text::toT(path), [=] { WinUtil::openFolder(Text::toT(path)); });
					}
				}

				if (!fpl.empty()) {
					pathMenu->InsertSeparatorLast(CTSTRING(FINISHED_BUNDLES));
					for(auto j=fpl.begin(); j != fpl.end(); j++) {
						string path = j->first;
						pathMenu->appendItem(Text::toT(path) + (j->second > 0 ? _T(" (") + Text::toT(Util::formatTime("%Y-%m-%d %H:%M", j->second)) + _T(")") : Util::emptyStringT), [=] { WinUtil::openFolder(Text::toT(path)); });
					}

					pathMenu->appendSeparator();
					pathMenu->appendItem(CTSTRING(CLEAR_FINISHED_PATHS), [=] { AutoSearchManager::getInstance()->clearPaths(as); });
				}
			}

			if (!as->getLastError().empty()) {
				asMenu.appendSeparator();
				asMenu.appendItem(TSTRING(CLEAR_ERROR), [=] { 
					as->setLastError(Util::emptyString); 
					updateItem(as);
				});
			}
		}

		asMenu.AppendMenu(MF_SEPARATOR);
		asMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));

		asMenu.EnableMenuItem(IDC_CHANGE, enable);
		asMenu.EnableMenuItem(IDC_MOVE_UP, enable);
		asMenu.EnableMenuItem(IDC_MOVE_DOWN, enable);
		
		asMenu.SetMenuDefaultItem(IDC_CHANGE);

		//make a menu title from the search string, its probobly too long to fit but atleast it shows something.
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



void AutoSearchFrame::appendDialogParams(const AutoSearchPtr& as, AutoSearchDlg& dlg) {
	dlg.searchString = as->getSearchString();
	dlg.excludedWords = as->getExcludedString();
	dlg.fileTypeStr = as->getFileType();
	dlg.action = as->getAction();
	dlg.remove = as->getRemove();
	dlg.target = as->getTarget();
	dlg.userMatch = as->getNickPattern();
	dlg.matcherString = as->getMatcherString();
	dlg.matcherType = as->getMethod();
	dlg.expireTime = as->getExpireTime();
	dlg.searchDays = as->searchDays;
	dlg.startTime = as->startTime;
	dlg.endTime = as->endTime;
	dlg.targetType = as->getTargetType();
	dlg.checkQueued = as->getCheckAlreadyQueued();
	dlg.checkShared = as->getCheckAlreadyShared();
	dlg.matchFullPath = as->getMatchFullPath();

	dlg.curNumber = as->getCurNumber();
	dlg.numberLen = as->getNumberLen();
	dlg.maxNumber = as->getMaxNumber();
	dlg.useParams = as->getUseParams();
}

void AutoSearchFrame::setItemProperties(AutoSearchPtr& as, const AutoSearchDlg& dlg, const string& aSearchString) {
	as->setSearchString(aSearchString);
	as->setExcludedString(dlg.excludedWords);
	as->setFileType(dlg.fileTypeStr);
	as->setAction((AutoSearch::ActionType)dlg.action);
	as->setRemove(dlg.remove);
	as->setTargetType(dlg.targetType);
	as->setTarget(dlg.target);
	as->setMethod((StringMatch::Method)dlg.matcherType);
	as->setMatcherString(dlg.matcherString);
	as->setUserMatcher(dlg.userMatch);
	as->setExpireTime(dlg.expireTime);
	as->setCheckAlreadyQueued(dlg.checkQueued);
	as->setCheckAlreadyShared(dlg.checkShared);
	as->setMatchFullPath(dlg.matchFullPath);

	if (as->getCurNumber() != dlg.curNumber)
		as->setLastIncFinish(0);

	as->startTime = dlg.startTime;
	as->endTime = dlg.endTime;
	as->searchDays = dlg.searchDays;

	as->setCurNumber(dlg.curNumber);
	as->setMaxNumber(dlg.maxNumber);
	as->setNumberLen(dlg.numberLen);
	as->setUseParams(dlg.useParams);
}

LRESULT AutoSearchFrame::onAdd(WORD , WORD , HWND , BOOL& ) {
	AutoSearchDlg dlg;
	dlg.expireTime = SETTING(AUTOSEARCH_EXPIRE_DAYS) > 0 ? GET_TIME() + (SETTING(AUTOSEARCH_EXPIRE_DAYS)*24*60*60) : 0;
	dlg.fileTypeStr = SETTING(LAST_AS_FILETYPE);
	if(dlg.DoModal() == IDOK) {
		SettingsManager::getInstance()->set(SettingsManager::LAST_AS_FILETYPE, dlg.fileTypeStr);
		addFromDialog(dlg);
	}
	return 0;
}

LRESULT AutoSearchFrame::onDuplicate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlAutoSearch.GetSelectedCount() == 1) {
		int sel = ctrlAutoSearch.GetNextItem(-1, LVNI_SELECTED);
		AutoSearchPtr as = AutoSearchManager::getInstance()->getSearchByIndex(sel);

		AutoSearchDlg dlg(as);
		appendDialogParams(as, dlg);

		if(dlg.DoModal() == IDOK) {
			addFromDialog(dlg);
		}
	}
	return 0;
}

void AutoSearchFrame::addFromDialog(const AutoSearchDlg& dlg) {
	string search = dlg.searchString + "\r\n";
	string::size_type j = 0;
	string::size_type i = 0;
	
	while((i = search.find("\r\n", j)) != string::npos) {
		string str = search.substr(j, i-j);
		j = i +2;
		if(str.size() >= 5) { //dont accept shorter search strings than 5 chars
			AutoSearchPtr as = new AutoSearch;

			setItemProperties(as, dlg, str);
			AutoSearchManager::getInstance()->addAutoSearch(as, false);
		} else if(search.size() < 5) { // dont report if empty line between/end when adding multiple
			MessageBox(CTSTRING(LINE_EMPTY_OR_TOO_SHORT));
		}
	}
}

LRESULT AutoSearchFrame::onChange(WORD , WORD , HWND , BOOL& ) {
	if(ctrlAutoSearch.GetSelectedCount() == 1) {
		int sel = ctrlAutoSearch.GetNextItem(-1, LVNI_SELECTED);
		AutoSearchPtr as = AutoSearchManager::getInstance()->getSearchByIndex(sel);

		AutoSearchDlg dlg(as);
		appendDialogParams(as, dlg);

		if(dlg.DoModal() == IDOK) {
			setItemProperties(as, dlg, dlg.searchString);
			if (AutoSearchManager::getInstance()->updateAutoSearch(as)) {
				ctrlAutoSearch.DeleteItem(sel);
				addEntry(as, sel);
				ctrlAutoSearch.SelectItem(sel);
			}
		}
	}
	return 0;
}

LRESULT AutoSearchFrame::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	//use a removelist, the selection position will change when removing multiple.
	AutoSearchList removelist;

	int i = -1;
	while( (i = ctrlAutoSearch.GetNextItem(i, LVNI_SELECTED)) != -1) {
		removelist.push_back((AutoSearch*)ctrlAutoSearch.GetItemData(i));
	}

	if(WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_AS_REMOVAL, TSTRING(REALLY_REMOVE))) {
		for(auto a = removelist.begin(); a !=removelist.end(); ++a )
			AutoSearchManager::getInstance()->removeAutoSearch(*a);
	}
	return 0;
}



LRESULT AutoSearchFrame::onMoveUp(WORD , WORD , HWND , BOOL& ) {
	int i = ctrlAutoSearch.GetNextItem(-1, LVNI_SELECTED);
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
	int i = ctrlAutoSearch.GetNextItem(-1, LVNI_SELECTED);
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
void AutoSearchFrame::handleSearch(bool onBackground) {
	if(ctrlAutoSearch.GetSelectedCount() == 1) {
		int sel = ctrlAutoSearch.GetNextItem(-1, LVNI_SELECTED);
		MainFrame::getMainFrame()->addThreadedTask([=] {
			AutoSearchPtr as = AutoSearchManager::getInstance()->getSearchByIndex(sel);
			if (as) {
				AutoSearchManager::getInstance()->searchItem(as, onBackground ? AutoSearchManager::TYPE_MANUAL_BG : AutoSearchManager::TYPE_MANUAL_FG);
			}
		});
	}
}

void AutoSearchFrame::on(AutoSearchManagerListener::SearchForeground, const AutoSearchPtr& as, const string& searchString) noexcept {
	callAsync([=] { SearchFrame::openWindow(Text::toT(searchString), 0, SearchManager::SIZE_DONTCARE, as->getFileType()); });
}

void AutoSearchFrame::handleState(bool disabled) {
	//just set the checkstate, onitemchanged will handle it from there
	int i = -1;
	while ((i = ctrlAutoSearch.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ctrlAutoSearch.SetCheckState(i, disabled ? FALSE : TRUE);
	}
}

LRESULT AutoSearchFrame::onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* l = (NMITEMACTIVATE*)pnmh;
	
	::EnableWindow(GetDlgItem(IDC_REMOVE), (ctrlAutoSearch.GetSelectedCount() >= 1));
	::EnableWindow(GetDlgItem(IDC_CHANGE), (ctrlAutoSearch.GetSelectedCount() == 1));
	::EnableWindow(GetDlgItem(IDC_DUPLICATE), (ctrlAutoSearch.GetSelectedCount() == 1));
	::EnableWindow(GetDlgItem(IDC_MOVE_UP), (ctrlAutoSearch.GetSelectedCount() == 1));
	::EnableWindow(GetDlgItem(IDC_MOVE_DOWN), (ctrlAutoSearch.GetSelectedCount() == 1));
	
	if(!loading && l->iItem != -1 && ((l->uNewState & LVIS_STATEIMAGEMASK) != (l->uOldState & LVIS_STATEIMAGEMASK))) {
		AutoSearchPtr as = AutoSearchManager::getInstance()->getSearchByIndex(l->iItem);
		if (as) {
			AutoSearchManager::getInstance()->setItemActive(as, Util::toBool(ctrlAutoSearch.GetCheckState(l->iItem)));
		}	
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

	lst.push_back(Text::toT(as->getDisplayName()));
	lst.push_back(Text::toT(SearchManager::isDefaultTypeStr(as->getFileType()) ? SearchManager::getTypeStr(as->getFileType()[0]-'0') : as->getFileType()));
	lst.push_back(Text::toT(as->getSearchingStatus()));
	lst.push_back((as->getLastSearch() > 0 ? formatSearchDate(as->getLastSearch()).c_str() : _T("Unknown")));
	lst.push_back(Text::toT(AutoSearchManager::getInstance()->getBundleStatuses(as)));
		
	if(as->getAction() == 0){
		lst.push_back(CTSTRING(DOWNLOAD));
	}else if(as->getAction() == 1){
		lst.push_back(CTSTRING(ADD_TO_QUEUE));
	}else if(as->getAction() == 2){
		lst.push_back(CTSTRING(AS_REPORT));
	}
		
	lst.push_back(Text::toT(as->getExpiration()));

	string target = as->getTarget();
	if (target.empty()) {
		target = CSTRING(SETTINGS_DOWNLOAD_DIRECTORY);
	} else if (as->getTargetType() == TargetUtil::TARGET_FAVORITE) {
		target += " (" + Text::toLower(STRING(SETTINGS_COLOR_FAVORITE)) +  ")";
	} else if (as->getTargetType() == TargetUtil::TARGET_SHARE) {
		target += " (" + Text::toLower(STRING(SHARED)) +  ")";
	}

	lst.push_back(Text::toT(target));
	lst.push_back(Text::toT(as->getRemove()? "Yes" : "No"));
	lst.push_back(Text::toT(as->getNickPattern()));
	lst.push_back(Text::toT(as->getLastError()));

	bool b = as->getEnabled();


	//don't disable/enable the item here
	loading = true;
	int i = ctrlAutoSearch.insert(pos, lst, as->getStatus(), (LPARAM)as.get());
	loading = false;

	ctrlAutoSearch.SetCheckState(i, b);
}

int AutoSearchFrame::findItem(ProfileToken aToken) {
	auto itemCount = ctrlAutoSearch.GetItemCount();
	for(int pos = 0; pos < itemCount; ++pos) {
		auto as = (AutoSearch*)ctrlAutoSearch.GetItemData(pos);
		if(aToken == as->getToken()) {
			return pos;
		}
	}

	return -1;
}

void AutoSearchFrame::updateItem(const AutoSearchPtr as) {
	int pos = findItem(as->getToken());
	if (pos >= 0) {
		ctrlAutoSearch.SetItemText(pos, COLUMN_VALUE, Text::toT(as->getDisplayName()).c_str());
		ctrlAutoSearch.SetItemText(pos, COLUMN_LASTSEARCH, (as->getLastSearch() > 0 ? formatSearchDate(as->getLastSearch()).c_str() : _T("Unknown")));
		ctrlAutoSearch.SetItemText(pos, COLUMN_TYPE, Text::toT(as->getDisplayType()).c_str());
		ctrlAutoSearch.SetItemText(pos, COLUMN_BUNDLES, Text::toT(AutoSearchManager::getInstance()->getBundleStatuses(as)).c_str());
		ctrlAutoSearch.SetItemText(pos, COLUMN_SEARCH_STATUS, Text::toT(as->getSearchingStatus()).c_str());
		ctrlAutoSearch.SetItemText(pos, COLUMN_EXPIRATION, Text::toT(as->getExpiration()).c_str());
		ctrlAutoSearch.SetItemText(pos, COLUMN_ERROR, Text::toT(as->getLastError()).c_str());

		ctrlAutoSearch.SetItem(pos, 0 ,LVIF_IMAGE, NULL, as->getStatus(), 0, 0, NULL);
	}
}

void AutoSearchFrame::removeItem(const AutoSearchPtr as) {
	int pos = findItem(as->getToken());
	if (pos >= 0) {
		ctrlAutoSearch.DeleteItem(pos);
	}
}

void AutoSearchFrame::on(AutoSearchManagerListener::RemoveItem, const AutoSearchPtr& as) noexcept { 
	callAsync([=] { removeItem(as); if(SETTING(AUTOSEARCH_BOLD)) setDirty(); }); 
}

void AutoSearchFrame::on(AutoSearchManagerListener::AddItem, const AutoSearchPtr& as) noexcept { 
	callAsync([=] { addEntry(as, ctrlAutoSearch.GetItemCount()); if(SETTING(AUTOSEARCH_BOLD)) setDirty();  });
}

void AutoSearchFrame::on(AutoSearchManagerListener::UpdateItem, const AutoSearchPtr& as, bool aSetDirty) noexcept {
	callAsync([=] { updateItem(as); if (aSetDirty && SETTING(AUTOSEARCH_BOLD)) setDirty();  }); 
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