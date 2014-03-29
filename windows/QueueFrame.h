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


#if !defined(QUEUE_FRAME2_H)
#define QUEUE_FRAME2_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"

#include "../client/QueueManager.h"
#include "../client/TaskQueue.h"

#define STATUS_MSG_MAP 19

class QueueFrame : public MDITabChildWindowImpl<QueueFrame>, public StaticFrame<QueueFrame, ResourceManager::DOWNLOAD_QUEUE, IDC_QUEUE>,
	public CSplitterImpl<QueueFrame>,
	private QueueManagerListener, private DownloadManagerListener, private Async<QueueFrame>
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("QueueFrame"), IDR_QUEUE2, 0, COLOR_3DFACE);

	QueueFrame() : closed(false), statusDirty(true), curSel(TREE_BUNDLES), ctrlStatusContainer(WC_BUTTON, this, STATUS_MSG_MAP) {}

	~QueueFrame() {}

	typedef MDITabChildWindowImpl<QueueFrame> baseClass;

	BEGIN_MSG_MAP(QueueFrame)
		NOTIFY_HANDLER(IDC_QUEUE_LIST, LVN_GETDISPINFO, ctrlQueue.onGetDispInfo)
		NOTIFY_HANDLER(IDC_QUEUE_LIST, LVN_COLUMNCLICK, ctrlQueue.onColumnClick)
		NOTIFY_HANDLER(IDC_QUEUE_LIST, LVN_GETINFOTIP, ctrlQueue.onInfoTip)
		NOTIFY_HANDLER(IDC_QUEUE_LIST, NM_CUSTOMDRAW, onCustomDraw)
		NOTIFY_HANDLER(IDC_QUEUE_LIST, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_QUEUE_LIST, NM_DBLCLK, onDoubleClick)
		NOTIFY_HANDLER(IDC_TREE, TVN_SELCHANGED, onSelChanged)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_TIMER, onTimer)
		COMMAND_ID_HANDLER(IDC_REMOVE_OFFLINE, onRemoveOffline)
		COMMAND_ID_HANDLER(IDC_READD_ALL, onReaddAll)
		CHAIN_MSG_MAP(baseClass)
		CHAIN_MSG_MAP(CSplitterImpl<QueueFrame>)
		ALT_MSG_MAP(STATUS_MSG_MAP)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);

	LRESULT onReaddAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveOffline(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT onSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) {
		ctrlQueue.SetFocus();
		return 0;
	}
	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	LRESULT onSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /* bHandled */) {
		NMTREEVIEW* nmtv = (NMTREEVIEW*)pnmh;
		if (nmtv->itemNew.lParam != -1){
			curSel = nmtv->itemNew.lParam;
			curItem = nmtv->itemNew.hItem;
			updateList();
		}

		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE );

private:
	class QueueItemInfo;
	enum {
		COLUMN_FIRST,
		COLUMN_NAME = COLUMN_FIRST,
		COLUMN_SIZE,
		COLUMN_TYPE,
		COLUMN_PRIORITY,
		COLUMN_STATUS,
		COLUMN_TIMELEFT,
		COLUMN_SPEED,
		COLUMN_SOURCES,
		COLUMN_DOWNLOADED,
		COLUMN_TIME_ADDED,
		COLUMN_TIME_FINISHED,
		COLUMN_PATH,
		COLUMN_LAST
	};

	enum {
		TASK_ADD,
		TASK_REMOVE,
		TASK_UPDATE
	};

	enum {
		TREE_FIRST,
		TREE_BUNDLES = TREE_FIRST,
		TREE_FINISHED,
		TREE_QUEUED,
		TREE_FAILED,
		TREE_PAUSED,
		TREE_LOCATION,
		TREE_FILELIST,
		TREE_TEMP,
		TREE_LAST
	};

	/*
	Currently its treating every QueueItemInfo as a bundle or Qi and information is called directly, we might want to cache the needed infos.
	*/
	class QueueItemInfo : public FastAlloc<QueueItemInfo>, boost::noncopyable {
	public:
		/*
		Constructors with bundle and QueueItem, even if we might want to change to caching information of the items, 
		these 2 basic constructors could feed the correct information.
		*/
		QueueItemInfo(const BundlePtr& aBundle) : bundle(aBundle), qi(nullptr), collapsed(true), parent(NULL), hits(-1), childrenCreated(aBundle->isFileBundle()) {}
		QueueItemInfo(const QueueItemPtr& aQi) : bundle(nullptr), qi(aQi), collapsed(true), parent(NULL), hits(-1), childrenCreated(true) {}
		
		~QueueItemInfo() {
			dcdebug("itemInfo destructed %s \r\n", bundle ? bundle->getName().c_str() : qi->getTargetFileName().c_str());
		}

		BundlePtr bundle;
		QueueItemPtr qi;

		QueueItemInfo* parent;
		bool collapsed;
		int16_t hits;
		bool childrenCreated;

		inline const string& getGroupCond() const { 
			if (bundle) {
				return bundle->getToken();
			} else if(qi->getBundle()) {
				return qi->getBundle()->getToken();
			} else {
				return qi->getTarget();
			}
		}

		tstring getName() const;
		tstring getType() const;
		tstring getStatusString() const;
		int getPriority() const;
		int64_t getDownloadedBytes() const;
		int64_t getSize() const;
		int64_t getSpeed() const;
		uint64_t getSecondsLeft() const;
		time_t getTimeAdded() const;
		time_t getTimeFinished() const;
		tstring getSourceString() const;
		bool isFinished() const;
		bool isTempItem() const;
		bool isFilelist() const;
		bool isFailed() const;
		bool isPaused() const;
		const string& getTarget() const;
		double getPercentage() const;


		QueueItemInfo* createParent() { return this; }

		const tstring getText(int col) const;
		static int compareItems(const QueueItemInfo* a, const QueueItemInfo* b, int col);
		int getImageIndex() const;

	};

	static int columnIndexes[COLUMN_LAST];
	static int columnSizes[COLUMN_LAST];

	void onRenameBundle(BundlePtr b);
	void onBundleAdded(const BundlePtr& aBundle);
	void onBundleRemoved(const BundlePtr& aBundle, const string& aPath);
	void onBundleUpdated(const BundlePtr& aBundle);

	void insertItems(QueueItemInfo* Qii);

	void onQueueItemRemoved(const QueueItemPtr& aQI);
	void onQueueItemUpdated(const QueueItemPtr& aQI);
	void onQueueItemAdded(const QueueItemPtr& aQI);

	void AppendBundleMenu(BundleList& bl, OMenu& bundleMenu);
	void AppendQiMenu(QueueItemList& ql, OMenu& fileMenu);
	void AppendTreeMenu(BundleList& bl, QueueItemList& queueItems, OMenu& bundleMenu);

	static tstring handleCopyMagnet(const QueueItemInfo* ii);
	void handleMoveBundles(BundleList bl);
	void handleRemoveBundles(BundleList bl, bool removeFinished);
	void handleRemoveFiles(QueueItemList ql, bool removeFinished);
	void handleSearchQI(const QueueItemPtr& aQI);
	void handleCheckSFV(bool treeMenu);
	void handleOpenFile(const QueueItemPtr& aQI);
	void handleOpenFolder();
	void handleSearchDirectory();

	void getSelectedItems(BundleList& bl, QueueItemList& ql, DWORD aFlag = LVNI_SELECTED);
	tstring formatUser(const Bundle::BundleSource& bs) const;
	tstring formatUser(const QueueItem::Source& s) const;
	
	void updateList();
	bool show(const QueueItemInfo* Qii) const;
	QueueItemInfo* findQueueItem(const QueueItemPtr& aQI);
	void updateCollapsedState(QueueItemInfo* aQii, int pos);

	bool closed;
	int curSel;

	typedef TypedTreeListViewCtrl<QueueItemInfo, IDC_QUEUE_LIST, string, noCaseStringHash, noCaseStringEq, NO_GROUP_UNIQUE_CHILDREN | VIRTUAL_CHILDREN> ListType;
	ListType ctrlQueue;

	CTreeViewCtrl ctrlTree;
	void FillTree();

	CStatusBarCtrl ctrlStatus;
	int statusSizes[6];
	CContainedWindow ctrlStatusContainer;

	TaskQueue tasks;

	void addGuiTask(uint8_t task, std::function<void()> f) {
		tasks.add(task, unique_ptr<AsyncTask>(new AsyncTask(f)));
	}

	void executeGuiTasks();
	void updateStatus();
	bool statusDirty;
	COLORREF getStatusColor(uint8_t status);

	void addLocationItem(const string& aPath);
	void removeLocationItem(const string& aPath);
	string getBundleParent(const BundlePtr aBundle);

	HTREEITEM addTreeItem(const HTREEITEM& parent, int item, const tstring& name, HTREEITEM insertAfter = TVI_LAST) {
		TVINSERTSTRUCT tvis = { 0 };
		tvis.hParent = parent;
		tvis.hInsertAfter = insertAfter;
		tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
		tvis.item.pszText = (LPWSTR)name.c_str();
		tvis.item.iImage = item;
		tvis.item.iSelectedImage = item;
		tvis.item.lParam = item;
		return ctrlTree.InsertItem(&tvis);
	}

	struct treeLocationItem {
		treeLocationItem(HTREEITEM aItem) : item(aItem), bundles(1) {};
		int bundles;
		HTREEITEM item;
	};

	std::unordered_map<string, treeLocationItem, noCaseStringHash, noCaseStringEq> locations;
	HTREEITEM bundleParent;
	HTREEITEM locationParent;
	HTREEITEM curItem;

	//bundle update listeners
	void on(QueueManagerListener::BundleAdded, const BundlePtr& aBundle) noexcept;
	void on(QueueManagerListener::BundleRemoved, const BundlePtr& aBundle) noexcept;
	void on(QueueManagerListener::BundleMoved, const BundlePtr& aBundle) noexcept;
	void on(QueueManagerListener::BundleMerged, const BundlePtr& aBundle, const string&) noexcept;
	void on(QueueManagerListener::BundleSize, const BundlePtr& aBundle) noexcept;
	void on(QueueManagerListener::BundlePriority, const BundlePtr& aBundle) noexcept;
	void on(QueueManagerListener::BundleStatusChanged, const BundlePtr& aBundle) noexcept;
	void on(QueueManagerListener::BundleSources, const BundlePtr& aBundle) noexcept;

	void on(DownloadManagerListener::BundleTick, const BundleList& tickBundles, uint64_t aTick) noexcept;

	//QueueItem update listeners
	void on(QueueManagerListener::Removed, const QueueItemPtr& aQI, bool /*finished*/) noexcept;
	void on(QueueManagerListener::Added, QueueItemPtr& aQI) noexcept;
	void on(QueueManagerListener::SourcesUpdated, const QueueItemPtr& aQI) noexcept;
	void on(QueueManagerListener::StatusUpdated, const QueueItemPtr& aQI) noexcept;



	template<typename SourceType>
	void appendUserMenu(OMenu& aMenu, const vector<SourceType>& aSources) {
		auto browseMenu = aMenu.createSubMenu(TSTRING(BROWSE_FILE_LIST), true);
		auto getListMenu = aMenu.createSubMenu(TSTRING(GET_FILE_LIST), true);
		auto pmMenu = aMenu.createSubMenu(TSTRING(SEND_PRIVATE_MESSAGE), true);

		for (auto& s : aSources) {
			auto u = s.getUser();
			auto nick = formatUser(s);

			// get list
			getListMenu->appendItem(nick, [=] {
				try {
					QueueManager::getInstance()->addList(u, QueueItem::FLAG_CLIENT_VIEW);
				} catch (const QueueException& e) {
					ctrlStatus.SetText(1, Text::toT(e.getError()).c_str());
				}
			});

			// browse list
			browseMenu->appendItem(nick, [=] {
				try {
					QueueManager::getInstance()->addList(u, QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_PARTIAL_LIST);
				} catch (const QueueException& e) {
					ctrlStatus.SetText(1, Text::toT(e.getError()).c_str());
				}
			});

			// PM
			if (s.getUser().user->isOnline()) {
				pmMenu->appendItem(nick, [=] { PrivateFrame::openWindow(u); });
			}
		}
	}
};
#endif // !defined(QUEUE_FRAME_H)