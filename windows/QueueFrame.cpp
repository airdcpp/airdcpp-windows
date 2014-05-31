/*
* Copyright (C) 2011-2014 AirDC++ Project
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

#include "QueueFrame.h"

#include "BarShader.h"
#include "BrowseDlg.h"
#include "LineDlg.h"
#include "MainFrm.h"
#include "PrivateFrame.h"
#include "ResourceLoader.h"

#include "../client/AirUtil.h"
#include "../client/DownloadManager.h"
#include "../client/ShareScannerManager.h"

int QueueFrame::columnIndexes[] = { COLUMN_NAME, COLUMN_SIZE, COLUMN_TYPE, COLUMN_PRIORITY, COLUMN_STATUS, COLUMN_TIMELEFT, 
	COLUMN_SPEED, COLUMN_SOURCES, COLUMN_DOWNLOADED, COLUMN_TIME_ADDED, COLUMN_TIME_FINISHED, COLUMN_PATH };

int QueueFrame::columnSizes[] = { 450, 70, 70, 100, 250, 80, 
	80, 80, 80, 100, 100, 500 };

static ResourceManager::Strings columnNames[] = { ResourceManager::NAME, ResourceManager::SIZE, ResourceManager::TYPE_CONTENT, ResourceManager::PRIORITY, ResourceManager::STATUS, ResourceManager::TIME_LEFT,
	ResourceManager::SPEED, ResourceManager::SOURCES, ResourceManager::DOWNLOADED, ResourceManager::TIME_ADDED, ResourceManager::TIME_FINISHED, ResourceManager::PATH };

static ResourceManager::Strings treeNames[] = { ResourceManager::BUNDLES, ResourceManager::FINISHED, ResourceManager::QUEUED, ResourceManager::FAILED, ResourceManager::PAUSED, ResourceManager::AUTO_SEARCH, ResourceManager::LOCATIONS, ResourceManager::FILE_LISTS, ResourceManager::TEMP_ITEMS };

LRESULT QueueFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{

	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	ctrlStatusContainer.SubclassWindow(ctrlStatus.m_hWnd);

	ctrlQueue.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_QUEUE_LIST);
	ctrlQueue.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
	ctrlQueue.SetImageList(ResourceLoader::getFileImages(), LVSIL_SMALL);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(QUEUEFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(QUEUEFRAME_WIDTHS), COLUMN_LAST);

	for (uint8_t j = 0; j < COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_SPEED || j == COLUMN_TIMELEFT) ? LVCFMT_RIGHT : (j == COLUMN_STATUS) ? LVCFMT_CENTER : LVCFMT_LEFT;
		ctrlQueue.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlQueue.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlQueue.setSortColumn(COLUMN_NAME);
	ctrlQueue.setVisible(SETTING(QUEUEFRAME_VISIBLE));
	ctrlQueue.SetBkColor(WinUtil::bgColor);
	ctrlQueue.SetTextBkColor(WinUtil::bgColor);
	ctrlQueue.SetTextColor(WinUtil::textColor);
	ctrlQueue.setFlickerFree(WinUtil::bgBrush);
	ctrlQueue.setInsertFunction(bind(&QueueFrame::insertItems, this, placeholders::_1));
	ctrlQueue.setFilterFunction(bind(&QueueFrame::show, this, placeholders::_1));

	ctrlTree.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
		TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP | TVS_TRACKSELECT,
		WS_EX_CLIENTEDGE, IDC_TREE);
	ctrlTree.SetExtendedStyle(TVS_EX_FADEINOUTEXPANDOS | TVS_EX_DOUBLEBUFFER, TVS_EX_FADEINOUTEXPANDOS | TVS_EX_DOUBLEBUFFER);

	if (SETTING(USE_EXPLORER_THEME)) {
		SetWindowTheme(ctrlTree.m_hWnd, L"explorer", NULL);
	}
	ctrlTree.SetImageList(ResourceLoader::getQueueTreeImages(), TVSIL_NORMAL);
	ctrlTree.SetBkColor(WinUtil::bgColor);
	ctrlTree.SetTextColor(WinUtil::textColor);

	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	SetSplitterPanes(ctrlTree.m_hWnd, ctrlQueue.m_hWnd);
	m_nProportionalPos = SETTING(QUEUE_SPLITTER_POS);

	CRect rc(SETTING(QUEUE_LEFT), SETTING(QUEUE_TOP), SETTING(QUEUE_RIGHT), SETTING(QUEUE_BOTTOM));
	if (!(rc.top == 0 && rc.bottom == 0 && rc.left == 0 && rc.right == 0))
		MoveWindow(rc, TRUE);


	FillTree();

	{
		auto qm = QueueManager::getInstance();

		RLock l(qm->getCS());
		for (const auto& b : qm->getBundles() | map_values)
			onBundleAdded(b);

		for (const auto& q : qm->getFileQueue() | map_values) {
			if (!q->getBundle())
				onQueueItemAdded(q);
		}
	}
	ctrlTree.SelectItem(bundleParent);

	QueueManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);

	memzero(statusSizes, sizeof(statusSizes));
	statusSizes[0] = 16;
	ctrlStatus.SetParts(6, statusSizes);
	updateStatus();

	::SetTimer(m_hWnd, 0, 500, 0);

	WinUtil::SetIcon(m_hWnd, IDI_QUEUE);
	bHandled = FALSE;
	return 1;
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
		int w[6];
		ctrlStatus.GetClientRect(sr);

		w[5] = sr.right - 16;
#define setw(x) w[x] = max(w[x+1] - statusSizes[x], 0)
		setw(4); setw(3); setw(2); setw(1);

		w[0] = 16;

		ctrlStatus.SetParts(6, w);

		ctrlStatus.GetRect(1, sr);
	}
	CRect rc = rect;
	SetSplitterRect(&rect);
	//ctrlQueue.MoveWindow(&rc);
}


LRESULT QueueFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if (!closed) {
		::KillTimer(m_hWnd, 0);
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
			SettingsManager::getInstance()->set(SettingsManager::QUEUE_SPLITTER_POS, m_nProportionalPos);
			SettingsManager::getInstance()->set(SettingsManager::QUEUE_BOTTOM, (rc.bottom > 0 ? rc.bottom : 0));
			SettingsManager::getInstance()->set(SettingsManager::QUEUE_TOP, (rc.top > 0 ? rc.top : 0));
			SettingsManager::getInstance()->set(SettingsManager::QUEUE_LEFT, (rc.left > 0 ? rc.left : 0));
			SettingsManager::getInstance()->set(SettingsManager::QUEUE_RIGHT, (rc.right > 0 ? rc.right : 0));
		}

		ctrlQueue.saveHeaderOrder(SettingsManager::QUEUEFRAME_ORDER,
			SettingsManager::QUEUEFRAME_WIDTHS, SettingsManager::QUEUEFRAME_VISIBLE);
		
		ctrlQueue.SetRedraw(FALSE);
		ctrlQueue.deleteAllItems();
		ctrlQueue.SetRedraw(TRUE);

		ctrlTree.DeleteAllItems();

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
			if (qii->bundle) {
				if (qii->bundle->isFailed()) {
					cd->clrText = SETTING(ERROR_COLOR);
					return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
				}
			}
		}
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: 
	{
			auto ii = reinterpret_cast<QueueItemInfo*>(cd->nmcd.lItemlParam);

			int colIndex = ctrlQueue.findColumn(cd->iSubItem);
			cd->clrTextBk = WinUtil::bgColor;

			if (colIndex == COLUMN_STATUS) {
				if (!SETTING(SHOW_PROGRESS_BARS) || !SETTING(SHOW_QUEUE_BARS) || ii->getSize() == -1 || (ii->bundle && ii->bundle->isFailed())) { // file lists don't have size in queue, don't even start to draw...
					bHandled = FALSE;
					return 0;
				}

				COLORREF clr = SETTING(PROGRESS_OVERRIDE_COLORS) ? 
					(ii->parent ? SETTING(PROGRESS_SEGMENT_COLOR) : getStatusColor(ii->bundle ? ii->bundle->getStatus() : 1)) : GetSysColor(COLOR_HIGHLIGHT);

				//this is just severely broken, msdn says GetSubItemRect requires a one based index
				//but it wont work and index 0 gives the rect of the whole item
				CRect rc;
				if (cd->iSubItem == 0) {
					ctrlQueue.GetItemRect((int)cd->nmcd.dwItemSpec, &rc, LVIR_LABEL);
				} else {
					ctrlQueue.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, &rc);
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
					ctrlQueue.GetItem(&lvItem);

					HIMAGELIST imageList = (HIMAGELIST)::SendMessage(ctrlQueue.m_hWnd, LVM_GETIMAGELIST, LVSIL_SMALL, 0);
					if (imageList) {
						//let's find out where to paint it
						//and draw the background to avoid having 
						//the selection color as background
						CRect iconRect;
						ctrlQueue.GetSubItemRect((int)cd->nmcd.dwItemSpec, 0, LVIR_ICON, iconRect);
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

LRESULT QueueFrame::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	if (kd->wVKey == VK_DELETE) {
		BundleList bl;
		QueueItemList ql;
		getSelectedItems(bl, ql);
		if (!bl.empty()) {
			handleRemoveBundles(bl, false);
		} else {
			handleRemoveFiles(ql, false);
		}
	}
	return 0;
}

LRESULT QueueFrame::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMITEMACTIVATE* l = (NMITEMACTIVATE*)pnmh;

	if (l->iItem != -1) {
		auto ii = (QueueItemInfo*)ctrlQueue.GetItemData(l->iItem);
		if (!ii->bundle)
			return 0;

		if (ii->collapsed)
			ctrlQueue.Expand(ii, l->iItem);
		else
			ctrlQueue.Collapse(ii, l->iItem);
	}
	bHandled = FALSE;
	return 0;
}

void QueueFrame::getSelectedItems(BundleList& bl, QueueItemList& ql, DWORD aFlag/* = LVNI_SELECTED*/) {
	int sel = -1;
	while ((sel = ctrlQueue.GetNextItem(sel, aFlag)) != -1) {
		QueueItemInfo* qii = (QueueItemInfo*) ctrlQueue.GetItemData(sel);
		if (qii->bundle)
			bl.push_back(qii->bundle);
		else
			ql.push_back(qii->qi);
	}
}

LRESULT QueueFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	bool listviewMenu = false;
	bool treeMenu = false;
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	if (reinterpret_cast<HWND>(wParam) == ctrlQueue && ctrlQueue.GetSelectedCount() > 0) {
		if (pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlQueue, pt);
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

		getSelectedItems(bl, queueItems, treeMenu ? LVNI_ALL : LVNI_SELECTED);
		if (treeMenu) {
			AppendTreeMenu(bl, queueItems, menu);
		} else {
			if (!bl.empty()) 
				AppendBundleMenu(bl, menu);
			else if (!queueItems.empty())
				AppendQiMenu(queueItems, menu);
		}
		menu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
		return TRUE;
	}

	bHandled = FALSE;
	return FALSE;
}

tstring QueueFrame::formatUser(const Bundle::BundleSource& bs) const {
	auto& u = bs.getUser();
	tstring nick = WinUtil::escapeMenu(WinUtil::getNicks(u));
	// add hub hint to menu
	bool addHint = !u.hint.empty(), addSpeed = u.user->getSpeed() > 0;
	nick += _T(" (") + TSTRING(FILES) + _T(": ") + Util::toStringW(bs.files);
	if (addHint || addSpeed) {
		nick += _T(", ");
		if (addSpeed) {
			nick += TSTRING(SPEED) + _T(": ") + Util::formatBytesW(u.user->getSpeed()) + _T("/s)");
		}
		if (addHint) {
			if (addSpeed) {
				nick += _T(", ");
			}
			nick += TSTRING(HUB) + _T(": ") + Text::toT(u.hint);
		}
	}
	nick += _T(")");
	return nick;
}

tstring QueueFrame::formatUser(const QueueItem::Source& s) const {
	tstring nick = WinUtil::escapeMenu(WinUtil::getNicks(s.getUser()));
	// add hub hint to menu
	if (!s.getUser().hint.empty())
		nick += _T(" (") + Text::toT(s.getUser().hint) + _T(")");

	return nick;
}

template<typename SourceType>
tstring formatSourceFlags(const SourceType& s) {
	OrderedStringSet reasons_;
	if (s.isSet(QueueItem::Source::FLAG_FILE_NOT_AVAILABLE)) {
		reasons_.insert(STRING(FILE_NOT_AVAILABLE));
	} else if (s.isSet(QueueItem::Source::FLAG_BAD_TREE)) {
		reasons_.insert(STRING(INVALID_TREE));
	} else if (s.isSet(QueueItem::Source::FLAG_NO_NEED_PARTS)) {
		reasons_.insert(STRING(NO_NEEDED_PART));
	} else if (s.isSet(QueueItem::Source::FLAG_NO_TTHF)) {
		reasons_.insert(STRING(SOURCE_TOO_OLD));
	} else if (s.isSet(QueueItem::Source::FLAG_SLOW_SOURCE)) {
		reasons_.insert(STRING(SLOW_USER));
	} else if (s.isSet(QueueItem::Source::FLAG_UNTRUSTED)) {
		reasons_.insert(STRING(CERTIFICATE_NOT_TRUSTED));
	}

	if (!reasons_.empty()) {
		return _T(" (") + Text::toT(Util::listToString(reasons_)) + _T(")");
	}

	return Util::emptyStringT;
}
void QueueFrame::AppendTreeMenu(BundleList& bl, QueueItemList& ql, OMenu& aMenu) {
	if (!bl.empty()) {
		bool filesOnly = all_of(bl.begin(), bl.end(), [](const BundlePtr& b) { return b->isFileBundle(); });
		bool hasFinished = all_of(bl.begin(), bl.end(), [](const BundlePtr& b) { return b->isFinished(); });

		aMenu.InsertSeparatorFirst(CTSTRING_F(X_BUNDLES, bl.size()));

		WinUtil::appendBundlePrioMenu(aMenu, bl);
		if (hasFinished)
			aMenu.appendItem(TSTRING(RUN_SFV_CHECK), [=] { handleCheckSFV(true); });
		aMenu.appendSeparator();
		aMenu.appendItem(TSTRING(REMOVE), [=] { handleRemoveBundles(bl, false); });
		if (!filesOnly || hasFinished)
			aMenu.appendItem(TSTRING(REMOVE_WITH_FILES), [=] { handleRemoveBundles(bl, true); });
	}
	else if (!ql.empty()) {
		aMenu.InsertSeparatorFirst(CTSTRING_F(X_FILES, ql.size()));
		bool hasFinished = any_of(ql.begin(), ql.end(), [](const QueueItemPtr& q) { return q->isFinished(); });
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

	WinUtil::appendBundlePrioMenu(bundleMenu, bl);
	ListType::MenuItemList customItems;
	ctrlQueue.appendCopyMenu(bundleMenu, customItems);
	
	bool hasFinished = any_of(bl.begin(), bl.end(), [](const BundlePtr& b) { return b->isFinished(); });
	bool filesOnly = all_of(bl.begin(), bl.end(), [](const BundlePtr& b) { return b->isFileBundle(); });

	/* Insert sub menus */
	BundlePtr b = nullptr;
	if (bl.size() == 1) {
		b = bl.front();
	}

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
					QueueManager::getInstance()->readdBundleSource(b, si.getUser());
			}, OMenu::FLAG_THREADED);
			readdMenu->appendSeparator();
		}

		for (auto& bs : badBundleSources) {
			auto u = bs.getUser();
			readdMenu->appendItem(formatUser(bs) + formatSourceFlags(bs), [=] { QueueManager::getInstance()->readdBundleSource(b, u); }, OMenu::FLAG_THREADED);
		}
		/* Sub menus end */

		// search
		bundleMenu.appendItem(TSTRING(SEARCH_BUNDLE_ALT), [=] {
			auto bundle = b;
			QueueManager::getInstance()->searchBundle(bundle, true);
		}, OMenu::FLAG_THREADED);

		bundleMenu.appendSeparator();

		WinUtil::appendSearchMenu(bundleMenu, b->getName());
		bundleMenu.appendItem(TSTRING(SEARCH_DIRECTORY), [this] { handleSearchDirectory(); });

		if (b->isFailed()) {
			bundleMenu.appendSeparator();
			bundleMenu.appendItem(TSTRING(RETRY_SHARING), [=] { QueueManager::getInstance()->shareBundle(b, false); }, OMenu::FLAG_THREADED);
			if (b->getStatus() == Bundle::STATUS_SHARING_FAILED || b->getStatus() == Bundle::STATUS_FAILED_MISSING) {
				bundleMenu.appendItem(TSTRING(FORCE_SHARING), [=] { QueueManager::getInstance()->shareBundle(b, true); }, OMenu::FLAG_THREADED);
			}
		}

		if (!b->isFinished()) {
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
	
	bundleMenu.appendSeparator();
	bundleMenu.appendItem(TSTRING(RUN_SFV_CHECK), [=] { handleCheckSFV(false); });
	if (b) {
		bundleMenu.appendItem(TSTRING(RENAME), [=] { onRenameBundle(b); });
	}
	bundleMenu.appendItem(TSTRING(MOVE), [=] { handleMoveBundles(bl); });

	bundleMenu.appendSeparator();
	bundleMenu.appendItem(TSTRING(REMOVE), [=] { handleRemoveBundles(bl, false); });
	if (!filesOnly || hasFinished)
		bundleMenu.appendItem(TSTRING(REMOVE_WITH_FILES), [=] { handleRemoveBundles(bl, true); });

	if (b)
		bundleMenu.appendShellMenu({ b->getTarget() });
}

/*QueueItem Menu*/
void QueueFrame::AppendQiMenu(QueueItemList& ql, ShellMenu& fileMenu) {

	/* Do we need to control segment counts??
	OMenu segmentsMenu;
	segmentsMenu.CreatePopupMenu();
	segmentsMenu.InsertSeparatorFirst(TSTRING(MAX_SEGMENTS_NUMBER));
	for (int i = IDC_SEGMENTONE; i <= IDC_SEGMENTTEN; i++)
	segmentsMenu.AppendMenu(MF_STRING, i, (Util::toStringW(i - 109) + _T(" ") + TSTRING(SEGMENTS)).c_str());
	*/
	bool hasFinished = any_of(ql.begin(), ql.end(), [](const QueueItemPtr& q) { return q->isFinished(); });
	bool hasBundleItems = any_of(ql.begin(), ql.end(), [](const QueueItemPtr& q) { return q->getBundle(); });

	if (hasBundleItems)
		WinUtil::appendFilePrioMenu(fileMenu, ql);


	ListType::MenuItemList customItems{
		{ TSTRING(MAGNET_LINK), &handleCopyMagnet }
	};
	ctrlQueue.appendCopyMenu(fileMenu, customItems);

	QueueItemPtr qi = nullptr;

	if (ql.size() == 1) {
		qi = ql.front();
	}

	if (qi) {
		OMenu* removeAllMenu = fileMenu.getMenu();
		OMenu* removeMenu = fileMenu.getMenu();
		OMenu* readdMenu = fileMenu.getMenu();

		/* Create submenus */
		//segmentsMenu.CheckMenuItem(qi->getMaxSegments(), MF_BYPOSITION | MF_CHECKED);

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
					QueueManager::getInstance()->readdQISource(qi->getTarget(), si.getUser());
			}, OMenu::FLAG_THREADED);
			readdMenu->appendSeparator();
		}

		for (auto& s : badSources) {
			auto u = s.getUser();
			readdMenu->appendItem(formatUser(s), [=] { QueueManager::getInstance()->readdQISource(qi->getTarget(), u); });
		}

		//fileMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)segmentsMenu, CTSTRING(MAX_SEGMENTS_NUMBER));
		/* Submenus end */

		fileMenu.InsertSeparatorFirst(TSTRING(FILE));
		if (!qi->isSet(QueueItem::FLAG_USER_LIST)) {
			fileMenu.appendItem(CTSTRING(SEARCH_FOR_ALTERNATES), [=] { handleSearchQI(qi); });
			WinUtil::appendPreviewMenu(fileMenu, qi->getTarget());
		}

		if (hasBundleItems) {
			WinUtil::appendSearchMenu(fileMenu, Util::getFilePath(qi->getTarget()));
			//fileMenu.appendSeparator();
		}

		if (!qi->isFinished()) {
			fileMenu.appendSeparator();
			readdMenu->appendThis(TSTRING(READD_SOURCE), true);
			removeMenu->appendThis(TSTRING(REMOVE_SOURCE), true);
			removeAllMenu->appendThis(TSTRING(REMOVE_FROM_ALL), true);
			fileMenu.AppendMenu(MF_STRING, IDC_REMOVE_OFFLINE, CTSTRING(REMOVE_OFFLINE));

			fileMenu.appendSeparator();
			appendUserMenu<QueueItem::Source>(fileMenu, sources);
			fileMenu.appendSeparator();
		}
		else if (hasBundleItems) {
			fileMenu.appendItem(TSTRING(OPEN), [=] { handleOpenFile(qi); });
		}
		else {
			fileMenu.appendSeparator();
		}

		//TODO: rechecker
		//fileMenu.AppendMenu(MF_SEPARATOR);
		//fileMenu.AppendMenu(MF_STRING, IDC_RECHECK, CTSTRING(RECHECK_FILE));
	} else {
		fileMenu.InsertSeparatorFirst(TSTRING(FILES));
		//fileMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)segmentsMenu, CTSTRING(MAX_SEGMENTS_NUMBER));

		//if (hasBundleItems)
		//	WinUtil::appendFilePrioMenu(fileMenu, ql);

		fileMenu.AppendMenu(MF_SEPARATOR);
		fileMenu.AppendMenu(MF_STRING, IDC_REMOVE_OFFLINE, CTSTRING(REMOVE_OFFLINE));
		fileMenu.AppendMenu(MF_STRING, IDC_READD_ALL, CTSTRING(READD_ALL));
		fileMenu.appendSeparator();
	}

	if (hasBundleItems) {
		fileMenu.appendItem(TSTRING(OPEN_FOLDER), [=] { handleOpenFolder(); });
		fileMenu.appendItem(TSTRING(RUN_SFV_CHECK), [=] { handleCheckSFV(false); });
		fileMenu.appendSeparator();
	}

	fileMenu.appendItem(TSTRING(REMOVE), [=] { handleRemoveFiles(ql, false); });
	if (hasFinished)
		fileMenu.appendItem(TSTRING(REMOVE_WITH_FILES), [=] { handleRemoveFiles(ql, true); });

	if (qi && !qi->isSet(QueueItem::FLAG_USER_LIST)) {
		fileMenu.appendShellMenu({ qi->getTarget() });
	}
}

void QueueFrame::handleCheckSFV(bool treeMenu) {
	StringList paths;
	int i = -1;
	while ((i = ctrlQueue.GetNextItem(i, treeMenu ? LVNI_ALL : LVNI_SELECTED)) != -1) {
		const QueueItemInfo* qii = ctrlQueue.getItemData(i);
		if (!qii->isTempItem() && qii->isFinished() && (!treeMenu || qii->bundle))
			paths.push_back(qii->getTarget());
	}

	ShareScannerManager::getInstance()->checkSfv(paths);
}

void QueueFrame::handleOpenFile(const QueueItemPtr& aQI) {
	WinUtil::openFile(Text::toT(aQI->getTarget()));
}

void QueueFrame::handleOpenFolder() {
	ctrlQueue.filteredForEachSelectedT([&](const QueueItemInfo* qii) {
		if (!qii->isTempItem())
			WinUtil::openFolder(Text::toT(qii->getTarget()));
	});
}

void QueueFrame::handleSearchDirectory() {
	ctrlQueue.filteredForEachSelectedT([&](const QueueItemInfo* qii) {
		if (qii->bundle)
			WinUtil::searchAny(qii->bundle->isFileBundle() ? Util::getLastDir(Text::toT(qii->bundle->getTarget())) : Text::toT(qii->bundle->getName()));
	});
}

LRESULT QueueFrame::onRemoveOffline(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while ((i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const QueueItemInfo* ii = ctrlQueue.getItemData(i);

		const auto sources = QueueManager::getInstance()->getSources(ii->qi);
		for (const auto& s : sources) {
			if (!s.getUser().user->isOnline()) {
				QueueManager::getInstance()->removeFileSource(ii->qi->getTarget(), s.getUser().user, QueueItem::Source::FLAG_REMOVED);
			}
		}
	}
	return 0;
}
LRESULT QueueFrame::onReaddAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while ((i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const QueueItemInfo* ii = ctrlQueue.getItemData(i);
		if (ii->bundle)
			continue;

		// re-add all sources
		const auto badSources = QueueManager::getInstance()->getBadSources(ii->qi);
		for (const auto& bs : badSources) {
			QueueManager::getInstance()->readdQISource(ii->qi->getTarget(), bs.getUser());
		}
	}
	return 0;
}

LRESULT QueueFrame::onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	executeGuiTasks();
	updateStatus();
	bHandled = TRUE;
	return 0;
}

void QueueFrame::updateList() {
	ctrlQueue.SetRedraw(FALSE);
	ctrlQueue.DeleteAllItems();
	for (auto& pp : ctrlQueue.getParents() | map_values) {
		auto parent = pp.parent;
		if (!show(parent))
			continue;

		int pos = ctrlQueue.insertItem(ctrlQueue.getSortPos(parent), parent, parent->getImageIndex());
		updateCollapsedState(parent, pos);
	}
	ctrlQueue.SetRedraw(TRUE);
}
void QueueFrame::updateCollapsedState(QueueItemInfo* aQii, int pos) {
	if (aQii->parent == NULL && !aQii->collapsed && !aQii->bundle->isFileBundle()) {
		aQii->collapsed = true;
		ctrlQueue.Expand(aQii, pos);
	} else {
		int state = aQii->bundle && !aQii->bundle->isFileBundle() ? 1 : 0;
		ctrlQueue.SetItemState(pos, INDEXTOSTATEIMAGEMASK(state), LVIS_STATEIMAGEMASK);
	}
}

bool QueueFrame::show(const QueueItemInfo* Qii) const {
	bool isTempOrFilelist = Qii->isFilelist() || Qii->isTempItem();
	switch (curSel)
	{
	case TREE_BUNDLES:
		return !isTempOrFilelist;
	case TREE_QUEUED:
		return !Qii->isFinished() && !isTempOrFilelist;
	case TREE_FINISHED:
		return Qii->isFinished() && !isTempOrFilelist;
	case TREE_PAUSED:
		return Qii->isPaused() && !isTempOrFilelist;
	case TREE_FAILED:
		return Qii->isFailed() && !isTempOrFilelist;
	case TREE_FILELIST:
		return Qii->isFilelist();
	case TREE_TEMP:
		return Qii->isTempItem();
	case TREE_AUTOSEARCH: {
		BundlePtr b = Qii->bundle ? Qii->bundle : Qii->qi->getBundle();
		return b && b->getAddedByAutoSearch();
		break;
	}
	case TREE_LOCATION: {
		if (Qii->bundle && curItem != locationParent){
			//do it this way so we can have counts after the name
			auto i = locations.find(Qii->bundle->isFileBundle() ? Util::getFilePath(Qii->bundle->getTarget()) : Util::getParentDir(Qii->bundle->getTarget()));
			if (i != locations.end())
				return curItem == i->second.item;
		}
		return !isTempOrFilelist;
	}
	default:
		return false;
	}
}

tstring QueueFrame::handleCopyMagnet(const QueueItemInfo* aII) {
	if (aII->qi && !aII->isFilelist())
		return Text::toT(WinUtil::makeMagnet(aII->qi->getTTH(), Util::getFileName(aII->qi->getTarget()), aII->qi->getSize()));

	return Util::emptyStringT;
}

void QueueFrame::handleMoveBundles(BundleList bundles) {
	tstring targetPath = Text::toT(Util::getParentDir(bundles.front()->getTarget()));

	BrowseDlg dlg(m_hWnd, BrowseDlg::TYPE_GENERIC, BrowseDlg::DIALOG_SELECT_FOLDER);
	dlg.setPath(targetPath);
	if (!dlg.show(targetPath)) {
		return;
	}

	string newDir = Util::validatePath(Text::fromT(targetPath), true);
	if (bundles.size() == 1) {
		if (!WinUtil::showQuestionBox(TSTRING_F(CONFIRM_MOVE_DIR_BUNDLE, Text::toT(bundles.front()->getName()) % Text::toT(newDir)), MB_ICONQUESTION)) {
			return;
		}
	} else if (!WinUtil::showQuestionBox(TSTRING_F(CONFIRM_MOVE_DIR_MULTIPLE, bundles.size() % Text::toT(newDir)), MB_ICONQUESTION)) {
		return;
	}

	for (auto& sourceBundle : bundles) {
		MainFrame::getMainFrame()->addThreadedTask([=] {
			QueueManager::getInstance()->moveBundle(sourceBundle, newDir, true);
		});
	}
}

void QueueFrame::handleRemoveBundles(BundleList bundles, bool removeFinished) {
	if (bundles.empty())
		return;

	string tmp;

	bool allFinished = all_of(bundles.begin(), bundles.end(), [](const BundlePtr& b) { return b->isFinished(); });
	if (bundles.size() == 1) {
		if (removeFinished) {
			if (!WinUtil::showQuestionBox(TSTRING_F(CONFIRM_REMOVE_DIR_FINISHED, Text::toT(bundles.front()->getName())), MB_ICONQUESTION)) {
				return;
			}
		} else if (!allFinished && !WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_QUEUE_REMOVAL, TSTRING_F(CONFIRM_REMOVE_DIR_BUNDLE, Text::toT(bundles.front()->getName())))) {
			return;
		}
	} else {
		if (removeFinished) {
			if (!WinUtil::showQuestionBox(TSTRING_F(CONFIRM_REMOVE_DIR_FINISHED_MULTIPLE, bundles.size()), MB_ICONQUESTION)) {
				return;
			}
		} else if (!allFinished && !WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_QUEUE_REMOVAL, TSTRING_F(CONFIRM_REMOVE_DIR_MULTIPLE, bundles.size()))) {
			return;
		}
	}

	MainFrame::getMainFrame()->addThreadedTask([=] {
		for (auto b : bundles)
			QueueManager::getInstance()->removeBundle(b, removeFinished);
	});
}

void QueueFrame::handleRemoveFiles(QueueItemList queueitems, bool removeFinished) {
	if (queueitems.size() >= 1) {
		if (WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_QUEUE_REMOVAL, TSTRING(REALLY_REMOVE))) {
			for (auto& qi : queueitems)
				QueueManager::getInstance()->removeFile(qi->getTarget(), removeFinished);
		}
	}
}

void QueueFrame::handleSearchQI(const QueueItemPtr& aQI) {
	if (aQI)
		WinUtil::searchHash(aQI->getTTH(), Util::getFileName(aQI->getTarget()), aQI->getSize());
}

void QueueFrame::onRenameBundle(BundlePtr b) {
	LineDlg dlg;
	dlg.title = TSTRING(RENAME);
	dlg.description = TSTRING(NEW_NAME);
	dlg.line = Text::toT(b->getName());
	if (dlg.DoModal(m_hWnd) == IDOK) {
		auto newName = Util::validatePath(Text::fromT(dlg.line), false);
		if (newName == b->getName()) {
			return;
		}

		MainFrame::getMainFrame()->addThreadedTask([=] {
			QueueManager::getInstance()->renameBundle(b, newName);
		});
	}
}

void QueueFrame::onBundleAdded(const BundlePtr& aBundle) {
	auto parent = ctrlQueue.findParent(aBundle->getToken());
	if (!parent) {
		auto item = new QueueItemInfo(aBundle);
		ctrlQueue.insertGroupedItem(item, false, 0, !aBundle->isFileBundle()); // file bundles wont be having any children.
		addLocationItem(getBundleParent(aBundle));
	}
}

/*
OK, here's the deal, we insert bundles as parents and assume every bundle (except file bundles) to have sub items, thus the + expand icon.
The bundle QueueItems(its sub items) are really created and inserted only at expanding the bundle,
once its expanded we start to collect some garbage when collapsing it to avoid continuous allocations and reallocations.
Notes, Mostly there should be no reason to expand every bundle at least with a big queue,
so this way we avoid creating and updating itemInfos we wont be showing,
with a small queue its more likely for the user to expand and collapse the same items more than once.
*/
void QueueFrame::insertItems(QueueItemInfo* Qii) {
	if (Qii->childrenCreated)
		return;

	Qii->childrenCreated = true;
	BundlePtr aBundle = Qii->bundle;

	auto addItem = [&](QueueItemPtr& qi) {
		auto item = findQueueItem(qi);
		if (!item) {
			item = new QueueItemInfo(qi);
			ctrlQueue.insertGroupedItem(item, false);
		}
	};

	if (aBundle && !aBundle->isFileBundle()) {
		RLock l(QueueManager::getInstance()->getCS());
		for_each(aBundle->getQueueItems(), addItem);
		for_each(aBundle->getFinishedFiles(), addItem);
	}
}
QueueFrame::QueueItemInfo* QueueFrame::findQueueItem(const QueueItemPtr& aQI) {
	if (aQI->getBundle()){
		auto parent = ctrlQueue.findParent(aQI->getBundle()->getToken());
		if (parent) {
			auto& children = ctrlQueue.findChildren(parent->getGroupCond());
			for (auto& child : children) {
				if (child->qi == aQI)
					return child;
			}
		}
	} 
	return ctrlQueue.findParent(aQI->getTarget());
}

void QueueFrame::onBundleRemoved(const BundlePtr& aBundle, const string& aPath) {
	auto parent = ctrlQueue.findParent(aBundle->getToken());
	if (parent) {
		ctrlQueue.removeGroupedItem(parent, true); //also deletes item info
		removeLocationItem(aPath);
	}
}

void QueueFrame::onBundleUpdated(const BundlePtr& aBundle) {
	auto parent = ctrlQueue.findParent(aBundle->getToken());
	if (parent) {
		int i = ctrlQueue.findItem(parent);
		if (show(parent)) {
			if (i == -1) {
				i = ctrlQueue.insertItem(ctrlQueue.getSortPos(parent), parent, parent->getImageIndex());
				updateCollapsedState(parent, i);
			} else {
				ctrlQueue.updateItem(i);
			}
		} else {
			if (!parent->collapsed) { // we are erasing the parent, so make sure we erase the children too.
				auto& children = ctrlQueue.findChildren(parent->getGroupCond());
				for (const auto& c : children)
					ctrlQueue.deleteItem(c);
			}
			ctrlQueue.DeleteItem(i);
		}
	}
}

void QueueFrame::onQueueItemRemoved(const QueueItemPtr& aQI) {
	auto item = findQueueItem(aQI);
	if (item)
		ctrlQueue.removeGroupedItem(item, true); //also deletes item info
}

void QueueFrame::onQueueItemUpdated(const QueueItemPtr& aQI) {
	auto item = findQueueItem(aQI);
	if (item && (!item->parent || !item->parent->collapsed)) { // no need to update if its collapsed right?
		//we don't keep queueitems without bundle as finished, so no need to check filtering here.
		if (show(item)) 
			ctrlQueue.updateItem(item);
		 else
			ctrlQueue.deleteItem(item);
	}
}

void QueueFrame::onQueueItemAdded(const QueueItemPtr& aQI) {
	if (aQI->getBundle()) {
		auto parent = ctrlQueue.findParent(aQI->getBundle()->getToken());
		if (!parent || (parent->collapsed && !parent->childrenCreated))
			return;
	}
	auto item = findQueueItem(aQI);
	if (!item) {
		auto i = new QueueItemInfo(aQI);
		ctrlQueue.insertGroupedItem(i, false);
	}
}

void QueueFrame::executeGuiTasks() {
	if (tasks.getTasks().empty())
		return;
	ctrlQueue.SetRedraw(FALSE);
	for (;;) {
		TaskQueue::TaskPair t;
		if (!tasks.getFront(t))
			break;

		static_cast<AsyncTask*>(t.second)->f();
		tasks.pop_front();
	}
	statusDirty = true;
	ctrlQueue.SetRedraw(TRUE);
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

		auto qm = QueueManager::getInstance();
		{
			RLock l(qm->getCS());
			for (auto& b : qm->getBundles() | map_values){
				b->isFinished() ? finishedBundles++ : queuedBundles++;
				if (b->isFailed()) failedBundles++;
				if (b->isPausedPrio()) pausedItems++;
				if (b->getAddedByAutoSearch()) autosearchAdded++;
			}

			for (const auto& q : qm->getFileQueue() | map_values) {
				totalItems++;
				if (!q->getBundle()){
					queuedItems++;
					if (q->isSet(QueueItem::FLAG_USER_LIST))
						filelistItems++;
				}
			}

		}
		ctrlTree.SetRedraw(FALSE);
		HTREEITEM ht = bundleParent;
		while (ht != NULL) {
			switch (ctrlTree.GetItemData(ht)) {
			case TREE_BUNDLES:
				ctrlTree.SetItemText(ht, (TSTRING(BUNDLES) + _T(" ( ") + Util::toStringW(finishedBundles + queuedBundles) + _T(" )")).c_str());
				ht = ctrlTree.GetChildItem(bundleParent);
				break;
			case TREE_FINISHED:
				ctrlTree.SetItemText(ht, (TSTRING(FINISHED) + _T(" ( ") + (Util::toStringW(finishedBundles)) + _T(" )")).c_str());
				ht = ctrlTree.GetNextSiblingItem(ht);
				break;
			case TREE_QUEUED:
				ctrlTree.SetItemText(ht, (TSTRING(QUEUED) + _T(" ( ") + (Util::toStringW(queuedBundles)) + _T(" )")).c_str());
				ht = ctrlTree.GetNextSiblingItem(ht);
				break;
			case TREE_FAILED:
				ctrlTree.SetItemText(ht, (TSTRING(FAILED) + _T(" ( ") + (Util::toStringW(failedBundles)) + _T(" )")).c_str());
				ht = ctrlTree.GetNextSiblingItem(ht);
				break;
			case TREE_PAUSED:
				ctrlTree.SetItemText(ht, (TSTRING(PAUSED) + _T(" ( ") + (Util::toStringW(pausedItems)) + _T(" )")).c_str());
				ht = ctrlTree.GetNextSiblingItem(ht);
				break;
			case TREE_AUTOSEARCH:
				ctrlTree.SetItemText(ht, (TSTRING(AUTO_SEARCH) + _T(" ( ") + (Util::toStringW(autosearchAdded)) + _T(" )")).c_str());
				ht = ctrlTree.GetNextSiblingItem(ht);
				break;
			case TREE_LOCATION:
				ctrlTree.SetItemText(ht, (TSTRING(LOCATIONS) + _T(" ( ") + (Util::toStringW(finishedBundles + queuedBundles)) + _T(" )")).c_str());
				ht = ctrlTree.GetNextSiblingItem(bundleParent); //End of bundle sub items, get next parent item.
				break;
			case TREE_FILELIST:
				ctrlTree.SetItemText(ht, (TSTRING(FILE_LISTS) + _T(" ( ") + (Util::toStringW(filelistItems)) + _T(" )")).c_str());
				ht = ctrlTree.GetNextSiblingItem(ht);
				break;
			case TREE_TEMP:
				ctrlTree.SetItemText(ht, (TSTRING(TEMP_ITEMS) + _T(" ( ") + (Util::toStringW(queuedItems - filelistItems)) + _T(" )")).c_str());
				ht = ctrlTree.GetNextSiblingItem(ht);
				break;

			default: break;
			}

		}
		ctrlTree.SetRedraw(TRUE);
		
		if (ctrlStatus.IsWindow()) {

			tstring tmp = TSTRING(FINISHED_BUNDLES) + _T(": ") + Util::toStringW(finishedBundles);
			int w = WinUtil::getTextWidth(tmp, ctrlStatus.m_hWnd);
			if (statusSizes[1] < w) {
				statusSizes[1] = w;
				u = true;
			}
			ctrlStatus.SetText(2, (tmp).c_str());

			tmp = TSTRING(QUEUED_BUNDLES) + _T(": ") + Util::toStringW(queuedBundles);
			w = WinUtil::getTextWidth(tmp, ctrlStatus.m_hWnd);
			if (statusSizes[2] < w) {
				statusSizes[2] = w;
				u = true;
			}
			ctrlStatus.SetText(3, (tmp).c_str());

			tmp = TSTRING_F(TOTAL_FILES, totalItems);
			w = WinUtil::getTextWidth(tmp, ctrlStatus.m_hWnd);
			if (statusSizes[3] < w) {
				statusSizes[3] = w;
				u = true;
			}
			ctrlStatus.SetText(4, (tmp).c_str());

			tmp = TSTRING(QUEUE_SIZE) + _T(": ") + Util::formatBytesW(qm->getTotalQueueSize());
			w = WinUtil::getTextWidth(tmp, ctrlStatus.m_hWnd);
			if (statusSizes[4] < w) {
				statusSizes[4] = w;
				u = true;
			}
			ctrlStatus.SetText(5, (tmp).c_str());

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
	return aBundle->isFileBundle() ? Util::getFilePath(aBundle->getTarget()) : Util::getParentDir(aBundle->getTarget());
}

void QueueFrame::on(QueueManagerListener::BundleAdded, const BundlePtr& aBundle) noexcept {
	addGuiTask(TASK_ADD, [=] { onBundleAdded(aBundle); });
}
void QueueFrame::on(QueueManagerListener::BundleRemoved, const BundlePtr& aBundle) noexcept{
	addGuiTask(TASK_REMOVE, [=] { onBundleRemoved(aBundle, getBundleParent(aBundle)); });
}
void QueueFrame::on(QueueManagerListener::BundleMoved, const BundlePtr& aBundle) noexcept {
	string path = getBundleParent(aBundle);
	addGuiTask(TASK_REMOVE, [=] { onBundleRemoved(aBundle, path); });
}
void QueueFrame::on(QueueManagerListener::BundleMerged, const BundlePtr& aBundle, const string&) noexcept { 
	addGuiTask(TASK_UPDATE, [=] { onBundleUpdated(aBundle); });
}
void QueueFrame::on(QueueManagerListener::BundleSize, const BundlePtr& aBundle) noexcept { 
	addGuiTask(TASK_UPDATE, [=] { onBundleUpdated(aBundle); });
}
void QueueFrame::on(QueueManagerListener::BundlePriority, const BundlePtr& aBundle) noexcept { 
	addGuiTask(TASK_UPDATE, [=] { onBundleUpdated(aBundle); });
}
void QueueFrame::on(QueueManagerListener::BundleStatusChanged, const BundlePtr& aBundle) noexcept { 
	addGuiTask(TASK_UPDATE, [=] { onBundleUpdated(aBundle); });
}
void QueueFrame::on(QueueManagerListener::BundleSources, const BundlePtr& aBundle) noexcept { 
	addGuiTask(TASK_UPDATE, [=] { onBundleUpdated(aBundle); });
}

void QueueFrame::on(QueueManagerListener::Removed, const QueueItemPtr& aQI, bool /*finished*/) noexcept{
	addGuiTask(TASK_REMOVE, [=] { onQueueItemRemoved(aQI); });
}
void QueueFrame::on(QueueManagerListener::Added, QueueItemPtr& aQI) noexcept{
	addGuiTask(TASK_ADD, [=] { onQueueItemAdded(aQI); });
}
void QueueFrame::on(QueueManagerListener::SourcesUpdated, const QueueItemPtr& aQI) noexcept {
	addGuiTask(TASK_UPDATE, [=] { onQueueItemUpdated(aQI); });
}
void QueueFrame::on(QueueManagerListener::StatusUpdated, const QueueItemPtr& aQI) noexcept{
	addGuiTask(TASK_UPDATE, [=] { onQueueItemUpdated(aQI); });
}

void QueueFrame::on(DownloadManagerListener::BundleTick, const BundleList& tickBundles, uint64_t /*aTick*/) noexcept{
	for (auto& b : tickBundles) {
		addGuiTask(TASK_UPDATE, [=] { onBundleUpdated(b); });
	}
}

/*QueueItemInfo functions*/

int QueueFrame::QueueItemInfo::getImageIndex() const {
	if (bundle)
		return bundle->isFileBundle() ? ResourceLoader::getIconIndex(Text::toT(bundle->getTarget())) : ResourceLoader::DIR_NORMAL;
	else 
		return ResourceLoader::getIconIndex(Text::toT(qi->getTarget()));
}

const tstring QueueFrame::QueueItemInfo::getText(int col) const {

	switch (col) {
		case COLUMN_NAME: return getName();
		case COLUMN_SIZE: return (getSize() != -1) ? Util::formatBytesW(getSize()) : TSTRING(UNKNOWN);
		case COLUMN_TYPE: return getType();
		case COLUMN_PRIORITY:
		{
			if (isFinished() || getPriority() == -1)
				return Util::emptyStringT;
			bool autoPrio = (bundle && bundle->getAutoPriority()) || (qi && qi->getAutoPriority());
			return Text::toT(AirUtil::getPrioText(getPriority())) + (autoPrio ? _T(" (") + TSTRING(AUTO) + _T(")") : Util::emptyStringT);
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
		case COLUMN_SOURCES: return !isFinished() ? getSourceString() : Util::emptyStringT;
		case COLUMN_DOWNLOADED: return Util::formatBytesW(getDownloadedBytes());
		case COLUMN_TIME_ADDED: return getTimeAdded() > 0 ? Text::toT(Util::formatTime("%Y-%m-%d %H:%M", getTimeAdded())) : Util::emptyStringT;
		case COLUMN_TIME_FINISHED: return isFinished() ? Text::toT(Util::formatTime("%Y-%m-%d %H:%M", getTimeFinished())) : Util::emptyStringT;
		case COLUMN_PATH: return Text::toT(getTarget());
		default: return Util::emptyStringT;
	}
}

tstring QueueFrame::QueueItemInfo::getType() const {
	if (bundle) {
		if (bundle->isFileBundle()) {
			return WinUtil::formatFileType(bundle->getTarget());
		} else {
			RLock l(QueueManager::getInstance()->getCS());
			return WinUtil::formatFolderContent(bundle->getQueueItems().size() + bundle->getFinishedFiles().size(), bundle->getBundleDirs().size() - 1);
		}
	} else if (isFilelist()) {
		return TSTRING(FILE_LIST);
	} else {
		return WinUtil::formatFileType(qi->getTarget());
	}
}

tstring QueueFrame::QueueItemInfo::getName() const {
	if (bundle) {
		return Text::toT(bundle->getName());
	} else if (qi) {
		//show files in subdirectories as subdir/file.ext
		string path = qi->getTarget();
		if (qi->getBundle())
			return Text::toT(path.substr(qi->getBundle()->getTarget().size(), qi->getTarget().size()));
		else
			return Text::toT(qi->getTargetFileName());
	}
	return Util::emptyStringT;
}

const string& QueueFrame::QueueItemInfo::getTarget() const {
	return bundle ? bundle->getTarget() : qi->getTarget();
}

int64_t QueueFrame::QueueItemInfo::getSize() const {
	return bundle ? bundle->getSize() : qi ? qi->getSize() : -1;
}

int64_t QueueFrame::QueueItemInfo::getSpeed() const {
	return bundle ? bundle->getSpeed() : qi ? QueueManager::getInstance()->getAverageSpeed(qi) : 0;
}

uint64_t QueueFrame::QueueItemInfo::getSecondsLeft() const {
	return bundle ? bundle->getSecondsLeft() : qi ? QueueManager::getInstance()->getSecondsLeft(qi) : 0;
}

time_t QueueFrame::QueueItemInfo::getTimeAdded() const {
	return bundle ? bundle->getAdded() : qi ? qi->getAdded() : 0;
}

time_t QueueFrame::QueueItemInfo::getTimeFinished() const {
	return bundle ? bundle->getBundleFinished() : qi ? qi->getFileFinished() : 0;
}

int QueueFrame::QueueItemInfo::getPriority() const {
	return  bundle ? bundle->getPriority() : qi ? qi->getPriority() : -1;
}
 
bool QueueFrame::QueueItemInfo::isFinished() const {
	return bundle ? bundle->isFinished() : QueueManager::getInstance()->isFinished(qi);
}

bool QueueFrame::QueueItemInfo::isPaused() const {
	return bundle ? bundle->isPausedPrio() : qi->getBundle() ? qi->getBundle()->isPausedPrio() : qi->isPausedPrio();
}

bool QueueFrame::QueueItemInfo::isTempItem() const {
	if (bundle)
		return false;
	return !qi->getBundle();
}

bool QueueFrame::QueueItemInfo::isFilelist() const {
	if (bundle)
		return false;
	return qi->isSet(QueueItem::FLAG_USER_LIST);
}

bool QueueFrame::QueueItemInfo::isFailed() const {
	return bundle ? bundle->isFailed() : qi->getBundle() ? qi->getBundle()->isFailed() : false;
}


double QueueFrame::QueueItemInfo::getPercentage() const {
	return getSize() > 0 ? (double) getDownloadedBytes()*100.0 / (double) getSize() : 0;
}

tstring QueueFrame::QueueItemInfo::getStatusString() const {
	//Yeah, think about these a little more...
	if (bundle) {
		if (bundle->isPausedPrio()) 
			return TSTRING_F(PAUSED_PCT, getPercentage());
	
		switch (bundle->getStatus()) {
		case Bundle::STATUS_NEW:
		case Bundle::STATUS_QUEUED: {
			if (bundle->getSpeed() > 0) { // Bundle->isRunning() ?
				return TSTRING_F(RUNNING_PCT, getPercentage());
			} else {
				return TSTRING_F(WAITING_PCT, getPercentage());
			}
		}
		case Bundle::STATUS_DOWNLOADED: return TSTRING(MOVING);
		case Bundle::STATUS_MOVED: return TSTRING(DOWNLOADED);
		case Bundle::STATUS_FAILED_MISSING:
		case Bundle::STATUS_SHARING_FAILED: return Text::toT(bundle->getLastError());
		case Bundle::STATUS_FINISHED: return TSTRING(FINISHED);
		case Bundle::STATUS_HASHING: return TSTRING(HASHING);
		case Bundle::STATUS_HASH_FAILED: return TSTRING(HASH_FAILED);
		case Bundle::STATUS_HASHED: return TSTRING(HASHING_FINISHED);
		case Bundle::STATUS_SHARED: return TSTRING(SHARED);
		default:
			return Util::emptyStringT;
		}
	} else if(qi) {
		if (qi->isPausedPrio()) 
			return TSTRING_F(PAUSED_PCT, getPercentage());
		if (isFinished())
			return qi->isSet(QueueItem::FLAG_MOVED) ? TSTRING(FINISHED) : TSTRING(MOVING);
		if (QueueManager::getInstance()->isWaiting(qi)) {
			return TSTRING_F(WAITING_PCT, getPercentage());
		} else {
			return TSTRING_F(RUNNING_PCT, getPercentage());
		} 
	}
	return Util::emptyStringT;
}
tstring QueueFrame::QueueItemInfo::getSourceString() const {
	auto size = 0;
	int online = 0;
	if (bundle) {
		Bundle::SourceList sources = QueueManager::getInstance()->getBundleSources(bundle);
		size = sources.size();
		for (const auto& s : sources) {
			if (s.getUser().user->isOnline())
				online++;
		}
	} else {
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
	return bundle ? bundle->getDownloadedBytes() : qi ? QueueManager::getInstance()->getDownloadedBytes(qi) : 0;
}

int QueueFrame::QueueItemInfo::compareItems(const QueueItemInfo* a, const QueueItemInfo* b, int col) {
	// TODO: lots of things to fix

	switch (col) {
	case COLUMN_NAME: {
		if (a->bundle && b->bundle) {
			if (a->bundle->isFileBundle() && !b->bundle->isFileBundle()) return 1;
			if (!a->bundle->isFileBundle() && b->bundle->isFileBundle()) return -1;
		}

		auto textA = a->getName();
		auto textB = b->getName();
		if (!a->bundle) {
			auto hasSubA = textA.find(PATH_SEPARATOR) != tstring::npos;
			auto hasSubB = textB.find(PATH_SEPARATOR) != tstring::npos;
			if (hasSubA && !hasSubB) return -1;
			if (!hasSubA && hasSubB) return 1;
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
				auto dirsA = a->bundle->getBundleDirs().size() - 1;
				auto dirsB = a->bundle->getBundleDirs().size() - 1;
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
	case Bundle::STATUS_NEW: return SETTING(DOWNLOAD_BAR_COLOR);
	case Bundle::STATUS_QUEUED: return SETTING(DOWNLOAD_BAR_COLOR);
	case Bundle::STATUS_DOWNLOADED: return SETTING(DOWNLOAD_BAR_COLOR);;
	case Bundle::STATUS_MOVED: return SETTING(DOWNLOAD_BAR_COLOR);
	case Bundle::STATUS_FINISHED: return SETTING(COLOR_STATUS_FINISHED);
	case Bundle::STATUS_HASHED: return SETTING(COLOR_STATUS_FINISHED);
	case Bundle::STATUS_SHARED: return SETTING(COLOR_STATUS_SHARED);
	default:
		return SETTING(DOWNLOAD_BAR_COLOR);
	}
}