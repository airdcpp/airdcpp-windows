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

#if !defined(DIRECTORY_LISTING_FRM_H)
#define DIRECTORY_LISTING_FRM_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "../client/User.h"
#include "../client/FastAlloc.h"

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"
#include "WinUtil.h"
#include "UCHandler.h"
#include "MenuBaseHandlers.h"

#include "../client/DirectoryListing.h"
#include "../client/DirectoryListingListener.h"
#include "../client/StringSearch.h"
#include "../client/ADLSearch.h"
#include "../client/LogManager.h"
#include "../client/ShareManager.h"
#include "../client/TargetUtil.h"

#define STATUS_MESSAGE_MAP 9
#define CONTROL_MESSAGE_MAP 10
class DirectoryListingFrame : public MDITabChildWindowImpl<DirectoryListingFrame>, public CSplitterImpl<DirectoryListingFrame>, 
	public UCHandler<DirectoryListingFrame>, private SettingsManagerListener, public UserInfoBaseHandler<DirectoryListingFrame>,
	public DownloadBaseHandler<DirectoryListingFrame>, private DirectoryListingListener
{
public:
	static void openWindow(DirectoryListing* aList, const string& aDir);
	static void closeAll();

	typedef MDITabChildWindowImpl<DirectoryListingFrame> baseClass;
	typedef UCHandler<DirectoryListingFrame> ucBase;
	typedef UserInfoBaseHandler<DirectoryListingFrame> uibBase;
	typedef DownloadBaseHandler<DirectoryListingFrame> dlBase;

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
		UPDATE_STATUS,
		STARTED,
		FINISHED,
		ABORTED
	};	
		
	enum {
		STATUS_TEXT,
		STATUS_SPEED,
		STATUS_TOTAL_FILES,
		STATUS_TOTAL_SIZE,
		STATUS_SELECTED_FILES,
		STATUS_SELECTED_SIZE,
		STATUS_GET_FULL_LIST,
		STATUS_MATCH_ADL,
		STATUS_FILE_LIST_DIFF,
		STATUS_MATCH_QUEUE,
		STATUS_FIND,
		STATUS_NEXT,
		STATUS_DUMMY,
		STATUS_LAST
	};
	
	DirectoryListingFrame(DirectoryListing* aList);
	 ~DirectoryListingFrame();


	DECLARE_FRAME_WND_CLASS(_T("DirectoryListingFrame"), IDR_DIRECTORY)

	BEGIN_MSG_MAP(DirectoryListingFrame)
		NOTIFY_HANDLER(IDC_FILES, LVN_GETDISPINFO, ctrlList.onGetDispInfo)
		NOTIFY_HANDLER(IDC_FILES, LVN_COLUMNCLICK, ctrlList.onColumnClick)
		NOTIFY_HANDLER(IDC_FILES, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_FILES, NM_DBLCLK, onDoubleClickFiles)
		NOTIFY_HANDLER(IDC_FILES, LVN_ITEMCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_KEYDOWN, onKeyDownDirs)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_SELCHANGED, onSelChangedDirectories)
		NOTIFY_HANDLER(IDC_FILES, NM_CUSTOMDRAW, onCustomDrawList)
		NOTIFY_HANDLER(IDC_DIRECTORIES, NM_CUSTOMDRAW, onCustomDrawTree)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(FTM_CONTEXTMENU, onTabContextMenu)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		COMMAND_ID_HANDLER(IDC_GO_TO_DIRECTORY, onGoToDirectory)
		COMMAND_ID_HANDLER(IDC_VIEW_AS_TEXT, onViewAsText)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchByTTH)
		COMMAND_ID_HANDLER(IDC_COPY_LINK, onCopy)
		COMMAND_ID_HANDLER(IDC_COPY_DATE, onCopy)
		COMMAND_ID_HANDLER(IDC_COPY_TTH, onCopy)
		COMMAND_ID_HANDLER(IDC_COPY_NICK, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_FILENAME, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_DIR, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_SIZE, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_EXACT_SIZE, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_PATH, onCopy);
		COMMAND_ID_HANDLER(IDC_COPY_DIRECTORY, onCopyDir);
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		COMMAND_ID_HANDLER(IDC_OPEN, onOpenDupe)
		COMMAND_ID_HANDLER(IDC_OPEN_FOLDER, onOpenDupe)
		COMMAND_ID_HANDLER(IDC_SEARCH, onSearch)
		COMMAND_RANGE_HANDLER(IDC_SEARCH_SITES, IDC_SEARCH_SITES + WebShortcuts::getInstance()->list.size(), onSearchSite)
		COMMAND_ID_HANDLER(IDC_FINDMISSING, onFindMissing)
		COMMAND_ID_HANDLER(IDC_CHECKSFV, onCheckSFV)

		COMMAND_ID_HANDLER(IDC_SEARCHLEFT, onSearchLeft)
		COMMAND_ID_HANDLER(IDC_SEARCHDIR, onSearchDir)
		COMMAND_RANGE_HANDLER(IDC_SEARCH_SITES+90, IDC_SEARCH_SITES+90 + WebShortcuts::getInstance()->list.size(), onSearchSiteDir)

		CHAIN_COMMANDS(ucBase)
		CHAIN_COMMANDS(uibBase)
		CHAIN_COMMANDS(dlBase)
		CHAIN_MSG_MAP(baseClass)
		CHAIN_MSG_MAP(CSplitterImpl<DirectoryListingFrame>)
	ALT_MSG_MAP(STATUS_MESSAGE_MAP)
		COMMAND_ID_HANDLER(IDC_FIND, onFind)
		COMMAND_ID_HANDLER(IDC_NEXT, onNext)
		COMMAND_ID_HANDLER(IDC_MATCH_QUEUE, onMatchQueue)
		COMMAND_ID_HANDLER(IDC_FILELIST_DIFF, onListDiff)
		COMMAND_ID_HANDLER(IDC_MATCH_ADL, onMatchADL)
		COMMAND_ID_HANDLER(IDC_GETLIST, onGetFullList)
	ALT_MSG_MAP(CONTROL_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_XBUTTONUP, onXButtonUp)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onViewAsText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchByTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyDir(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGoToDirectory(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDoubleClickFiles(int idCtrl, LPNMHDR pnmh, BOOL& bHandled); 
	LRESULT onSelChangedDirectories(int idCtrl, LPNMHDR pnmh, BOOL& bHandled); 
	LRESULT onCustomDrawList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onCustomDrawTree(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onXButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onOpenDupe(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onSearchLeft(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onFindMissing(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCheckSFV(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMatchADL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGetFullList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onSearch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onSearchSite(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onSearchSiteDir(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void updateTree(DirectoryListing::Directory* tree, HTREEITEM treeItem);
	void UpdateLayout(BOOL bResizeBars = TRUE);
	void findFile(bool findNext);
	void runUserCommand(UserCommand& uc);

	void refreshTree(const tstring& root, bool convertFromPartial, bool searching);

	HTREEITEM findItem(HTREEITEM ht, const tstring& name);
	void selectItem(const tstring& name);
	
	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
		updateStatus();
		return 0;
	}

	LRESULT onSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlList.SetFocus();
		return 0;
	}

	void setWindowTitle() {
		if(error.empty())
			SetWindowText((WinUtil::getNicks(dl->getHintedUser()) + _T(" - ") + WinUtil::getHubNames(dl->getHintedUser()).first).c_str());
		else
			SetWindowText(error.c_str());		
	}

	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return 1;
	}

	void clearList() {
		 int j = ctrlList.GetItemCount(); 	                
	         
		 for(int i = 0; i < j; i++) { 	 
	                delete ctrlList.getItemData(i); 	 
	             } 	 
	                 
		ctrlList.DeleteAllItems();
	}

	LRESULT onFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		searching = true;
		findFile(false);
		searching = false;
		updateStatus();
		return 0;
	}
	LRESULT onNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		searching = true;
		findFile(true);
		searching = false;
		updateStatus();
		return 0;
	}

	LRESULT onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onListDiff(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	LRESULT onKeyDownDirs(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMTVKEYDOWN* kd = (NMTVKEYDOWN*) pnmh;
		if(kd->wVKey == VK_TAB) {
			onTab();
		}
		return 0;
	}

	void onTab() {
		HWND focus = ::GetFocus();
		if(focus == ctrlTree.m_hWnd) {
			ctrlList.SetFocus();
		} else if(focus == ctrlList.m_hWnd) {
			ctrlTree.SetFocus();
		}
	}
	
	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}
	
	struct UserListHandler {
		UserListHandler(DirectoryListing& _dl) : dl(_dl) { }
		
		void forEachSelected(void (UserInfoBase::*func)()) {
			(dl.*func)();
		}
		
		template<class _Function>
		_Function forEachSelectedT(_Function pred) {
			pred(&dl);
			return pred;
		}
		
	private:
		DirectoryListing& dl;	
	};
	
	UserListHandler getUserList() { return UserListHandler(*dl); }

	/* DownloadBaseHandler functions */
	void appendDownloadItems(OMenu& aMenu, bool isWhole);
	int64_t getDownloadSize(bool isWhole);
	void download(const string& aTarget, QueueItem::Priority p, bool usingTree, TargetUtil::TargetType aTargetType = TargetUtil::TARGET_PATH);
	bool showDirDialog(string& fileName);
private:
	void changeWindowState(bool enable);
	
	void changeDir(const DirectoryListing::Directory* d, BOOL enableRedraw);
	void findSearchHit();
	int searchPos;

	void updateStatus();
	void initStatus();
	void addHistory(const string& name);
	void up();
	void back();
	void forward();

	int64_t loadTime;

	class ItemInfo : public FastAlloc<ItemInfo> {
	public:
		enum ItemType {
			FILE,
			DIRECTORY
		} type;
		
		union {
			DirectoryListing::File* file;
			DirectoryListing::Directory* dir;
		};

		ItemInfo(DirectoryListing::File* f) : type(FILE), file(f) { }
		ItemInfo(DirectoryListing::Directory* d) : type(DIRECTORY), dir(d) { }

		const tstring getText(uint8_t col) const {
			switch(col) {
				case COLUMN_FILENAME: return type == DIRECTORY ? Text::toT(dir->getName()) : Text::toT(file->getName());
				case COLUMN_TYPE: 
					if(type == FILE) {
						tstring type = Util::getFileExt(Text::toT(file->getName()));
						if(type.size() > 0 && type[0] == '.')
							type.erase(0, 1);
						return type;
					} else {
						return Util::emptyStringT;
					}
				case COLUMN_EXACTSIZE: return type == DIRECTORY ? Util::formatExactSize(dir->getTotalSize()) : Util::formatExactSize(file->getSize());
				case COLUMN_SIZE: return  type == DIRECTORY ? Util::formatBytesW(dir->getTotalSize()) : Util::formatBytesW(file->getSize());
				case COLUMN_TTH: return type == FILE ? Text::toT(file->getTTH().toBase32()) : Util::emptyStringT;
				case COLUMN_DATE: return (type == DIRECTORY && dir->getDate() > 0) ? Text::toT(Util::getDateTime(dir->getDate())) : Util::emptyStringT;
				default: return Util::emptyStringT;
			}
		}
		
		struct TotalSize {
			TotalSize() : total(0) { }
			void operator()(ItemInfo* a) { total += a->type == DIRECTORY ? a->dir->getTotalSize() : a->file->getSize(); }
			int64_t total;
		};

		static int compareItems(const ItemInfo* a, const ItemInfo* b, uint8_t col) {
			if(a->type == DIRECTORY) {
				if(b->type == DIRECTORY) {
					switch(col) {
					case COLUMN_EXACTSIZE: return compare(a->dir->getTotalSize(), b->dir->getTotalSize());
					case COLUMN_SIZE: return compare(a->dir->getTotalSize(), b->dir->getTotalSize());
					default: return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str(), true);
					}
				} else {
					return -1;
				}
			} else if(b->type == DIRECTORY) {
				return 1;
			} else {
				switch(col) {
				case COLUMN_EXACTSIZE: return compare(a->file->getSize(), b->file->getSize());
				case COLUMN_SIZE: return compare(a->file->getSize(), b->file->getSize());
				default: return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str(), false);
				}
			}
		}
		int getImageIndex() const {
			if(type == DIRECTORY)
				return WinUtil::getDirIconIndex();
			else
				return WinUtil::getIconIndex(getText(COLUMN_FILENAME));
		}
	};

	static int CALLBACK DefaultSort(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/) {
		DirectoryListing::Directory* pItem1 = (DirectoryListing::Directory*)lParam1;
		DirectoryListing::Directory* pItem2 = (DirectoryListing::Directory*)lParam2;

		return Util::DefaultSort(Text::toT(pItem1->getName()).c_str(), Text::toT(pItem2->getName()).c_str());
	}

	CContainedWindow statusContainer;
	CContainedWindow treeContainer;
	CContainedWindow listContainer;
	
	deque<string> history;
	size_t historyIndex;
	
	CTreeViewCtrl ctrlTree;
	TypedListViewCtrl<ItemInfo, IDC_FILES> ctrlList;
	CStatusBarCtrl ctrlStatus;
	HTREEITEM treeRoot;
	
	CButton ctrlFind, ctrlFindNext;
	CButton ctrlListDiff;
	CButton ctrlMatchQueue;
	CButton ctrlADLMatch;
	CButton ctrlGetFullList;

	tstring error;
	string size;

	int skipHits;

	size_t files;

	bool updating;
	bool searching;
	bool closed;

	int statusSizes[12];
	
	DirectoryListing* dl;

	ParamMap ucLineParams;

	typedef unordered_map<UserPtr, DirectoryListingFrame*, User::Hash> UserMap;
	typedef UserMap::const_iterator UserIter;

	static int columnIndexes[COLUMN_LAST];
	static int columnSizes[COLUMN_LAST];

	typedef map< HWND , DirectoryListingFrame* > FrameMap;
	typedef pair< HWND , DirectoryListingFrame* > FramePair;
	typedef FrameMap::iterator FrameIter;

	static FrameMap frames;
	void DisableWindow(){
		ctrlTree.EnableWindow(FALSE);
		ctrlList.EnableWindow(FALSE);
	}
	void EnableWindow(){
		ctrlTree.EnableWindow(TRUE);
		ctrlList.EnableWindow(TRUE);
	}


	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;

	void on(DirectoryListingListener::LoadingFinished, int64_t aStart, const string& aDir, bool convertFromPartial) noexcept;
	void on(DirectoryListingListener::LoadingFailed, const string& aReason) noexcept;
	void on(DirectoryListingListener::LoadingStarted) noexcept;
	void on(DirectoryListingListener::QueueMatched, const string& aMessage) noexcept;
	void on(DirectoryListingListener::Close) noexcept;
	void on(DirectoryListingListener::SearchStarted) noexcept;
	void on(DirectoryListingListener::SearchFailed, bool timedOut) noexcept;
	void on(DirectoryListingListener::ChangeDirectory, const string& aDir, bool isSearchChange) noexcept;
};

#endif // !defined(DIRECTORY_LISTING_FRM_H)

/**
 * @file
 * $Id: DirectoryListingFrm.h 500 2010-06-25 22:08:18Z bigmuscle $
 */
