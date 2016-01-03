/*
 * Copyright (C) 2001-2015 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(DIRECTORY_LISTING_FRM_H)
#define DIRECTORY_LISTING_FRM_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <airdcpp/User.h>
#include <airdcpp/FastAlloc.h>

#include "FlatTabCtrl.h"
#include "FilteredListViewCtrl.h"
#include "UCHandler.h"
#include "UserInfoBaseHandler.h"
#include "DownloadBaseHandler.h"
#include "TypedTreeCtrl.h"
#include "BrowserBar.h"

#include "Async.h"

#include <airdcpp/concurrency.h>
#include <airdcpp/DirectoryListing.h>
#include <airdcpp/DirectoryListingListener.h>
#include <airdcpp/TargetUtil.h>

#define CONTROL_MESSAGE_MAP 10
#define COMBO_SEL_MAP 11

struct cmdBarButton {
	int id, image;
	ResourceManager::Strings tooltip;
};

static const cmdBarButton cmdBarButtons[] = {
	{IDC_RELOAD_DIR, 0, ResourceManager::RELOAD},
	{IDC_GETLIST, -2, ResourceManager::GET_FULL_LIST},
	{IDC_MATCH_ADL, -2, ResourceManager::MATCH_ADL},
	{IDC_MATCH_QUEUE, -2, ResourceManager::MATCH_QUEUE},
	{IDC_FILELIST_DIFF, -2, ResourceManager::FILE_LIST_DIFF},
	{IDC_FIND, 1, ResourceManager::FIND},
	{IDC_PREV, 2, ResourceManager::PREVIOUS_SHORT},
	{IDC_NEXT, 3, ResourceManager::NEXT},
};

class DirectoryListingFrame : public MDITabChildWindowImpl<DirectoryListingFrame>, public CSplitterImpl<DirectoryListingFrame>,
	public UCHandler<DirectoryListingFrame>, private SettingsManagerListener, public UserInfoBaseHandler<DirectoryListingFrame>,
	public DownloadBaseHandler<DirectoryListingFrame>, private DirectoryListingListener, private Async<DirectoryListingFrame> {
public:
	static void openWindow(const DirectoryListingPtr& aList, const string& aDir, const string& aXML);
	static void closeAll();

	typedef MDITabChildWindowImpl<DirectoryListingFrame> baseClass;
	typedef UCHandler<DirectoryListingFrame> ucBase;
	typedef UserInfoBaseHandler<DirectoryListingFrame> uibBase;

	enum {
		COLUMN_FILENAME,
		COLUMN_TYPE,
		COLUMN_EXACTSIZE,
		COLUMN_SIZE,
		COLUMN_TTH,
		COLUMN_DATE,
		COLUMN_LAST
	};

	enum {
		STATUS_TEXT,
		STATUS_TOTAL_SIZE,
		STATUS_TOTAL_FILES,
		STATUS_SELECTED_FILES,
		STATUS_SELECTED_SIZE,
		STATUS_UPDATED,
		STATUS_HUB,
		STATUS_DUMMY,
		STATUS_LAST
	};

	DirectoryListingFrame(const DirectoryListingPtr& aList);
	~DirectoryListingFrame();


	DECLARE_FRAME_WND_CLASS(_T("DirectoryListingFrame"), IDR_DIRECTORY)

	BEGIN_MSG_MAP(DirectoryListingFrame)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(FTM_CONTEXTMENU, onTabContextMenu)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)

		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)

		NOTIFY_HANDLER(IDC_FILES, LVN_GETDISPINFO, ctrlFiles.list.onGetDispInfo)
		NOTIFY_HANDLER(IDC_FILES, LVN_COLUMNCLICK, ctrlFiles.list.onColumnClick)
		NOTIFY_HANDLER(IDC_FILES, LVN_ITEMCHANGED, onItemChanged)

		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_ITEMEXPANDING, ctrlTree.OnItemExpanding)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_GETDISPINFO, ctrlTree.OnGetItemDispInfo)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_SELCHANGED, onSelChangedDirectories)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_GETINFOTIP, ctrlTree.OnGetChildInfo)

		NOTIFY_HANDLER(IDC_FILES, NM_CUSTOMDRAW, onCustomDrawList)
		NOTIFY_HANDLER(IDC_DIRECTORIES, NM_CUSTOMDRAW, onCustomDrawTree)

		NOTIFY_HANDLER(IDC_FILES, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_FILES, NM_DBLCLK, onDoubleClickFiles)

		NOTIFY_HANDLER(IDC_DIRECTORIES, NM_DBLCLK, onDoubleClickDirs)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_KEYDOWN, onKeyDownDirs)
		NOTIFY_HANDLER(IDC_DIRECTORIES, NM_CLICK, onClickTree)

		COMMAND_ID_HANDLER(ID_FILE_RECONNECT, onFileReconnect)

		COMMAND_ID_HANDLER(IDC_FIND, onFind)
		COMMAND_ID_HANDLER(IDC_NEXT, onNext)
		COMMAND_ID_HANDLER(IDC_PREV, onPrev)

		COMMAND_ID_HANDLER(IDC_RELOAD, onReloadList)
		COMMAND_ID_HANDLER(IDC_RELOAD_DIR, onReloadDir)
		COMMAND_ID_HANDLER(IDC_MATCH_QUEUE, onMatchQueue)
		COMMAND_ID_HANDLER(IDC_MATCH_ADL, onMatchADL)
		COMMAND_ID_HANDLER(IDC_GETLIST, onGetFullList)
		NOTIFY_CODE_HANDLER(TBN_DROPDOWN, onListDiff)

		MESSAGE_HANDLER(WM_EXITMENULOOP, onExitMenuLoop)
		MESSAGE_HANDLER(WM_TIMER, onTimer)
		CHAIN_MSG_MAP_MEMBER(browserBar)

		CHAIN_COMMANDS(ucBase)
		CHAIN_COMMANDS(uibBase)
		CHAIN_MSG_MAP(baseClass)
		CHAIN_MSG_MAP(CSplitterImpl<DirectoryListingFrame>)
		//CHAIN_MSG_MAP_MEMBER(ctrlFiles)
	ALT_MSG_MAP(HISTORY_MSG_MAP)
		CHAIN_MSG_MAP_ALT_MEMBER(browserBar, HISTORY_MSG_MAP)
	ALT_MSG_MAP(CONTROL_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_XBUTTONUP, onXButtonUp)
		MESSAGE_HANDLER(WM_CHAR, onChar)
	ALT_MSG_MAP(COMBO_SEL_MAP)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, onComboSelChanged)
	END_MSG_MAP()

	LRESULT onFileReconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onComboSelChanged(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
	LRESULT onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onDoubleClickFiles(int idCtrl, LPNMHDR pnmh, BOOL& bHandled); 
	LRESULT onDoubleClickDirs(int idCtrl, LPNMHDR pnmh, BOOL& bHandled); 
	LRESULT onSelChangedDirectories(int idCtrl, LPNMHDR pnmh, BOOL& bHandled); 
	LRESULT onClickTree(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled);
	LRESULT onCustomDrawList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onCustomDrawTree(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onXButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onChar(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onMatchADL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGetFullList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onReloadList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
	LRESULT onReloadDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/);

	void UpdateLayout(BOOL bResizeBars = TRUE);
	void findFile(bool findNext);
	void runUserCommand(UserCommand& uc);

	void refreshTree(const string& root, bool reloadList, bool changeDir);

	void selectItem(const string& name);
	
	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);


	LRESULT onSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	void onFind();

	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return 1;
	}

	LRESULT onFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPrev(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onListDiff(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	LRESULT onExitMenuLoop(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onKeyDownDirs(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	void onTab();
	
	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}

	UserListHandler<DirectoryListing> getUserList() { return UserListHandler<DirectoryListing>(*dl); }

	/* DownloadBaseHandler functions */
	int64_t getDownloadSize(bool isWhole);
	void handleDownload(const string& aTarget, QueueItemBase::Priority p, bool usingTree, TargetUtil::TargetType aTargetType, bool isSizeUnknown);
	bool showDirDialog(string& fileName);
private:
	void appendTreeContextMenu(CPoint& pt, DirectoryListing::Directory::Ptr& aDir);
	void appendListContextMenu(CPoint& pt);

	bool checkCommonKey(int key);
	bool allowPopup() const;
	bool getLocalPaths(StringList& paths_, bool usingTree, bool dirsOnly);
	void openDupe(const DirectoryListing::Directory::Ptr& d);
	void openDupe(const DirectoryListing::File::Ptr& f, bool openDir);

	// safe to be called from any thread
	void updateStatus(const tstring& aMsg);
	void updateStatusText(int aTotalCount, int64_t totalSize, int selectedCount, int displayCount, time_t aUpdateDate);

	string curPath;

	void updateToolbarState();
	
	void updateItems(const DirectoryListing::Directory::Ptr& d);
	void insertItems(const optional<string>& selectedName);

	void findSearchHit(bool newDir = false);
	int searchPos;
	bool gotoPrev;

	enum ChangeType {
		CHANGE_LIST,
		CHANGE_TREE_SINGLE,
		CHANGE_TREE_DOUBLE,
		CHANGE_TREE_EXPAND,
		CHANGE_TREE_COLLAPSE,
		CHANGE_HISTORY,
		CHANGE_LAST
	};

	ChangeType changeType;

	void updateStatus();
	void initStatus();
	void up();
	void handleHistoryClick(const string& aPath, bool byHistory);

	class ItemInfo : public FastAlloc<ItemInfo> {
	public:
		enum ItemType {
			FILE,
			DIRECTORY
		} type;
		
		union {
			const DirectoryListing::File::Ptr file;
			const DirectoryListing::Directory::Ptr dir;
		};

		ItemInfo(const DirectoryListing::File::Ptr& f) : type(FILE), file(f), name(Text::toT(f->getName())) { }
		ItemInfo(const DirectoryListing::Directory::Ptr& d) : type(DIRECTORY), dir(d), name(Text::toT(d->getName())) {}
		~ItemInfo() { }

		const tstring getText(uint8_t col) const;
		const string getTextNormal(uint8_t col) const;
		
		/*struct TotalSize {
			TotalSize() : total(0) { }
			void operator()(ItemInfo* a) { total += a->type == DIRECTORY ? a->dir->getTotalSize(true) : a->file->getSize(); }
			int64_t total;
		};*/

		static int compareItems(const ItemInfo* a, const ItemInfo* b, uint8_t col);

		int getImageIndex() const;
		DupeType getDupe() const { return type == DIRECTORY ? dir->getDupe() : file->getDupe(); }
		const string& getName() const { return type == DIRECTORY ? dir->getName() : file->getName(); }
		string getPath() const { return type == DIRECTORY ? dir->getPath() : file->getPath(); }
		bool isAdl() const { return type == DIRECTORY ? dir->getAdls() : file->getAdls(); }

		struct NameSort {
			bool operator()(const ItemInfo& a, const ItemInfo& b) const;
		};

		const tstring& getNameW() const { return name; }
	private:
		//int64_t aTotalSize
		const tstring name;
	};

	unique_ptr<ItemInfo> root;

	void handleItemAction(bool usingTree, std::function<void (const ItemInfo* ii)> aF, bool firstOnly = false);
	void onListItemAction();
	void changeDir(const ItemInfo* d, DirectoryListing::ReloadMode aReload = DirectoryListing::RELOAD_NONE);

	static tstring handleCopyMagnet(const ItemInfo* ii);
	static tstring handleCopyPath(const ItemInfo* ii);
	static tstring handleCopyDirectory(const ItemInfo* ii);

	void handleRefreshShare(bool usingTree);
	void handleScanShare(bool usingTree);
	void handleCheckSfv(bool usingTree);

	void handleCopyDir();
	void handleOpenFile();
	void handleViewAsText();
	void handleViewNFO(bool usingTree);
	void handleGoToDirectory(bool usingTree);

	void handleSearchByName(bool usingTree, bool dirsOnly);
	void handleSearchByTTH();

	void handleReloadPartial(bool dirOnly);

	CContainedWindow treeContainer;
	CContainedWindow listContainer;

	typedef TypedVirtualTreeCtrl<DirectoryListingFrame, ItemInfo> TreeType;
	friend class TreeType;
	TreeType ctrlTree;

	typedef TypedListViewCtrl<ItemInfo, IDC_FILES> ListBaseType;
	typedef FilteredListViewCtrl <ListBaseType, DirectoryListingFrame, IDC_FILES> ListType;
	friend class ListType;
	ListType ctrlFiles;

	CStatusBarCtrl ctrlStatus;
	HTREEITEM treeRoot;
	
	CToolBarCtrl ctrlToolbar;
	void addCmdBarButtons();

	BrowserBar<DirectoryListingFrame> browserBar;

	int skipHits;

	bool updating;
	bool statusDirty;

	int statusSizes[STATUS_LAST];
	
	DirectoryListingPtr dl;

	ParamMap ucLineParams;

	static int columnIndexes[COLUMN_LAST];
	static int columnSizes[COLUMN_LAST];

	typedef map<HWND, DirectoryListingFrame*> FrameMap;
	static FrameMap frames;

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;

	void on(DirectoryListingListener::LoadingFinished, int64_t aStart, const string& aDir, bool reloadList, bool changeDir) noexcept;
	void on(DirectoryListingListener::LoadingFailed, const string& aReason) noexcept;
	void on(DirectoryListingListener::LoadingStarted, bool changeDir) noexcept;
	void on(DirectoryListingListener::QueueMatched, const string& aMessage) noexcept;
	void on(DirectoryListingListener::Close) noexcept;
	void on(DirectoryListingListener::SearchStarted) noexcept;
	void on(DirectoryListingListener::SearchFailed, bool timedOut) noexcept;
	void on(DirectoryListingListener::ChangeDirectory, const string& aDir, bool isSearchChange) noexcept;
	void on(DirectoryListingListener::UpdateStatusMessage, const string& aMessage) noexcept;
	void on(DirectoryListingListener::RemovedQueue, const string& aDir) noexcept;
	void on(DirectoryListingListener::SetActive) noexcept;
	void on(DirectoryListingListener::UserUpdated) noexcept;

	void filterList();
	void createRoot();
	void convertToFull();
	void onLoadingFinished(int64_t aStart, const string& aDir, bool reloadList, bool changeDir);


	CComboBox selCombo;

	void updateSelCombo(bool init = false);
	User::UserInfoList hubs;
	bool online = true;
	void onComboSelChanged(bool manual);
	void showSelCombo(bool show, bool init);
	tstring getComboDesc();

	tstring onlineNicks;
	tstring onlineHubNames;

	CContainedWindow selComboContainer;

	typedef set<ItemInfo, ItemInfo::NameSort> ItemInfoSet;
	struct ItemInfoCache {
		ItemInfoCache(const DirectoryListing::Directory::Ptr& aDir) : dir(aDir) {}

		const DirectoryListing::Directory::Ptr dir;

		ItemInfoSet files;
		ItemInfoSet directories;
	};

	unordered_map<string, unique_ptr<ItemInfoCache>, noCaseStringHash, noCaseStringEq> itemInfos;
	void updateItemCache(const string& aPath, std::function<void()> completionF);
protected:
	/* TypedTreeViewCtrl */
	TreeType::ChildrenState DirectoryListingFrame::getChildrenState(const ItemInfo* d) const;
	int getIconIndex(const ItemInfo* d) const;
	void expandDir(ItemInfo* d, bool /*collapsing*/);
	void insertTreeItems(const string& aPath, HTREEITEM aParent);

	/* FilteredListViewCtrl */
	void createColumns();
	size_t getTotalListItemCount() const;

private:
	typedef std::set<string, Util::PathSortOrderBool> PathSet;

	void updateItemCacheImpl(const string& aPath, std::function<void()> completionF, const PathSet& paths);
};

#endif // !defined(DIRECTORY_LISTING_FRM_H)
