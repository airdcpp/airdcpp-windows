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

#include "stdafx.h"
#include "../client/DCPlusPlus.h"



#include "Resource.h"

#include "DirectoryListingFrm.h"
#include "WinUtil.h"
#include "LineDlg.h"
#include "PrivateFrame.h"
#include "ShellContextMenu.h"
#include "../client/File.h"
#include "../client/QueueManager.h"
#include "../client/StringTokenizer.h"
#include "../client/ADLSearch.h"
#include "../client/MerkleTree.h"
#include "../client/User.h"
#include "../client/ClientManager.h"
#include "../client/ShareScannerManager.h"
#include "../client/Wildcards.h"
#include "../client/DirectoryListingManager.h"
#include "TextFrame.h"


DirectoryListingFrame::FrameMap DirectoryListingFrame::frames;
int DirectoryListingFrame::columnIndexes[] = { COLUMN_FILENAME, COLUMN_TYPE, COLUMN_EXACTSIZE, COLUMN_SIZE, COLUMN_TTH, COLUMN_DATE };
int DirectoryListingFrame::columnSizes[] = { 300, 60, 100, 100, 200, 100 };

static ResourceManager::Strings columnNames[] = { ResourceManager::FILE, ResourceManager::TYPE, ResourceManager::EXACT_SIZE, ResourceManager::SIZE, ResourceManager::TTH_ROOT, ResourceManager::DATE };

void DirectoryListingFrame::openWindow(DirectoryListing* aList, const string& aDir) {

	HWND aHWND = NULL;
	DirectoryListingFrame* frame = new DirectoryListingFrame(aList);
	if(BOOLSETTING(POPUNDER_FILELIST)) {
		aHWND = WinUtil::hiddenCreateEx(frame);
	} else {
		aHWND = frame->CreateEx(WinUtil::mdiClient);
	}
	if(aHWND != 0) {
		frame->ctrlStatus.SetText(0, CTSTRING(LOADING_FILE_LIST));
		if (aList->getPartialList())
			aList->addPartialListTask(aDir);
		else
			aList->addFullListTask(aDir);
		frames.insert( FramePair( frame->m_hWnd, frame ) );
	} else {
		delete frame;
	}
}

DirectoryListingFrame::DirectoryListingFrame(DirectoryListing* aList) :
	statusContainer(STATUSCLASSNAME, this, STATUS_MESSAGE_MAP), treeContainer(WC_TREEVIEW, this, CONTROL_MESSAGE_MAP),
		listContainer(WC_LISTVIEW, this, CONTROL_MESSAGE_MAP), historyIndex(0),
		treeRoot(NULL), skipHits(0), files(0), updating(false), searching(false), dl(aList)
{ 
	dl->addListener(this);
}

DirectoryListingFrame::~DirectoryListingFrame() {
	dl->join();
	delete dl;
}

void DirectoryListingFrame::on(DirectoryListingListener::LoadingFinished, int64_t aStart, const string& aDir, bool convertFromPartial) noexcept {
	refreshTree(Text::toT(aDir), convertFromPartial);

	int64_t end = GET_TICK();
	loadTime = (end - aStart) / 1000;
	PostMessage(WM_SPEAKER, DirectoryListingFrame::FINISHED);
}

void DirectoryListingFrame::on(DirectoryListingListener::LoadingFailed, const string& aReason) noexcept {

	PostMessage(WM_SPEAKER, DirectoryListingFrame::ABORTED);
}

void DirectoryListingFrame::on(DirectoryListingListener::LoadingStarted) noexcept {
	PostMessage(WM_SPEAKER, DirectoryListingFrame::STARTED);
	changeWindowState(false);
}

void DirectoryListingFrame::on(DirectoryListingListener::QueueMatched, const string& aMessage) noexcept {
	PostMessage(WM_SPEAKER, DirectoryListingFrame::UPDATE_STATUS, (LPARAM)new tstring(Text::toT(aMessage)));
	changeWindowState(true);
}

void DirectoryListingFrame::on(DirectoryListingListener::Close) noexcept {
	PostMessage(WM_SPEAKER, DirectoryListingFrame::ABORTED);
	//PostMessage(WM_CLOSE);
}

void DirectoryListingFrame::on(DirectoryListingListener::SearchStarted) noexcept {
	PostMessage(WM_SPEAKER, DirectoryListingFrame::UPDATE_STATUS, (LPARAM)new tstring(TSTRING(SEARCHING)));
	changeWindowState(false);
}

void DirectoryListingFrame::on(DirectoryListingListener::SearchFailed, bool timedOut) noexcept {
	PostMessage(WM_SPEAKER, DirectoryListingFrame::UPDATE_STATUS, (LPARAM)new tstring(timedOut ? TSTRING(SEARCH_TIMED_OUT) : TSTRING(NO_RESULTS_FOUND)));
	changeWindowState(true);
}

void DirectoryListingFrame::on(DirectoryListingListener::ChangeDirectory, const string& aDir, bool isSearchChange) noexcept {
	selectItem(Text::toT(aDir));
	if (isSearchChange) {
		PostMessage(WM_SPEAKER, DirectoryListingFrame::UPDATE_STATUS, (LPARAM)new tstring(_T("Search finished")));
		findFile();
	}

	changeWindowState(true);
}

LRESULT DirectoryListingFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {

	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	ctrlStatus.SetFont(WinUtil::boldFont);
	statusContainer.SubclassWindow(ctrlStatus.m_hWnd);

	ctrlTree.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP | TVS_TRACKSELECT, WS_EX_CLIENTEDGE, IDC_DIRECTORIES);
	
	if(BOOLSETTING(USE_EXPLORER_THEME) &&
		((WinUtil::getOsMajor() >= 5 && WinUtil::getOsMinor() >= 1) //WinXP & WinSvr2003
		|| (WinUtil::getOsMajor() >= 6))) //Vista & Win7
	{
		SetWindowTheme(ctrlTree.m_hWnd, L"explorer", NULL);
	}
	
	treeContainer.SubclassWindow(ctrlTree);
	ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_FILES);
	listContainer.SubclassWindow(ctrlList);
	ctrlList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);	

	ctrlList.SetBkColor(WinUtil::bgColor);
	ctrlList.SetTextBkColor(WinUtil::bgColor);
	ctrlList.SetTextColor(WinUtil::textColor);
	
	ctrlTree.SetBkColor(WinUtil::bgColor);
	ctrlTree.SetTextColor(WinUtil::textColor);
	

	
	WinUtil::splitTokens(columnIndexes, SETTING(DIRECTORYLISTINGFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(DIRECTORYLISTINGFRAME_WIDTHS), COLUMN_LAST);
	for(uint8_t j = 0; j < COLUMN_LAST; j++) 
	{
		int fmt = ((j == COLUMN_SIZE) || (j == COLUMN_EXACTSIZE) || (j == COLUMN_TYPE)) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlList.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	ctrlList.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlList.setVisible(SETTING(DIRECTORYLISTINGFRAME_VISIBLE));

	ctrlTree.SetImageList(WinUtil::fileImages, TVSIL_NORMAL);
	ctrlList.SetImageList(WinUtil::fileImages, LVSIL_SMALL);
	ctrlList.setSortColumn(COLUMN_FILENAME);

	ctrlFind.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON, 0, IDC_FIND);
	ctrlFind.SetWindowText(CTSTRING(FIND));
	ctrlFind.SetFont(WinUtil::systemFont);

	ctrlFindNext.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON, 0, IDC_NEXT);
	ctrlFindNext.SetWindowText(CTSTRING(NEXT));
	ctrlFindNext.SetFont(WinUtil::systemFont);

	ctrlMatchQueue.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON, 0, IDC_MATCH_QUEUE);
	ctrlMatchQueue.SetWindowText(CTSTRING(MATCH_QUEUE));
	ctrlMatchQueue.SetFont(WinUtil::systemFont);

	ctrlListDiff.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		BS_PUSHBUTTON, 0, IDC_FILELIST_DIFF);
	ctrlListDiff.SetWindowText(CTSTRING(FILE_LIST_DIFF));
	ctrlListDiff.SetFont(WinUtil::systemFont);

	ctrlADLMatch.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
	BS_PUSHBUTTON, 0, IDC_MATCH_ADL);
	ctrlADLMatch.SetWindowText(CTSTRING(MATCH_ADL));
	ctrlADLMatch.SetFont(WinUtil::systemFont);

	ctrlGetFullList.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
	BS_PUSHBUTTON, 0, IDC_GETLIST);
	ctrlGetFullList.SetWindowText(CTSTRING(GET_FULL_LIST));
	ctrlGetFullList.SetFont(WinUtil::systemFont);
	
	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	SetSplitterPanes(ctrlTree.m_hWnd, ctrlList.m_hWnd);
	m_nProportionalPos = 2500;
	string nick = ClientManager::getInstance()->getNicks(dl->getHintedUser())[0];
	treeRoot = ctrlTree.InsertItem(TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM, Text::toT(nick).c_str(), WinUtil::getDirIconIndex(), WinUtil::getDirIconIndex(), 0, 0, (LPARAM)dl->getRoot(), NULL, TVI_SORT);
	dcassert(treeRoot != NULL);
	
	memzero(statusSizes, sizeof(statusSizes));
	statusSizes[STATUS_FILE_LIST_DIFF] = WinUtil::getTextWidth(TSTRING(FILE_LIST_DIFF), m_hWnd) + 8;
	statusSizes[STATUS_MATCH_QUEUE] = WinUtil::getTextWidth(TSTRING(MATCH_QUEUE), m_hWnd) + 8;
	statusSizes[STATUS_FIND] = WinUtil::getTextWidth(TSTRING(FIND), m_hWnd) + 8;
	statusSizes[STATUS_NEXT] = WinUtil::getTextWidth(TSTRING(NEXT), m_hWnd) + 8;
	statusSizes[STATUS_MATCH_ADL] = WinUtil::getTextWidth(TSTRING(MATCH_ADL), m_hWnd) + 8;
	statusSizes[STATUS_GET_FULL_LIST] = WinUtil::getTextWidth(TSTRING(GET_FULL_LIST), m_hWnd) + 8;

	ctrlStatus.SetParts(STATUS_LAST, statusSizes);

	ctrlTree.EnableWindow(FALSE);
	ctrlGetFullList.EnableWindow(dl->getPartialList());
	
	SettingsManager::getInstance()->addListener(this);
	closed = false;

	CRect rc(SETTING(DIRLIST_LEFT), SETTING(DIRLIST_TOP), SETTING(DIRLIST_RIGHT), SETTING(DIRLIST_BOTTOM));
	if(! (rc.top == 0 && rc.bottom == 0 && rc.left == 0 && rc.right == 0) )
		MoveWindow(rc, TRUE);
	
	setWindowTitle();
	WinUtil::SetIcon(m_hWnd, _T("Directory.ico"));
	bHandled = FALSE;
	return 1;
}

void DirectoryListingFrame::changeWindowState(bool enable) {
	ctrlMatchQueue.EnableWindow(enable);
	ctrlADLMatch.EnableWindow(enable);
	ctrlFind.EnableWindow(enable);
	ctrlFindNext.EnableWindow(enable);
	ctrlListDiff.EnableWindow(enable);

	if (enable) {
		EnableWindow();
		ctrlGetFullList.EnableWindow(dl->getPartialList());
	} else {
		DisableWindow();
		ctrlGetFullList.EnableWindow(false);
	}
}

LRESULT DirectoryListingFrame::onGetFullList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	HTREEITEM ht = ctrlTree.GetSelectedItem();
	DirectoryListing::Directory* d = (DirectoryListing::Directory*)ctrlTree.GetItemData(ht);


	if (dl->getIsOwnList())
		dl->addFullListTask(dl->getPath(d));
	else
		QueueManager::getInstance()->addList(dl->getHintedUser(), QueueItem::FLAG_CLIENT_VIEW, dl->getPath(d));
	return 1;
}

void DirectoryListingFrame::updateTree(DirectoryListing::Directory* aTree, HTREEITEM aParent) {
	for(auto i = aTree->directories.begin(); i != aTree->directories.end(); ++i) {
		tstring name = Text::toT((*i)->getName());
		int index = (*i)->getComplete() ? WinUtil::getDirIconIndex() : WinUtil::getDirMaskedIndex();
		HTREEITEM ht = ctrlTree.InsertItem(TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM, name.c_str(), index, index, 0, 0, (LPARAM)*i, aParent, TVI_LAST);
		if((*i)->getAdls())
			ctrlTree.SetItemState(ht, TVIS_BOLD, TVIS_BOLD);
		updateTree(*i, ht);
	}
	// sort
	TVSORTCB tvsortcb;
	tvsortcb.hParent = aParent;
	tvsortcb.lpfnCompare = DefaultSort;
	tvsortcb.lParam = 0;
	ctrlTree.SortChildrenCB(&tvsortcb);

}

void DirectoryListingFrame::refreshTree(const tstring& root, bool convertFromPartial) {
	ctrlTree.SetRedraw(FALSE);

	HTREEITEM ht = convertFromPartial ? treeRoot : findItem(treeRoot, root);
	if(ht == NULL) {
		ht = treeRoot;
	}

	DirectoryListing::Directory* d = (DirectoryListing::Directory*)ctrlTree.GetItemData(ht);
	ctrlTree.SelectItem(NULL);

	HTREEITEM next = NULL;
	while((next = ctrlTree.GetChildItem(ht)) != NULL) {
		ctrlTree.DeleteItem(next);
	}
	updateTree(d, ht);

	ctrlTree.Expand(treeRoot);

	int index = d->getComplete() ? WinUtil::getDirIconIndex() : WinUtil::getDirMaskedIndex();
	ctrlTree.SetItemImage(ht, index, index);

	ctrlTree.SelectItem(NULL);
	selectItem(root);
	if (!dl->getIsOwnList() && SETTING(DUPES_IN_FILELIST))
		dl->checkShareDupes();
	ctrlTree.SetRedraw(TRUE);

	if (dl->isCurrentSearchPath(Text::fromT(root))) {
		findFile();
	}
}

HTREEITEM DirectoryListingFrame::findFile() {
	auto search = dl->curSearch;
	dcassert(search);
	bool found = false;

	// Check file names in list pane
	while(searchPos <ctrlList.GetItemCount()) {
		const ItemInfo* ii = ctrlList.getItemData(searchPos);
		if (search->hasRoot && ii->type == ItemInfo::FILE) {
			if (search->root == ii->file->getTTH()) {
				found = true;
				break;
			}
		} else if(ii->type == ItemInfo::FILE) {
			if(search->matchesDirectFile(ii->file->getName(), ii->file->getSize())) {
				found = true;
				break;
			}
		} else if(search->matchesDirectDirectoryName(ii->dir->getName())) {
			if (search->matchesSize(ii->dir->getSize())) {
				found = true;
				break;
			}
		}
		searchPos++;
	}

	if (found) {
		// Remove prev. selection from file list
		if(ctrlList.GetSelectedCount() > 0)		{
			for(int i=0; i<ctrlList.GetItemCount(); i++)
				ctrlList.SetItemState(i, 0, LVIS_SELECTED);
		}

		// Highlight and focus the dir/file if possible
		ctrlList.SetFocus();
		ctrlList.EnsureVisible(searchPos, FALSE);
		ctrlList.SetItemState(searchPos, LVIS_SELECTED | LVIS_FOCUSED, (UINT)-1);
		searchPos++;
	} else {
		//move to next dir (if there are any)
		searchPos = 0;
		if (!dl->nextResult()) {
			MessageBox(CTSTRING(NO_MATCHES), CTSTRING(SEARCH_FOR_FILE));
		}
	}

	return 0;
}

void DirectoryListingFrame::findFile(bool findNext) {
	if(!findNext)	{
		searchPos = 0;

		// Prompt for substring to find
		LineDlg dlg;
		dlg.title = TSTRING(SEARCH_FOR_FILE);
		dlg.description = TSTRING(ENTER_SEARCH_STRING);
		dlg.line = Util::emptyStringT;

		if(dlg.DoModal() != IDOK)
			return;

		string findStr = Text::fromT(dlg.line);
		if(findStr.empty())
			return;

		StringList extList;
		dl->addSearchTask(findStr, 0, SearchManager::TYPE_ANY, SearchManager::SIZE_DONTCARE, extList);

		return;
	} else {
		findFile();
	}
}

void DirectoryListingFrame::updateStatus() {
	if(!searching && !updating && ctrlStatus.IsWindow()) {
		int cnt = ctrlList.GetSelectedCount();
		int64_t total = 0;
		if(cnt == 0) {
			cnt = ctrlList.GetItemCount ();
			total = ctrlList.forEachT(ItemInfo::TotalSize()).total;
		} else {
			total = ctrlList.forEachSelectedT(ItemInfo::TotalSize()).total;
		}

		tstring tmp = TSTRING(ITEMS) + _T(": ") + Util::toStringW(cnt);
		bool u = false;

		int w = WinUtil::getTextWidth(tmp, ctrlStatus.m_hWnd);
		if(statusSizes[STATUS_SELECTED_FILES] < w) {
			statusSizes[STATUS_SELECTED_FILES] = w;
			u = true;
		}
		ctrlStatus.SetText(STATUS_SELECTED_FILES, tmp.c_str());

		tmp = TSTRING(SIZE) + _T(": ") + Util::formatBytesW(total);
		w = WinUtil::getTextWidth(tmp, ctrlStatus.m_hWnd);
		if(statusSizes[STATUS_SELECTED_SIZE] < w) {
			statusSizes[STATUS_SELECTED_SIZE] = w;
			u = true;
		}
		ctrlStatus.SetText(STATUS_SELECTED_SIZE, tmp.c_str());

		if(u)
			UpdateLayout(TRUE);
	}
}

void DirectoryListingFrame::initStatus() {
	files = dl->getTotalFileCount();
	size = Util::formatBytes(dl->getTotalSize());

	tstring tmp = TSTRING(FILES) + _T(": ") + Util::toStringW(dl->getTotalFileCount(true));
	statusSizes[STATUS_TOTAL_FILES] = WinUtil::getTextWidth(tmp, m_hWnd);
	ctrlStatus.SetText(STATUS_TOTAL_FILES, tmp.c_str());

	tmp = TSTRING(SIZE) + _T(": ") + Util::formatBytesW(dl->getTotalSize(true));
	statusSizes[STATUS_TOTAL_SIZE] = WinUtil::getTextWidth(tmp, m_hWnd);
	ctrlStatus.SetText(STATUS_TOTAL_SIZE, tmp.c_str());

	tmp = TSTRING(SPEED) + _T(": ") + Util::formatBytesW(dl->getspeed()) + _T("/s");
	statusSizes[STATUS_SPEED] = WinUtil::getTextWidth(tmp, m_hWnd);
	ctrlStatus.SetText(STATUS_SPEED, tmp.c_str());

	UpdateLayout(FALSE);
}

LRESULT DirectoryListingFrame::onSelChangedDirectories(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMTREEVIEW* p = (NMTREEVIEW*) pnmh;

	if(p->itemNew.state & TVIS_SELECTED) {
		DirectoryListing::Directory* d = (DirectoryListing::Directory*)p->itemNew.lParam;
		changeDir(d, TRUE);
		addHistory(dl->getPath(d));
	}
	return 0;
}


void DirectoryListingFrame::addHistory(const string& name) {
	history.erase(history.begin() + historyIndex, history.end());
	while(history.size() > 25)
		history.pop_front();
	history.push_back(name);
	historyIndex = history.size();
}

void DirectoryListingFrame::changeDir(const DirectoryListing::Directory* d, BOOL enableRedraw)

{
	ctrlList.SetRedraw(FALSE);
	updating = true;
	clearList();

	for(auto i = d->directories.begin(); i != d->directories.end(); ++i) {
		ctrlList.insertItem(ctrlList.GetItemCount(), new ItemInfo(*i), (*i)->getComplete() ? WinUtil::getDirIconIndex() : WinUtil::getDirMaskedIndex());
	}
	for(auto j = d->files.begin(); j != d->files.end(); ++j) {
		ItemInfo* ii = new ItemInfo(*j);
		ctrlList.insertItem(ctrlList.GetItemCount(), ii, WinUtil::getIconIndex(ii->getText(COLUMN_FILENAME)));
	}
	ctrlList.resort();
	ctrlList.SetRedraw(enableRedraw);
	updating = false;
	updateStatus();

	if(!d->getComplete()) {
		if (dl->getIsOwnList()) {
			dl->addPartialListTask(dl->getPath(d));
		} else if(dl->getUser()->isOnline()) {
			try {
				// TODO provide hubHint?
				QueueManager::getInstance()->addList(dl->getHintedUser(), QueueItem::FLAG_PARTIAL_LIST | QueueItem::FLAG_CLIENT_VIEW, dl->getPath(d));
				ctrlStatus.SetText(STATUS_TEXT, CTSTRING(DOWNLOADING_LIST));
			} catch(const QueueException& e) {
				ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
			}
		} else {
			ctrlStatus.SetText(STATUS_TEXT, CTSTRING(USER_OFFLINE));
		}
	}
}

void DirectoryListingFrame::up() {
	HTREEITEM t = ctrlTree.GetSelectedItem();
	if(t == NULL)
		return;
	t = ctrlTree.GetParentItem(t);
	if(t == NULL)
		return;
	ctrlTree.SelectItem(t);
}

void DirectoryListingFrame::back() {
	if(history.size() > 1 && historyIndex > 1) {
		size_t n = min(historyIndex, history.size()) - 1;
		deque<string> tmp = history;
		selectItem(Text::toT(history[n - 1]));
		historyIndex = n;
		history = tmp;
	}
}

void DirectoryListingFrame::forward() {
	if(history.size() > 1 && historyIndex < history.size()) {
		size_t n = min(historyIndex, history.size() - 1);
		deque<string> tmp = history;
		selectItem(Text::toT(history[n]));
		historyIndex = n + 1;
		history = tmp;
	}
}

LRESULT DirectoryListingFrame::onDoubleClickFiles(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*) pnmh;

	HTREEITEM t = ctrlTree.GetSelectedItem();
	if(t != NULL && item->iItem != -1) {
		const ItemInfo* ii = ctrlList.getItemData(item->iItem);

		if(ii->type == ItemInfo::FILE) {
			try {
				dl->download(ii->file, SETTING(DOWNLOAD_DIRECTORY) + Text::fromT(ii->getText(COLUMN_FILENAME)), false, WinUtil::isShift(), QueueItem::DEFAULT);
			} catch(const Exception& e) {
				ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
			}
		} else {
			HTREEITEM ht = ctrlTree.GetChildItem(t);
			while(ht != NULL) {
				if((DirectoryListing::Directory*)ctrlTree.GetItemData(ht) == ii->dir) {
					ctrlTree.SelectItem(ht);
					break;
				}
				ht = ctrlTree.GetNextSiblingItem(ht);
			}
		} 
	}
	return 0;
}

LRESULT DirectoryListingFrame::onViewAsText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const ItemInfo* ii = ctrlList.getItemData(i);
		try {
			if(ii->type == ItemInfo::FILE) {
				if (dl->getIsOwnList()) {
					tstring path = Text::toT(ShareManager::getInstance()->getRealPath(ii->file->getTTH()));
					TextFrame::openWindow(path, TextFrame::NORMAL);
				} else {
					File::deleteFile(Util::getTempPath() + Util::validateFileName(ii->file->getName()));
					dl->download(ii->file, Util::getTempPath() + Text::fromT(ii->getText(COLUMN_FILENAME)), true, true);
				}
			}
		} catch(const Exception& e) {
			ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
		}
	}
	return 0;
}

LRESULT DirectoryListingFrame::onSearchByTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	const ItemInfo* ii = ctrlList.getSelectedItem();
	if(ii != NULL && ii->type == ItemInfo::FILE) {
		WinUtil::searchHash(ii->file->getTTH());
	}
	return 0;
}

LRESULT DirectoryListingFrame::onSearchDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	const ItemInfo* ii = ctrlList.getSelectedItem();
	tstring dir;
	if(ii->type == ItemInfo::FILE) {
		dir = Text::toT(Util::getDir(ii->file->getPath(), true, true));
	} else if(ii->type == ItemInfo::DIRECTORY){
		dir = ii->getText(COLUMN_FILENAME);
	}

	WinUtil::searchAny(dir);
	return 0;
}

LRESULT DirectoryListingFrame::onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	changeWindowState(false);
	dl->addQueueMatchTask();
	return 0;
}

LRESULT DirectoryListingFrame::onListDiff(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring file;
	if(WinUtil::browseFile(file, m_hWnd, false, Text::toT(Util::getListPath()), _T("File Lists\0*.xml.bz2\0All Files\0*.*\0"))) {
		DisableWindow();
		ctrlStatus.SetText(0, CTSTRING(MATCHING_FILE_LIST));
		dl->addListDiffTask(Text::fromT(file));
	}
	return 0;
}
LRESULT DirectoryListingFrame::onMatchADL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlStatus.SetText(0, CTSTRING(MATCHING_ADL));
	DisableWindow();
	dl->addMatchADLTask();
	
	return 0;
}

LRESULT DirectoryListingFrame::onGoToDirectory(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlList.GetSelectedCount() != 1) 
		return 0;

	tstring fullPath;
	const ItemInfo* ii = ctrlList.getItemData(ctrlList.GetNextItem(-1, LVNI_SELECTED));
	if(ii->type == ItemInfo::FILE) {
		if(!ii->file->getAdls())
			return 0;
		DirectoryListing::Directory* pd = ii->file->getParent();
		while(pd != NULL && pd != dl->getRoot()) {
			fullPath = Text::toT(pd->getName()) + _T("\\") + fullPath;
			pd = pd->getParent();
		}
	} else if(ii->type == ItemInfo::DIRECTORY)	{
		if(!(ii->dir->getAdls() && ii->dir->getParent() != dl->getRoot()))
			return 0;
		fullPath = Text::toT(((DirectoryListing::AdlDirectory*)ii->dir)->getFullPath()).substr(1);
	}

	if(!fullPath.empty())
		selectItem(fullPath);
	
	return 0;
}

LRESULT DirectoryListingFrame::onFindMissing(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	StringList localPaths;
	string error;
	if(ctrlList.GetSelectedCount() >= 1) {
		int sel = -1;
		while((sel = ctrlList.GetNextItem(sel, LVNI_SELECTED)) != -1) {
			try {
				const ItemInfo* ii = ctrlList.getItemData(sel);
				if(ii->type == ItemInfo::FILE) {
					string path = ShareManager::getInstance()->getRealPath(ii->file->getTTH());
					localPaths.push_back(Util::getFilePath(path));
				} else if(ii->type == ItemInfo::DIRECTORY) {
					dl->getLocalPaths(ii->dir, localPaths);
				}
			
			} catch(ShareException& e) { 
				error = e.getError();
			}
		}
	}
	if(!localPaths.empty()) {
		ctrlStatus.SetText(0, CTSTRING(SEE_SYSLOG_FOR_RESULTS));
		ShareScannerManager::getInstance()->scan(localPaths);
	} else 
		ctrlStatus.SetText(0, Text::toT(error).c_str());
	
	return 0;
}

LRESULT DirectoryListingFrame::onCheckSFV(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	ctrlStatus.SetText(0, CTSTRING(SEE_SYSLOG_FOR_RESULTS));

	StringList scanList;
	StringList temp;
	tstring path;
	string error;
	
	if(ctrlList.GetSelectedCount() >= 1) {
		int sel = -1;
		while((sel = ctrlList.GetNextItem(sel, LVNI_SELECTED)) != -1) {
			const ItemInfo* ii = ctrlList.getItemData(sel);
			try {
				if (ii->type == ItemInfo::FILE) {
					string path = ShareManager::getInstance()->getRealPath(ii->file->getTTH());
					scanList.push_back(path);
				} else if (ii->type == ItemInfo::DIRECTORY)  {
					dl->getLocalPaths(ii->dir, scanList);
				}
			} catch(ShareException& e) { 
				error = e.getError();
			}
		}
	}

	if (!scanList.empty()) {
		ShareScannerManager::getInstance()->scan(scanList, true);
	} else 
		ctrlStatus.SetText(0, Text::toT(error).c_str());

	return 0;
}


HTREEITEM DirectoryListingFrame::findItem(HTREEITEM ht, const tstring& name) {
	string::size_type i = name.find('\\');
	if(i == string::npos)
		return ht;
	
	for(HTREEITEM child = ctrlTree.GetChildItem(ht); child != NULL; child = ctrlTree.GetNextSiblingItem(child)) {
		DirectoryListing::Directory* d = (DirectoryListing::Directory*)ctrlTree.GetItemData(child);
		if(Text::toT(d->getName()) == name.substr(0, i)) {
			return findItem(child, name.substr(i+1));
		}
	}
	return NULL;
}

void DirectoryListingFrame::selectItem(const tstring& name) {
	HTREEITEM ht = findItem(treeRoot, name);
	if(ht != NULL) {
		ctrlTree.EnsureVisible(ht);
		ctrlTree.SelectItem(ht);
	}
}

LRESULT DirectoryListingFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlList && ctrlList.GetSelectedCount() > 0) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlList, pt);
		}
		const ItemInfo* ii = ctrlList.getItemData(ctrlList.GetNextItem(-1, LVNI_SELECTED));
		int dirs = 0;
		OMenu fileMenu, copyMenu, SearchMenu;



		if(BOOLSETTING(SHOW_SHELL_MENU) && dl->getIsOwnList() && (ctrlList.GetSelectedCount() == 1) && (LOBYTE(LOWORD(GetVersion())) >= 5)) {
			tstring path; 
	
			if(ii->type == ItemInfo::FILE){
				try {
					path = Text::toT(ShareManager::getInstance()->getRealPath(ii->file->getTTH()));
				}catch(...) {
					goto clientmenu;
				}
			} else if(ii->type == ItemInfo::DIRECTORY) {
				try {
					//Fix this Someway
					StringList localPaths;
					dl->getLocalPaths(ii->dir, localPaths);
					for(auto i = localPaths.begin(); i != localPaths.end(); i++) {
						path = Text::toT(*i);
						dirs++; //if we count more than 1 its a virtualfolder
					}	
				} catch(...) {
					goto clientmenu;
				}
			}
			
			if(GetFileAttributes(path.c_str()) != 0xFFFFFFFF && !path.empty() && (dirs <= 1) ){ // Check that the file still exists
				CShellContextMenu shellMenu;
				shellMenu.SetPath(path);
				CMenu* pShellMenu = shellMenu.GetMenu();
					
				//do we need to see anything else on own list?
				copyMenu.CreatePopupMenu();
				SearchMenu.CreatePopupMenu();
				if(ctrlList.GetSelectedCount() == 1 && ii->type == ItemInfo::FILE && ii->file->getAdls()) {
					pShellMenu->AppendMenu(MF_STRING, IDC_GO_TO_DIRECTORY, CTSTRING(GO_TO_DIRECTORY));
				}
				if(ctrlList.GetSelectedCount() == 1/* && ii->type == ItemInfo::FILE*/ ) {				
					pShellMenu->AppendMenu(MF_STRING, IDC_OPEN_FOLDER, CTSTRING(OPEN_FOLDER));
					pShellMenu->AppendMenu(MF_SEPARATOR);
				}
					
				pShellMenu->AppendMenu(MF_POPUP, (UINT)(HMENU)copyMenu, CTSTRING(COPY));
				pShellMenu->AppendMenu(MF_STRING, IDC_VIEW_AS_TEXT, CTSTRING(VIEW_AS_TEXT));
				pShellMenu->AppendMenu(MF_STRING, IDC_FINDMISSING, CTSTRING(SCAN_FOLDER_MISSING));
				pShellMenu->AppendMenu(MF_STRING, IDC_CHECKSFV, CTSTRING(RUN_SFV_CHECK));
				pShellMenu->AppendMenu(MF_SEPARATOR);
				pShellMenu->AppendMenu(MF_STRING, IDC_SEARCH, CTSTRING(SEARCH));
				pShellMenu->AppendMenu(MF_STRING, IDC_SEARCHDIR, CTSTRING(SEARCH_DIRECTORY));
				pShellMenu->AppendMenu(MF_SEPARATOR);
				pShellMenu->AppendMenu(MF_POPUP, (UINT)(HMENU)SearchMenu, CTSTRING(SEARCH_SITES));
				pShellMenu->AppendMenu(MF_SEPARATOR);
				//copyMenu.InsertSeparatorFirst(CTSTRING(COPY));
				copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK, CTSTRING(COPY_NICK));
				copyMenu.AppendMenu(MF_STRING, IDC_COPY_FILENAME, CTSTRING(FILENAME));
				copyMenu.AppendMenu(MF_STRING, IDC_COPY_DIR, CTSTRING(DIRECTORY));
				copyMenu.AppendMenu(MF_STRING, IDC_COPY_SIZE, CTSTRING(SIZE));
				copyMenu.AppendMenu(MF_STRING, IDC_COPY_EXACT_SIZE, CTSTRING(EXACT_SIZE));
				copyMenu.AppendMenu(MF_STRING, IDC_COPY_DATE, CTSTRING(DATE));
				copyMenu.AppendMenu(MF_STRING, IDC_COPY_TTH, CTSTRING(TTH_ROOT));
				copyMenu.AppendMenu(MF_STRING, IDC_COPY_LINK, CTSTRING(COPY_MAGNET_LINK));
				copyMenu.AppendMenu(MF_STRING, IDC_COPY_PATH, CTSTRING(PATH));
			
				//SearchMenu.InsertSeparatorFirst(CTSTRING(SEARCH_SITES));
				WinUtil::AppendSearchMenu(SearchMenu);

				UINT idCommand = shellMenu.ShowContextMenu(m_hWnd, pt);
				if(idCommand != 0) {
					PostMessage(WM_COMMAND, idCommand);
				}
					
			} else {
				goto clientmenu;
			}
		} else {

clientmenu:
			fileMenu.CreatePopupMenu();
			copyMenu.CreatePopupMenu();
			SearchMenu.CreatePopupMenu();

			copyMenu.InsertSeparatorFirst(CTSTRING(COPY));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK, CTSTRING(COPY_NICK));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_FILENAME, CTSTRING(FILENAME));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_DIR, CTSTRING(DIRECTORY));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_SIZE, CTSTRING(SIZE));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_EXACT_SIZE, CTSTRING(EXACT_SIZE));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_TTH, CTSTRING(TTH_ROOT));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_LINK, CTSTRING(COPY_MAGNET_LINK));
			copyMenu.AppendMenu(MF_STRING, IDC_COPY_PATH, CTSTRING(PATH));
		
			targets.clear();
			if(ctrlList.GetSelectedCount() == 1 && ii->type == ItemInfo::FILE) {
				targets = QueueManager::getInstance()->getTargets(ii->file->getTTH());
			}

			appendDownloadMenu(fileMenu, DownloadBaseHandler::FILELIST, false);
			fileMenu.AppendMenu(MF_STRING, IDC_VIEW_AS_TEXT, CTSTRING(VIEW_AS_TEXT));
			fileMenu.AppendMenu(MF_SEPARATOR);
		
			if (ctrlList.GetSelectedCount() == 1 && !dl->getIsOwnList()) {
				if (ii->type == ItemInfo::DIRECTORY && (ShareManager::getInstance()->isDirShared(ii->dir->getPath()) || ii->dir->getDupe() == QUEUE_DUPE)) {
					fileMenu.AppendMenu(MF_STRING, IDC_OPEN_FOLDER, CTSTRING(OPEN_FOLDER));
					fileMenu.AppendMenu(MF_SEPARATOR);
				} else if(ii->type == ItemInfo::FILE && (ii->file->getDupe() == SHARE_DUPE || ii->file->getDupe() == FINISHED_DUPE)) {
					fileMenu.AppendMenu(MF_STRING, IDC_OPEN, CTSTRING(OPEN));
					fileMenu.AppendMenu(MF_STRING, IDC_OPEN_FOLDER, CTSTRING(OPEN_FOLDER));
					fileMenu.AppendMenu(MF_SEPARATOR);
				}
			}

			if(dl->getIsOwnList() && !(ii->type == ItemInfo::DIRECTORY && ii->dir->getAdls())) {
				fileMenu.AppendMenu(MF_STRING, IDC_FINDMISSING, CTSTRING(SCAN_FOLDER_MISSING));
				fileMenu.AppendMenu(MF_STRING, IDC_CHECKSFV, CTSTRING(RUN_SFV_CHECK)); //sfv checker
				if(ctrlList.GetSelectedCount() == 1) {				
					fileMenu.AppendMenu(MF_STRING, IDC_OPEN_FOLDER, CTSTRING(OPEN_FOLDER));
				}
				fileMenu.AppendMenu(MF_SEPARATOR);
			}

			fileMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_TTH));
			fileMenu.AppendMenu(MF_STRING, IDC_SEARCHDIR, CTSTRING(SEARCH_DIRECTORY));
			fileMenu.AppendMenu(MF_STRING, IDC_SEARCH, CTSTRING(SEARCH));

			fileMenu.AppendMenu(MF_SEPARATOR);

			//Search menus
			fileMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)SearchMenu, CTSTRING(SEARCH_SITES));
			SearchMenu.InsertSeparatorFirst(CTSTRING(SEARCH_SITES));
			WinUtil::AppendSearchMenu(SearchMenu);

			fileMenu.AppendMenu(MF_SEPARATOR);

			fileMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)copyMenu, CTSTRING(COPY));

			fileMenu.SetMenuDefaultItem(IDC_DOWNLOAD);
			if(ctrlList.GetSelectedCount() == 1 && ii->type == ItemInfo::FILE) {
				fileMenu.InsertSeparatorFirst(Text::toT(Util::getFileName(ii->file->getName())));
				if(ii->file->getAdls())	{
					fileMenu.AppendMenu(MF_STRING, IDC_GO_TO_DIRECTORY, CTSTRING(GO_TO_DIRECTORY));
				}
				fileMenu.EnableMenuItem(IDC_SEARCH_ALTERNATES, MF_BYCOMMAND | MFS_ENABLED);
			} else {
				fileMenu.EnableMenuItem(IDC_SEARCH_ALTERNATES, MF_BYCOMMAND | MFS_DISABLED);
				if(ii->type == ItemInfo::DIRECTORY && ii->type == ItemInfo::DIRECTORY && ii->dir->getAdls() && ii->dir->getParent() != dl->getRoot()) {
					fileMenu.AppendMenu(MF_STRING, IDC_GO_TO_DIRECTORY, CTSTRING(GO_TO_DIRECTORY));
				}
			}

			prepareMenu(fileMenu, UserCommand::CONTEXT_FILELIST, ClientManager::getInstance()->getHubUrls(dl->getHintedUser().user->getCID(), dl->getHintedUser().hint));
			fileMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

		}
		return TRUE; 
	} else if(reinterpret_cast<HWND>(wParam) == ctrlTree && ctrlTree.GetSelectedItem() != NULL) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		
		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlTree, pt);
		} else {
			ctrlTree.ScreenToClient(&pt);
			UINT a = 0;
			HTREEITEM ht = ctrlTree.HitTest(pt, &a);
			if(ht != NULL && ht != ctrlTree.GetSelectedItem())
				ctrlTree.SelectItem(ht);
			ctrlTree.ClientToScreen(&pt);
		}

		OMenu directoryMenu, SearchMenu;

		directoryMenu.CreatePopupMenu();
		SearchMenu.CreatePopupMenu();

		appendDownloadMenu(directoryMenu, DownloadBaseHandler::FILELIST, true);
		directoryMenu.AppendMenu(MF_SEPARATOR);

		directoryMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)SearchMenu, CTSTRING(SEARCH_SITES));
		SearchMenu.InsertSeparatorFirst(CTSTRING(SEARCH_SITES));
		WinUtil::AppendSearchMenu(SearchMenu, 90);
		directoryMenu.AppendMenu(MF_SEPARATOR);

		directoryMenu.AppendMenu(MF_STRING,IDC_COPY_DIRECTORY, CTSTRING(COPY_DIRECTORY));
		directoryMenu.AppendMenu(MF_STRING, IDC_SEARCHLEFT, CTSTRING(SEARCH));
		
		// Strange, windows doesn't change the selection on right-click... (!)
			
		directoryMenu.InsertSeparatorFirst(TSTRING(DIRECTORY));
		directoryMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
			
		return TRUE; 
	} 
	
	bHandled = FALSE;
	return FALSE; 
}

LRESULT DirectoryListingFrame::onXButtonUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /* lParam */, BOOL& /* bHandled */) {
	if(GET_XBUTTON_WPARAM(wParam) & XBUTTON1) {
		back();
		return TRUE;
	} else if(GET_XBUTTON_WPARAM(wParam) & XBUTTON2) {
		forward();
		return TRUE;
	}

	return FALSE;
}

void DirectoryListingFrame::download(const string& aTarget, QueueItem::Priority prio, bool usingTree, TargetUtil::TargetType aTargetType) {
	if (usingTree) {
		HTREEITEM t = ctrlTree.GetSelectedItem();
		auto dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
		try {
			dl->download(dir, aTarget, WinUtil::isShift(), prio);
		} catch(const Exception& e) {
			ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
		}
	} else if(ctrlList.GetSelectedCount() >= 1) {
		int i = -1;
		while( (i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			const ItemInfo* ii = ctrlList.getItemData(i);

			try {
				if(ii->type == ItemInfo::FILE) {
					dl->download(ii->file, aTarget + (aTargetType == TargetUtil::NO_APPEND ? Util::emptyString : Text::fromT(ii->getText(COLUMN_FILENAME))), false, 
						WinUtil::isShift(), prio);
				} else {
					dl->download(ii->dir, aTarget, WinUtil::isShift(), prio);
				} 
			} catch(const Exception& e) {
				ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
			}
		}
	}
}

bool DirectoryListingFrame::showDirDialog(string& fileName) {
	//could be improved
	if (ctrlList.GetSelectedCount() == 1) {
		const ItemInfo* ii = ctrlList.getItemData(ctrlList.GetNextItem(-1, LVNI_SELECTED));
		if (ii->type == ItemInfo::DIRECTORY) {
			return true;
		} else {
			fileName = ii->file->getName();
			return false;
		}
	}

	return true;
}

void DirectoryListingFrame::appendDownloadItems(OMenu& aMenu, bool isTree) {
	//Append general items
	aMenu.AppendMenu(MF_STRING, isTree ? IDC_DOWNLOADDIR : IDC_DOWNLOAD, CTSTRING(DOWNLOAD));

	auto targetMenu = aMenu.createSubMenu(TSTRING(DOWNLOAD_TO));
	targetMenu->InsertSeparatorFirst(TSTRING(DOWNLOAD_TO));
	appendDownloadTo(*targetMenu, isTree ? true : false);

	//Append the "Download with prio" menu
	appendPriorityMenu(aMenu, isTree);
}

int64_t DirectoryListingFrame::getDownloadSize(bool isWhole) {
	int64_t size = 0;
	if (isWhole) {
		HTREEITEM t = ctrlTree.GetSelectedItem();
		auto dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
		size = dir->getSize();
	} else {
		int i = -1;
		while( (i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			const ItemInfo* ii = ctrlList.getItemData(i);
			if (ii->type == ItemInfo::DIRECTORY) {
				size += ii->dir->getSize();
			} else {
				size += ii->file->getSize();
			}
		}
	}
	return size;
}

LRESULT DirectoryListingFrame::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	if(kd->wVKey == VK_BACK) {
		up();
	} else if(kd->wVKey == VK_TAB) {
		onTab();
	} else if(kd->wVKey == VK_LEFT && WinUtil::isAlt()) {
		back();
	} else if(kd->wVKey == VK_RIGHT && WinUtil::isAlt()) {
		forward();
	} else if(kd->wVKey == VK_RETURN) {
		if(ctrlList.GetSelectedCount() == 1) {
			const ItemInfo* ii = ctrlList.getItemData(ctrlList.GetNextItem(-1, LVNI_SELECTED));
			if(ii->type == ItemInfo::DIRECTORY) {
				HTREEITEM ht = ctrlTree.GetChildItem(ctrlTree.GetSelectedItem());
				while(ht != NULL) {
					if((DirectoryListing::Directory*)ctrlTree.GetItemData(ht) == ii->dir) {
						ctrlTree.SelectItem(ht);
						break;
					}
					ht = ctrlTree.GetNextSiblingItem(ht);
				}
			} else {
				download(SETTING(DOWNLOAD_DIRECTORY), QueueItem::DEFAULT, false);
			}
		} else {
			download(SETTING(DOWNLOAD_DIRECTORY), QueueItem::DEFAULT, false);
		}
	}
	return 0;
}

void DirectoryListingFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[STATUS_LAST];
		ctrlStatus.GetClientRect(sr);
		w[STATUS_DUMMY-1] = sr.right - 16;
		for(int i = STATUS_DUMMY - 2; i >= 0; --i) {
			w[i] = max(w[i+1] - statusSizes[i+1], 0);
		}

		ctrlStatus.SetParts(STATUS_LAST, w);
		ctrlStatus.GetRect(0, sr);

		sr.left = w[STATUS_GET_FULL_LIST - 1];
		sr.right = w[STATUS_GET_FULL_LIST];
		ctrlGetFullList.MoveWindow(sr);

		sr.left = w[STATUS_MATCH_ADL - 1];
		sr.right = w[STATUS_MATCH_ADL];
		ctrlADLMatch.MoveWindow(sr);

		sr.left = w[STATUS_FILE_LIST_DIFF - 1];
		sr.right = w[STATUS_FILE_LIST_DIFF];
		ctrlListDiff.MoveWindow(sr);

		sr.left = w[STATUS_MATCH_QUEUE - 1];
		sr.right = w[STATUS_MATCH_QUEUE];
		ctrlMatchQueue.MoveWindow(sr);

		sr.left = w[STATUS_FIND - 1];
		sr.right = w[STATUS_FIND];
		ctrlFind.MoveWindow(sr);

		sr.left = w[STATUS_NEXT - 1];
		sr.right = w[STATUS_NEXT];
		ctrlFindNext.MoveWindow(sr);
	}

	SetSplitterRect(&rect);
}

void DirectoryListingFrame::runUserCommand(UserCommand& uc) {
	if(!WinUtil::getUCParams(m_hWnd, uc, ucLineParams))
		return;

	auto ucParams = ucLineParams;

	set<UserPtr> nicks;

	int sel = -1;
	while((sel = ctrlList.GetNextItem(sel, LVNI_SELECTED)) != -1) {
		const ItemInfo* ii = ctrlList.getItemData(sel);
		if(uc.once()) {
			if(nicks.find(dl->getUser()) != nicks.end())
				continue;
			nicks.insert(dl->getUser());
		}
		if(!dl->getUser()->isOnline())
			return;
		//ucParams["fileTR"] = "NONE";
		if(ii->type == ItemInfo::FILE) {
			ucParams["type"] = [] { return "File"; };
			ucParams["fileFN"] = [this, ii] { return dl->getPath(ii->file) + ii->file->getName(); };
			ucParams["fileSI"] = [ii] { return Util::toString(ii->file->getSize()); };
			ucParams["fileSIshort"] = [ii] { return Util::formatBytes(ii->file->getSize()); };
			ucParams["fileTR"] = [ii] { return ii->file->getTTH().toBase32(); };
			ucParams["fileMN"] = [ii] { return WinUtil::makeMagnet(ii->file->getTTH(), ii->file->getName(), ii->file->getSize()); };
		} else {
			ucParams["type"] = [] { return "Directory"; };
			ucParams["fileFN"] = [this, ii] { return dl->getPath(ii->dir) + ii->dir->getName(); };
			ucParams["fileSI"] = [ii] { return Util::toString(ii->dir->getTotalSize()); };
			ucParams["fileSIshort"] = [ii] { return Util::formatBytes(ii->dir->getTotalSize()); };
		}

		// compatibility with 0.674 and earlier
		ucParams["file"] = ucParams["fileFN"];
		ucParams["filesize"] = ucParams["fileSI"];
		ucParams["filesizeshort"] = ucParams["fileSIshort"];
		ucParams["tth"] = ucParams["fileTR"];

		auto tmp = ucParams;
		UserPtr tmpPtr = dl->getUser();
		ClientManager::getInstance()->userCommand(dl->getHintedUser(), uc, tmp, true);
	}
}

void DirectoryListingFrame::closeAll(){
	for(FrameIter i = frames.begin(); i != frames.end(); ++i)
		i->second->PostMessage(WM_CLOSE, 0, 0);
}

LRESULT DirectoryListingFrame::onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring sCopy;
	if(ctrlList.GetSelectedCount() >= 1) {
		int xsel = ctrlList.GetNextItem(-1, LVNI_SELECTED);
		for (;;) {
			const ItemInfo* ii = ctrlList.getItemData(xsel);
			switch (wID) {
				case IDC_COPY_NICK:
					sCopy += WinUtil::getNicks(dl->getHintedUser());
					break;
				case IDC_COPY_FILENAME:
					sCopy += ii->getText(COLUMN_FILENAME);
					break;
				case IDC_COPY_SIZE:
					sCopy += ii->getText(COLUMN_SIZE);
					break;
				case IDC_COPY_EXACT_SIZE:
					sCopy += ii->getText(COLUMN_EXACTSIZE);
					break;
				case IDC_COPY_LINK:
					if(ii->type == ItemInfo::FILE) {
						sCopy += Text::toT(WinUtil::makeMagnet(ii->file->getTTH(), ii->file->getName(), ii->file->getSize()));
					} else if(ii->type == ItemInfo::DIRECTORY){
						sCopy += Text::toT("Directories don't have Magnet links");
					}
					break;
				case IDC_COPY_DATE:
					sCopy += ii->getText(COLUMN_DATE);
					break;
				case IDC_COPY_TTH:
					sCopy += ii->getText(COLUMN_TTH);
					break;
				case IDC_COPY_PATH:
					if(ii->type == ItemInfo::FILE){
						sCopy += Text::toT(ii->file->getPath() + ii->file->getName());
					} else if(ii->type == ItemInfo::DIRECTORY){
						if(ii->dir->getAdls() && ii->dir->getParent() != dl->getRoot()) {
							sCopy += Text::toT(((DirectoryListing::AdlDirectory*)ii->dir)->getFullPath().substr(1));
						} else {
							sCopy += Text::toT(ii->dir->getPath());
						}
					}
					break;
				case IDC_COPY_DIR:
					if(ii->type == ItemInfo::FILE){
						sCopy += Text::toT(Util::getDir(ii->file->getPath(), true, true));
					} else if(ii->type == ItemInfo::DIRECTORY){
						sCopy += ii->getText(COLUMN_FILENAME);
					}
					break;
				default:
					dcdebug("DIRECTORYLISTINGFRAME DON'T GO HERE\n");
					return 0;
			}
			xsel = ctrlList.GetNextItem(xsel, LVNI_SELECTED);
			if (xsel == -1) {
				break;
			}

			sCopy += Text::toT("\r\n");
		}

		if (!sCopy.empty())
			WinUtil::setClipboard(sCopy);
	}
	return S_OK;
}

LRESULT DirectoryListingFrame::onCopyDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring sCopy;
	tstring directory;
	HTREEITEM t = ctrlTree.GetSelectedItem();
	DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
	sCopy = Text::toT((dir)->getName());
	if (!sCopy.empty()) {
		WinUtil::setClipboard(sCopy);
	}
	return S_OK;
}


LRESULT DirectoryListingFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	//tell the thread to abort and wait until we get a notification
	//that it's done.
	dl->setAbort(true);
	
	if(!closed) {
		SettingsManager::getInstance()->removeListener(this);

		ctrlList.SetRedraw(FALSE);
		clearList();
		frames.erase(m_hWnd);

		ctrlList.saveHeaderOrder(SettingsManager::DIRECTORYLISTINGFRAME_ORDER, SettingsManager::DIRECTORYLISTINGFRAME_WIDTHS,
			SettingsManager::DIRECTORYLISTINGFRAME_VISIBLE);

		closed = true;
		PostMessage(WM_CLOSE);
		//changeWindowState(false);
		//ctrlStatus.SetText(0, _T("Closing down, please wait..."));
		//dl->close();
		return 0;
	} else {
		CRect rc;
		if(!IsIconic()){
			//Get position of window
			GetWindowRect(&rc);
				
			//convert the position so it's relative to main window
			::ScreenToClient(GetParent(), &rc.TopLeft());
			::ScreenToClient(GetParent(), &rc.BottomRight());
				
			//save the position
			SettingsManager::getInstance()->set(SettingsManager::DIRLIST_BOTTOM, (rc.bottom > 0 ? rc.bottom : 0));
			SettingsManager::getInstance()->set(SettingsManager::DIRLIST_TOP, (rc.top > 0 ? rc.top : 0));
			SettingsManager::getInstance()->set(SettingsManager::DIRLIST_LEFT, (rc.left > 0 ? rc.left : 0));
			SettingsManager::getInstance()->set(SettingsManager::DIRLIST_RIGHT, (rc.right > 0 ? rc.right : 0));
		}
		bHandled = FALSE;
		dl->removeListener(this);
		DirectoryListingManager::getInstance()->removeList(dl->getUser());
		return 0;
	}
}

LRESULT DirectoryListingFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 

	OMenu tabMenu;
	tabMenu.CreatePopupMenu();
	appendUserItems(tabMenu);
	tabMenu.AppendMenu(MF_SEPARATOR);
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));

	tstring nick = Text::toT(ClientManager::getInstance()->getNicks(dl->getHintedUser())[0]);

	tabMenu.InsertSeparatorFirst(nick);
	tabMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	return TRUE;
}

void DirectoryListingFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	bool refresh = false;
	if(ctrlList.GetBkColor() != WinUtil::bgColor) {
		ctrlList.SetBkColor(WinUtil::bgColor);
		ctrlList.SetTextBkColor(WinUtil::bgColor);
		ctrlTree.SetBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlList.GetTextColor() != WinUtil::textColor) {
		ctrlList.SetTextColor(WinUtil::textColor);
		ctrlTree.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

LRESULT DirectoryListingFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	switch(wParam) {
		case FINISHED:
			initStatus();
			ctrlStatus.SetFont(WinUtil::systemFont);
			//tstring tmp = TSTRING(LOADED_FILE_LIST) + Util::formatSeconds(loadTime);
			//tmp.resize(STRING(LOADED_FILE_LIST).size() + 16);
			//tmp.resize(snprintf(&tmp[0], tmp.size(), CSTRING(LOADED_FILE_LIST), Util::formatSeconds(loadTime)));
			ctrlStatus.SetText(0, (TSTRING(LOADED_FILE_LIST) + Util::formatSeconds(loadTime, true)).c_str());
			changeWindowState(true);
			//notify the user that we've loaded the list
			setDirty();
			break;
		case ABORTED:
			PostMessage(WM_CLOSE, 0, 0);
			break;
		case STARTED:
			ctrlStatus.SetText(0, CTSTRING(LOADING_FILE_LIST));
			break;
		case UPDATE_STATUS:
			{
				auto_ptr<tstring> msg(reinterpret_cast<tstring*>(lParam));
				ctrlStatus.SetText(0, (*msg).c_str());
				break;
			}
		default: dcassert(0); break;
	}
	return 0;
}

LRESULT DirectoryListingFrame::onCustomDrawList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {

	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;
	switch(cd->nmcd.dwDrawStage) {

	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT: {
		ItemInfo *ii = reinterpret_cast<ItemInfo*>(cd->nmcd.lItemlParam);

		if(!SETTING(HIGHLIGHT_LIST).empty() && !dl->getIsOwnList() && ii->type == ItemInfo::DIRECTORY) {
			//Todo Regex string?
			if(Wildcard::patternMatch(ii->dir->getName(), SETTING(HIGHLIGHT_LIST), '|')) {
				cd->clrText = SETTING(LIST_HL_COLOR);
				cd->clrTextBk = SETTING(LIST_HL_BG_COLOR);
			}
		}
		
		if (SETTING(DUPES_IN_FILELIST) && !dl->getIsOwnList() && ii != NULL) {
			cd->clrText = ii->type == ItemInfo::FILE ? WinUtil::getDupeColor(ii->file->getDupe()) : WinUtil::getDupeColor(ii->dir->getDupe());
		}
		return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
	}

	default:
		return CDRF_DODEFAULT;
	}
}

LRESULT DirectoryListingFrame::onCustomDrawTree(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {

	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;
	switch(cd->nmcd.dwDrawStage) {

	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT: {

		if (SETTING(DUPES_IN_FILELIST) && !dl->getIsOwnList()) {
			DirectoryListing::Directory* dir = reinterpret_cast<DirectoryListing::Directory*>(cd->nmcd.lItemlParam);
			if(dir) {
				cd->clrText = WinUtil::getDupeColor(dir->getDupe());
			}
		}
		return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
	}

	default:
		return CDRF_DODEFAULT;
	}
}


LRESULT DirectoryListingFrame::onOpenDupe(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	const ItemInfo* ii = ctrlList.getSelectedItem();
	try {
		tstring path;
		if(ii->type == ItemInfo::FILE) {
			if (ii->file->getDupe() == SHARE_DUPE || dl->getIsOwnList()) {
				path = Text::toT(ShareManager::getInstance()->getRealPath(ii->file->getTTH()));
			} else {
				StringList targets = QueueManager::getInstance()->getTargets(ii->file->getTTH());
				if (!targets.empty()) {
					path = Text::toT(targets.front());
				}
			}

			if (path.empty()) {
				return 0;
			}
		} else if(ii->type == ItemInfo::DIRECTORY) {
			if (dl->getIsOwnList()) {
				StringList tmp;

				if(ii->dir->getAdls())
					return 0;

				dl->getLocalPaths(ii->dir, tmp);

				if(tmp.empty())
					return 0;

				path = Text::toT(*tmp.begin());  //could open all virtualfolders but some have so many.

			} else {
				if (ii->dir->getDupe() == SHARE_DUPE || (dl->getPartialList() && ii->dir->getDupe() == PARTIAL_SHARE_DUPE)) {
					path = ShareManager::getInstance()->getDirPath(ii->dir->getPath());
				} else {
					path = QueueManager::getInstance()->getDirPath(ii->dir->getPath());
				}
				if (path.empty()) {
					return 0;
				}
			}
		} else if(ii->dir->getFileCount() > 0) {
			DirectoryListing::File::Iter i = ii->dir->files.begin();
			for (; i != ii->dir->files.end(); ++i) {
				if((*i)->getDupe() ==SHARE_DUPE)
					break;
			}
			if(i != ii->dir->files.end()) {
				path = Text::toT(ShareManager::getInstance()->getRealPath(((*i)->getTTH())));
				wstring::size_type end = path.find_last_of(_T("\\")); //makes it open the above folder if dir is selected with open folder
				if (end != wstring::npos) {
					path = path.substr(0, end);
				}
			}
		}

		if(wID == IDC_OPEN) {
			WinUtil::openFile(path);
		} else {
			WinUtil::openFolder(path);
		}
	} catch(const ShareException& e) {
		//error = Text::toT(e.getError());
		ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
	}

	return 0;
}

LRESULT DirectoryListingFrame::onSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	tstring searchTerm;
	if(ctrlList.GetSelectedCount() == 1) {
		const ItemInfo* ii = ctrlList.getSelectedItem();
		searchTerm = ii->getText(COLUMN_FILENAME);
	} else {
		HTREEITEM t = ctrlTree.GetSelectedItem();
		if(t != NULL) {
			DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
			searchTerm = Text::toT((dir)->getName());
		}
	}

	WinUtil::searchAny(searchTerm);
	return 0;
}

LRESULT DirectoryListingFrame::onSearchSite(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	if(ctrlList.GetSelectedCount() >= 1) {
		size_t newId = (size_t)wID - IDC_SEARCH_SITES;
		if(newId < WebShortcuts::getInstance()->list.size()) {
			tstring searchTermFull;
			WebShortcut *ws = WebShortcuts::getInstance()->list[newId];
		
			int sel = -1;
			while((sel = ctrlList.GetNextItem(sel, LVNI_SELECTED)) != -1) {
				const ItemInfo* ii =  ctrlList.getItemData(sel);

				if(ii->type == ItemInfo::FILE && (SETTING(SETTINGS_PROFILE) == 1)){
					searchTermFull = Text::toT(Util::getDir(ii->file->getPath(), true, true));
				} else {
					searchTermFull = ii->getText(COLUMN_FILENAME);
				}
				if(ws != NULL) 
					WinUtil::SearchSite(ws, searchTermFull); 
			}
		}
	}
	return S_OK;
}

LRESULT DirectoryListingFrame::onSearchLeft(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	tstring searchTerm;
	HTREEITEM t = ctrlTree.GetSelectedItem();
	if(t != NULL) {
		DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
		searchTerm = Text::toT((dir)->getName());
		WinUtil::searchAny(searchTerm);
	}
	return 0;
}


LRESULT DirectoryListingFrame::onSearchSiteDir(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	tstring searchTermFull;

	HTREEITEM t = ctrlTree.GetSelectedItem();
	if(t != NULL) {
		DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
		searchTermFull = Text::toT((dir)->getName());

		size_t newId = (size_t)wID - IDC_SEARCH_SITES -90;
		if(newId < WebShortcuts::getInstance()->list.size()) {
			WebShortcut *ws = WebShortcuts::getInstance()->list[newId];
			if(ws != NULL) 
				WinUtil::SearchSite(ws, searchTermFull); 
		}
	}

	return S_OK;
}
/**
 * @file
 * $Id: DirectoryListingFrm.cpp 486 2010-02-27 16:44:26Z bigmuscle $
 */
