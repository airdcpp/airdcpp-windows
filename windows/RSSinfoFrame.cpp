/*
* Copyright (C) 2011-2016 AirDC++ Project
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

#include "RSSinfoFrame.h"
#include "MainFrm.h"
#include "WinUtil.h"
#include "ResourceLoader.h"

#include <airdcpp/File.h>
#include <airdcpp/LogManager.h>

#include <boost/algorithm/cxx11/all_of.hpp>

using boost::algorithm::all_of;

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
	m_nProportionalPos = 1500;

	treeParent = addTreeItem(TVI_ROOT, 0, TSTRING(RSS_FEEDS));
	ctrlTree.SelectItem(treeParent);
	{
		Lock l(RSSManager::getInstance()->getCS());
		auto lst = RSSManager::getInstance()->getRss();
		for (auto feed : lst) {
			addFeed(feed);
			for (auto data : feed->getFeedData() | map_values) {
				ItemInfos.emplace(data->getTitle(), unique_ptr<ItemInfo>(new ItemInfo(data)));
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

	ItemInfos.clear();
	bHandled = FALSE;
	return 0;
}

LRESULT RssInfoFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	bool listMenu = false;
	bool treeMenu = false;
	bool hitTreeitem = false;
	if (reinterpret_cast<HWND>(wParam) == ctrlRss && ctrlRss.list.GetSelectedCount() == 1) {
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
		auto ii = getSelectedListitem();
		if (ii) {
			OMenu menu;
			menu.CreatePopupMenu();
			menu.InsertSeparatorFirst(Text::toT(ii->item->getTitle()));

			menu.appendItem(TSTRING(OPEN_LINK), [=] { WinUtil::openLink(Text::toT(ii->item->getLink())); }, OMenu::FLAG_DEFAULT);
			menu.appendSeparator();

			ListBaseType::MenuItemList customItems;
			ctrlRss.list.appendCopyMenu(menu, customItems);
			menu.appendSeparator();

			if (ii->isRelease) {
				//autosearch menus
				appendDownloadMenu(menu, DownloadBaseHandler::TYPE_SECONDARY, true, boost::none, ii->item->getTitle() + PATH_SEPARATOR, false);
				menu.appendSeparator();
			}

			menu.appendItem(TSTRING(SEARCH), [=] { WinUtil::search(Text::toT(ii->item->getTitle())); });
			WinUtil::appendSearchMenu(menu, ii->item->getTitle());
			if (AirUtil::allowOpenDupe(ii->getDupe())) {
				menu.appendSeparator();
				menu.appendItem(TSTRING(OPEN_FOLDER), [=] {
					auto paths = AirUtil::getDirDupePaths(ii->getDupe(), ii->item->getTitle());
					if (!paths.empty())
						WinUtil::openFolder(Text::toT(paths.front()));
				});
			}

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
			menu.appendItem(TSTRING(MATCH_AUTOSEARCH), [=] { RSSManager::getInstance()->matchFilters(feed);  }, OMenu::FLAG_THREADED);
			menu.appendItem(TSTRING(CLEAR), [=] { RSSManager::getInstance()->clearRSSData(feed);  }, OMenu::FLAG_THREADED);
			menu.appendSeparator();
			menu.appendItem(TSTRING(ADD), [=] { add(); });
			menu.appendItem(TSTRING(SETTINGS_CHANGE), [=] { change(); });
			menu.appendItem(TSTRING(REMOVE), [=] { remove(); });
			menu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
			return TRUE;
		}
		else if (hitTreeitem && treeParent == ctrlTree.GetSelectedItem()) {
			bool allEnabled = all_of(feeds | map_keys, [&](const RSSPtr& r) { return r->getEnable(); });
			bool allDisabled = all_of(feeds | map_keys, [&](const RSSPtr& r) { return !r->getEnable(); });
			menu.InsertSeparatorFirst(Util::toStringW(feeds.size()) + _T(" ") + TSTRING(FEEDS));
			//menu.appendItem(TSTRING(UPDATE), [=] { for_each(feeds | map_keys, [&](const RSSPtr& feed) { RSSManager::getInstance()->downloadFeed(feed, true); }); }, OMenu::FLAG_THREADED);
			if(!allEnabled)
				menu.appendItem(TSTRING(ENABLE_RSS), [=] { for_each(feeds | map_keys, [&](const RSSPtr& feed) { RSSManager::getInstance()->enableFeedUpdate(feed, true); }); }, OMenu::FLAG_THREADED);
			if(!allDisabled)
				menu.appendItem(TSTRING(DISABLE_RSS), [=] { for_each(feeds | map_keys, [&](const RSSPtr& feed) { RSSManager::getInstance()->enableFeedUpdate(feed, false); }); }, OMenu::FLAG_THREADED);
			menu.appendItem(TSTRING(MATCH_AUTOSEARCH), [=] { for_each(feeds | map_keys, [&](const RSSPtr& feed) { RSSManager::getInstance()->matchFilters(feed); }); }, OMenu::FLAG_THREADED);
			menu.appendItem(TSTRING(CLEAR), [=] { for_each(feeds | map_keys, [&](const RSSPtr& feed) { RSSManager::getInstance()->clearRSSData(feed); }); }, OMenu::FLAG_THREADED);
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

LRESULT RssInfoFrame::onDoubleClickList(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
	auto ii = getSelectedListitem();
	if(ii)
		WinUtil::openLink(Text::toT(ii->item->getLink()));
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
		const int textbox_width = 30;

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

void RssInfoFrame::on(AutoSearchManagerListener::AddItem, const AutoSearchPtr& as) noexcept {
	addGuiTask([=] { updateDupeType(as->getSearchString()); });
}
void RssInfoFrame::on(AutoSearchManagerListener::RemoveItem, const AutoSearchPtr& as) noexcept {
	addGuiTask([=] { updateDupeType(as->getSearchString()); });
}

void RssInfoFrame::updateDupeType(const string& aName) {
	auto i = ItemInfos.find(aName);
	if (i != ItemInfos.end()) {
		i->second->setDupe(AirUtil::checkDirDupe(i->second->item->getTitle(), 0));

		if(i->second->getDupe() == DupeType::DUPE_NONE)
			i->second->isAutosearchDupe =  AutoSearchManager::getInstance()->getSearchesByString(aName) != AutoSearchList();

		ctrlRss.list.Invalidate();
	}
}

void RssInfoFrame::handleDownload(const string& aTarget, Priority /*p*/, bool /*aIsRelease*/) {
	auto ii = ctrlRss.list.getSelectedItem();
	if(ii)
		AutoSearchManager::getInstance()->addAutoSearch(ii->item->getTitle(), aTarget, true, AutoSearch::RSS_DOWNLOAD);
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
		RSSManager::getInstance()->saveConfig(false);
	});
}
void RssInfoFrame::openDialog(RSSPtr& aFeed) {
	TabbedDialog dlg(STRING(RSS_CONFIG));
	dlg.addPage<RssFeedsPage>(shared_ptr<RssFeedsPage>(new RssFeedsPage(STRING(SETTINGS_GENERAL), aFeed)));
	dlg.addPage<RssFilterPage>(shared_ptr<RssFilterPage>(new RssFilterPage(STRING(FILTER), aFeed)));
	if (dlg.DoModal() == IDOK) {
		MainFrame::getMainFrame()->addThreadedTask([=] {
			RSSManager::getInstance()->saveConfig(false);
		});
	}
}

void RssInfoFrame::clearData(const RSSPtr& aFeed) {
	ctrlRss.list.SetRedraw(FALSE);
	ItemInfos.erase(boost::remove_if(ItemInfos | map_values, [&](const unique_ptr<ItemInfo>& a) {

		if (aFeed == a->item->getFeed()) {
			ctrlRss.list.deleteItem(a.get());
			return true;
		}
		return false;

	}).base(), ItemInfos.end());

	ctrlRss.list.SetRedraw(TRUE);
}

void RssInfoFrame::onItemAdded(const RSSDataPtr& aData) {
	auto i = ItemInfos.emplace(aData->getTitle(), unique_ptr<ItemInfo>(new ItemInfo(aData))).first;
	if (show(i->second.get()))
		ctrlRss.list.insertItem(i->second.get(), i->second->getImageIndex());

}

void RssInfoFrame::addFeed(const RSSPtr& aFeed) {
	auto cg = feeds.find(aFeed);
	if (cg == feeds.end()) {
		auto ht = addTreeItem(treeParent, aFeed->getEnable() ? 0 : 1, Text::toT(aFeed->getFeedName()));
		feeds.emplace(aFeed, ht);
		ctrlTree.EnsureVisible(ht);
	}
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
	for (auto& p : ItemInfos | map_values) {
		if (!show(p.get()))
			continue;

		ctrlRss.list.insertItem(ctrlRss.list.getSortPos(p.get()), p.get(), p->getImageIndex());
	}
	ctrlRss.list.SetRedraw(TRUE);
}

RssInfoFrame::ItemInfo* RssInfoFrame::getSelectedListitem() {
	if (ctrlRss.list.GetSelectedCount() == 1) {
		int sel = ctrlRss.list.GetNextItem(-1, LVNI_SELECTED);
		auto ii = (ItemInfo*)ctrlRss.list.GetItemData(sel);
		if (ii) {
			return ii;
		}
	}
	return nullptr;
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
