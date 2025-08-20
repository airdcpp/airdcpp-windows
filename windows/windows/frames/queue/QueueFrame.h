/*
* Copyright (C) 2011-2024 AirDC++ Project
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


#ifndef QUEUE_FRAME_H
#define QUEUE_FRAME_H

#include <windows/Async.h>
#include <windows/frames/StaticFrame.h>
#include <windows/components/TypedListViewCtrl.h>
#include <windows/components/FilteredListViewCtrl.h>
#include <windows/components/BrowserBar.h>
#include <windows/frames/private_chat/PrivateFrame.h>
#include <windows/util/ActionUtil.h>

#include <airdcpp/queue/Bundle.h>
#include <airdcpp/transfer/download/DownloadManagerListener.h>
#include <airdcpp/queue/QueueManagerListener.h>
#include <airdcpp/queue/QueueItem.h>
#include <airdcpp/core/classes/Pointer.h>
#include <airdcpp/core/queue/TaskQueue.h>

#define STATUS_MSG_MAP 19
//#define CONTROL_MSG_MAP 22

namespace wingui {
class QueueFrame : public MDITabChildWindowImpl<QueueFrame>, public StaticFrame<QueueFrame, ResourceManager::DOWNLOAD_QUEUE, IDC_QUEUE>,
	public CSplitterImpl<QueueFrame>,
	private QueueManagerListener, private DownloadManagerListener, private SettingsManagerListener, private Async<QueueFrame>
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("QueueFrame"), IDR_QUEUE2, CS_DBLCLKS, COLOR_3DFACE);

	QueueFrame();

	~QueueFrame() {}

	typedef MDITabChildWindowImpl<QueueFrame> baseClass;

	BEGIN_MSG_MAP(QueueFrame)
		NOTIFY_HANDLER(IDC_QUEUE_LIST, LVN_GETDISPINFO, ctrlQueue.list.onGetDispInfo)
		NOTIFY_HANDLER(IDC_QUEUE_LIST, LVN_COLUMNCLICK, ctrlQueue.list.onColumnClick)
		NOTIFY_HANDLER(IDC_QUEUE_LIST, LVN_GETINFOTIP, ctrlQueue.list.onInfoTip)
		NOTIFY_HANDLER(IDC_QUEUE_LIST, NM_CUSTOMDRAW, onCustomDraw)
		NOTIFY_HANDLER(IDC_QUEUE_LIST, LVN_KEYDOWN, onKeyDownList)
		NOTIFY_HANDLER(IDC_QUEUE_LIST, NM_DBLCLK, onDoubleClick)
		NOTIFY_HANDLER(IDC_TREE, TVN_SELCHANGED, onSelChanged)
		NOTIFY_HANDLER(IDC_TREE, NM_CLICK, onTreeItemClick)
		NOTIFY_HANDLER(IDC_TREE, NM_RCLICK, onTreeItemClick)
		NOTIFY_HANDLER(IDC_TREE, TVN_KEYDOWN, onKeyDownTree)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		MESSAGE_HANDLER(WM_TIMER, onTimer)
		CHAIN_MSG_MAP_MEMBER(browserBar)
		CHAIN_MSG_MAP(baseClass)
		CHAIN_MSG_MAP(CSplitterImpl<QueueFrame>)
		//ALT_MSG_MAP(CONTROL_MSG_MAP)
			//MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		ALT_MSG_MAP(STATUS_MSG_MAP)
		ALT_MSG_MAP(HISTORY_MSG_MAP)
		CHAIN_MSG_MAP_ALT_MEMBER(browserBar, HISTORY_MSG_MAP)
		
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT onSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) {
		ctrlQueue.list.SetFocus();
		return 0;
	}

	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}

	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onKeyDownList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onKeyDownTree(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT onSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /* bHandled */) {
		NMTREEVIEW* nmtv = (NMTREEVIEW*)pnmh;
		if (nmtv->itemNew.lParam != -1){
			curSel = nmtv->itemNew.lParam;
			curItem = nmtv->itemNew.hItem;
			reloadList();
		}

		return 0;
	}

	LRESULT onTreeItemClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /* bHandled */) {
		//Reload the list view when clicking on the tree item currently selected after browsing bundle content
		if (!curDirectory)
			return 0;

		DWORD dwPos = ::GetMessagePos();
		POINT pt;
		pt.x = GET_X_LPARAM(dwPos);
		pt.y = GET_Y_LPARAM(dwPos);

		ctrlTree.ScreenToClient(&pt);

		UINT uFlags;
		HTREEITEM ht = ctrlTree.HitTest(pt, &uFlags);
		if (ht && ht == curItem)
			reloadList();

		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE );
	void createColumns();
	size_t getTotalListItemCount() { return curDirectory ? curDirectory->children.size() : parents.size(); }
	void filterList();

	static string id;

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
		TASK_BUNDLE_STATUS,
		TASK_BUNDLE_UPDATE,
		TASK_QI_UPDATE
	};

	enum {
		TREE_FIRST,
		TREE_BUNDLES = TREE_FIRST,
		TREE_FINISHED,
		TREE_QUEUED,
		TREE_FAILED,
		TREE_PAUSED,
		TREE_AUTOSEARCH,
		TREE_LOCATION,
		TREE_FILELIST,
		TREE_TEMP,
		TREE_LAST
	};

	typedef boost::intrusive_ptr<QueueItemInfo> QueueItemInfoPtr;
	class QueueItemInfo : public intrusive_ptr_base<QueueItemInfo>, boost::noncopyable {
	public:

		QueueItemInfo(const BundlePtr& aBundle) : 
			bundle(aBundle), childrenCreated(aBundle->isFileBundle()) {}
		QueueItemInfo(const QueueItemPtr& aQi, QueueItemInfo* aParent) :
			qi(aQi), parent(aParent), childrenCreated(true) {}
		QueueItemInfo(const tstring& aName, QueueItemInfo* aParent, const string& aTarget = Util::emptyString) : 
			parent(aParent), name(aName), isDirectory(true), target(aTarget) {}
		
		~QueueItemInfo();

		BundlePtr bundle = nullptr;
		QueueItemPtr qi = nullptr;

		//for subdirectory items
		tstring name;
		string target;

		IGETSET(QueueItemInfo*, parent, Parent, nullptr);
		IGETSET(int64_t, finshedbytes, FinishedBytes, 0);
		IGETSET(int64_t, totalsize, TotalSize, -1);

		unordered_map<string, QueueItemInfoPtr, noCaseStringHash, noCaseStringEq> children;
		
		bool isDirectory = false;
		bool childrenCreated = false;

		tstring getName() const;
		tstring getType() const;
		tstring getStatusString() const;
		Priority getPriority() const;
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

		const string& getTarget() const;
		double getPercentage() const;

		const tstring getText(int col) const;
		static int compareItems(const QueueItemInfo* a, const QueueItemInfo* b, int col);
		int getImageIndex() const;

		QueueItemInfoPtr addChild(const QueueItemPtr& aQI);
		void updateSubDirectories();
		QueueItemInfoPtr findChild(const string& aKey);
		void getChildQueueItems(QueueItemList& ret);
		void deleteSubdirs();
	};

	static int columnIndexes[COLUMN_LAST];
	static int columnSizes[COLUMN_LAST];

	typedef vector<QueueItemInfoPtr> QueueItemInfoList;

	void onBundleAdded(const BundlePtr& aBundle);
	void onBundleRemoved(const BundlePtr& aBundle, const string& aPath);
	void onBundleUpdated(const BundlePtr& aBundle);

	void insertItems(QueueItemInfoPtr Qii);
	void updateParentDirectories(QueueItemInfoPtr Qii);

	void onQueueItemRemoved(const QueueItemPtr& aQI);
	void onQueueItemUpdated(const QueueItemPtr& aQI);
	void onQueueItemAdded(const QueueItemPtr& aQI);

	void AppendBundleMenu(BundleList& bl, ShellMenu& bundleMenu);
	void AppendQiMenu(QueueItemList& ql, ShellMenu& fileMenu);
	void AppendTreeMenu(BundleList& bl, QueueItemList& queueItems, OMenu& bundleMenu);
	void AppendDirectoryMenu(QueueItemInfoList& dirs, QueueItemList& ql, ShellMenu& dirMenu);

	static tstring handleCopyMagnet(const QueueItemInfo* ii);
	static tstring handleCopyTTH(const QueueItemInfo* ii);

	void handleRecheckFiles(QueueItemList ql);
	void handleRecheckBundles(BundleList bl);
	void handleRemoveBundles(BundleList bl, bool removeFinished, bool aCompletedOnly = false);
	void handleRemoveFiles(QueueItemList ql, bool removeFinished);
	void handleSearchQI(const QueueItemPtr& aQI, bool byName);
	void handleOpenFile(const QueueItemPtr& aQI);
	void handleOpenFolder();
	void handleSearchDirectory();
	void handleItemClick(const QueueItemInfoPtr& aII, bool byHistory = false);
	void handleReaddAll(QueueItemList ql);
	void handleRemoveOffline(QueueItemList ql);

	void handleTab();

	const QueueItemInfoPtr findItemByPath(const string& aPath);
	void handleHistoryClick(const string& aPath, bool byHistory);

	void getSelectedItems(BundleList& bl, QueueItemList& ql, QueueItemInfoList& dirs, DWORD aFlag = LVNI_SELECTED);
	tstring formatUser(const Bundle::BundleSource& bs) const;
	tstring formatUser(const QueueItem::Source& s) const;
	
	void reloadList(bool ByHistory = false);
	bool show(QueueItemInfoPtr& Qii);

	QueueItemInfoPtr findQueueItem(const QueueItemPtr& aQI);

	bool closed;
	int curSel;

	typedef TypedListViewCtrl<QueueItemInfo, IDC_QUEUE_LIST> ListBaseType;
	typedef FilteredListViewCtrl <ListBaseType, QueueFrame, IDC_QUEUE_LIST, FLV_HAS_OPTIONS> ListType;
	friend class ListType;
	ListType ctrlQueue;

	//CContainedWindow listContainer;

	CTreeViewCtrl ctrlTree;
	void FillTree();

	CStatusBarCtrl ctrlStatus;
	int statusSizes[5];
	CContainedWindow ctrlStatusContainer;
	
	BrowserBar<QueueFrame> browserBar;

	/*map of parent items (bundles and queue items without bundle)*/
	unordered_map<QueueToken, QueueItemInfoPtr> parents;

	QueueItemInfo* curDirectory; // currently viewed Directory
	static QueueItemInfoPtr iBack; // dummy item for step back ( Directory .. );

	QueueItemInfoPtr findParent(QueueToken aToken);

	TaskQueue tasks;

	void addGuiTask(uint8_t task, Callback f) {
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

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept override;

	//bundle update listeners
	void on(QueueManagerListener::BundleAdded, const BundlePtr& aBundle) noexcept override;
	void on(QueueManagerListener::BundleRemoved, const BundlePtr& aBundle) noexcept override;
	void on(QueueManagerListener::BundleSize, const BundlePtr& aBundle) noexcept override;
	void on(QueueManagerListener::BundlePriority, const BundlePtr& aBundle) noexcept override;
	void on(QueueManagerListener::BundleStatusChanged, const BundlePtr& aBundle) noexcept override;
	void on(QueueManagerListener::BundleSources, const BundlePtr& aBundle) noexcept override;
	void on(FileRecheckFailed, const QueueItemPtr&, const string&) noexcept override;

	void on(DownloadManagerListener::BundleTick, const BundleList& tickBundles, uint64_t aTick) noexcept override;

	//QueueItem update listeners
	void on(QueueManagerListener::BundleDownloadStatus, const BundlePtr& aBundle) noexcept override;
	void on(QueueManagerListener::ItemRemoved, const QueueItemPtr& aQI, bool /*finished*/) noexcept override;
	void on(QueueManagerListener::ItemAdded, const QueueItemPtr& aQI) noexcept override;
	void on(QueueManagerListener::ItemSources, const QueueItemPtr& aQI) noexcept override;
	void on(QueueManagerListener::ItemStatus, const QueueItemPtr& aQI) noexcept override;
	void on(QueueManagerListener::ItemPriority, const QueueItemPtr& aQI) noexcept override;
	void on(QueueManagerListener::ItemTick, const QueueItemPtr& aQI) noexcept override;

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
				ActionUtil::GetList()(u.user, u.hint);
			});

			// browse list
			browseMenu->appendItem(nick, [=] {
				ActionUtil::BrowseList()(u.user, u.hint);
			});

			// PM
			if (s.getUser().user->isOnline()) {
				pmMenu->appendItem(nick, [=] { PrivateFrame::openWindow(u); });
			}
		}
	}
};

}

#endif // !defined(QUEUE_FRAME_H)