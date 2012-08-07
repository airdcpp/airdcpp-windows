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
#include "Resource.h"

#include "PublicHubsFrm.h"
#include "HubFrame.h"
#include "WinUtil.h"
#include "PublicHubsListDlg.h"

#include "../client/Client.h"
#include "../client/StringTokenizer.h"
#include "../client/version.h"
#include "../client/Localization.h"

int PublicHubsFrame::columnIndexes[] = { 
	COLUMN_NAME,
	COLUMN_DESCRIPTION,
	COLUMN_USERS,
	COLUMN_SERVER,
	COLUMN_COUNTRY,
	COLUMN_SHARED,
	COLUMN_MINSHARE,
	COLUMN_MINSLOTS,
	COLUMN_MAXHUBS,
	COLUMN_MAXUSERS,
	COLUMN_RELIABILITY,
	COLUMN_RATING
 };

int PublicHubsFrame::columnSizes[] = { 200, 290, 50, 100, 100, 100, 100, 100, 100, 100, 100, 100 };

static ResourceManager::Strings columnNames[] = { 
	ResourceManager::HUB_NAME, 
	ResourceManager::DESCRIPTION, 
	ResourceManager::USERS, 
	ResourceManager::HUB_ADDRESS,
	ResourceManager::COUNTRY,
	ResourceManager::SHARED,
	ResourceManager::MIN_SHARE,
	ResourceManager::MIN_SLOTS,
	ResourceManager::MAX_HUBS,
	ResourceManager::MAX_USERS,
	ResourceManager::RELIABILITY,
	ResourceManager::RATING,
	
};

LRESULT PublicHubsFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	int w[3] = { 0, 0, 0};
	ctrlStatus.SetParts(3, w);
	
	ctrlHubs.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_HUBLIST);
	ctrlHubs.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT| LVS_EX_DOUBLEBUFFER  | LVS_EX_INFOTIP);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(PUBLICHUBSFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(PUBLICHUBSFRAME_WIDTHS), COLUMN_LAST);
	
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_USERS) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlHubs.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlHubs.SetColumnOrderArray(COLUMN_LAST, columnIndexes);
	
	ctrlHubs.SetBkColor(WinUtil::bgColor);
	ctrlHubs.SetTextBkColor(WinUtil::bgColor);
	ctrlHubs.SetTextColor(WinUtil::textColor);
	
	ctrlHubs.setSort(COLUMN_USERS, ExListViewCtrl::SORT_INT, false);
	ctrlHubs.SetImageList(WinUtil::flagImages, LVSIL_SMALL);
	ctrlHubs.SetFocus();

	ctrlConfigure.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_PUB_LIST_CONFIG);
	ctrlConfigure.SetWindowText(CTSTRING(CONFIGURE));
	ctrlConfigure.SetFont(WinUtil::systemFont);

	ctrlRefresh.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_REFRESH);
	ctrlRefresh.SetWindowText(CTSTRING(REFRESH));
	ctrlRefresh.SetFont(WinUtil::systemFont);

	ctrlLists.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_GROUPBOX, WS_EX_TRANSPARENT);
	ctrlLists.SetFont(WinUtil::systemFont);
	ctrlLists.SetWindowText(CTSTRING(CONFIGURED_HUB_LISTS));

	ctrlPubLists.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE, IDC_PUB_LIST_DROPDOWN);
	ctrlPubLists.SetFont(WinUtil::systemFont, FALSE);
	// populate with values from the settings
	updateDropDown();
	
	ctrlFilter.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);
	filterContainer.SubclassWindow(ctrlFilter.m_hWnd);
	ctrlFilter.SetFont(WinUtil::systemFont);
	
	ctrlFilterSel.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE);
	ctrlFilterSel.SetFont(WinUtil::systemFont, FALSE);

	//populate the filter list with the column names
	for(int j=0; j<COLUMN_LAST; j++) {
		ctrlFilterSel.AddString(CTSTRING_I(columnNames[j]));
	}
	ctrlFilterSel.AddString(CTSTRING(ANY));
	ctrlFilterSel.SetCurSel(COLUMN_LAST);
	
	ctrlFilterDesc.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_GROUPBOX, WS_EX_TRANSPARENT);
	ctrlFilterDesc.SetWindowText(CTSTRING(FILTER));
	ctrlFilterDesc.SetFont(WinUtil::systemFont);

	FavoriteManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	hubs = FavoriteManager::getInstance()->getPublicHubs();
	if(FavoriteManager::getInstance()->isDownloading()) 
		ctrlStatus.SetText(0, CTSTRING(DOWNLOADING_HUB_LIST));
	else if(hubs.empty())
		FavoriteManager::getInstance()->refresh();

	updateList();
	
	hubsMenu.CreatePopupMenu();
	hubsMenu.AppendMenu(MF_STRING, IDC_CONNECT, CTSTRING(CONNECT));
	hubsMenu.AppendMenu(MF_STRING, IDC_CONNECT_WITH, CTSTRING(CONNECT_WITH_PROFILE));
	hubsMenu.AppendMenu(MF_STRING, IDC_ADD, CTSTRING(ADD_TO_FAVORITES));
	hubsMenu.AppendMenu(MF_STRING, IDC_COPY_HUB, CTSTRING(COPY_HUB));
	hubsMenu.SetMenuDefaultItem(IDC_CONNECT);

	WinUtil::SetIcon(m_hWnd, _T("PublicHubs.ico"));

	bHandled = FALSE;
	return TRUE;
}

LRESULT PublicHubsFrame::onColumnClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLISTVIEW* l = (NMLISTVIEW*)pnmh;
	if(l->iSubItem == ctrlHubs.getSortColumn()) {
		if (!ctrlHubs.isAscending())
			ctrlHubs.setSort(-1, ctrlHubs.getSortType());
		else
			ctrlHubs.setSortDirection(false);
	} else {
		// BAH, sorting on bytes will break of course...oh well...later...
		if(l->iSubItem == COLUMN_USERS || l->iSubItem == COLUMN_MINSLOTS ||l->iSubItem == COLUMN_MAXHUBS || l->iSubItem == COLUMN_MAXUSERS) {
			ctrlHubs.setSort(l->iSubItem, ExListViewCtrl::SORT_INT);
		} else if(l->iSubItem == COLUMN_RELIABILITY) {
			ctrlHubs.setSort(l->iSubItem, ExListViewCtrl::SORT_FLOAT);
		} else if (l->iSubItem == COLUMN_SHARED || l->iSubItem == COLUMN_MINSHARE){
			ctrlHubs.setSort(l->iSubItem, ExListViewCtrl::SORT_BYTES);
		} else {
			ctrlHubs.setSort(l->iSubItem, ExListViewCtrl::SORT_STRING_NOCASE);
		}
	}
	return 0;
}
	
LRESULT PublicHubsFrame::onDoubleClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	if(!checkNick())
		return 0;
	
	NMITEMACTIVATE* item = (NMITEMACTIVATE*) pnmh;

	if(item->iItem != -1) {
		TCHAR buf[256];
		
		RecentHubEntry r;
		ctrlHubs.GetItemText(item->iItem, COLUMN_NAME, buf, 256);
		r.setName(Text::fromT(buf));
		ctrlHubs.GetItemText(item->iItem, COLUMN_DESCRIPTION, buf, 256);
		r.setDescription(Text::fromT(buf));
		ctrlHubs.GetItemText(item->iItem, COLUMN_USERS, buf, 256);
		r.setUsers(Text::fromT(buf));
		ctrlHubs.GetItemText(item->iItem, COLUMN_SHARED, buf, 256);
		r.setShared(Text::fromT(buf));
		ctrlHubs.GetItemText(item->iItem, COLUMN_SERVER, buf, 256);
		r.setServer(Text::fromT(buf));
		FavoriteManager::getInstance()->addRecent(r);
		HubFrame::openWindow(buf);
	}

	return 0;
}

LRESULT PublicHubsFrame::onEnter(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
	if(!checkNick())
		return 0;

	int item = ctrlHubs.GetNextItem(-1, LVNI_FOCUSED);
	if(item != -1) {
		TCHAR buf[256];

		ctrlHubs.GetItemText(item, COLUMN_SERVER, buf, 256);
		HubFrame::openWindow(buf);
	}

	return 0;
}

LRESULT PublicHubsFrame::onConnectWith(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/) {
	if(!checkNick())
		return 0;

	int item = ctrlHubs.GetNextItem(-1, LVNI_FOCUSED);
	if(item != -1) {
		TCHAR buf[256];

		ctrlHubs.GetItemText(item, COLUMN_SERVER, buf, 256);

		ConnectDlg dlg(true);
		dlg.title = TSTRING(CONNECT_WITH_PROFILE);
		dlg.address = buf;
		if(dlg.DoModal(m_hWnd) == IDOK){
			HubFrame::openWindow(buf, 0, true, dlg.curProfile);
		}
	}

	return 0;
}

LRESULT PublicHubsFrame::onClickedRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlStatus.SetText(0, CTSTRING(DOWNLOADING_HUB_LIST));
	FavoriteManager::getInstance()->refresh(true);
	updateDropDown();
	return 0;
}

LRESULT PublicHubsFrame::onClickedConfigure(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PublicHubListDlg dlg;
	if(dlg.DoModal(m_hWnd) == IDOK) {
		updateDropDown();
	}
	return 0;
}

LRESULT PublicHubsFrame::onClickedConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!checkNick())
		return 0;

	if(ctrlHubs.GetSelectedCount() >= 1) {
		TCHAR buf[256];
		int i = -1;
		while( (i = ctrlHubs.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ctrlHubs.GetItemText(i, COLUMN_SERVER, buf, 256);

		RecentHubEntry r;
		ctrlHubs.GetItemText(i, COLUMN_NAME, buf, 256);
		r.setName(Text::fromT(buf));
		ctrlHubs.GetItemText(i, COLUMN_DESCRIPTION, buf, 256);
		r.setDescription(Text::fromT(buf));
		ctrlHubs.GetItemText(i, COLUMN_USERS, buf, 256);
		r.setUsers(Text::fromT(buf));
		ctrlHubs.GetItemText(i, COLUMN_SHARED, buf, 256);
		r.setShared(Text::fromT(buf));
		ctrlHubs.GetItemText(i, COLUMN_SERVER, buf, 256);
		r.setServer(Text::fromT(buf));
		FavoriteManager::getInstance()->addRecent(r);
				
		HubFrame::openWindow(buf);
	}
	}
	return 0;
}

LRESULT PublicHubsFrame::onFilterFocus(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	bHandled = true;
	ctrlFilter.SetFocus();
	return 0;
}

LRESULT PublicHubsFrame::onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!checkNick())
		return 0;
	
	TCHAR buf[256];
	
	if(ctrlHubs.GetSelectedCount() >= 1) {
		int i = ctrlHubs.GetNextItem(-1, LVNI_SELECTED);

		FavoriteHubEntry e;
		ctrlHubs.GetItemText(i, COLUMN_NAME, buf, 256);
		e.setName(Text::fromT(buf));

		ctrlHubs.GetItemText(i, COLUMN_DESCRIPTION, buf, 256);
		e.setDescription(Text::fromT(buf));

		ctrlHubs.GetItemText(i, COLUMN_SERVER, buf, 256);
		e.setServer(Text::fromT(buf));

		FavoriteManager::getInstance()->addFavorite(e);
	}
	return 0;
}

LRESULT PublicHubsFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		FavoriteManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		closed = true;
		WinUtil::setButtonPressed(ID_FILE_CONNECT, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		WinUtil::saveHeaderOrder(ctrlHubs, SettingsManager::PUBLICHUBSFRAME_ORDER,
		SettingsManager::PUBLICHUBSFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);
		bHandled = FALSE;
		return 0;
	}
}
	
LRESULT PublicHubsFrame::onListSelChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	FavoriteManager::getInstance()->setHubList(ctrlPubLists.GetCurSel());
	hubs = FavoriteManager::getInstance()->getPublicHubs();
	updateList();
	bHandled = FALSE;
	return 0;
}

void PublicHubsFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	
	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[3];
		ctrlStatus.GetClientRect(sr);
		int tmp = (sr.Width()) > 316 ? 216 : ((sr.Width() > 116) ? sr.Width()-100 : 16);
		
		w[0] = sr.right - tmp;
		w[1] = w[0] + (tmp-16)/2;
		w[2] = w[0] + (tmp);
		
		ctrlStatus.SetParts(3, w);
	}
	
	int const comboH = 140;

	// listview
	CRect rc = rect;
	rc.top += 2;
	rc.bottom -=(56);
	ctrlHubs.MoveWindow(rc);

	// filter box
	rc = rect;
	rc.top = rc.bottom - 52;
	rc.bottom = rc.top + 46;
	rc.right -= 100;
	rc.right -= ((rc.right - rc.left) / 2) + 1;
	ctrlFilterDesc.MoveWindow(rc);

	// filter edit
	rc.top += 16;
	rc.bottom -= 8;
	rc.left += 8;
	rc.right -= ((rc.right - rc.left - 4) / 3);
	ctrlFilter.MoveWindow(rc);

	//filter sel
	rc.bottom += comboH;
	rc.right += ((rc.right - rc.left - 12) / 2) ;
	rc.left += ((rc.right - rc.left + 8) / 3) * 2;
	ctrlFilterSel.MoveWindow(rc);

	// lists box
	rc = rect;
	rc.top = rc.bottom - 52;
	rc.bottom = rc.top + 46;
	rc.right -= 100;
	rc.left += ((rc.right - rc.left) / 2) + 1;
	ctrlLists.MoveWindow(rc);
	
	// lists dropdown
	rc.top += 16;
	rc.bottom -= 8 - comboH;
	rc.right -= 8 + 100;
	rc.left += 8;
	ctrlPubLists.MoveWindow(rc);
	
	// configure button
	rc.left = rc.right + 4;
	rc.bottom -= comboH;
	rc.right += 100;
	ctrlConfigure.MoveWindow(rc);

	// refresh button
	rc = rect;
	rc.bottom -= 2 + 8 + 4;
	rc.top = rc.bottom - 22;
	rc.left = rc.right - 96;
	rc.right -= 2;
	ctrlRefresh.MoveWindow(rc);
}

bool PublicHubsFrame::checkNick() {
	if(SETTING(NICK).empty()) {
		MessageBox(CTSTRING(ENTER_NICK), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
		return false;
	}
	return true;
}

void PublicHubsFrame::updateList() {
	ctrlHubs.DeleteAllItems();
	users = 0;
	visibleHubs = 0;
	
	ctrlHubs.SetRedraw(FALSE);
	
	double size = -1;
	FilterModes mode = NONE;

	int sel = ctrlFilterSel.GetCurSel();

	bool doSizeCompare = parseFilter(mode, size);
	
	for(HubEntry::List::const_iterator i = hubs.begin(); i != hubs.end(); ++i) {
		if(matchFilter(*i, sel, doSizeCompare, mode, size)) {

			TStringList l;
			l.resize(COLUMN_LAST);
			l[COLUMN_NAME] = Text::toT(i->getName());
			l[COLUMN_DESCRIPTION] = Text::toT(i->getDescription());
			l[COLUMN_USERS] = Util::toStringW(i->getUsers());
			l[COLUMN_SERVER] = Text::toT(i->getServer());
			l[COLUMN_COUNTRY] = Text::toT(i->getCountry());
			l[COLUMN_SHARED] = Util::formatBytesW(i->getShared());
			l[COLUMN_MINSHARE] = Util::formatBytesW(i->getMinShare());
			l[COLUMN_MINSLOTS] = Util::toStringW(i->getMinSlots());
			l[COLUMN_MAXHUBS] = Util::toStringW(i->getMaxHubs());
			l[COLUMN_MAXUSERS] = Util::toStringW(i->getMaxUsers());
			l[COLUMN_RELIABILITY] = Util::toStringW(i->getReliability());
			l[COLUMN_RATING] = Text::toT(i->getRating());
			ctrlHubs.insert(ctrlHubs.GetItemCount(), l, Localization::getFlagIndexByName(i->getCountry().c_str()));
			visibleHubs++;
			users += i->getUsers();
		}
	}
	
	ctrlHubs.SetRedraw(TRUE);
	ctrlHubs.resort();

	updateStatus();
}

void PublicHubsFrame::updateStatus() {
	ctrlStatus.SetText(1, (TSTRING(HUBS) + _T(": ") + Util::toStringW(visibleHubs)).c_str());
	ctrlStatus.SetText(2, (TSTRING(USERS) + _T(": ") + Util::toStringW(users)).c_str());
}

LRESULT PublicHubsFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(wParam == FINISHED) {
		hubs = FavoriteManager::getInstance()->getPublicHubs();
		updateList();
		tstring* x = (tstring*)lParam;
		ctrlStatus.SetText(0, (*x).c_str());
		delete x;
	} else if(wParam == SET_TEXT) {
		tstring* x = (tstring*)lParam;
		ctrlStatus.SetText(0, (*x).c_str());
		delete x;
	}
	return 0;
}

LRESULT PublicHubsFrame::onFilterChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	if(wParam == VK_RETURN) {
		tstring tmp;
		tmp.resize(ctrlFilter.GetWindowTextLength());
		ctrlFilter.GetWindowText(&tmp[0], tmp.size() + 1);

		filter = Text::fromT(tmp);

		updateList();
	} else {
		bHandled = FALSE;
	}
	return 0;
}

LRESULT PublicHubsFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if(reinterpret_cast<HWND>(wParam) == ctrlHubs && ctrlHubs.GetSelectedCount() >= 1) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		
		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlHubs, pt);
		}

		hubsMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		return TRUE; 
	}

	bHandled = FALSE;
	return FALSE; 
}

LRESULT PublicHubsFrame::onCopyHub(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlHubs.GetSelectedCount() == 1) {
		TCHAR buf[256];
		int i = ctrlHubs.GetNextItem(-1, LVNI_SELECTED);
			ctrlHubs.GetItemText(i, COLUMN_SERVER, buf, 256);
		WinUtil::setClipboard(buf);
	}
	return 0;
}

void PublicHubsFrame::updateDropDown() {
	ctrlPubLists.ResetContent();
	StringList lists(FavoriteManager::getInstance()->getHubLists());
	for(StringList::iterator idx = lists.begin(); idx != lists.end(); ++idx) {
		ctrlPubLists.AddString(Text::toT(*idx).c_str());
	}
	ctrlPubLists.SetCurSel(FavoriteManager::getInstance()->getSelectedHubList());
}

bool PublicHubsFrame::parseFilter(FilterModes& mode, double& size) {
	string::size_type start = (string::size_type)string::npos;
	string::size_type end = (string::size_type)string::npos;
	int64_t multiplier = 1;

	if(filter.empty()) return false;
	if(filter.compare(0, 2, ">=") == 0) {
		mode = GREATER_EQUAL;
		start = 2;
	} else if(filter.compare(0, 2, "<=") == 0) {
		mode = LESS_EQUAL;
		start = 2;
	} else if(filter.compare(0, 2, "==") == 0) {
		mode = EQUAL;
		start = 2;
	} else if(filter.compare(0, 2, "!=") == 0) {
		mode = NOT_EQUAL;
		start = 2;
	} else if(filter[0] == _T('<')) {
		mode = LESS;
		start = 1;
	} else if(filter[0] == _T('>')) {
		mode = GREATER;
		start = 1;
	} else if(filter[0] == _T('=')) {
		mode = EQUAL;
		start = 1;
	}

	if(start == string::npos)
		return false;
	if(filter.length() <= start)
		return false;

	if((end = Util::findSubString(filter, "TiB")) != tstring::npos) {
		multiplier = 1024LL * 1024LL * 1024LL * 1024LL;
	} else if((end = Util::findSubString(filter, "GiB")) != tstring::npos) {
		multiplier = 1024*1024*1024;
	} else if((end = Util::findSubString(filter, "MiB")) != tstring::npos) {
		multiplier = 1024*1024;
	} else if((end = Util::findSubString(filter, "KiB")) != tstring::npos) {
		multiplier = 1024;
	} else if((end = Util::findSubString(filter, "TB")) != tstring::npos) {
		multiplier = 1000LL * 1000LL * 1000LL * 1000LL;
	} else if((end = Util::findSubString(filter, "GB")) != tstring::npos) {
		multiplier = 1000*1000*1000;
	} else if((end = Util::findSubString(filter, "MB")) != tstring::npos) {
		multiplier = 1000*1000;
	} else if((end = Util::findSubString(filter, "kB")) != tstring::npos) {
		multiplier = 1000;
	} else if((end = Util::findSubString(filter, "B")) != tstring::npos) {
		multiplier = 1;
	}
	

	if(end == string::npos) {
		end = filter.length();
	}

	string tmpSize = filter.substr(start, end-start);
	size = Util::toDouble(tmpSize) * multiplier;

	return true;
}

bool PublicHubsFrame::matchFilter(const HubEntry& entry, const int& sel, bool doSizeCompare, const FilterModes& mode, const double& size) {
	if(filter.empty())
		return true;

	double entrySize = 0;
	string entryString = "";

	switch(sel) {
		case COLUMN_NAME: entryString = entry.getName(); doSizeCompare = false; break;
		case COLUMN_DESCRIPTION: entryString = entry.getDescription(); doSizeCompare = false; break;
		case COLUMN_USERS: entrySize = entry.getUsers(); break;
		case COLUMN_SERVER: entryString = entry.getServer(); doSizeCompare = false; break;
		case COLUMN_COUNTRY: entryString = entry.getCountry(); doSizeCompare = false; break;
		case COLUMN_SHARED: entrySize = (double)entry.getShared(); break;
		case COLUMN_MINSHARE: entrySize = (double)entry.getMinShare(); break;
		case COLUMN_MINSLOTS: entrySize = entry.getMinSlots(); break;
		case COLUMN_MAXHUBS: entrySize = entry.getMaxHubs(); break;
		case COLUMN_MAXUSERS: entrySize = entry.getMaxUsers(); break;
		case COLUMN_RELIABILITY: entrySize = entry.getReliability(); break;
		case COLUMN_RATING: entryString = entry.getRating(); doSizeCompare = false; break;
		default: break;
	}

	bool insert = false;
	if(doSizeCompare) {
		switch(mode) {
			case EQUAL: insert = (size == entrySize); break;
			case GREATER_EQUAL: insert = (size <=  entrySize); break;
			case LESS_EQUAL: insert = (size >=  entrySize); break;
			case GREATER: insert = (size < entrySize); break;
			case LESS: insert = (size > entrySize); break;
			case NOT_EQUAL: insert = (size != entrySize); break;
		}
	} else {
		if(sel >= COLUMN_LAST) {
			if( Util::findSubString(entry.getName(), filter) != string::npos ||
				Util::findSubString(entry.getDescription(), filter) != string::npos ||
				Util::findSubString(entry.getServer(), filter) != string::npos ||
				Util::findSubString(entry.getCountry(), filter) != string::npos ||
				Util::findSubString(entry.getRating(), filter) != string::npos ) {
					insert = true;
				}
		}
		if(Util::findSubString(entryString, filter) != string::npos)
			insert = true;
	}

	return insert;
}

void PublicHubsFrame::on(DownloadStarting, const string& l) noexcept { 
	speak(SET_TEXT, TSTRING(DOWNLOADING_HUB_LIST) + _T(" (") + Text::toT(l) + _T(")")); 
}

void PublicHubsFrame::on(DownloadFailed, const string& l) noexcept { 
	speak(SET_TEXT, TSTRING(DOWNLOAD_FAILED) + _T(" ") + Text::toT(l)); 
}

void PublicHubsFrame::on(DownloadFinished, const string& l, bool /*fromCoral*/) noexcept { 
	speak(FINISHED, TSTRING(HUB_LIST_DOWNLOADED) + _T(" (") + Text::toT(l) + _T(")"));
}

void PublicHubsFrame::on(LoadedFromCache, const string& l, const string& d) noexcept { 
	speak(FINISHED, TSTRING(HUB_LIST_LOADED_FROM_CACHE) + _T(" (") + Text::toT(l) + _T(")") + _T(" Download Date: ") + Text::toT(d)); 
}

void PublicHubsFrame::on(Corrupted, const string& l) noexcept {
	if (l.empty()) {
		speak(FINISHED, TSTRING(HUBLIST_CACHE_CORRUPTED));
	} else {	
		speak(FINISHED, TSTRING(HUBLIST_DOWNLOAD_CORRUPTED) + _T(" (") + Text::toT(l) + _T(")"));
	}
}

void PublicHubsFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	bool refresh = false;
	if(ctrlHubs.GetBkColor() != WinUtil::bgColor) {
		ctrlHubs.SetBkColor(WinUtil::bgColor);
		ctrlHubs.SetTextBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlHubs.GetTextColor() != WinUtil::textColor) {
		ctrlHubs.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
	updateDropDown();
}



/**
 * @file
 * $Id: PublicHubsFrm.cpp 470 2010-01-02 23:23:39Z bigmuscle $
 */
