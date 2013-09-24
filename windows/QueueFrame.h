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

#if !defined(QUEUE_FRAME_H)
#define QUEUE_FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CSplitterEx.h"
#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"

#include "../client/QueueManager.h"
#include "../client/FastAlloc.h"
#include "../client/TaskQueue.h"

#include <boost/noncopyable.hpp>

#define SHOWTREE_MESSAGE_MAP 12

class QueueFrame : public MDITabChildWindowImpl<QueueFrame>, public StaticFrame<QueueFrame, ResourceManager::DOWNLOAD_QUEUE, IDC_QUEUE>,
	private QueueManagerListener, private DownloadManagerListener, public CSplitterEx<QueueFrame>, private SettingsManagerListener
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("QueueFrame"), IDR_QUEUE, 0, COLOR_3DFACE);

	QueueFrame() : menuItems(0), queueSize(0), queueItems(0), spoken(false), dirty(false), 
		usingDirMenu(false),  readdItems(0), fileLists(NULL), tempItems(NULL), showTree(true), closed(false),
		showTreeContainer(WC_BUTTON, this, SHOWTREE_MESSAGE_MAP) 
	{
	}

	~QueueFrame();
	
	typedef MDITabChildWindowImpl<QueueFrame> baseClass;
	typedef CSplitterEx<QueueFrame> splitBase;

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
		MESSAGE_HANDLER(WM_TIMER, onTimer)
		COMMAND_ID_HANDLER(IDC_SEARCH_BUNDLE, onSearchBundle)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchAlternates)
		COMMAND_ID_HANDLER(IDC_SEARCHDIR, onSearchAlternates)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_RECHECK, onRecheck);
		COMMAND_ID_HANDLER(IDC_REMOVE_OFFLINE, onRemoveOffline)
		COMMAND_ID_HANDLER(IDC_READD_ALL, onReaddAll)
		COMMAND_ID_HANDLER(IDC_MOVE, onMove)
		COMMAND_ID_HANDLER(IDC_USE_SEQ_ORDER, onSeqOrder)
		COMMAND_ID_HANDLER(IDC_RENAME, onRenameDir)
		COMMAND_ID_HANDLER(IDC_OPEN_FOLDER, onOpenFolder)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		COMMAND_RANGE_HANDLER(IDC_SEGMENTONE, IDC_SEGMENTTEN, onSegments)
		NOTIFY_HANDLER(IDC_QUEUE, NM_CUSTOMDRAW, onCustomDraw)
		CHAIN_MSG_MAP(splitBase)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(SHOWTREE_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onShowTree)
	END_MSG_MAP()

	LRESULT onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	LRESULT onReaddAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSegments(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPM(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRecheck(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchBundle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onRemoveOffline(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRenameDir(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSeqOrder(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE);
	void removeDir(HTREEITEM ht);
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


	class DirItemInfo;
	friend class DirItemInfo;

	class DirItemInfo {
	public:
		DirItemInfo(const string& dir, BundlePtr aBundle = nullptr);
		~DirItemInfo();

		const string& getDir() const { return directory; }
		void addBundle(BundlePtr aBundle);
		void removeBundle(const string& aDir);
		tstring getBundleName(bool remove);
		int countFileBundles();
		Bundle::StringBundleList& getBundles() { return bundles; }
	private:
		Bundle::StringBundleList bundles;
		string directory;

		DirItemInfo(const DirItemInfo&);
		DirItemInfo& operator=(const DirItemInfo&);
	};
	

	class QueueItemInfo;
	friend class QueueItemInfo;
	
	class QueueItemInfo : public FastAlloc<QueueItemInfo>, boost::noncopyable {
	public:

		QueueItemInfo(QueueItemPtr aQI) : qi(aQI), fileName(aQI->getTargetFileName()) { }

		~QueueItemInfo() { }

		void remove() { QueueManager::getInstance()->removeFile(getTarget()); }

		// TypedListViewCtrl functions
		const tstring getText(int col) const;

		static int compareItems(const QueueItemInfo* a, const QueueItemInfo* b, int col);

		int getImageIndex() const;

		const BundlePtr getBundle() const { return qi->getBundle(); }
		const QueueItemPtr getQueueItem() const { return qi; }
		string getPath() const { return Util::getFilePath(getTarget()); }

		bool isSet(Flags::MaskType aFlag) const { return (qi->getFlags() & aFlag) == aFlag; }

		const string& getTarget() const { return qi->getTarget(); }

		int64_t getSize() const { return qi->getSize(); }
		int64_t getDownloadedBytes() const;

		time_t getAdded() const { return qi->getAdded(); }
		const TTHValue& getTTH() const { return qi->getTTH(); }

		QueueItemBase::Priority getPriority() const { return qi->getPriority(); }
		bool isWaiting() const;
		bool isFinished() const;

		bool getAutoPriority() const { return qi->getAutoPriority(); }

		//cache in case we move the item to avoid threading issues
		GETSET(string, fileName, FileName);
	private:
		QueueItemPtr qi;
	};
		
	struct QueueItemInfoTask : FastAlloc<QueueItemInfoTask>, public Task {
		QueueItemInfoTask(QueueItemInfo* ii_) : ii(ii_) { }
		QueueItemInfo* ii;
	};

	struct DirItemInfoTask : FastAlloc<DirItemInfoTask>, public Task {
		DirItemInfoTask(DirItemInfo* ii_) : ii(ii_) { }
		DirItemInfo* ii;
	};

	struct UpdateTask : FastAlloc<UpdateTask>, public Task {
		UpdateTask(const QueueItemPtr& source) : target(source->getTarget()) { }
		string target;
	};

	static int CALLBACK DefaultSort(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/) {
		string* pItem1 = (string*)lParam1;
		string* pItem2 = (string*)lParam2;

		return Util::DefaultSort(Text::toT(*pItem1).c_str(), Text::toT(*pItem2).c_str());
	}

	TaskQueue tasks;
	bool spoken;

	CButton ctrlShowTree;
	CContainedWindow showTreeContainer;
	bool showTree;

	bool usingDirMenu;

	bool dirty;

	int menuItems;
	int readdItems;

	HTREEITEM fileLists;
	HTREEITEM tempItems;

	typedef unordered_multimap<string, QueueItemInfo*, noCaseStringHash, noCaseStringEq> DirectoryMap;
	typedef unordered_map<string, HTREEITEM> BundleMap;
	typedef DirectoryMap::iterator DirectoryIter;
	typedef DirectoryMap::const_iterator DirectoryIterC;
	typedef pair<DirectoryIterC, DirectoryIterC> DirectoryPairC;
	DirectoryMap directories;
	BundleMap bundleMap;
	string curDir;

	typedef TypedListViewCtrl<QueueItemInfo, IDC_QUEUE> ListType;
	ListType ctrlQueue;
	CTreeViewCtrl ctrlDirs;
	
	CStatusBarCtrl ctrlStatus;
	int statusSizes[6];
	
	int64_t queueSize;
	int queueItems;

	bool closed;
	bool statusDirty;
	
	static int columnIndexes[COLUMN_LAST];
	static int columnSizes[COLUMN_LAST];

	void addQueueList(const QueueItem::StringMap& l);
	void addQueueItem(QueueItemInfo* qi, bool noSort);
	HTREEITEM addItemDir(bool isFileList);
	HTREEITEM addBundleDir(const string& dir, const BundlePtr& aBundle, HTREEITEM startAt = NULL);
	HTREEITEM createDir(TVINSERTSTRUCT& tvi, string&& dir, const BundlePtr& aBundle, HTREEITEM parent, bool subDir=false);
	HTREEITEM createSplitDir(TVINSERTSTRUCT& tvi, string&& dir, HTREEITEM parent, DirItemInfo* bii, bool subDir=false);
	void removeQueueItem(QueueItemInfo* ii, bool noSort);
	void removeItemDir(bool isFileList);
	void removeBundleDir(const string& dir);
	void removeBundle(const string& aDir, bool isFileBundle);

	void updateQueue();
	void updateStatus();

	static tstring handleCopyMagnet(const QueueItemInfo* ii);
	
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

	bool isCurDir(const string& aDir) const;

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
	
	const string& getDir(HTREEITEM ht) const { dcassert(ht != NULL); return ((DirItemInfo*)(ctrlDirs.GetItemData(ht)))->getDir(); }

	void on(QueueManagerListener::Added, QueueItemPtr& aQI) noexcept;
	void on(QueueManagerListener::Removed, const QueueItemPtr& aQI, bool finished) noexcept;
	void on(QueueManagerListener::SourcesUpdated, const QueueItemPtr& aQI) noexcept;
	void on(QueueManagerListener::StatusUpdated, const QueueItemPtr& aQI) noexcept { on(QueueManagerListener::SourcesUpdated(), aQI); }
	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;
	
	void onRechecked(const string& target, const string& message);
	
	void on(QueueManagerListener::RecheckStarted, const string& target) noexcept;
	void on(QueueManagerListener::RecheckNoFile, const string& target) noexcept;
	void on(QueueManagerListener::RecheckFileTooSmall, const string& target) noexcept;
	void on(QueueManagerListener::RecheckDownloadsRunning, const string& target) noexcept;
	void on(QueueManagerListener::RecheckNoTree, const string& target) noexcept;
	void on(QueueManagerListener::RecheckAlreadyFinished, const string& target) noexcept;
	void on(QueueManagerListener::RecheckDone, const string& target) noexcept;
	void on(QueueManagerListener::Moved, const QueueItemPtr& aQI, const string& oldTarget) noexcept;

	void on(QueueManagerListener::BundleMoved, const BundlePtr& aBundle) noexcept;
	void on(QueueManagerListener::BundleMerged, const BundlePtr& aBundle, const string& oldTarget) noexcept;
	void on(QueueManagerListener::BundleSources, const BundlePtr& aBundle) noexcept { on(QueueManagerListener::BundlePriority(), aBundle); };
	void on(QueueManagerListener::BundleSize, const BundlePtr& aBundle) noexcept { on(QueueManagerListener::BundlePriority(), aBundle); };
	void on(QueueManagerListener::BundlePriority, const BundlePtr& aBundle) noexcept;
	void on(QueueManagerListener::BundleAdded, const BundlePtr& aBundle) noexcept;
	void on(QueueManagerListener::BundleRemoved, const BundlePtr& aBundle) noexcept;
	void on(QueueManagerListener::BundleStatusChanged, const BundlePtr& aBundle) noexcept;

	void on(DownloadManagerListener::BundleTick, const BundleList& tickBundles, uint64_t aTick) noexcept;

	void onBundleRemoved(const BundlePtr& aBundle) noexcept;
};

#endif // !defined(QUEUE_FRAME_H)
