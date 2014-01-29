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

#include "QueueFrame2.h"
#include "MainFrm.h"
#include "Async.h"

#include "../client/AirUtil.h"
#include "../client/ShareManager.h"
#include "../client/ClientManager.h"
#include "../client/DownloadManager.h"
#include "ResourceLoader.h"

#define FILE_LIST_NAME _T("File Lists")
#define TEMP_NAME _T("Temp items")

int QueueFrame2::columnIndexes[] = { COLUMN_NAME, COLUMN_SIZE, COLUMN_PRIORITY, COLUMN_DOWNLOADED, COLUMN_SOURCES, COLUMN_PATH};

int QueueFrame2::columnSizes[] = { 400, 80, 120, 120, 120, 500 };

static ResourceManager::Strings columnNames[] = { ResourceManager::NAME, ResourceManager::SIZE, ResourceManager::PRIORITY, ResourceManager::DOWNLOADED, ResourceManager::SOURCES, ResourceManager::PATH };

LRESULT QueueFrame2::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{

	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	ctrlQueue.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_QUEUE_LIST);
	ctrlQueue.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
	ctrlQueue.SetImageList(ResourceLoader::getFileImages(), LVSIL_SMALL);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(QUEUEFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(QUEUEFRAME_WIDTHS), COLUMN_LAST);

	for (uint8_t j = 0; j < COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_DOWNLOADED) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlQueue.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}

	ctrlQueue.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlQueue.setSortColumn(COLUMN_NAME);
	ctrlQueue.setVisible(SETTING(QUEUEFRAME_VISIBLE));
	ctrlQueue.SetBkColor(WinUtil::bgColor);
	ctrlQueue.SetTextBkColor(WinUtil::bgColor);
	ctrlQueue.SetTextColor(WinUtil::textColor);
	ctrlQueue.setFlickerFree(WinUtil::bgBrush);

	CRect rc(SETTING(QUEUE_LEFT), SETTING(QUEUE_TOP), SETTING(QUEUE_RIGHT), SETTING(QUEUE_BOTTOM));
	if (!(rc.top == 0 && rc.bottom == 0 && rc.left == 0 && rc.right == 0))
		MoveWindow(rc, TRUE);

	{
		auto qm = QueueManager::getInstance();
		RLock l(qm->getCS());
		for (const auto& b : qm->getBundles() | map_values)
			callAsync([=] { onBundleAdded(b); });
	}
	//ctrlQueue.resort();

	QueueManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);

	memzero(statusSizes, sizeof(statusSizes));
	statusSizes[0] = 16;
	ctrlStatus.SetParts(6, statusSizes);
	//updateStatus();

	WinUtil::SetIcon(m_hWnd, IDI_QUEUE);
	bHandled = FALSE;
	return 1;
}

void QueueFrame2::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
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

		ctrlStatus.GetRect(0, sr);
	}
	CRect rc = rect;
	ctrlQueue.MoveWindow(&rc);
}


LRESULT QueueFrame2::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if (!closed) {
		QueueManager::getInstance()->removeListener(this);
		DownloadManager::getInstance()->removeListener(this);
		closed = true;
		WinUtil::setButtonPressed(IDC_QUEUE2, false);
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
			SettingsManager::getInstance()->set(SettingsManager::QUEUE_BOTTOM, (rc.bottom > 0 ? rc.bottom : 0));
			SettingsManager::getInstance()->set(SettingsManager::QUEUE_TOP, (rc.top > 0 ? rc.top : 0));
			SettingsManager::getInstance()->set(SettingsManager::QUEUE_LEFT, (rc.left > 0 ? rc.left : 0));
			SettingsManager::getInstance()->set(SettingsManager::QUEUE_RIGHT, (rc.right > 0 ? rc.right : 0));
		}

		ctrlQueue.saveHeaderOrder(SettingsManager::QUEUEFRAME_ORDER,
			SettingsManager::QUEUEFRAME_WIDTHS, SettingsManager::QUEUEFRAME_VISIBLE);
	
		ctrlQueue.DeleteAllItems(); //don't allow list view to delete itemInfos, its too slow...
		for (auto& i : itemInfos)
			delete i.second;

		itemInfos.clear();

		bHandled = FALSE;
		return 0;
	}
}

LRESULT QueueFrame2::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	map<string, BundlePtr> bundles;
	QueueItemList queueitems;
	int sel = -1;
	int finishedFiles = 0;
	int fileBundles = 0;
	while ((sel = ctrlQueue.GetNextItem(sel, LVNI_SELECTED)) != -1) {
		QueueItemInfo* qii = (QueueItemInfo*)ctrlQueue.GetItemData(sel);
		if (qii->bundle) {
			finishedFiles += qii->bundle->getFinishedFiles().size();
			if (qii->bundle->isFileBundle())
				fileBundles++;
			bundles.emplace(qii->bundle->getToken(), qii->bundle);
		} else {
			//did we select the bundle to be deleted?
			if (bundles.find(qii->qi->getBundle()->getToken()) == bundles.end()) {
				queueitems.push_back(qii->qi);
			}
		}

	}

	if (bundles.size() >= 1) {
		string tmp;
		int dirBundles = bundles.size() - fileBundles;
		bool moveFinished = false;

		if (bundles.size() == 1) {
			BundlePtr bundle = bundles.begin()->second;
			tmp = STRING_F(CONFIRM_REMOVE_DIR_BUNDLE, bundle->getName().c_str());
			if (!WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_QUEUE_REMOVAL, Text::toT(tmp))) {
				return 0;
			}
			else {
				if (finishedFiles > 0) {
					tmp = STRING_F(CONFIRM_REMOVE_DIR_FINISHED_BUNDLE, finishedFiles);
					if (WinUtil::showQuestionBox(Text::toT(tmp), MB_ICONQUESTION)) {
						moveFinished = true;
					}
				}
			}
		}
		else {
			tmp = STRING_F(CONFIRM_REMOVE_DIR_MULTIPLE, dirBundles % fileBundles);
			if (!WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_QUEUE_REMOVAL, Text::toT(tmp))) {
				return 0;
			}
			else {
				if (finishedFiles > 0) {
					tmp = STRING_F(CONFIRM_REMOVE_DIR_FINISHED_MULTIPLE, finishedFiles);
					if (WinUtil::showQuestionBox(Text::toT(tmp), MB_ICONQUESTION)) {
						moveFinished = true;
					}
				}
			}
		}

		MainFrame::getMainFrame()->addThreadedTask([=] {
			for (auto b : bundles | map_values)
				QueueManager::getInstance()->removeBundle(b, false, moveFinished);
		});
	}

	if (queueitems.size() >= 1) {
		if (WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_QUEUE_REMOVAL, TSTRING(REALLY_REMOVE))) {
			for (auto& qi : queueitems)
				QueueManager::getInstance()->removeFile(qi->getTarget());
		}
	}

	return 0;
}

LRESULT QueueFrame2::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	if (reinterpret_cast<HWND>(wParam) == ctrlQueue && ctrlQueue.GetSelectedCount() > 0) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if (pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlQueue, pt);
		}

		OMenu bundleMenu;
		bundleMenu.CreatePopupMenu();

		OMenu* removeMenu = bundleMenu.getMenu();
		OMenu* readdMenu = bundleMenu.getMenu();

		int selCount = ctrlQueue.GetSelectedCount();
		BundleList bl;
		int sel = -1;
		while ((sel = ctrlQueue.GetNextItem(sel, LVNI_SELECTED)) != -1) {
			QueueItemInfo* qii = (QueueItemInfo*)ctrlQueue.GetItemData(sel);
			//temp for QueueItems, need to do something smart for them..
			if (qii->bundle)
				bl.push_back(qii->bundle);
		}
		if (bl.size() >= 1) {
			if (bl.size() == 1) {
				bundleMenu.InsertSeparatorFirst(TSTRING(BUNDLE));
				WinUtil::appendBundlePrioMenu(bundleMenu, bl);
			} else {
				bundleMenu.InsertSeparatorFirst(CTSTRING_F(X_BUNDLES, bl.size()));
				WinUtil::appendBundlePrioMenu(bundleMenu, bl);
			}
		}

		/* Insert sub menus */
		auto formatUser = [this](Bundle::BundleSource& bs) -> tstring {
			auto& u = bs.user;
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
		};

		if (selCount == 1) {
			QueueItemInfo* qii = (QueueItemInfo*)ctrlQueue.GetItemData(ctrlQueue.GetNextItem(-1, LVNI_SELECTED));
			if (qii->bundle) {
				BundlePtr b = qii->bundle;
				//current sources
				auto bundleSources = move(QueueManager::getInstance()->getBundleSources(b));
				if (!bundleSources.empty()) {
					removeMenu->appendItem(TSTRING(ALL), [b] {
						auto sources = move(QueueManager::getInstance()->getBundleSources(b));
						for (auto& si : sources)
							QueueManager::getInstance()->removeBundleSource(b, si.user.user, QueueItem::Source::FLAG_REMOVED);
					}, OMenu::FLAG_THREADED);
					removeMenu->appendSeparator();
				}

				for (auto& bs : bundleSources) {
					auto u = bs.user;
					removeMenu->appendItem(formatUser(bs), [=] { QueueManager::getInstance()->removeBundleSource(b, u, QueueItem::Source::FLAG_REMOVED); }, OMenu::FLAG_THREADED);
				}

				//bad sources
				auto badBundleSources = move(QueueManager::getInstance()->getBadBundleSources(b));

				if (!badBundleSources.empty()) {
					readdMenu->appendItem(TSTRING(ALL), [=] {
						auto sources = move(QueueManager::getInstance()->getBadBundleSources(b));
						for (auto& si : sources)
							QueueManager::getInstance()->readdBundleSource(b, si.user);
					}, OMenu::FLAG_THREADED);
					readdMenu->appendSeparator();
				}

				for (auto& bs : badBundleSources) {
					auto u = bs.user;
					readdMenu->appendItem(formatUser(bs), [=] { QueueManager::getInstance()->readdBundleSource(b, u); }, OMenu::FLAG_THREADED);
				}
				/* Sub menus end */

				// search
				bundleMenu.appendItem(TSTRING(SEARCH_BUNDLE_ALT), [=] {
					auto bundle = b;
					QueueManager::getInstance()->searchBundle(bundle, true);
				}, OMenu::FLAG_THREADED);

				bundleMenu.appendSeparator();

				WinUtil::appendSearchMenu(bundleMenu, b->getName());
				bundleMenu.appendItem(TSTRING(SEARCH_DIRECTORY), [=] {
					WinUtil::searchAny(b->isFileBundle() ? Util::getLastDir(Text::toT(b->getTarget())) : Text::toT(b->getName()));
				});

				bundleMenu.appendItem(TSTRING(OPEN_FOLDER), [=] { WinUtil::openFolder(Text::toT(b->getTarget())); });
				//bundleMenu.AppendMenu(MF_STRING, IDC_MOVE, CTSTRING(MOVE_DIR));
				bundleMenu.appendItem(TSTRING(RENAME), [=] { onRenameBundle(b); });

				bundleMenu.appendSeparator();

				readdMenu->appendThis(TSTRING(READD_SOURCE), true);
				removeMenu->appendThis(TSTRING(REMOVE_SOURCE), true);

				bundleMenu.appendItem(TSTRING(USE_SEQ_ORDER), [=] {
					auto bundle = b;
					QueueManager::getInstance()->onUseSeqOrder(bundle);
				}, b->getSeqOrder() ? OMenu::FLAG_CHECKED : 0 | OMenu::FLAG_THREADED);
			}
		}
		bundleMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));

		bundleMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
		return TRUE;
	}

	bHandled = FALSE;
	return FALSE;
}

void QueueFrame2::onRenameBundle(BundlePtr b) {
	LineDlg dlg;
	dlg.title = TSTRING(RENAME);
	dlg.description = TSTRING(NEW_NAME);
	dlg.line = Text::toT(b->getName());
	if (dlg.DoModal(m_hWnd) == IDOK) {
		auto newName = Util::validatePath(Text::fromT(dlg.line), true);
		if (newName == b->getName()) {
			return;
		}

		MainFrame::getMainFrame()->addThreadedTask([=] {
			QueueManager::getInstance()->renameBundle(b, newName);
		});
	}
}

void QueueFrame2::onBundleAdded(const BundlePtr& aBundle) {
	auto i = itemInfos.find(aBundle->getToken());
	if (i == itemInfos.end()) {
		auto b = itemInfos.emplace(aBundle->getToken(), new QueueItemInfo(aBundle)).first;
		if (aBundle->isFileBundle()) {
			ctrlQueue.insertItem(b->second, b->second->getImageIndex());
			return;
		}
		ctrlQueue.insertGroupedItem(b->second, false);
		for (auto& qi : aBundle->getQueueItems()){
			auto item = new QueueItemInfo(qi);
			itemInfos.emplace(qi->getTarget(), item);
			ctrlQueue.insertGroupedItem(item, false);
		}

	}
}

void QueueFrame2::onBundleRemoved(const BundlePtr& aBundle) {
	auto i = itemInfos.find(aBundle->getToken());
	if (i != itemInfos.end()) {
		if (aBundle->isFileBundle() || aBundle->getQueueItems().empty()) //think of this more, now all queueitems get removed first then the whole bundle.
			ctrlQueue.deleteItem(i->second);
		else
			ctrlQueue.removeGroupedItem(i->second);
		itemInfos.erase(i);
	}
}

void QueueFrame2::onBundleUpdated(const BundlePtr& aBundle) {
	auto i = itemInfos.find(aBundle->getToken());
	if (i != itemInfos.end()) {
		ctrlQueue.updateItem(i->second);
	}
}

void QueueFrame2::onQueueItemRemoved(const QueueItemPtr& aQI) {
	auto item = itemInfos.find(aQI->getTarget());
	if (item != itemInfos.end()) {
		ctrlQueue.removeGroupedItem(item->second);
		itemInfos.erase(item);
	}
}

void QueueFrame2::onQueueItemUpdated(const QueueItemPtr& aQI) {
	auto item = itemInfos.find(aQI->getTarget());
	if (item != itemInfos.end()) {
		auto itemInfo = item->second;
		if (itemInfo->parent && !itemInfo->parent->collapsed) // no need to update if its collapsed right?
			ctrlQueue.updateItem(itemInfo);
	}
}

void QueueFrame2::on(QueueManagerListener::BundleAdded, const BundlePtr& aBundle) noexcept {
	callAsync([=] { onBundleAdded(aBundle); });
}
void QueueFrame2::on(QueueManagerListener::BundleRemoved, const BundlePtr& aBundle) noexcept{
	callAsync([=] { onBundleRemoved(aBundle); });
}
void QueueFrame2::on(QueueManagerListener::BundleMoved, const BundlePtr& aBundle) noexcept{
	callAsync([=] { onBundleRemoved(aBundle); }); 
}
void QueueFrame2::on(QueueManagerListener::BundleMerged, const BundlePtr& aBundle, const string&) noexcept { 
	callAsync([=] { onBundleUpdated(aBundle); }); 
}
void QueueFrame2::on(QueueManagerListener::BundleSize, const BundlePtr& aBundle) noexcept { 
	callAsync([=] { onBundleUpdated(aBundle); }); 
}
void QueueFrame2::on(QueueManagerListener::BundlePriority, const BundlePtr& aBundle) noexcept { 
	callAsync([=] { onBundleUpdated(aBundle); }); 
}
void QueueFrame2::on(QueueManagerListener::BundleStatusChanged, const BundlePtr& aBundle) noexcept { 
	callAsync([=] { onBundleUpdated(aBundle); }); 
}
void QueueFrame2::on(QueueManagerListener::BundleSources, const BundlePtr& aBundle) noexcept { 
	callAsync([=] { onBundleUpdated(aBundle); }); 
}

void QueueFrame2::on(QueueManagerListener::Removed, const QueueItemPtr& aQI, bool /*finished*/) noexcept{
	callAsync([=] { onQueueItemRemoved(aQI); });
}
void QueueFrame2::on(QueueManagerListener::SourcesUpdated, const QueueItemPtr& aQI) noexcept {
	callAsync([=] { onQueueItemUpdated(aQI); });
}
void QueueFrame2::on(QueueManagerListener::StatusUpdated, const QueueItemPtr& aQI) noexcept{
	callAsync([=] { onQueueItemUpdated(aQI); });
}

void QueueFrame2::on(DownloadManagerListener::BundleTick, const BundleList& tickBundles, uint64_t /*aTick*/) noexcept{
	for (auto& b : tickBundles) {
		callAsync([=] { onBundleUpdated(b); });
	}
}

//QueueItemInfo functions
int QueueFrame2::QueueItemInfo::getImageIndex() const {
	//should bundles have own icon and sub items the file type image?
	if (bundle)
		return bundle->isFileBundle() ? ResourceLoader::getIconIndex(Text::toT(bundle->getTarget())) : ResourceLoader::DIR_NORMAL;
	else 
		return ResourceLoader::getIconIndex(Text::toT(qi->getTarget()));

}

const tstring QueueFrame2::QueueItemInfo::getText(int col) const {
	if (bundle){
		switch (col) {
		case COLUMN_NAME: return Text::toT(bundle->getName());
		case COLUMN_SIZE: return (bundle->getSize() == -1) ? TSTRING(UNKNOWN) : Util::formatBytesW(bundle->getSize());
		case COLUMN_DOWNLOADED:
		{
			int64_t downloadedBytes = bundle->getDownloadedBytes();
			return (bundle->getSize() > 0) ? Util::formatBytesW(downloadedBytes) + _T(" (") + Util::toStringW((double)downloadedBytes*100.0 / (double)bundle->getSize()) + _T("%)") : Util::emptyStringT;
		}
		case COLUMN_PRIORITY:
		{
			tstring priority = Text::toT(AirUtil::getPrioText(bundle->getPriority()));
			if (bundle->getAutoPriority()) {
				priority += _T(" (") + TSTRING(AUTO) + _T(")");
			}
			return priority;
		}
		case COLUMN_SOURCES: return Util::toStringW(bundle->getSources().size()) + _T(" sources");
		case COLUMN_PATH: return Text::toT(bundle->getTarget());
		default: return Util::emptyStringT;
		}
	} else {
		switch (col) {
		case COLUMN_NAME: 
		{
			//show files in subdirectories as subdir/file.ext
			string name = qi->getTarget().substr(qi->getBundle()->getTarget().size(), qi->getTarget().size());
			return Text::toT(name);
		}
		case COLUMN_SIZE: return (qi->getSize() == -1) ? TSTRING(UNKNOWN) : Util::formatBytesW(qi->getSize());
		case COLUMN_DOWNLOADED:
		{
			int64_t downloadedBytes = qi->getDownloadedBytes();
			return (qi->getSize() > 0) ? Util::formatBytesW(downloadedBytes) + _T(" (") + Util::toStringW((double)downloadedBytes*100.0 / (double)qi->getSize()) + _T("%)") : Util::emptyStringT;
		}
		case COLUMN_PRIORITY:
		{
			tstring priority = Text::toT(AirUtil::getPrioText(qi->getPriority()));
			if (qi->getAutoPriority()) {
					priority += _T(" (") + TSTRING(AUTO) + _T(")");
			}
			return priority;
		}
		case COLUMN_SOURCES: return Util::toStringW(qi->getSources().size()) + _T(" sources");
		case COLUMN_PATH: return Text::toT(qi->getTarget());
		default: return Util::emptyStringT;
		}
	}

}

int QueueFrame2::QueueItemInfo::compareItems(const QueueItemInfo* a, const QueueItemInfo* b, int col) {
	//switch (col) {
	//case COLUMN_SIZE: return compare(a->bundle->getSize(), b->bundle->getSize());
	//case COLUMN_PRIORITY: return compare(static_cast<int>(a->bundle->getPriority()), static_cast<int>(b->bundle->getPriority()));
	//case COLUMN_DOWNLOADED: return compare(a->bundle->getDownloadedBytes(), b->bundle->getDownloadedBytes());
	//default: 
		return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str());
	//}
}
