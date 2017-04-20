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
#include "ResourceLoader.h"
#include "DynamicDialogBase.h"
#include <airdcpp/ScopedFunctor.h>
#include <api/common/SettingUtils.h>

string ExtensionsFrame::id = "Extensions";

int ExtensionsFrame::columnIndexes[] = { COLUMN_NAME, COLUMN_DESCRIPTION};
int ExtensionsFrame::columnSizes[] = { 300, 900};
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

	//TODO: add new icons
	listImages.Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 3);
	listImages.AddIcon(CIcon(ResourceLoader::loadIcon(IDI_ONLINE, 16))); //extension running
	listImages.AddIcon(CIcon(ResourceLoader::convertGrayscaleIcon(ResourceLoader::loadIcon(IDI_ONLINE, 16)))); //extension stopped
	listImages.AddIcon(CIcon(ResourceLoader::convertGrayscaleIcon(ResourceLoader::loadIcon(IDI_QCONNECT, 16))));  // remote extension, not installed
	ctrlList.SetImageList(listImages, LVSIL_SMALL);

	SettingsManager::getInstance()->addListener(this);

	getExtensionManager().addListener(this);
	auto list = getExtensionManager().getExtensions();
	for (const auto& i : list) {
		itemInfos.emplace(i ->getName(), make_unique<ItemInfo>(i));
	}

	callAsync([=] { updateList(); downloadExtensionList(); });

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
		
		if (ctrlList.GetSelectedCount() == 1) {
			int sel = ctrlList.GetNextItem(-1, LVNI_SELECTED);
			auto ii = ctrlList.getItemData(sel);
			string title = ii->item ? ii->item->getName() : ii->getName();
			OMenu menu;
			menu.CreatePopupMenu();
			menu.InsertSeparatorFirst(Text::toT(title));

			if (!ii->item) {
				menu.appendItem(TSTRING(INSTALL), [=] { downloadExtensionInfo(ii); });
			} else {
				if (!ii->item->isRunning())
					menu.appendItem(TSTRING(START), [=] { onStartExtension(ii); });
				else
					menu.appendItem(TSTRING(STOP), [=] { onStopExtension(ii); });

				menu.appendSeparator();
				menu.appendItem(TSTRING(REMOVE), [=] { onRemoveExtension(ii); });
				menu.appendItem(TSTRING(SETTINGS_CHANGE), [=] { onConfigExtension(ii); });
			}
			menu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
			return TRUE;
		}
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
		addEntry(i.second.get());
	}
	ctrlList.SetRedraw(TRUE);
}

void ExtensionsFrame::addEntry(const ItemInfo* ii) {
	ctrlList.insertItem(ctrlList.getSortPos(ii), ii, ii->getImageIndex());
}

void ExtensionsFrame::updateEntry(const ItemInfo* ii) {
	ctrlList.updateItemImage(ii);
	ctrlList.updateItem(ii);
}

void ExtensionsFrame::onStopExtension(const ItemInfo* ii) {
	getExtensionManager().stopExtension(ii->item);
	updateEntry(ii);
}

void ExtensionsFrame::onStartExtension(const ItemInfo* ii) {
	getExtensionManager().startExtension(ii->item);
	updateEntry(ii);
}

void ExtensionsFrame::onRemoveExtension(const ItemInfo* ii) {
	getExtensionManager().removeExtension(ii->item);
}

void ExtensionsFrame::onConfigExtension(const ItemInfo* ii) {
	DynamicDialogBase dlg(STRING(SETTINGS_EXTENSIONS));

	for (int i = 0; i < 10; i++) {
		dlg.getPage()->addConfigItem(Util::toString(i) + " Test label for CEdit config", Util::toString(i), webserver::ApiSettingItem::TYPE_STRING);
	}

	for (int i = 15; i < 20; i++) {
		dlg.getPage()->addConfigItem(Util::toString(i) + " Test label for CEdit config", Util::toString(i), webserver::ApiSettingItem::TYPE_BOOLEAN);
	}

	for (int i = 20; i < 22; i++) {
		dlg.getPage()->addConfigItem(Util::toString(i) + " Test label for CEdit config", Util::toString(i), webserver::ApiSettingItem::TYPE_FILE_PATH);
	}


	if (dlg.DoModal() == IDOK) {
	}
}


void ExtensionsFrame::downloadExtensionList() {
	httpDownload.reset(new HttpDownload(extensionUrl,
		[this] { onExtensionListDownloaded(); }, false));
}

void ExtensionsFrame::onExtensionListDownloaded() {

	ScopedFunctor([&] { httpDownload.reset(); });

	if (httpDownload->buf.empty()) {
		return;
	}

	try {

		json listJson = json::parse(httpDownload->buf);
		auto pJson = listJson.at("objects");

		for (const auto i : pJson) {
			json package = i.at("package");
			string aName = package.at("name");
			string aDesc = package.at("description");
			itemInfos.emplace(aName, make_unique<ItemInfo>(aName, aDesc));
		}

		updateList();
	}	catch (const std::exception& /*e*/) { }

}

void ExtensionsFrame::downloadExtensionInfo(const ItemInfo* ii) {
	httpDownload.reset(new HttpDownload(packageUrl + ii->getName() + "/latest",
		[this] { onExtensionInfoDownloaded(); }, false));
}

void ExtensionsFrame::onExtensionInfoDownloaded() {
	ScopedFunctor([&] { httpDownload.reset(); });

	if (httpDownload->buf.empty()) {
		return;
	}

	try {
		const json packageJson = json::parse(httpDownload->buf);
		json dist = packageJson.at("dist");

		const string aSha = dist.at("shasum");
		const string aUrl = dist.at("tarball");

		getExtensionManager().downloadExtension(aUrl, aSha);
	} catch (const std::exception& /*e*/) {}

}
string ExtensionsFrame::getData(const string& aData, const string& aEntry, size_t& pos) {
	pos = aData.find(aEntry, pos);
	if (pos == string::npos)
		return Util::emptyString;

	pos += aEntry.length();
	size_t iend = aData.find(",", pos);
	return aData.substr(pos, iend-1 - pos);

}
LRESULT ExtensionsFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if (!closed) {
		SettingsManager::getInstance()->removeListener(this);
		getExtensionManager().removeListener(this);
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
		auto i = itemInfos.find(e->getName());
		if (i == itemInfos.end()) {
			auto ret = itemInfos.emplace(e->getName(), make_unique<ItemInfo>(e));
			addEntry(ret.first->second.get());
		} else {
			auto& ii = i->second;
			ii->item = e;
			updateEntry(ii.get());
		}
	});
}

void ExtensionsFrame::on(webserver::ExtensionManagerListener::ExtensionRemoved, const webserver::ExtensionPtr& e) noexcept {
	callAsync([=] {
		ctrlList.SetRedraw(FALSE);
		auto i = itemInfos.find(e->getName());
		if (i != itemInfos.end()) {
			ctrlList.deleteItem(i->second.get());
			itemInfos.erase(i);
			downloadExtensionList();
		}
		ctrlList.SetRedraw(TRUE);
	});
}

const tstring ExtensionsFrame::ItemInfo::getText(int col) const {

	switch (col) {

	case COLUMN_NAME: return item == nullptr ? Text::toT(getName()) : Text::toT(item->getName());
	case COLUMN_DESCRIPTION: return item == nullptr ? Text::toT(getDescription()) : Text::toT(item->getDescription());

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
