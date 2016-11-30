/* 
 * 
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

#include "RecentsFrm.h"
#include "HubFrame.h"
#include "LineDlg.h"

int RecentHubsFrame::columnIndexes[] = { COLUMN_NAME, COLUMN_DESCRIPTION, COLUMN_USERS, COLUMN_SHARED, COLUMN_SERVER };
int RecentHubsFrame::columnSizes[] = { 200, 290, 50, 50, 100 };
static ResourceManager::Strings columnNames[] = { ResourceManager::NAME, ResourceManager::DESCRIPTION, 
ResourceManager::USERS, ResourceManager::SHARED, ResourceManager::HUB_ADDRESS
};

LRESULT RecentHubsFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	ctrlHubs.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, IDC_RECENTS);
	
	ctrlHubs.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);	
	ctrlHubs.SetBkColor(WinUtil::bgColor);
	ctrlHubs.SetTextBkColor(WinUtil::bgColor);
	ctrlHubs.SetTextColor(WinUtil::textColor);
	ctrlHubs.SetFont(WinUtil::listViewFont);
	
	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(RECENTFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(RECENTFRAME_WIDTHS), COLUMN_LAST);
	
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = LVCFMT_LEFT;
		ctrlHubs.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlHubs.SetColumnOrderArray(COLUMN_LAST, columnIndexes);
	
	ctrlConnect.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_CONNECT);
	ctrlConnect.SetWindowText(CTSTRING(CONNECT));
	ctrlConnect.SetFont(WinUtil::font);

	ctrlRemove.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_REMOVE);
	ctrlRemove.SetWindowText(CTSTRING(REMOVE));
	ctrlRemove.SetFont(WinUtil::font);

	ctrlRemoveAll.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON , 0, IDC_REMOVE_ALL);
	ctrlRemoveAll.SetWindowText(CTSTRING(REMOVE_ALL));
	ctrlRemoveAll.SetFont(WinUtil::font);

	FavoriteManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);
	updateList(FavoriteManager::getInstance()->getRecentHubs());
	
	hubsMenu.CreatePopupMenu();
	hubsMenu.AppendMenu(MF_STRING, IDC_CONNECT, CTSTRING(CONNECT));
	hubsMenu.AppendMenu(MF_STRING, IDC_ADD, CTSTRING(ADD_TO_FAVORITES));
	hubsMenu.AppendMenu(MF_STRING, IDC_EDIT, CTSTRING(PROPERTIES));
	hubsMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
	hubsMenu.AppendMenu(MF_STRING, IDC_REMOVE_ALL, CTSTRING(REMOVE_ALL));
	hubsMenu.SetMenuDefaultItem(IDC_CONNECT);

	WinUtil::SetIcon(m_hWnd, IDI_RECENTS);
	bHandled = FALSE;
	return TRUE;
}

LRESULT RecentHubsFrame::onDoubleClickHublist(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
	connectSelected();
	return 0;
}

LRESULT RecentHubsFrame::onEnter(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
	connectSelected();
	return 0;
}

LRESULT RecentHubsFrame::onClickedConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	connectSelected();
	return 0;
}

void RecentHubsFrame::connectSelected() {
	int i = -1;
	while ((i = ctrlHubs.GetNextItem(i, LVNI_SELECTED)) != -1) {
		auto r = FavoriteManager::getInstance()->getRecentHubEntry(((RecentHubEntry*) ctrlHubs.GetItemData(i))->getServer());
		WinUtil::connectHub(r);
	}
}

LRESULT RecentHubsFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL & /*bHandled*/) {
	if (reinterpret_cast<HWND>(wParam) == ctrlHubs && ctrlHubs.GetSelectedCount() > 0) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 

		CRect rc;
		ctrlHubs.GetHeader().GetWindowRect(&rc);
		if (PtInRect(&rc, pt)) {
			return 0;
		}

		if (pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlHubs, pt);
		}

		hubsMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

		return TRUE;
	}

	return FALSE;
}

LRESULT RecentHubsFrame::onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/) {
	ctrlHubs.SetFocus();
	return 0;
}

LRESULT RecentHubsFrame::onColumnClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL & /*bHandled*/) {
	NMLISTVIEW* l = (NMLISTVIEW*) pnmh;
	if (l->iSubItem == ctrlHubs.getSortColumn()) {
		if (!ctrlHubs.isAscending())
			ctrlHubs.setSort(-1, ctrlHubs.getSortType());
		else
			ctrlHubs.setSortDirection(false);
	} else {
		if (l->iSubItem == 2 || l->iSubItem == 3) {
			ctrlHubs.setSort(l->iSubItem, ExListViewCtrl::SORT_INT);
		} else {
			ctrlHubs.setSort(l->iSubItem, ExListViewCtrl::SORT_STRING);
		}
	}
	return 0;
}

void RecentHubsFrame::updateList(const RecentHubEntryList& fl) {
	ctrlHubs.SetRedraw(FALSE);
	for (const auto& i : fl) {
		addEntry(i, ctrlHubs.GetItemCount());
	}
	ctrlHubs.SetRedraw(TRUE);
	ctrlHubs.Invalidate();
}

void RecentHubsFrame::addEntry(const RecentHubEntryPtr& entry, int pos) {
	TStringList l;
	l.push_back(Text::toT(entry->getName()));
	l.push_back(Text::toT(entry->getDescription()));
	l.push_back(Text::toT(entry->getUsers()));
	l.push_back(Text::toT(Util::formatBytes(entry->getShared())));
	l.push_back(Text::toT(entry->getServer()));

	ctrlHubs.insert(pos, l, 0, (LPARAM) entry.get());
}

LRESULT RecentHubsFrame::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL & /*bHandled*/) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	if (kd->wVKey == VK_DELETE) {
		int i = -1;
		while ((i = ctrlHubs.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			FavoriteManager::getInstance()->removeRecent((RecentHubEntry*) ctrlHubs.GetItemData(i));
		}
	}
	return 0;
}

LRESULT RecentHubsFrame::onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TCHAR buf[256];
	
	if(ctrlHubs.GetSelectedCount() == 1) {
		int i = ctrlHubs.GetNextItem(-1, LVNI_SELECTED);
		FavoriteHubEntryPtr e = new FavoriteHubEntry();
		ctrlHubs.GetItemText(i, COLUMN_NAME, buf, 256);
		e->setName(Text::fromT(buf));

		ctrlHubs.GetItemText(i, COLUMN_DESCRIPTION, buf, 256);
		e->setDescription(Text::fromT(buf));

		ctrlHubs.GetItemText(i, COLUMN_SERVER, buf, 256);
		e->setServer(Text::fromT(buf));

		FavoriteManager::getInstance()->addFavoriteHub(e);
	}
	return 0;
}

LRESULT RecentHubsFrame::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlHubs.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		FavoriteManager::getInstance()->removeRecent((RecentHubEntry*)ctrlHubs.GetItemData(i));
	}
	return 0;
}

LRESULT RecentHubsFrame::onRemoveAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlHubs.DeleteAllItems();
	FavoriteManager::getInstance()->clearRecent();
	return 0;
}

LRESULT RecentHubsFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		FavoriteManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		closed = true;
		WinUtil::setButtonPressed(IDC_RECENTS, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		WinUtil::saveHeaderOrder(ctrlHubs, SettingsManager::RECENTFRAME_ORDER, 
			SettingsManager::RECENTFRAME_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);

		bHandled = FALSE;
		return 0;
	}	
}

void RecentHubsFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
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
	ctrlConnect.MoveWindow(rc);

	rc.OffsetRect(bspace + bwidth +2, 0);
	ctrlRemove.MoveWindow(rc);

	rc.OffsetRect(bwidth+2, 0);
	ctrlRemoveAll.MoveWindow(rc);
}

LRESULT RecentHubsFrame::onEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	if((i = ctrlHubs.GetNextItem(i, LVNI_SELECTED)) != -1)
	{
		auto r = FavoriteManager::getInstance()->getRecentHubEntry(((RecentHubEntry*)ctrlHubs.GetItemData(i))->getServer());
		dcassert(r != NULL);
		LineDlg dlg;
		dlg.description = TSTRING(DESCRIPTION);
		dlg.title = Text::toT(r->getName());
		dlg.line = Text::toT(r->getDescription());
		if(dlg.DoModal(m_hWnd) == IDOK) {
			r->setDescription(Text::fromT(dlg.line));
			ctrlHubs.SetItemText(i, COLUMN_DESCRIPTION, Text::toT(r->getDescription()).c_str());
			FavoriteManager::getInstance()->saveRecent();
		}
	}
	return 0;
}

void RecentHubsFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
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

	if (ctrlHubs.GetFont() != WinUtil::listViewFont){
		ctrlHubs.SetFont(WinUtil::listViewFont);
		refresh = true;
	}

	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}


LRESULT RecentHubsFrame::onItemchangedDirectories(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	NM_LISTVIEW* lv = (NM_LISTVIEW*) pnmh;
	::EnableWindow(GetDlgItem(IDC_CONNECT), (lv->uNewState & LVIS_FOCUSED));
	::EnableWindow(GetDlgItem(IDC_REMOVE), (lv->uNewState & LVIS_FOCUSED));
	return 0;
}
