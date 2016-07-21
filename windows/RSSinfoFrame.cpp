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
#include "WinUtil.h"
#include <airdcpp/File.h>
#include <airdcpp/LogManager.h>

int RssInfoFrame::columnIndexes[] = { COLUMN_FILE, COLUMN_DIRECTORY, COLUMN_DATE, COLUMN_SHARED, COLUMN_CATEGORIE };
int RssInfoFrame::columnSizes[] = { 800, 350, 150, 80, 150};
static ResourceManager::Strings columnNames[] = { ResourceManager::FILE, ResourceManager::LINK, ResourceManager::DATE, ResourceManager::SHARED, ResourceManager::CATEGORIES };
static SettingsManager::BoolSetting filterSettings[] = { SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST };
static ColumnType columnTypes[] = { COLUMN_TEXT, COLUMN_TEXT, COLUMN_TEXT, COLUMN_TEXT, COLUMN_TEXT };

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

	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	SetSplitterPanes(ctrlTree.m_hWnd, ctrlRss.m_hWnd);
	m_nProportionalPos = 1000;

	FillTree();
	ctrlTree.SelectItem(treeParent);
	auto tr = RSSManager::getInstance()->getRssData();
	for(auto i : tr){
		onItemAdded(i);
	}

	RSSManager::getInstance()->addListener(this);
	
	memzero(statusSizes, sizeof(statusSizes));
	statusSizes[0] = 16;
	ctrlStatus.SetParts(1, statusSizes);

	WinUtil::SetIcon(m_hWnd, IDI_RSS);
	bHandled = FALSE;
	return 1;
}

LRESULT RssInfoFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	RSSManager::getInstance()->removeListener(this);
	//temporary...
	for (auto ii : ItemInfos | map_values)
		delete ii;
	bHandled = FALSE;
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
	}

	SetSplitterRect(&rect);
	
}

void RssInfoFrame::on(RSSAdded, const RSSdata& aData) noexcept {
	callAsync([=] { onItemAdded(aData); } );

}

void RssInfoFrame::on(RSSRemoved, const string& fname) noexcept {
	callAsync([=] {
		ctrlRss.SetRedraw(FALSE);
		auto i = ItemInfos.find(fname);
		if (i != ItemInfos.end()) {
			ctrlRss.list.deleteItem(i->second);
			delete i->second;
			ItemInfos.erase(i);
		}
		ctrlRss.SetRedraw(TRUE);
	});
}

void RssInfoFrame::onItemAdded(const RSSdata& aData) {
	auto newItem = new ItemInfo(aData);
	ItemInfos.emplace(aData.getTitle(), newItem);
	if (show(newItem))
		ctrlRss.list.insertItem(newItem, 0);
	
	auto cg = categories.find(aData.getCategorie());
	if (cg == categories.end())
		addTreeItem(treeParent, 0, Text::toT(aData.getCategorie()));

}

bool RssInfoFrame::show(const ItemInfo* aItem) {

	auto treeFilter = [&]() -> bool {
		if (curItem != treeParent) {
			auto c = categories.find(aItem->item.getCategorie());
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
		if (!show(p))
			continue;

		ctrlRss.list.insertItem(ctrlRss.list.getSortPos(p), p, p->getImageIndex());
	}
	ctrlRss.list.SetRedraw(TRUE);
}


LRESULT RssInfoFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

	if (reinterpret_cast<HWND>(wParam) == ctrlRss && ctrlRss.list.GetSelectedCount() > 0) {
		if (pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlRss.list, pt);
		}

		OMenu menu;
		menu.CreatePopupMenu();
		menu.AppendMenu(MF_STRING, IDR_SEARCH, CTSTRING(SEARCH));
		menu.AppendMenu(MF_STRING, IDC_OPEN_LINK, CTSTRING(OPEN_LINK));
		menu.AppendMenu(MF_STRING, IDC_OPEN_FOLDER, CTSTRING(OPEN_FOLDER));

		menu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
		return TRUE;
	}

	bHandled = FALSE;
	return FALSE;
}

LRESULT RssInfoFrame::onOpenFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlRss.list. GetSelectedCount() == 1) {
		int sel = ctrlRss.list.GetNextItem(-1, LVNI_SELECTED);
		ItemInfo* ii = (ItemInfo*)ctrlRss.list.GetItemData(sel);
		if (ii)
			WinUtil::openFolder(Text::toT(ii->item.getFolder()));
	}
	return 0;
}

LRESULT RssInfoFrame::onSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (ctrlRss.list.GetSelectedCount() == 1) {
		int sel = ctrlRss.list.GetNextItem(-1, LVNI_SELECTED);
		ItemInfo* ii = (ItemInfo*)ctrlRss.list.GetItemData(sel);
		if (ii)
			WinUtil::searchAny(Text::toT(ii->item.getTitle()));
	}
	return 0;
}

LRESULT RssInfoFrame::onOpenLink(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (ctrlRss.list.GetSelectedCount() == 1) {
		int sel = ctrlRss.list.GetNextItem(-1, LVNI_SELECTED);
		ItemInfo* ii = (ItemInfo*)ctrlRss.list.GetItemData(sel);
		if (ii)
			WinUtil::openLink(Text::toT(ii->item.getLink()));
	}
	return 0;
}

const tstring RssInfoFrame::ItemInfo::getText(int col) const {

	switch (col) {

	case COLUMN_FILE: return Text::toT(item.getTitle());
	case COLUMN_DIRECTORY: return Text::toT(item.getFolder());
	case COLUMN_DATE: return Text::toT(item.getDate());
	case COLUMN_SHARED: return item.getShared() ? TSTRING(YES) : TSTRING(NO);
	case COLUMN_CATEGORIE: return Text::toT(item.getCategorie());

	default: return Util::emptyStringT;
	}
}

void RssInfoFrame::createColumns() {
	// Create listview columns
	//WinUtil::splitTokens(columnIndexes, SETTING(RSSFRAME_ORDER), COLUMN_LAST);
	//WinUtil::splitTokens(columnSizes, SETTING(RSSFRAME_WIDTHS), COLUMN_LAST);

	for (uint8_t j = 0; j < COLUMN_LAST; j++) {
		int fmt = LVCFMT_LEFT;
		ctrlRss.list.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlRss.list.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlRss.list.setSortColumn(COLUMN_FILE);
	//ctrlRss.list.setVisible(SETTING(RSSFRAME_VISIBLE));
}
