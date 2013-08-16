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

#if !defined(SEARCH_FRM_H)
#define SEARCH_FRM_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Async.h"
#include "FlatTabCtrl.h"
#include "FilteredListViewCtrl.h"
#include "UserInfoBaseHandler.h"
#include "DownloadBaseHandler.h"

#include "../client/AdcSearch.h"
#include "../client/Client.h"
#include "../client/SearchManager.h"

#include "../client/ClientManagerListener.h"
#include "../client/SearchResult.h"

#include "UCHandler.h"
#include "SearchTypeCombo.h"
#include "ListFilter.h"

#define SEARCH_MESSAGE_MAP 6		// This could be any number, really...
#define SHOWUI_MESSAGE_MAP 7
#define EXCLUDE_MESSAGE_MAP 8

class SearchFrame : public MDITabChildWindowImpl<SearchFrame>, 
	private SearchManagerListener, private ClientManagerListener,
	public UCHandler<SearchFrame>, public UserInfoBaseHandler<SearchFrame>, public DownloadBaseHandler<SearchFrame>,
	private SettingsManagerListener, private TimerManagerListener, private Async<SearchFrame>
{
public:
	static void openWindow(const tstring& str = Util::emptyStringW, LONGLONG size = 0, SearchManager::SizeModes mode = SearchManager::SIZE_ATLEAST, const string& type = SEARCH_TYPE_ANY);
	static void closeAll();

	DECLARE_FRAME_WND_CLASS_EX(_T("SearchFrame"), IDR_SEARCH, 0, COLOR_3DFACE)

	typedef MDITabChildWindowImpl<SearchFrame> baseClass;
	typedef UCHandler<SearchFrame> ucBase;
	typedef UserInfoBaseHandler<SearchFrame> uicBase;

	BEGIN_MSG_MAP(SearchFrame)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_GETDISPINFO, ctrlResults.list.onGetDispInfo)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_COLUMNCLICK, ctrlResults.list.onColumnClick)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_GETINFOTIP, ctrlResults.list.onInfoTip)
		NOTIFY_HANDLER(IDC_HUB, LVN_GETDISPINFO, ctrlHubs.onGetDispInfo)
		NOTIFY_HANDLER(IDC_RESULTS, NM_DBLCLK, onDoubleClickResults)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_HUB, LVN_ITEMCHANGED, onItemChangedHub)
		NOTIFY_HANDLER(IDC_RESULTS, NM_CUSTOMDRAW, onCustomDraw)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, onCtlColor)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_DRAWITEM, onDrawItem)
		MESSAGE_HANDLER(WM_MEASUREITEM, onMeasure)
		MESSAGE_HANDLER(WM_TIMER, onTimer)
		MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBackground)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_SEARCH, onSearch)
		COMMAND_ID_HANDLER(IDC_SEARCH_PAUSE, onPause)
		COMMAND_ID_HANDLER(IDC_COPY_NICK, onCopy)
		COMMAND_ID_HANDLER(IDC_COPY_FILENAME, onCopy)
		COMMAND_ID_HANDLER(IDC_COPY_DIR, onCopy)
		COMMAND_ID_HANDLER(IDC_COPY_PATH, onCopy)
		COMMAND_ID_HANDLER(IDC_COPY_SIZE, onCopy)
		COMMAND_ID_HANDLER(IDC_FREESLOTS, onFreeSlots)
		COMMAND_ID_HANDLER(IDC_COLLAPSED, onCollapsed)
		COMMAND_ID_HANDLER(IDC_COPY_LINK, onCopy)
		COMMAND_ID_HANDLER(IDC_COPY_TTH, onCopy)
		COMMAND_ID_HANDLER(IDC_PURGE, onPurge)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		COMMAND_ID_HANDLER(IDC_USE_EXCLUDED, onUseExcluded)
		CHAIN_COMMANDS(ucBase)
		CHAIN_COMMANDS(uicBase)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(SEARCH_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CHAR, onChar)
		MESSAGE_HANDLER(WM_KEYDOWN, onChar)
		MESSAGE_HANDLER(WM_KEYUP, onChar)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
	ALT_MSG_MAP(SHOWUI_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onShowUI)
	ALT_MSG_MAP(EXCLUDE_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, onCtlColor)
		//MESSAGE_HANDLER(WM_CHAR, onFilterChar)
		//MESSAGE_HANDLER(WM_KEYUP, onFilterChar)
	END_MSG_MAP()

	SearchFrame();
	~SearchFrame() {
		images.Destroy();
	}

	LRESULT onEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	LRESULT onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onDrawItem(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onMeasure(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onCtlColor(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onDoubleClickResults(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onFilterChar(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onPurge(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE);
	void runUserCommand(UserCommand& uc);

	void removeSelected();

	enum ListType {
		TYPE_FULL,
		TYPE_PARTIAL,
		TYPE_MIXED
	};

	void handleGetList() {
		handleGetList(TYPE_FULL);
	}

	void handleBrowseList() {
		handleGetList(TYPE_PARTIAL);
	}

	void handleGetBrowseList() {
		handleGetList(TYPE_MIXED);
	}

	void handleSearchTTH();
	void handleGetList(ListType aType);
	void handleOpenItem(bool isClientView);
	void handleViewNfo();
	void handleMatchPartial();
	void handleSearchDir();
	void handleOpenFolder();

	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		removeSelected();
		return 0;
	}

	LRESULT onFreeSlots(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		onlyFree = (ctrlSlots.GetCheck() == 1);
		return 0;
	}

	LRESULT onUseExcluded(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onCollapsed(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		expandSR = (ctrlCollapsed.GetCheck() == 1);
		return 0;
	}

	LRESULT onSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		onEnter();
 		return 0;
	}

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
		
		if(kd->wVKey == VK_DELETE) {
			removeSelected();
		} 
		return 0;
	}

	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		if(::IsWindow(ctrlSearch))
			ctrlSearch.SetFocus();
		return 0;
	}

	LRESULT onShowUI(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		showUI = (wParam == BST_CHECKED);
		UpdateLayout(FALSE);
		return 0;
	}

	void SearchFrame::setInitial(const tstring& str, LONGLONG size, SearchManager::SizeModes mode, const string& type) {
		initialString = str; initialSize = size; initialMode = mode; initialType = type; running = true;
	}

	/* DownloadBaseHandler functions */
	void handleDownload(const string& aTarget, QueueItemBase::Priority p, bool usingTree, TargetUtil::TargetType aTargetType, bool isSizeUnknown);
	int64_t getDownloadSize(bool isWhole);
	bool showDirDialog(string& fileName);

	/* FilteredListViewCtrl */
	void createColumns();
	size_t getTotalListItemCount() const;
private:
	class SearchInfo;
	
public:	
	typedef TypedTreeListViewCtrl < SearchInfo, IDC_RESULTS, TTHValue, hash<TTHValue*>, equal_to < TTHValue*>> SearchInfoList;
	typedef FilteredListViewCtrl<SearchInfoList, SearchFrame, IDC_RESULTS> FilteredList;
	SearchInfoList& getUserList() { return ctrlResults.list; }

private:
	enum {
		COLUMN_FIRST,
		COLUMN_FILENAME = COLUMN_FIRST,
		//COLUMN_FILES,
		COLUMN_HITS,
		COLUMN_USERS,
		COLUMN_TYPE,
		COLUMN_SIZE,
		COLUMN_PATH,
		COLUMN_SLOTS,
		COLUMN_CONNECTION,
		COLUMN_HUB,
		COLUMN_EXACT_SIZE,
		COLUMN_IP,		
		COLUMN_TTH,
		COLUMN_DATE,
		COLUMN_LAST
	};

	enum Images {
		IMAGE_UNKOWN,
		IMAGE_SLOW,
		IMAGE_NORMAL,
		IMAGE_FAST
	};

	class SearchInfo : public UserInfoBase {
	public:
		typedef SearchInfo* Ptr;
		typedef vector<Ptr> List;
		typedef List::const_iterator Iter;

		SearchInfo::List subItems;

		SearchInfo(const SearchResultPtr& aSR);
		~SearchInfo() {	}

		const UserPtr& getUser() const { return sr->getUser().user; }
		const string& getHubUrl() const { return sr->getUser().hint; }

		bool collapsed;
		size_t hits;
		SearchInfo* parent;

		struct CheckTTH {
			CheckTTH() : op(true), firstHubs(true), firstPath(true), firstTTH(true) { }

			void operator()(SearchInfo* si);
			bool firstHubs;
			StringList hubs;
			bool op;

			bool firstTTH;
			bool firstPath;
			optional<TTHValue> tth;
			optional<string> path;
		};
	
		const tstring getText(uint8_t col) const;
	
		static int compareItems(const SearchInfo* a, const SearchInfo* b, uint8_t col);

		int getImageIndex() const;
		
		//inline SearchInfo* createParent() { return new SearchInfo(*this); }
		inline SearchInfo* createParent() { return this; }
		inline const TTHValue& getGroupCond() const { return sr->getTTH(); }

		bool isDupe() const { return dupe != DUPE_NONE; }
		bool isShareDupe() const { return dupe == SHARE_DUPE || dupe == PARTIAL_SHARE_DUPE; }
		bool isQueueDupe() const { return dupe == QUEUE_DUPE || dupe == FINISHED_DUPE; }
		//string getHubUrl() { return sr->getHubURL(); }

		SearchResultPtr sr;
		GETSET(uint8_t, flagIndex, FlagIndex);
		GETSET(DupeType, dupe, Dupe);
		GETSET(tstring, ipText, IpText);
	};
	
	void performAction(std::function<void (const SearchInfo* aInfo)> f, bool oncePerParent=false);

	struct HubInfo : public FastAlloc<HubInfo> {
		HubInfo(const tstring& aUrl, const tstring& aName, bool aOp) : url(aUrl),
			name(aName), op(aOp) { }

		inline const TCHAR* getText(uint8_t col) const {
			return (col == 0) ? name.c_str() : Util::emptyStringT.c_str();
		}
		inline static int compareItems(const HubInfo* a, const HubInfo* b, uint8_t col) {
			return (col == 0) ? Util::DefaultSort(a->name.c_str(), b->name.c_str()) : 0;
		}
		uint8_t getImageIndex() const { return 0; }

		tstring url;
		tstring name;
		bool op;
	};

	tstring initialString;
	int64_t initialSize;
	SearchManager::SizeModes initialMode;
	string initialType;

	CStatusBarCtrl ctrlStatus;
	CEdit ctrlSearch;
	CComboBox ctrlSearchBox;
	CEdit ctrlSize;
	CComboBox ctrlSizeMode;
	CComboBox ctrlSizeUnit;
	SearchTypeCombo ctrlFileType;
	CButton ctrlDoSearch;
	CButton ctrlPauseSearch;
	CButton ctrlPurge;
	CEdit ctrlDate;
	CComboBox ctrlDateUnit;
	
	CContainedWindow searchContainer;
	CContainedWindow searchBoxContainer;
	CContainedWindow sizeContainer;
	CContainedWindow modeContainer;
	CContainedWindow sizeModeContainer;
	CContainedWindow fileTypeContainer;
	CContainedWindow slotsContainer;
	CContainedWindow collapsedContainer;
	CContainedWindow showUIContainer;
	CContainedWindow doSearchContainer;
	CContainedWindow resultsContainer;
	CContainedWindow hubsContainer;
	CContainedWindow purgeContainer;
	CContainedWindow excludedContainer;
	CContainedWindow dateContainer;
	CContainedWindow dateUnitContainer;
	
	CStatic searchLabel, sizeLabel, optionLabel, typeLabel, hubsLabel, dateLabel;
	CButton ctrlSlots, ctrlShowUI, ctrlCollapsed, ctrlExcludedBool;
	bool showUI;

	CImageList images;
	FilteredList ctrlResults;
	TypedListViewCtrl<HubInfo, IDC_HUB> ctrlHubs;

	unique_ptr<AdcSearch> curSearch;
	StringList wholeTargets;
	SearchInfo::List pausedResults;

	CComboBox ctrlExcluded;

	bool statusDirty;
	bool usingExcludes;
	bool onlyFree;
	bool expandSR;
	bool running;
	bool exactSize1;
	bool useGrouping;
	bool waiting;
	int64_t exactSize2;
	int64_t resultsCount;

	uint64_t searchEndTime;
	uint64_t searchStartTime;
	tstring target;
	
	SharedMutex cs;
	size_t droppedResults;

	bool closed;

	ParamMap ucLineParams;
	
	std::string token;
		
	static int columnIndexes[];
	static int columnSizes[];

	typedef map<HWND, SearchFrame*> FrameMap;
	typedef FrameMap::const_iterator FrameIter;
	typedef pair<HWND, SearchFrame*> FramePair;

	static FrameMap frames;

	StringList lastDisabledHubs;

	void onEnter();
	void fixControls();

	void onResultFiltered();

	void onTab(bool shift);
	
	void on(SearchManagerListener::SR, const SearchResultPtr& aResult) noexcept;
	void on(TimerManagerListener::Second, uint64_t aTick) noexcept;

	// ClientManagerListener
	void on(ClientConnected, const Client* c) noexcept;
	void on(ClientUpdated, const Client* c) noexcept;
	void on(ClientDisconnected, const string& aHubUrl) noexcept;

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;

	void initHubs();
	void onHubAdded(HubInfo* info);
	void onHubChanged(HubInfo* info);
	void onHubRemoved(tstring&& aHubUrl);
	void updateSearchList(SearchInfo* si = nullptr);
	void addSearchResult(SearchInfo* si);

	LRESULT onItemChangedHub(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
};

#endif // !defined(SEARCH_FRM_H)