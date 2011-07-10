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

#include "../client/DirectoryListing.h"
#include "../client/StringSearch.h"
#include "../client/ADLSearch.h"
#include "../client/LogManager.h"
#include "../client/ShareManager.h"

class ThreadedDirectoryListing;

#define STATUS_MESSAGE_MAP 9
#define CONTROL_MESSAGE_MAP 10
class DirectoryListingFrame : public MDITabChildWindowImpl<DirectoryListingFrame>, public CSplitterImpl<DirectoryListingFrame>, 
	public UCHandler<DirectoryListingFrame>, private SettingsManagerListener, public UserInfoBaseHandler<DirectoryListingFrame>

{
public:
	static void openWindow(const tstring& aFile, const tstring& aDir, const HintedUser& aUser, int64_t aSpeed, bool myList = false);
	static void openWindow(const HintedUser& aUser, const string& txt, int64_t aSpeed, bool myList = false);
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
		STATUS_FILE_LIST_DIFF,
		STATUS_MATCH_QUEUE,
		STATUS_FIND,
		STATUS_NEXT,
		STATUS_DUMMY,
		STATUS_LAST
	};
	
	DirectoryListingFrame(const HintedUser& aUser, int64_t aSpeed, bool myList = false);
	 ~DirectoryListingFrame() { 
		dcassert(lists.find(dl->getUser()) != lists.end());
		lists.erase(dl->getUser());
	}


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
		COMMAND_ID_HANDLER(IDC_DOWNLOAD, onDownload)
		COMMAND_ID_HANDLER(IDC_DOWNLOADDIR, onDownloadDir)
		COMMAND_ID_HANDLER(IDC_DOWNLOADDIRTO, onDownloadDirTo)
		COMMAND_ID_HANDLER(IDC_DOWNLOADTO, onDownloadTo)
		COMMAND_ID_HANDLER(IDC_GO_TO_DIRECTORY, onGoToDirectory)
		COMMAND_ID_HANDLER(IDC_VIEW_AS_TEXT, onViewAsText)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchByTTH)
		COMMAND_ID_HANDLER(IDC_COPY_LINK, onCopy)
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
		COMMAND_ID_HANDLER(IDC_GOOGLE_TITLE, onSearchSite)
		COMMAND_ID_HANDLER(IDC_GOOGLE_FULL, onSearchSite)
		COMMAND_ID_HANDLER(IDC_TVCOM, onSearchSite)
		COMMAND_ID_HANDLER(IDC_METACRITIC, onSearchSite)
		COMMAND_ID_HANDLER(IDC_IMDB, onSearchSite)
		COMMAND_ID_HANDLER(IDC_FINDMISSING, onFindMissing)
		COMMAND_ID_HANDLER(IDC_CHECKSFV, onCheckSFV)

		COMMAND_ID_HANDLER(IDC_SEARCHLEFT, onSearchLeft)
		COMMAND_ID_HANDLER(IDC_SEARCHDIR, onSearchDir)
		COMMAND_ID_HANDLER(IDC_GOOGLE_TITLE +90, onSearchSiteDir)
		COMMAND_ID_HANDLER(IDC_GOOGLE_FULL+90, onSearchSiteDir)
		COMMAND_ID_HANDLER(IDC_TVCOM+90, onSearchSiteDir)
		COMMAND_ID_HANDLER(IDC_METACRITIC+90, onSearchSiteDir)
		COMMAND_ID_HANDLER(IDC_IMDB+90, onSearchSiteDir)
		
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_TARGET, IDC_DOWNLOAD_TARGET + targets.size() + WinUtil::lastDirs.size(), onDownloadTarget)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_TARGET_DIR, IDC_DOWNLOAD_TARGET_DIR + WinUtil::lastDirs.size(), onDownloadTargetDir)
		COMMAND_RANGE_HANDLER(IDC_PRIORITY_PAUSED, IDC_PRIORITY_HIGHEST, onDownloadWithPrio)
		COMMAND_RANGE_HANDLER(IDC_PRIORITY_PAUSED+90, IDC_PRIORITY_HIGHEST+90, onDownloadDirWithPrio)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_FAVORITE_DIRS, IDC_DOWNLOAD_FAVORITE_DIRS + FavoriteManager::getInstance()->getFavoriteDirs().size(), onDownloadFavoriteDirs)
		COMMAND_RANGE_HANDLER(IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS, IDC_DOWNLOAD_WHOLE_FAVORITE_DIRS + FavoriteManager::getInstance()->getFavoriteDirs().size(), onDownloadWholeFavoriteDirs)
		CHAIN_COMMANDS(ucBase)
		CHAIN_COMMANDS(uibBase)
		CHAIN_MSG_MAP(baseClass)
		CHAIN_MSG_MAP(CSplitterImpl<DirectoryListingFrame>)
	ALT_MSG_MAP(STATUS_MESSAGE_MAP)
		COMMAND_ID_HANDLER(IDC_FIND, onFind)
		COMMAND_ID_HANDLER(IDC_NEXT, onNext)
		COMMAND_ID_HANDLER(IDC_MATCH_QUEUE, onMatchQueue)
		COMMAND_ID_HANDLER(IDC_FILELIST_DIFF, onListDiff)
	ALT_MSG_MAP(CONTROL_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_XBUTTONUP, onXButtonUp)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onDownload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadWithPrio(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadDirWithPrio(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadDirTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadTo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onViewAsText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchByTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyDir(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGoToDirectory(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadTarget(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadTargetDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDoubleClickFiles(int idCtrl, LPNMHDR pnmh, BOOL& bHandled); 
	LRESULT onSelChangedDirectories(int idCtrl, LPNMHDR pnmh, BOOL& bHandled); 
	LRESULT onCustomDrawList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onCustomDrawTree(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onXButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onDownloadFavoriteDirs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDownloadWholeFavoriteDirs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onOpenDupe(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onSearchLeft(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onFindMissing(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCheckSFV(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	
	LRESULT onSearch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onSearchSite(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onSearchSiteDir(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void downloadList(const tstring& aTarget, bool view = false,  QueueItem::Priority prio = QueueItem::DEFAULT);
	void updateTree(DirectoryListing::Directory* tree, HTREEITEM treeItem);
	void UpdateLayout(BOOL bResizeBars = TRUE);
	void findFile(bool findNext);
	void runUserCommand(UserCommand& uc);
	void loadFile(const tstring& name, const tstring& dir);
	void loadXML(const string& txt);
	void refreshTree(const tstring& root);

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
	
	UserListHandler getUserList() { return UserListHandler(*dl.get()); }

private:
	friend class ThreadedDirectoryListing;
	
	void changeDir(DirectoryListing::Directory* d, BOOL enableRedraw);
	HTREEITEM findFile(const StringSearch& str, HTREEITEM root, int &foundFile, int &skipHits);
	void updateStatus();
	void initStatus();
	void addHistory(const string& name);
	void up();
	void back();
	void forward();

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
				case COLUMN_DATE: return type == DIRECTORY ? Text::toT(dir->getDirDate()) : Util::emptyStringT;
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

	StringList targets;
	
	deque<string> history;
	size_t historyIndex;
	
	CTreeViewCtrl ctrlTree;
	TypedListViewCtrl<ItemInfo, IDC_FILES> ctrlList;
	CStatusBarCtrl ctrlStatus;
	HTREEITEM treeRoot;
	
	CButton ctrlFind, ctrlFindNext;
	CButton ctrlListDiff;
	CButton ctrlMatchQueue;

	string findStr;
	tstring error;
	string size;

	int skipHits;

	size_t files;
	int64_t speed;		/**< Speed at which this file list was downloaded */

	bool updating;
	bool searching;
	bool closed;
	bool loading;
	bool mylist;

	int statusSizes[10];

	
	boost::shared_ptr<DirectoryListing> dl;

	StringMap ucLineParams;

	typedef unordered_map<UserPtr, DirectoryListingFrame*, User::Hash> UserMap;
	typedef UserMap::const_iterator UserIter;
	
	static UserMap lists;

	static int columnIndexes[COLUMN_LAST];
	static int columnSizes[COLUMN_LAST];

	typedef map< HWND , DirectoryListingFrame* > FrameMap;
	typedef pair< HWND , DirectoryListingFrame* > FramePair;
	typedef FrameMap::iterator FrameIter;

	static FrameMap frames;
	
	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw();
};

class ThreadedDirectoryListing : public Thread
{
public:
	ThreadedDirectoryListing(DirectoryListingFrame* pWindow, 
		const string& pFile, const string& pTxt, const tstring& aDir = Util::emptyStringT, bool myList = false) : mWindow(pWindow),
		mFile(pFile), mTxt(pTxt), mDir(aDir), mylist(myList)
	{ }

protected:
	DirectoryListingFrame* mWindow;
	string mFile;
	string mTxt;
	tstring mDir;
	bool mylist;
private:
	int run() {
		try {
			if(!mFile.empty()) {
				
				if(mylist){
				// if its own list regenerate it before opening, but only if its dirty
				mFile = ShareManager::getInstance()->generateOwnList();
				}
				mWindow->dl->loadFile(mFile);
				

				if((BOOLSETTING(USE_ADLS) && !mylist) || (BOOLSETTING(USE_ADLS_OWN_LIST) && mylist)) {
				ADLSearchManager::getInstance()->matchListing(*mWindow->dl);
				} 
				if(!mylist)
				mWindow->dl->checkDupes();

				mWindow->refreshTree(mDir);
			} else {
				mWindow->refreshTree(Text::toT(Util::toNmdcFile(mWindow->dl->updateXML(mTxt))));
			}

			mWindow->PostMessage(WM_SPEAKER, DirectoryListingFrame::FINISHED);
		} catch(const AbortException) {
			mWindow->PostMessage(WM_SPEAKER, DirectoryListingFrame::ABORTED);
		} catch(const Exception& e) {
			mWindow->error = Text::toT(ClientManager::getInstance()->getNicks(mWindow->dl->getUser()->getCID(), mWindow->dl->getHintedUser().hint)[0] + ": " + e.getError());
			mWindow->PostMessage(WM_SPEAKER, DirectoryListingFrame::ABORTED);
		}

		//cleanup the thread object
		delete this;

		return 0;
	}
};

#endif // !defined(DIRECTORY_LISTING_FRM_H)

/**
 * @file
 * $Id: DirectoryListingFrm.h 500 2010-06-25 22:08:18Z bigmuscle $
 */
