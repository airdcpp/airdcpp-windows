/*
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(QUEUE_FRAME_H)
#define QUEUE_FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"

#include "../client/QueueManager.h"
#include "../client/FastAlloc.h"
#include "../client/TaskQueue.h"
#include "boost/unordered_map.hpp"

#define SHOWTREE_MESSAGE_MAP 12

class QueueFrame : public MDITabChildWindowImpl<QueueFrame>, public StaticFrame<QueueFrame, ResourceManager::DOWNLOAD_QUEUE, IDC_QUEUE>,
	private QueueManagerListener, private DownloadManagerListener, public CSplitterImpl<QueueFrame>, private SettingsManagerListener
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("QueueFrame"), IDR_QUEUE, 0, COLOR_3DFACE);

	QueueFrame() : menuItems(0), queueSize(0), queueItems(0), spoken(false), dirty(false), 
		usingDirMenu(false),  readdItems(0), fileLists(NULL), tempItems(NULL), showTree(true), closed(false), PreviewAppsSize(0),
		showTreeContainer(WC_BUTTON, this, SHOWTREE_MESSAGE_MAP) 
	{
	}

	~QueueFrame() {
		// Clear up dynamicly allocated menu objects
		browseMenu.ClearMenu();
		removeMenu.ClearMenu();
		removeAllMenu.ClearMenu();
		pmMenu.ClearMenu();
		readdMenu.ClearMenu();		
	}
	
	typedef MDITabChildWindowImpl<QueueFrame> baseClass;
	typedef CSplitterImpl<QueueFrame> splitBase;

	BEGIN_MSG_MAP(QueueFrame)
		NOTIFY_HANDLER(IDC_QUEUE, LVN_GETDISPINFO, ctrlQueue.onGetDispInfo)
		NOTIFY_HANDLER(IDC_QUEUE, LVN_COLUMNCLICK, ctrlQueue.onColumnClick)
		NOTIFY_HANDLER(IDC_QUEUE, LVN_GETINFOTIP, ctrlQueue.onInfoTip)
		NOTIFY_HANDLER(IDC_QUEUE, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_QUEUE, LVN_ITEMCHANGED, onItemChangedQueue)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_SELCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_KEYDOWN, onKeyDownDirs)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		COMMAND_ID_HANDLER(IDC_SEARCH_BUNDLE, onSearchBundle)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchAlternates)
		COMMAND_ID_HANDLER(IDC_SEARCHDIR, onSearchAlternates)
		COMMAND_ID_HANDLER(IDC_COPY_LINK, onCopyMagnet)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_RECHECK, onRecheck);
		COMMAND_ID_HANDLER(IDC_REMOVE_OFFLINE, onRemoveOffline)
		COMMAND_ID_HANDLER(IDC_READD_ALL, onReaddAll)
		COMMAND_ID_HANDLER(IDC_MOVE, onMove)
		COMMAND_RANGE_HANDLER(IDC_SEARCH_SITES, IDC_SEARCH_SITES + WebShortcuts::getInstance()->list.size(), onSearchSite)
		COMMAND_ID_HANDLER(IDC_OPEN_FOLDER, onOpenFolder)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		COMMAND_RANGE_HANDLER(IDC_COPY, IDC_COPY + COLUMN_LAST-1, onCopy)
		COMMAND_RANGE_HANDLER(IDC_PRIORITY_PAUSED, IDC_PRIORITY_HIGHEST, onPriority)
		COMMAND_RANGE_HANDLER(IDC_SEGMENTONE, IDC_SEGMENTTEN, onSegments)
		COMMAND_RANGE_HANDLER(IDC_BROWSELIST, IDC_BROWSELIST + menuItems, onBrowseList)
		COMMAND_RANGE_HANDLER(IDC_REMOVE_SOURCE, IDC_REMOVE_SOURCE + menuItems, onRemoveSource)
		COMMAND_RANGE_HANDLER(IDC_REMOVE_SOURCES, IDC_REMOVE_SOURCES + 1 + menuItems, onRemoveSources)
		COMMAND_RANGE_HANDLER(IDC_PM, IDC_PM + menuItems, onPM)
		COMMAND_RANGE_HANDLER(IDC_READD, IDC_READD + 1 + readdItems, onReadd)
		COMMAND_ID_HANDLER(IDC_AUTOPRIORITY, onAutoPriority)
		COMMAND_RANGE_HANDLER(IDC_PREVIEW_APP, IDC_PREVIEW_APP + PreviewAppsSize, onPreviewCommand)
		NOTIFY_HANDLER(IDC_QUEUE, NM_CUSTOMDRAW, onCustomDraw)
		CHAIN_MSG_MAP(splitBase)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(SHOWTREE_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onShowTree)
	END_MSG_MAP()

	LRESULT onPriority(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSegments(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBrowseList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveSource(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveSources(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPM(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onReadd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onReaddAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRecheck(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchBundle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyMagnet(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onAutoPriority(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPreviewCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onRemoveOffline(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchSite(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE);
	void removeDir(HTREEITEM ht);
	void setPriority(HTREEITEM ht, const QueueItem::Priority& p);
	void setAutoPriority(HTREEITEM ht, const bool& ap);
	void changePriority(bool inc);

	LRESULT onItemChangedQueue(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLISTVIEW* lv = (NMLISTVIEW*)pnmh;
		if((lv->uNewState & LVIS_SELECTED) != (lv->uOldState & LVIS_SELECTED))
			updateStatus();
		return 0;
	}

	LRESULT onSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) {
		ctrlQueue.SetFocus();
		return 0;
	}

	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		usingDirMenu ? removeSelectedDir() : removeSelected();
		return 0;
	}

	LRESULT onMove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		usingDirMenu ? moveSelectedDir() : moveSelected();
		return 0;
	}

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
		if(kd->wVKey == VK_DELETE) {
			removeSelected();
		} else if(kd->wVKey == VK_ADD){
			// Increase Item priority
			changePriority(true);
		} else if(kd->wVKey == VK_SUBTRACT){
			// Decrease item priority
			changePriority(false);
		} else if(kd->wVKey == VK_TAB) {
			onTab();
		}
		return 0;
	}

	LRESULT onKeyDownDirs(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMTVKEYDOWN* kd = (NMTVKEYDOWN*) pnmh;
		if(kd->wVKey == VK_DELETE) {
			removeSelectedDir();
		} else if(kd->wVKey == VK_TAB) {
			onTab();
		}
		return 0;
	}

	void onTab();

	LRESULT onShowTree(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		showTree = (wParam == BST_CHECKED);
		UpdateLayout(FALSE);
		return 0;
	}
	
private:

	enum {
		COLUMN_FIRST,
		COLUMN_TARGET = COLUMN_FIRST,
		COLUMN_STATUS,
		COLUMN_SEGMENTS,
		COLUMN_SIZE,
		COLUMN_PROGRESS,
		COLUMN_DOWNLOADED,
		COLUMN_PRIORITY,
		COLUMN_USERS,
		COLUMN_PATH,
		COLUMN_EXACT_SIZE,
		COLUMN_ERRORS,
		COLUMN_ADDED,
		COLUMN_TTH,
		COLUMN_LAST
	};
	enum Tasks {
		ADD_ITEM,
		ADD_BUNDLE,
		REMOVE_ITEM,
		REMOVE_BUNDLE,
		UPDATE_ITEM,
		UPDATE_BUNDLE,
		UPDATE_STATUS,
		UPDATE_STATUS_ITEMS,
	};


	class BundleItemInfo;
	friend class BundleItemInfo;

	class BundleItemInfo {
	public:
		BundleItemInfo(const string dir, BundlePtr aBundle = NULL) : directory(dir)	{
			if (aBundle) {
				bundles.push_back(aBundle);
			}
		}
		~BundleItemInfo() { }

		const string& getDir() const { return directory; }
		void addBundle(BundlePtr aBundle);
		void removeBundle(BundlePtr aBundle);
		tstring getBundleName(bool remove);
		int countFileBundles();
		BundleList& getBundles() { return bundles; }

		/*const BundlePtr getBundle() const { return b; }
		string getPath() const { return Util::getFilePath(getTarget()); }

		bool isSet(Flags::MaskType aFlag) const { return (b->getFlags() & aFlag) == aFlag; }

		const string& getTarget() const { return b->getTarget(); }

		int64_t getSize() const { return b->getSize(); }
		int64_t getDownloadedBytes() const { return b->getDownloadedBytes(); }

		time_t getAdded() const { return b->getAdded(); }

		Bundle::Priority getPriority() const { return b->getPriority(); }
		//bool isWaiting() const { return b->isRunning(); }
		bool isFinished() const { return b->isFinished(); }

		bool getAutoPriority() const { return b->getAutoPriority(); } */
	private:
		//BundlePtr b;
		BundleList bundles;
		string directory;

		BundleItemInfo(const BundleItemInfo&);
		BundleItemInfo& operator=(const BundleItemInfo&);
	};
	

	class QueueItemInfo;
	friend class QueueItemInfo;
	
	class QueueItemInfo : public FastAlloc<QueueItemInfo> {
	public:

		QueueItemInfo(QueueItemPtr aQI) : qi(aQI)	{ }

		~QueueItemInfo() { }

		void remove() { QueueManager::getInstance()->remove(getTarget()); }

		// TypedListViewCtrl functions
		const tstring getText(int col) const;

		static int compareItems(const QueueItemInfo* a, const QueueItemInfo* b, int col) {
			switch(col) {
				case COLUMN_SIZE: case COLUMN_EXACT_SIZE: return compare(a->getSize(), b->getSize());
				case COLUMN_PRIORITY: return compare((int)a->getPriority(), (int)b->getPriority());
				case COLUMN_DOWNLOADED: return compare(a->getDownloadedBytes(), b->getDownloadedBytes());
				case COLUMN_ADDED: return compare(a->getAdded(), b->getAdded());
				default: return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str());
			}
		}
		int getImageIndex() const { return WinUtil::getIconIndex(Text::toT(getTarget()));	}

		const BundlePtr getBundle() const { return qi->getBundle(); }
		const QueueItemPtr getQueueItem() const { return qi; }
		string getPath() const { return Util::getFilePath(getTarget()); }

		bool isSet(Flags::MaskType aFlag) const { return (qi->getFlags() & aFlag) == aFlag; }

		const string& getTarget() const { return qi->getTarget(); }

		int64_t getSize() const { return qi->getSize(); }
		int64_t getDownloadedBytes() const { return qi->getDownloadedBytes(); }

		time_t getAdded() const { return qi->getAdded(); }
		const TTHValue& getTTH() const { return qi->getTTH(); }

		QueueItem::Priority getPriority() const { return qi->getPriority(); }
		bool isWaiting() const { return qi->isWaiting(); }
		bool isFinished() const { return qi->isFinished(); }

		bool getAutoPriority() const { return qi->getAutoPriority(); }

	private:
		QueueItemPtr qi;

		QueueItemInfo(const QueueItemInfo&);
		QueueItemInfo& operator=(const QueueItemInfo&);
	};
		
	struct QueueItemInfoTask : FastAlloc<QueueItemInfoTask>, public Task {
		QueueItemInfoTask(QueueItemInfo* ii_) : ii(ii_) { }
		QueueItemInfo* ii;
	};

	struct BundleItemInfoTask : FastAlloc<BundleItemInfoTask>, public Task {
		BundleItemInfoTask(BundleItemInfo* ii_) : ii(ii_) { }
		BundleItemInfo* ii;
	};

	struct UpdateTask : FastAlloc<UpdateTask>, public Task {
		UpdateTask(const QueueItem& source) : target(source.getTarget()) { }
		string target;
	};

	static int CALLBACK DefaultSort(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/) {
		string* pItem1 = (string*)lParam1;
		string* pItem2 = (string*)lParam2;

		return Util::DefaultSort(Text::toT(*pItem1).c_str(), Text::toT(*pItem2).c_str());
	}

	TaskQueue tasks;
	bool spoken;

	OMenu browseMenu;
	OMenu removeMenu;
	OMenu removeAllMenu;
	OMenu pmMenu;
	OMenu readdMenu;

	int PreviewAppsSize;

	CButton ctrlShowTree;
	CContainedWindow showTreeContainer;
	bool showTree;

	bool usingDirMenu;

	bool dirty;

	int menuItems;
	int readdItems;

	HTREEITEM fileLists;
	HTREEITEM tempItems;

	typedef boost::unordered_multimap<string, QueueItemInfo*> DirectoryMap;
	typedef boost::unordered_map<string, HTREEITEM> BundleMap;
	typedef DirectoryMap::iterator DirectoryIter;
	typedef DirectoryMap::const_iterator DirectoryIterC;
	typedef pair<DirectoryIterC, DirectoryIterC> DirectoryPairC;
	DirectoryMap directories;
	BundleMap bundleMap;
	string curDir;

	TypedListViewCtrl<QueueItemInfo, IDC_QUEUE> ctrlQueue;
	CTreeViewCtrl ctrlDirs;
	
	CStatusBarCtrl ctrlStatus;
	int statusSizes[6];
	
	int64_t queueSize;
	int queueItems;

	bool closed;
	
	static int columnIndexes[COLUMN_LAST];
	static int columnSizes[COLUMN_LAST];

	void addQueueList(const QueueItem::StringMap& l);
	void addQueueItem(QueueItemInfo* qi, bool noSort);
	HTREEITEM addItemDir(bool isFileList);
	HTREEITEM addBundleDir(const string& dir, const BundlePtr aBundle, HTREEITEM startAt = NULL);
	HTREEITEM createDir(TVINSERTSTRUCT& tvi, const string& dir, const BundlePtr aBundle, HTREEITEM parent, bool subDir=false);
	HTREEITEM createSplitDir(TVINSERTSTRUCT& tvi, const string& dir, HTREEITEM parent, BundleItemInfo* bii, bool subDir=false);
	void removeQueueItem(QueueItemInfo* ii, bool noSort);
	void removeItemDir(bool isFileList);
	void removeBundleDir(const string& dir, const BundlePtr aBundle);
	void QueueFrame::removeBundle(const BundlePtr aBundle);

	void updateQueue();
	void updateStatus();
	
	/**
	 * This one is different from the others because when a lot of files are removed
	 * at the same time, the WM_SPEAKER messages seem to get lost in the handling or
	 * something, they're not correctly processed anyway...thanks windows.
	 */
	void speak(Tasks t, Task* p) {
        tasks.add(static_cast<uint8_t>(t), unique_ptr<Task>(p));
		if(!spoken) {
			spoken = true;
			PostMessage(WM_SPEAKER);
		}
	}

	bool isCurDir(const string& aDir) const { return stricmp(curDir, aDir) == 0; }

	void moveSelected();	
	void moveSelectedDir();

	void moveNode(HTREEITEM item, HTREEITEM parent);

	void clearTree(HTREEITEM item);

	QueueItemInfo* getItemInfo(const string& target) const;

	void removeSelected();
	void removeSelectedDir();
	
	const string& getSelectedDir() const {
		HTREEITEM ht = ctrlDirs.GetSelectedItem();
		return ht == NULL ? Util::emptyString : getDir(ctrlDirs.GetSelectedItem());
	}
	
	const string& getDir(HTREEITEM ht) const { dcassert(ht != NULL); return ((BundleItemInfo*)(ctrlDirs.GetItemData(ht)))->getDir(); }

	void on(QueueManagerListener::Added, QueueItemPtr aQI) noexcept;
	void on(QueueManagerListener::BundleMoved, const BundlePtr aBundle) noexcept;
	void on(QueueManagerListener::Removed, const QueueItemPtr aQI) noexcept;
	void on(QueueManagerListener::SourcesUpdated, const QueueItemPtr aQI) noexcept;
	void on(QueueManagerListener::StatusUpdated, const QueueItemPtr aQI) noexcept { on(QueueManagerListener::SourcesUpdated(), aQI); }
	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;
	
	void onRechecked(const string& target, const string& message);
	
	void on(QueueManagerListener::RecheckStarted, const string& target) noexcept;
	void on(QueueManagerListener::RecheckNoFile, const string& target) noexcept;
	void on(QueueManagerListener::RecheckFileTooSmall, const string& target) noexcept;
	void on(QueueManagerListener::RecheckDownloadsRunning, const string& target) noexcept;
	void on(QueueManagerListener::RecheckNoTree, const string& target) noexcept;
	void on(QueueManagerListener::RecheckAlreadyFinished, const string& target) noexcept;
	void on(QueueManagerListener::RecheckDone, const string& target) noexcept;
	void on(QueueManagerListener::Moved, const QueueItemPtr aQI, const string& oldTarget) noexcept;

	void on(QueueManagerListener::BundleSources, const BundlePtr aBundle) noexcept { on(QueueManagerListener::BundlePriority(), aBundle); };
	void on(QueueManagerListener::BundlePriority, const BundlePtr aBundle) noexcept;
	void on(QueueManagerListener::BundleAdded, const BundlePtr aBundle) noexcept;
	void on(QueueManagerListener::BundleRemoved, const BundlePtr aBundle) noexcept;
	void on(QueueManagerListener::BundleFinished, const BundlePtr aBundle) noexcept { on(QueueManagerListener::BundleRemoved(), aBundle); }

	void on(DownloadManagerListener::BundleTick, const BundleList& tickBundles) noexcept;
};

#endif // !defined(QUEUE_FRAME_H)

/**
 * @file
 * $Id: QueueFrame.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
