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
#include "ChatCtrl.h"
#include "WinUtil.h"

#include "../client/Client.h"
#include "../client/SearchManager.h"

#include "../client/ClientManagerListener.h"
#include "../client/FavoriteManager.h"
#include "../client/QueueManager.h"
#include "../client/ShareManager.h"
#include "../client/SearchResult.h"
#include "UCHandler.h"

#define SEARCH_MESSAGE_MAP 6		// This could be any number, really...
#define SHOWUI_MESSAGE_MAP 7
#define FILTER_MESSAGE_MAP 8

class SearchFrame : public MDITabChildWindowImpl<SearchFrame>, 
	private SearchManagerListener, private ClientManagerListener,
	public UCHandler<SearchFrame>, public UserInfoBaseHandler<SearchFrame>,
	private SettingsManagerListener, private TimerManagerListener
{
public:
	static void openWindow(const tstring& str = Util::emptyStringW, LONGLONG size = 0, SearchManager::SizeModes mode = SearchManager::SIZE_ATLEAST, SearchManager::TypeModes type = SearchManager::TYPE_ANY);
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
		COMMAND_ID_HANDLER(IDC_DOWNLOAD, onDownload)
		COMMAND_ID_HANDLER(IDC_DOWNLOADTO, onDownloadTo)
		COMMAND_ID_HANDLER(IDC_DOWNLOADDIR, onDownloadWhole)
		COMMAND_ID_HANDLER(IDC_DOWNLOADDIRTO, onDownloadWholeTo)
		COMMAND_ID_HANDLER(IDC_VIEW_AS_TEXT, onViewAsText)
		COMMAND_ID_HANDLER(IDC_VIEW_NFO, onViewNfo)
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
		COMMAND_ID_HANDLER(IDC_BITZI_LOOKUP, onBitziLookup)
		COMMAND_ID_HANDLER(IDC_COPY_LINK, onCopy)
		COMMAND_ID_HANDLER(IDC_COPY_TTH, onCopy)
		COMMAND_ID_HANDLER(IDC_PURGE, onPurge)	
		COMMAND_ID_HANDLER(IDC_GOOGLE_TITLE, onSearchSite)
		COMMAND_ID_HANDLER(IDC_GOOGLE_FULL, onSearchSite)
		COMMAND_ID_HANDLER(IDC_TVCOM, onSearchSite)
		COMMAND_ID_HANDLER(IDC_METACRITIC, onSearchSite)
		COMMAND_ID_HANDLER(IDC_IMDB, onSearchSite)
		COMMAND_ID_HANDLER(IDC_OPEN_FOLDER, onOpenDupe)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_FAVORITE_DIRS, IDC_DOWNLOAD_FAVORITE_DIRS + FavoriteManager::getInstance()->getFavoriteDirs().size(), onDownloadFavoriteDirs)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS, IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS + FavoriteManager::getInstance()->getFavoriteDirs().size(), onDownloadWholeFavoriteDirs)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_TARGET, IDC_DOWNLOAD_TARGET + targets.size() + WinUtil::lastDirs.size(), onDownloadTarget)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_WHOLE_TARGET, IDC_DOWNLOAD_WHOLE_TARGET + WinUtil::lastDirs.size(), onDownloadWholeTarget)
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
		MESSAGE_HANDLER(WM_KEYUP, onSkiplist)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, onSelChange)
	END_MSG_MAP()

	SearchFrame() : 
	searchBoxContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
		searchContainer(WC_EDIT, this, SEARCH_MESSAGE_MAP), 
		purgeContainer(WC_EDIT, this, SEARCH_MESSAGE_MAP), 
		sizeContainer(WC_EDIT, this, SEARCH_MESSAGE_MAP), 
		modeContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
		sizeModeContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
		fileTypeContainer(WC_COMBOBOX, this, SEARCH_MESSAGE_MAP),
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
		initialSize(0), initialMode(SearchManager::SIZE_ATLEAST), initialType(SearchManager::TYPE_ANY),
		showUI(true), onlyFree(false), closed(false), isHash(false), UseSkiplist(false), droppedResults(0), resultsCount(0),
		expandSR(false), exactSize1(false), exactSize2(0), searchEndTime(0), searchStartTime(0), waiting(false)
	{	
		SearchManager::getInstance()->addListener(this);
		useGrouping = BOOLSETTING(GROUP_SEARCH_RESULTS);
	}

	~SearchFrame() {
		images.Destroy();
		searchTypes.Destroy();
	}

	LRESULT onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onDrawItem(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onMeasure(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onCtlColor(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onDoubleClickResults(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onDownloadTarget(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadWholeTarget(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadWholeTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadFavoriteDirs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadWholeFavoriteDirs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onSearchByTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBitziLookup(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onFilterChar(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPurge(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBrowseList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSkiplist(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onPause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchSite(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchDir(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenDupe(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE);
	void runUserCommand(UserCommand& uc);

	void removeSelected() {
		int i = -1;
		Lock l(cs);
		while( (i = ctrlResults.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			ctrlResults.removeGroupedItem(ctrlResults.getItemData(i));
		}
	}
	
	LRESULT onDownload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlResults.forEachSelectedT(SearchInfo::Download(Text::toT(SETTING(DOWNLOAD_DIRECTORY)), this));
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

	LRESULT onDownloadWhole(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlResults.forEachSelectedT(SearchInfo::DownloadWhole(Text::toT(SETTING(DOWNLOAD_DIRECTORY))));
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

	void SearchFrame::setInitial(const tstring& str, LONGLONG size, SearchManager::SizeModes mode, SearchManager::TypeModes type) {
		initialString = str; initialSize = size; initialMode = mode; initialType = type; running = true;
	}

	
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

		SearchInfo(const SearchResultPtr& aSR) : sr(aSR), collapsed(true), parent(NULL), flagIndex(0), hits(0) { 
			
			if(BOOLSETTING(DUPE_SEARCH)) {
				if(sr->getType() == SearchResult::TYPE_DIRECTORY) {
					shareDupe = ShareManager::getInstance()->isDirShared(sr->getFile());
					queueDupe = QueueManager::getInstance()->isDirQueued(sr->getFile());
				} else {
					shareDupe = ShareManager::getInstance()->isTTHShared(sr->getTTH());
					queueDupe = QueueManager::getInstance()->isTTHQueued(sr->getTTH());
				}

				if (!sr->getIP().empty()) {
					// Only attempt to grab a country mapping if we actually have an IP address
					string tmpCountry = Util::getIpCountry(sr->getIP());
					if(!tmpCountry.empty()) {
						flagIndex = WinUtil::getFlagIndexByCode(tmpCountry.c_str());
					}
				}
			}
		}
	

		~SearchInfo() {	}

		const UserPtr& getUser() const { return sr->getUser(); }

		bool collapsed;
		size_t hits;
		SearchInfo* parent;

		void getList();
		void browseList();

		void view();
		void viewNfo();
		struct Download {
			Download(const tstring& aTarget, SearchFrame* aSf) : tgt(aTarget), sf(aSf) { }
			void operator()(SearchInfo* si);
			const tstring& tgt;
			SearchFrame* sf;
		};
		struct DownloadWhole {
			DownloadWhole(const tstring& aTarget) : tgt(aTarget) { }
				void operator()(SearchInfo* si);
			const tstring& tgt;
		};
		struct DownloadTarget {
			DownloadTarget(const tstring& aTarget) : tgt(aTarget) { }
				void operator()(SearchInfo* si);
			const tstring& tgt;
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
	
		const tstring getText(uint8_t col) const {
			switch(col) {
				case COLUMN_FILENAME:
					if(sr->getType() == SearchResult::TYPE_FILE) {
						if(sr->getFile().rfind(_T('\\')) == tstring::npos) {
							return Text::toT(sr->getFile());
						} else {
	    					return Text::toT(Util::getFileName(sr->getFile()));
						}      
					} else {
						return Text::toT(sr->getFileName());
					}
				case COLUMN_HITS: return hits == 0 ? Util::emptyStringT : Util::toStringW(hits + 1) + _T(' ') + TSTRING(USERS);
				case COLUMN_NICK: return WinUtil::getNicks(sr->getUser(), sr->getHubURL());
				case COLUMN_TYPE:
					if(sr->getType() == SearchResult::TYPE_FILE) {
						tstring type = Text::toT(Util::getFileExt(Text::fromT(getText(COLUMN_FILENAME))));
						if(!type.empty() && type[0] == _T('.'))
							type.erase(0, 1);
						return type;
					} else {
						return TSTRING(DIRECTORY);
					}
				case COLUMN_SIZE: 
					if(sr->getType() == SearchResult::TYPE_FILE) {
						return Util::formatBytesW(sr->getSize());
					} else if (SETTING(SETTINGS_PROFILE) == SettingsManager::PROFILE_RAR) {
						return sr->getSize() > 0 ? Util::formatBytesW(sr->getSize()) : Util::emptyStringT;
					} else {
						return Util::emptyStringT;
					}					
				case COLUMN_PATH:
					if(sr->getType() == SearchResult::TYPE_FILE) {
						return Text::toT(Util::getFilePath(sr->getFile()));
					} else {
						return Text::toT(sr->getFile());
					}
				case COLUMN_SLOTS: return Text::toT(sr->getSlotString());
				case COLUMN_CONNECTION: return Text::toT(ClientManager::getInstance()->getConnection(getUser()->getCID()));
				case COLUMN_HUB: return Text::toT(sr->getHubName());
				case COLUMN_EXACT_SIZE: return sr->getSize() > 0 ? Util::formatExactSize(sr->getSize()) : Util::emptyStringT;
				case COLUMN_IP: {
					string ip = sr->getIP();
					if (!ip.empty()) {
						// Only attempt to grab a country mapping if we actually have an IP address
						string tmpCountry = Util::getIpCountry(ip);
						if(!tmpCountry.empty()) {
							ip = tmpCountry + " (" + ip + ")";
						}
					}
					return Text::toT(ip);
				}
				case COLUMN_TTH: return sr->getType() == SearchResult::TYPE_FILE ? Text::toT(sr->getTTH().toBase32()) : Util::emptyStringT;
				default: return Util::emptyStringT;
			}
		}
	
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

		int getImageIndex() const {
			int image = 0;
			if (BOOLSETTING(USE_SYSTEM_ICONS)) {
				image = sr->getType() == SearchResult::TYPE_FILE ? WinUtil::getIconIndex(Text::toT(sr->getFile())) : WinUtil::getDirIconIndex();
			} else {
				string tmp = ClientManager::getInstance()->getConnection(sr->getUser()->getCID());
				if( (tmp == "28.8Kbps") ||
					(tmp == "33.6Kbps") ||
					(tmp == "56Kbps") ||
					(tmp == "Modem") ||
					(tmp == "Satellite") ||
					(tmp == "Wireless") ||
					(tmp == "ISDN") ) {
					image = 1;
				} else if( (tmp == "Cable") ||
					(tmp == "DSL") ) {
					image = 2;
				} else if( (tmp == "LAN(T1)") ||
					(tmp == "LAN(T3)") ) {
					image = 3;
				}
				if(sr->getType() == SearchResult::TYPE_FILE)
					image += 4;
			}
			return image;
		}
		
		inline SearchInfo* createParent() { return this; }
		inline const TTHValue& getGroupCond() const { return sr->getTTH(); }
		
		bool isShareDupe() const { return shareDupe; }
		bool isQueueDupe() const { return queueDupe; }

		SearchResultPtr sr;
		GETSET(uint8_t, flagIndex, FlagIndex);
	private:
		bool shareDupe;
		bool queueDupe;
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
	SearchManager::TypeModes initialType;

	CStatusBarCtrl ctrlStatus;
	CEdit ctrlSearch;
	CComboBox ctrlSearchBox;
	CEdit ctrlSize;
	CComboBox ctrlMode;
	CComboBox ctrlSizeMode;
	CComboBox ctrlFiletype;
	CImageList searchTypes;
	CButton ctrlDoSearch;
	CButton ctrlPauseSearch;
	CButton ctrlPurge;	

	BOOL ListMeasure(MEASUREITEMSTRUCT *mis);
	BOOL ListDraw(DRAWITEMSTRUCT *dis);
	
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
	StringList targets;
	StringList wholeTargets;
	SearchInfo::List pausedResults;

	CEdit ctrlFilter;
	CComboBox ctrlFilterSel;
	CComboBox ctrlSkiplist;


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
	TStringList lastSearches;
	size_t droppedResults;

	bool closed;

	StringMap ucLineParams;
	
	std::string token;
		
	static int columnIndexes[];
	static int columnSizes[];

	typedef map<HWND, SearchFrame*> FrameMap;
	typedef FrameMap::const_iterator FrameIter;
	typedef pair<HWND, SearchFrame*> FramePair;

	static FrameMap frames;

	void downloadSelected(const tstring& aDir, bool view = false); 
	void downloadWholeSelected(const tstring& aDir);
	void onEnter();
	void onTab(bool shift);

	void download(const SearchResultPtr& aSR, const tstring& aDir, bool view);
	
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

/**
 * @file
 * $Id: SearchFrm.h 500 2010-06-25 22:08:18Z bigmuscle $
 */

