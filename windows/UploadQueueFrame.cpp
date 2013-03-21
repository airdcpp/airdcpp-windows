/* 
 * Copyright (C) 2001-2003 BlackClaw, blackclaw@parsoma.net
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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
#include "UploadQueueFrame.h"
#include "ResourceLoader.h"

#include "BarShader.h"

int UploadQueueFrame::columnSizes[] = { 250, 100, 75, 75, 75, 75, 100, 100 };
int UploadQueueFrame::columnIndexes[] = { UploadQueueItem::COLUMN_FILE, UploadQueueItem::COLUMN_PATH, UploadQueueItem::COLUMN_NICK,
	UploadQueueItem::COLUMN_HUB, UploadQueueItem::COLUMN_TRANSFERRED, UploadQueueItem::COLUMN_SIZE, UploadQueueItem::COLUMN_ADDED,
	UploadQueueItem::COLUMN_WAITING };
static ResourceManager::Strings columnNames[] = { ResourceManager::FILENAME, ResourceManager::PATH, ResourceManager::NICK, 
	ResourceManager::HUB, ResourceManager::TRANSFERRED, ResourceManager::SIZE, ResourceManager::ADDED, ResourceManager::WAITING_TIME };

LRESULT UploadQueueFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	showTree = SETTING(UPLOADQUEUEFRAME_SHOW_TREE);

	// status bar
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_UPLOAD_QUEUE);

	ctrlList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
	ctrlQueued.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
		TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP | TVS_TRACKSELECT,
		 WS_EX_CLIENTEDGE, IDC_DIRECTORIES);

	if(SETTING(USE_EXPLORER_THEME)) {
		SetWindowTheme(ctrlQueued.m_hWnd, L"explorer", NULL);
	}

	ctrlQueued.SetImageList(ResourceLoader::fileImages, TVSIL_NORMAL);
	ctrlList.SetImageList(ResourceLoader::fileImages, LVSIL_SMALL);

	m_nProportionalPos = 2500;
	SetSplitterPanes(ctrlQueued.m_hWnd, ctrlList.m_hWnd);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(UPLOADQUEUEFRAME_ORDER), UploadQueueItem::COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(UPLOADQUEUEFRAME_WIDTHS), UploadQueueItem::COLUMN_LAST);

	// column names, sizes
	for (uint8_t j=0; j<UploadQueueItem::COLUMN_LAST; j++) {
		int fmt = (j == UploadQueueItem::COLUMN_TRANSFERRED || j == UploadQueueItem::COLUMN_SIZE) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlList.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
		
	ctrlList.setColumnOrderArray(UploadQueueItem::COLUMN_LAST, columnIndexes);
	ctrlList.setSortColumn(UploadQueueItem::COLUMN_NICK);
	
	// colors
	ctrlList.SetBkColor(WinUtil::bgColor);
	ctrlList.SetTextBkColor(WinUtil::bgColor);
	ctrlList.SetTextColor(WinUtil::textColor);

	ctrlQueued.SetBkColor(WinUtil::bgColor);
	ctrlQueued.SetTextColor(WinUtil::textColor);
	
	ctrlShowTree.Create(ctrlStatus.m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlShowTree.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowTree.SetCheck(showTree);
	showTreeContainer.SubclassWindow(ctrlShowTree.m_hWnd);

    memzero(statusSizes, sizeof(statusSizes));
	statusSizes[0] = 16;
	ctrlStatus.SetParts(4, statusSizes);
	UpdateLayout();

	UploadManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	rootItem = ctrlQueued.InsertItem(CTSTRING(ALL), TVI_ROOT, TVI_LAST);
	LoadAll();
	
	ctrlQueued.Expand(rootItem);
	WinUtil::SetIcon(m_hWnd, IDI_UPLOAD_QUEUE);
	bHandled = FALSE;
	return TRUE;
}

LRESULT UploadQueueFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		UploadManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		WinUtil::setButtonPressed(IDC_UPLOAD_QUEUE, false);
		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		DeleteAll();

		SettingsManager::getInstance()->set(SettingsManager::UPLOADQUEUEFRAME_SHOW_TREE, ctrlShowTree.GetCheck() == BST_CHECKED);
		ctrlList.saveHeaderOrder(SettingsManager::UPLOADQUEUEFRAME_ORDER, SettingsManager::UPLOADQUEUEFRAME_WIDTHS,
			SettingsManager::UPLOADQUEUEFRAME_VISIBLE);

		bHandled = FALSE;
		return 0;
	}
}

void UploadQueueFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[4];
		ctrlStatus.GetClientRect(sr);
		w[3] = sr.right - 16;
#define setw(x) w[x] = max(w[x+1] - statusSizes[x], 0)
		setw(2); setw(1);

		w[0] = 16;

		ctrlStatus.SetParts(4, w);

		ctrlStatus.GetRect(0, sr);
		ctrlShowTree.MoveWindow(sr);
	}
	if(showTree) {
		if(GetSinglePaneMode() != SPLIT_PANE_NONE) {
			SetSinglePaneMode(SPLIT_PANE_NONE);
		}
	} else {
		if(GetSinglePaneMode() != SPLIT_PANE_RIGHT) {
			SetSinglePaneMode(SPLIT_PANE_RIGHT);
		}
	}
	CRect rc = rect;
	SetSplitterRect(rc);
}

LRESULT UploadQueueFrame::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(usingUserMenu) {
		UserPtr User = getSelectedUser();
		if(User) {
			UploadManager::getInstance()->clearUserFiles(User, true);
		}
	} else {
		int i = -1;
		UserList RemoveUsers;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			RemoveUsers.push_back(((UploadQueueItem*)ctrlList.getItemData(i))->getUser());
		}
		for(UserList::const_iterator i = RemoveUsers.begin(); i != RemoveUsers.end(); ++i) {
			UploadManager::getInstance()->clearUserFiles(*i, true);
		}
	}
	updateStatus();
	return 0;
}

void UploadQueueFrame::removeSelected() {
	int i = -1;
	UserList RemoveUsers;
	while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
		// Ok let's cheat here, if you try to remove more users here is not working :(
		RemoveUsers.push_back(((UploadQueueItem*)ctrlList.getItemData(i))->getUser());
	}
	for(auto i = RemoveUsers.begin(); i != RemoveUsers.end(); ++i) {
		UploadManager::getInstance()->clearUserFiles(*i, true);
	}
	updateStatus();
}
	
void UploadQueueFrame::removeSelectedUser() {
	UserPtr User = getSelectedUser();
	if(User) {
		UploadManager::getInstance()->clearUserFiles(User, true);
	}
	updateStatus();
}

LRESULT UploadQueueFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	RECT rc;
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	ctrlList.GetHeader().GetWindowRect(&rc);
	if(PtInRect(&rc, pt)){
		ctrlList.showMenu(pt);
		return TRUE;
	}

	usingUserMenu = reinterpret_cast<HWND>(wParam) == ctrlQueued;
	
	OMenu contextMenu;
	contextMenu.CreatePopupMenu();
	contextMenu.InsertSeparatorFirst(CTSTRING(UPLOAD_QUEUE));

	if(reinterpret_cast<HWND>(wParam) == ctrlList && ctrlList.GetSelectedCount() > 0) {
     	if(pt.x == -1 && pt.y == -1) {
    		WinUtil::getContextMenuPos(ctrlList, pt);
    	}
    	
		appendUserItems(contextMenu);
		contextMenu.AppendMenu(MF_SEPARATOR);
		contextMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
		contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		return TRUE;
	} else if(usingUserMenu && ctrlQueued.GetSelectedItem() != NULL) {
     	if(pt.x == -1 && pt.y == -1) {
    		WinUtil::getContextMenuPos(ctrlQueued, pt);
    	} else {
			UINT a = 0;
    		ctrlQueued.ScreenToClient(&pt);
			HTREEITEM ht = ctrlQueued.HitTest(pt, &a);
			
			if(!ht || ht == rootItem)
				return FALSE;
				
			if(ht != ctrlQueued.GetSelectedItem())
				ctrlQueued.SelectItem(ht);
    
			ctrlQueued.ClientToScreen(&pt);
        }
		
		appendUserItems(contextMenu);
		contextMenu.AppendMenu(MF_SEPARATOR);
		contextMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));        
		contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		return TRUE;
	}	
	return FALSE; 
}

void UploadQueueFrame::LoadAll() {
	ctrlList.SetRedraw(FALSE);
	ctrlQueued.SetRedraw(FALSE);	
	
	// Load queue
	UploadManager::SlotQueue users = UploadManager::getInstance()->getUploadQueue();
	for(UploadManager::SlotQueue::const_iterator uit = users.begin(); uit != users.end(); ++uit) {
		//ctrlQueued.InsertItem(TVIF_PARAM | TVIF_TEXT, (Text::toT(uit->first->getFirstNick()) + _T(" - ") + WinUtil::getHubNames(uit->first).first).c_str(), 
		//	0, 0, 0, 0, (LPARAM)(new UserItem(uit->first)), rootItem, TVI_LAST);
		for(auto i = uit->files.cbegin(); i != uit->files.cend(); ++i) {
			AddFile(*i);
		}
	}
	ctrlList.resort();
	ctrlList.SetRedraw(TRUE);
	ctrlQueued.SetRedraw(TRUE);
	ctrlQueued.Invalidate(); 
	updateStatus();
}

void UploadQueueFrame::DeleteAll() {
	HTREEITEM userNode = ctrlQueued.GetChildItem(rootItem);
	while (userNode) {
		delete reinterpret_cast<UserItem *>(ctrlQueued.GetItemData(userNode));
		ctrlQueued.DeleteItem(userNode);
		userNode = ctrlQueued.GetChildItem(rootItem);
	}
	ctrlList.DeleteAllItems();
	//ctrlQueued.DeleteAllItems();
}

void UploadQueueFrame::RemoveUser(const UserPtr& aUser) {
	HTREEITEM userNode = ctrlQueued.GetChildItem(rootItem);

	while(userNode) {
		UserItem *u = reinterpret_cast<UserItem *>(ctrlQueued.GetItemData(userNode));
		if (aUser == u->u.user) {
			delete u;
			ctrlQueued.DeleteItem(userNode);
			return;
		}
		userNode = ctrlQueued.GetNextSiblingItem(userNode);
	}
	updateStatus();
}

LRESULT UploadQueueFrame::onItemChanged(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
	HTREEITEM userNode = ctrlQueued.GetSelectedItem();

	while(userNode) {
		ctrlList.DeleteAllItems();
		UserItem* ui = reinterpret_cast<UserItem *>(ctrlQueued.GetItemData(userNode));
		if(ui) {
			auto users = UploadManager::getInstance()->getUploadQueue();
			auto it = find_if(users.begin(), users.end(), [&](const UserPtr& u) { return u == ui->u.user; });
			if(it != users.end()) {
				ctrlList.SetRedraw(FALSE);
				ctrlQueued.SetRedraw(FALSE);
				for(auto i = it->files.cbegin(); i != it->files.cend(); ++i) {
					AddFile(*i);
				}
				ctrlList.resort();
				ctrlList.SetRedraw(TRUE);
				ctrlQueued.SetRedraw(TRUE);
				ctrlQueued.Invalidate(); 
				updateStatus();
				return 0;
			}
		} else {
			LoadAll();
		}
		userNode = ctrlQueued.GetNextSiblingItem(userNode);
	}
	return 0;
}

void UploadQueueFrame::AddFile(UploadQueueItem* aUQI) { 
	HTREEITEM userNode = ctrlQueued.GetChildItem(rootItem);
	bool add = true;

	HTREEITEM selNode = ctrlQueued.GetSelectedItem();

	while(userNode) {
		UserItem *u = reinterpret_cast<UserItem *>(ctrlQueued.GetItemData(userNode));
		if (aUQI->getUser() == u->u.user) {
			add = false;
			break;
		}
		userNode = ctrlQueued.GetNextSiblingItem(userNode);
	}

	if(add) {
		userNode = ctrlQueued.InsertItem(TVIF_PARAM | TVIF_TEXT, (WinUtil::getNicks(aUQI->getHintedUser()) + _T(" - ") + WinUtil::getHubNames(aUQI->getHintedUser()).first).c_str(), 
			0, 0, 0, 0, (LPARAM)(new UserItem(aUQI->getHintedUser())), rootItem, TVI_LAST);
	}	
	if(selNode && selNode != rootItem) {
		TCHAR selBuf[256];
		ctrlQueued.GetItemText(selNode, selBuf, 255);
		if(_tcscmp(selBuf, (WinUtil::getNicks(aUQI->getHintedUser()) + _T(" - ") + WinUtil::getHubNames(aUQI->getHintedUser()).first).c_str()) != 0) {
			return;
		}
	}
	ctrlList.insertItem(ctrlList.GetItemCount(), aUQI, aUQI->getImageIndex());
}

void UploadQueueFrame::updateStatus() {
	if(ctrlStatus.IsWindow()) {

		int cnt = ctrlList.GetItemCount();
		int users = ctrlQueued.GetCount() - 1;	

		tstring tmp[2];
		if(showTree) {
			tmp[0] = TSTRING(USERS) + _T(": ") + Util::toStringW(users);
		} else {
			tmp[0] = _T("");
		}    		  
		tmp[1] = TSTRING(ITEMS) + _T(": ") + Util::toStringW(cnt);
		bool u = false;

		for(int i = 1; i < 3; i++) {
			int w = WinUtil::getTextWidth(tmp[i-1], ctrlStatus.m_hWnd);
				
			if(statusSizes[i] < w) {
				statusSizes[i] = w + 50;
				u = true;
			}
			ctrlStatus.SetText(i+1, tmp[i-1].c_str());
		}

		if(u)
			UpdateLayout(TRUE);
	}
}

int UploadQueueItem::getImageIndex() const {
	return ResourceLoader::getIconIndex(Text::toT(file));
}

const tstring UploadQueueItem::getText(uint8_t col) const {
	switch(col) {
		case COLUMN_FILE: return Text::toT(Util::getFileName(file));
		case COLUMN_PATH: return Text::toT(Util::getFilePath(file));
		case COLUMN_NICK: return WinUtil::getNicks(user);
		case COLUMN_HUB: return WinUtil::getHubNames(user).first;
		case COLUMN_SIZE: return Util::formatBytesW(size);
		case COLUMN_ADDED: return Text::toT(Util::formatTime("%Y-%m-%d %H:%M", time));
		case COLUMN_TRANSFERRED: return Util::formatBytesW(pos) + _T(" (") + (size > 0 ? Util::toStringW((double)pos*100.0/(double)size) : _T("0")) + _T("%)");
		case COLUMN_WAITING: return Util::formatSecondsW(GET_TIME() - time);
		default: return Util::emptyStringT;
	}
}

LRESULT UploadQueueFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	ctrlList.SetRedraw(FALSE);
	switch(wParam) {
	case REMOVE_ITEM: 
	{
		UploadQueueItem* i = (UploadQueueItem*)lParam;
		ctrlList.deleteItem(i);
		updateStatus();
		i->dec();
		if(SETTING(BOLD_WAITING_USERS))
			setDirty();
		break;
	}
	case REMOVE:
	{
		std::unique_ptr<UserItem> item((UserItem*)lParam);
		RemoveUser(item->getUser());
		updateStatus();
		if(SETTING(BOLD_WAITING_USERS))
			setDirty();
		break;
	}
	case ADD_ITEM:
		AddFile((UploadQueueItem*)lParam);
		updateStatus();
		ctrlList.resort();
		if(SETTING(BOLD_WAITING_USERS))
			setDirty();
		break;
	case UPDATE_ITEMS:
		for(int i = 0; i < ctrlList.GetItemCount(); i++) {
			ctrlList.updateItem(i, UploadQueueItem::COLUMN_TRANSFERRED);
			ctrlList.updateItem(i, UploadQueueItem::COLUMN_WAITING);
		}
		break;
	}
	ctrlList.SetRedraw(TRUE);
	return 0;
}

void UploadQueueFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	bool refresh = false;
	if(ctrlList.GetBkColor() != WinUtil::bgColor) {
		ctrlList.SetBkColor(WinUtil::bgColor);
		ctrlList.SetTextBkColor(WinUtil::bgColor);
		ctrlQueued.SetBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlList.GetTextColor() != WinUtil::textColor) {
		ctrlList.SetTextColor(WinUtil::textColor);
		ctrlQueued.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

LRESULT UploadQueueFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	if(!SETTING(SHOW_PROGRESS_BARS)) {
		bHandled = FALSE;
		return 0;
	}

	CRect rc;
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
		// Let's draw a box if needed...
		if(ctrlList.findColumn(cd->iSubItem) == UploadQueueItem::COLUMN_TRANSFERRED) {
			// draw something nice...
				TCHAR buf[256];
				UploadQueueItem *ii = (UploadQueueItem*)cd->nmcd.lItemlParam;

				ctrlList.GetItemText((int)cd->nmcd.dwItemSpec, cd->iSubItem, buf, 255);
				buf[255] = 0;

				ctrlList.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);

				// Text rect
				CRect rc2 = rc;
				rc2.MoveToXY(0, 0);
                rc2.left = 6; // indented with 6 pixels
				rc2.right -= 2; // and without messing with the border of the cell

				// Set references
				CDC cdc;
				cdc.CreateCompatibleDC(cd->nmcd.hdc);
				HBITMAP pOldBmp = cdc.SelectBitmap(CreateCompatibleBitmap(cd->nmcd.hdc,  rc.Width(),  rc.Height()));
				HDC& dc = cdc.m_hDC;

				HFONT oldFont = (HFONT)SelectObject(dc, WinUtil::font);
				SetBkMode(dc, TRANSPARENT);

				CBarShader statusBar(rc.Height(), rc.Width(), SETTING(UPLOAD_BAR_COLOR), ii->getSize());
				statusBar.FillRange(0, ii->getPos(), SETTING(COLOR_DONE));
				statusBar.Draw(cdc, 0, 0, SETTING(PROGRESS_3DDEPTH));

				SetTextColor(dc, SETTING(PROGRESS_TEXT_COLOR_UP));
                ::ExtTextOut(dc, rc2.left, rc2.top + (rc2.Height() - WinUtil::getTextHeight(dc) - 1)/2, ETO_CLIPPED, rc2, buf, _tcslen(buf), NULL);

				SelectObject(dc, oldFont);
				
				BitBlt(cd->nmcd.hdc,rc.left, rc.top, rc.Width(), rc.Height(), dc, 0, 0, SRCCOPY);

				DeleteObject(cdc.SelectBitmap(pOldBmp));
				return CDRF_SKIPDEFAULT;	
		}
		// Fall through
	default:
		return CDRF_DODEFAULT;
	}
}

/**
 * @file
 * $Id: UploadQueueFrame.cpp,v 1.4 2003/05/13 11:34:07
 */
