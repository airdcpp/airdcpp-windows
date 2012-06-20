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

#include "FavoritesFrm.h"
#include "HubFrame.h"
#include "FavHubProperties.h"
#include "FavHubGroupsDlg.h"
#include "TextFrame.h"
#include "../client/ClientManager.h"
#include "../client/StringTokenizer.h"
#include "../client/version.h"

int FavoriteHubsFrame::columnIndexes[] = { COLUMN_NAME, COLUMN_DESCRIPTION, COLUMN_NICK, COLUMN_PASSWORD, COLUMN_SERVER, COLUMN_USERDESCRIPTION };
int FavoriteHubsFrame::columnSizes[] = { 200, 290, 125, 100, 100, 125 };
static ResourceManager::Strings columnNames[] = { ResourceManager::AUTO_CONNECT, ResourceManager::DESCRIPTION, 
ResourceManager::NICK, ResourceManager::PASSWORD, ResourceManager::SERVER, ResourceManager::USER_DESCRIPTION
};

LRESULT FavoriteHubsFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	ctrlHubs.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, IDC_HUBLIST);
	ctrlHubs.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);	
	ctrlHubs.SetBkColor(WinUtil::bgColor);
	ctrlHubs.SetTextBkColor(WinUtil::bgColor);
	ctrlHubs.SetTextColor(WinUtil::textColor);
	ctrlHubs.EnableGroupView(TRUE);

	LVGROUPMETRICS metrics = {0};
	metrics.cbSize = sizeof(metrics);
	metrics.mask = LVGMF_TEXTCOLOR;
	metrics.crHeader = SETTING(TEXT_GENERAL_FORE_COLOR);
	ctrlHubs.SetGroupMetrics(&metrics);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(FAVORITESFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(FAVORITESFRAME_WIDTHS), COLUMN_LAST);
	
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = LVCFMT_LEFT;
		ctrlHubs.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlHubs.SetColumnOrderArray(COLUMN_LAST, columnIndexes);
	
	ctrlConnect.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_CONNECT);
	ctrlConnect.SetWindowText(CTSTRING(CONNECT));
	ctrlConnect.SetFont(WinUtil::systemFont);

	ctrlNew.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_NEWFAV);
	ctrlNew.SetWindowText(CTSTRING(NEW));
	ctrlNew.SetFont(WinUtil::systemFont);

	ctrlProps.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_EDIT);
	ctrlProps.SetWindowText(CTSTRING(PROPERTIES));
	ctrlProps.SetFont(WinUtil::systemFont);

	ctrlRemove.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_REMOVE);
	ctrlRemove.SetWindowText(CTSTRING(REMOVE));
	ctrlRemove.SetFont(WinUtil::systemFont);

	ctrlUp.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_MOVE_UP);
	ctrlUp.SetWindowText(CTSTRING(MOVE_UP));
	ctrlUp.SetFont(WinUtil::systemFont);

	ctrlDown.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_MOVE_DOWN);
	ctrlDown.SetWindowText(CTSTRING(MOVE_DOWN));
	ctrlDown.SetFont(WinUtil::systemFont);

	ctrlManageGroups.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_MANAGE_GROUPS);
	ctrlManageGroups.SetWindowText(CTSTRING(MANAGE_GROUPS));
	ctrlManageGroups.SetFont(WinUtil::systemFont);

	FavoriteManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	fillList();


	hubsMenu.CreatePopupMenu();
	hubsMenu.AppendMenu(MF_STRING, IDC_OPEN_HUB_LOG, CTSTRING(OPEN_HUB_LOG));
	hubsMenu.AppendMenu(MF_SEPARATOR);
	hubsMenu.AppendMenu(MF_STRING, IDC_CONNECT, CTSTRING(CONNECT));
	hubsMenu.AppendMenu(MF_STRING, IDC_NEWFAV, CTSTRING(NEW));
	hubsMenu.AppendMenu(MF_STRING, IDC_MOVE_UP, CTSTRING(MOVE_UP));
	hubsMenu.AppendMenu(MF_STRING, IDC_MOVE_DOWN, CTSTRING(MOVE_DOWN));
	hubsMenu.AppendMenu(MF_SEPARATOR);
	hubsMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
	hubsMenu.AppendMenu(MF_SEPARATOR);
	hubsMenu.AppendMenu(MF_STRING, IDC_EDIT, CTSTRING(PROPERTIES));
	hubsMenu.SetMenuDefaultItem(IDC_CONNECT);

	CRect rc(SETTING(FAV_LEFT), SETTING(FAV_TOP), SETTING(FAV_RIGHT), SETTING(FAV_BOTTOM));
	if(! (rc.top == 0 && rc.bottom == 0 && rc.left == 0 && rc.right == 0) )
		MoveWindow(rc, TRUE);

	nosave = false;
	WinUtil::SetIcon(m_hWnd,  _T("favorites.ico"));
	bHandled = FALSE;
	return TRUE;
}

FavoriteHubsFrame::StateKeeper::StateKeeper(ExListViewCtrl& hubs_, bool ensureVisible_) :
hubs(hubs_),
ensureVisible(ensureVisible_)
{
	hubs.SetRedraw(FALSE);

	// in grouped mode, the indexes of each item are completely random, so use entry pointers instead
	int i = -1;
	while( (i = hubs.GetNextItem(i, LVNI_SELECTED)) != -1) {
		selected.push_back((FavoriteHubEntry*)hubs.GetItemData(i));
	}

	SCROLLINFO si = { 0 };
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS;
	hubs.GetScrollInfo(SB_VERT, &si);

	scroll = si.nPos;
}

FavoriteHubsFrame::StateKeeper::~StateKeeper() {
	// restore visual updating now, otherwise it doesn't always scroll
	hubs.SetRedraw(TRUE);

	for(FavoriteHubEntryList::const_iterator i = selected.begin(), iend = selected.end(); i != iend; ++i) {
		for(int j = 0; j < hubs.GetItemCount(); ++j) {
			if((FavoriteHubEntry*)hubs.GetItemData(j) == *i)
			{
				hubs.SetItemState(j, LVIS_SELECTED, LVIS_SELECTED);
				if(ensureVisible)
					hubs.EnsureVisible(j, FALSE);
				break;
			}
		}
	}

	ListView_Scroll(hubs.m_hWnd, 0, scroll);
}

const FavoriteHubEntryList& FavoriteHubsFrame::StateKeeper::getSelection() const {
	return selected;
}

void FavoriteHubsFrame::openSelected() {
	if(!checkNick())
		return;
	
	int i = -1;
	while( (i = ctrlHubs.GetNextItem(i, LVNI_SELECTED)) != -1) {
		FavoriteHubEntry* entry = (FavoriteHubEntry*)ctrlHubs.GetItemData(i);
		RecentHubEntry r;
		r.setName(entry->getName());
		r.setDescription(entry->getDescription());
		r.setUsers("*");
		r.setShared("*");
		r.setServer(entry->getServer());
		FavoriteManager::getInstance()->addRecent(r);
		HubFrame::openWindow(Text::toT(entry->getServer()), entry->getChatUserSplit(), entry->getUserListState());
	}
	return;
}

void FavoriteHubsFrame::addEntry(const FavoriteHubEntry* entry, int pos, int groupIndex) {
	TStringList l;
	l.push_back(Text::toT(entry->getName()));
	l.push_back(Text::toT(entry->getDescription()));
	l.push_back(Text::toT(entry->getNick(false)));
	l.push_back(tstring(entry->getPassword().size(), '*'));
	l.push_back(Text::toT(entry->getServer()));
	l.push_back(Text::toT(entry->getUserDescription()));	
	bool b = entry->getConnect();
	int i = ctrlHubs.insert(pos, l, 0, (LPARAM)entry);
	ctrlHubs.SetCheckState(i, b);

    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_GROUPID;
    lvItem.iItem = i;
    lvItem.iSubItem = 0;
    lvItem.iGroupId = groupIndex;
    ctrlHubs.SetItem( &lvItem );
}

LRESULT FavoriteHubsFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if(reinterpret_cast<HWND>(wParam) == ctrlHubs) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		CRect rc;
		ctrlHubs.GetHeader().GetWindowRect(&rc);
		if (PtInRect(&rc, pt)) {
			return 0;
		}

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlHubs, pt);
		}

		int status = ctrlHubs.GetSelectedCount() > 0 ? MFS_ENABLED : MFS_DISABLED;
		hubsMenu.EnableMenuItem(IDC_OPEN_HUB_LOG, status);
		hubsMenu.EnableMenuItem(IDC_CONNECT, status);
		hubsMenu.EnableMenuItem(IDC_EDIT, status);
		hubsMenu.EnableMenuItem(IDC_MOVE_UP, status);
		hubsMenu.EnableMenuItem(IDC_MOVE_DOWN, status);
		hubsMenu.EnableMenuItem(IDC_REMOVE, status);

		tstring x;
		if (ctrlHubs.GetSelectedCount() == 1) {
			FavoriteHubEntry* f = (FavoriteHubEntry*)ctrlHubs.GetItemData(WinUtil::getFirstSelectedIndex(ctrlHubs));
			x = Text::toT(f->getName());
		} else {
			x = _T("");
		}
		if (!x.empty())
			hubsMenu.InsertSeparatorFirst(x);

		hubsMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

		if (!x.empty())
			hubsMenu.RemoveFirstItem();

		return TRUE; 
	}
	
	bHandled = FALSE;
	return FALSE; 
}

LRESULT FavoriteHubsFrame::onDoubleClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*) pnmh;

	if(item->iItem == -1) {
		PostMessage(WM_COMMAND, IDC_NEWFAV, 0);
	} else {
		PostMessage(WM_COMMAND, IDC_CONNECT, 0);
	}

	return 0;
}

LRESULT FavoriteHubsFrame::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	switch(kd->wVKey) {
	case VK_INSERT:
		PostMessage(WM_COMMAND, IDC_NEWFAV, 0);
		break;
	case VK_DELETE:
		PostMessage(WM_COMMAND, IDC_REMOVE, 0);
		break;
	case VK_RETURN:
		PostMessage(WM_COMMAND, IDC_CONNECT, 0);
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}

LRESULT FavoriteHubsFrame::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	if(!BOOLSETTING(CONFIRM_HUB_REMOVAL) || MessageBox(CTSTRING(REALLY_REMOVE), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
		while( (i = ctrlHubs.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			FavoriteManager::getInstance()->removeFavorite((FavoriteHubEntry*)ctrlHubs.GetItemData(i));
		}
	}
	return 0;
}

LRESULT FavoriteHubsFrame::onEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	if((i = ctrlHubs.GetNextItem(i, LVNI_SELECTED)) != -1)
	{
		FavoriteHubEntry* e = (FavoriteHubEntry*)ctrlHubs.GetItemData(i);
		dcassert(e != NULL);
		FavHubProperties dlg(e);
		if(dlg.DoModal(m_hWnd) == IDOK)
		{
			StateKeeper keeper(ctrlHubs);
			fillList();
		}
	}
	return 0;
}

LRESULT FavoriteHubsFrame::onNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	FavoriteHubEntry e;
	FavHubProperties dlg(&e);

	while (true) {
		if(dlg.DoModal((HWND)*this) == IDOK) {
			if (FavoriteManager::getInstance()->isFavoriteHub(e.getServer())){
				MessageBox(
					CTSTRING(FAVORITE_HUB_ALREADY_EXISTS), _T(" "), MB_ICONWARNING | MB_OK);
			} else {
				FavoriteManager::getInstance()->addFavorite(e);
				break;
			}
		} else {
			break;
		}
	}
	return 0;
}

bool FavoriteHubsFrame::checkNick() {
	if(SETTING(NICK).empty()) {
		MessageBox(CTSTRING(ENTER_NICK), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
		return false;
	}
	return true;
}

LRESULT FavoriteHubsFrame::onMoveUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	handleMove(true);
	return 0;
}

LRESULT FavoriteHubsFrame::onMoveDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	handleMove(false);
	return 0;
}

void FavoriteHubsFrame::handleMove(bool up) {
	FavoriteHubEntryList& fh = FavoriteManager::getInstance()->getFavoriteHubs();
	if(fh.size() <= 1)
		return;
	
	StateKeeper keeper(ctrlHubs);
	const FavoriteHubEntryList& selected = keeper.getSelection();

	FavoriteHubEntryList fh_copy = fh;
	if(!up)
		reverse(fh_copy.begin(), fh_copy.end());
	FavoriteHubEntryList moved;
	for(FavoriteHubEntryList::iterator i = fh_copy.begin(); i != fh_copy.end(); ++i) {
		if(find(selected.begin(), selected.end(), *i) == selected.end())
			continue;
		if(find(moved.begin(), moved.end(), *i) != moved.end())
			continue;
		const string& group = (*i)->getGroup();
		for(FavoriteHubEntryList::iterator j = i; ;) {
			if(j == fh_copy.begin()) {
				// couldn't move within the same group; change group.
				TStringList groups(getSortedGroups());
				if(!up)
					reverse(groups.begin(), groups.end());
				TStringIterC ig = find(groups.begin(), groups.end(), Text::toT(group));
				if(ig != groups.begin()) {
					FavoriteHubEntryPtr f = *i;
					f->setGroup(Text::fromT(*(ig - 1)));
					i = fh_copy.erase(i);
					fh_copy.push_back(f);
					moved.push_back(f);
				}
				break;
			}
			if((*--j)->getGroup() == group) {
				swap(*i, *j);
				break;
			}
		}
	}
	if(!up)
		reverse(fh_copy.begin(), fh_copy.end());
	fh = fh_copy;
	FavoriteManager::getInstance()->save();

	fillList();
}

TStringList FavoriteHubsFrame::getSortedGroups() const {
	set<tstring, noCaseStringLess> sorted_groups;
	const FavHubGroups& favHubGroups = FavoriteManager::getInstance()->getFavHubGroups();
	for(FavHubGroups::const_iterator i = favHubGroups.begin(), iend = favHubGroups.end(); i != iend; ++i)
		sorted_groups.insert(Text::toT(i->first));

	TStringList groups(sorted_groups.begin(), sorted_groups.end());
	groups.insert(groups.begin(), Util::emptyStringT); // default group (otherwise, hubs without group don't show up)
	return groups;
}

void FavoriteHubsFrame::fillList()
{
	bool old_nosave = nosave;
	nosave = true;

	ctrlHubs.DeleteAllItems(); 
	
	// sort groups
	TStringList groups(getSortedGroups());
	
	for(size_t i = 0; i < groups.size(); ++i) {
		// insert groups
		LVGROUP lg = {0};
		lg.cbSize = sizeof(lg);
		lg.iGroupId = i;
		lg.state = LVGS_NORMAL | (WinUtil::getOsMajor() >= 6 ? LVGS_COLLAPSIBLE : 0);
		lg.mask = LVGF_GROUPID | LVGF_HEADER | LVGF_STATE | LVGF_ALIGN;
		lg.uAlign = LVGA_HEADER_LEFT;

		// Header-title must be unicode (Convert if necessary)
		lg.pszHeader = (LPWSTR)groups[i].c_str();
		lg.cchHeader = groups[i].size();
		ctrlHubs.InsertGroup(i, &lg );
	}

	const FavoriteHubEntryList& fl = FavoriteManager::getInstance()->getFavoriteHubs();
	for(FavoriteHubEntryList::const_iterator i = fl.begin(); i != fl.end(); ++i) {
		const string& group = (*i)->getGroup();

		int index = 0;
		if(!group.empty()) {
			TStringList::const_iterator groupI = find(groups.begin() + 1, groups.end(), Text::toT(group));
			if(groupI != groups.end())
				index = groupI - groups.begin();
		}

		addEntry(*i, ctrlHubs.GetItemCount(), index);
	}

	nosave = old_nosave;
}

LRESULT FavoriteHubsFrame::onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* l = (NMITEMACTIVATE*)pnmh;
	::EnableWindow(GetDlgItem(IDC_CONNECT), ctrlHubs.GetItemState(l->iItem, LVIS_SELECTED));
	::EnableWindow(GetDlgItem(IDC_REMOVE), ctrlHubs.GetItemState(l->iItem, LVIS_SELECTED));
	::EnableWindow(GetDlgItem(IDC_EDIT), ctrlHubs.GetItemState(l->iItem, LVIS_SELECTED));
	if(!nosave && l->iItem != -1 && ((l->uNewState & LVIS_STATEIMAGEMASK) != (l->uOldState & LVIS_STATEIMAGEMASK))) {
		FavoriteHubEntry* f = (FavoriteHubEntry*)ctrlHubs.GetItemData(l->iItem);
		f->setConnect(ctrlHubs.GetCheckState(l->iItem) != FALSE);
		FavoriteManager::getInstance()->save();
	}
	return 0;
}

LRESULT FavoriteHubsFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		FavoriteManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		closed = true;		
		WinUtil::setButtonPressed(IDC_FAVORITES, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		WinUtil::saveHeaderOrder(ctrlHubs, SettingsManager::FAVORITESFRAME_ORDER, 
		SettingsManager::FAVORITESFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);

		CRect rc;
		if(!IsIconic()){
			//Get position of window
			GetWindowRect(&rc);
				
			//convert the position so it's relative to main window
			::ScreenToClient(GetParent(), &rc.TopLeft());
			::ScreenToClient(GetParent(), &rc.BottomRight());
				
			//save the position
			SettingsManager::getInstance()->set(SettingsManager::FAV_BOTTOM, (rc.bottom > 0 ? rc.bottom : 0));
			SettingsManager::getInstance()->set(SettingsManager::FAV_TOP, (rc.top > 0 ? rc.top : 0));
			SettingsManager::getInstance()->set(SettingsManager::FAV_LEFT, (rc.left > 0 ? rc.left : 0));
			SettingsManager::getInstance()->set(SettingsManager::FAV_RIGHT, (rc.right > 0 ? rc.right : 0));
		}

		bHandled = FALSE;
		return 0;
	}	
}

void FavoriteHubsFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */)
{
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	CRect rc = rect;
	rc.bottom -=28;
	ctrlHubs.MoveWindow(rc);

	const long bwidth = 90;
	const long bspace = 10;

	rc = rect;
	rc.bottom -= 2;
	rc.top = rc.bottom - 22;

	rc.left = 2;
	rc.right = rc.left + bwidth;
	ctrlNew.MoveWindow(rc);

	rc.OffsetRect(bwidth+2, 0);
	ctrlProps.MoveWindow(rc);

	rc.OffsetRect(bwidth+2, 0);
	ctrlRemove.MoveWindow(rc);

	rc.OffsetRect(bspace + bwidth +2, 0);
	ctrlUp.MoveWindow(rc);

	rc.OffsetRect(bwidth+2, 0);
	ctrlDown.MoveWindow(rc);

	rc.OffsetRect(bspace + bwidth +2, 0);
	ctrlConnect.MoveWindow(rc);

	rc.OffsetRect(bspace + bwidth + 2, 0);
	rc.right += 16;
	ctrlManageGroups.MoveWindow(rc);
}

LRESULT FavoriteHubsFrame::onOpenHubLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlHubs.GetSelectedCount() == 1) {
		int i = ctrlHubs.GetNextItem(-1, LVNI_SELECTED);
		FavoriteHubEntry* entry = (FavoriteHubEntry*)ctrlHubs.GetItemData(i);
		ParamMap params;
		params["hubNI"] = entry->getName();
		params["hubURL"] = entry->getServer();
		params["myNI"] = entry->getNick(); 
		string file = LogManager::getInstance()->getPath(LogManager::CHAT, params);
		if(Util::fileExists(file)){
			WinUtil::viewLog(file);
		} else {
			MessageBox(CTSTRING(NO_LOG_FOR_HUB), CTSTRING(NO_LOG_FOR_HUB), MB_OK );	  
		}
	}
	return 0;
}

LRESULT FavoriteHubsFrame::onManageGroups(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	FavHubGroupsDlg dlg;
	dlg.DoModal();

	StateKeeper keeper(ctrlHubs);
	fillList();
	return 0;
}

void FavoriteHubsFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
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
}

LRESULT FavoriteHubsFrame::onColumnClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLISTVIEW* l = (NMLISTVIEW*)pnmh;
	if(l->iSubItem == ctrlHubs.getSortColumn()) {
		if (!ctrlHubs.isAscending())
			ctrlHubs.setSort(-1, ctrlHubs.getSortType());
		else
			ctrlHubs.setSortDirection(false);
	} else {
		ctrlHubs.setSort(l->iSubItem, ExListViewCtrl::SORT_STRING_NOCASE);
	}
	return 0;
}

/**
 * @file
 * $Id: FavoritesFrm.cpp 485 2010-02-27 11:22:12Z bigmuscle $
 */
