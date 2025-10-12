/* 
 * 
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

#include <windows/frames/recents/RecentsFrm.h>
#include <windows/frames/hub/HubFrame.h>
#include <windows/util/ActionUtil.h>
#include <windows/util/FormatUtil.h>

#include <airdcpp/recents/RecentManager.h>

namespace wingui {
string RecentsFrame::id = "Recents";

int RecentsFrame::columnIndexes[] = { COLUMN_NAME, COLUMN_DESCRIPTION, COLUMN_SERVER, COLUMN_DATE };
int RecentsFrame::columnSizes[] = { 200, 290, 100, 150 };
static ResourceManager::Strings columnNames[] = { ResourceManager::NAME, ResourceManager::DESCRIPTION, ResourceManager::HUB_ADDRESS, ResourceManager::DATE };
static SettingsManager::BoolSetting filterSettings[] = { SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST, SettingsManager::BOOL_LAST };

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
	listImages.AddIcon(CIcon(ResourceLoader::loadIcon(IDR_PRIVATE, 16)));
	listImages.AddIcon(CIcon(ResourceLoader::loadIcon(IDI_OPEN_LIST, 16)));
	ctrlList.list.SetImageList(listImages, LVSIL_SMALL);

	RecentManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	auto list = RecentManager::getInstance()->getRecents(RecentEntry::TYPE_HUB);
	for (const auto& i : list) {
		itemInfos.emplace_back(make_unique<ItemInfo>(i, RecentEntry::TYPE_HUB));
	}

	list = RecentManager::getInstance()->getRecents(RecentEntry::TYPE_PRIVATE_CHAT);
	for (const auto& i : list) {
		itemInfos.emplace_back(make_unique<ItemInfo>(i, RecentEntry::TYPE_PRIVATE_CHAT));
	}

	list = RecentManager::getInstance()->getRecents(RecentEntry::TYPE_FILELIST);
	for (const auto& i : list) {
		itemInfos.emplace_back(make_unique<ItemInfo>(i, RecentEntry::TYPE_FILELIST));
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
			vector<ItemInfo*> items;
			int i = -1;
			while ((i = ctrlList.list.GetNextItem(i, LVNI_SELECTED)) != -1) {
				auto ii = (ItemInfo*)ctrlList.list.GetItemData(i);
				items.push_back(ii);
			}

			string title = items.size() == 1 ? (items.front()->item->getName()) : STRING_F(ITEMS_X, items.size());

			menu.InsertSeparatorFirst(Text::toT(title));

			menu.appendItem(TSTRING(OPEN), [=] { for_each(items.begin(), items.end(), [=](const ItemInfo* ii) { handleOpen(ii); }); }, OMenu::FLAG_DEFAULT);

			menu.appendItem(TSTRING(REMOVE), [=] { for_each(items.begin(), items.end(), [=](const ItemInfo* ii) {
				RecentManager::getInstance()->removeRecent((RecentEntry::Type)ii->recentType, ii->item); }); }, OMenu::FLAG_DEFAULT);
			menu.appendSeparator();
		}

		menu.appendItem(TSTRING(CLEAR), [=] { 
			RecentManager::getInstance()->clearRecents(RecentEntry::TYPE_HUB); 
			RecentManager::getInstance()->clearRecents(RecentEntry::TYPE_FILELIST);
			RecentManager::getInstance()->clearRecents(RecentEntry::TYPE_PRIVATE_CHAT);
		}, OMenu::FLAG_THREADED);
		
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
	for (auto& i : itemInfos) {
		if (!show(i.get()))
			continue;
		addEntry(i.get());
	}
	ctrlList.list.SetRedraw(TRUE);
}

void RecentsFrame::handleOpen(const ItemInfo* aItem) {
	switch (aItem->recentType) {
	case RecentEntry::TYPE_HUB: {
		ActionUtil::connectHub(aItem->item->getUrl());
		break;
	}
	case RecentEntry::TYPE_PRIVATE_CHAT: {
		ActionUtil::PM()(aItem->item->getUser(), aItem->item->getUrl());
		break;
	}
	case RecentEntry::TYPE_FILELIST: {
		ActionUtil::GetBrowseList()(aItem->item->getUser(), aItem->item->getUrl());
		break;
	}
	default:
		break;
	}
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
			RecentManager::getInstance()->removeRecent((RecentEntry::Type)r->recentType, r->item);
		}
	}
	return 0;
}

LRESULT RecentsFrame::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMITEMACTIVATE* l = (NMITEMACTIVATE*)pnmh;

	if (l->iItem != -1) {
		auto ii = (ItemInfo*)ctrlList.list.GetItemData(l->iItem);
		handleOpen(ii);
	}

	bHandled = FALSE;
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

void RecentsFrame::on(RecentManagerListener::RecentUpdated, const RecentEntryPtr& entry, RecentEntry::Type) noexcept {

	callAsync([=] {
		auto i = ranges::find_if(itemInfos, [=](unique_ptr<ItemInfo>& ii) { return  ii->item == entry; });
		if (i != itemInfos.end()) {
			ctrlList.list.updateItem((*i).get());
		}
	});
}

void RecentsFrame::on(RecentManagerListener::RecentAdded, const RecentEntryPtr& entry, RecentEntry::Type aType) noexcept {
	callAsync([=] {
		auto i = ranges::find_if(itemInfos, [=](unique_ptr<ItemInfo>& ii) { return ii->item == entry; });
		if (i == itemInfos.end()) {
			itemInfos.emplace_back(make_unique<ItemInfo>(entry, aType));
			addEntry(itemInfos.back().get());
		}
	});
}
void RecentsFrame::on(RecentManagerListener::RecentRemoved, const RecentEntryPtr& entry, RecentEntry::Type) noexcept  {
	callAsync([=] {
		auto i = ranges::find_if(itemInfos, [=](unique_ptr<ItemInfo>& ii) { return ii->item == entry; });
		if (i != itemInfos.end()) {
			ctrlList.list.deleteItem((*i).get());
			itemInfos.erase(i);
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
	case COLUMN_DATE: return item->getLastOpened() > 0 ? FormatUtil::formatDateTimeW(item->getLastOpened()) : TSTRING(UNKNOWN);

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
	ctrlList.list.setSortColumn(COLUMN_DATE);
	ctrlList.list.setAscending(false);
	//ctrlList.list.setVisible(SETTING(RECENTFRAME_VISIBLE));
}
}