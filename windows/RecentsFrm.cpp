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

#include <airdcpp/RecentManager.h>

string RecentsFrame::id = "Recents";

int RecentsFrame::columnIndexes[] = { COLUMN_NAME, COLUMN_DESCRIPTION, COLUMN_SERVER };
int RecentsFrame::columnSizes[] = { 200, 290, 100 };
static ResourceManager::Strings columnNames[] = { ResourceManager::NAME, ResourceManager::DESCRIPTION, ResourceManager::HUB_ADDRESS };
static SettingsManager::BoolSetting filterSettings[] = { SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST };

RecentsFrame::RecentsFrame() : closed(false),
ctrlList(this, COLUMN_LAST, [this] { callAsync([this] { updateList(); }); }, filterSettings, COLUMN_LAST)

{ };

LRESULT RecentsFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	//ctrlStatusContainer.SubclassWindow(ctrlStatus.m_hWnd);

	ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, NULL);
	ctrlList.list.SetBkColor(WinUtil::bgColor);
	ctrlList.list.SetTextBkColor(WinUtil::bgColor);
	ctrlList.list.SetTextColor(WinUtil::textColor);
	ctrlList.list.setFlickerFree(WinUtil::bgBrush);
	ctrlList.list.SetFont(WinUtil::listViewFont);

	listImages.Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 3);
	listImages.AddIcon(CIcon(ResourceLoader::loadIcon(IDI_HUB, 16)));
	ctrlList.list.SetImageList(listImages, LVSIL_SMALL);

	RecentManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	auto list = RecentManager::getInstance()->getRecents();
	for (auto i : list) {
		itemInfos.emplace(i->getUrl(), unique_ptr<ItemInfo>(new ItemInfo(i)));
	}
	callAsync([=] { updateList(); });
	
	memzero(statusSizes, sizeof(statusSizes));
	statusSizes[0] = 16;
	ctrlStatus.SetParts(1, statusSizes);

	WinUtil::SetIcon(m_hWnd, IDI_RECENTS);
	bHandled = FALSE;
	return TRUE;
}


LRESULT RecentsFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL & /*bHandled*/) {
	if (reinterpret_cast<HWND>(wParam) == ctrlList) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };  
		if (pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlList.list, pt);
		}

		OMenu menu;
		menu.CreatePopupMenu();

		if (ctrlList.list.GetSelectedCount() > 0) {
			vector<RecentEntryPtr> items;
			int i = -1;
			while ((i = ctrlList.list.GetNextItem(i, LVNI_SELECTED)) != -1) {
				auto r = (ItemInfo*)ctrlList.list.GetItemData(i);
				items.push_back(r->item);
			}

			string title = items.size() == 1 ? (items.front()->getUrl()) : STRING_F(ITEMS_X, items.size());

			menu.InsertSeparatorFirst(Text::toT(title));

			menu.appendItem(TSTRING(CONNECT), [=] { for_each(items.begin(), items.end(), [=](const RecentEntryPtr& r) { WinUtil::connectHub(r->getUrl()); }); }, OMenu::FLAG_DEFAULT);
			menu.appendItem(TSTRING(REMOVE), [=] { for_each(items.begin(), items.end(), [=](const RecentEntryPtr& r) { 
				RecentManager::getInstance()->removeRecent(r->getUrl()); }); }, OMenu::FLAG_DEFAULT);
			menu.appendSeparator();
		}

		menu.appendItem(TSTRING(CLEAR), [=] { RecentManager::getInstance()->clearRecents(); }, OMenu::FLAG_THREADED);
		
		menu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
		return TRUE;
	}

	return FALSE;
}

LRESULT RecentsFrame::onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/) {
	ctrlList.list.SetFocus();
	return 0;
}

void RecentsFrame::updateList() {
	ctrlList.list.SetRedraw(FALSE);
	ctrlList.list.DeleteAllItems();
	for (auto& i : itemInfos | map_values) {
		if (!show(i.get()))
			continue;
		addEntry(i.get());
	}
	ctrlList.list.SetRedraw(TRUE);
}

bool RecentsFrame::show(const ItemInfo* aItem) {

	auto filterNumericF = [&](int) -> double {
		return 0;
	};

	auto filterInfo = ctrlList.filter.prepare([this, aItem](int column) { return Text::fromT(aItem->getText(column)); }, filterNumericF);

	return ctrlList.filter.empty() || ctrlList.filter.match(filterInfo);

}

void RecentsFrame::addEntry(ItemInfo* ii) {
	ctrlList.list.insertItem(ctrlList.list.getSortPos(ii), ii, ii->getImageIndex());
}

LRESULT RecentsFrame::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL & /*bHandled*/) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	if (kd->wVKey == VK_DELETE) {
		int i = -1;
		while ((i = ctrlList.list.GetNextItem(i, LVNI_SELECTED)) != -1) {
			auto r = (ItemInfo*)ctrlList.list.GetItemData(i);
			RecentManager::getInstance()->removeRecent(r->item->getUrl());
		}
	}
	return 0;
}

LRESULT RecentsFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if (!closed) {
		::KillTimer(m_hWnd, 0);
		RecentManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		closed = true;
		WinUtil::setButtonPressed(IDC_RECENTS, false);
		PostMessage(WM_CLOSE);
		return 0;
	}

	//ctrlList.list.saveHeaderOrder(SettingsManager::RECENTFRAME_ORDER,
	//	SettingsManager::RECENTFRAME_WIDTHS, SettingsManager::RECENTFRAME_VISIBLE);

	ctrlList.list.SetRedraw(FALSE);
	ctrlList.list.DeleteAllItems();
	ctrlList.list.SetRedraw(TRUE);

	itemInfos.clear();
	bHandled = FALSE;
	return 0;
}

void RecentsFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	CRect rc = rect;
	if (ctrlStatus.IsWindow()) {
		CRect sr;
		int w[2];
		ctrlStatus.GetClientRect(sr);

		w[1] = sr.right - 16;
		w[0] = 16;

		ctrlStatus.SetParts(2, w);
	}
	ctrlList.MoveWindow(rc);
}

void RecentsFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	bool refresh = false;
	if(ctrlList.list.GetBkColor() != WinUtil::bgColor) {
		ctrlList.list.SetBkColor(WinUtil::bgColor);
		ctrlList.list.SetTextBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlList.list.GetTextColor() != WinUtil::textColor) {
		ctrlList.list.SetTextColor(WinUtil::textColor);
		refresh = true;
	}

	if (ctrlList.list.GetFont() != WinUtil::listViewFont){
		ctrlList.list.SetFont(WinUtil::listViewFont);
		refresh = true;
	}

	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

void RecentsFrame::on(RecentManagerListener::RecentUpdated, const RecentEntryPtr& entry) noexcept {
	callAsync([=] {
		auto i = itemInfos.find(entry->getUrl());
		if (i != itemInfos.end()) {
			ctrlList.list.updateItem(i->second.get());
		}
	});
}

const tstring RecentsFrame::ItemInfo::getText(int col) const {
	if(!item)
		return Util::emptyStringT;

	switch (col) {

	case COLUMN_NAME: return Text::toT(item->getName());
	case COLUMN_DESCRIPTION: return Text::toT(item->getDescription());
	case COLUMN_SERVER: return Text::toT(item->getUrl());

	default: return Util::emptyStringT;
	}
}

void RecentsFrame::createColumns() {
	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(RECENTFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(RECENTFRAME_WIDTHS), COLUMN_LAST);

	for (uint8_t j = 0; j < COLUMN_LAST; j++) {
		int fmt = LVCFMT_LEFT;
		ctrlList.list.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlList.list.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlList.list.setSortColumn(COLUMN_NAME);
	//ctrlList.list.setVisible(SETTING(RECENTFRAME_VISIBLE));
}
