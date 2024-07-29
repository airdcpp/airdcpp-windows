/*
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DIRECTORY_LISTING_FRM_H
#define DIRECTORY_LISTING_FRM_H

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

#include <airdcpp/User.h>
#include <airdcpp/FastAlloc.h>

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

	// Open own filelist
	static void openWindow(ProfileToken aProfile, const string& aDir = ADC_ROOT_STR) noexcept;

	// Open local file
	static void openWindow(const HintedUser& aUser, const string& aFile, const string& aDir = ADC_ROOT_STR) noexcept;

	// Open remote filelist
	static void openWindow(const HintedUser& aUser, Flags::MaskType aFlags, const string& aInitialDir = ADC_ROOT_STR) noexcept;

	static DirectoryListingFrame* findFrame(const UserPtr& aUser) noexcept;
	static void activate(const DirectoryListingPtr& aList) noexcept;

	static void closeAll();
	static bool getWindowParams(HWND hWnd, StringMap &params);
	static bool parseWindowParams(StringMap& params);

	typedef MDITabChildWindowImpl<DirectoryListingFrame> baseClass;
	typedef CSplitterImpl<DirectoryListingFrame> splitBase;
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

		COMMAND_ID_HANDLER(ID_FILE_RECONNECT, onReloadDir)

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

		CHAIN_COMMANDS(uibBase)
		CHAIN_MSG_MAP(splitBase)
		CHAIN_MSG_MAP(baseClass)
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
	void runUserCommand(UserCommand& uc);

	void refreshTree(const string& root, bool aSelectDir);

	HTREEITEM selectItem(const string& aPath);
	
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
	void handleDownload(const string& aTarget, Priority p, bool aUsingTree);
	bool showDirDialog(string& fileName);

	static string id;

	void activate() noexcept;
private:
	void appendTreeContextMenu(CPoint& pt, const HTREEITEM& aTreeItem);
	void appendListContextMenu(CPoint& pt);

	bool checkCommonKey(int key);
	bool allowPopup() const;
	bool getLocalPaths(StringList& paths_, bool usingTree, bool dirsOnly);
	void openDupe(const DirectoryListing::Directory::Ptr& d);
	void openDupe(const DirectoryListing::File::Ptr& f, bool openDir) noexcept;

	// Current list path in the core (may not match the current view path during directory changes)
	const string getCurrentListPath() const noexcept;

	// safe to be called from any thread
	void updateStatus(const tstring& aMsg);
	void updateStatusText(int aTotalCount, int64_t totalSize, int selectedCount, int displayCount, time_t aUpdateDate);

	// string curPath = ADC_ROOT_STR;
	
	void updateItems(const DirectoryListing::Directory::Ptr& d);
	void insertItems(const string& aPath, const optional<string>& aSelectedName);

	void findSearchHit(bool newDir = false);
	int searchPos = 0;
	bool gotoPrev = false;

	enum ChangeType {
		CHANGE_LIST,
		CHANGE_TREE_SINGLE,
		CHANGE_TREE_DOUBLE,
		CHANGE_TREE_EXPAND,
		CHANGE_TREE_COLLAPSE,
		CHANGE_HISTORY,
		CHANGE_LAST
	};

	ChangeType changeType = CHANGE_LIST;

	void updateStatus() noexcept;
	void updateStatus(const DirectoryListing::Directory::Ptr& aCurrentDir) noexcept;
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

		ItemInfo(const DirectoryListing::File::Ptr& f);
		ItemInfo(const DirectoryListing::Directory::Ptr& d);
		~ItemInfo();

		DirectoryListingToken getToken() const noexcept {
			return hash<string>()(type == DIRECTORY ? dir->getName() : file->getName());
		}

		const tstring getText(uint8_t col) const noexcept;
		const string getTextNormal(uint8_t col) const noexcept;
		
		/*struct TotalSize {
			TotalSize() : total(0) { }
			void operator()(ItemInfo* a) { total += a->type == DIRECTORY ? a->dir->getTotalSize(true) : a->file->getSize(); }
			int64_t total;
		};*/

		static int compareItems(const ItemInfo* a, const ItemInfo* b, uint8_t col) noexcept;

		int getImageIndex() const noexcept;
		DupeType getDupe() const noexcept { return type == DIRECTORY ? dir->getDupe() : file->getDupe(); }
		const string& getName() const noexcept { return type == DIRECTORY ? dir->getName() : file->getName(); }
		string getAdcPath() const noexcept { return type == DIRECTORY ? dir->getAdcPath() : file->getAdcPath(); }
		bool isAdl() const noexcept;

		struct NameSort {
			bool operator()(const ItemInfo& a, const ItemInfo& b) const noexcept;
		};

		const tstring& getNameW() const noexcept { return name; }
	private:
		//int64_t aTotalSize
		const tstring name;
	};

	unique_ptr<ItemInfo> root;

	void handleItemAction(bool usingTree, std::function<void (const ItemInfo* ii)> aF, bool firstOnly = false);
	void onListItemAction();
	void loadPath(const ItemInfo* d, DirectoryListing::DirectoryLoadType aChangeType);

	static tstring handleCopyMagnet(const ItemInfo* ii);
	static tstring handleCopyPath(const ItemInfo* ii);
	static tstring handleCopyDirectory(const ItemInfo* ii);

	void handleRefreshShare(bool usingTree);

	void handleCopyDir();
	void handleOpenFile();
	void handleViewAsText();
	void handleViewNFO(bool usingTree);
	void handleGoToDirectory(bool usingTree);

	void handleSearchByName(bool usingTree, bool dirsOnly);
	void handleSearchByTTH();

	void handleReloadPartial();

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
	HTREEITEM treeRoot = nullptr;
	
	CToolBarCtrl ctrlToolbar;
	void addCmdBarButtons();

	BrowserBar<DirectoryListingFrame> browserBar;

	int skipHits = 0;

	bool updating = false;
	bool statusDirty = false;

	int statusSizes[STATUS_LAST];
	
	const DirectoryListingPtr dl;

	ParamMap ucLineParams;

	void listViewSelectSubDir(const string& aSubPath, const string& aParentPath);

	static int columnIndexes[COLUMN_LAST];
	static int columnSizes[COLUMN_LAST];

	typedef map<HWND, DirectoryListingFrame*> FrameMap;
	static FrameMap frames;

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept override;

	void on(DirectoryListingListener::LoadingFinished, int64_t aStart, const string& aDir, uint8_t aType) noexcept override;
	void on(DirectoryListingListener::LoadingFailed, const string& aReason) noexcept override;
	void on(DirectoryListingListener::LoadingStarted, bool changeDir) noexcept override;
	void on(DirectoryListingListener::QueueMatched, const string& aMessage) noexcept override;
	void on(DirectoryListingListener::Close) noexcept override;
	void on(DirectoryListingListener::SearchStarted) noexcept override;
	void on(DirectoryListingListener::SearchFailed, bool aTimedOut) noexcept override;
	void on(DirectoryListingListener::ChangeDirectory, const string& aDir, uint8_t aChangeType) noexcept override;
	void on(DirectoryListingListener::UpdateStatusMessage, const string& aMessage) noexcept override;
	void on(DirectoryListingListener::RemovedQueue, const string& aDir) noexcept override;
	void on(DirectoryListingListener::UserUpdated) noexcept override;
	void on(DirectoryListingListener::ShareProfileChanged) noexcept override;
	void on(DirectoryListingListener::Read) noexcept override;

	void filterList();
	void createRoot();
	void convertToFull();
	void onLoadingFinished(int64_t aStart, const string& aDir, uint8_t aType);


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
		ItemInfoCache() noexcept {}

		ItemInfoSet files;
		ItemInfoSet directories;
	};

	unordered_map<string, unique_ptr<ItemInfoCache>, noCaseStringHash, noCaseStringEq> itemInfos;
	void updateItemCache(const string& aPath);
protected:
	/* TypedTreeViewCtrl */
	TreeType::ChildrenState getChildrenState(const ItemInfo* d) const noexcept;
	int getIconIndex(const ItemInfo* d) const noexcept;
	void expandDir(ItemInfo* d, bool /*collapsing*/) noexcept;
	void insertTreeItems(const string& aPath, HTREEITEM aParent) noexcept;

	/* FilteredListViewCtrl */
	void createColumns() noexcept;
	size_t getTotalListItemCount() const noexcept;

private:
	enum WindowState {
		STATE_ENABLED,
		STATE_DISABLED,
	};

	WindowState windowState = STATE_ENABLED;

	void changeWindowState(bool enable, bool redraw = true);
	void disableBrowserLayout(bool redraw = true);
	void enableBrowserLayout(bool redraw = true);

	typedef std::set<string, Util::PathSortOrderBool> PathSet;

	bool matchADL = false;
};

#endif // !defined(DIRECTORY_LISTING_FRM_H)
