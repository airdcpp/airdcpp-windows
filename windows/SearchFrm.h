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

#if !defined(SEARCH_FRM_H)
#define SEARCH_FRM_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"
#include "RichTextBox.h"
#include "MenuBaseHandlers.h"

#include "../client/Client.h"
#include "../client/SearchManager.h"

#include "../client/ClientManagerListener.h"
#include "../client/FavoriteManager.h"
#include "../client/SearchResult.h"
#include "../client/StringMatch.h"
#include "UCHandler.h"

#define SEARCH_MESSAGE_MAP 6		// This could be any number, really...
#define SHOWUI_MESSAGE_MAP 7
#define FILTER_MESSAGE_MAP 8

class SearchFrame : public MDITabChildWindowImpl<SearchFrame>, 
	private SearchManagerListener, private ClientManagerListener,
	public UCHandler<SearchFrame>, public UserInfoBaseHandler<SearchFrame>, public DownloadBaseHandler<SearchFrame>,
	private SettingsManagerListener, private TimerManagerListener
{
public:
	static void openWindow(const tstring& str = Util::emptyStringW, LONGLONG size = 0, SearchManager::SizeModes mode = SearchManager::SIZE_ATLEAST, const string& type = SEARCH_TYPE_ANY);
	static void closeAll();

	DECLARE_FRAME_WND_CLASS_EX(_T("SearchFrame"), IDR_SEARCH, 0, COLOR_3DFACE)

	typedef MDITabChildWindowImpl<SearchFrame> baseClass;
	typedef UCHandler<SearchFrame> ucBase;
	typedef UserInfoBaseHandler<SearchFrame> uicBase;

	BEGIN_MSG_MAP(SearchFrame)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_GETDISPINFO, ctrlResults.onGetDispInfo)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_COLUMNCLICK, ctrlResults.onColumnClick)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_GETINFOTIP, ctrlResults.onInfoTip)
		NOTIFY_HANDLER(IDC_HUB, LVN_GETDISPINFO, ctrlHubs.onGetDispInfo)
		NOTIFY_HANDLER(IDC_RESULTS, NM_DBLCLK, onDoubleClickResults)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_HUB, LVN_ITEMCHANGED, onItemChangedHub)
		NOTIFY_HANDLER(IDC_RESULTS, NM_CUSTOMDRAW, onCustomDraw)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, onCtlColor)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_DRAWITEM, onDrawItem)
		MESSAGE_HANDLER(WM_MEASUREITEM, onMeasure)
		COMMAND_ID_HANDLER(IDC_VIEW_AS_TEXT, onViewAsText)
		COMMAND_ID_HANDLER(IDC_VIEW_NFO, onViewNfo)
		COMMAND_ID_HANDLER(IDC_MATCH, onMatchPartial)
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
		COMMAND_ID_HANDLER(IDC_USKIPLIST, onUseSkiplist)
		COMMAND_ID_HANDLER(IDC_GETLIST, onGetList)
		COMMAND_ID_HANDLER(IDC_BROWSELIST, onBrowseList)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchByTTH)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES_DIR, onSearchDir)
		COMMAND_ID_HANDLER(IDC_COPY_LINK, onCopy)
		COMMAND_ID_HANDLER(IDC_COPY_TTH, onCopy)
		COMMAND_ID_HANDLER(IDC_PURGE, onPurge)	
		COMMAND_RANGE_HANDLER(IDC_SEARCH_SITES, IDC_SEARCH_SITES + WebShortcuts::getInstance()->list.size(), onSearchSite)
		COMMAND_ID_HANDLER(IDC_OPEN_FOLDER, onOpenFolder)
		COMMAND_ID_HANDLER(IDC_OPEN, onOpen)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		CHAIN_COMMANDS(ucBase)
		CHAIN_COMMANDS(uicBase)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(SEARCH_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CHAR, onChar)
		MESSAGE_HANDLER(WM_KEYDOWN, onChar)
		MESSAGE_HANDLER(WM_KEYUP, onChar)
	ALT_MSG_MAP(SHOWUI_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onShowUI)
	ALT_MSG_MAP(FILTER_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, onCtlColor)
		MESSAGE_HANDLER(WM_KEYUP, onFilterChar)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, onSelChange)
	END_MSG_MAP()

	SearchFrame() : 
	searchBoxContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
		searchContainer(WC_EDIT, this, SEARCH_MESSAGE_MAP), 
		purgeContainer(WC_EDIT, this, SEARCH_MESSAGE_MAP), 
		sizeContainer(WC_EDIT, this, SEARCH_MESSAGE_MAP), 
		modeContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
		sizeModeContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
		fileTypeContainer(WC_COMBOBOXEX, this, SEARCH_MESSAGE_MAP),
		showUIContainer(WC_COMBOBOX, this, SHOWUI_MESSAGE_MAP),
		slotsContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
		collapsedContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
		doSearchContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
		resultsContainer(WC_LISTVIEW, this, SEARCH_MESSAGE_MAP),
		hubsContainer(WC_LISTVIEW, this, SEARCH_MESSAGE_MAP),
		ctrlFilterContainer(WC_EDIT, this, FILTER_MESSAGE_MAP),
		ctrlFilterSelContainer(WC_COMBOBOX, this, FILTER_MESSAGE_MAP),
		ctrlSkiplistContainer(WC_EDIT, this, FILTER_MESSAGE_MAP),
		SkipBoolContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
		initialSize(0), initialMode(SearchManager::SIZE_ATLEAST), initialType(SEARCH_TYPE_ANY),
		showUI(true), onlyFree(false), closed(false), isHash(false), UseSkiplist(false), droppedResults(0), resultsCount(0),
		expandSR(false), exactSize1(false), exactSize2(0), searchEndTime(0), searchStartTime(0), waiting(false)
	{	
		SearchManager::getInstance()->addListener(this);
		useGrouping = BOOLSETTING(GROUP_SEARCH_RESULTS);
	}

	~SearchFrame() {
		images.Destroy();
	}

	LRESULT onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onDrawItem(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onMeasure(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onCtlColor(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onDoubleClickResults(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onSearchByTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onFilterChar(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPurge(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBrowseList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchSite(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchDir(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}

	void updateSkipList();
	void UpdateLayout(BOOL bResizeBars = TRUE);
	void runUserCommand(UserCommand& uc);

	void removeSelected() {
		int i = -1;
		Lock l(cs);
		while( (i = ctrlResults.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			ctrlResults.removeGroupedItem(ctrlResults.getItemData(i));
		}
	}

	LRESULT onOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlResults.forEachSelected(&SearchInfo::open);
		return 0;
	}

	LRESULT onViewAsText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlResults.forEachSelected(&SearchInfo::view);
		return 0;
	}

	LRESULT onViewNfo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlResults.forEachSelected(&SearchInfo::viewNfo);
		return 0;
	}
	
	LRESULT onMatchPartial(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlResults.forEachSelected(&SearchInfo::matchPartial);
		return 0;
	}
	
	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		removeSelected();
		return 0;
	}

	LRESULT onFreeSlots(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		onlyFree = (ctrlSlots.GetCheck() == 1);
		return 0;
	}
	LRESULT onUseSkiplist(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		UseSkiplist = (ctrlSkipBool.GetCheck() == 1);
		return 0;
	}

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
	void appendDownloadItems(OMenu& aMenu, bool isWhole);
	void handleDownload(const string& aTarget, QueueItem::Priority p, bool usingTree, TargetUtil::TargetType aTargetType, bool isSizeUnknown);
	int64_t getDownloadSize(bool isWhole);
	bool showDirDialog(string& fileName);
private:
	class SearchInfo;
	
public:	
	typedef TypedTreeListViewCtrl<SearchInfo, IDC_RESULTS, TTHValue, hash<TTHValue*>, equal_to<TTHValue*>> SearchInfoList;
	SearchInfoList& getUserList() { return ctrlResults; }

private:
	enum {
		COLUMN_FIRST,
		COLUMN_FILENAME = COLUMN_FIRST,
		COLUMN_HITS,
		COLUMN_NICK,
		COLUMN_TYPE,
		COLUMN_SIZE,
		COLUMN_PATH,
		COLUMN_SLOTS,
		COLUMN_CONNECTION,
		COLUMN_HUB,
		COLUMN_EXACT_SIZE,
		COLUMN_IP,		
		COLUMN_TTH,
		COLUMN_LAST
	};

	enum Images {
		IMAGE_UNKOWN,
		IMAGE_SLOW,
		IMAGE_NORMAL,
		IMAGE_FAST
	};

	enum FilterModes{
		NONE,
		EQUAL,
		GREATER_EQUAL,
		LESS_EQUAL,
		GREATER,
		LESS,
		NOT_EQUAL
	};

	class SearchInfo : public UserInfoBase {
	public:
		typedef SearchInfo* Ptr;
		typedef vector<Ptr> List;
		typedef List::const_iterator Iter;

		SearchInfo::List subItems;

		SearchInfo(const SearchResultPtr& aSR);
		~SearchInfo() {	}

		const UserPtr& getUser() const { return sr->getUser(); }
		const string& getHubUrl() const { return sr->getHubURL(); }

		bool collapsed;
		size_t hits;
		SearchInfo* parent;

		void getList();
		void browseList();

		void open();
		void view();
		void viewNfo();
		void matchPartial();
		struct Download {
			Download(const string& aTarget, SearchFrame* aSf, QueueItem::Priority aPrio, bool aNoAppend, TargetUtil::TargetType aTargetType, bool aIsSizeUnknown) : tgt(aTarget), 
				sf(aSf), p(aPrio), noAppend(aNoAppend), targetType(aTargetType), unknownSize(aIsSizeUnknown) { }
			void operator()(SearchInfo* si);

			const string& tgt;
			SearchFrame* sf;
			QueueItem::Priority p;
			bool noAppend;
			TargetUtil::TargetType targetType;
			bool unknownSize;
		};
		struct DownloadWhole {
			DownloadWhole(const string& aTarget, QueueItem::Priority aPrio, TargetUtil::TargetType aTargetType, bool aIsSizeUnknown) : tgt(aTarget), p(aPrio), targetType(aTargetType), 
				unknownSize(aIsSizeUnknown) { }
				void operator()(SearchInfo* si);
			const string& tgt;
			QueueItem::Priority p;
			TargetUtil::TargetType targetType;
			bool unknownSize;
		};
		struct CheckTTH {
			CheckTTH() : op(true), firstHubs(true), hasTTH(false), firstTTH(true) { }
				void operator()(SearchInfo* si);
			bool firstHubs;
			StringList hubs;
			bool op;
			bool hasTTH;
			bool firstTTH;
			tstring tth;
		};
	
		const tstring getText(uint8_t col) const;
	
		static int compareItems(const SearchInfo* a, const SearchInfo* b, uint8_t col) {
			if(!a->sr || !b->sr)
				return 0;
	
			switch(col) {
				// I think its nicer to sort the names too, otherwise could do it with typecolumn
				case COLUMN_FILENAME: 
					if(a->sr->getType() == b->sr->getType())
						return lstrcmpi(a->getText(COLUMN_FILENAME).c_str(), b->getText(COLUMN_FILENAME).c_str());
					else 
						return ( a->sr->getType() == SearchResult::TYPE_DIRECTORY ) ? -1 : 1;

				case COLUMN_TYPE: 
					if(a->sr->getType() == b->sr->getType())
						return lstrcmpi(a->getText(COLUMN_TYPE).c_str(), b->getText(COLUMN_TYPE).c_str());
					else
						return(a->sr->getType() == SearchResult::TYPE_DIRECTORY) ? -1 : 1;
				case COLUMN_HITS: return compare(a->hits, b->hits);
				case COLUMN_SLOTS: 
					if(a->sr->getFreeSlots() == b->sr->getFreeSlots())
						return compare(a->sr->getSlots(), b->sr->getSlots());
					else
						return compare(a->sr->getFreeSlots(), b->sr->getFreeSlots());
				case COLUMN_SIZE:
				case COLUMN_EXACT_SIZE: return compare(a->sr->getSize(), b->sr->getSize());
				default: return lstrcmpi(a->getText(col).c_str(), b->getText(col).c_str());
			}
		}

		int getImageIndex() const;
		
		inline SearchInfo* createParent() { return this; }
		inline const TTHValue& getGroupCond() const { return sr->getTTH(); }

		bool isDupe() const { return dupe != NONE; }
		bool isShareDupe() const { return dupe == SHARE_DUPE || dupe == PARTIAL_SHARE_DUPE; }
		bool isQueueDupe() const { return dupe == QUEUE_DUPE || dupe == FINISHED_DUPE; }
		//string getHubUrl() { return sr->getHubURL(); }

		SearchResultPtr sr;
		GETSET(uint8_t, flagIndex, FlagIndex);
		GETSET(DupeType, dupe, Dupe);
	};
	
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

	// WM_SPEAKER
	enum Speakers {
		ADD_RESULT,
		FILTER_RESULT,
		HUB_ADDED,
		HUB_CHANGED,
		HUB_REMOVED,
		QUEUE_STATS
	};

	tstring initialString;
	int64_t initialSize;
	SearchManager::SizeModes initialMode;
	string initialType;

	CStatusBarCtrl ctrlStatus;
	CEdit ctrlSearch;
	CComboBox ctrlSearchBox;
	CEdit ctrlSize;
	CComboBox ctrlMode;
	CComboBox ctrlSizeMode;
	CComboBoxEx ctrlFiletype;
	CButton ctrlDoSearch;
	CButton ctrlPauseSearch;
	CButton ctrlPurge;	
	
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
	CContainedWindow ctrlFilterContainer;
	CContainedWindow ctrlFilterSelContainer;
	CContainedWindow ctrlSkiplistContainer;
	CContainedWindow SkipBoolContainer;
	tstring filter;
	
	CStatic searchLabel, sizeLabel, optionLabel, typeLabel, hubsLabel, srLabel;
	CButton ctrlSlots, ctrlShowUI, ctrlCollapsed, ctrlSkipBool;
	bool showUI;

	CImageList images;
	SearchInfoList ctrlResults;
	TypedListViewCtrl<HubInfo, IDC_HUB> ctrlHubs;

	TStringList search;
	StringList wholeTargets;
	SearchInfo::List pausedResults;

	CEdit ctrlFilter;
	CComboBox ctrlFilterSel;
	CComboBox ctrlSkiplist;

	StringMatch searchSkipList;

	bool onlyFree;
	bool UseSkiplist;
	bool isHash;
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
	
	CriticalSection cs;
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

	static StringList lastDisabledHubs;

	void onEnter();
	void onTab(bool shift);
	
	void on(SearchManagerListener::SR, const SearchResultPtr& aResult) noexcept;
	void on(TimerManagerListener::Second, uint64_t aTick) noexcept;

	// ClientManagerListener
	void on(ClientConnected, const Client* c) noexcept { speak(HUB_ADDED, c); }
	void on(ClientUpdated, const Client* c) noexcept { speak(HUB_CHANGED, c); }
	void on(ClientDisconnected, const Client* c) noexcept { speak(HUB_REMOVED, c); }
	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;

	void initHubs();
	void onHubAdded(HubInfo* info);
	void onHubChanged(HubInfo* info);
	void onHubRemoved(HubInfo* info);
	bool matchFilter(SearchInfo* si, int sel, bool doSizeCompare = false, FilterModes mode = NONE, int64_t size = 0);
	bool parseFilter(FilterModes& mode, int64_t& size);
	void updateSearchList(SearchInfo* si = NULL);
	void addSearchResult(SearchInfo* si);

	LRESULT onItemChangedHub(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	void speak(Speakers s, const Client* aClient) {
		HubInfo* hubInfo = new HubInfo(Text::toT(aClient->getHubUrl()), Text::toT(aClient->getHubName()), aClient->getMyIdentity().isOp());
		PostMessage(WM_SPEAKER, WPARAM(s), LPARAM(hubInfo)); 
	}
};

#endif // !defined(SEARCH_FRM_H)