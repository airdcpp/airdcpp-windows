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

#include "ExtensionsFrame.h"
#include <web-server/Extension.h>

string ExtensionsFrame::id = "Extensions";

int ExtensionsFrame::columnIndexes[] = { COLUMN_NAME, COLUMN_DESCRIPTION};
int ExtensionsFrame::columnSizes[] = { 200, 350};
static ResourceManager::Strings columnNames[] = { ResourceManager::NAME, ResourceManager::DESCRIPTION };

ExtensionsFrame::ExtensionsFrame() : closed(false)
{ };

LRESULT ExtensionsFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	//ctrlStatusContainer.SubclassWindow(ctrlStatus.m_hWnd);

	ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, IDC_EXTENSIONS_LIST);
	ctrlList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_SUBITEMIMAGES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
	createColumns();

	ctrlList.SetBkColor(WinUtil::bgColor);
	ctrlList.SetTextBkColor(WinUtil::bgColor);
	ctrlList.SetTextColor(WinUtil::textColor);
	ctrlList.setFlickerFree(WinUtil::bgBrush);
	ctrlList.SetFont(WinUtil::listViewFont);

	SettingsManager::getInstance()->addListener(this);

	webserver::ExtensionManager& emgr = webserver::WebServerManager::getInstance()->getExtensionManager();
	emgr.addListener(this);
	auto list = emgr.getExtensions();
	for (const auto& i : list) {
		itemInfos.emplace_back(make_unique<ItemInfo>(i));
	}

	callAsync([=] { updateList(); });

	memzero(statusSizes, sizeof(statusSizes));
	statusSizes[0] = 16;
	ctrlStatus.SetParts(1, statusSizes);

	WinUtil::SetIcon(m_hWnd, IDI_QCONNECT);
	bHandled = FALSE;
	return TRUE;
}


LRESULT ExtensionsFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL & /*bHandled*/) {
	if (reinterpret_cast<HWND>(wParam) == ctrlList) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		if (pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlList, pt);
		}

		//OMenu menu;
		//menu.CreatePopupMenu();
		//menu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
		//return TRUE;
	}

	return FALSE;
}

LRESULT ExtensionsFrame::onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/) {
	ctrlList.SetFocus();
	return 0;
}

void ExtensionsFrame::updateList() {
	ctrlList.SetRedraw(FALSE);
	ctrlList.DeleteAllItems();
	for (auto& i : itemInfos) {
		addEntry(i.get());
	}
	ctrlList.SetRedraw(TRUE);
}

void ExtensionsFrame::addEntry(ItemInfo* ii) {
	ctrlList.insertItem(ctrlList.getSortPos(ii), ii, ii->getImageIndex());
}

LRESULT ExtensionsFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if (!closed) {
		::KillTimer(m_hWnd, 0);
		SettingsManager::getInstance()->removeListener(this);
		webserver::ExtensionManager& emgr = webserver::WebServerManager::getInstance()->getExtensionManager();
		emgr.removeListener(this);
		closed = true;
		WinUtil::setButtonPressed(IDC_EXTENSIONS, false);
		PostMessage(WM_CLOSE);
		return 0;
	}


	ctrlList.SetRedraw(FALSE);
	ctrlList.DeleteAllItems();
	ctrlList.SetRedraw(TRUE);

	itemInfos.clear();
	bHandled = FALSE;
	return 0;
}

void ExtensionsFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
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

void ExtensionsFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	bool refresh = false;
	if (ctrlList.GetBkColor() != WinUtil::bgColor) {
		ctrlList.SetBkColor(WinUtil::bgColor);
		ctrlList.SetTextBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if (ctrlList.GetTextColor() != WinUtil::textColor) {
		ctrlList.SetTextColor(WinUtil::textColor);
		refresh = true;
	}

	if (ctrlList.GetFont() != WinUtil::listViewFont) {
		ctrlList.SetFont(WinUtil::listViewFont);
		refresh = true;
	}

	if (refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

void ExtensionsFrame::on(webserver::ExtensionManagerListener::ExtensionAdded, const webserver::ExtensionPtr& e) noexcept {
	callAsync([=] {
		itemInfos.emplace_back(make_unique<ItemInfo>(e));
		addEntry(itemInfos.back().get());
	});
}

void ExtensionsFrame::on(webserver::ExtensionManagerListener::ExtensionRemoved, const webserver::ExtensionPtr& e) noexcept {
	callAsync([=] {
		ctrlList.SetRedraw(FALSE);
		itemInfos.erase(boost::remove_if(itemInfos, [&](const unique_ptr<ItemInfo>& a) {

			if (e == a->item) {
				ctrlList.deleteItem(a.get());
				return true;
			}
			return false;

		}), itemInfos.end());

		ctrlList.SetRedraw(TRUE);
	});
}

const tstring ExtensionsFrame::ItemInfo::getText(int col) const {
	if (!item)
		return Util::emptyStringT;

	switch (col) {

	case COLUMN_NAME: return Text::toT(item->getName());
	case COLUMN_DESCRIPTION: return Text::toT(item->getDescription());

	default: return Util::emptyStringT;
	}
}

void ExtensionsFrame::createColumns() {
	// Create list view columns
	for (uint8_t j = 0; j < COLUMN_LAST; j++) {
		int fmt = LVCFMT_LEFT;
		ctrlList.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlList.setSortColumn(COLUMN_NAME);

}
