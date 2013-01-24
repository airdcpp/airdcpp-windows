/* 
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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
#include "PrivateFrame.h"
#include "LineDlg.h"
#include "MainFrm.h"

#include "../client/AirUtil.h"
#include "../client/ShareManager.h"
#include "../client/ClientManager.h"
#include "../client/version.h"
#include "../client/DownloadManager.h"
#include "../client/ScopedFunctor.h"
#include "BarShader.h"

#define FILE_LIST_NAME _T("File Lists")
#define TEMP_NAME _T("Temp items")

int QueueFrame::columnIndexes[] = { COLUMN_TARGET, COLUMN_STATUS, COLUMN_SEGMENTS, COLUMN_SIZE, COLUMN_PROGRESS, COLUMN_DOWNLOADED, COLUMN_PRIORITY,
COLUMN_USERS, COLUMN_PATH, COLUMN_EXACT_SIZE, COLUMN_ERRORS, COLUMN_ADDED, COLUMN_TTH };

int QueueFrame::columnSizes[] = { 200, 300, 70, 75, 100, 120, 75, 200, 200, 75, 200, 100, 125 };

static ResourceManager::Strings columnNames[] = { ResourceManager::FILENAME, ResourceManager::STATUS, ResourceManager::SEGMENTS, ResourceManager::SIZE, 
ResourceManager::DOWNLOADED_PARTS, ResourceManager::DOWNLOADED,
ResourceManager::PRIORITY, ResourceManager::USERS, ResourceManager::PATH, ResourceManager::EXACT_SIZE, ResourceManager::ERRORS,
ResourceManager::ADDED, ResourceManager::TTH_ROOT };

LRESULT QueueFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	showTree = SETTING(QUEUEFRAME_SHOW_TREE);
	
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	
	ctrlQueue.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_QUEUE);
	ctrlQueue.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

	ctrlDirs.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
		TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP | TVS_TRACKSELECT, 
		 WS_EX_CLIENTEDGE, IDC_DIRECTORIES);
	
	if(SETTING(USE_EXPLORER_THEME)) {
		SetWindowTheme(ctrlDirs.m_hWnd, L"explorer", NULL);
	}

	ctrlDirs.SetImageList(ResourceLoader::fileImages, TVSIL_NORMAL);
	ctrlQueue.SetImageList(ResourceLoader::fileImages, LVSIL_SMALL);
	
	t_bVertical = !SETTING(HORIZONTAL_QUEUE);
	m_nProportionalPos = SETTING(QUEUE_SPLITTER_POS);
	SetSplitterPanes(ctrlDirs.m_hWnd, ctrlQueue.m_hWnd);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(QUEUEFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(QUEUEFRAME_WIDTHS), COLUMN_LAST);
	
	for(uint8_t j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_DOWNLOADED || j == COLUMN_EXACT_SIZE|| j == COLUMN_SEGMENTS) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlQueue.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlQueue.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlQueue.setSortColumn(COLUMN_TARGET);
	ctrlQueue.setVisible(SETTING(QUEUEFRAME_VISIBLE));
	
	ctrlQueue.SetBkColor(WinUtil::bgColor);
	ctrlQueue.SetTextBkColor(WinUtil::bgColor);
	ctrlQueue.SetTextColor(WinUtil::textColor);
	ctrlQueue.setFlickerFree(WinUtil::bgBrush);

	ctrlDirs.SetBkColor(WinUtil::bgColor);
	ctrlDirs.SetTextColor(WinUtil::textColor);

	ctrlShowTree.Create(ctrlStatus.m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlShowTree.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowTree.SetCheck(showTree);
	showTreeContainer.SubclassWindow(ctrlShowTree.m_hWnd);

	CRect rc(SETTING(QUEUE_LEFT), SETTING(QUEUE_TOP), SETTING(QUEUE_RIGHT), SETTING(QUEUE_BOTTOM));
	if(! (rc.top == 0 && rc.bottom == 0 && rc.left == 0 && rc.right == 0) )
		MoveWindow(rc, TRUE);

	QueueManager::getInstance()->readLockedOperation([this](const QueueItem::StringMap& qsm) { addQueueList(qsm); });
	//auto queue = QueueManager::getInstance()->getQueue();
	//addQueueList(queue);
	//concurrency::reader_writer_lock
	//startupTask.

	QueueManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	memzero(statusSizes, sizeof(statusSizes));
	statusSizes[0] = 16;
	ctrlStatus.SetParts(6, statusSizes);
	updateStatus();

	WinUtil::SetIcon(m_hWnd, IDI_QUEUE);

	bHandled = FALSE;
	return 1;
}


QueueFrame::~QueueFrame() { }

int QueueFrame::QueueItemInfo::compareItems(const QueueItemInfo* a, const QueueItemInfo* b, int col) {
	switch(col) {
		case COLUMN_SIZE: case COLUMN_EXACT_SIZE: return compare(a->getSize(), b->getSize());
		case COLUMN_PRIORITY: return compare(static_cast<int>(a->getPriority()), static_cast<int>(b->getPriority()));
		case COLUMN_DOWNLOADED: return compare(a->getDownloadedBytes(), b->getDownloadedBytes());
		case COLUMN_ADDED: return compare(a->getAdded(), b->getAdded());
		default: return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str());
	}
}

int64_t QueueFrame::QueueItemInfo::getDownloadedBytes() const { 
	return QueueManager::getInstance()->getDownloadedBytes(qi);
}

bool QueueFrame::QueueItemInfo::isWaiting() const { 
	return QueueManager::getInstance()->isWaiting(qi); 
}

bool QueueFrame::QueueItemInfo::isFinished() const { 
	return QueueManager::getInstance()->isFinished(qi); 
}

int QueueFrame::QueueItemInfo::getImageIndex() const { return ResourceLoader::getIconIndex(Text::toT(getTarget())); }

const tstring QueueFrame::QueueItemInfo::getText(int col) const {
	switch(col) {
		case COLUMN_TARGET: return Text::toT(Util::getFileName(getTarget()));
		case COLUMN_STATUS: {
			int online = 0;
			QueueItem::SourceList sources = QueueManager::getInstance()->getSources(qi);
			for(const auto& s: sources) {
				if(s.getUser().user->isOnline())
					online++;
			}

			if(isFinished()) {
				return TSTRING(DOWNLOAD_FINISHED_IDLE);
			} else if(isWaiting()) {
				if(online > 0) {
					size_t size = QueueManager::getInstance()->getSourcesCount(qi);
					if(size == 1) {
						return TSTRING(WAITING_USER_ONLINE);
					} else {
						return TSTRING_F(WAITING_USERS_ONLINE, online % size);
					}
				} else {
					size_t size = QueueManager::getInstance()->getSourcesCount(qi);
					if(size == 0) {
						return TSTRING(NO_USERS_TO_DOWNLOAD_FROM);
					} else if(size == 1) {
						return TSTRING(USER_OFFLINE);
					} else if(size == 2) {
						return TSTRING(BOTH_USERS_OFFLINE);
					} else {
						return TSTRING_F(ALL_USERS_OFFLINE, size);
					}
				}
			} else {
				size_t size = QueueManager::getInstance()->getSourcesCount(qi);
				if(size == 1) {
					return TSTRING(USER_ONLINE);
				} else {
					return TSTRING_F(USERS_ONLINE, online % size);
				}
			}
		}
		case COLUMN_SEGMENTS: {
			int64_t min_seg_size = (SETTING(MIN_SEGMENT_SIZE)*1024);

			auto qm = QueueManager::getInstance();
			qm->lockRead();
			ScopedFunctor([qm] { qm->unlockRead(); });
			if(getSize() < min_seg_size){
				return Util::toStringW(qi->getDownloads().size()) + _T("/") + Util::toStringW(1);
			} else {
				return Util::toStringW(qi->getDownloads().size()) + _T("/") + Util::toStringW(getQueueItem()->getMaxSegments());
			}
		}
		case COLUMN_SIZE: return (getSize() == -1) ? TSTRING(UNKNOWN) : Util::formatBytesW(getSize());
		case COLUMN_DOWNLOADED: {
			int64_t downloadedBytes = getDownloadedBytes();
			return (getSize() > 0) ? Util::formatBytesW(downloadedBytes) + _T(" (") + Util::toStringW((double)downloadedBytes*100.0/(double)getSize()) + _T("%)") : Util::emptyStringT;
		}
		case COLUMN_PRIORITY: {
			tstring priority;
			switch(getPriority()) {
				case QueueItem::PAUSED: priority = TSTRING(PAUSED); break;
				case QueueItem::LOWEST: priority = TSTRING(LOWEST); break;
				case QueueItem::LOW: priority = TSTRING(LOW); break;
				case QueueItem::NORMAL: priority = TSTRING(NORMAL); break;
				case QueueItem::HIGH: priority = TSTRING(HIGH); break;
				case QueueItem::HIGHEST: priority = TSTRING(HIGHEST); break;
				default: dcassert(0); break;
			}
			if(getAutoPriority()) {
				priority += _T(" (") + TSTRING(AUTO) + _T(")");
			}
			return priority;
		}
		case COLUMN_USERS: {
			tstring tmp;

			QueueItem::SourceList sources = QueueManager::getInstance()->getSources(qi);
			for(const auto& s: sources) {
				if(tmp.size() > 0)
					tmp += _T(", ");

				tmp += WinUtil::getNicks(s.getUser());
			}
			return tmp.empty() ? TSTRING(NO_USERS) : tmp;
		}
		case COLUMN_PATH: return Text::toT(getPath());
		case COLUMN_EXACT_SIZE: return (getSize() == -1) ? TSTRING(UNKNOWN) : Util::formatExactSize(getSize());
		case COLUMN_ERRORS: {
			tstring tmp;
			QueueItem::SourceList badSources = QueueManager::getInstance()->getBadSources(qi);
			for(auto& bs: badSources) {
				if(!bs.isSet(QueueItem::Source::FLAG_REMOVED)) {
				if(tmp.size() > 0)
					tmp += _T(", ");
					tmp += WinUtil::getNicks(bs.getUser());
					tmp += _T(" (");
					if(bs.isSet(QueueItem::Source::FLAG_FILE_NOT_AVAILABLE)) {
						tmp += TSTRING(FILE_NOT_AVAILABLE);
					}  else if(bs.isSet(QueueItem::Source::FLAG_BAD_TREE)) {
						tmp += TSTRING(INVALID_TREE);
					} else if(bs.isSet(QueueItem::Source::FLAG_SLOW_SOURCE)) {
						tmp += TSTRING(SLOW_USER);
					} else if(bs.isSet(QueueItem::Source::FLAG_NO_TTHF)) {
						tmp += TSTRING(SOURCE_TOO_OLD);						
					} else if(bs.isSet(QueueItem::Source::FLAG_NO_NEED_PARTS)) {
						tmp += TSTRING(NO_NEEDED_PART);
					} else if(bs.isSet(QueueItem::Source::FLAG_UNTRUSTED)) {
						tmp += TSTRING(CERTIFICATE_NOT_TRUSTED);
					}
					tmp += _T(')');
				}
			}
			return tmp.empty() ? TSTRING(NO_ERRORS) : tmp;
		}
		case COLUMN_ADDED: return Text::toT(Util::formatTime("%Y-%m-%d %H:%M", getAdded()));
		case COLUMN_TTH: 
			return (qi->isSet(QueueItem::FLAG_USER_LIST) || SettingsManager::lanMode) ? Util::emptyStringT : Text::toT(getTTH().toBase32());

		default: return Util::emptyStringT;
	}
}

void QueueFrame::on(QueueManagerListener::Added, QueueItemPtr& aQI) {
	QueueItemInfo* ii = new QueueItemInfo(aQI);

	speak(ADD_ITEM,	new QueueItemInfoTask(ii));
	speak(UPDATE_STATUS_ITEMS, new StringTask(aQI->getTarget()));
}

void QueueFrame::addQueueItem(QueueItemInfo* ii, bool noSort) {
	if(!ii->isSet(QueueItem::FLAG_USER_LIST)) {
		queueSize+=ii->getSize();
	}
	queueItems++;
	dirty = true;
	const string& dir = ii->getPath();
	bool updateDir = (directories.find(dir) == directories.end());
	directories.emplace(dir, ii);
	BundlePtr b = ii->getBundle();

	if (!b) {
		addItemDir(ii->isSet(QueueItem::FLAG_USER_LIST));
	} else if (updateDir) {
		//make sure that the bundle dir is being added
		if (stricmp(dir, b->getTarget()) != 0 && !b->isFileBundle() && bundleMap.find(b->getTarget()) == bundleMap.end() && directories.find(b->getTarget()) == directories.end()) {
			addBundleDir(b->getTarget(), b);
		}
		addBundleDir(dir, b);
	} else if (b->isFileBundle()) {
		//even though the folder exists, we need to update the name and add the bundle in the tree
		string::size_type i = 0;

		HTREEITEM next = ctrlDirs.GetRootItem();
		HTREEITEM parent = NULL;
	
		while(i < dir.length()) {
			while(next != NULL) {
				if(next != fileLists && next != tempItems) {
					const string& n = getDir(next);
					if(strnicmp(n.c_str()+i, dir.c_str()+i, n.length()-i) == 0) {
						// Match!
						parent = next;
						((DirItemInfo*)ctrlDirs.GetItemData(next))->addBundle(b);
						next = ctrlDirs.GetChildItem(next);
						i = n.length();
						break;
					}
				}
				next = ctrlDirs.GetNextSiblingItem(next);
			}
			if(next == NULL)
				break;
		}

		//we have the dir now
		tstring text = ((DirItemInfo*)ctrlDirs.GetItemData(parent))->getBundleName(false);
		ctrlDirs.SetItemText(parent, const_cast<TCHAR*>(text.c_str()));
		//ctrlDirs.SetItemState(parent, TVIS_BOLD, TVIS_BOLD);
	}

	if(!showTree || isCurDir(dir)) {
		if(noSort)
			ctrlQueue.insertItem(ctrlQueue.GetItemCount(), ii, ResourceLoader::getIconIndex(Text::toT(ii->getTarget())));
		else
			ctrlQueue.insertItem(ii, ResourceLoader::getIconIndex(Text::toT(ii->getTarget())));
	}
}

QueueFrame::QueueItemInfo* QueueFrame::getItemInfo(const string& target) const {
	string fileName = Util::getFileName(target);

	auto items = directories.equal_range(Util::getFilePath(target));

	for(auto i = items.first; i != items.second; ++i) {
		if(compare(i->second->getFileName(), fileName) == 0) {
			return i->second;
		}
	}
	return nullptr;
}

void QueueFrame::addQueueList(const QueueItem::StringMap& li) {
	ctrlQueue.SetRedraw(FALSE);
	ctrlDirs.SetRedraw(FALSE);
	for(auto& qi: li | map_values) {
		if (qi->isSet(QueueItem::FLAG_FINISHED) && !SETTING(KEEP_FINISHED_FILES)) {
			continue;
		}
		QueueItemInfo* ii = new QueueItemInfo(qi);
		addQueueItem(ii, true);
	}
	ctrlQueue.resort();
	ctrlQueue.SetRedraw(TRUE);
	ctrlDirs.SetRedraw(TRUE);
	ctrlDirs.Invalidate();
}

HTREEITEM QueueFrame::addItemDir(bool isFileList) {
	TVINSERTSTRUCT tvi;
	tvi.hInsertAfter = TVI_SORT;
	tvi.item.mask = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT;
	tvi.item.iImage = tvi.item.iSelectedImage = ResourceLoader::DIR_NORMAL;

	if(SETTING(EXPAND_QUEUE)) {
		tvi.itemex.mask |= TVIF_STATE;
		tvi.itemex.state = TVIS_EXPANDED;
		tvi.itemex.stateMask = TVIS_EXPANDED;
	}

	if(isFileList) {
		// We assume we haven't added it yet, and that all filelists go to the same
		// directory...
		if(fileLists == NULL) {
			tvi.hParent = NULL;
			tvi.item.pszText = FILE_LIST_NAME;
			tvi.item.lParam = (LPARAM)new DirItemInfo(Util::getListPath());
			fileLists = ctrlDirs.InsertItem(&tvi);
		}
		return fileLists;
	} else {
		if (tempItems == NULL) {
			dcassert(tempItems == NULL);
			tvi.hParent = NULL;
			tvi.item.pszText = TEMP_NAME;
			tvi.item.lParam = (LPARAM)new DirItemInfo(Util::getOpenPath());
			tempItems = ctrlDirs.InsertItem(&tvi);
		}
		return tempItems;
	}
}

HTREEITEM QueueFrame::createDir(TVINSERTSTRUCT& tvi, const string&& dir, const BundlePtr& aBundle, HTREEITEM parent, bool subDir /*false*/) {
	bool resetFormating = false;
	bool mainBundle = false;
	DirItemInfo* dii = new DirItemInfo(dir, aBundle);
	tvi.item.lParam = (LPARAM)dii;
	tstring name;
	if (stricmp(dir, Util::getFilePath(aBundle->getTarget())) == 0) {
		if (aBundle->isFileBundle()) {
			name = dii->getBundleName(false);
		} else {
			mainBundle=true;
			name = aBundle->getBundleText();
		}
		tvi.item.state = TVIS_BOLD;
		tvi.item.stateMask = TVIS_BOLD;
		resetFormating = true;
	} else {
		name = Text::toT(dir);
		if (subDir) {
			name = Util::getLastDir(name);
		}
	}
	tvi.item.pszText = const_cast<TCHAR*>(name.c_str());
	tvi.hParent = parent;
	HTREEITEM ret = ctrlDirs.InsertItem(&tvi);
	if (mainBundle) {
		//LogManager::getInstance()->message("MAINBUNDLE ADD: " + aBundle->getName() + ", dir: " + ((DirItemInfo*)ctrlDirs.GetItemData(parent))->getDir());
		dcassert(bundleMap.find(dir) == bundleMap.end());
		bundleMap.emplace(aBundle->getTarget(), ret);
		mainBundle = false;
	}

	if (resetFormating) {
		//in case we need to add more dirs
		tvi.item.state = 0;
		tvi.item.stateMask = TVIS_BOLD;
	}
	return ret;
}

HTREEITEM QueueFrame::createSplitDir(TVINSERTSTRUCT& tvi, const string&& dir, HTREEITEM parent, DirItemInfo* dii, bool subDir /*false*/) {
	/* Bundles */
	DirItemInfo* diiNew = new DirItemInfo(dir);
	dcassert(!dii->getBundles().empty());
	tstring name;
	bool updateMap = false;
	bool setBold = false;
	//check if we need to modify the bundlemap
	for (auto& bp: dii->getBundles()) {
		diiNew->addBundle(bp.second);
		if (stricmp(dir, bp.first) == 0) {
			name = bp.second->getBundleText();
			updateMap = true;
			setBold=true;
			break;
		} else if (bp.second->isFileBundle() && (Util::getFilePath(bp.second->getTarget()) == dir)) {
			name = dii->getBundleName(false);
			setBold=true;
			break;
		}
	}

	if (setBold) {
		tvi.itemex.state = TVIS_BOLD;
		tvi.itemex.stateMask = TVIS_BOLD;
	} else {
		name = Text::toT(dir);
		if (subDir) {
			name = Util::getLastDir(name);
		}
	}

	tvi.item.pszText = const_cast<TCHAR*>(name.c_str());
	tvi.hParent = parent;
	tvi.item.lParam = (LPARAM)diiNew;
	HTREEITEM ret = ctrlDirs.InsertItem(&tvi);
	if (updateMap) {
		auto s = bundleMap.find(dir);
		dcassert(s != bundleMap.end());
		s->second = ret;
	}
	if (setBold) {
		tvi.item.state = 0;
		tvi.item.stateMask = TVIS_BOLD;
	}
	return ret;
}

HTREEITEM QueueFrame::addBundleDir(const string& dir, const BundlePtr& aBundle, HTREEITEM startAt /* = NULL */) {
	TVINSERTSTRUCT tvi;
	tvi.hInsertAfter = TVI_SORT;
	tvi.item.mask = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT;
	tvi.item.iImage = tvi.item.iSelectedImage = ResourceLoader::DIR_NORMAL;

	if(SETTING(EXPAND_QUEUE)) {
		tvi.itemex.mask |= TVIF_STATE;
		tvi.itemex.state = TVIS_EXPANDED;
		tvi.itemex.stateMask = TVIS_EXPANDED;
	}

	// We have to find the last available tree item and then see...
	string::size_type i = 0;
	string::size_type j;

	HTREEITEM next = NULL;
	HTREEITEM parent = NULL;

	if(startAt == NULL) {
		// First find the correct drive letter
		dcassert(dir[1] == ':');
		dcassert(dir[2] == '\\');

		next = ctrlDirs.GetRootItem();

		while(next != NULL) {
			if(next != fileLists && next != tempItems) {
				if(strnicmp(((DirItemInfo*)ctrlDirs.GetItemData(next))->getDir(), dir, 3) == 0) {
					break;
				}
			}
			next = ctrlDirs.GetNextSiblingItem(next);
		}

		if(next == NULL) {
			// First addition, set commonStart to the dir minus the last part...
			i = dir.rfind('\\', dir.length()-2);
			next = createDir(tvi, dir.substr(0, (i != string::npos ? i+1 : dir.length())), aBundle, NULL);
		} 

		// Ok, next now points to x:\... find how much is common

		const string& rootStr = ((DirItemInfo*)ctrlDirs.GetItemData(next))->getDir();

		i = 0;

		for(;;) {
			j = dir.find('\\', i);
			if(j == string::npos)
				break;
			if(strnicmp(dir.c_str() + i, rootStr.c_str() + i, j - i + 1) != 0)
				break;
			i = j + 1;
		}

		if(i < rootStr.length()) {
			HTREEITEM oldRoot = next;

			// Create a new root
			DirItemInfo* dii = ((DirItemInfo*)ctrlDirs.GetItemData(next));
			HTREEITEM newRoot = next = createSplitDir(tvi, rootStr.substr(0, i), NULL, dii, false);

			parent = addBundleDir(rootStr, aBundle, newRoot);
			((DirItemInfo*)ctrlDirs.GetItemData(newRoot))->addBundle(aBundle);

			next = ctrlDirs.GetChildItem(oldRoot);
			while(next != NULL) {
				moveNode(next, parent);
				next = ctrlDirs.GetChildItem(oldRoot);
			}
			delete dii;
			ctrlDirs.DeleteItem(oldRoot);
			parent = newRoot;
		} else {
			// Use this root as parent
			parent = next;
			((DirItemInfo*)ctrlDirs.GetItemData(parent))->addBundle(aBundle);
			if (aBundle->isFileBundle() && stricmp(getDir(parent), Util::getFilePath(aBundle->getTarget())) == 0) {
				tstring text = ((DirItemInfo*)ctrlDirs.GetItemData(next))->getBundleName(false);
				ctrlDirs.SetItemText(next, const_cast<TCHAR*>(text.c_str()));
				ctrlDirs.SetItemState(next, TVIS_BOLD, TVIS_BOLD);
			}
			next = ctrlDirs.GetChildItem(parent);
		}
	} else {
		parent = startAt;
		next = ctrlDirs.GetChildItem(parent);
		i = getDir(parent).length();
		dcassert(strnicmp(getDir(parent), dir, getDir(parent).length()) == 0);
	}

	while( i < dir.length() ) {
		while(next != NULL) {
			if(next != fileLists && next != tempItems) {
				const string& n = getDir(next);
				if(strncmp(n.c_str()+i, dir.c_str()+i, n.length()-i) == 0) {
					// Found a part, we assume it's the best one we can find...
					i = n.length();
					DirItemInfo* dii = (DirItemInfo*)ctrlDirs.GetItemData(next);
					if (dii->getBundles().empty() && !aBundle->isFileBundle() && compare(n, aBundle->getTarget()) == 0) {
						ctrlDirs.SetItemText(next, aBundle->getBundleText().c_str());
						ctrlDirs.SetItemState(next, TVIS_BOLD, TVIS_BOLD);
						dcassert(bundleMap.find(aBundle->getTarget()) == bundleMap.end());
						bundleMap[aBundle->getTarget()] = next;
					}

					dii->addBundle(aBundle);
					if (aBundle->isFileBundle() && stricmp(n, Util::getFilePath(aBundle->getTarget())) == 0) {
						tstring text = dii->getBundleName(false);
						ctrlDirs.SetItemText(next, const_cast<TCHAR*>(text.c_str()));
						ctrlDirs.SetItemState(next, TVIS_BOLD, TVIS_BOLD);
					}

					parent = next;
					next = ctrlDirs.GetChildItem(next);
					break;
				}
			}
			next = ctrlDirs.GetNextSiblingItem(next);
		}

		if(next == NULL) {
			// We didn't find it, add...
			j = dir.find('\\', i);
			dcassert(j != string::npos);
			if (startAt != NULL) {
				DirItemInfo* dii = ((DirItemInfo*)ctrlDirs.GetItemData(startAt));
				//dii->addBundle(aBundle);
				parent = createSplitDir(tvi, dir.substr(0, j+1), parent, dii, true);
				//delete dii;
			} else {
				parent = createDir(tvi, dir.substr(0, j+1), aBundle, parent, true);
			}

			i = j + 1;
		}
	}

	return parent;
}

QueueFrame::DirItemInfo::DirItemInfo(const string& dir, BundlePtr aBundle) : directory(dir)	{
	//LogManager::getInstance()->message("Create DII: " + dir);
	if (aBundle) {
		bundles.emplace_back(aBundle->getTarget(), aBundle);
	}
}

QueueFrame::DirItemInfo::~DirItemInfo() { 
	//LogManager::getInstance()->message("Remove DII: " + directory);
}

void QueueFrame::DirItemInfo::addBundle(BundlePtr aBundle) {
	if (find_if(bundles.begin(), bundles.end(), CompareSecond<string, BundlePtr>(aBundle)) == bundles.end()) {
		//LogManager::getInstance()->message("BUNDLE " + aBundle->getName() + " added for dir " + this->getDir());
		bundles.emplace_back(aBundle->getTarget(), aBundle);
	}
}

void QueueFrame::DirItemInfo::removeBundle(const string& aDir) {
	auto s = find_if(bundles.begin(), bundles.end(), CompareFirst<string, BundlePtr>(aDir));
	dcassert(s != bundles.end());
	if (s != bundles.end()) {
		bundles.erase(s);
	}
	dcassert(find_if(bundles.begin(), bundles.end(), CompareFirst<string, BundlePtr>(aDir)) == bundles.end());
}

int QueueFrame::DirItemInfo::countFileBundles() {
	int ret = 0;
	for (auto b: bundles | map_values) {
		if (b->isFileBundle())
			ret++;
	}
	return ret;
}

tstring QueueFrame::DirItemInfo::getBundleName(bool remove) {
	int bundles = remove ? countFileBundles()-1 : countFileBundles();
	dcassert(bundles > 0);
	return Text::toT(Util::getLastDir(directory) + " (" + Util::toString(bundles) + " file bundles)");
}


void QueueFrame::removeItemDir(bool isFileList) {
	if(isFileList) {
		dcassert(fileLists != NULL);
		delete (DirItemInfo*)ctrlDirs.GetItemData(fileLists);
		ctrlDirs.DeleteItem(fileLists);
		fileLists = NULL;
	} else {
		dcassert(tempItems != NULL);
		delete (DirItemInfo*)ctrlDirs.GetItemData(tempItems);
		ctrlDirs.DeleteItem(tempItems);
		tempItems = NULL;
	} 
}

void QueueFrame::removeBundleDir(const string& dir) {
	// First, find the last name available
	string::size_type i = 0;

	HTREEITEM next = ctrlDirs.GetRootItem();
	HTREEITEM parent = NULL;
	
	while(i < dir.length()) {
		while(next != NULL) {
			if(next != fileLists && next != tempItems) {
				const string& n = getDir(next);
				if(strncmp(n.c_str()+i, dir.c_str()+i, n.length()-i) == 0) {
					// Match!
					parent = next;
					next = ctrlDirs.GetChildItem(next);
					i = n.length();
					break;
				}
			}
			next = ctrlDirs.GetNextSiblingItem(next);
		}
		if(next == NULL)
			break;
	}

	next = parent;

	while((ctrlDirs.GetChildItem(next) == NULL) && (directories.find(getDir(next)) == directories.end())) {
		delete (DirItemInfo*)ctrlDirs.GetItemData(next);
		parent = ctrlDirs.GetParentItem(next);
		
		ctrlDirs.DeleteItem(next);
		if(parent == NULL)
			break;
		next = parent;
	}
}

void QueueFrame::removeBundle(const string& aDir, bool isFileBundle) {
	//dcassert(aBundle);
	if (!isFileBundle) {
		dcassert(bundleMap.find(aDir) != bundleMap.end());
		bundleMap.erase(aDir);
	}
	const string& dir = isFileBundle ? Util::getFilePath(aDir) : aDir;
	// First, find the last name available
	string::size_type i = 0;

	HTREEITEM next = ctrlDirs.GetRootItem();
	//HTREEITEM parent = NULL;
	
	while(i < dir.length()) {
		while(next != NULL) {
			if(next != fileLists && next != tempItems) {
				const string& n = getDir(next);
				string test1 = n.c_str()+i;
				string test2 = dir.c_str()+i;
				if(strncmp(n.c_str()+i, dir.c_str()+i, n.length()-i) == 0) {
					// Match!
					DirItemInfo* dii = (DirItemInfo*)ctrlDirs.GetItemData(next);
					if (isFileBundle && stricmp(n, Util::getFilePath(aDir)) == 0) {
						tstring text;
						if (dii->countFileBundles() == 1) {
							//reset the item
							text = Text::toT(Util::getLastDir(dir));
							ctrlDirs.SetItemState(next, 0, TVIS_BOLD);
						} else {
							//update the number of file bundles
							text = dii->getBundleName(true);
						}
						ctrlDirs.SetItemText(next, const_cast<TCHAR*>(text.c_str()));
					}

					dii->removeBundle(aDir);
					if (dii->getBundles().empty() && !isFileBundle) {
						ctrlDirs.SetItemText(next, Text::toT(Util::getLastDir(dii->getDir())).c_str());
						ctrlDirs.SetItemState(next, 0, TVIS_BOLD);
					}
					next = ctrlDirs.GetChildItem(next);
					i = n.length();
					break;
				}
			}
			next = ctrlDirs.GetNextSiblingItem(next);
		}
		if(next == NULL)
			break;
	}
}

void QueueFrame::on(QueueManagerListener::Removed, const QueueItemPtr& aQI, bool updateStatus) {
	speak(REMOVE_ITEM, new StringTask(aQI->getTarget()));
	if (updateStatus)
		speak(UPDATE_STATUS_ITEMS, new StringTask(aQI->getTarget()));
}

void QueueFrame::on(QueueManagerListener::Moved, const QueueItemPtr&, const string& oldTarget) {
	speak(REMOVE_ITEM, new StringTask(oldTarget));
}

void QueueFrame::on(QueueManagerListener::BundleMoved, const BundlePtr& aBundle) {
	for(auto& qi: aBundle->getQueueItems()) 
		speak(REMOVE_ITEM, new StringTask(qi->getTarget()));

	speak(REMOVE_BUNDLE, new DirItemInfoTask(new DirItemInfo(aBundle->getTarget(), aBundle)));
}

void QueueFrame::on(QueueManagerListener::BundleMerged, const BundlePtr& aBundle, const string& oldTarget) noexcept {
	speak(REMOVE_BUNDLE, new DirItemInfoTask(new DirItemInfo(oldTarget, aBundle)));
	speak(ADD_BUNDLE, new DirItemInfoTask(new DirItemInfo(aBundle->getTarget(), aBundle)));
}

void QueueFrame::on(QueueManagerListener::SourcesUpdated, const QueueItemPtr& aQI) {
	speak(UPDATE_ITEM, new UpdateTask(*aQI));
}

void QueueFrame::on(DownloadManagerListener::BundleTick, const BundleList& tickBundles, uint64_t /*aTick*/) noexcept {
	for (auto& b: tickBundles) {
		if (b->isFileBundle()) {
			return;
		}
		speak(UPDATE_BUNDLE, new DirItemInfoTask(new DirItemInfo(b->getTarget(), b)));
	}
}

void QueueFrame::on(QueueManagerListener::BundlePriority, const BundlePtr& aBundle) {
	if (aBundle->isFileBundle()) {
		return;
	}
	speak(UPDATE_BUNDLE, new DirItemInfoTask(new DirItemInfo(aBundle->getTarget(), aBundle)));
}

void QueueFrame::on(QueueManagerListener::BundleAdded, const BundlePtr& aBundle) {
	for (auto& qi: aBundle->getQueueItems()) {
		QueueItemInfo* ii = new QueueItemInfo(qi);
		speak(ADD_ITEM,	new QueueItemInfoTask(ii));
	}
	speak(ADD_BUNDLE, new DirItemInfoTask(new DirItemInfo(aBundle->getTarget(), aBundle)));
}

void QueueFrame::on(QueueManagerListener::BundleRemoved, const BundlePtr& aBundle) {
	//for_each(aBundle->getQueueItems().begin(), aBundle->getQueueItems().end(), [&](QueueItemPtr qi) { speak(REMOVE_ITEM, new StringTask(qi->getTarget())); });
	speak(REMOVE_BUNDLE, new DirItemInfoTask(new DirItemInfo(aBundle->getTarget(), aBundle)));
}

bool QueueFrame::isCurDir(const string& aDir) const { 
	return stricmp(curDir, aDir) == 0; 
}

LRESULT QueueFrame::onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	TaskQueue::List t;

	tasks.get(t);
	spoken = false;

	for(auto& ti: t) {
		if(ti.first == ADD_ITEM) {
			auto &iit = static_cast<QueueItemInfoTask&>(*ti.second);
			
			dcassert(ctrlQueue.findItem(iit.ii) == -1);
			addQueueItem(iit.ii, false);
			//updateStatus();
		} else if(ti.first == ADD_BUNDLE) {
			auto &ui = static_cast<DirItemInfoTask&>(*ti.second);
			//bundleMap.insert(make_pair(ui.ii->getDir(), ui.ii));
			updateStatus();
			delete ui.ii;
		} else if(ti.first == REMOVE_ITEM) {
			auto &target = static_cast<StringTask&>(*ti.second);

			auto fileName = Util::getFileName(target.str);
			auto dirs = directories.equal_range(Util::getFilePath(target.str));

			auto j = boost::find_if(dirs | map_values, [&fileName](const QueueItemInfo* qii) { return compare(qii->getFileName(), fileName) == 0; });
			if (j.base() == dirs.second) {
				dcassert(0);
				continue;
			}

			const QueueItemInfo* ii = *j;
			bool removeTree = distance(dirs.first, dirs.second) == 1;


			directories.erase(j.base());
			
			if(!showTree || isCurDir(Util::getFilePath(target.str))) {
				dcassert(ctrlQueue.findItem(ii) != -1);
				ctrlQueue.deleteItem(ii);
			}
			
			bool userList = ii->isSet(QueueItem::FLAG_USER_LIST);
			
			if(!userList) {
				queueSize-=ii->getSize();
				dcassert(queueSize >= 0);
			}
			queueItems--;
			dcassert(queueItems >= 0);

			if(removeTree) {
				if (!ii->getBundle()) {
					removeItemDir(userList);
				} else {
					removeBundleDir(Util::getFilePath(target.str));
				}

				if(isCurDir(Util::getFilePath(target.str)))
					curDir.clear();
			}
			
			delete ii;
			if (!userList && SETTING(BOLD_QUEUE)) {
				setDirty();
			}
			dirty = true;
		} else if(ti.first == REMOVE_BUNDLE) {
			auto &ui = static_cast<DirItemInfoTask&>(*ti.second);
			removeBundle(ui.ii->getDir(), ui.ii->getBundles().front().second->isFileBundle());
			updateStatus();
			delete ui.ii;
		} else if(ti.first == UPDATE_BUNDLE) {
			auto &ui = static_cast<DirItemInfoTask&>(*ti.second);
			const string& path = ui.ii->getDir();
			auto i = bundleMap.find(path);
			if (i != bundleMap.end()) {
				ctrlDirs.SetItemText(i->second, const_cast<TCHAR*>(ui.ii->getBundles().front().second->getBundleText().c_str()));
			}
			delete ui.ii;
		} else if(ti.first == UPDATE_ITEM) {
			auto &ui = static_cast<UpdateTask&>(*ti.second);
            QueueItemInfo* ii = getItemInfo(ui.target);
			if(!ii)
				continue;

			if(!showTree || isCurDir(ii->getPath())) {
				int pos = ctrlQueue.findItem(ii);
				if(pos != -1) {
					ctrlQueue.updateItem(pos, COLUMN_SEGMENTS);
					ctrlQueue.updateItem(pos, COLUMN_PROGRESS);
					ctrlQueue.updateItem(pos, COLUMN_PRIORITY);
					ctrlQueue.updateItem(pos, COLUMN_USERS);
					ctrlQueue.updateItem(pos, COLUMN_ERRORS);
					ctrlQueue.updateItem(pos, COLUMN_STATUS);
					ctrlQueue.updateItem(pos, COLUMN_DOWNLOADED);
				}
			}
		} else if(ti.first == UPDATE_STATUS) {
			auto &status = static_cast<StringTask&>(*ti.second);
			ctrlStatus.SetText(1, Text::toT(status.str).c_str());
		} else if(ti.first == UPDATE_STATUS_ITEMS) {
			updateStatus();
		}
	}

	return 0;
}

void QueueFrame::removeSelected() {
	if(WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_QUEUE_REMOVAL, TSTRING(REALLY_REMOVE)))
		ctrlQueue.forEachSelected(&QueueItemInfo::remove);
}
	
void QueueFrame::removeSelectedDir() {
	if(ctrlDirs.GetSelectedItem() == NULL)
		return;

	dcassert(!curDir.empty());
	tstring name = Text::toT(curDir);
	if (isCurDir(Util::getListPath()) || isCurDir(Util::getOpenPath())) {
		removeDir(ctrlDirs.GetSelectedItem());
		return;
	}

	int finishedFiles = 0, fileBundles = 0;
	string tmp;
	BundleList bundles;
	QueueManager::getInstance()->getBundleInfo(curDir, bundles, finishedFiles, fileBundles);
	int dirBundles = bundles.size() - fileBundles;
	bool moveFinished = false;
	
	if (bundles.size() == 1) {
		BundlePtr bundle = bundles.front();
		if (stricmp(bundle->getTarget(), curDir) != 0) {
			tmp = STRING_F(CONFIRM_REMOVE_DIR_BUNDLE_PART, Util::getLastDir(curDir).c_str() % bundle->getName().c_str());
			if(!WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_QUEUE_REMOVAL, Text::toT(tmp))) {
				return;
			} else {
				if (finishedFiles > 0) {
					tmp = STRING_F(CONFIRM_REMOVE_DIR_FINISHED_BUNDLE_PART, finishedFiles);
					if(MessageBox(Text::toT(tmp).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
						moveFinished = true;
					}
				}
			}
		} else {
			tmp = STRING_F(CONFIRM_REMOVE_DIR_BUNDLE, bundle->getName().c_str());
			if(!WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_QUEUE_REMOVAL, Text::toT(tmp))) {
				return;
			} else {
				if (finishedFiles > 0) {
					tmp = STRING_F(CONFIRM_REMOVE_DIR_FINISHED_BUNDLE, finishedFiles);
					if(MessageBox(Text::toT(tmp).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
						moveFinished = true;
					}
				}
			}
		}
	} else {
		tmp = STRING_F(CONFIRM_REMOVE_DIR_MULTIPLE, dirBundles % fileBundles);
		if(!WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_QUEUE_REMOVAL, Text::toT(tmp))) {
			return;
		} else {
			if (finishedFiles > 0) {
				tmp = STRING_F(CONFIRM_REMOVE_DIR_FINISHED_MULTIPLE, finishedFiles);
				if(MessageBox(Text::toT(tmp).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
					moveFinished = true;
				}
			}
		}
	}

	auto sourceDir = curDir;
	MainFrame::getMainFrame()->addThreadedTask([=] {
		QueueManager::getInstance()->removeDir(sourceDir, bundles, moveFinished);
	});
}

void QueueFrame::moveSelected() {
	const QueueItemInfo* ii = ctrlQueue.getItemData(ctrlQueue.GetNextItem(-1, LVNI_SELECTED));
	BundlePtr b = ii->getBundle();
	if (!b) {
		return;
	}

	int n = ctrlQueue.GetSelectedCount();

	if(n == 1) {
		// Single file, get the full filename and move...
		tstring target = Text::toT(ii->getTarget());
		tstring ext = Util::getFileExt(target);
		tstring ext2;
		if (!ext.empty())
		{
			ext = ext.substr(1); // remove leading dot so default extension works when browsing for file
			ext2 = _T("*.") + ext;
			ext2 += (TCHAR)0;
			ext2 += _T("*.") + ext;
		}
		ext2 += _T("*.*\0*.*\0\0");

		tstring path = Text::toT(ii->getPath());
		if(WinUtil::browseFile(target, m_hWnd, true, path, ext2.c_str(), ext.empty() ? NULL : ext.c_str())) {
			StringPairList ret;
			ret.emplace_back(ii->getTarget(), Text::fromT(target));
			QueueManager::getInstance()->moveFiles(ret);
		}
	} else if(n > 1) {
		tstring name;
		if(showTree) {
			name = Text::toT(curDir);
		}

		if(WinUtil::browseDirectory(name, m_hWnd)) {
			int i = -1;
			StringPairList ret;
			while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
				const QueueItemInfo* ii = ctrlQueue.getItemData(i);
				ret.emplace_back(ii->getTarget(), Text::fromT(name) + Util::getFileName(ii->getTarget()));
			}

			MainFrame::getMainFrame()->addThreadedTask([=] { 
				QueueManager::getInstance()->moveFiles(ret);
			});
		}
	}
}

void QueueFrame::moveSelectedDir() {
	if(ctrlDirs.GetSelectedItem() == NULL)
		return;

	dcassert(!curDir.empty());
	tstring name = Text::toT(curDir);
	int finishedFiles = 0, fileBundles = 0;
	BundleList bundles;
	QueueManager::getInstance()->getBundleInfo(curDir, bundles, finishedFiles, fileBundles);
	int dirBundles = bundles.size() - fileBundles;
	bool moveFinished = false;
	
	if(WinUtil::browseDirectory(name, m_hWnd)) {
		string newDir = Util::validateFileName(Text::fromT(name) + Util::getLastDir(curDir) + PATH_SEPARATOR);
		string tmp;
		if (bundles.size() == 1) {
			BundlePtr bundle = bundles.front();
			if (stricmp(bundle->getTarget(), curDir) != 0) {
				tmp = STRING_F(CONFIRM_MOVE_DIR_BUNDLE_PART, Util::getLastDir(curDir).c_str() % bundle->getName().c_str() % newDir.c_str());
				if(MessageBox(Text::toT(tmp).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) {
					return;
				} else if (finishedFiles > 0) {
					tmp = STRING_F(CONFIRM_MOVE_DIR_FINISHED_BUNDLE_PART, finishedFiles);
					if(MessageBox(Text::toT(tmp).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
						moveFinished = true;
					}
				}
			} else {
				tmp = STRING_F(CONFIRM_MOVE_DIR_BUNDLE, bundle->getName().c_str() % newDir.c_str());
				if(MessageBox(Text::toT(tmp).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) {
					return;
				} else if (finishedFiles > 0) {
					tmp = STRING_F(CONFIRM_MOVE_DIR_FINISHED_BUNDLE, finishedFiles);
					if(MessageBox(Text::toT(tmp).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
						moveFinished = true;
					}
				}
			}
		} else {
			tmp = STRING_F(CONFIRM_MOVE_DIR_MULTIPLE, dirBundles % fileBundles % newDir.c_str());
			if(MessageBox(Text::toT(tmp).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) {
				return;
			} else if (finishedFiles > 0) {
				tmp = STRING_F(CONFIRM_MOVE_DIR_FINISHED_MULTIPLE, finishedFiles);
				if(MessageBox(Text::toT(tmp).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
					moveFinished = true;
				}
			}
		}

		if (curDir == newDir) {
			return;
		}
		
		for(auto& sourceBundle: bundles) {
			if (!sourceBundle->isFileBundle()) {
				auto sourceDir = curDir;
				MainFrame::getMainFrame()->addThreadedTask([=] {
					if (AirUtil::isParentOrExact(sourceDir, sourceBundle->getTarget())) {
						//we are moving the root bundle dir or some of it's parents
						QueueManager::getInstance()->moveBundle(sourceDir, newDir, sourceBundle, moveFinished);
					} else {
						//we are moving a subfolder of a bundle
						QueueManager::getInstance()->splitBundle(sourceDir, newDir, sourceBundle, moveFinished);
					}
				});
			} else {
				//move queue items
				QueueManager::getInstance()->moveFileBundle(sourceBundle, AirUtil::convertMovePath(sourceBundle->getTarget(), curDir, newDir));
			}
		}
	}
}

LRESULT QueueFrame::onRenameDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring cur = Text::toT(curDir);

	LineDlg virt;
	virt.title = TSTRING(VIRTUAL_NAME);
	virt.description = TSTRING(VIRTUAL_NAME_LONG);
	virt.line = Util::getLastDir(cur);
	if(virt.DoModal(m_hWnd) == IDOK) {
		if (virt.line == Util::getLastDir(cur)) {
			return 0;
		}

		size_t pos = cur.find(Util::getLastDir(cur));
		string newDir = Text::fromT(cur.substr(0, pos));
		newDir += Util::validateFileName(Text::fromT(virt.line));
		if (newDir[newDir.length()-1] != PATH_SEPARATOR)
			newDir += PATH_SEPARATOR;

		DirItemInfo* dii = (DirItemInfo*)ctrlDirs.GetItemData(ctrlDirs.GetSelectedItem());
		//BundleList bundles = dii->getBundles();
		if (dii->getBundles().size() == 1) {
			BundlePtr b = QueueManager::getInstance()->getBundle(dii->getBundles().front().second->getToken());
			if (curDir == newDir)
				return 0;

			auto sourceDir = curDir;
			MainFrame::getMainFrame()->addThreadedTask([=] {
				if (isCurDir(b->getTarget())) {
					QueueManager::getInstance()->moveBundle(sourceDir, newDir, b, true);
				} else {
					QueueManager::getInstance()->splitBundle(sourceDir, newDir, b, true);
				}
			});
		}
	}
	return 0;
}

LRESULT QueueFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	OMenu priorityMenu;

	priorityMenu.CreatePopupMenu();
	priorityMenu.InsertSeparatorFirst(TSTRING(PRIORITY));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_PAUSED, CTSTRING(PAUSED));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_LOWEST, CTSTRING(LOWEST));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_LOW, CTSTRING(LOW));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_NORMAL, CTSTRING(NORMAL));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_HIGH, CTSTRING(HIGH));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_HIGHEST, CTSTRING(HIGHEST));
	priorityMenu.AppendMenu(MF_STRING, IDC_AUTOPRIORITY, CTSTRING(AUTO));


	if (reinterpret_cast<HWND>(wParam) == ctrlQueue && ctrlQueue.GetSelectedCount() > 0) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	
		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlQueue, pt);
		}

		OMenu fileMenu;
		fileMenu.CreatePopupMenu();

		OMenu segmentsMenu;

		segmentsMenu.CreatePopupMenu();
		segmentsMenu.InsertSeparatorFirst(TSTRING(MAX_SEGMENTS_NUMBER));
		for(int i = IDC_SEGMENTONE; i <= IDC_SEGMENTTEN; i++)
			segmentsMenu.AppendMenu(MF_STRING, i, (Util::toStringW(i - 109) + _T(" ") + TSTRING(SEGMENTS)).c_str());
			
		if(ctrlQueue.GetSelectedCount() > 0) { 
			usingDirMenu = false;

			if(ctrlQueue.GetSelectedCount() == 1) {
				OMenu* pmMenu = fileMenu.getMenu();
				OMenu* browseMenu = fileMenu.getMenu();
				OMenu* removeAllMenu = fileMenu.getMenu();
				OMenu* previewMenu = fileMenu.getMenu();
				OMenu* removeMenu = fileMenu.getMenu();
				OMenu* readdMenu = fileMenu.getMenu();
				OMenu* getListMenu = fileMenu.getMenu();
				OMenu* copyMenu = fileMenu.getMenu();


				/* Create submenus */

				const QueueItemInfo* ii = ctrlQueue.getItemData(ctrlQueue.GetNextItem(-1, LVNI_SELECTED));
				if(ii) {
					segmentsMenu.CheckMenuItem(ii->getQueueItem()->getMaxSegments(), MF_BYPOSITION | MF_CHECKED);


					copyMenu->appendItem(TSTRING(COPY_MAGNET_LINK), [=] { WinUtil::copyMagnet(ii->getTTH(), Util::getFileName(ii->getTarget()), ii->getSize()); });
					for(int i = 0; i <COLUMN_LAST; ++i)
						copyMenu->appendItem(TSTRING_I(columnNames[i]), [=] { WinUtil::setClipboard(ii->getText(i)); });

					if(!ii->isSet(QueueItem::FLAG_USER_LIST)) {
						WinUtil::appendPreviewMenu(previewMenu, ii->getTarget());
					}

					bool hasPMItems = false;
					auto sources = move(QueueManager::getInstance()->getSources(ii->getQueueItem()));
					for(auto& s: sources) {
						tstring nick = WinUtil::escapeMenu(WinUtil::getNicks(s.getUser()));
						// add hub hint to menu
						if(!s.getUser().hint.empty())
							nick += _T(" (") + Text::toT(s.getUser().hint) + _T(")");

						auto u = s.getUser();
						auto target = ii->getTarget();
						getListMenu->appendItem(nick, [=] { QueueManager::getInstance()->addList(u, QueueItem::FLAG_CLIENT_VIEW); });
						browseMenu->appendItem(nick, [=] { QueueManager::getInstance()->addList(u, QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_PARTIAL_LIST); });
						removeMenu->appendItem(nick, [=] { QueueManager::getInstance()->removeSource(target, u, QueueItem::Source::FLAG_REMOVED); });
						removeAllMenu->appendItem(nick, [=]{ MainFrame::getMainFrame()->addThreadedTask([=] { QueueManager::getInstance()->removeSource(u, QueueItem::Source::FLAG_REMOVED); }); });

						if(s.getUser().user->isOnline()) {
							pmMenu->appendItem(nick, [=] { PrivateFrame::openWindow(u); });
							hasPMItems = true;
						}
					}

					if (!sources.empty()) {
						removeMenu->appendSeparator();
						removeMenu->appendItem(TSTRING(ALL), [=] {
							MainFrame::getMainFrame()->addThreadedTask([=] { 
								auto sources = QueueManager::getInstance()->getSources(ii->getQueueItem());
								for(auto& si: sources)
									QueueManager::getInstance()->removeSource(ii->getTarget(), si.getUser(), QueueItem::Source::FLAG_REMOVED);
							});
						});
					}
					
					auto badSources = move(QueueManager::getInstance()->getBadSources(ii->getQueueItem()));
					for(auto& s: badSources) {
						tstring nick = WinUtil::getNicks(s.getUser());
						if(s.isSet(QueueItem::Source::FLAG_FILE_NOT_AVAILABLE)) {
							nick += _T(" (") + TSTRING(FILE_NOT_AVAILABLE) + _T(")");
						} else if(s.isSet(QueueItem::Source::FLAG_BAD_TREE)) {
							nick += _T(" (") + TSTRING(INVALID_TREE) + _T(")");
						} else if(s.isSet(QueueItem::Source::FLAG_NO_NEED_PARTS)) {
							nick += _T(" (") + TSTRING(NO_NEEDED_PART) + _T(")");
						} else if(s.isSet(QueueItem::Source::FLAG_NO_TTHF)) {
							nick += _T(" (") + TSTRING(SOURCE_TOO_OLD) + _T(")");
						} else if(s.isSet(QueueItem::Source::FLAG_SLOW_SOURCE)) {
							nick += _T(" (") + TSTRING(SLOW_USER) + _T(")");
						} else if(s.isSet(QueueItem::Source::FLAG_UNTRUSTED)) {
							nick += _T(" (") + TSTRING(CERTIFICATE_NOT_TRUSTED) + _T(")");
						}

						// add hub hint to menu
						if(!s.getUser().hint.empty())
							nick += _T(" (") + Text::toT(s.getUser().hint) + _T(")");

						auto u = s.getUser();
						auto target = ii->getTarget();
						readdMenu->appendItem(nick, [=] { QueueManager::getInstance()->readdQISource(target, u); });
					}

					if (!badSources.empty()) {
						readdMenu->appendSeparator();
						readdMenu->appendItem(TSTRING(ALL), [=] {
							auto sources = QueueManager::getInstance()->getBadSources(ii->getQueueItem());
							for(auto& si: sources)
								QueueManager::getInstance()->readdQISource(ii->getTarget(), si.getUser());
						});
					}
				}

				priorityMenu.CheckMenuItem(ii->getPriority() + 1, MF_BYPOSITION | MF_CHECKED);
				if(ii->getAutoPriority())
					priorityMenu.CheckMenuItem(7, MF_BYPOSITION | MF_CHECKED);

				/* Submenus end */


				fileMenu.InsertSeparatorFirst(TSTRING(FILE));
				fileMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));

				previewMenu->appendThis(TSTRING(PREVIEW_MENU), true);

				fileMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)segmentsMenu, CTSTRING(MAX_SEGMENTS_NUMBER));
				fileMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)priorityMenu, CTSTRING(SET_FILE_PRIORITY));

				browseMenu->appendThis(TSTRING(BROWSE_FILE_LIST), true);
				getListMenu->appendThis(TSTRING(GET_FILE_LIST), true);
				pmMenu->appendThis(TSTRING(SEND_PRIVATE_MESSAGE), true);
				readdMenu->appendThis(TSTRING(READD_SOURCE), true);

				copyMenu->appendThis(TSTRING(COPY), true);
				fileMenu.AppendMenu(MF_SEPARATOR);

				WinUtil::appendSearchMenu(fileMenu, ii->getPath());

				fileMenu.AppendMenu(MF_SEPARATOR);
				fileMenu.AppendMenu(MF_STRING, IDC_MOVE, CTSTRING(MOVE_RENAME_FILE));
				fileMenu.AppendMenu(MF_STRING, IDC_OPEN_FOLDER, CTSTRING(OPEN_FOLDER));
				fileMenu.AppendMenu(MF_SEPARATOR);

				removeMenu->appendThis(TSTRING(REMOVE_SOURCE), true);
				removeAllMenu->appendThis(TSTRING(REMOVE_FROM_ALL), true);

				fileMenu.AppendMenu(MF_STRING, IDC_REMOVE_OFFLINE, CTSTRING(REMOVE_OFFLINE));
				fileMenu.AppendMenu(MF_SEPARATOR);
				fileMenu.AppendMenu(MF_STRING, IDC_RECHECK, CTSTRING(RECHECK_FILE));
				fileMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
			} else {
				fileMenu.InsertSeparatorFirst(TSTRING(FILES));
				fileMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)segmentsMenu, CTSTRING(MAX_SEGMENTS_NUMBER));
				fileMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)priorityMenu, CTSTRING(SET_FILE_PRIORITIES));
				fileMenu.AppendMenu(MF_STRING, IDC_MOVE, CTSTRING(MOVE_RENAME_FILE));
				fileMenu.AppendMenu(MF_SEPARATOR);
				fileMenu.AppendMenu(MF_STRING, IDC_REMOVE_OFFLINE, CTSTRING(REMOVE_OFFLINE));
				fileMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
				fileMenu.AppendMenu(MF_STRING, IDC_READD_ALL, CTSTRING(READD_ALL));
			}
		
			fileMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
			return TRUE; 
		}
	} else if (reinterpret_cast<HWND>(wParam) == ctrlDirs && ctrlDirs.GetSelectedItem() != NULL) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlDirs, pt);
		} else {
			// Strange, windows doesn't change the selection on right-click... (!)
			UINT a = 0;
			ctrlDirs.ScreenToClient(&pt);
			HTREEITEM ht = ctrlDirs.HitTest(pt, &a);
			if(ht != NULL && ht != ctrlDirs.GetSelectedItem())
				ctrlDirs.SelectItem(ht);
			ctrlDirs.ClientToScreen(&pt);
		}
		usingDirMenu = true;
		
		OMenu dirMenu;
		dirMenu.CreatePopupMenu();

		OMenu* removeMenu = dirMenu.getMenu();
		OMenu* readdMenu = dirMenu.getMenu();

		DirItemInfo* dii = (DirItemInfo*)ctrlDirs.GetItemData(ctrlDirs.GetSelectedItem());
		Bundle::StringBundleList bundles = dii->getBundles();
		bool mainBundle = false;
		BundlePtr b = NULL;
		if (!bundles.empty()) {
			if (bundles.size() == 1) {
				b = bundles.front().second;
				mainBundle = !AirUtil::isSub(curDir, b->getTarget());
				if (mainBundle) {
					dirMenu.InsertSeparatorFirst(TSTRING(BUNDLE));
					dirMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)priorityMenu, CTSTRING(SET_BUNDLE_PRIORITY));
				} else {
					int files = QueueManager::getInstance()->getDirItemCount(b, curDir);
					if (files == 1) {
						dirMenu.InsertSeparatorFirst(CTSTRING(FILE));
						dirMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)priorityMenu, CTSTRING(SET_FILE_PRIORITY));
					} else {
						dirMenu.InsertSeparatorFirst(CTSTRING_F(X_FILES, files));
						dirMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)priorityMenu, CTSTRING(SET_FILE_PRIORITIES));
					}
				}
			} else {
				dirMenu.InsertSeparatorFirst(CTSTRING_F(X_BUNDLES, bundles.size()));
				dirMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)priorityMenu, CTSTRING(SET_BUNDLE_PRIORITIES));
			}
		} else {
			dirMenu.InsertSeparatorFirst(TSTRING(FOLDER));
		}


		/* Insert submenus */
		if (mainBundle) {
			priorityMenu.CheckMenuItem(b->getPriority() + 1, MF_BYPOSITION | MF_CHECKED);
			if(b->getAutoPriority()) {
				priorityMenu.CheckMenuItem(7, MF_BYPOSITION | MF_CHECKED);
			}

			auto formatUser = [this] (Bundle::SourceTuple& bs) -> tstring {
				auto u = get<Bundle::SOURCE_USER>(bs);
				tstring nick = WinUtil::escapeMenu(WinUtil::getNicks(u));
				// add hub hint to menu
				bool addHint = !u.hint.empty(), addSpeed = u.user->getSpeed() > 0;
				nick += _T(" (") + TSTRING(FILES) + _T(": ") + Util::toStringW(get<Bundle::SOURCE_FILES>(bs));
				if(addHint || addSpeed) {
					nick += _T(", ");
					if (addSpeed) {
						nick += TSTRING(SPEED) + _T(": ") + Util::formatBytesW(u.user->getSpeed()) + _T("/s)");
					}
					if(addHint) {
						if(addSpeed) {
							nick += _T(", ");
						}
						nick += TSTRING(HUB) + _T(": ") + Text::toT(u.hint);
					}
				}
				nick += _T(")");
				return nick;
			};

			//current sources
			auto bundleSources = move(QueueManager::getInstance()->getBundleSources(b));
			for(auto& bs: bundleSources) {
				auto u = get<Bundle::SOURCE_USER>(bs);
				removeMenu->appendItem(formatUser(bs), [b, u] { QueueManager::getInstance()->removeBundleSource(b, u); });
			}
			if (!bundleSources.empty()) {
				removeMenu->appendSeparator();
				removeMenu->appendItem(TSTRING(ALL), [b] {
					MainFrame::getMainFrame()->addThreadedTask([=] { 
						auto sources = move(QueueManager::getInstance()->getBundleSources(b));
						for(auto& si: sources)
							QueueManager::getInstance()->removeBundleSource(b, get<Bundle::SOURCE_USER>(si).user);
					});
				});
			}

			//bad sources
			auto badBundleSources = move(QueueManager::getInstance()->getBadBundleSources(b));
			for(auto& bs: badBundleSources) {
				auto u = get<Bundle::SOURCE_USER>(bs);
				readdMenu->appendItem(formatUser(bs), [b, u] { QueueManager::getInstance()->readdBundleSource(b, u); });
			}
			if (!badBundleSources.empty()) {
				readdMenu->appendSeparator();
				readdMenu->appendItem(TSTRING(ALL), [b] { 
					MainFrame::getMainFrame()->addThreadedTask([=] { 
						auto sources = move(QueueManager::getInstance()->getBadBundleSources(b));
						for(auto& si: sources)
							QueueManager::getInstance()->readdBundleSource(b, get<Bundle::SOURCE_USER>(si));
					});
				});
			}
		}
		/* Submenus end */

		if (curDir != Util::getListPath() && curDir != Util::getOpenPath()) {
			if (mainBundle) {
				dirMenu.AppendMenu(MF_STRING, IDC_SEARCH_BUNDLE, CTSTRING(SEARCH_BUNDLE_ALT));
			}
			dirMenu.AppendMenu(MF_SEPARATOR);
			WinUtil::appendSearchMenu(dirMenu, curDir);
			dirMenu.AppendMenu(MF_STRING, IDC_SEARCHDIR, CTSTRING(SEARCH_DIRECTORY));
			dirMenu.AppendMenu(MF_STRING, IDC_COPY, CTSTRING(COPY_DIRECTORY));
			dirMenu.AppendMenu(MF_SEPARATOR);
			dirMenu.AppendMenu(MF_STRING, IDC_OPEN_FOLDER, CTSTRING(OPEN_FOLDER));
			dirMenu.AppendMenu(MF_STRING, IDC_MOVE, CTSTRING(MOVE_DIR));
			if (b && AirUtil::isParentOrExact(b->getTarget(), curDir)) {
				dirMenu.AppendMenu(MF_STRING, IDC_RENAME, CTSTRING(RENAME_DIR));
			}
			dirMenu.AppendMenu(MF_SEPARATOR);

			if (mainBundle) {
				readdMenu->appendThis(CTSTRING(READD_SOURCE), true, true);
				removeMenu->appendThis(CTSTRING(REMOVE_SOURCE), true, true);
				if (!b->getSeqOrder()) {
					dirMenu.AppendMenu(MF_POPUP, IDC_USE_SEQ_ORDER, CTSTRING(USE_SEQ_ORDER));
				}
			}
		}
		dirMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));

		dirMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
		return TRUE;
	}

	bHandled = FALSE;
	return FALSE; 
}

LRESULT QueueFrame::onSeqOrder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	DirItemInfo* dii = (DirItemInfo*)ctrlDirs.GetItemData(ctrlDirs.GetSelectedItem());
	if (dii) {
		BundlePtr b = dii->getBundles().front().second;
		QueueManager::getInstance()->onUseSeqOrder(b);
	}
	return 0;
}

LRESULT QueueFrame::onRecheck(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlQueue.GetSelectedCount() == 1) {
		int i = ctrlQueue.GetNextItem(-1, LVNI_SELECTED);
		const QueueItemInfo* ii = ctrlQueue.getItemData(i);
		QueueManager::getInstance()->recheck(ii->getTarget());
	}	
	return 0;
}

LRESULT QueueFrame::onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlQueue.GetSelectedCount() == 1 || ctrlDirs.GetSelectedItem() != NULL) {
		if(usingDirMenu) {
			WinUtil::searchAny(Util::getLastDir(Text::toT(getSelectedDir())));
		} else {
			int i = ctrlQueue.GetNextItem(-1, LVNI_SELECTED);
			const QueueItemInfo* ii = ctrlQueue.getItemData(i);
			if(ii != NULL)
				WinUtil::searchHash(ii->getTTH(), Util::getFileName(ii->getTarget()), ii->getSize());
		}
	}
	return 0;
}

LRESULT QueueFrame::onSearchBundle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlQueue.GetSelectedCount() == 1 || ctrlDirs.GetSelectedItem() != NULL) {
		if(usingDirMenu) {
			auto i = bundleMap.find(curDir);
			if (i != bundleMap.end()) {
				DirItemInfo* dii = ((DirItemInfo*)ctrlDirs.GetItemData(i->second));
				dcassert(dii->getBundles().front().second);
				BundlePtr b = QueueManager::getInstance()->getBundle(dii->getBundles().front().second->getToken());
				if (b) {
					QueueManager::getInstance()->searchBundle(b, true);
				}
			}
		}
	}
	return 0;
}

LRESULT QueueFrame::onAutoPriority(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {	

	if(usingDirMenu) {
		//setAutoPriority(ctrlDirs.GetSelectedItem(), true);
		int finishedFiles = 0, fileBundles = 0;
		BundleList bundles;
		QueueManager::getInstance()->getBundleInfo(curDir, bundles, finishedFiles, fileBundles);
		if (!bundles.empty()) {
			QueueManager::getInstance()->setBundlePriorities(curDir, bundles, Bundle::DEFAULT, true);
		}
	} else {
		int i = -1;
		while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
			QueueManager::getInstance()->setQIAutoPriority(ctrlQueue.getItemData(i)->getTarget(),!ctrlQueue.getItemData(i)->getAutoPriority());
		}
	}
	return 0;
}

LRESULT QueueFrame::onSegments(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
		QueueItemInfo* ii = ctrlQueue.getItemData(i);
		QueueManager::getInstance()->setSegments(ii->getTarget(), (uint8_t)(wID - 109));

		ctrlQueue.updateItem(ctrlQueue.findItem(ii), COLUMN_SEGMENTS);
	}

	return 0;
}

LRESULT QueueFrame::onPriority(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	QueueItem::Priority p;

	switch(wID) {
		case IDC_PRIORITY_PAUSED: p = QueueItem::PAUSED; break;
		case IDC_PRIORITY_LOWEST: p = QueueItem::LOWEST; break;
		case IDC_PRIORITY_LOW: p = QueueItem::LOW; break;
		case IDC_PRIORITY_NORMAL: p = QueueItem::NORMAL; break;
		case IDC_PRIORITY_HIGH: p = QueueItem::HIGH; break;
		case IDC_PRIORITY_HIGHEST: p = QueueItem::HIGHEST; break;
		default: p = QueueItem::DEFAULT; break;
	}

	if(usingDirMenu) {
		int tmp1=0, tmp2=0;
		BundleList bundles;
		QueueManager::getInstance()->getBundleInfo(curDir, bundles, tmp1, tmp2);
		if (!bundles.empty()) {
			QueueManager::getInstance()->setBundlePriorities(curDir, bundles, (Bundle::Priority)p);
		}
		//setPriority(ctrlDirs.GetSelectedItem(), p);
	} else {
		int i = -1;
		while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
			QueueManager::getInstance()->setQIAutoPriority(ctrlQueue.getItemData(i)->getTarget(), false);
			QueueManager::getInstance()->setQIPriority(ctrlQueue.getItemData(i)->getTarget(), p);
		}
	}

	return 0;
}

void QueueFrame::removeDir(HTREEITEM ht) {
	if(ht == NULL)
		return;
	HTREEITEM child = ctrlDirs.GetChildItem(ht);
	while(child) {
		removeDir(child);
		child = ctrlDirs.GetNextSiblingItem(child);
	}
	const string& name = getDir(ht);
	auto dp = directories.equal_range(name);
	for(auto i = dp.first; i != dp.second; ++i) {
		QueueManager::getInstance()->remove(i->second->getTarget());
	}
}

/*
 * @param inc True = increase, False = decrease
 */
void QueueFrame::changePriority(bool inc){
	int i = -1;
	while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1){
		QueueItem::Priority p = ctrlQueue.getItemData(i)->getPriority();

		if ((inc && p == QueueItem::HIGHEST) || (!inc && p == QueueItem::PAUSED)){
			// Trying to go higher than HIGHEST or lower than PAUSED
			// so do nothing
			continue;
		}

		switch(p){
			case QueueItem::HIGHEST: p = QueueItem::HIGH; break;
			case QueueItem::HIGH:    p = inc ? QueueItem::HIGHEST : QueueItem::NORMAL; break;
			case QueueItem::NORMAL:  p = inc ? QueueItem::HIGH    : QueueItem::LOW; break;
			case QueueItem::LOW:     p = inc ? QueueItem::NORMAL  : QueueItem::LOWEST; break;
			case QueueItem::LOWEST:  p = inc ? QueueItem::LOW     : QueueItem::PAUSED; break;
			case QueueItem::PAUSED:  p = QueueItem::LOWEST; break;
		}
		QueueManager::getInstance()->setQIAutoPriority(ctrlQueue.getItemData(i)->getTarget(), false);
		QueueManager::getInstance()->setQIPriority(ctrlQueue.getItemData(i)->getTarget(), p);
	}
}

void QueueFrame::setPriority(HTREEITEM ht, const QueueItem::Priority& p) {
	if(ht == NULL)
		return;
	HTREEITEM child = ctrlDirs.GetChildItem(ht);
	while(child) {
		setPriority(child, p);
		child = ctrlDirs.GetNextSiblingItem(child);
	}
	const string& name = getDir(ht);
	DirectoryPairC dp = directories.equal_range(name);
	for(auto i = dp.first; i != dp.second; ++i) {
		QueueManager::getInstance()->setQIAutoPriority(i->second->getTarget(), false);
		QueueManager::getInstance()->setQIPriority(i->second->getTarget(), p);
	}
}

void QueueFrame::setAutoPriority(HTREEITEM ht, const bool& ap) {
	if(ht == NULL)
		return;
	HTREEITEM child = ctrlDirs.GetChildItem(ht);
	while(child) {
		setAutoPriority(child, ap);
		child = ctrlDirs.GetNextSiblingItem(child);
	}
	const string& name = getDir(ht);
	auto dp = directories.equal_range(name);
	for(auto i = dp.first; i != dp.second; ++i) {
		QueueManager::getInstance()->setQIAutoPriority(i->second->getTarget(), ap);
	}
}

void QueueFrame::updateStatus() {
	if(ctrlStatus.IsWindow()) {
		int64_t total = 0;
		int cnt = ctrlQueue.GetSelectedCount();
		if(cnt == 0) {
			cnt = ctrlQueue.GetItemCount();
			if(showTree) {
				for(int i = 0; i < cnt; ++i) {
					const QueueItemInfo* ii = ctrlQueue.getItemData(i);
					total += (ii->getSize() > 0) ? ii->getSize() : 0;
				}
			} else {
				total = queueSize;
			}
		} else {
			int i = -1;
			while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
				const QueueItemInfo* ii = ctrlQueue.getItemData(i);
				total += (ii->getSize() > 0) ? ii->getSize() : 0;
			}

		}

		tstring tmp1 = TSTRING(ITEMS) + _T(": ") + Util::toStringW(cnt);
		tstring tmp2 = TSTRING(SIZE) + _T(": ") + Util::formatBytesW(total);
		bool u = false;

		int w = WinUtil::getTextWidth(tmp1, ctrlStatus.m_hWnd);
		if(statusSizes[1] < w) {
			statusSizes[1] = w;
			u = true;
		}
		ctrlStatus.SetText(2, tmp1.c_str());
		w = WinUtil::getTextWidth(tmp2, ctrlStatus.m_hWnd);
		if(statusSizes[2] < w) {
			statusSizes[2] = w;
			u = true;
		}
		ctrlStatus.SetText(3, tmp2.c_str());

		if(dirty) {
			tmp1 = TSTRING(FILES) + _T(": ") + Util::toStringW(queueItems);
			tmp2 = TSTRING(SIZE) + _T(": ") + Util::formatBytesW(queueSize);

			w = WinUtil::getTextWidth(tmp2, ctrlStatus.m_hWnd);
			if(statusSizes[3] < w) {
				statusSizes[3] = w;
				u = true;
			}
			ctrlStatus.SetText(4, tmp1.c_str());

			w = WinUtil::getTextWidth(tmp2, ctrlStatus.m_hWnd);
			if(statusSizes[4] < w) {
				statusSizes[4] = w;
				u = true;
			}
			ctrlStatus.SetText(5, tmp2.c_str());

			dirty = false;
		}

		if(u)
			UpdateLayout(TRUE);
	}
}

void QueueFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[6];
		ctrlStatus.GetClientRect(sr);
		w[5] = sr.right - 16;
#define setw(x) w[x] = max(w[x+1] - statusSizes[x], 0)
		setw(4); setw(3); setw(2); setw(1);

		w[0] = 16;

		ctrlStatus.SetParts(6, w);

		ctrlStatus.GetRect(0, sr);
		ctrlShowTree.MoveWindow(sr);
	}

	if(showTree) {
		if(GetSinglePaneMode() != SPLIT_PANE_NONE) {
			SetSinglePaneMode(SPLIT_PANE_NONE);
			updateQueue();
		}
	} else {
		if(GetSinglePaneMode() != SPLIT_PANE_RIGHT) {
			SetSinglePaneMode(SPLIT_PANE_RIGHT);
			updateQueue();
		}
	}

	CRect rc = rect;
	SetSplitterRect(rc);
}

LRESULT QueueFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		QueueManager::getInstance()->removeListener(this);
		DownloadManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		closed = true;
		WinUtil::setButtonPressed(IDC_QUEUE, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		HTREEITEM ht = ctrlDirs.GetRootItem();
		while(ht != NULL) {
			clearTree(ht);
			ht = ctrlDirs.GetNextSiblingItem(ht);
		}

		SettingsManager::getInstance()->set(SettingsManager::QUEUEFRAME_SHOW_TREE, ctrlShowTree.GetCheck() == BST_CHECKED);

		for(auto d: directories | map_values)
			delete d;

		directories.clear();
		ctrlQueue.DeleteAllItems();

		CRect rc;
		if(!IsIconic()){
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

		bHandled = FALSE;
		return 0;
	}
}

LRESULT QueueFrame::onItemChanged(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
	updateQueue();
	return 0;
}

void QueueFrame::onTab() {
	if(showTree) {
		HWND focus = ::GetFocus();
		if(focus == ctrlDirs.m_hWnd) {
			ctrlQueue.SetFocus();
		} else if(focus == ctrlQueue.m_hWnd) {
			ctrlDirs.SetFocus();
		}
	}
}

void QueueFrame::updateQueue() {
	ctrlQueue.DeleteAllItems();
	pair<DirectoryIter, DirectoryIter> i;
	if(showTree) {
		i = directories.equal_range(getSelectedDir());
	} else {
		i.first = directories.begin();
		i.second = directories.end();
	}

	ctrlQueue.SetRedraw(FALSE);
	for(auto j = i.first; j != i.second; ++j) {
		QueueItemInfo* ii = j->second;
		ctrlQueue.insertItem(ctrlQueue.GetItemCount(), ii, ResourceLoader::getIconIndex(Text::toT(ii->getTarget())));
	}
	ctrlQueue.resort();
	ctrlQueue.SetRedraw(TRUE);
	curDir = getSelectedDir();
	updateStatus();
}

void QueueFrame::clearTree(HTREEITEM item) {
	HTREEITEM next = ctrlDirs.GetChildItem(item);
	while(next != NULL) {
		clearTree(next);
		next = ctrlDirs.GetNextSiblingItem(next);
	}
	delete (DirItemInfo*)ctrlDirs.GetItemData(item);
}

// Put it here to avoid a copy for each recursion...
static TCHAR tmpBuf[1024];
void QueueFrame::moveNode(HTREEITEM item, HTREEITEM parent) {
	TVINSERTSTRUCT tvis;
	memzero(&tvis, sizeof(tvis));
	tvis.itemex.hItem = item;
	tvis.itemex.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_IMAGE | TVIF_INTEGRAL | TVIF_PARAM |
		TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_TEXT;

	/* Bundles */
	DirItemInfo* dii = ((DirItemInfo*)ctrlDirs.GetItemData(item));
	Bundle::StringBundleList bundles = dii->getBundles();
	dcassert(!bundles.empty());
	tstring name;
	bool updateMap = false;
	bool setBold = false;
	//check if we need to modify the bundlemap
	for (auto& b: bundles | map_values) {
		if (stricmp(b->getTarget(), dii->getDir()) == 0) {
			updateMap = true;
			setBold=true;
			break;
		} else if (b->isFileBundle() && stricmp(Util::getFilePath(b->getTarget()), dii->getDir()) == 0) {
			setBold=true;
			break;
		}
	}
	if (setBold) {
		tvis.itemex.state = TVIS_BOLD;
		tvis.itemex.stateMask = TVIS_BOLD;
	}

	tvis.itemex.pszText = tmpBuf;
	tvis.itemex.cchTextMax = 1024;
	ctrlDirs.GetItem((TVITEM*)&tvis.itemex);
	tvis.hInsertAfter =	TVI_SORT;
	tvis.hParent = parent;
	tvis.itemex.lParam = (LPARAM)dii;
	HTREEITEM ht = ctrlDirs.InsertItem(&tvis);
	if (updateMap) {
		auto s = bundleMap.find(dii->getDir());
		dcassert(s != bundleMap.end());
		s->second = ht;
	}
	HTREEITEM next = ctrlDirs.GetChildItem(item);
	while(next != NULL) {
		moveNode(next, ht);
		next = ctrlDirs.GetChildItem(item);
	}
	ctrlDirs.DeleteItem(item);
}

LRESULT QueueFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		{
			if(!QueueManager::getInstance()->getBadSources(((QueueItemInfo*)cd->nmcd.lItemlParam)->getQueueItem()).empty()) {
				cd->clrText = SETTING(ERROR_COLOR);
				return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
			}		
		}
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
		if(ctrlQueue.findColumn(cd->iSubItem) == COLUMN_PROGRESS) {
			if(!SETTING(SHOW_PROGRESS_BARS) || !SETTING(SHOW_QUEUE_BARS) ) {
				bHandled = FALSE;
				return 0;
			}			
			QueueItemInfo *qii = (QueueItemInfo*)cd->nmcd.lItemlParam;
			if(qii->getSize() == -1) return CDRF_DODEFAULT;

			// draw something nice...
			CRect rc;
			ctrlQueue.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);
			CBarShader statusBar(rc.Height(), rc.Width(), SETTING(PROGRESS_BACK_COLOR), qii->getSize());

			vector<Segment> v;

			// running chunks
			v = QueueManager::getInstance()->getChunksVisualisation(qii->getQueueItem(), 0);
			for(const auto& s: v) {
				statusBar.FillRange(s.getStart(), s.getEnd(), SETTING(COLOR_RUNNING));
			}

			// downloaded bytes
			v = QueueManager::getInstance()->getChunksVisualisation(qii->getQueueItem(), 1);
			for(const auto& s: v) {
				statusBar.FillRange(s.getStart(), s.getEnd(), SETTING(COLOR_DOWNLOADED));
			}

			// done chunks
			v = QueueManager::getInstance()->getChunksVisualisation(qii->getQueueItem(), 2);
			for(const auto& s: v) {
				statusBar.FillRange(s.getStart(), s.getEnd(), SETTING(COLOR_DONE));
			}

			CDC cdc;
			cdc.CreateCompatibleDC(cd->nmcd.hdc);
			HBITMAP pOldBmp = cdc.SelectBitmap(CreateCompatibleBitmap(cd->nmcd.hdc,  rc.Width(),  rc.Height()));

			statusBar.Draw(cdc, 0, 0, SETTING(PROGRESS_3DDEPTH));
			BitBlt(cd->nmcd.hdc, rc.left, rc.top, rc.Width(), rc.Height(), cdc.m_hDC, 0, 0, SRCCOPY);
			DeleteObject(cdc.SelectBitmap(pOldBmp));

			return CDRF_SKIPDEFAULT;
		}
	}
	default:
		return CDRF_DODEFAULT;
	}
}			

LRESULT QueueFrame::onCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	WinUtil::setClipboard(Text::toT(Util::getLastDir(curDir)));
	return 0;
}
LRESULT QueueFrame::onOpenFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	if(usingDirMenu) {
		//..
		WinUtil::openFolder(Text::toT(curDir));
	} else {
		const QueueItemInfo* ii = ctrlQueue.getItemData(ctrlQueue.GetNextItem(-1, LVNI_SELECTED));
		if(ii != NULL)
			WinUtil::openFolder(ii->getText(COLUMN_PATH));
	}
	return 0;
}

LRESULT QueueFrame::onRemoveOffline(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const QueueItemInfo* ii = ctrlQueue.getItemData(i);

		QueueItem::SourceList sources = QueueManager::getInstance()->getSources(ii->getQueueItem());
		for(const auto& s: sources) {
			if(!s.getUser().user->isOnline()) {
				QueueManager::getInstance()->removeSource(ii->getTarget(), s.getUser().user, QueueItem::Source::FLAG_REMOVED);
			}
		}
	}
	return 0;
}
LRESULT QueueFrame::onReaddAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const QueueItemInfo* ii = ctrlQueue.getItemData(i);

		// re-add all sources
		QueueItem::SourceList badSources = QueueManager::getInstance()->getBadSources(ii->getQueueItem());
		for(const auto& bs: badSources) {
			QueueManager::getInstance()->readdQISource(ii->getTarget(), bs.getUser());
		}
	}
	return 0; 
}

void QueueFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	bool refresh = false;
	if(ctrlQueue.GetBkColor() != WinUtil::bgColor) {
		ctrlQueue.SetBkColor(WinUtil::bgColor);
		ctrlQueue.SetTextBkColor(WinUtil::bgColor);
		ctrlQueue.setFlickerFree(WinUtil::bgBrush);
		ctrlDirs.SetBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlQueue.GetTextColor() != WinUtil::textColor) {
		ctrlQueue.SetTextColor(WinUtil::textColor);
		ctrlDirs.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

void QueueFrame::onRechecked(const string& target, const string& message) {
	string buf;
	buf.resize(STRING(INTEGRITY_CHECK).length() + message.length() + target.length() + 16);
	sprintf(&buf[0], CSTRING(INTEGRITY_CHECK), message.c_str(), target.c_str());
		
	speak(UPDATE_STATUS, new StringTask(&buf[0]));
}

void QueueFrame::on(QueueManagerListener::RecheckStarted, const string& target) noexcept {
	onRechecked(target, STRING(STARTED));
}

void QueueFrame::on(QueueManagerListener::RecheckNoFile, const string& target) noexcept {
	onRechecked(target, STRING(UNFINISHED_FILE_NOT_FOUND));
}

void QueueFrame::on(QueueManagerListener::RecheckFileTooSmall, const string& target) noexcept {
	onRechecked(target, STRING(UNFINISHED_FILE_TOO_SMALL));
}

void QueueFrame::on(QueueManagerListener::RecheckDownloadsRunning, const string& target) noexcept {
	onRechecked(target, STRING(DOWNLOADS_RUNNING));
}

void QueueFrame::on(QueueManagerListener::RecheckNoTree, const string& target) noexcept {
	onRechecked(target, STRING(NO_FULL_TREE));
}

void QueueFrame::on(QueueManagerListener::RecheckAlreadyFinished, const string& target) noexcept {
	onRechecked(target, STRING(FILE_ALREADY_FINISHED));
}

void QueueFrame::on(QueueManagerListener::RecheckDone, const string& target) noexcept {
	onRechecked(target, STRING(DONE));
}