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

#include <windows/frames/rss/RSSinfoFrame.h>
#include <windows/MainFrm.h>
#include <windows/util/WinUtil.h>
#include <windows/util/ActionUtil.h>
#include <windows/ResourceLoader.h>

#include <airdcpp/modules/AutoSearchManager.h>

#include <airdcpp/util/DupeUtil.h>
#include <airdcpp/core/io/File.h>
#include <airdcpp/events/LogManager.h>

#include <boost/algorithm/cxx11/all_of.hpp>


namespace wingui {
string RssInfoFrame::id = "RSS";

int RssInfoFrame::columnIndexes[] = { COLUMN_FILE, COLUMN_LINK, COLUMN_DATE, COLUMN_NAME };
int RssInfoFrame::columnSizes[] = { 800, 350, 150, 150};
static ResourceManager::Strings columnNames[] = { ResourceManager::TITLE, ResourceManager::LINK, ResourceManager::DATE, ResourceManager::FEED_NAME };
static SettingsManager::BoolSetting filterSettings[] = { SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST };
static ColumnType columnTypes[] = { COLUMN_TEXT, COLUMN_TEXT, COLUMN_TEXT, COLUMN_TEXT };

RssInfoFrame::RssInfoFrame() : 
	ctrlRss(this, COLUMN_LAST, [this] { callAsync([this] { reloadList(); }); }, filterSettings, COLUMN_LAST),
	ctrlStatusContainer(WC_BUTTON, this, RSS_STATUS_MSG_MAP)
{ }

LRESULT RssInfoFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	ctrlStatusContainer.SubclassWindow(ctrlStatus.m_hWnd);

	ctrlRss.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, NULL);
	ctrlRss.list.SetBkColor(WinUtil::bgColor);
	ctrlRss.list.SetTextBkColor(WinUtil::bgColor);
	ctrlRss.list.SetTextColor(WinUtil::textColor);
	ctrlRss.list.setFlickerFree(WinUtil::bgBrush);
	ctrlRss.list.SetFont(WinUtil::listViewFont);

	ctrlTree.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
		TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP | TVS_TRACKSELECT,
		WS_EX_CLIENTEDGE, IDC_RSS_TREE);
	ctrlTree.SetExtendedStyle(TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);

	if (SETTING(USE_EXPLORER_THEME)) {
		SetWindowTheme(ctrlTree.m_hWnd, L"explorer", NULL);
	}
	ctrlTree.SetBkColor(WinUtil::bgColor);
	ctrlTree.SetTextColor(WinUtil::textColor);
	ctrlTree.SetFont(WinUtil::listViewFont);

	//create buttons
	ctrlAdd.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON, 0, IDC_ADD);
	ctrlAdd.SetWindowText(CTSTRING(ADD));
	ctrlAdd.SetFont(WinUtil::systemFont);

	ctrlRemove.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON, 0, IDC_REMOVE);
	ctrlRemove.SetWindowText(CTSTRING(REMOVE));
	ctrlRemove.SetFont(WinUtil::systemFont);

	ctrlChange.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON, 0, IDC_CHANGE);
	ctrlChange.SetWindowText(CTSTRING(SETTINGS_CHANGE));
	ctrlChange.SetFont(WinUtil::systemFont);


	treeImages.Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 3);
	treeImages.AddIcon(CIcon(ResourceLoader::loadIcon(IDI_RSS, 16)));
	treeImages.AddIcon(CIcon(ResourceLoader::convertGrayscaleIcon(ResourceLoader::loadIcon(IDI_RSS, 16))));
	ctrlTree.SetImageList(treeImages);

	listImages.Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 3);
	listImages.AddIcon(CIcon(ResourceLoader::loadIcon(IDI_TEXT, 16)));
	listImages.AddIcon(CIcon(ResourceLoader::getFileImages().GetIcon(ResourceLoader::DIR_NORMAL)));
	ctrlRss.list.SetImageList(listImages, LVSIL_SMALL);

	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	SetSplitterPanes(ctrlTree.m_hWnd, ctrlRss.m_hWnd);
	SetSplitterPosPct(15);

	treeParent = addTreeItem(TVI_ROOT, 0, TSTRING(RSS_FEEDS));
	ctrlTree.SelectItem(treeParent);
	{
		Lock l(RSSManager::getInstance()->getCS());
		auto lst = RSSManager::getInstance()->getRss();
		for (auto feed : lst) {
			addFeed(feed);
			for (auto data : feed->getFeedData() | views::values) {
				itemInfos.emplace_back(make_unique<ItemInfo>(data));
			}
		}
	}
	ctrlTree.Expand(treeParent);

	callAsync([=] { reloadList(); });

	RSSManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);
	AutoSearchManager::getInstance()->addListener(this);
	
	memzero(statusSizes, sizeof(statusSizes));
	statusSizes[0] = 16;
	ctrlStatus.SetParts(1, statusSizes);

	::SetTimer(m_hWnd, 0, 500, 0);

	WinUtil::SetIcon(m_hWnd, IDI_RSS);
	bHandled = FALSE;
	return 1;
}

LRESULT RssInfoFrame::onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if (tasks.getTasks().empty())
		return 0;
	ctrlRss.list.SetRedraw(FALSE);
	for (;;) {
		TaskQueue::TaskPair t;
		if (!tasks.getFront(t))
			break;

		static_cast<AsyncTask*>(t.second)->f();
		tasks.pop_front();
	}
	ctrlRss.list.SetRedraw(TRUE);
	bHandled = TRUE;
	return 0;
}

LRESULT RssInfoFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if (!closed) {
		::KillTimer(m_hWnd, 0);
		AutoSearchManager::getInstance()->removeListener(this);
		QueueManager::getInstance()->removeListener(this);
		RSSManager::getInstance()->removeListener(this);
		WinUtil::setButtonPressed(IDC_RSSFRAME, false);
		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	}

	ctrlRss.list.saveHeaderOrder(SettingsManager::RSSFRAME_ORDER,
		SettingsManager::RSSFRAME_WIDTHS, SettingsManager::RSSFRAME_VISIBLE);

	ctrlRss.list.SetRedraw(FALSE);
	ctrlRss.list.DeleteAllItems();
	ctrlRss.list.SetRedraw(TRUE);

	itemInfos.clear();
	bHandled = FALSE;
	return 0;
}

LRESULT RssInfoFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	bool listMenu = false;
	bool treeMenu = false;
	bool hitTreeitem = false;
	if (reinterpret_cast<HWND>(wParam) == ctrlRss && ctrlRss.list.GetSelectedCount() > 0) {
		if (pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlRss.list, pt);
		}
		listMenu = true;
	}
	else if (reinterpret_cast<HWND>(wParam) == ctrlTree) {
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
			hitTreeitem = true;
		}
		ctrlTree.ClientToScreen(&pt);
		treeMenu = true;
	}

	if (listMenu) {
		auto items = getSelectedListitems();
		if (!items.empty()) {
			OMenu menu;
			menu.CreatePopupMenu();
			string title = items.size() == 1 ? (items.front()->item->getTitle() + PATH_SEPARATOR) : STRING_F(ITEMS_X, items.size());

			menu.InsertSeparatorFirst(Text::toT(title));

			menu.appendItem(TSTRING(OPEN_LINK), [=] { ranges::for_each(items, [=](const ItemInfo* a) { ActionUtil::openLink(Text::toT(a->item->getLink())); }); }, OMenu::FLAG_DEFAULT);
			menu.appendSeparator();

			ListBaseType::MenuItemList customItems;
			ctrlRss.list.appendCopyMenu(menu, customItems);
			menu.appendSeparator();

			bool allRelease = ranges::all_of(items, [=](const ItemInfo* a) { return a->isRelease; });
			if (allRelease) {
				//auto search menus
				appendDownloadMenu(menu, DownloadBaseHandler::TYPE_SECONDARY, true, nullopt, nullopt, false);
				menu.appendSeparator();
			}

			if (items.size() == 1) {
				menu.appendItem(TSTRING(SEARCH), [=] { ActionUtil::search(Text::toT(items.front()->item->getTitle())); });
				if (DupeUtil::allowOpenDirectoryDupe(items.front()->getDupe())) {
					menu.appendSeparator();
					menu.appendItem(TSTRING(OPEN_FOLDER), [=] {
						auto paths = DupeUtil::getAdcDirectoryDupePaths(items.front()->getDupe(), items.front()->item->getTitle());
						if (!paths.empty())
							ActionUtil::openFolder(Text::toT(paths.front()));
					});
				}
				menu.appendSeparator();
			}
			menu.appendItem(TSTRING(REMOVE), [=] {
				for_each(items.begin(), items.end(), [=](const ItemInfo* a) {
					RSSManager::getInstance()->removeFeedData(a->item->getFeed(), a->item); });
			});
			menu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
			return TRUE;
		}
	}
	else if (treeMenu) {

		OMenu menu;
		menu.CreatePopupMenu();
		auto feed = getSelectedFeed();
		if (feed && hitTreeitem) {
			menu.InsertSeparatorFirst(Text::toT(feed->getFeedName()));

			menu.appendItem(TSTRING(UPDATE), [=] { RSSManager::getInstance()->downloadFeed(feed, true); }, OMenu::FLAG_THREADED);
			menu.appendItem(feed->getEnable() ? TSTRING(DISABLE_RSS) : TSTRING(ENABLE_RSS), [=] { RSSManager::getInstance()->enableFeedUpdate(feed, !feed->getEnable()); }, OMenu::FLAG_THREADED);
			menu.appendItem(TSTRING(MATCH_FILTERS), [=] { RSSManager::getInstance()->matchFilters(feed);  }, OMenu::FLAG_THREADED);
			menu.appendItem(TSTRING(CLEAR), [=] { RSSManager::getInstance()->clearRSSData(feed);  }, OMenu::FLAG_THREADED);
			menu.appendSeparator();
			menu.appendItem(TSTRING(ADD), [=] { add(); });
			menu.appendItem(TSTRING(SETTINGS_CHANGE), [=] { change(); });
			menu.appendItem(TSTRING(REMOVE), [=] { remove(); });
			menu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
			return TRUE;
		}
		else if (hitTreeitem && treeParent == ctrlTree.GetSelectedItem()) {
			bool allEnabled = ranges::all_of(feeds | views::keys, [&](const RSSPtr& r) { return r->getEnable(); });
			bool allDisabled = ranges::all_of(feeds | views::keys, [&](const RSSPtr& r) { return !r->getEnable(); });
			menu.InsertSeparatorFirst(WinUtil::toStringW(feeds.size()) + _T(" ") + TSTRING(FEEDS));
			//menu.appendItem(TSTRING(UPDATE), [=] { for_each(feeds | views::keys, [&](const RSSPtr& feed) { RSSManager::getInstance()->downloadFeed(feed, true); }); }, OMenu::FLAG_THREADED);
			if(!allEnabled)
				menu.appendItem(TSTRING(ENABLE_RSS), [=] { ranges::for_each(feeds | views::keys, [&](const RSSPtr& feed) { RSSManager::getInstance()->enableFeedUpdate(feed, true); }); }, OMenu::FLAG_THREADED);
			if(!allDisabled)
				menu.appendItem(TSTRING(DISABLE_RSS), [=] { ranges::for_each(feeds | views::keys, [&](const RSSPtr& feed) { RSSManager::getInstance()->enableFeedUpdate(feed, false); }); }, OMenu::FLAG_THREADED);
			menu.appendItem(TSTRING(MATCH_FILTERS), [=] { ranges::for_each(feeds | views::keys, [&](const RSSPtr& feed) { RSSManager::getInstance()->matchFilters(feed); }); }, OMenu::FLAG_THREADED);
			menu.appendItem(TSTRING(CLEAR), [=] { ranges::for_each(feeds | views::keys, [&](const RSSPtr& feed) { RSSManager::getInstance()->clearRSSData(feed); }); }, OMenu::FLAG_THREADED);
			menu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
			return TRUE;
		} else {
			menu.appendItem(TSTRING(ADD), [=] { add(); });
			menu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
			return TRUE;
		}
	}

	bHandled = FALSE;
	return FALSE;
}

LRESULT RssInfoFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;

	switch (cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT: {
		auto ii = (ItemInfo*)cd->nmcd.lItemlParam;

		if (ii->getDupe() == DupeType::DUPE_NONE && ii->isAutosearchDupe) {
			cd->clrText = WinUtil::blendColors(cd->clrText, cd->clrTextBk);
		} else {
			auto c = WinUtil::getDupeColors(ii->getDupe());
			cd->clrText = c.first;
			cd->clrTextBk = c.second;
		}

		return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
	}
	default:
		return CDRF_DODEFAULT;
	}
}

LRESULT RssInfoFrame::onConfig(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	switch (wID) {
	case IDC_ADD:
	{
		add();
		break;
	}
	case IDC_CHANGE :
	{
		change();
		break;
	}
	case IDC_REMOVE: {
		remove();
		break;
	}
	default:
		break;
	} 
	return 0;
}

LRESULT RssInfoFrame::onSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /* bHandled */) {
	NMTREEVIEW* nmtv = (NMTREEVIEW*)pnmh;
	if (nmtv->itemNew.lParam != -1) {
		curSel = nmtv->itemNew.lParam;
		curItem = nmtv->itemNew.hItem;
		if (getSelectedFeed()) {
			ctrlChange.EnableWindow(TRUE);
			ctrlRemove.EnableWindow(TRUE);
		}
		else {
			ctrlChange.EnableWindow(FALSE);
			ctrlRemove.EnableWindow(FALSE);
		}
		reloadList();
	}

	return 0;
}

LRESULT RssInfoFrame::onTreeItemClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /* bHandled */) {
	DWORD dwPos = ::GetMessagePos();
	POINT pt;
	pt.x = GET_X_LPARAM(dwPos);
	pt.y = GET_Y_LPARAM(dwPos);

	ctrlTree.ScreenToClient(&pt);

	UINT uFlags;
	HTREEITEM ht = ctrlTree.HitTest(pt, &uFlags);
	if (ht && ht == curItem)
		reloadList();

	return 0;
}


LRESULT RssInfoFrame::onDoubleClickList(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
	auto list = getSelectedListitems();
	if (!list.empty()) {
		ActionUtil::openLink(Text::toT(list.front()->item->getLink()));
	}
	return 0;
}

void RssInfoFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */)
{
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	if (ctrlStatus.IsWindow()) {
		CRect sr;
		int w[2];
		ctrlStatus.GetClientRect(sr);

		w[1] = sr.right - 16;
		w[0] = 16;

		ctrlStatus.SetParts(2, w);

		CRect rc;
		ctrlStatus.GetRect(0, &rc);

		const int button_width = 80;

		rc.left += 2;
		rc.right = rc.left + button_width;
		ctrlAdd.MoveWindow(rc);

		rc.OffsetRect(button_width + 2, 0);
		ctrlChange.MoveWindow(rc);

		rc.OffsetRect(button_width + 2, 0);
		ctrlRemove.MoveWindow(rc);
	}

	SetSplitterRect(&rect);

	
}

void RssInfoFrame::on(RSSDataAdded, const RSSDataPtr& aData) noexcept {
	addGuiTask([=] { onItemAdded(aData); } );

}

void RssInfoFrame::on(RSSDataRemoved, const RSSDataPtr& aData) noexcept {
	addGuiTask([=] { onItemRemoved(aData); });

}

void RssInfoFrame::on(RSSDataCleared, const RSSPtr& aFeed) noexcept {
	addGuiTask([=] { clearData(aFeed); });

}

void RssInfoFrame::on(RSSFeedRemoved, const RSSPtr& aFeed) noexcept {
	addGuiTask([=] {
		ctrlTree.SetRedraw(FALSE);
		clearData(aFeed);
		auto j = feeds.find(aFeed);
		auto ht = j->second;
		feeds.erase(j);
		ctrlTree.DeleteItem(ht);
		ctrlTree.SetRedraw(TRUE);
	});
}

void RssInfoFrame::on(RSSFeedAdded, const RSSPtr& aFeed) noexcept {
	addGuiTask([=] {
		addFeed(aFeed);
	});
}

void RssInfoFrame::on(RSSFeedChanged, const RSSPtr& aFeed) noexcept {
	addGuiTask([=] {

		ctrlTree.SetRedraw(FALSE);
		auto j = feeds.find(aFeed);
		auto ht = j->second;
		ctrlTree.SetItemText(ht, Text::toT(aFeed->getFeedName()).c_str());
		int img = aFeed->getEnable() ? 0 : 1;
		ctrlTree.SetItemImage(ht, img, img);

		ctrlTree.SetRedraw(TRUE);
		reloadList();
	});
}

void RssInfoFrame::on(QueueManagerListener::BundleStatusChanged, const BundlePtr& aBundle) noexcept {
	if ((aBundle->getStatus() == Bundle::STATUS_QUEUED) || (aBundle->getStatus() == Bundle::STATUS_SHARED)) {
		addGuiTask([=] { updateDupeType(aBundle->getName()); });
	}
}

void RssInfoFrame::on(QueueManagerListener::BundleAdded, const BundlePtr& aBundle) noexcept {
	addGuiTask([=] { updateDupeType(aBundle->getName()); });
}

void RssInfoFrame::on(QueueManagerListener::BundleRemoved, const BundlePtr& aBundle) noexcept {
	if ((aBundle->getStatus() < Bundle::STATUS_SHARED)) {
		addGuiTask([=] { updateDupeType(aBundle->getName()); });
	}
}

void RssInfoFrame::on(AutoSearchManagerListener::ItemAdded, const AutoSearchPtr& as) noexcept {
	addGuiTask([=] { updateDupeType(as->getSearchString()); });
}
void RssInfoFrame::on(AutoSearchManagerListener::ItemRemoved, const AutoSearchPtr& as) noexcept {
	addGuiTask([=] { updateDupeType(as->getSearchString()); });
}

void RssInfoFrame::updateDupeType(const string& aName) {
	for (auto& i : itemInfos) {
		if (i->isRelease && i->item->getTitle() == aName) {
			i->setDupe(DupeUtil::checkAdcDirectoryDupe(i->item->getTitle(), 0));

			if (i->getDupe() == DupeType::DUPE_NONE)
				i->isAutosearchDupe = AutoSearchManager::getInstance()->getSearchesByString(i->item->getTitle()) != AutoSearchList();

			ctrlRss.list.updateItem(i.get());
		}
	}
}

void RssInfoFrame::handleDownload(const string& aTarget, Priority /*p*/, bool /*aIsRelease*/) {
	auto items = getSelectedListitems();
	if (items.empty())
		return;
	for_each(items.begin(), items.end(), [=](const ItemInfo* ii) {
		AutoSearchManager::getInstance()->addAutoSearch(ii->item->getTitle(), aTarget, true, AutoSearch::RSS_DOWNLOAD);
	});
}

void RssInfoFrame::add() {
	auto feed = std::make_shared<RSS>();
	openDialog(feed);
}

void RssInfoFrame::change() {
	auto feed = getSelectedFeed();
	openDialog(feed);
}

void RssInfoFrame::remove() {
	RSSManager::getInstance()->removeFeedItem(getSelectedFeed());
	MainFrame::getMainFrame()->addThreadedTask([=] {
		RSSManager::getInstance()->save();
	});
}
void RssInfoFrame::openDialog(RSSPtr& aFeed) {
	TabbedDialog dlg(STRING(RSS_CONFIG));
	dlg.addPage<RssFeedsPage>(shared_ptr<RssFeedsPage>(new RssFeedsPage(STRING(SETTINGS_GENERAL), aFeed)));
	dlg.addPage<RssFilterPage>(shared_ptr<RssFilterPage>(new RssFilterPage(STRING(FILTER), aFeed)));
	if (dlg.DoModal() == IDOK) {
		MainFrame::getMainFrame()->addThreadedTask([=] {
			RSSManager::getInstance()->save();
		});
	}
}

void RssInfoFrame::clearData(const RSSPtr& aFeed) {
	ctrlRss.list.SetRedraw(FALSE);
	itemInfos.erase(ranges::remove_if(itemInfos, [&](const unique_ptr<ItemInfo>& a) {

		if (aFeed == a->item->getFeed()) {
			ctrlRss.list.deleteItem(a.get());
			return true;
		}
		return false;

	}).begin(), itemInfos.end());

	ctrlRss.list.SetRedraw(TRUE);
}

void RssInfoFrame::onItemRemoved(const RSSDataPtr& aData) {
	ctrlRss.list.SetRedraw(FALSE);
	auto [first, last] = ranges::remove_if(itemInfos, [&](const unique_ptr<ItemInfo>& a) {

		if (aData == a->item) {
			ctrlRss.list.deleteItem(a.get());
			return true;
		}
		return false;

	});

	itemInfos.erase(first, last);

	ctrlRss.list.SetRedraw(TRUE);
}

void RssInfoFrame::onItemAdded(const RSSDataPtr& aData) {
	itemInfos.emplace_back(make_unique<ItemInfo>(aData));
	auto& d = itemInfos.back();
	if (show(d.get())) {
		ctrlRss.list.SetRedraw(FALSE);
		ctrlRss.list.insertItem(ctrlRss.list.getSortPos(d.get()), d.get(), d->getImageIndex());
		ctrlRss.list.SetRedraw(TRUE);
	}

}

void RssInfoFrame::addFeed(const RSSPtr& aFeed) {
	auto cg = feeds.find(aFeed);
	if (cg == feeds.end()) {
		auto ht = addTreeItem(treeParent, aFeed->getEnable() ? 0 : 1, Text::toT(aFeed->getFeedName()));
		feeds.emplace(aFeed, ht);
		ctrlTree.EnsureVisible(ht);
	}
}

HTREEITEM RssInfoFrame::addTreeItem(const HTREEITEM& parent, int img, const tstring& name, HTREEITEM insertAfter/* = TVI_SORT*/) {
	TVINSERTSTRUCT tvis = { 0 };
	tvis.hParent = parent;
	tvis.hInsertAfter = insertAfter;
	tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvis.item.pszText = (LPWSTR)name.c_str();
	tvis.item.iImage = img;
	tvis.item.iSelectedImage = img;
	return ctrlTree.InsertItem(&tvis);
}

bool RssInfoFrame::show(const ItemInfo* aItem) {

	auto treeFilter = [&]() -> bool {
		if (curItem != treeParent) {
			auto c = feeds.find(aItem->item->getFeed());
			return curItem == c->second;
		}
		return true;
	};

	auto filterNumericF = [&](int) -> double {
		 return 0;
	};

	auto filterInfo = ctrlRss.filter.prepare([this, aItem](int column) { return Text::fromT(aItem->getText(column)); }, filterNumericF);

	return treeFilter() && (ctrlRss.filter.empty() || ctrlRss.filter.match(filterInfo));

}

void RssInfoFrame::reloadList() {
	ctrlRss.list.SetRedraw(FALSE);
	ctrlRss.list.DeleteAllItems();
	for (auto& p : itemInfos) {
		if (!show(p.get()))
			continue;

		ctrlRss.list.insertItem(ctrlRss.list.getSortPos(p.get()), p.get(), p->getImageIndex());
	}
	ctrlRss.list.SetRedraw(TRUE);
}

vector<RssInfoFrame::ItemInfo*> RssInfoFrame::getSelectedListitems() {

	vector<ItemInfo*> list;
	if (ctrlRss.list.GetSelectedCount() > 0) {
		int i = -1;
		while ((i = ctrlRss.list.GetNextItem(i, LVNI_SELECTED)) != -1) {
			auto ii = (ItemInfo*)ctrlRss.list.GetItemData(i);
			list.push_back(ii);
		}
	}
	return list;
}

RSSPtr RssInfoFrame::getSelectedFeed() {
	auto ht = ctrlTree.GetSelectedItem();
	if (ht && ht != treeParent) {
		for (auto i : feeds) {
			if (ht == i.second) {
				return i.first;
			}
		}
	}
	return nullptr;
}

RssInfoFrame::ItemInfo::ItemInfo(const RSSDataPtr& aFeedData) : item(aFeedData) {
	isRelease = DupeUtil::isRelease(aFeedData->getTitle());
	setDupe(isRelease ? DupeUtil::checkAdcDirectoryDupe(aFeedData->getTitle(), 0) : DUPE_NONE);
	isAutosearchDupe = isRelease && AutoSearchManager::getInstance()->getSearchesByString(aFeedData->getTitle()) != AutoSearchList();
}

const tstring RssInfoFrame::ItemInfo::getText(int col) const {

	switch (col) {

	case COLUMN_FILE: return Text::toT(item->getTitle());
	case COLUMN_LINK: return Text::toT(item->getLink());
	case COLUMN_DATE: return Text::toT(item->getPubDate());
	case COLUMN_NAME: return Text::toT(item->getFeed()->getFeedName());

	default: return Util::emptyStringT;
	}
}

void RssInfoFrame::createColumns() {
	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(RSSFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(RSSFRAME_WIDTHS), COLUMN_LAST);

	for (uint8_t j = 0; j < COLUMN_LAST; j++) {
		int fmt = LVCFMT_LEFT;
		ctrlRss.list.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlRss.list.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlRss.list.setSortColumn(COLUMN_FILE);
	ctrlRss.list.setVisible(SETTING(RSSFRAME_VISIBLE));
}
}