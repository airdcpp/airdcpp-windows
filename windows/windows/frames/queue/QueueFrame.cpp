/*
* Copyright (C) 2011-2024 AirDC++ Project
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
#include <windows/Resource.h>

#include <windows/frames/queue/QueueFrame.h>

#include <windows/components/BarShader.h>
#include <windows/dialog/BrowseDlg.h>
#include <windows/MainFrm.h>
#include <windows/frames/private_chat/PrivateFrame.h>
#include <windows/ResourceLoader.h>
#include <windows/util/FormatUtil.h>
#include <windows/util/ActionUtil.h>

#include <airdcpp/transfer/download/DownloadManager.h>
#include <airdcpp/util/PathUtil.h>

#include <web-server/ContextMenuManager.h>
#include <web-server/WebServerManager.h>


namespace wingui {
string QueueFrame::id = "Queue";

int QueueFrame::columnIndexes[] = { COLUMN_NAME, COLUMN_SIZE, COLUMN_TYPE, COLUMN_PRIORITY, COLUMN_STATUS, COLUMN_TIMELEFT, 
	COLUMN_SPEED, COLUMN_SOURCES, COLUMN_DOWNLOADED, COLUMN_TIME_ADDED, COLUMN_TIME_FINISHED, COLUMN_PATH };

int QueueFrame::columnSizes[] = { 450, 70, 70, 100, 250, 80, 
	80, 80, 80, 100, 100, 500 };

static ResourceManager::Strings columnNames[] = { ResourceManager::NAME, ResourceManager::SIZE, ResourceManager::TYPE_CONTENT, ResourceManager::PRIORITY, ResourceManager::STATUS, ResourceManager::TIME_LEFT,
	ResourceManager::SPEED, ResourceManager::SOURCES, ResourceManager::DOWNLOADED, ResourceManager::TIME_ADDED, ResourceManager::TIME_FINISHED, ResourceManager::PATH };

static ResourceManager::Strings treeNames[] = { ResourceManager::BUNDLES, ResourceManager::FINISHED, ResourceManager::QUEUED, ResourceManager::FAILED, ResourceManager::PAUSED, ResourceManager::AUTO_SEARCH, ResourceManager::LOCATIONS, ResourceManager::FILE_LISTS, ResourceManager::TEMP_ITEMS };

static SettingsManager::BoolSetting filterSettings[] = { SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST, SettingsManager::FILTER_QUEUE_INVERSED, SettingsManager::FILTER_QUEUE_TOP, SettingsManager::BOOL_LAST, SettingsManager::FILTER_QUEUE_RESET_CHANGE };

static ColumnType columnTypes[] = { COLUMN_TEXT, COLUMN_SIZE, COLUMN_TEXT, COLUMN_TEXT, COLUMN_TEXT, COLUMN_TIME, COLUMN_SPEED, COLUMN_TEXT, COLUMN_SIZE, COLUMN_TIME, COLUMN_TIME, COLUMN_TEXT };

QueueFrame::QueueItemInfoPtr QueueFrame::iBack;

QueueFrame::QueueFrame() : closed(false), statusDirty(true), 
	curSel(TREE_BUNDLES), curDirectory(nullptr),
	browserBar(this, [this](const string& a, bool b) { handleHistoryClick(a, b); }, [this] { handleItemClick(iBack); }),
	ctrlStatusContainer(WC_BUTTON, this, STATUS_MSG_MAP),
	//listContainer(WC_LISTVIEW, this, CONTROL_MSG_MAP),
	ctrlQueue(this, COLUMN_LAST, [this] { callAsync([this] { filterList(); }); }, filterSettings, COLUMN_LAST)
{
	iBack.reset(new QueueItemInfo(_T(".."), nullptr));
}

LRESULT QueueFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{

	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	ctrlStatusContainer.SubclassWindow(ctrlStatus.m_hWnd);

	//ctrlQueue.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_QUEUE_LIST);
	ctrlQueue.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, NULL);
	ctrlQueue.list.SetImageList(ResourceLoader::getFileImages(), LVSIL_SMALL);
	//listContainer.SubclassWindow(ctrlQueue.list);

	ctrlQueue.list.SetBkColor(WinUtil::bgColor);
	ctrlQueue.list.SetTextBkColor(WinUtil::bgColor);
	ctrlQueue.list.SetTextColor(WinUtil::textColor);
	ctrlQueue.list.setFlickerFree(WinUtil::bgBrush);
	ctrlQueue.list.SetFont(WinUtil::listViewFont);

	ctrlTree.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
		TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP | TVS_TRACKSELECT,
		WS_EX_CLIENTEDGE, IDC_TREE);
	ctrlTree.SetExtendedStyle(TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);

	if (SETTING(USE_EXPLORER_THEME)) {
		SetWindowTheme(ctrlTree.m_hWnd, L"explorer", NULL);
	}
	ctrlTree.SetImageList(ResourceLoader::getQueueTreeImages(), TVSIL_NORMAL);
	ctrlTree.SetBkColor(WinUtil::bgColor);
	ctrlTree.SetTextColor(WinUtil::textColor);
	ctrlTree.SetFont(WinUtil::listViewFont);

	browserBar.Init();
	//maximize the path field.
	CReBarCtrl rebar = m_hWndToolBar;
	rebar.MaximizeBand(1);
	rebar.LockBands(true);

	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	SetSplitterPanes(ctrlTree.m_hWnd, ctrlQueue.m_hWnd);
	SetSplitterPosPct(SETTING(QUEUE_SPLITTER_POS) / 100);

	CRect rc(SETTING(QUEUE_LEFT), SETTING(QUEUE_TOP), SETTING(QUEUE_RIGHT), SETTING(QUEUE_BOTTOM));
	if (!(rc.top == 0 && rc.bottom == 0 && rc.left == 0 && rc.right == 0))
		MoveWindow(rc, TRUE);

	FillTree();

	//Select item results in TVN_SELCHANGED, which reloads the queue list so do it before adding the bundles to avoid double filtering
	ctrlTree.SelectItem(bundleParent);

	{
		auto qm = QueueManager::getInstance();

		RLock l(qm->getCS());
		for (const auto& b : qm->getBundlesUnsafe() | views::values)
			onBundleAdded(b);

		for (const auto& q : qm->getFileQueueUnsafe() | views::values) {
			if (!q->getBundle())
				onQueueItemAdded(q);
		}
	}

	SettingsManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);

	memzero(statusSizes, sizeof(statusSizes));
	statusSizes[0] = 16;
	ctrlStatus.SetParts(5, statusSizes);
	updateStatus();

	::SetTimer(m_hWnd, 0, 500, 0);

	WinUtil::SetIcon(m_hWnd, IDI_QUEUE);
	bHandled = FALSE;
	return 1;
}

void QueueFrame::createColumns() {
	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(QUEUEFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(QUEUEFRAME_WIDTHS), COLUMN_LAST);

	for (uint8_t j = 0; j < COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_SPEED || j == COLUMN_TIMELEFT) ? LVCFMT_RIGHT : (j == COLUMN_STATUS) ? LVCFMT_CENTER : LVCFMT_LEFT;
		ctrlQueue.list.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlQueue.list.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlQueue.list.setSortColumn(COLUMN_NAME);
	ctrlQueue.list.setVisible(SETTING(QUEUEFRAME_VISIBLE));
}

void QueueFrame::FillTree() {
	HTREEITEM ht;
	bundleParent = TVI_ROOT;

	for (int i = TREE_FIRST; i < TREE_LAST; ++i) {
		if (i > TREE_LOCATION){
			ht = addTreeItem(TVI_ROOT, i, TSTRING_I(treeNames[i]));
		} else {
			ht = addTreeItem(bundleParent, i, TSTRING_I(treeNames[i]));
			if (i == TREE_BUNDLES)
				bundleParent = ht;
			if (i == TREE_LOCATION)
				locationParent = ht;
		}
	}
	
	ctrlTree.Expand(bundleParent);
}

void QueueFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	if (ctrlStatus.IsWindow()) {
		CRect sr;
		int w[5];
		ctrlStatus.GetClientRect(sr);

		w[4] = sr.right - 16;
#define setw(x) w[x] = max(w[x+1] - statusSizes[x], 0)
		setw(3); setw(2); setw(1);

		w[0] = 16;

		ctrlStatus.SetParts(5, w);

		ctrlStatus.GetRect(1, sr);
	}
	CRect rc = rect;
	SetSplitterRect(&rect);
	//ctrlQueue.MoveWindow(&rc);
}


LRESULT QueueFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if (!closed) {
		::KillTimer(m_hWnd, 0);
		SettingsManager::getInstance()->removeListener(this);
		QueueManager::getInstance()->removeListener(this);
		DownloadManager::getInstance()->removeListener(this);
		closed = true;
		WinUtil::setButtonPressed(IDC_QUEUE, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		CRect rc;
		if (!IsIconic()){
			//Get position of window
			GetWindowRect(&rc);
			//convert the position so it's relative to main window
			::ScreenToClient(GetParent(), &rc.TopLeft());
			::ScreenToClient(GetParent(), &rc.BottomRight());
			//save the position
			SettingsManager::getInstance()->set(SettingsManager::QUEUE_SPLITTER_POS, GetSplitterPosPct() * 100);
			SettingsManager::getInstance()->set(SettingsManager::QUEUE_BOTTOM, (rc.bottom > 0 ? rc.bottom : 0));
			SettingsManager::getInstance()->set(SettingsManager::QUEUE_TOP, (rc.top > 0 ? rc.top : 0));
			SettingsManager::getInstance()->set(SettingsManager::QUEUE_LEFT, (rc.left > 0 ? rc.left : 0));
			SettingsManager::getInstance()->set(SettingsManager::QUEUE_RIGHT, (rc.right > 0 ? rc.right : 0));
		}

		ctrlQueue.list.saveHeaderOrder(SettingsManager::QUEUEFRAME_ORDER,
			SettingsManager::QUEUEFRAME_WIDTHS, SettingsManager::QUEUEFRAME_VISIBLE);
		
		ctrlQueue.list.SetRedraw(FALSE);
		ctrlQueue.list.DeleteAllItems();
		ctrlQueue.list.SetRedraw(TRUE);

		ctrlTree.DeleteAllItems();
		parents.clear();

		bHandled = FALSE;
		return 0;
	}
}

LRESULT QueueFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)pnmh;

	switch (cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		{
			auto qii = ((QueueItemInfo*) cd->nmcd.lItemlParam);
			dcassert(qii);
			if (qii && qii->bundle && qii->bundle->isFailed()) {
				cd->clrText = SETTING(ERROR_COLOR);
				return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
			}
		}
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: 
	{
			auto ii = reinterpret_cast<QueueItemInfo*>(cd->nmcd.lItemlParam);

			int colIndex = ctrlQueue.list.findColumn(cd->iSubItem);
			cd->clrTextBk = WinUtil::bgColor;
			dcassert(ii);
			if (colIndex == COLUMN_STATUS) {
				if (!SETTING(SHOW_PROGRESS_BARS) || !SETTING(SHOW_QUEUE_BARS) || !ii || (ii->getSize() <= 0 || (ii->bundle && ii->bundle->isFailed()))) { // file lists don't have size in queue, don't even start to draw...
					bHandled = FALSE;
					return 0;
				}

				COLORREF clr = SETTING(PROGRESS_OVERRIDE_COLORS) ? 
					(ii->getParent() ? SETTING(PROGRESS_SEGMENT_COLOR) : getStatusColor(ii->bundle ? ii->bundle->getStatus() : 1)) : GetSysColor(COLOR_HIGHLIGHT);

				//this is just severely broken, msdn says GetSubItemRect requires a one based index
				//but it wont work and index 0 gives the rect of the whole item
				CRect rc;
				if (cd->iSubItem == 0) {
					ctrlQueue.list.GetItemRect((int)cd->nmcd.dwItemSpec, &rc, LVIR_LABEL);
				} else {
					ctrlQueue.list.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, &rc);
				}

				WinUtil::drawProgressBar(cd->nmcd.hdc, rc, clr, SETTING(PROGRESS_TEXT_COLOR_DOWN), SETTING(PROGRESS_BACK_COLOR), ii->getText(colIndex),
					ii->getSize(), ii->getDownloadedBytes(), SETTING(PROGRESSBAR_ODC_STYLE), SETTING(PROGRESS_OVERRIDE_COLORS2), SETTING(PROGRESS_3DDEPTH),
					SETTING(PROGRESS_LIGHTEN), DT_CENTER);

				//bah crap, if we return CDRF_SKIPDEFAULT windows won't paint the icons, so we have to do it
				if (!SETTING(USE_EXPLORER_THEME) && cd->iSubItem == 0){
					LVITEM lvItem;
					lvItem.iItem = cd->nmcd.dwItemSpec;
					lvItem.iSubItem = 0;
					lvItem.mask = LVIF_IMAGE | LVIF_STATE;
					lvItem.stateMask = LVIS_SELECTED;
					ctrlQueue.list.GetItem(&lvItem);

					HIMAGELIST imageList = (HIMAGELIST)::SendMessage(ctrlQueue.list.m_hWnd, LVM_GETIMAGELIST, LVSIL_SMALL, 0);
					if (imageList) {
						//let's find out where to paint it
						//and draw the background to avoid having 
						//the selection color as background
						CRect iconRect;
						ctrlQueue.list.GetSubItemRect((int)cd->nmcd.dwItemSpec, 0, LVIR_ICON, iconRect);
						ImageList_Draw(imageList, lvItem.iImage, cd->nmcd.hdc, iconRect.left, iconRect.top, ILD_TRANSPARENT);
					}
				}
				return CDRF_SKIPDEFAULT;
		}

	}
	default:
		return CDRF_DODEFAULT;
	}
}
void QueueFrame::handleTab() {
	HWND focus = GetFocus();
	if (focus == ctrlQueue.list.m_hWnd)
		ctrlTree.SetFocus();
	else if (focus == ctrlTree.m_hWnd) {
		ctrlQueue.list.SetFocus();
		if (ctrlQueue.list.GetSelectedCount() == 0)
			ctrlQueue.list.SelectItem(0);
	}
		
}

void QueueFrame::handleHistoryClick(const string& aPath, bool byHistory) {
	if (aPath == "/"){
		auto sel = curDirectory;
		reloadList(byHistory);
		if (sel)
			callAsync([=] { ctrlQueue.list.selectItem(sel); });
	} else {
		handleItemClick(findItemByPath(aPath), byHistory);
	}
}

LRESULT QueueFrame::onKeyDownTree(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMTVKEYDOWN* kd = (NMTVKEYDOWN*)pnmh;
	if (kd->wVKey == VK_TAB) {
		handleTab();
		bHandled = TRUE;
		return 1;
	}
	return 0;
}

LRESULT QueueFrame::onKeyDownList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	if (kd->wVKey == VK_TAB) {
		handleTab();
		bHandled = TRUE;
	} else if (kd->wVKey == VK_DELETE) {
		BundleList bl;
		QueueItemList ql;
		QueueItemInfoList dirs;
		getSelectedItems(bl, ql, dirs);
		if (!bl.empty()) {
			handleRemoveBundles(bl, false);
		} else {
			handleRemoveFiles(ql, false);
		}
		bHandled = TRUE;
	}
	else if (kd->wVKey == VK_BACK) {
		handleItemClick(iBack, true);
		bHandled = TRUE;
	}
	else if (kd->wVKey == VK_RETURN) {
		if (ctrlQueue.list.GetSelectedCount() > 1)
			return 0;

		int sel = -1;
		sel = ctrlQueue.list.GetNextItem(sel, LVNI_SELECTED);
		if (sel != -1) {
			auto ii = (QueueItemInfo*)ctrlQueue.list.GetItemData(sel);
			handleItemClick(ii);
		}
		bHandled = TRUE;
	}

	return 0;
}

LRESULT QueueFrame::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMITEMACTIVATE* l = (NMITEMACTIVATE*)pnmh;

	if (l->iItem != -1) {
		auto ii = (QueueItemInfo*)ctrlQueue.list.GetItemData(l->iItem);
		handleItemClick(ii);
	}

	bHandled = FALSE;
	return 0;
}

void QueueFrame::handleItemClick(const QueueItemInfoPtr& aII, bool byHistory/*false*/) {
	if (!aII) 
		return;
	
	if (aII->qi || (aII->bundle && aII->bundle->isFileBundle())) {
		if (aII->isFinished()) 
			ActionUtil::openFile(Text::toT(aII->getTarget()));
		return;
	}

	if (!aII->bundle && !aII->isDirectory || (aII == iBack && !curDirectory))
		return;

	auto sel = curDirectory;
	auto item = aII;
	if (aII == iBack){
		if (curDirectory->bundle){
			reloadList(byHistory);
			ctrlQueue.list.selectItem(sel);
			return;
		} else {
			item = curDirectory->getParent();
		}
	} 

	if (!byHistory) {
		browserBar.addHistory(item->getTarget());
	}

	ctrlQueue.list.SetRedraw(FALSE);
	ctrlQueue.list.DeleteAllItems();
	curDirectory = item.get();
	insertItems(item);
	ctrlQueue.list.SetRedraw(TRUE);
	if (sel)
		callAsync([=] { ctrlQueue.list.selectItem(sel); });

	ctrlQueue.onListChanged(false);
}

void QueueFrame::getSelectedItems(BundleList& bl, QueueItemList& ql, QueueItemInfoList& dirs, DWORD aFlag/* = LVNI_SELECTED*/) {
	int sel = -1;
	while ((sel = ctrlQueue.list.GetNextItem(sel, aFlag)) != -1) {
		QueueItemInfo* qii = (QueueItemInfo*)ctrlQueue.list.GetItemData(sel);
		if (qii->bundle)
			bl.push_back(qii->bundle);
		else if (qii->qi) {
			ql.push_back(qii->qi);
		} else if (qii->isDirectory && qii != iBack) {
			dirs.push_back(qii);
			qii->getChildQueueItems(ql);
		}
	}
}

LRESULT QueueFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	bool listviewMenu = false;
	bool treeMenu = false;
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	if (reinterpret_cast<HWND>(wParam) == ctrlQueue && ctrlQueue.list.GetSelectedCount() > 0) {
		if (pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlQueue.list, pt);
		}
		listviewMenu = true;
	}

	if (reinterpret_cast<HWND>(wParam) == ctrlTree) {
		if (pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlTree, pt);
		}
		// Select the item we right-clicked on
		UINT a = 0;
		ctrlTree.ScreenToClient(&pt);
		HTREEITEM ht = ctrlTree.HitTest(pt, &a);
		if (ht) {
			if (ht != ctrlTree.GetSelectedItem())
				ctrlTree.SelectItem(ht);
			ctrlTree.ClientToScreen(&pt);
			treeMenu = true;
		}
	}

	if (treeMenu || listviewMenu) {
		ShellMenu menu;
		menu.CreatePopupMenu();
		BundleList bl;
		QueueItemList queueItems;
		QueueItemInfoList dirs;

		getSelectedItems(bl, queueItems, dirs, treeMenu ? LVNI_ALL : LVNI_SELECTED);
		if (treeMenu) {
			AppendTreeMenu(bl, queueItems, menu);
		} else {
			if (!bl.empty())
				AppendBundleMenu(bl, menu);
			else if (!dirs.empty())
				AppendDirectoryMenu(dirs, queueItems, menu);
			else if (!queueItems.empty())
				AppendQiMenu(queueItems, menu);
		}
		menu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
		return TRUE;
	}

	bHandled = FALSE;
	return FALSE;
}

void QueueFrame::AppendDirectoryMenu(QueueItemInfoList& dirs, QueueItemList& ql, ShellMenu& dirMenu) {
	bool hasFinished = ranges::any_of(dirs, [](const QueueItemInfoPtr& d) { return d->getFinishedBytes() > 0; });
	bool hasBundleItems = ranges::any_of(ql, [](const QueueItemPtr& q) { return !!q->getBundle(); });
	
	if (hasBundleItems)
		ActionUtil::appendFilePrioMenu(dirMenu, ql);

	dirMenu.InsertSeparatorFirst(TSTRING(DIRECTORY));
	ctrlQueue.list.appendCopyMenu(dirMenu);
	dirMenu.AppendMenu(MF_SEPARATOR);

	dirMenu.appendItem(TSTRING(SEARCH), [this] { handleSearchDirectory(); });

	dirMenu.AppendMenu(MF_SEPARATOR);
	dirMenu.appendItem(TSTRING(REMOVE_OFFLINE), [=] { handleRemoveOffline(ql); });
	dirMenu.appendItem(TSTRING(READD_ALL), [=] { handleReaddAll(ql); });
	dirMenu.appendSeparator();

	dirMenu.appendItem(TSTRING(REMOVE), [=] { handleRemoveFiles(ql, false); });

	//remove created files and directories (only the ones associated with the current bundle in queue, not sure if we should just delete the existing directory and all files recursively regardless if its items from current bundle?)
	if (hasFinished) 
		dirMenu.appendItem(TSTRING(REMOVE_WITH_FILES), [=] { handleRemoveFiles(ql, true); ranges::for_each(dirs, [&](QueueItemInfoPtr dir) { dir->deleteSubdirs(); }); });

	if (dirs.size() == 1) {
		dirMenu.appendShellMenu({ dirs.front()->target });
	}
}

tstring QueueFrame::formatUser(const Bundle::BundleSource& bs) const {
	auto& u = bs.getUser();
	tstring nick = WinUtil::escapeMenu(FormatUtil::getNicks(u));

	nick += _T(" (") + TSTRING(FILES) + _T(": ") + WinUtil::toStringW(bs.files);
	if (u.user->getSpeed() > 0) {
		nick += _T(", ");
		nick += TSTRING(SPEED) + _T(": ") + Util::formatBytesW(u.user->getSpeed()) + _T("/s)");
	}
	nick += _T(")");
	return nick;
}

tstring QueueFrame::formatUser(const QueueItem::Source& s) const {
	tstring nick = WinUtil::escapeMenu(FormatUtil::getNicks(s.getUser()));
	return nick;
}

template<typename SourceType>
tstring formatSourceFlags(const SourceType& s) {
	auto reason = QueueItem::Source::formatError(s);
	if (!reason.empty()) {
		return _T(" (") + Text::toT(reason) + _T(")");
	}

	return Util::emptyStringT;
}
void QueueFrame::AppendTreeMenu(BundleList& bl, QueueItemList& ql, OMenu& aMenu) {
	if (!bl.empty()) {
		bool filesOnly = ranges::all_of(bl, [](const BundlePtr& b) { return b->isFileBundle(); });
		bool hasFinished = ranges::any_of(bl, [](const BundlePtr& b) { return b->isDownloaded(); });
		bool allFinished = ranges::all_of(bl, [](const BundlePtr& b) { return b->isDownloaded(); });

		aMenu.InsertSeparatorFirst(CTSTRING_F(X_BUNDLES, bl.size()));

		if (!allFinished) {
			ActionUtil::appendBundlePrioMenu(aMenu, bl);
			ActionUtil::appendBundlePauseMenu(aMenu, bl);
			aMenu.appendSeparator();
		}

		if (curSel == TREE_FAILED) {
			aMenu.appendItem(TSTRING(RESCAN_BUNDLE), [=] {
				ranges::for_each(bl, [&](const BundlePtr& b) { QueueManager::getInstance()->shareBundle(b, false); });
			}, OMenu::FLAG_THREADED);
			aMenu.appendItem(TSTRING(FORCE_SHARING), [=] { 
				ranges::for_each(bl, [&](const BundlePtr& b) { QueueManager::getInstance()->shareBundle(b, true); });
			}, OMenu::FLAG_THREADED);
			aMenu.appendSeparator();
		}

		aMenu.appendItem(TSTRING(REMOVE), [=] { handleRemoveBundles(bl, false); });
		if (!filesOnly)
			aMenu.appendItem(TSTRING(REMOVE_WITH_FILES), [=] { handleRemoveBundles(bl, true); });
		if (hasFinished && (curSel != TREE_FAILED)) {
			aMenu.appendSeparator();
			aMenu.appendItem(TSTRING(REMOVE_FINISHED), [=] { handleRemoveBundles(bl, false, true); });
		}
	} else if (!ql.empty()) {
		aMenu.InsertSeparatorFirst(CTSTRING_F(X_FILES, ql.size()));
		bool hasFinished = ranges::any_of(ql, [](const QueueItemPtr& q) { return q->isDownloaded(); });
		aMenu.appendItem(TSTRING(REMOVE), [=] { handleRemoveFiles(ql, false); });
		if (hasFinished)
			aMenu.appendItem(TSTRING(REMOVE_WITH_FILES), [=] { handleRemoveFiles(ql, true); });
	}
}

/*Bundle Menu*/
void QueueFrame::AppendBundleMenu(BundleList& bl, ShellMenu& bundleMenu) {
	OMenu* removeMenu = bundleMenu.getMenu();
	OMenu* readdMenu = bundleMenu.getMenu();

	if (bl.size() == 1) {
		bundleMenu.InsertSeparatorFirst(Text::toT(bl.front()->getName()));
	} else {
		bundleMenu.InsertSeparatorFirst(CTSTRING_F(X_BUNDLES, bl.size()));
	}

	bool hasFinished = ranges::any_of(bl, [](const BundlePtr& b) { return b->isDownloaded(); });
	bool filesOnly = ranges::all_of(bl, [](const BundlePtr& b) { return b->isFileBundle(); });
	bool allFinished = ranges::all_of(bl, [](const BundlePtr& b) { return b->isDownloaded(); });
	bool hasFailed = ranges::any_of(bl, [](const BundlePtr& b) { return !!b->getHookError(); });

	/* Insert sub menus */
	BundlePtr b = nullptr;
	if (bl.size() == 1) {
		b = bl.front();
	}

	{
		vector<QueueToken> tokens;
		for (const auto& bundle : bl) {
			tokens.push_back(bundle->getToken());
		}

		EXT_CONTEXT_MENU(bundleMenu, QueueBundle, tokens);
	}

	if (!allFinished) {
		ActionUtil::appendBundlePrioMenu(bundleMenu, bl);
		ActionUtil::appendBundlePauseMenu(bundleMenu, bl);
		bundleMenu.appendSeparator();
	}

	ListBaseType::MenuItemList customItems;
	ctrlQueue.list.appendCopyMenu(bundleMenu, customItems);

	if (b) {
		//current sources
		auto bundleSources = QueueManager::getInstance()->getBundleSources(b);
		if (!bundleSources.empty()) {
			removeMenu->appendItem(TSTRING(ALL), [=] {
				for (auto& si : bundleSources)
					QueueManager::getInstance()->removeBundleSource(b, si.getUser().user, QueueItem::Source::FLAG_REMOVED);
			}, OMenu::FLAG_THREADED);
			removeMenu->appendSeparator();
		}

		for (auto& bs : bundleSources) {
			auto u = bs.getUser();
			removeMenu->appendItem(formatUser(bs), [=] { QueueManager::getInstance()->removeBundleSource(b, u, QueueItem::Source::FLAG_REMOVED); }, OMenu::FLAG_THREADED);
		}

		//bad sources
		auto badBundleSources = QueueManager::getInstance()->getBadBundleSources(b);

		if (!badBundleSources.empty()) {
			readdMenu->appendItem(TSTRING(ALL), [=] {
				for (auto& si : badBundleSources)
					QueueManager::getInstance()->readdBundleSourceHooked(b, si.getUser());
			}, OMenu::FLAG_THREADED);
			readdMenu->appendSeparator();
		}

		for (auto& bs : badBundleSources) {
			auto u = bs.getUser();
			readdMenu->appendItem(formatUser(bs) + formatSourceFlags(bs), [=] { QueueManager::getInstance()->readdBundleSourceHooked(b, u); }, OMenu::FLAG_THREADED);
		}
		/* Sub menus end */

		// search
		if (!b->isDownloaded()) {
			bundleMenu.appendItem(TSTRING(SEARCH_BUNDLE_ALT), [=] {
				auto bundle = b;
				auto queuedFileSearches = QueueManager::getInstance()->searchBundleAlternates(bundle);
				ctrlStatus.SetText(1, TSTRING_F(BUNDLE_ALT_SEARCH, Text::toT(bundle->getName()) % queuedFileSearches).c_str());
			}, OMenu::FLAG_THREADED);
		}
		bundleMenu.appendSeparator();

		bundleMenu.appendItem(TSTRING(SEARCH_DIRECTORY), [=] { handleSearchDirectory(); });

		if (!b->isDownloaded()) {
			bundleMenu.appendSeparator();
			readdMenu->appendThis(TSTRING(READD_SOURCE), true);
			removeMenu->appendThis(TSTRING(REMOVE_SOURCE), true);

			bundleMenu.appendSeparator();
			appendUserMenu<Bundle::BundleSource>(bundleMenu, bundleSources);

			bundleMenu.appendSeparator();
			bundleMenu.appendItem(TSTRING(USE_SEQ_ORDER), [=] {
				auto bundle = b;
				QueueManager::getInstance()->onUseSeqOrder(bundle);
			}, b->getSeqOrder() ? OMenu::FLAG_CHECKED : 0 | OMenu::FLAG_THREADED);
		}
	}

	if (hasFailed) {
		bundleMenu.appendSeparator();
		bundleMenu.appendItem(TSTRING(RESCAN_BUNDLE), [=] {
			for (auto i : bl) {
				if (i->getHookError())
					QueueManager::getInstance()->shareBundle(i, false);
			}
			}, OMenu::FLAG_THREADED);

		bundleMenu.appendItem(TSTRING(FORCE_SHARING), [=] {
			for (auto i : bl) {
				if (i->getHookError())
					QueueManager::getInstance()->shareBundle(i, true);
			}
			}, OMenu::FLAG_THREADED);
	}

	bundleMenu.appendSeparator();
	bundleMenu.appendItem(TSTRING(REMOVE), [=] { handleRemoveBundles(bl, false); });
	if (!filesOnly || hasFinished)
		bundleMenu.appendItem(TSTRING(REMOVE_WITH_FILES), [=] { handleRemoveBundles(bl, true); });

	if (b)
		bundleMenu.appendShellMenu({ b->getTarget() });

	bundleMenu.appendItem(TSTRING(RECHECK_INTEGRITY), [=] { handleRecheckBundles(bl); });
}

/*QueueItem Menu*/
void QueueFrame::AppendQiMenu(QueueItemList& ql, ShellMenu& fileMenu) {

	bool hasFinished = ranges::any_of(ql, [](const QueueItemPtr& q) { return q->isDownloaded(); });
	bool hasBundleItems = ranges::any_of(ql, [](const QueueItemPtr& q) { return !!q->getBundle(); });

	if (hasBundleItems)
		ActionUtil::appendFilePrioMenu(fileMenu, ql);


	ListBaseType::MenuItemList customItems{
		{ TSTRING(MAGNET_LINK), &handleCopyMagnet },
		{ _T("TTH"), &handleCopyTTH }
	};
	ctrlQueue.list.appendCopyMenu(fileMenu, customItems);

	QueueItemPtr qi = nullptr;

	if (ql.size() == 1) {
		qi = ql.front();
	}

	if (qi) {
		OMenu* removeAllMenu = fileMenu.getMenu();
		OMenu* removeMenu = fileMenu.getMenu();
		OMenu* readdMenu = fileMenu.getMenu();

		/* Create submenus */
		auto sources = QueueManager::getInstance()->getSources(qi);

		//remove all sources from this file
		if (!sources.empty()) {
			removeMenu->appendItem(TSTRING(ALL), [=] {
				auto sources = QueueManager::getInstance()->getSources(qi);
				for (auto& si : sources)
					QueueManager::getInstance()->removeFileSource(qi->getTarget(), si.getUser(), QueueItem::Source::FLAG_REMOVED);
			}, OMenu::FLAG_THREADED);
			removeMenu->appendSeparator();
		}

		for (auto& s : sources) {

			auto nick = formatUser(s);
			auto u = s.getUser();
			// remove source (this file)
			removeMenu->appendItem(nick, [=] { QueueManager::getInstance()->removeFileSource(qi->getTarget(), u, QueueItem::Source::FLAG_REMOVED); }, OMenu::FLAG_THREADED);
			//remove source (all files)
			removeAllMenu->appendItem(nick, [=] { QueueManager::getInstance()->removeSource(u, QueueItem::Source::FLAG_REMOVED); }, OMenu::FLAG_THREADED);
		}

		auto badSources = QueueManager::getInstance()->getBadSources(qi);
		if (!badSources.empty()) {
			readdMenu->appendItem(TSTRING(ALL), [=] {
				auto sources = QueueManager::getInstance()->getBadSources(qi);
				for (auto& si : sources)
					QueueManager::getInstance()->readdQISourceHooked(qi->getTarget(), si.getUser());
			}, OMenu::FLAG_THREADED);
			readdMenu->appendSeparator();
		}

		for (auto& s : badSources) {
			auto u = s.getUser();
			readdMenu->appendItem(formatUser(s), [=] { QueueManager::getInstance()->readdQISourceHooked(qi->getTarget(), u); });
		}
		/* Submenus end */

		fileMenu.InsertSeparatorFirst(TSTRING(FILE));
		if (!qi->isSet(QueueItem::FLAG_USER_LIST)) {
			fileMenu.appendItem(CTSTRING(SEARCH), [=] { handleSearchQI(qi, true); });
			fileMenu.appendItem(CTSTRING(SEARCH_FOR_ALTERNATES), [=] { handleSearchQI(qi, false); });
			ActionUtil::appendPreviewMenu(fileMenu, qi->getTarget());
		}

		if (!qi->isDownloaded()) {
			fileMenu.appendSeparator();
			readdMenu->appendThis(TSTRING(READD_SOURCE), true);
			removeMenu->appendThis(TSTRING(REMOVE_SOURCE), true);
			removeAllMenu->appendThis(TSTRING(REMOVE_FROM_ALL), true);
			fileMenu.AppendMenu(MF_STRING, IDC_REMOVE_OFFLINE, CTSTRING(REMOVE_OFFLINE));

			fileMenu.appendSeparator();
			appendUserMenu<QueueItem::Source>(fileMenu, sources);
			fileMenu.appendSeparator();
		} else if (hasBundleItems) {
			fileMenu.appendSeparator();
			fileMenu.appendItem(TSTRING(OPEN), [=] { handleOpenFile(qi); }, OMenu::FLAG_DEFAULT);
		} else {
			fileMenu.appendSeparator();
		}
	} else {
		fileMenu.InsertSeparatorFirst(TSTRING(FILES));

		fileMenu.AppendMenu(MF_SEPARATOR);
		fileMenu.appendItem(TSTRING(REMOVE_OFFLINE), [=] { handleRemoveOffline(ql); });
		fileMenu.appendItem(TSTRING(READD_ALL), [=] { handleReaddAll(ql); });
		fileMenu.appendSeparator();
	}

	{
		vector<QueueToken> tokens;
		for (const auto& q: ql) {
			tokens.push_back(q->getToken());
		}

		EXT_CONTEXT_MENU(fileMenu, QueueFile, tokens);
	}

	if (hasBundleItems) {
		fileMenu.appendItem(TSTRING(RECHECK_INTEGRITY), [=] { handleRecheckFiles(ql); });
		fileMenu.appendSeparator();
	}

	fileMenu.appendItem(TSTRING(REMOVE), [=] { handleRemoveFiles(ql, false); });
	if (hasFinished)
		fileMenu.appendItem(TSTRING(REMOVE_WITH_FILES), [=] { handleRemoveFiles(ql, true); });

	if (qi && !qi->isSet(QueueItem::FLAG_USER_LIST)) {
		fileMenu.appendShellMenu({ qi->getTarget() });
	}
}

void QueueFrame::handleOpenFile(const QueueItemPtr& aQI) {
	ActionUtil::openFile(Text::toT(aQI->getTarget()));
}

void QueueFrame::handleOpenFolder() {
	ctrlQueue.list.forEachSelectedT([&](const QueueItemInfoPtr qii) {
		if (!qii->isTempItem())
			ActionUtil::openFolder(Text::toT(qii->getTarget()));
	});
}

void QueueFrame::handleSearchDirectory() {
	ctrlQueue.list.forEachSelectedT([&](const QueueItemInfoPtr qii) {
		if (qii->bundle)
			ActionUtil::search(qii->bundle->isFileBundle() ? PathUtil::getLastDir(Text::toT(qii->bundle->getTarget())) : Text::toT(qii->bundle->getName()), true);
		else if ( qii->isDirectory && qii != iBack)
			ActionUtil::search(qii->name, true);
	});
}


void QueueFrame::handleReaddAll(QueueItemList ql) {
	MainFrame::getMainFrame()->addThreadedTask([=] {
		for (auto q: ql) {
			// re-add all sources
			const auto badSources = QueueManager::getInstance()->getBadSources(q);
			for (const auto& bs : badSources) {
				QueueManager::getInstance()->readdQISourceHooked(q->getTarget(), bs.getUser());
			}
		}
	});
}

void QueueFrame::handleRemoveOffline(QueueItemList ql) {
	MainFrame::getMainFrame()->addThreadedTask([=] {
		for (auto q : ql) {
			const auto sources = QueueManager::getInstance()->getSources(q);
			for (const auto& s : sources) {
				if (!s.getUser().user->isOnline()) {
					QueueManager::getInstance()->removeFileSource(q->getTarget(), s.getUser().user, QueueItem::Source::FLAG_REMOVED);
				}
			}
		}
	});
}


LRESULT QueueFrame::onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	executeGuiTasks();
	updateStatus();
	bHandled = TRUE;
	return 0;
}

void QueueFrame::reloadList(bool byHistory/*false*/) {
	
	if(!byHistory) 
		browserBar.addHistory("/");

	curDirectory = nullptr;
	ctrlQueue.list.SetRedraw(FALSE);
	ctrlQueue.list.DeleteAllItems();
	for (auto& p : parents | views::values) {
		if (!show(p))
			continue;

		ctrlQueue.list.insertItem(ctrlQueue.list.getSortPos(p.get()), p.get(), p->getImageIndex());
	}
	ctrlQueue.list.SetRedraw(TRUE);
	ctrlQueue.onListChanged(false);
}

void QueueFrame::filterList() {

	ctrlQueue.list.SetRedraw(FALSE);
	ctrlQueue.list.DeleteAllItems();
	
	if (!curDirectory) {
		for (auto& p : parents | views::values) {
			if (!show(p))
				continue;
			ctrlQueue.list.insertItem(ctrlQueue.list.getSortPos(p.get()), p.get(), p->getImageIndex());
		}
	} else {
		ctrlQueue.list.insertItem(0, iBack.get(), iBack->getImageIndex());
		for (auto& p : curDirectory->children | views::values) {
			if (!show(p))
				continue;
			ctrlQueue.list.insertItem(ctrlQueue.list.getSortPos(p.get()), p.get(), p->getImageIndex());
		}
	}

	ctrlQueue.list.SetRedraw(TRUE);
}


bool QueueFrame::show(QueueItemInfoPtr& Qii) {
	//we are browsing inside a bundle already, no case for new bundles to be inserted.
	if (Qii->bundle && curDirectory)
		return false;
	if (Qii->isDirectory && Qii->children.empty()) //don't show empty directories
		return false;

	auto treeFilter = [&]() -> bool {
		bool isTempOrFilelist = Qii->isFilelist() || Qii->isTempItem();
		switch (curSel)
		{
		case TREE_BUNDLES:
			return !isTempOrFilelist;
			// TREE_QUEUED and TREE_FINISHED are the only ones that hide/show items INSIDE a Bundle.
		case TREE_QUEUED:
			return !Qii->isFinished() && !isTempOrFilelist;
		case TREE_FINISHED:
			return Qii->isFinished() && !isTempOrFilelist;
		case TREE_PAUSED:
			return (!Qii->bundle || Qii->bundle->isPausedPrio()) && !isTempOrFilelist;
		case TREE_FAILED:
			return (!Qii->bundle || Qii->bundle->isFailed()) && !isTempOrFilelist;
		case TREE_FILELIST:
			return Qii->isFilelist();
		case TREE_TEMP:
			return Qii->isTempItem();
		case TREE_AUTOSEARCH:
			return (!Qii->bundle || Qii->bundle->getAddedByAutoSearch()) && !isTempOrFilelist;

		case TREE_LOCATION: {
			if (Qii->bundle && curItem != locationParent){
				//do it this way so we can have counts after the name
				auto i = locations.find(Qii->bundle->isFileBundle() ? PathUtil::getFilePath(Qii->bundle->getTarget()) : PathUtil::getParentDir(Qii->bundle->getTarget()));
				if (i != locations.end())
					return curItem == i->second.item;
			}
			return !isTempOrFilelist;
		}
		default:
			return false;
		}
	};

	auto filterNumericF = [&](int column) -> double {
		switch (column) {
		case COLUMN_DOWNLOADED: return Qii->getDownloadedBytes();
		default: dcassert(0); return 0;
		}
	};

	auto filterInfo = ctrlQueue.filter.prepare([this, &Qii](int column) { return Text::fromT(Qii->getText(column)); }, filterNumericF);


	return treeFilter() && (ctrlQueue.filter.empty() || ctrlQueue.filter.match(filterInfo));

}

tstring QueueFrame::handleCopyMagnet(const QueueItemInfo* aII) {
	if (aII->qi && !aII->isFilelist())
		return Text::toT(ActionUtil::makeMagnet(aII->qi->getTTH(), PathUtil::getFileName(aII->qi->getTarget()), aII->qi->getSize()));

	return Util::emptyStringT;
}
tstring QueueFrame::handleCopyTTH(const QueueItemInfo* aII) {
	if (aII->qi && !aII->isFilelist())
		return Text::toT(aII->qi->getTTH().toBase32());

	return Util::emptyStringT;
}

void QueueFrame::handleRecheckBundles(BundleList bl) {
	MainFrame::getMainFrame()->addThreadedTask([=] {
		for (auto& b : bl)
			QueueManager::getInstance()->recheckBundle(b->getToken());
	});
}

void QueueFrame::handleRecheckFiles(QueueItemList ql) {
	MainFrame::getMainFrame()->addThreadedTask([ql] {
		QueueManager::getInstance()->recheckFiles(ql);
	});
}

void QueueFrame::handleRemoveBundles(BundleList bundles, bool removeFinished, bool aCompletedOnly) {
	if (bundles.empty())
		return;

	bool allFinished = ranges::all_of(bundles, [](const BundlePtr& b) { return b->isDownloaded(); });
	if (bundles.size() == 1 && !aCompletedOnly) {
		if (removeFinished) {
			if (!WinUtil::showQuestionBox(TSTRING_F(CONFIRM_REMOVE_DIR_FINISHED, Text::toT(bundles.front()->getName())), MB_ICONQUESTION)) {
				return;
			}
		} else if (!allFinished && !WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_QUEUE_REMOVAL, TSTRING_F(CONFIRM_REMOVE_DIR_BUNDLE, Text::toT(bundles.front()->getName())))) {
			return;
		}
	} else if(!aCompletedOnly) {
		if (removeFinished) {
			if (!WinUtil::showQuestionBox(TSTRING_F(CONFIRM_REMOVE_DIR_FINISHED_MULTIPLE, bundles.size()), MB_ICONQUESTION)) {
				return;
			}
		} else if (!allFinished && !WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_QUEUE_REMOVAL, TSTRING_F(CONFIRM_REMOVE_DIR_MULTIPLE, bundles.size()))) {
			return;
		}
	}

	MainFrame::getMainFrame()->addThreadedTask([=] {
		for (auto b : bundles) {
			if (!aCompletedOnly || b->isCompleted()) {
				QueueManager::getInstance()->removeBundle(b, removeFinished);
			}
		}
	});
}

void QueueFrame::handleRemoveFiles(QueueItemList queueitems, bool removeFinished) {
	if (queueitems.size() >= 1) {
		if (WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_QUEUE_REMOVAL, TSTRING(REALLY_REMOVE))) {
			for (auto& qi : queueitems)
				QueueManager::getInstance()->removeFile(qi->getToken(), removeFinished);
		}
	}
}

void QueueFrame::handleSearchQI(const QueueItemPtr& aQI, bool byName) {
	if (aQI) {
		if (byName)
			ActionUtil::search(Text::toT(PathUtil::getFileName(aQI->getTarget())));
		else
			ActionUtil::searchHash(aQI->getTTH(), PathUtil::getFileName(aQI->getTarget()), aQI->getSize());
	}
}

void QueueFrame::onBundleAdded(const BundlePtr& aBundle) {
	auto p = findParent(aBundle->getToken());
	if (!p) {
		QueueItemInfoPtr item = new QueueItemInfo(aBundle);
		parents.emplace(aBundle->getToken(), item);
		if (show(item))
			ctrlQueue.list.insertItem(item.get(), item->getImageIndex());
		addLocationItem(getBundleParent(aBundle));
	}
}

void QueueFrame::insertItems(QueueItemInfoPtr Qii) {

	BundlePtr aBundle = Qii->bundle;

	ctrlQueue.list.insertItem(0, iBack.get(), iBack->getImageIndex());

	if (aBundle && !aBundle->isFileBundle() && !Qii->childrenCreated) {
		Qii->childrenCreated = true;
		{
			RLock l(QueueManager::getInstance()->getCS());
			ranges::for_each(aBundle->getQueueItems(), [&](const QueueItemPtr& qi) { Qii->addChild(qi); });
			ranges::for_each(aBundle->getFinishedFiles(), [&](const QueueItemPtr& qi) { Qii->addChild(qi); });
		}
	}

	if (Qii->isDirectory || (aBundle && !aBundle->isFileBundle())) {
		for (auto item : Qii->children | views::values) {
			if (item->isDirectory)
				item->updateSubDirectories();
			if (show(item))
				ctrlQueue.list.insertItem(item.get(), item->getImageIndex());
		}
	}
}

QueueFrame::QueueItemInfoPtr QueueFrame::findParent(QueueToken aToken) {
	auto i = parents.find(aToken);
	return i != parents.end() ? i->second : nullptr;
}

const QueueFrame::QueueItemInfoPtr QueueFrame::findItemByPath(const string& aPath) {
	for (auto b : parents | views::values){
		if (b->bundle && !b->bundle->isFileBundle()) {
			if (aPath == b->bundle->getTarget())
				return b;
			else if (PathUtil::isSubLocal(aPath, b->bundle->getTarget())) {
				return b->findChild(aPath);
			}
		}
	}
	return nullptr;
}

QueueFrame::QueueItemInfoPtr QueueFrame::findQueueItem(const QueueItemPtr& aQI) {
	auto parent = findParent(aQI->getBundle() ? aQI->getBundle()->getToken() : aQI->getToken());
	if (parent && aQI->getBundle() && !aQI->getBundle()->isFileBundle()) {
		return parent->childrenCreated ? parent->findChild(aQI->getTarget()) : nullptr;
	}
	return parent;
}

void QueueFrame::onBundleRemoved(const BundlePtr& aBundle, const string& aPath) {
	auto i = parents.find(aBundle->getToken());;
	if (i != parents.end()) {
		removeLocationItem(aPath);
		if (curDirectory && PathUtil::isParentOrExactLocal(aBundle->getTarget(), curDirectory->getTarget()))
			ctrlQueue.list.DeleteAllItems();
		else
			ctrlQueue.list.deleteItem(i->second.get());
		parents.erase(i);
	}
}

void QueueFrame::onBundleUpdated(const BundlePtr& aBundle) {
	auto parent = findParent(aBundle->getToken());
	if (parent) {
		int i = ctrlQueue.list.findItem(parent.get());
		if (show(parent)) {
			if (i == -1) {
				i = ctrlQueue.list.insertItem(ctrlQueue.list.getSortPos(parent.get()), parent.get(), parent->getImageIndex());
			} else {
				ctrlQueue.list.updateItem(i);
			}
		} else if(i != -1) {
			ctrlQueue.list.DeleteItem(i);
		}
	}
}

void QueueFrame::onQueueItemRemoved(const QueueItemPtr& aQI) {
	auto item = findQueueItem(aQI);
	if (!item)
		return;

	if (!item->getParent()){ //temp or file list
		ctrlQueue.list.deleteItem(item.get());
		parents.erase(aQI->getToken());
		return;
	}

	if (item->getParent()){
		item->getParent()->children.erase(item->getTarget());
	}
	if (item->getParent() == curDirectory)
		ctrlQueue.list.deleteItem(item.get());

	updateParentDirectories(item);

}

void QueueFrame::onQueueItemUpdated(const QueueItemPtr& aQI) {
	if (aQI->getBundle() && !aQI->getBundle()->isFileBundle() && !curDirectory) //Nothing to update, not worth looking for the item.
		return;
	
	auto item = findQueueItem(aQI);
	if (!item)
		return;

	updateParentDirectories(item);

	if (item->getParent() == curDirectory) {
		if (show(item))
			ctrlQueue.list.updateItem(item.get());
		else
			ctrlQueue.list.deleteItem(item.get());
	}
}

void QueueFrame::onQueueItemAdded(const QueueItemPtr& aQI) {
	if (aQI->getBundle()) {
		auto parent = findParent(aQI->getBundle()->getToken());
		if (!parent || !parent->childrenCreated)
			return;

		if (aQI->getBundle()->isFileBundle()) {
			if(show(parent))
				ctrlQueue.list.updateItem(parent.get());
			return;
		}

		auto item = parent->addChild(aQI);
		if (curDirectory == item->getParent()) {
			ctrlQueue.list.insertItem(item.get(), item->getImageIndex());
		}

		updateParentDirectories(item);

	} else { // File list or a Temp item
		auto item = findParent(aQI->getToken());
		if (!item) {
			item = new QueueItemInfo(aQI, nullptr);
			parents.emplace(aQI->getToken(), item);
			if (show(item))
				ctrlQueue.list.insertItem(item.get(), item->getImageIndex());
		}
	}
}

void QueueFrame::executeGuiTasks() {
	if (tasks.getTasks().empty())
		return;
	ctrlQueue.list.SetRedraw(FALSE);
	for (;;) {
		TaskQueue::TaskPair t;
		if (!tasks.getFront(t))
			break;

		static_cast<AsyncTask*>(t.second)->f();
		statusDirty = statusDirty || t.first < TASK_BUNDLE_UPDATE;
		tasks.pop_front();
	}
	ctrlQueue.list.SetRedraw(TRUE);
}

void QueueFrame::updateStatus() {
	if (statusDirty) {
		bool u = false;
		int queuedBundles = 0;
		int finishedBundles = 0;
		int failedBundles = 0;
		int queuedItems = 0;
		int filelistItems = 0;
		int totalItems = 0;
		int pausedItems = 0;
		int autosearchAdded = 0;

		for (auto ii : parents | views::values) {
			if (ii->bundle) {
				BundlePtr b = ii->bundle;
				b->isDownloaded() ? finishedBundles++ : queuedBundles++;
				if (b->isFailed()) failedBundles++;
				if (b->isPausedPrio()) pausedItems++;
				if (b->getAddedByAutoSearch()) autosearchAdded++;
			} else {
				queuedItems++;
				if (ii->qi && ii->qi->isSet(QueueItem::FLAG_USER_LIST))
					filelistItems++;
			}
		}

		auto qm = QueueManager::getInstance();
		{
			RLock l(qm->getCS());
			totalItems = qm->getFileQueueUnsafe().size();
		}

		ctrlTree.SetRedraw(FALSE);
		HTREEITEM ht = bundleParent;
		while (ht != NULL) {
			switch (ctrlTree.GetItemData(ht)) {
			case TREE_BUNDLES:
				ctrlTree.SetItemText(ht, (TSTRING(BUNDLES) + _T(" ( ") + WinUtil::toStringW(finishedBundles + queuedBundles) + _T(" )")).c_str());
				ht = ctrlTree.GetChildItem(bundleParent);
				break;
			case TREE_FINISHED:
				ctrlTree.SetItemText(ht, (TSTRING(FINISHED) + _T(" ( ") + (WinUtil::toStringW(finishedBundles)) + _T(" )")).c_str());
				ht = ctrlTree.GetNextSiblingItem(ht);
				break;
			case TREE_QUEUED:
				ctrlTree.SetItemText(ht, (TSTRING(QUEUED) + _T(" ( ") + (WinUtil::toStringW(queuedBundles)) + _T(" )")).c_str());
				ht = ctrlTree.GetNextSiblingItem(ht);
				break;
			case TREE_FAILED:
				ctrlTree.SetItemText(ht, (TSTRING(FAILED) + _T(" ( ") + (WinUtil::toStringW(failedBundles)) + _T(" )")).c_str());
				ht = ctrlTree.GetNextSiblingItem(ht);
				break;
			case TREE_PAUSED:
				ctrlTree.SetItemText(ht, (TSTRING(PAUSED) + _T(" ( ") + (WinUtil::toStringW(pausedItems)) + _T(" )")).c_str());
				ht = ctrlTree.GetNextSiblingItem(ht);
				break;
			case TREE_AUTOSEARCH:
				ctrlTree.SetItemText(ht, (TSTRING(AUTO_SEARCH) + _T(" ( ") + (WinUtil::toStringW(autosearchAdded)) + _T(" )")).c_str());
				ht = ctrlTree.GetNextSiblingItem(ht);
				break;
			case TREE_LOCATION:
				ctrlTree.SetItemText(ht, (TSTRING(LOCATIONS) + _T(" ( ") + (WinUtil::toStringW(finishedBundles + queuedBundles)) + _T(" )")).c_str());
				ht = ctrlTree.GetNextSiblingItem(bundleParent); //End of bundle sub items, get next parent item.
				break;
			case TREE_FILELIST:
				ctrlTree.SetItemText(ht, (TSTRING(FILE_LISTS) + _T(" ( ") + (WinUtil::toStringW(filelistItems)) + _T(" )")).c_str());
				ht = ctrlTree.GetNextSiblingItem(ht);
				break;
			case TREE_TEMP:
				ctrlTree.SetItemText(ht, (TSTRING(TEMP_ITEMS) + _T(" ( ") + (WinUtil::toStringW(queuedItems - filelistItems)) + _T(" )")).c_str());
				ht = ctrlTree.GetNextSiblingItem(ht);
				break;

			default: break;
			}

		}
		ctrlTree.SetRedraw(TRUE);
		
		if (ctrlStatus.IsWindow()) {

			if (finishedBundles > 500) {
				ctrlStatus.SetText(1, CTSTRING_F(BUNDLE_X_FINISHED_WARNING, finishedBundles));
				ctrlStatus.SetIcon(1, ResourceLoader::getSeverityIcon(LogMessage::SEV_WARNING));
			} else {
				ctrlStatus.SetText(1, _T(""));
				ctrlStatus.SetIcon(1, NULL);
			}

			tstring tmp = TSTRING(FINISHED_BUNDLES) + _T(": ") + WinUtil::toStringW(finishedBundles);
			int w = WinUtil::getStatusTextWidth(tmp, ctrlStatus.m_hWnd);
			if (statusSizes[1] < w) {
				statusSizes[1] = w;
				u = true;
			}
			ctrlStatus.SetText(2, (tmp).c_str());

			tmp = TSTRING(QUEUED_BUNDLES) + _T(": ") + WinUtil::toStringW(queuedBundles);
			w = WinUtil::getStatusTextWidth(tmp, ctrlStatus.m_hWnd);
			if (statusSizes[2] < w) {
				statusSizes[2] = w;
				u = true;
			}
			ctrlStatus.SetText(3, (tmp).c_str());

			tmp = TSTRING_F(TOTAL_FILES, totalItems);
			w = WinUtil::getStatusTextWidth(tmp, ctrlStatus.m_hWnd);
			if (statusSizes[3] < w) {
				statusSizes[3] = w;
				u = true;
			}
			ctrlStatus.SetText(4, (tmp).c_str());

			if (u)
				UpdateLayout(TRUE);
		}
		statusDirty = false;
	}
}

void QueueFrame::addLocationItem(const string& aPath) {
	auto i = locations.find(aPath);
	if (i == locations.end()){
		HTREEITEM ht = addTreeItem(locationParent, TREE_LOCATION, Text::toT(aPath) + _T(" ( 1 )"), TVI_SORT);
		locations.emplace(aPath, treeLocationItem(ht));
	} else {
		i->second.bundles++;
		ctrlTree.SetItemText(i->second.item, Text::toT(aPath + " ( " + Util::toString(i->second.bundles) + " )").c_str());
	}
}

void QueueFrame::removeLocationItem(const string& aPath) {
	auto i = locations.find(aPath);
	if (i != locations.end()){
		auto litem = &i->second;
		auto hits = --litem->bundles;
		if (hits == 0){
			ctrlTree.DeleteItem(litem->item);
			locations.erase(i);
		} else {
			ctrlTree.SetItemText(i->second.item, Text::toT(aPath + " ( " + Util::toString(hits) + " )").c_str());
		}
	}
}

string QueueFrame::getBundleParent(const BundlePtr aBundle) {
	return aBundle->isFileBundle() ? PathUtil::getFilePath(aBundle->getTarget()) : PathUtil::getParentDir(aBundle->getTarget());
}

void QueueFrame::on(QueueManagerListener::BundleAdded, const BundlePtr& aBundle) noexcept {
	addGuiTask(TASK_ADD, [=] { onBundleAdded(aBundle); });
}
void QueueFrame::on(QueueManagerListener::BundleRemoved, const BundlePtr& aBundle) noexcept{
	addGuiTask(TASK_REMOVE, [=] { onBundleRemoved(aBundle, getBundleParent(aBundle)); });
}
void QueueFrame::on(QueueManagerListener::BundleSize, const BundlePtr& aBundle) noexcept { 
	addGuiTask(TASK_BUNDLE_UPDATE, [=] { onBundleUpdated(aBundle); });
}
void QueueFrame::on(QueueManagerListener::BundlePriority, const BundlePtr& aBundle) noexcept { 
	addGuiTask(TASK_BUNDLE_STATUS, [=] { onBundleUpdated(aBundle); });
}
void QueueFrame::on(QueueManagerListener::BundleStatusChanged, const BundlePtr& aBundle) noexcept { 
	addGuiTask(TASK_BUNDLE_STATUS, [=] { onBundleUpdated(aBundle); });
}
void QueueFrame::on(QueueManagerListener::BundleSources, const BundlePtr& aBundle) noexcept { 
	addGuiTask(TASK_BUNDLE_UPDATE, [=] { onBundleUpdated(aBundle); });
}

void QueueFrame::on(QueueManagerListener::ItemRemoved, const QueueItemPtr& aQI, bool /*finished*/) noexcept{
	addGuiTask(TASK_REMOVE, [=] { onQueueItemRemoved(aQI); });
}
void QueueFrame::on(QueueManagerListener::ItemAdded, const QueueItemPtr& aQI) noexcept{
	addGuiTask(TASK_ADD, [=] { onQueueItemAdded(aQI); });
}
void QueueFrame::on(QueueManagerListener::ItemSources, const QueueItemPtr& aQI) noexcept {
	addGuiTask(TASK_QI_UPDATE, [=] { onQueueItemUpdated(aQI); });
}
void QueueFrame::on(QueueManagerListener::ItemStatus, const QueueItemPtr& aQI) noexcept{
	addGuiTask(TASK_QI_UPDATE, [=] { onQueueItemUpdated(aQI); });
}

void QueueFrame::on(QueueManagerListener::ItemPriority, const QueueItemPtr& aQI) noexcept {
	addGuiTask(TASK_QI_UPDATE, [=] { onQueueItemUpdated(aQI); });
}

void QueueFrame::on(QueueManagerListener::ItemTick, const QueueItemPtr& aQI) noexcept {
	addGuiTask(TASK_QI_UPDATE, [=] { onQueueItemUpdated(aQI); });
}

void QueueFrame::on(FileRecheckFailed, const QueueItemPtr&, const string& aError) noexcept {
	callAsync([=]{ ctrlStatus.SetText(1, Text::toT(aError).c_str()); });
}

void QueueFrame::on(DownloadManagerListener::BundleTick, const BundleList& tickBundles, uint64_t /*aTick*/) noexcept{
	for (auto& b : tickBundles) {
		addGuiTask(TASK_BUNDLE_UPDATE, [=] { onBundleUpdated(b); });
	}
}

void QueueFrame::on(QueueManagerListener::BundleDownloadStatus, const BundlePtr& aBundle) noexcept { 
	addGuiTask(TASK_BUNDLE_STATUS, [=] { onBundleUpdated(aBundle); });
}

void QueueFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept{
	bool refresh = false;
	if (ctrlQueue.list.GetBkColor() != WinUtil::bgColor) {
		ctrlQueue.list.SetBkColor(WinUtil::bgColor);
		ctrlQueue.list.SetTextBkColor(WinUtil::bgColor);
		ctrlQueue.list.setFlickerFree(WinUtil::bgBrush);
		ctrlTree.SetBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if (ctrlQueue.list.GetTextColor() != WinUtil::textColor) {
		ctrlQueue.list.SetTextColor(WinUtil::textColor);
		ctrlTree.SetTextColor(WinUtil::textColor);
		refresh = true;
	}

	if (ctrlQueue.list.GetFont() != WinUtil::listViewFont){
		ctrlQueue.list.SetFont(WinUtil::listViewFont);
		refresh = true;
	}

	if (refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

/*QueueItemInfo functions*/

QueueFrame::QueueItemInfo::~QueueItemInfo() {
	children.clear();
	// dcdebug("QueueItemInfo: destructed %s \r\n", bundle ? bundle->getName().c_str() : qi ? qi->getTargetFileName().c_str() : Text::fromT(name).c_str());
}

int QueueFrame::QueueItemInfo::getImageIndex() const {
	if (bundle)
		return bundle->isFileBundle() ? ResourceLoader::getIconIndex(Text::toT(bundle->getTarget())) : ResourceLoader::DIR_NORMAL;
	else if (qi)
		return ResourceLoader::getIconIndex(Text::toT(qi->getTarget()));
	else if (isDirectory) {
		if (!parent && this == iBack)
			return ResourceLoader::DIR_STEPBACK;
		return ResourceLoader::DIR_NORMAL;
	} else
		return -1;
}

void QueueFrame::QueueItemInfo::getChildQueueItems(QueueItemList& ret) {
	for (auto i : children | views::values) {
		if (i->isDirectory)
			i->getChildQueueItems(ret);
		else if (i->qi)
			ret.push_back(i->qi);
	}
}

QueueFrame::QueueItemInfoPtr QueueFrame::QueueItemInfo::findChild(const string& aKey) {
	string::size_type i = 0, j = 0;

	string itemTarget = PathUtil::ensureTrailingSlash(getTarget());

	string tmp = aKey.substr(itemTarget.size());

	QueueItemInfoPtr dir = this;
	while ((i = tmp.find(PATH_SEPARATOR, j)) != string::npos) {
		auto d = dir->children.find(itemTarget + tmp.substr(0, i + 1));
		if (d != dir->children.end())
			dir = d->second;
		j = i + 1;
	}

	if (PathUtil::isDirectoryPath(aKey))
		return dir;

	auto ret = dir->children.find(aKey);
	if (ret != dir->children.end())
		return ret->second;

	return nullptr;
}

QueueFrame::QueueItemInfoPtr QueueFrame::QueueItemInfo::addChild(const QueueItemPtr& aQI) {
	string itemTarget = PathUtil::ensureTrailingSlash(getTarget());
	
	string tmp = aQI->getTarget().substr(itemTarget.size());

	size_t i = 0, j = 0;
	QueueItemInfoPtr dir(this);
	while ((i = tmp.find(PATH_SEPARATOR, j)) != string::npos) {
		string curPath = itemTarget + tmp.substr(0, i + 1);
		auto d = dir->children.find(curPath);
		if (d == dir->children.end()) {
			dir = new QueueItemInfo(Text::toT(PathUtil::getLastDir(curPath)), dir.get(), curPath);
			dir->getParent()->children.emplace(curPath, dir);
		} else {
			dir = d->second;
		}
		j = i + 1;
	}

	QueueItemInfoPtr item = new QueueItemInfo(aQI, dir.get());
	dir->setTotalSize(dir->getTotalSize() + aQI->getSize());
	dir->children.emplace(aQI->getTarget(), item);

	return item;
}

void QueueFrame::QueueItemInfo::updateSubDirectories() {
	int64_t newSize = 0;
	int64_t newDownloadedBytes = 0;
	for (auto ii : children | views::values) {
		if (ii->isDirectory)
			ii->updateSubDirectories();

		newSize += ii->getSize();
		newDownloadedBytes += ii->getDownloadedBytes();

	}
	setTotalSize(newSize);
	setFinishedBytes(newDownloadedBytes);
}

void QueueFrame::updateParentDirectories(QueueItemInfoPtr Qii) {
	if (!Qii->getParent() || !curDirectory)
		return;
	
	if (PathUtil::isSubLocal(Qii->getParent()->getTarget(), curDirectory->getTarget())) {
		auto cur = Qii;
		while ((cur = cur->getParent()) && !cur->bundle && cur != curDirectory) {
			int64_t newSize = 0;
			int64_t newDownloadedBytes = 0;
			for (auto ii : cur->children | views::values) {
				newSize += ii->getSize();
				newDownloadedBytes += ii->getDownloadedBytes();
			}
			cur->setTotalSize(newSize);
			cur->setFinishedBytes(newDownloadedBytes);

			//remove empty directories
			if (cur->children.empty()){
				cur->getParent()->children.erase(cur->target);
			}

			//update the directory that is currently inside view
			if (cur->getParent() == curDirectory) {
				if (show(cur)) {
					int i = ctrlQueue.list.findItem(cur.get());
					if (i != -1)
						ctrlQueue.list.updateItem(i);
					else
						ctrlQueue.list.insertItem(cur.get(), cur->getImageIndex());
				} else
					ctrlQueue.list.deleteItem(cur.get());
			}
		}
	}
}


const tstring QueueFrame::QueueItemInfo::getText(int col) const {

	switch (col) {
		case COLUMN_NAME: return getName();
		case COLUMN_SIZE: return (getSize() != -1) ? Util::formatBytesW(getSize()) : Util::emptyStringT;
		case COLUMN_TYPE: return getType();
		case COLUMN_PRIORITY:
		{
			if (isFinished() || getPriority() == Priority::DEFAULT)
				return Util::emptyStringT;
			bool autoPrio = (bundle && bundle->getAutoPriority()) || (qi && qi->getAutoPriority());

			if (getPriority() == Priority::PAUSED_FORCE && bundle && bundle->getResumeTime() > 0)
				return TSTRING_F(PAUSED_UNTIL_X, Text::toT(Util::getTimeStamp(bundle->getResumeTime()))); //Show time left instead? needs bundle updates to be implemented for these...

			return Text::toT(Util::formatPriority(getPriority())) + (autoPrio ? _T(" (") + TSTRING(AUTO) + _T(")") : Util::emptyStringT);
		}
		case COLUMN_STATUS: return getStatusString();
		case COLUMN_TIMELEFT: {
			uint64_t left = getSecondsLeft();
			return left > 0 ? Util::formatSecondsW(left) : Util::emptyStringT;
		}
		case COLUMN_SPEED: {
			int64_t speed = getSpeed();
			return speed > 0 ? Util::formatBytesW(speed) + _T("/s") : Util::emptyStringT;
		}
		case COLUMN_SOURCES: return /*!isFinished() && */!isDirectory ? getSourceString() : Util::emptyStringT;
		case COLUMN_DOWNLOADED: return Util::formatBytesW(getDownloadedBytes());
		case COLUMN_TIME_ADDED: return getTimeAdded() > 0 ? FormatUtil::formatDateTimeW(getTimeAdded()) : Util::emptyStringT;
		case COLUMN_TIME_FINISHED: return isFinished() && getTimeFinished() > 0 ? FormatUtil::formatDateTimeW(getTimeFinished()) : Util::emptyStringT;
		case COLUMN_PATH: return Text::toT(getTarget());
		default: return Util::emptyStringT;
	}
}

tstring QueueFrame::QueueItemInfo::getType() const {
	if (bundle) {
		if (bundle->isFileBundle()) {
			return WinUtil::formatFileType(bundle->getTarget());
		} else {
			return WinUtil::formatFolderContent(QueueManager::getInstance()->getBundleContent(bundle));
		}
	} else if (isFilelist()) {
		return TSTRING(TYPE_FILE_LIST);
	} else if (qi) {
		return WinUtil::formatFileType(qi->getTarget());
	} else
		return Util::emptyStringT;
}

tstring QueueFrame::QueueItemInfo::getName() const {
	if (bundle) {
		return Text::toT(bundle->getName());
	} else if (qi) {
		return Text::toT(qi->getTargetFileName());
	}
	return name;
}

const string& QueueFrame::QueueItemInfo::getTarget() const {
	return bundle ? bundle->getTarget() : qi ? qi->getTarget() : target;
}

int64_t QueueFrame::QueueItemInfo::getSize() const {
	return bundle ? bundle->getSize() : qi ? qi->getSize() : isDirectory ? getTotalSize() : -1;
}

int64_t QueueFrame::QueueItemInfo::getSpeed() const {
	return bundle ? bundle->getSpeed() : qi ? QueueManager::getInstance()->getAverageSpeed(qi) : 0;
}

uint64_t QueueFrame::QueueItemInfo::getSecondsLeft() const {
	return bundle ? bundle->getSecondsLeft() : qi ? QueueManager::getInstance()->getSecondsLeft(qi) : 0;
}

time_t QueueFrame::QueueItemInfo::getTimeAdded() const {
	return bundle ? bundle->getTimeAdded() : qi ? qi->getTimeAdded() : 0;
}

time_t QueueFrame::QueueItemInfo::getTimeFinished() const {
	return bundle ? bundle->getTimeFinished() : qi ? qi->getTimeFinished() : 0;
}

Priority QueueFrame::QueueItemInfo::getPriority() const {
	return bundle ? bundle->getPriority() : qi ? qi->getPriority() : Priority::DEFAULT;
}
 
bool QueueFrame::QueueItemInfo::isFinished() const {
	return bundle ? bundle->isDownloaded() : 
		qi ? qi->isDownloaded() :
		isDirectory && getTotalSize() == getFinishedBytes();
}

bool QueueFrame::QueueItemInfo::isTempItem() const {
	if (bundle)
		return false;
	return qi && !qi->getBundle() && !qi->isSet(QueueItem::FLAG_USER_LIST);
}

bool QueueFrame::QueueItemInfo::isFilelist() const {
	if (bundle)
		return false;
	return qi && qi->isSet(QueueItem::FLAG_USER_LIST);
}

double QueueFrame::QueueItemInfo::getPercentage() const {
	return getSize() > 0 ? (double) getDownloadedBytes()*100.0 / (double) getSize() : 0;
}

tstring QueueFrame::QueueItemInfo::getStatusString() const {
	if (bundle) {
		return Text::toT(bundle->getStatusString());
	} else if (qi) {
		return Text::toT(qi->getStatusString(QueueManager::getInstance()->getDownloadedBytes(qi), QueueManager::getInstance()->isWaiting(qi)));
	} else if (isDirectory && getTotalSize() != -1) {
		if (getFinishedBytes() > 0 && getTotalSize() == getFinishedBytes())
			return TSTRING(FINISHED);
		return Text::toT(str(boost::format("%.01f%%") % getPercentage()));
	}

	return Util::emptyStringT;
}
tstring QueueFrame::QueueItemInfo::getSourceString() const {
	auto size = 0;
	int online = 0;
	if (bundle) {
		if (isFinished())
			return Util::emptyStringT;

		Bundle::SourceList sources = QueueManager::getInstance()->getBundleSources(bundle);
		size = sources.size();
		for (const auto& s : sources) {
			if (s.getUser().user->isOnline())
				online++;
		}
	} else if(qi) {
		if (isFinished())
			return Text::toT(qi->getLastSource());

		QueueItem::SourceList sources = QueueManager::getInstance()->getSources(qi);
		size = sources.size();
		for (const auto& s : sources) {
			if (s.getUser().user->isOnline())
				online++;
		}
	}

	return size == 0 ? TSTRING(NONE) : TSTRING_F(USERS_ONLINE, online % size);
}

int64_t QueueFrame::QueueItemInfo::getDownloadedBytes() const {
	return bundle ? bundle->getDownloadedBytes() : qi ? QueueManager::getInstance()->getDownloadedBytes(qi) : getFinishedBytes();
}

void QueueFrame::QueueItemInfo::deleteSubdirs() {
	ranges::for_each(children | views::values, [&](QueueItemInfoPtr d) { if (d->isDirectory) { d->deleteSubdirs(); } });
	File::removeDirectory(target);
}

int QueueFrame::QueueItemInfo::compareItems(const QueueItemInfo* a, const QueueItemInfo* b, int col) {
	// TODO: lots of things to fix

	if (a == iBack) return -1;
	if (b == iBack) return 1;

	switch (col) {
	case COLUMN_NAME: {

		if (a->bundle && b->bundle) {
			if (a->bundle->isFileBundle() && !b->bundle->isFileBundle()) return 1;
			if (!a->bundle->isFileBundle() && b->bundle->isFileBundle()) return -1;
		}

		auto textA = a->getName();
		auto textB = b->getName();
		if (!a->bundle) {
			if (a->isDirectory && !b->isDirectory) return -1;
			if (!a->isDirectory && b->isDirectory) return 1;
		}

		return Util::DefaultSort(textA.c_str(), textB.c_str());
	}
	case COLUMN_SIZE: return compare(a->getSize(), b->getSize());
	case COLUMN_TYPE: {
		if (a->bundle && b->bundle) {
			if (a->bundle->isFileBundle() != b->bundle->isFileBundle()) {
				return a->bundle->isFileBundle();
			} else if (!a->bundle->isFileBundle()) {
				RLock l(QueueManager::getInstance()->getCS());
				auto dirsA = QueueManager::getInstance()->bundleQueue.getDirectoryCount(a->bundle);
				auto dirsB = QueueManager::getInstance()->bundleQueue.getDirectoryCount(b->bundle);
				if (dirsA != dirsB) {
					return dirsA < dirsB ? 1 : -1;
				}

				auto filesA = a->bundle->getQueueItems().size() + a->bundle->getFinishedFiles().size();
				auto filesB = b->bundle->getQueueItems().size() + b->bundle->getFinishedFiles().size();

				return filesA < filesB ? 1 : -1;
			}
		}
	}
	case COLUMN_PRIORITY: {
		auto aFinished = a->isFinished();
		auto bFinished = b->isFinished();
		if (aFinished && !bFinished) return -1;
		if (!aFinished && bFinished) return 1;
		return compare(static_cast<int>(a->getPriority()), static_cast<int>(b->getPriority()));
	}
	case COLUMN_TIMELEFT: return compare(a->getSecondsLeft(), b->getSecondsLeft());
	case COLUMN_SPEED: return compare(a->getSpeed(), b->getSpeed());
	case COLUMN_DOWNLOADED: return compare(a->getDownloadedBytes(), b->getDownloadedBytes());
	default: 
		return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str());
	}
}

COLORREF QueueFrame::getStatusColor(uint8_t status) {
	switch (status) {
		case Bundle::STATUS_NEW:
		case Bundle::STATUS_QUEUED:
		case Bundle::STATUS_DOWNLOADED: return SETTING(DOWNLOAD_BAR_COLOR);
		case Bundle::STATUS_COMPLETED: return SETTING(COLOR_STATUS_FINISHED);
		case Bundle::STATUS_SHARED: return SETTING(COLOR_STATUS_SHARED);
		default: return SETTING(DOWNLOAD_BAR_COLOR);
	}
}
}