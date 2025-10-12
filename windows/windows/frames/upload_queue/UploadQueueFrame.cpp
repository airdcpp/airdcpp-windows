/* 
 * Copyright (C) 2001-2003 BlackClaw, blackclaw@parsoma.net
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
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

#include <windows/stdafx.h>
#include <windows/resource.h>
#include <windows/frames/upload_queue/UploadQueueFrame.h>
#include <windows/ResourceLoader.h>

#include <windows/components/BarShader.h>
#include <windows/util/FormatUtil.h>
#include <windows/util/WinUtil.h>

#include <airdcpp/util/PathUtil.h>
#include <airdcpp/transfer/upload/UploadManager.h>
#include <airdcpp/transfer/upload/UploadQueueManager.h>

namespace wingui {

int UploadQueueFrame::ItemInfo::getImageIndex() const noexcept {
	return wingui::ResourceLoader::getIconIndex(Text::toT(uqi->getFile()));
}

int UploadQueueFrame::ItemInfo::compareItems(const ItemInfo* a, const ItemInfo* b, uint8_t col) noexcept {
	switch (col) {
	case COLUMN_TRANSFERRED: return compare(a->uqi->getPos(), b->uqi->getPos());
	case COLUMN_SIZE: return compare(a->uqi->getSize(), b->uqi->getSize());
	case COLUMN_ADDED:
	case COLUMN_WAITING: return compare(a->uqi->getTime(), b->uqi->getTime());
	default: return Util::stricmp(a->getText(col).c_str(), b->getText(col).c_str());
	}
}

const tstring UploadQueueFrame::ItemInfo::getText(uint8_t col) const noexcept {
	switch (col) {
	case COLUMN_FILE: return Text::toT(PathUtil::getFileName(uqi->getFile()));
	case COLUMN_PATH: return Text::toT(PathUtil::getFilePath(uqi->getFile()));
	case COLUMN_NICK: return wingui::FormatUtil::getNicks(uqi->getHintedUser());
	case COLUMN_HUB: return wingui::FormatUtil::getHubNames(uqi->getHintedUser());
	case COLUMN_SIZE: return Util::formatBytesW(uqi->getSize());
	case COLUMN_ADDED: return FormatUtil::formatDateTimeW(uqi->getTime());
	case COLUMN_TRANSFERRED: return Util::formatBytesW(uqi->getPos()) + _T(" (") + (uqi->getSize() > 0 ? wingui::WinUtil::toStringW((double)uqi->getPos() * 100.0 / (double)uqi->getSize()) : _T("0")) + _T("%)");
	case COLUMN_WAITING: return Util::formatSecondsW(GET_TIME() - uqi->getTime());
	default: return Util::emptyStringT;
	}
}

string UploadQueueFrame::id = "UploadQueue";

int UploadQueueFrame::columnSizes[] = { 250, 100, 75, 75, 75, 75, 100, 100 };
int UploadQueueFrame::columnIndexes[] = { UploadQueueFrame::ItemInfo::COLUMN_FILE, UploadQueueFrame::ItemInfo::COLUMN_PATH, UploadQueueFrame::ItemInfo::COLUMN_NICK,
	UploadQueueFrame::ItemInfo::COLUMN_HUB, UploadQueueFrame::ItemInfo::COLUMN_TRANSFERRED, UploadQueueFrame::ItemInfo::COLUMN_SIZE, UploadQueueFrame::ItemInfo::COLUMN_ADDED,
	UploadQueueFrame::ItemInfo::COLUMN_WAITING };
static ResourceManager::Strings columnNames[] = { ResourceManager::FILENAME, ResourceManager::PATH, ResourceManager::NICK, 
	ResourceManager::HUB, ResourceManager::TRANSFERRED, ResourceManager::SIZE, ResourceManager::ADDED, ResourceManager::WAITING_TIME };


UploadQueueFrame::UploadQueueFrame() : showTreeContainer(_T("BUTTON"), this, SHOWTREE_MESSAGE_MAP) { }

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

	ctrlQueued.SetImageList(ResourceLoader::getFileImages(), TVSIL_NORMAL);
	ctrlList.SetImageList(ResourceLoader::getFileImages(), LVSIL_SMALL);

	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	SetSplitterPanes(ctrlQueued.m_hWnd, ctrlList.m_hWnd);
	SetSplitterPosPct(25);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(UPLOADQUEUEFRAME_ORDER), UploadQueueFrame::ItemInfo::COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(UPLOADQUEUEFRAME_WIDTHS), UploadQueueFrame::ItemInfo::COLUMN_LAST);

	// column names, sizes
	for (uint8_t j = 0; j < UploadQueueFrame::ItemInfo::COLUMN_LAST; j++) {
		int fmt = (j == UploadQueueFrame::ItemInfo::COLUMN_TRANSFERRED || j == UploadQueueFrame::ItemInfo::COLUMN_SIZE) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlList.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
		
	ctrlList.setColumnOrderArray(UploadQueueFrame::ItemInfo::COLUMN_LAST, columnIndexes);
	ctrlList.setSortColumn(UploadQueueFrame::ItemInfo::COLUMN_NICK);
	// colors
	ctrlList.SetBkColor(WinUtil::bgColor);
	ctrlList.SetTextBkColor(WinUtil::bgColor);
	ctrlList.SetTextColor(WinUtil::textColor);
	ctrlList.SetFont(WinUtil::listViewFont);

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

	UploadManager::getInstance()->getQueue().addListener(this);
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
		UploadManager::getInstance()->getQueue().removeListener(this);
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
	auto& mgr = UploadManager::getInstance()->getQueue();
	if (usingUserMenu) {
		auto User = getSelectedUser();
		if (User) {
			mgr.clearUserFiles(User);
		}
	} else {
		int i = -1;
		UserList RemoveUsers;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			RemoveUsers.push_back(((ItemInfo*)ctrlList.getItemData(i))->getUser());
		}
		for (const auto& u: RemoveUsers) {
			mgr.clearUserFiles(u);
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
		RemoveUsers.push_back(((ItemInfo*)ctrlList.getItemData(i))->getUser());
	}

	auto& mgr = UploadManager::getInstance()->getQueue();
	for(auto u = RemoveUsers.begin(); u != RemoveUsers.end(); ++u) {
		mgr.clearUserFiles(*u);
	}
	updateStatus();
}
	
void UploadQueueFrame::removeSelectedUser() {
	auto User = getSelectedUser();
	if (User) {
		UploadManager::getInstance()->getQueue().clearUserFiles(User);
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
		contextMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
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
		contextMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
		return TRUE;
	}	
	return FALSE; 
}

void UploadQueueFrame::LoadAll() {
	ctrlList.SetRedraw(FALSE);
	ctrlQueued.SetRedraw(FALSE);	
	
	// Load queue
	auto users = UploadManager::getInstance()->getQueue().getUploadQueue();
	for (auto uit = users.begin(); uit != users.end(); ++uit) {
		for (const auto& file: uit->files) {
			AddFile(file);
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

	auto& mgr = UploadManager::getInstance()->getQueue();
	while(userNode) {
		ctrlList.DeleteAllItems();
		UserItem* ui = reinterpret_cast<UserItem *>(ctrlQueued.GetItemData(userNode));
		if(ui) {
			auto users = mgr.getUploadQueue();
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

void UploadQueueFrame::AddFile(const UploadQueueItemPtr& aUQI) {
	HTREEITEM userNode = ctrlQueued.GetChildItem(rootItem);
	bool add = true;

	HTREEITEM selNode = ctrlQueued.GetSelectedItem();

	while(userNode) {
		auto u = reinterpret_cast<UserItem*>(ctrlQueued.GetItemData(userNode));
		if (aUQI->getHintedUser() == u->u.user) {
			add = false;
			break;
		}
		userNode = ctrlQueued.GetNextSiblingItem(userNode);
	}

	auto formatTitle = [aUQI] {
		return FormatUtil::getNicks(aUQI->getHintedUser()) + _T(" - ") + FormatUtil::getHubNames(aUQI->getHintedUser());
	};

	if(add) {
		userNode = ctrlQueued.InsertItem(TVIF_PARAM | TVIF_TEXT, formatTitle().c_str(), 
			0, 0, 0, 0, (LPARAM)(new UserItem(aUQI->getHintedUser())), rootItem, TVI_LAST);
	}	
	if(selNode && selNode != rootItem) {
		TCHAR selBuf[256];
		ctrlQueued.GetItemText(selNode, selBuf, 255);
		if(_tcscmp(selBuf, formatTitle().c_str()) != 0) {
			return;
		}
	}

	auto ii = std::make_unique<ItemInfo>(aUQI);
	auto insertInfo = itemInfos.emplace(aUQI->getToken(), std::move(ii));
	ctrlList.insertItem(ctrlList.GetItemCount(), insertInfo.first->second.get(), insertInfo.first->second->getImageIndex());
}

void UploadQueueFrame::updateStatus() {
	if(ctrlStatus.IsWindow()) {

		int cnt = ctrlList.GetItemCount();
		int users = ctrlQueued.GetCount() - 1;	

		tstring tmp[2];
		if(showTree) {
			tmp[0] = TSTRING(USERS) + _T(": ") + WinUtil::toStringW(users);
		} else {
			tmp[0] = _T("");
		}    		  
		tmp[1] = TSTRING(ITEMS) + _T(": ") + WinUtil::toStringW(cnt);
		bool u = false;

		for(int i = 1; i < 3; i++) {
			int w = WinUtil::getStatusTextWidth(tmp[i-1], ctrlStatus.m_hWnd);
				
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

void UploadQueueFrame::on(UploadQueueManagerListener::QueueAdd, const UploadQueueItemPtr& aUQI) noexcept {
	callAsync([=] {
		ctrlList.SetRedraw(FALSE);
		AddFile(aUQI);
		updateStatus();
		ctrlList.resort();

		if (SETTING(BOLD_WAITING_USERS))
			setDirty();

		ctrlList.SetRedraw(TRUE);
	});
}

void UploadQueueFrame::on(UploadQueueManagerListener::QueueUserRemove, const UserPtr& aUser) noexcept {
	callAsync([=] {
		RemoveUser(aUser);
		updateStatus();

		if (SETTING(BOLD_WAITING_USERS)) {
			setDirty();
		}
	});
}

void UploadQueueFrame::on(UploadQueueManagerListener::QueueItemRemove, const UploadQueueItemPtr& aUQI) noexcept {
	callAsync([=] {
		auto ii = itemInfos.find(aUQI->getToken());
		if (ii == itemInfos.end()) {
			return; // Item not found, nothing to do
		}
		
		ctrlList.deleteItem(ii->second.get());
		itemInfos.erase(aUQI->getToken());

		updateStatus();
		if (SETTING(BOLD_WAITING_USERS))
			setDirty();
	});
}

void UploadQueueFrame::on(UploadQueueManagerListener::QueueUpdate) noexcept {
	callAsync([=] {
		ctrlList.SetRedraw(FALSE);

		for (int i = 0; i < ctrlList.GetItemCount(); i++) {
			ctrlList.updateItem(i, ItemInfo::COLUMN_TRANSFERRED);
			ctrlList.updateItem(i, ItemInfo::COLUMN_WAITING);
		}

		ctrlList.SetRedraw(TRUE);
	});
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

	if (ctrlList.GetFont() != WinUtil::listViewFont){
		ctrlList.SetFont(WinUtil::listViewFont);
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
		if(ctrlList.findColumn(cd->iSubItem) == ItemInfo::COLUMN_TRANSFERRED) {
			// draw something nice...
				TCHAR buf[256];
				auto ii = (ItemInfo*)cd->nmcd.lItemlParam;

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

				CBarShader statusBar(rc.Height(), rc.Width(), SETTING(UPLOAD_BAR_COLOR), ii->uqi->getSize());
				statusBar.FillRange(0, ii->uqi->getPos(), SETTING(COLOR_DONE));
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

}