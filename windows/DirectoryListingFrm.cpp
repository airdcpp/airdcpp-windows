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

#include "stdafx.h"

#include "Resource.h"

#include "DirectoryListingFrm.h"
#include "DirectoryListingDlg.h"

#include "WinUtil.h"
#include "ResourceLoader.h"
#include "LineDlg.h"
#include "ShellContextMenu.h"
#include "ListFilter.h"

#include "../client/HighlightManager.h"
#include "../client/File.h"
#include "../client/QueueManager.h"
#include "../client/StringTokenizer.h"
#include "../client/ADLSearch.h"
#include "../client/User.h"
#include "../client/ClientManager.h"
#include "../client/ShareScannerManager.h"
#include "../client/Wildcards.h"
#include "../client/DirectoryListingManager.h"
#include "../client/version.h"
#include "TextFrame.h"


DirectoryListingFrame::FrameMap DirectoryListingFrame::frames;
int DirectoryListingFrame::columnIndexes[] = { COLUMN_FILENAME, COLUMN_TYPE, COLUMN_EXACTSIZE, COLUMN_SIZE, COLUMN_TTH, COLUMN_DATE };
int DirectoryListingFrame::columnSizes[] = { 300, 60, 100, 100, 200, 100 };

static ResourceManager::Strings columnNames[] = { ResourceManager::FILE, ResourceManager::TYPE, ResourceManager::EXACT_SIZE, ResourceManager::SIZE, ResourceManager::TTH_ROOT, ResourceManager::DATE };
static SettingsManager::BoolSetting filterSettings[] = { SettingsManager::FILTER_FL_SHARED, SettingsManager::FILTER_FL_QUEUED, SettingsManager::FILTER_FL_INVERSED };


void DirectoryListingFrame::openWindow(DirectoryListing* aList, const string& aDir, const string& aXML) {

	HWND aHWND = NULL;
	DirectoryListingFrame* frame = new DirectoryListingFrame(aList);
	if((SETTING(POPUNDER_FILELIST) && !aList->getPartialList()) || (SETTING(POPUNDER_PARTIAL_LIST) && aList->getPartialList())) {
		aHWND = WinUtil::hiddenCreateEx(frame);
	} else {
		aHWND = frame->CreateEx(WinUtil::mdiClient);
	}
	if(aHWND != 0) {
		if (aList->getPartialList()) {
			aList->addPartialListTask(aDir, aXML, false);
		} else {
			frame->ctrlStatus.SetText(0, CTSTRING(LOADING_FILE_LIST));
			aList->addFullListTask(aDir);
		}
		frames.emplace(frame->m_hWnd, frame);
	} else {
		delete frame;
	}
}

DirectoryListingFrame::DirectoryListingFrame(DirectoryListing* aList) :
	pathContainer(WC_COMBOBOX, this, PATH_MESSAGE_MAP), treeContainer(WC_TREEVIEW, this, CONTROL_MESSAGE_MAP),
	listContainer(WC_LISTVIEW, this, CONTROL_MESSAGE_MAP), historyIndex(1),
		treeRoot(NULL), skipHits(0), files(0), updating(false), dl(aList), 
		UserInfoBaseHandler(true, false), changeType(CHANGE_LIST), disabled(false), ctrlTree(this), statusDirty(false), selComboContainer(WC_COMBOBOX, this, COMBO_SEL_MAP), ctrlFiles(this, COLUMN_LAST, [this] { filterList(); }, filterSettings)
{
	dl->addListener(this);
}

DirectoryListingFrame::~DirectoryListingFrame() {
	dl->join();
	dl->removeListener(this);

	// dl will be automatically deleted by DirectoryListingManager
	DirectoryListingManager::getInstance()->removeList(dl->getUser());
}

void DirectoryListingFrame::on(DirectoryListingListener::LoadingFinished, int64_t aStart, const string& aDir, bool reloadList, bool changeDir, bool loadInGUIThread) noexcept {
	auto f = [=] { onLoadingFinished(aStart, aDir, reloadList, changeDir, loadInGUIThread); };
	if (loadInGUIThread) {
		callAsync(f);
	} else {
		f();
	}
}

void DirectoryListingFrame::onLoadingFinished(int64_t aStart, const string& aDir, bool reloadList, bool changeDir, bool usingGuiThread) {
	//keep the messages in right order...
	auto runF = [=](std::function<void ()> aF) {
		if (!usingGuiThread) {
			callAsync(aF);
		} else {
			aF();
		}
	};

	bool searching = dl->isCurrentSearchPath(aDir);

	if (!dl->getPartialList())
		runF([=] { updateStatus(CTSTRING(UPDATING_VIEW)); });

	if (searching)
		ctrlFiles.filter.clear();

	try{
		refreshTree(Text::toT(aDir), reloadList, changeDir);
	} catch(const AbortException) {
		return;
	}

	if (!searching) {
		int64_t loadTime = (GET_TICK() - aStart) / 1000;
		string msg;
		if (dl->getPartialList()) {
			if (aDir.empty()) {
				msg = STRING(PARTIAL_LIST_LOADED);
			} /*else if (!usingGuiThread) {
				//msg = STRING_F(DIRECTORY_LOADED, Util::getLastDir(aDir));
				msg = Util::emptyString;
			}*/
		} else {
			msg = STRING_F(FILELIST_LOADED_IN, Util::formatSeconds(loadTime, true));
		}

		changeWindowState(true);

		runF([=] {
			initStatus();
			//if (!msg.empty())
			updateStatus(Text::toT(msg));

			//notify the user that we've loaded the list
			setDirty();
		});
	} else {
		findSearchHit(true);
		changeWindowState(true);
		runF([=] { 
			updateStatus(TSTRING_F(X_RESULTS_FOUND, dl->getResultCount()));
			dl->setWaiting(false);
		});
	}

	dl->setWaiting(false);
}

void DirectoryListingFrame::on(DirectoryListingListener::LoadingFailed, const string& aReason) noexcept {
	if (!closed) {
		callAsync([=] {
			updateStatus(Text::toT(aReason));
			if (!dl->getPartialList()) {
				PostMessage(WM_CLOSE, 0, 0);
			} else {
				changeWindowState(true);
			}
		});
	}
}

void DirectoryListingFrame::on(DirectoryListingListener::LoadingStarted, bool changeDir) noexcept {
	callAsync([=] { 
		if (changeDir) {
			DisableWindow(false);
		} else {
			changeWindowState(false);
			ctrlStatus.SetText(0, CTSTRING(LOADING_FILE_LIST));
		}
		dl->setWaiting(false);
	});
	
}

void DirectoryListingFrame::on(DirectoryListingListener::QueueMatched, const string& aMessage) noexcept {
	callAsync([=] { updateStatus(Text::toT(aMessage)); });
	changeWindowState(true);
}

void DirectoryListingFrame::on(DirectoryListingListener::Close) noexcept {
	callAsync([this] { PostMessage(WM_CLOSE, 0, 0); });
}

void DirectoryListingFrame::on(DirectoryListingListener::SearchStarted) noexcept {
	callAsync([=] { updateStatus(TSTRING(SEARCHING)); });
	changeWindowState(false);
}

void DirectoryListingFrame::on(DirectoryListingListener::SearchFailed, bool timedOut) noexcept {
	callAsync([=] {
		tstring msg;
		if (timedOut) {
			msg = dl->supportsASCH() ? TSTRING(SEARCH_TIMED_OUT) : TSTRING(NO_RESULTS_SPECIFIED_TIME);
		} else {
			msg = TSTRING(NO_RESULTS_FOUND);
		}
		updateStatus(msg); 
	});
	changeWindowState(true);
}

void DirectoryListingFrame::on(DirectoryListingListener::ChangeDirectory, const string& aDir, bool isSearchChange) noexcept {
	if (isSearchChange)
		ctrlFiles.filter.clear();

	selectItem(Text::toT(aDir));
	if (isSearchChange) {
		callAsync([=] { updateStatus(TSTRING_F(X_RESULTS_FOUND, dl->getResultCount())); });
		findSearchHit(true);
	}

	changeWindowState(true);
}

void DirectoryListingFrame::on(DirectoryListingListener::UpdateStatusMessage, const string& aMessage) noexcept {
	callAsync([=] { updateStatus(Text::toT(aMessage)); });
}

void DirectoryListingFrame::createColumns() {
	WinUtil::splitTokens(columnIndexes, SETTING(DIRECTORYLISTINGFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(DIRECTORYLISTINGFRAME_WIDTHS), COLUMN_LAST);
	for (uint8_t j = 0; j < COLUMN_LAST; j++)
	{
		int fmt = ((j == COLUMN_SIZE) || (j == COLUMN_EXACTSIZE) || (j == COLUMN_TYPE)) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlFiles.list.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	ctrlFiles.list.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlFiles.list.setVisible(SETTING(DIRECTORYLISTINGFRAME_VISIBLE));

	ctrlFiles.list.SetImageList(ResourceLoader::getFileImages(), LVSIL_SMALL);
	ctrlFiles.list.setSortColumn(COLUMN_FILENAME);
}

LRESULT DirectoryListingFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {

	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	//ctrlStatus.SetFont(WinUtil::boldFont);
	ctrlStatus.SetFont(WinUtil::systemFont);

	ctrlTree.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP | TVS_TRACKSELECT, WS_EX_CLIENTEDGE, IDC_DIRECTORIES);
	
	if(SETTING(USE_EXPLORER_THEME)) {
		SetWindowTheme(ctrlTree.m_hWnd, L"explorer", NULL);
	}
	
	treeContainer.SubclassWindow(ctrlTree);
	ctrlFiles.Create(m_hWnd);
	listContainer.SubclassWindow(ctrlFiles.list);
	ctrlFiles.list.SetBkColor(WinUtil::bgColor);
	ctrlFiles.list.SetTextBkColor(WinUtil::bgColor);
	ctrlFiles.list.SetTextColor(WinUtil::textColor);
	
	ctrlTree.SetBkColor(WinUtil::bgColor);
	ctrlTree.SetTextColor(WinUtil::textColor);
	ctrlTree.SetImageList(ResourceLoader::getFileImages(), TVSIL_NORMAL);

	selCombo.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE, IDC_HUB);
	selComboContainer.SubclassWindow(selCombo.m_hWnd);
	selCombo.SetFont(WinUtil::systemFont);
	
	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	SetSplitterPanes(ctrlTree.m_hWnd, ctrlFiles.m_hWnd);
	m_nProportionalPos = 2500;
	createRoot();
	
	memzero(statusSizes, sizeof(statusSizes));

	int desclen = WinUtil::getTextWidth(getComboDesc(), ctrlStatus.m_hWnd);
	statusSizes[STATUS_HUB] = 150 + desclen;

	ctrlStatus.SetParts(STATUS_LAST, statusSizes);

	//arrow buttons
	arrowBar.Create(m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST, 0, ATL_IDW_TOOLBAR);
	arrowBar.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS);
	arrowBar.SetImageList(ResourceLoader::getArrowImages());
	arrowBar.SetButtonStructSize();
	addarrowBarButtons();
	arrowBar.AutoSize();

	//path field
	ctrlPath.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL, 0);
	ctrlPath.SetFont(WinUtil::systemFont);
	pathContainer.SubclassWindow(ctrlPath.m_hWnd);

	//Cmd bar
	ctrlToolbar.Create(m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST, 0, ATL_IDW_TOOLBAR);
	ctrlToolbar.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS);
	ctrlToolbar.SetImageList(ResourceLoader::getFilelistTbImages());
	ctrlToolbar.SetButtonStructSize();
	addCmdBarButtons();
	ctrlToolbar.AutoSize();
	
	CreateSimpleReBar(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | RBS_VARHEIGHT | RBS_AUTOSIZE | CCS_NODIVIDER);
	AddSimpleReBarBand(arrowBar.m_hWnd, NULL, FALSE, NULL, TRUE);
	AddSimpleReBarBand(ctrlPath.m_hWnd, NULL, FALSE, 300);
	AddSimpleReBarBand(ctrlToolbar.m_hWnd, NULL, FALSE, NULL, TRUE);
	
	//maximize the path field.
	CReBarCtrl rebar = m_hWndToolBar;
	rebar.MaximizeBand(1);
	rebar.LockBands(true);

	ctrlTree.EnableWindow(FALSE);
	
	SettingsManager::getInstance()->addListener(this);
	closed = false;

	CRect rc(SETTING(DIRLIST_LEFT), SETTING(DIRLIST_TOP), SETTING(DIRLIST_RIGHT), SETTING(DIRLIST_BOTTOM));
	if(! (rc.top == 0 && rc.bottom == 0 && rc.left == 0 && rc.right == 0) )
		MoveWindow(rc, TRUE);
	
	WinUtil::SetIcon(m_hWnd, dl->getIsOwnList() ? IDI_OWNLIST : IDI_OPEN_LIST);
	bHandled = FALSE;

	::SetTimer(m_hWnd, 0, 500, 0);

	if (!dl->getIsOwnList())
		ClientManager::getInstance()->addListener(this);

	callAsync([this] { updateHistoryCombo(); updateSelCombo(); });
	return 1;
}
void DirectoryListingFrame::addarrowBarButtons() {
	
	TBBUTTON nTB;
	memzero(&nTB, sizeof(TBBUTTON));
	
	nTB.iBitmap = 2;
	nTB.idCommand = IDC_BACK;
	nTB.fsState = TBSTATE_ENABLED;
	nTB.fsStyle = BTNS_BUTTON | TBSTYLE_AUTOSIZE;
	nTB.iString = arrowBar.AddStrings(CTSTRING(BACK));
	arrowBar.AddButtons(1, &nTB);

	nTB.iBitmap = 1;
	nTB.idCommand = IDC_FORWARD;
	nTB.fsState = TBSTATE_ENABLED;
	nTB.fsStyle = BTNS_BUTTON | TBSTYLE_AUTOSIZE;
	nTB.iString = arrowBar.AddStrings(CTSTRING(FORWARD));
	arrowBar.AddButtons(1, &nTB);

	nTB.fsStyle = TBSTYLE_SEP;
	arrowBar.AddButtons(1, &nTB);

	nTB.iBitmap = 0;
	nTB.idCommand = IDC_UP;
	nTB.fsState = TBSTATE_ENABLED;
	nTB.fsStyle = BTNS_BUTTON | TBSTYLE_AUTOSIZE;
	nTB.iString = arrowBar.AddStrings(CTSTRING(LEVEL_UP));
	arrowBar.AddButtons(1, &nTB);

}

void DirectoryListingFrame::addCmdBarButtons() {
	TBBUTTON nTB;
	memzero(&nTB, sizeof(TBBUTTON));

	int buttonsCount = sizeof(cmdBarButtons) / sizeof(cmdBarButtons[0]);
	for(int i = 0; i < buttonsCount; i++){
		if(i == 5 || i == 1) {
			nTB.fsStyle = TBSTYLE_SEP;
			ctrlToolbar.AddButtons(1, &nTB);
		}

		nTB.iBitmap = cmdBarButtons[i].image;
		nTB.idCommand = cmdBarButtons[i].id;
		nTB.fsState = TBSTATE_ENABLED;
		nTB.fsStyle = i == 0 ? TBSTYLE_AUTOSIZE : BTNS_SHOWTEXT | TBSTYLE_AUTOSIZE;
		nTB.iString = ctrlToolbar.AddStrings(CTSTRING_I((ResourceManager::Strings)cmdBarButtons[i].tooltip));
		ctrlToolbar.AddButtons(1, &nTB);
	}

	TBBUTTONINFO tbi;
	memzero(&tbi, sizeof(TBBUTTONINFO));
	tbi.cbSize = sizeof(TBBUTTONINFO);
	tbi.dwMask = TBIF_STYLE;

	if(ctrlToolbar.GetButtonInfo(IDC_FILELIST_DIFF, &tbi) != -1) {
		tbi.fsStyle |= BTNS_WHOLEDROPDOWN;
		ctrlToolbar.SetButtonInfo(IDC_FILELIST_DIFF, &tbi);
	}
}

LRESULT DirectoryListingFrame::onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if (statusDirty) {
		updateStatus();
		statusDirty = false;
	}
	return 0;
}

void DirectoryListingFrame::changeWindowState(bool enable) {

	ctrlToolbar.EnableButton(IDC_MATCH_QUEUE, enable && !dl->getIsOwnList());
	ctrlToolbar.EnableButton(IDC_MATCH_ADL, enable);
	ctrlToolbar.EnableButton(IDC_FIND, enable);
	ctrlToolbar.EnableButton(IDC_NEXT, enable && dl->curSearch ? TRUE : FALSE);
	ctrlToolbar.EnableButton(IDC_PREV, enable && dl->curSearch ? TRUE : FALSE);
	ctrlToolbar.EnableButton(IDC_FILELIST_DIFF, enable && dl->getPartialList() && !dl->getIsOwnList() ? false : enable);
	arrowBar.EnableButton(IDC_UP, enable);
	arrowBar.EnableButton(IDC_FORWARD, enable);
	arrowBar.EnableButton(IDC_BACK, enable);
	ctrlPath.EnableWindow(enable);
	ctrlFiles.changeFilterState(enable);
	selCombo.EnableWindow(enable);

	if (enable) {
		EnableWindow();
		ctrlToolbar.EnableButton(IDC_GETLIST, dl->getPartialList() && !dl->getIsOwnList());
		ctrlToolbar.EnableButton(IDC_RELOAD_DIR, dl->getPartialList());
	} else {
		DisableWindow();
		ctrlToolbar.EnableButton(IDC_RELOAD_DIR, FALSE);
		ctrlToolbar.EnableButton(IDC_GETLIST, FALSE);
	}
}

LRESULT DirectoryListingFrame::onGetFullList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	convertToFull();
	return 1;
}

void DirectoryListingFrame::convertToFull() {
	if (dl->getIsOwnList())
		dl->addFullListTask(curPath);
	else {
		try {
			QueueManager::getInstance()->addList(dl->getHintedUser(), QueueItem::FLAG_CLIENT_VIEW, curPath);
		} catch(...) {}
	}
}

ChildrenState DirectoryListingFrame::getChildrenState(const DirectoryListing::Directory* d) const {
	if (!d->directories.empty())
		return !d->isComplete() ? ChildrenState::CHILDREN_PART_PENDING : ChildrenState::CHILDREN_CREATED;

	if (d->getType() == DirectoryListing::Directory::TYPE_INCOMPLETE_CHILD)
		return ChildrenState::CHILDREN_ALL_PENDING;

	//if (d->getLoading() && d->getType() == )
	//	return ChildrenState::CHILDREN_LOADING;

	return ChildrenState::NO_CHILDREN;
}

void DirectoryListingFrame::expandDir(DirectoryListing::Directory* d, bool collapsing) {
	changeType = collapsing ? CHANGE_TREE_COLLAPSE : CHANGE_TREE_EXPAND;
	if (collapsing || !d->isComplete()) {
		changeDir(d, TRUE);
	}
}

bool DirectoryListingFrame::isBold(const DirectoryListing::Directory* d) const {
	return d->getAdls();
}

void DirectoryListingFrame::createRoot() {
//	const auto icon = getIconIndex(dl->getRoot());
	const auto icon = ResourceLoader::DIR_NORMAL;
	treeRoot = ctrlTree.InsertItem(TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM, Text::toT(dl->getNick(true)).c_str(), icon, icon, 0, 0, (LPARAM)dl->getRoot(), NULL, NULL);
	history.push_back(dl->getPath(dl->getRoot()));
	dcassert(treeRoot); 
}

void DirectoryListingFrame::refreshTree(const tstring& root, bool reloadList, bool changeDir) {
	ctrlTree.SetRedraw(FALSE);
	if (reloadList) {
		ctrlTree.DeleteAllItems();
		ctrlFiles.list.DeleteAllItems();
		if (dl->getIsOwnList()) {
			history.clear();
			historyIndex = 1;
		}

		createRoot();
		updateHistoryCombo();
	}

	bool initialChange = !ctrlTree.hasChildren(treeRoot);
	if (initialChange && !dl->getRoot()->directories.empty()) {
		ctrlTree.setHasChildren(treeRoot, true);
	}

	auto oldSel = ctrlTree.GetSelectedItem();
	HTREEITEM ht = reloadList ? treeRoot : ctrlTree.findItem(treeRoot, Text::fromT(root));
	if(ht == NULL) {
		ht = treeRoot;
	}

	DirectoryListing::Directory* d = (DirectoryListing::Directory*)ctrlTree.GetItemData(ht);
	d->setLoading(false);

	ctrlTree.SelectItem(NULL);

	bool isExpanded = ctrlTree.IsExpanded(ht);

	//make sure that all subitems are removed
	ctrlTree.Expand(ht, TVE_COLLAPSE | TVE_COLLAPSERESET);

	//d->sortDirs();

	if (initialChange || isExpanded || (changeDir && (changeType == CHANGE_TREE_EXPAND || changeType == CHANGE_TREE_DOUBLE)))
		ctrlTree.Expand(ht);


	if (changeDir) {
		if (changeType == CHANGE_TREE_EXPAND)
			ctrlTree.SelectItem(oldSel);
		else
			selectItem(root);
	} else {
		auto loadedDir = Text::fromT(root);

		if (curPath == Util::getParentDir(loadedDir, true)) {
			//set the dir complete
			int j = ctrlFiles.list.GetItemCount();        
			for(int i = 0; i < j; i++) {
				const ItemInfo* ii = ctrlFiles.list.getItemData(i);
				if (ii->type == ii->DIRECTORY && ii->dir->getPath() == loadedDir) {
					ctrlFiles.list.SetItem(i, 0, LVIF_IMAGE, NULL, ResourceLoader::DIR_NORMAL, 0, 0, NULL);
					ctrlFiles.list.updateItem(i);
					updateStatus();
					break;
				}
			}
		}

		if (!AirUtil::isParentOrExact(loadedDir, curPath))
			updating = true; //prevent reloading the listview unless we are in the directory already (recursive partial lists with directory downloads from tree)

		if (AirUtil::isSub(curPath, loadedDir)) {
			//the old tree item isn't valid anymore
			oldSel = ctrlTree.findItem(treeRoot, curPath);
			if (!oldSel)
				oldSel = ht;
		}

		ctrlTree.SelectItem(oldSel);
		updating = false;
	}

	if (!dl->getIsOwnList() && SETTING(DUPES_IN_FILELIST))
		dl->checkShareDupes();
	ctrlTree.SetRedraw(TRUE);
}

LRESULT DirectoryListingFrame::onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
	changeType = CHANGE_LIST;
	statusDirty = true;
	return 0;
}

void DirectoryListingFrame::findSearchHit(bool newDir /*false*/) {
	auto& search = dl->curSearch;
	if (!search)
		return;

	bool found = false;

	// set the starting position for the search
	if (newDir) {
		searchPos = gotoPrev ? ctrlFiles.list.GetItemCount()-1 : 0;
	} else if (gotoPrev) {
		searchPos--;
	} else {
		searchPos++;
	}

	// Check file names in list pane
	while(searchPos < ctrlFiles.list.GetItemCount() && searchPos >= 0) {
		const ItemInfo* ii = ctrlFiles.list.getItemData(searchPos);
		if (search->root && ii->type == ItemInfo::FILE) {
			if (*search->root == ii->file->getTTH()) {
				found = true;
				break;
			}
		} else if(ii->type == ItemInfo::FILE && search->itemType != AdcSearch::TYPE_DIRECTORY) {
			if(search->matchesFileLower(Text::toLower(ii->file->getName()), ii->file->getSize(), ii->file->getRemoteDate())) {
				found = true;
				break;
			}
		} else if(ii->type == ItemInfo::DIRECTORY && search->itemType != AdcSearch::TYPE_FILE && search->matchesDirectory(ii->dir->getName())) {
			if (search->matchesSize(ii->dir->getTotalSize(false))) {
				found = true;
				break;
			}
		}

		if (gotoPrev)
			searchPos--;
		else
			searchPos++;
	}

	if (found) {
		// Remove prev. selection from file list
		if(ctrlFiles.list.GetSelectedCount() > 0)		{
			for(int i=0; i<ctrlFiles.list.GetItemCount(); i++)
				ctrlFiles.list.SetItemState(i, 0, LVIS_SELECTED);
		}

		// Highlight and focus the dir/file if possible
		ctrlFiles.list.SetFocus();
		ctrlFiles.list.EnsureVisible(searchPos, FALSE);
		ctrlFiles.list.SetItemState(searchPos, LVIS_SELECTED | LVIS_FOCUSED, (UINT)-1);
		updateStatus();
	} else {
		//move to next dir (if there are any)
		dcassert(!newDir);
		if (!dl->nextResult(gotoPrev)) {
			MessageBox(CTSTRING(NO_ADDITIONAL_MATCHES), CTSTRING(SEARCH_FOR_FILE));
		}
	}
}

LRESULT DirectoryListingFrame::onFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	onFind();
	return 0;
}

void DirectoryListingFrame::onFind() {
	searchPos = 0;

	// Prompt for substring to find
	DirectoryListingDlg dlg(dl);
	if(dlg.DoModal() != IDOK)
		return;

	gotoPrev = false;
	string path;
	if (dlg.useCurDir) {
		path = Util::toAdcFile(curPath);
	}

	dl->addSearchTask(dlg.searchStr, dlg.size, dlg.fileType, dlg.sizeMode, dlg.extList, path);
}

LRESULT DirectoryListingFrame::onNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	gotoPrev = false;
	findSearchHit();
	return 0;
}

LRESULT DirectoryListingFrame::onPrev(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	gotoPrev = true;
	findSearchHit();
	return 0;
}

size_t DirectoryListingFrame::getTotalListItemCount() const {
	auto d = dl->findDirectory(curPath);
	if (d) {
		return d->files.size() + d->directories.size();
	}

	dcassert(0);
	return 0;
}

void DirectoryListingFrame::updateStatus() {
	if(!updating && ctrlStatus.IsWindow()) {
		int selectedCount = ctrlFiles.list.GetSelectedCount();
		int totalCount = 0;
		int displayCount = 0;
		int64_t total = 0;
		auto d = dl->findDirectory(curPath);
		if (selectedCount == 0) {
			displayCount = ctrlFiles.list.GetItemCount();
			if (d) {
				totalCount = d->files.size() + d->directories.size();
				total = d->getTotalSize(d != dl->getRoot());
			}
		} else {
			total = ctrlFiles.list.forEachSelectedT(ItemInfo::TotalSize()).total;
		}

		tstring tmp = TSTRING(ITEMS) + _T(": ");

		if (selectedCount != 0) {
			tmp += Util::toStringW(selectedCount);
		} else if (displayCount != totalCount) {
			tmp += Util::toStringW(displayCount) + _T("/") + Util::toStringW(totalCount);
		} else {
			tmp += Util::toStringW(displayCount);
		}
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

		tmp = TSTRING_F(UPDATED_ON_X, Text::toT(Util::formatTime("%c", d ? d->getUpdateDate() : 0)));
		w = WinUtil::getTextWidth(tmp, ctrlStatus.m_hWnd);
		if (statusSizes[STATUS_UPDATED] < w) {
			statusSizes[STATUS_UPDATED] = w;
			u = true;
		}
		ctrlStatus.SetText(STATUS_UPDATED, tmp.c_str());

		if (selCombo.GetStyle() & WS_VISIBLE) {
			tmp = getComboDesc();
		} else {
			tmp = TSTRING(USER_OFFLINE);
		}

		ctrlStatus.SetText(STATUS_HUB, tmp.c_str());
		if(u)
			UpdateLayout(TRUE);
	}
}

tstring DirectoryListingFrame::getComboDesc() {
	return (dl->getIsOwnList() ? TSTRING(SHARE_PROFILE) : TSTRING(BROWSE_VIA));
}

void DirectoryListingFrame::DisableWindow(bool redraw){
	disabled = true;

	if (redraw) {
		ctrlFiles.list.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
		ctrlTree.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}

	//can't use EnableWindow as that message seems the get queued for the list view...
	ctrlFiles.list.SetWindowLongPtr(GWL_STYLE, ctrlFiles.list.GetWindowLongPtr(GWL_STYLE) | WS_DISABLED);
	ctrlTree.SetWindowLongPtr(GWL_STYLE, ctrlTree.GetWindowLongPtr(GWL_STYLE) | WS_DISABLED);
}

void DirectoryListingFrame::EnableWindow(bool redraw){
	disabled = false;
	ctrlFiles.list.SetWindowLongPtr(GWL_STYLE, ctrlFiles.list.GetWindowLongPtr(GWL_STYLE) & ~ WS_DISABLED);
	ctrlTree.SetWindowLongPtr(GWL_STYLE, ctrlTree.GetWindowLongPtr(GWL_STYLE) & ~ WS_DISABLED);

	if (redraw) {
		ctrlFiles.list.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
		ctrlTree.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

void DirectoryListingFrame::initStatus() {
	size_t totalFiles = 0;
	int64_t totalSize = 0;
	if (dl->getPartialList() && !dl->getHintedUser().user->isNMDC()) {
		if (!dl->getIsOwnList()) {
			auto si = ClientManager::getInstance()->getShareInfo(dl->getHintedUser());
			totalSize = si.first;
			totalFiles = si.second;
		} else {
			ShareManager::getInstance()->getProfileInfo(Util::toInt(dl->getFileName()), totalSize, totalFiles);
		}
	} else {
		totalSize = dl->getTotalListSize();
		totalFiles = dl->getTotalFileCount();
	}

	tstring tmp = TSTRING_F(TOTAL_SIZE, Util::formatBytesW(totalSize));
	statusSizes[STATUS_TOTAL_SIZE] = WinUtil::getTextWidth(tmp, m_hWnd);
	ctrlStatus.SetText(STATUS_TOTAL_SIZE, tmp.c_str());

	tmp = TSTRING_F(TOTAL_FILES, totalFiles);
	statusSizes[STATUS_TOTAL_FILES] = WinUtil::getTextWidth(tmp, m_hWnd);
	ctrlStatus.SetText(STATUS_TOTAL_FILES, tmp.c_str());

	UpdateLayout(FALSE);
}

LRESULT DirectoryListingFrame::onSelChangedDirectories(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	if (updating)
		return 0;

	NMTREEVIEW* p = (NMTREEVIEW*) pnmh;

	if(p->itemNew.state & TVIS_SELECTED) {
		DirectoryListing::Directory* d = (DirectoryListing::Directory*)p->itemNew.lParam;
		//check if we really selected a new item.
		if(curPath != dl->getPath(d)) {
			if (changeType != CHANGE_HISTORY)
				addHistory(dl->getPath(d));

			updateHistoryCombo();
		}

		curPath = dl->getPath(d);
		changeDir(d, TRUE);
	}
	return 0;
}

void DirectoryListingFrame::updateHistoryCombo() {
	ctrlPath.ResetContent();
	for (auto& i : history) {
		ctrlPath.AddString(Text::toT(Util::toAdcFile(i)).c_str());
	}
	ctrlPath.SetCurSel(historyIndex - 1);
}

LRESULT DirectoryListingFrame::onClickTree(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled) {
	changeType = CHANGE_TREE_SINGLE;
	bHandled = FALSE;
	return 0;
}

void DirectoryListingFrame::addHistory(const string& name) {
	history.erase(history.begin() + historyIndex, history.end());
	while(history.size() > 25)
		history.pop_front();
	history.push_back(name);
	historyIndex = history.size();
}

void DirectoryListingFrame::on(DirectoryListingListener::RemovedQueue, const string& aDir) noexcept {
	callAsync([=] {
		HTREEITEM ht = ctrlTree.findItem(treeRoot, aDir);
		if (ht) {
			ctrlTree.updateItemImage(ht);
		}

		updateStatus(Text::toT(aDir) + _T(": ") + TSTRING(TARGET_REMOVED));
	});
}

void DirectoryListingFrame::filterList() {
	updating = true;
	ctrlFiles.list.SetRedraw(FALSE);

	/*TCHAR *buf = new TCHAR[ctrlFilter.GetWindowTextLength()+1];
	ctrlFilter.GetWindowText(buf, ctrlFilter.GetWindowTextLength()+1);
	string newFilter = AirUtil::regexEscape(Text::fromT(buf), false);
	delete[] buf;

	if (filter == newFilter)
		return;

	boost::regex regNew;
	try {
		regNew.assign(newFilter, boost::regex_constants::icase);
	} catch (...) {
		//ctrlStatus.SetText(STATUS_TEXT, Text::toT("Invalid pattern");
		return;
	}

	auto curDir = dl->findDirectory(curPath);
	if (!curDir) {
		return;
	}

	auto addItems = [this, &regNew, curDir] () -> void {
		//try to speed this up with large listings by comparing with the old filter
		try {
			boost::regex regOld(filter, boost::regex_constants::icase);
			for(auto d: curDir->directories) {
				string s = d->getName();
				if(boost::regex_search(s.begin(), s.end(), regNew) && !boost::regex_search(s.begin(), s.end(), regOld)) {
					ctrlFiles.list.insertItem(ctrlFiles.list.GetItemCount(), new ItemInfo(d), getIconIndex(d));
				}
			}

			for(auto f: curDir->files) {
				string s = f->getName();
				if(boost::regex_search(s.begin(), s.end(), regNew) && !boost::regex_search(s.begin(), s.end(), regOld)) {
					ctrlFiles.list.insertItem(ctrlFiles.list.GetItemCount(), new ItemInfo(f), ResourceLoader::getIconIndex(Text::toT(f->getName())));
				}
			}
		} catch (...) { }
	};

	auto removeItems = [this, &regNew] () -> void {
		try {
			for(int i=0; i<ctrlFiles.list.GetItemCount();) {
				const ItemInfo* ii = ctrlFiles.list.getItemData(i);
				string s = ii->type == 0 ? ii->file->getName() : ii->dir->getName();
				if(!boost::regex_search(s.begin(), s.end(), regNew)) {
					delete ctrlFiles.list.getItemData(i);
					ctrlFiles.list.DeleteItem(i);
				} else {
					i++;
				}
			}
		} catch (...) { }
	};

	if (AirUtil::isSub(filter, newFilter)) {
		//we are adding chars
		addItems();
	} else if (AirUtil::isSub(newFilter, filter)) {
		//we are removing chars
		removeItems();
	} else {
		//remove items that don't match
		removeItems();
		//add new matches
		addItems();
	}

	filter = newFilter;*/
	insertItems(nullptr);

	ctrlFiles.list.resort();
	ctrlFiles.list.SetRedraw(TRUE);
	updating = false;
	updateStatus();
}

void DirectoryListingFrame::insertItems(const optional<string>& selectedName) {
	int curPos = 0;
	int selectedPos = -1;

	auto curDir = dl->findDirectory(curPath);
	auto i = curDir->directories.begin();

	auto filterPrep = ctrlFiles.filter.prepare();
	auto filterInfoDir = [this, &i](int column) { return (*i)->getName(); };

	ctrlFiles.list.DeleteAllItems();
	for (; i != curDir->directories.end(); ++i) {
		if (ctrlFiles.checkDupe((*i)->getDupe()) && (ctrlFiles.filter.empty() || ctrlFiles.filter.match(filterPrep, filterInfoDir))) {
			if (selectedName && compare(*selectedName, (*i)->getName()) == 0) {
				selectedPos = curPos;
			}

			ctrlFiles.list.insertItem(curPos++, new ItemInfo(*i), getIconIndex(*i));
		}
	}

	auto f = curDir->files.begin();
	auto filterInfoFile = [this, &f](int column) { return (*f)->getName(); };

	for (; f != curDir->files.end(); ++f) {
		if (ctrlFiles.checkDupe((*f)->getDupe()) && (ctrlFiles.filter.empty() || ctrlFiles.filter.match(filterPrep, filterInfoFile))) {
			ctrlFiles.list.insertItem(curPos++, new ItemInfo(*f), ResourceLoader::getIconIndex(Text::toT((*f)->getName())));
		}
	}

	if (selectedPos != -1) {
		ctrlFiles.list.SelectItem(selectedPos);
	}
}

void DirectoryListingFrame::updateItems(const DirectoryListing::Directory* d, BOOL enableRedraw) {
	ctrlFiles.list.SetRedraw(FALSE);
	updating = true;
	clearList();

	optional<string> selectedName;
	if (changeType == CHANGE_HISTORY) {
		if (historyIndex < history.size() && (!d->getParent() || Util::getParentDir(history[historyIndex]) == d->getPath())) {
			selectedName = Util::getLastDir(history[historyIndex]);
		}

		changeType = CHANGE_LIST; //reset
	}

	insertItems(selectedName);

	ctrlFiles.list.resort();
	ctrlFiles.list.SetRedraw(enableRedraw);
	updating = false;
	updateStatus();
}

void DirectoryListingFrame::changeDir(DirectoryListing::Directory* d, BOOL enableRedraw, ReloadMode aReload /*RELOAD_NONE*/) {
	if (aReload == RELOAD_NONE)
		updateItems(d, enableRedraw);


	if(!d->isComplete() || aReload != RELOAD_NONE) {
		if (dl->getIsOwnList()) {
			dl->addPartialListTask(Util::emptyString, dl->getPath(d), aReload == RELOAD_ALL);
		} else if(dl->getUser()->isOnline()) {
			try {
				QueueManager::getInstance()->addList(dl->getHintedUser(), QueueItem::FLAG_PARTIAL_LIST | QueueItem::FLAG_CLIENT_VIEW, dl->getPath(d));
				d->setLoading(true);
				ctrlTree.updateItemImage(d);
				//ctrlStatus.SetText(STATUS_TEXT, CTSTRING(DOWNLOADING_LIST));
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
		changeType = CHANGE_HISTORY;
		historyIndex--;
		selectItem(Text::toT(history[historyIndex-1]));
	}
}

void DirectoryListingFrame::forward() {
	if(history.size() > 1 && historyIndex < history.size()) {
		changeType = CHANGE_HISTORY;
		historyIndex++;
		selectItem(Text::toT(history[historyIndex-1]));
	}
}

LRESULT DirectoryListingFrame::onDoubleClickDirs(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled) {
	changeType = CHANGE_TREE_DOUBLE;
	bHandled = FALSE;
	return 0;
}

LRESULT DirectoryListingFrame::onDoubleClickFiles(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
	onListItemAction();
	return 0;
}

void DirectoryListingFrame::onListItemAction() {
	HTREEITEM t = ctrlTree.GetSelectedItem();
	if(t) {
		if (ctrlFiles.list.GetSelectedCount() == 1) {
			const ItemInfo* ii = ctrlFiles.list.getItemData(ctrlFiles.list.GetNextItem(-1, LVNI_SELECTED));
			if(ii->type == ItemInfo::FILE) {
				onDownload(SETTING(DOWNLOAD_DIRECTORY), false, false, WinUtil::isShift() ? QueueItem::HIGHEST : QueueItem::DEFAULT);
			} else {
				changeType = CHANGE_LIST;
				HTREEITEM ht = ctrlTree.findItem(t, ii->dir->getName() + "\\");
				if (ht) {
					ctrlTree.SelectItem(ht);
				}
			}
		} else {
			onDownload(SETTING(DOWNLOAD_DIRECTORY), false, false, WinUtil::isShift() ? QueueItem::HIGHEST : QueueItem::DEFAULT);
		}
	}
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
		onListItemAction();
	}
	return 0;
}

LRESULT DirectoryListingFrame::onKeyDownDirs(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMTVKEYDOWN* kd = (NMTVKEYDOWN*) pnmh;
	if(kd->wVKey == VK_TAB) {
		onTab();
	} else if(kd->wVKey == VK_DOWN || kd->wVKey == VK_DOWN) {
		changeType = CHANGE_TREE_SINGLE;
	}
	return 0;
}

void DirectoryListingFrame::onTab() {
	HWND focus = ::GetFocus();
	if(focus == ctrlTree.m_hWnd) {
		ctrlFiles.list.SetFocus();
	} else if(focus == ctrlFiles.list.m_hWnd) {
		ctrlTree.SetFocus();
	}
}

LRESULT DirectoryListingFrame::onSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlFiles.list.SetFocus();
	return 0;
}

LRESULT DirectoryListingFrame::onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	changeWindowState(false);
	dl->addQueueMatchTask();
	return 0;
}

LRESULT DirectoryListingFrame::onListDiff(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMTOOLBAR tb = (LPNMTOOLBAR)pnmh;

	OMenu sMenu;
	sMenu.CreatePopupMenu();
	sMenu.InsertSeparatorFirst(CTSTRING(FILELIST_ON_DISK));

	sMenu.appendItem(TSTRING(BROWSE), [this] { 	
		tstring file;
		if (WinUtil::browseFile(file, m_hWnd, false, Text::toT(Util::getListPath()), TSTRING(OPEN_FILE_LIST), _T("File Lists\0*.xml.bz2\0All Files\0*.*\0"))) {
			changeWindowState(false);
			ctrlStatus.SetText(0, CTSTRING(MATCHING_FILE_LIST));
			dl->addListDiffTask(Text::fromT(file), false);
		}; 
	});

	sMenu.InsertSeparatorLast(CTSTRING(OWN_FILELIST));
	auto profiles = ShareManager::getInstance()->getProfiles();
	for (auto& sp: profiles) {
		if (sp->getToken() != SP_HIDDEN) {
			string profile = Util::toString(sp->getToken());
			sMenu.appendItem(Text::toT(sp->getDisplayName()), [this, profile] {
				changeWindowState(false);
				ctrlStatus.SetText(0, CTSTRING(MATCHING_FILE_LIST));
				dl->addListDiffTask(profile, true);
			}, !dl->getIsOwnList() || profile != dl->getFileName());
		}
	}

	POINT pt;
	pt.x = tb->rcButton.left;
	pt.y = tb->rcButton.bottom;
	ctrlToolbar.ClientToScreen(&pt);
	
	sMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERPOSANIMATION, pt);
	return TBDDRET_DEFAULT;

}

LRESULT DirectoryListingFrame::onExitMenuLoop(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	//ctrlFiles.listDiff.SetState(false);
	return 0;
}

LRESULT DirectoryListingFrame::onMatchADL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (dl->getPartialList() && (dl->getIsOwnList() || MessageBox(CTSTRING(ADL_DL_FULL_LIST), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)) {
		if (!dl->getIsOwnList())
			ctrlStatus.SetText(0, CTSTRING(DOWNLOADING_LIST));

		dl->setMatchADL(true);
		convertToFull();
	} else {
		changeWindowState(false);
		ctrlStatus.SetText(0, CTSTRING(MATCHING_ADL));
		dl->addMatchADLTask();
	}
	
	return 0;
}

LRESULT DirectoryListingFrame::onChar(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if((GetKeyState(VkKeyScan('F') & 0xFF) & 0xFF00) > 0 && WinUtil::isCtrl()){
		onFind();
		return 1;
	}

	if ((GetKeyState(VkKeyScan('C') & 0xFF) & 0xFF00) > 0 && WinUtil::isCtrl()){
		HWND focus = ::GetFocus();
		if (focus == ctrlTree.m_hWnd) {
			onCopyDir();
		} else {
			BOOL tmp;
			onCopy(0, IDC_COPY_FILENAME, 0, tmp);
		}
		return 1;
	}
		
	bHandled = FALSE;
	return 0;
}

void DirectoryListingFrame::selectItem(const tstring& name) {
	HTREEITEM ht = ctrlTree.findItem(treeRoot, Text::fromT(name));
	if(ht != NULL) {
		if (changeType == CHANGE_LIST)
			ctrlTree.EnsureVisible(ht);
		ctrlTree.SelectItem(ht);
	} else {
		dcassert(0);
	}
}

LRESULT DirectoryListingFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlFiles.list && ctrlFiles.list.GetSelectedCount() > 0) {
		changeType = CHANGE_LIST;
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlFiles.list, pt);
		}
		const ItemInfo* ii = ctrlFiles.list.getItemData(ctrlFiles.list.GetNextItem(-1, LVNI_SELECTED));

		CShellContextMenu shellMenu;
		OMenu& fileMenu(*shellMenu.GetMenu());

		if(SETTING(SHOW_SHELL_MENU) && dl->getIsOwnList() && (ctrlFiles.list.GetSelectedCount() == 1)) {
			try {
				StringList localPaths;
				ii->type == ItemInfo::FILE ? dl->getLocalPaths(ii->file, localPaths) : dl->getLocalPaths(ii->dir, localPaths);
				if (localPaths.size() == 1 && GetFileAttributes(Text::toT(localPaths.front()).c_str()) != 0xFFFFFFFF) {
					// Check that the file still exists
					shellMenu.SetPath(Text::toT(localPaths.front()).c_str());
				}
			} catch(...) {
			}
		}

		int i = -1;
		bool allComplete=true, hasFiles=false;
		while( (i = ctrlFiles.list.GetNextItem(i, LVNI_SELECTED)) != -1) {
			const ItemInfo* ii = ctrlFiles.list.getItemData(i);
			if (ii->type == ItemInfo::DIRECTORY && !ii->dir->isComplete() && ii->dir->getPartialSize() == 0) {
				allComplete = false;
			} else if (ii->type == ItemInfo::FILE) {
				hasFiles = true;
			}
		}

		if (!dl->getIsOwnList()) {
			// download menu
			optional<TTHValue> tth;
			optional<string> path;

			if (ii->type == ItemInfo::FILE) {
				if (ctrlFiles.list.GetSelectedCount() == 1)
					tth = ii->file->getTTH();

				path = ii->file->getPath();
			}
			else if (ctrlFiles.list.GetSelectedCount() == 1) {
				path = ii->dir->getPath();
			}

			appendDownloadMenu(fileMenu, DownloadBaseHandler::TYPE_PRIMARY, !allComplete, tth, path);
		}

		if (hasFiles)
			fileMenu.appendItem(TSTRING(VIEW_AS_TEXT), [this] { onViewAsText(); });

		if (hasFiles || !dl->getIsOwnList())
			fileMenu.appendSeparator();
		
		if (ctrlFiles.list.GetSelectedCount() == 1 && (dl->getIsOwnList() || (ii->type == ItemInfo::DIRECTORY && ii->dir->getDupe() != DUPE_NONE))) {
			fileMenu.appendItem(TSTRING(OPEN_FOLDER), [this] { onOpenDupeDir(false); });
			fileMenu.appendSeparator();
		} else if(ctrlFiles.list.GetSelectedCount() == 1 && !dl->getIsOwnList() && ii->type == ItemInfo::FILE && (ii->file->getDupe() == SHARE_DUPE || ii->file->getDupe() == FINISHED_DUPE)) {
			fileMenu.appendItem(TSTRING(OPEN), [this] { onOpen(); });
			fileMenu.appendItem(TSTRING(OPEN_FOLDER), [this] { onOpenDupeDir(false); });
			fileMenu.appendSeparator();
		} else if (hasFiles) {
			fileMenu.appendItem(TSTRING(OPEN), [this] { onOpen(); });
		}

		if (dl->getIsOwnList() && !hasFiles) {
			fileMenu.appendItem(TSTRING(REFRESH_IN_SHARE), [this] { onRefreshShare(false); });
		}

		if(dl->getIsOwnList() && !(ii->type == ItemInfo::DIRECTORY && ii->dir->getAdls())) {
			fileMenu.appendItem(TSTRING(SCAN_FOLDER_MISSING), [this] { onScanShare(false, false); });
			fileMenu.appendItem(TSTRING(RUN_SFV_CHECK), [this] { onScanShare(false, true); });
			fileMenu.appendSeparator();
		}

		if (!hasFiles)
			fileMenu.appendItem(TSTRING(VIEW_NFO), [this] { onViewNFO(); });

		// searching (client)
		if (ctrlFiles.list.GetSelectedCount() == 1 && ii->type == ItemInfo::FILE)
			fileMenu.appendItem(TSTRING(SEARCH_TTH), [this] { onSearchByTTH(); });

		fileMenu.appendItem(TSTRING(SEARCH_DIRECTORY), [this] { onSearchDir(false); });
		if (hasFiles)
			fileMenu.appendItem(TSTRING(SEARCH), [this] { onSearchFile(); });

		fileMenu.appendSeparator();

		// web shortcuts
		WinUtil::appendSearchMenu(fileMenu, [=](const WebShortcut* ws) {
			ctrlFiles.list.forEachSelectedT([=](const ItemInfo* ii) { 
				WinUtil::searchSite(ws, ii->type == ItemInfo::DIRECTORY ? ii->dir->getPath() : ii->file->getPath(), ii->type == ItemInfo::DIRECTORY); 
			});
		});

		fileMenu.appendSeparator();

		// copy menu
		OMenu copyMenu;
		copyMenu.CreatePopupMenu();
		copyMenu.InsertSeparatorFirst(CTSTRING(COPY));
		copyMenu.AppendMenu(MF_STRING, IDC_COPY_NICK, CTSTRING(NICK));
		copyMenu.AppendMenu(MF_STRING, IDC_COPY_FILENAME, CTSTRING(FILENAME));
		copyMenu.AppendMenu(MF_STRING, IDC_COPY_DIR, CTSTRING(DIRECTORY));
		copyMenu.AppendMenu(MF_STRING, IDC_COPY_SIZE, CTSTRING(SIZE));
		copyMenu.AppendMenu(MF_STRING, IDC_COPY_EXACT_SIZE, CTSTRING(EXACT_SIZE));
		copyMenu.AppendMenu(MF_STRING, IDC_COPY_TTH, CTSTRING(TTH_ROOT));
		copyMenu.AppendMenu(MF_STRING, IDC_COPY_LINK, CTSTRING(COPY_MAGNET_LINK));
		copyMenu.AppendMenu(MF_STRING, IDC_COPY_PATH, CTSTRING(PATH));
		fileMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)copyMenu, CTSTRING(COPY));


		if(ctrlFiles.list.GetSelectedCount() == 1 && ii->type == ItemInfo::FILE) {
			fileMenu.InsertSeparatorFirst(Text::toT(Util::getFileName(ii->file->getName())));
			if(ii->file->getAdls())	{
				fileMenu.appendItem(TSTRING(GO_TO_DIRECTORY), [this] { onGoToDirectory(); });
			}
		} else {
			if(ii->type == ItemInfo::DIRECTORY && ii->dir->getAdls() && ii->dir->getParent() != dl->getRoot()) {
				fileMenu.appendItem(TSTRING(GO_TO_DIRECTORY), [this] { onGoToDirectory(); });
			}
		}

		prepareMenu(fileMenu, UserCommand::CONTEXT_FILELIST, ClientManager::getInstance()->getHubUrls(dl->getHintedUser().user->getCID()));
		//fileMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
		shellMenu.ShowContextMenu(m_hWnd, pt);
		return TRUE; 
	} else if(reinterpret_cast<HWND>(wParam) == ctrlTree && ctrlTree.GetSelectedItem() != NULL) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlTree, pt);
		}

		ctrlTree.ScreenToClient(&pt);

		UINT a = 0;
		HTREEITEM ht = ctrlTree.HitTest(pt, &a);
		if (!ht)
			return 0;

		if(ht != ctrlTree.GetSelectedItem())
			ctrlTree.SelectItem(ht);
		ctrlTree.ClientToScreen(&pt);
		changeType = CHANGE_TREE_SINGLE;
		auto dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(ht);


		OMenu directoryMenu;
		directoryMenu.CreatePopupMenu();

		if (!dl->getIsOwnList()) {
			appendDownloadMenu(directoryMenu, DownloadBaseHandler::TYPE_SECONDARY, false, nullptr, dir->getPath());
			directoryMenu.appendSeparator();
		}

		WinUtil::appendSearchMenu(directoryMenu, curPath);
		directoryMenu.appendSeparator();

		directoryMenu.appendItem(TSTRING(COPY_DIRECTORY), [=] { onCopyDir(); });
		directoryMenu.appendItem(TSTRING(SEARCH), [=] { onSearchDir(true); });
		if (dl->getPartialList()) {
			directoryMenu.appendSeparator();
			directoryMenu.appendItem(TSTRING(RELOAD), [=] { onReloadPartial(true); });
		}
		
		if (dl->getIsOwnList() || (dir && dir->getDupe() != DUPE_NONE)) {
			directoryMenu.appendItem(TSTRING(REFRESH_IN_SHARE), [this] { onRefreshShare(true); });
			directoryMenu.appendItem(TSTRING(SCAN_FOLDER_MISSING), [this] { onScanShare(true, false); });
			directoryMenu.appendItem(TSTRING(OPEN_FOLDER), [=] { onOpenDupeDir(true); });
		}

		// Strange, windows doesn't change the selection on right-click... (!)
			
		directoryMenu.InsertSeparatorFirst(TSTRING(DIRECTORY));
		directoryMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
			
		return TRUE; 
	} 
	
	bHandled = FALSE;
	return FALSE; 
}

void DirectoryListingFrame::handleDownload(const string& aTarget, QueueItemBase::Priority prio, bool usingTree, TargetUtil::TargetType aTargetType, bool isSizeUnknown) {
	auto downloadDir = [&](const DirectoryListing::Directory* d) {
		DirectoryListingManager::getInstance()->addDirectoryDownload(d->getPath(), d->getName(), dl->getHintedUser(), aTarget, aTargetType, isSizeUnknown ? ASK_USER : NO_CHECK, WinUtil::isShift() ? QueueItem::HIGHEST : prio, false);
	};

	if (usingTree) {
		HTREEITEM t = ctrlTree.GetSelectedItem();
		auto dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
		downloadDir(dir);
	}
	else if (ctrlFiles.list.GetSelectedCount() >= 1) {
		int i = -1;
		while ((i = ctrlFiles.list.GetNextItem(i, LVNI_SELECTED)) != -1) {
			const ItemInfo* ii = ctrlFiles.list.getItemData(i);
			if (ii->type == ItemInfo::FILE) {
				WinUtil::addFileDownload(aTarget + (aTarget[aTarget.length() - 1] != PATH_SEPARATOR ? Util::emptyString : Text::fromT(ii->getText(COLUMN_FILENAME))), ii->file->getSize(), ii->file->getTTH(), dl->getHintedUser(), ii->file->getRemoteDate(),
					0, WinUtil::isShift() ? QueueItem::HIGHEST : prio);
			} else {
				downloadDir(ii->dir);
			}
		}
	}
}

void DirectoryListingFrame::onViewAsText() {
	int i = -1;
	while ((i = ctrlFiles.list.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const ItemInfo* ii = ctrlFiles.list.getItemData(i);
		try {
			if (ii->type == ItemInfo::FILE) {
				if (dl->getIsOwnList()) {
					StringList paths;
					dl->getLocalPaths(ii->file, paths);
					if (!paths.empty())
						TextFrame::openWindow(Text::toT(paths.front()), TextFrame::NORMAL);
				}
				else {
					dl->openFile(ii->file, true);
				}
			}
		}
		catch (const Exception& e) {
			ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
		}
	}
}

void DirectoryListingFrame::onSearchByTTH() {
	const ItemInfo* ii = ctrlFiles.list.getSelectedItem();
	if (ii != NULL && ii->type == ItemInfo::FILE) {
		WinUtil::searchHash(ii->file->getTTH(), ii->file->getName(), ii->file->getSize());
	}
	return;
}

void DirectoryListingFrame::onSearchDir(bool usingTree) {
	tstring dir;
	if (usingTree) {
		HTREEITEM t = ctrlTree.GetSelectedItem();
		if (t != NULL) {
			dir = Text::toT(((DirectoryListing::Directory*)ctrlTree.GetItemData(t))->getName());
		}
	} else {
		const ItemInfo* ii = ctrlFiles.list.getSelectedItem();
		if (ii->type == ItemInfo::FILE) {
			dir = Text::toT(Util::getReleaseDir(ii->file->getPath(), true));
		} else {
			dir = ii->getText(COLUMN_FILENAME);
		}
	}

	WinUtil::searchAny(dir);
}

void DirectoryListingFrame::onGoToDirectory() {
	if (ctrlFiles.list.GetSelectedCount() != 1)
		return;

	tstring fullPath;
	const ItemInfo* ii = ctrlFiles.list.getItemData(ctrlFiles.list.GetNextItem(-1, LVNI_SELECTED));
	if (ii->type == ItemInfo::FILE) {
		if (!ii->file->getAdls())
			return;
		DirectoryListing::Directory* pd = ii->file->getParent();
		while (pd && pd != dl->getRoot()) {
			fullPath = Text::toT(pd->getName()) + _T("\\") + fullPath;
			pd = pd->getParent();
		}
	}
	else if (ii->type == ItemInfo::DIRECTORY)	{
		if (!(ii->dir->getAdls() && ii->dir->getParent() != dl->getRoot()))
			return;
		fullPath = Text::toT(((DirectoryListing::AdlDirectory*)ii->dir)->getFullPath());
	}

	if (!fullPath.empty())
		selectItem(fullPath);
}

void DirectoryListingFrame::onViewNFO() {
	if (ctrlFiles.list.GetSelectedCount() >= 1) {
		int sel = -1;
		while ((sel = ctrlFiles.list.GetNextItem(sel, LVNI_SELECTED)) != -1) {
			const ItemInfo* ii = ctrlFiles.list.getItemData(sel);
			if (ii->type == ItemInfo::DIRECTORY) {
				if (dl->getIsOwnList()) {
					try {
						SearchResultList results;
						auto s = unique_ptr<AdcSearch>(AdcSearch::getSearch(Util::emptyString, Util::emptyString, 0, SearchManager::TYPE_ANY, SearchManager::SIZE_DONTCARE, { ".nfo" }, AdcSearch::MATCH_NAME, false));
						ShareManager::getInstance()->search(results, *s.get(), 10, Util::toInt(dl->getFileName()), ClientManager::getInstance()->getMyCID(), Util::toAdcFile(ii->dir->getPath()));

						if (!results.empty()) {
							auto path = Text::toT(ShareManager::getInstance()->getRealPath(results.front()->getTTH()));
							if (!path.empty())
								TextFrame::openWindow(path, TextFrame::NORMAL);
						}
						else {
							updateStatus(Text::toT(ii->dir->getName()) + _T(": ") + TSTRING(NO_NFO_FOUND));
						}
					}
					catch (...) {}
				}
				else if (!ii->dir->isComplete() || ii->dir->findIncomplete()) {
					try {
						QueueManager::getInstance()->addList(dl->getHintedUser(), QueueItem::FLAG_VIEW_NFO | QueueItem::FLAG_PARTIAL_LIST | QueueItem::FLAG_RECURSIVE_LIST, ii->dir->getPath());
					}
					catch (const Exception&) {}
				}
				else if (!dl->findNfo(ii->dir->getPath())) {
					//ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
				}
			}
		}
	}
}

void DirectoryListingFrame::onOpen() {
	if (ctrlFiles.list.GetSelectedCount() >= 1) {
		int sel = -1;
		while ((sel = ctrlFiles.list.GetNextItem(sel, LVNI_SELECTED)) != -1) {
			const ItemInfo* ii = ctrlFiles.list.getItemData(sel);
			if (ii->type == ItemInfo::FILE) {
				try {
					dl->openFile(ii->file, false);
				}
				catch (const Exception&) {}
			}
		}
	}
}

void DirectoryListingFrame::onOpenDupeDir(bool usingTree) {
	if (usingTree) {
		HTREEITEM t = ctrlTree.GetSelectedItem();
		if (t != NULL) {
			openDupe((DirectoryListing::Directory*)ctrlTree.GetItemData(t));
		}
	} else {
		const ItemInfo* ii = ctrlFiles.list.getSelectedItem();
		if (ii->type == ItemInfo::FILE) {
			openDupe(ii->file->getParent());
		} else {
			openDupe(ii->dir);
		}
	}
}

void DirectoryListingFrame::openDupe(const DirectoryListing::Directory* d) {
	try {
		tstring path;
		if (dl->getIsOwnList()) {
			StringList localPaths;
			dl->getLocalPaths(d, localPaths);
			if (!localPaths.empty()) {
				path = Text::toT(localPaths.front());
			}
		}
		else {
			path = Text::toT(AirUtil::getDirDupePath(d->getDupe(), d->getPath()));
		}

		if (!path.empty()) {
			WinUtil::openFolder(path);
		}
	}
	catch (const ShareException& e) {
		updateStatus(Text::toT(e.getError()));
	}
}

void DirectoryListingFrame::openDupe(const DirectoryListing::File* f, bool openDir) {
	try {
		tstring path;
		if (dl->getIsOwnList()) {
			StringList localPaths;
			dl->getLocalPaths(f, localPaths);
			if (!localPaths.empty()) {
				path = Text::toT(localPaths.front());
			}
		}
		else {
			path = Text::toT(AirUtil::getDupePath(f->getDupe(), f->getTTH()));
		}

		if (!path.empty()) {
			if (!openDir) {
				WinUtil::openFile(path);
			}
			else {
				WinUtil::openFolder(path);
			}
		}
	}
	catch (const ShareException& e) {
		updateStatus(Text::toT(e.getError()));
	}
}

void DirectoryListingFrame::onReloadPartial(bool dirOnly) {
	HTREEITEM t = ctrlTree.GetSelectedItem();
	if (t != NULL) {
		DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
		changeDir(dir, TRUE, dirOnly ? RELOAD_DIR : RELOAD_ALL);
	}
}

void DirectoryListingFrame::onSearchFile() {

	tstring searchTerm;
	if (ctrlFiles.list.GetSelectedCount() == 1) {
		const ItemInfo* ii = ctrlFiles.list.getSelectedItem();
		searchTerm = ii->getText(COLUMN_FILENAME);
	}
	else {
		HTREEITEM t = ctrlTree.GetSelectedItem();
		if (t != NULL) {
			DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
			searchTerm = Text::toT((dir)->getName());
		}
	}

	WinUtil::searchAny(searchTerm);
}

void DirectoryListingFrame::onRefreshShare(bool usingTree) {
	StringList refresh;
	if (getLocalPaths(refresh, usingTree, true)) {
		ShareManager::getInstance()->addRefreshTask(ShareManager::REFRESH_DIRS, refresh, ShareManager::TYPE_MANUAL);
	}
}

void DirectoryListingFrame::onScanShare(bool usingTree, bool isSfvCheck) {
	ctrlStatus.SetText(0, CTSTRING(SEE_SYSLOG_FOR_RESULTS));

	StringList scanList;
	if (getLocalPaths(scanList, usingTree, false)) {
		//ctrlStatus.SetText(0, CTSTRING(SEE_SYSLOG_FOR_RESULTS));
		ShareScannerManager::getInstance()->scan(scanList, isSfvCheck);
	}
}

bool DirectoryListingFrame::getLocalPaths(StringList& paths_, bool usingTree, bool dirsOnly) {
	string error;
	
	if (usingTree) {
		HTREEITEM t = ctrlTree.GetSelectedItem();
		auto dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
		try {
			dl->getLocalPaths(dir, paths_);
		} catch(ShareException& e) { 
			error = e.getError();
		}
	} else if(ctrlFiles.list.GetSelectedCount() >= 1) {
		int sel = -1;
		while((sel = ctrlFiles.list.GetNextItem(sel, LVNI_SELECTED)) != -1) {
			const ItemInfo* ii = ctrlFiles.list.getItemData(sel);
			try {
				if (!dirsOnly && ii->type == ItemInfo::FILE) {
					dl->getLocalPaths(ii->file, paths_);
				} else if (ii->type == ItemInfo::DIRECTORY)  {
					dl->getLocalPaths(ii->dir, paths_);
				}
			} catch(ShareException& e) { 
				error = e.getError();
			}
		}
	}

	if (paths_.empty()) {
		ctrlStatus.SetText(0, Text::toT(error).c_str());
		return false;
	}

	return true;
}

void DirectoryListingFrame::runUserCommand(UserCommand& uc) {
	if (!WinUtil::getUCParams(m_hWnd, uc, ucLineParams))
		return;

	auto ucParams = ucLineParams;

	set<UserPtr> nicks;

	int sel = -1;
	while ((sel = ctrlFiles.list.GetNextItem(sel, LVNI_SELECTED)) != -1) {
		const ItemInfo* ii = ctrlFiles.list.getItemData(sel);
		if (uc.once()) {
			if (nicks.find(dl->getUser()) != nicks.end())
				continue;
			nicks.insert(dl->getUser());
		}
		if (!dl->getUser()->isOnline())
			return;
		//ucParams["fileTR"] = "NONE";
		if (ii->type == ItemInfo::FILE) {
			ucParams["type"] = [] { return "File"; };
			ucParams["fileFN"] = [this, ii] { return dl->getPath(ii->file) + ii->file->getName(); };
			ucParams["fileSI"] = [ii] { return Util::toString(ii->file->getSize()); };
			ucParams["fileSIshort"] = [ii] { return Util::formatBytes(ii->file->getSize()); };
			ucParams["fileTR"] = [ii] { return ii->file->getTTH().toBase32(); };
			ucParams["fileMN"] = [ii] { return WinUtil::makeMagnet(ii->file->getTTH(), ii->file->getName(), ii->file->getSize()); };
		}
		else {
			ucParams["type"] = [] { return "Directory"; };
			ucParams["fileFN"] = [this, ii] { return dl->getPath(ii->dir) + ii->dir->getName(); };
			ucParams["fileSI"] = [this, ii] { return Util::toString(ii->dir->getTotalSize(ii->dir != dl->getRoot())); };
			ucParams["fileSIshort"] = [ii] { return Util::formatBytes(ii->dir->getTotalSize(true)); };
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

LRESULT DirectoryListingFrame::onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
	tstring sCopy;
	if (ctrlFiles.list.GetSelectedCount() >= 1) {
		int xsel = ctrlFiles.list.GetNextItem(-1, LVNI_SELECTED);
		for (;;) {
			const ItemInfo* ii = ctrlFiles.list.getItemData(xsel);
			switch (wID) {
			case IDC_COPY_NICK:
				sCopy += Text::toT(dl->getNick(false));
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
				if (ii->type == ItemInfo::FILE) {
					sCopy += Text::toT(WinUtil::makeMagnet(ii->file->getTTH(), ii->file->getName(), ii->file->getSize()));
				}
				else if (ii->type == ItemInfo::DIRECTORY){
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
				if (ii->type == ItemInfo::FILE){
					sCopy += Text::toT(ii->file->getPath() + ii->file->getName());
				}
				else if (ii->type == ItemInfo::DIRECTORY){
					if (ii->dir->getAdls() && ii->dir->getParent() != dl->getRoot()) {
						sCopy += Text::toT(((DirectoryListing::AdlDirectory*)ii->dir)->getFullPath());
					}
					else {
						sCopy += Text::toT(ii->dir->getPath());
					}
				}
				break;
			case IDC_COPY_DIR:
				if (ii->type == ItemInfo::FILE){
					sCopy += Text::toT(Util::getReleaseDir(ii->file->getPath(), true));
				}
				else if (ii->type == ItemInfo::DIRECTORY){
					sCopy += ii->getText(COLUMN_FILENAME);
				}
				break;
			default:
				dcdebug("DIRECTORYLISTINGFRAME DON'T GO HERE\n");
				return 0;
			}
			xsel = ctrlFiles.list.GetNextItem(xsel, LVNI_SELECTED);
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

void DirectoryListingFrame::onCopyDir() {
	tstring sCopy;
	tstring directory;
	HTREEITEM t = ctrlTree.GetSelectedItem();
	DirectoryListing::Directory* dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
	sCopy = Text::toT((dir)->getName());
	if (!sCopy.empty()) {
		WinUtil::setClipboard(sCopy);
	}
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

bool DirectoryListingFrame::showDirDialog(string& fileName) {
	//could be improved
	if (ctrlFiles.list.GetSelectedCount() == 1) {
		const ItemInfo* ii = ctrlFiles.list.getItemData(ctrlFiles.list.GetNextItem(-1, LVNI_SELECTED));
		if (ii->type == ItemInfo::DIRECTORY) {
			return true;
		} else {
			fileName = ii->file->getName();
			return false;
		}
	}

	return true;
}

int64_t DirectoryListingFrame::getDownloadSize(bool isWhole) {
	int64_t size = 0;
	if (isWhole) {
		HTREEITEM t = ctrlTree.GetSelectedItem();
		auto dir = (DirectoryListing::Directory*)ctrlTree.GetItemData(t);
		size = dir->getTotalSize(false);
	} else {
		int i = -1;
		while( (i = ctrlFiles.list.GetNextItem(i, LVNI_SELECTED)) != -1) {
			const ItemInfo* ii = ctrlFiles.list.getItemData(i);
			if (ii->type == ItemInfo::DIRECTORY) {
				size += ii->dir->getTotalSize(false);
			} else {
				size += ii->file->getSize();
			}
		}
	}
	return size;
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
		sr.bottom -= 1;

		if (selCombo.GetStyle() & WS_VISIBLE) {
			int desclen = WinUtil::getTextWidth(getComboDesc(), ctrlStatus.m_hWnd);
			sr.left = w[STATUS_HUB - 1] + desclen;
			sr.right = w[STATUS_HUB];
			selCombo.MoveWindow(sr);
		} else {

		}

		//sr.left = w[STATUS_FILTER - 1] + bspace;
		//sr.right = w[STATUS_FILTER];
		//ctrlFilter.MoveWindow(sr);
	}

	//ctrlFiles.UpdateLayout();
	SetSplitterRect(&rect);
}

LRESULT DirectoryListingFrame::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	HDC hDC = (HDC)wParam;
	::SetBkColor(hDC, WinUtil::bgColor);
	::SetTextColor(hDC, WinUtil::textColor);
	return (LRESULT)WinUtil::bgBrush;
}

void DirectoryListingFrame::clearList() {
	int j = ctrlFiles.list.GetItemCount();        
	for(int i = 0; i < j; i++) {
		delete ctrlFiles.list.getItemData(i);
	}
	ctrlFiles.list.DeleteAllItems();
}

int DirectoryListingFrame::ItemInfo::compareItems(const ItemInfo* a, const ItemInfo* b, uint8_t col) {
	if(a->type == DIRECTORY) {
		if(b->type == DIRECTORY) {
			switch(col) {
				case COLUMN_EXACTSIZE: return compare(a->dir->getTotalSize(true), b->dir->getTotalSize(true));
				case COLUMN_SIZE: return compare(a->dir->getTotalSize(true), b->dir->getTotalSize(true));
				case COLUMN_DATE: return compare(a->dir->getRemoteDate(), b->dir->getRemoteDate());
				case COLUMN_FILENAME: return Util::DefaultSort(Text::toT(a->dir->getName()).c_str(), Text::toT(b->dir->getName()).c_str(), true);
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
			case COLUMN_DATE: return compare(a->file->getRemoteDate(), b->file->getRemoteDate());
			case COLUMN_FILENAME: return Util::DefaultSort(Text::toT(a->file->getName()).c_str(), Text::toT(b->file->getName()).c_str(), false);
			default: return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str(), false);
		}
	}
}

const tstring DirectoryListingFrame::ItemInfo::getText(uint8_t col) const {
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
		case COLUMN_EXACTSIZE: return type == DIRECTORY ? Util::formatExactSize(dir->getTotalSize(true)) : Util::formatExactSize(file->getSize());
		case COLUMN_SIZE: return  type == DIRECTORY ? Util::formatBytesW(dir->getTotalSize(true)) : Util::formatBytesW(file->getSize());
		case COLUMN_TTH: return (type == FILE && !SettingsManager::lanMode) ? Text::toT(file->getTTH().toBase32()) : Util::emptyStringT;
		case COLUMN_DATE: return Util::getDateTimeW(type == DIRECTORY ? dir->getRemoteDate() : file->getRemoteDate());
		default: return Util::emptyStringT;
	}
}

int DirectoryListingFrame::ItemInfo::getImageIndex() const {
	/*if(type == DIRECTORY)
		return DirectoryListingFrame::getIconIndex(dir);
	else*/
	return ResourceLoader::getIconIndex(getText(COLUMN_FILENAME));
}

int DirectoryListingFrame::getIconIndex(const DirectoryListing::Directory* d) const {
	if (d->getLoading())
		return ResourceLoader::DIR_LOADING;
	
	if (d->getType() == DirectoryListing::Directory::TYPE_NORMAL || dl->getIsOwnList())
		return ResourceLoader::DIR_NORMAL;

	return ResourceLoader::DIR_INCOMPLETE;
}

void DirectoryListingFrame::closeAll(){
	for (auto f : frames | map_values)
		f->PostMessage(WM_CLOSE, 0, 0);
}

LRESULT DirectoryListingFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	//tell the thread to abort and wait until we get a notification
	//that it's done.
	dl->setAbort(true);
	
	if(!closed) {
		SettingsManager::getInstance()->removeListener(this);
		if (!dl->getIsOwnList())
			ClientManager::getInstance()->removeListener(this);

		ctrlFiles.list.SetRedraw(FALSE);
		clearList();
		frames.erase(m_hWnd);

		ctrlFiles.list.saveHeaderOrder(SettingsManager::DIRECTORYLISTINGFRAME_ORDER, SettingsManager::DIRECTORYLISTINGFRAME_WIDTHS,
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
		return 0;
	}
}

LRESULT DirectoryListingFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 

	OMenu tabMenu;
	tabMenu.CreatePopupMenu();
	tstring nick = Text::toT(dl->getNick(true));
	tabMenu.InsertSeparatorFirst(nick);

	appendUserItems(tabMenu);
	if (!dl->getPartialList() || !dl->getIsOwnList()) {
		tabMenu.AppendMenu(MF_SEPARATOR);
		tabMenu.AppendMenu(MF_STRING, IDC_RELOAD, CTSTRING(RELOAD));
	}
	tabMenu.AppendMenu(MF_SEPARATOR);
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));

	tabMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt);
	return TRUE;
}

LRESULT DirectoryListingFrame::onReloadList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
	if (dl->getPartialList()) {
		onReloadPartial(false);
	} else {
		convertToFull();
	}
	return 0;
}

LRESULT DirectoryListingFrame::onReloadDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
	onReloadPartial(true);
	return 0;
}

void DirectoryListingFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	bool refresh = false;
	if(ctrlFiles.list.GetBkColor() != WinUtil::bgColor) {
		ctrlFiles.list.SetBkColor(WinUtil::bgColor);
		ctrlFiles.list.SetTextBkColor(WinUtil::bgColor);
		ctrlTree.SetBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlFiles.list.GetTextColor() != WinUtil::textColor) {
		ctrlFiles.list.SetTextColor(WinUtil::textColor);
		ctrlTree.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}

	// in case the profiles have changed
	if (dl->getIsOwnList()) {
		updateSelCombo();
	}
}

void DirectoryListingFrame::updateStatus(const tstring& aMsg) {
	ctrlStatus.SetText(0, aMsg.c_str());
}

LRESULT DirectoryListingFrame::onCustomDrawList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {

	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;
	switch(cd->nmcd.dwDrawStage) {

	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT: {
		if(disabled){
			cd->clrTextBk = WinUtil::bgColor;
			cd->clrText = GetSysColor(COLOR_3DDKSHADOW);
			return CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;
		}
		ItemInfo *ii = reinterpret_cast<ItemInfo*>(cd->nmcd.lItemlParam);
		//dupe colors have higher priority than highlights.
		if (SETTING(DUPES_IN_FILELIST) && !dl->getIsOwnList() && ii != NULL) {
			auto c = WinUtil::getDupeColors(ii->type == ItemInfo::FILE ? ii->file->getDupe() : ii->dir->getDupe());
			cd->clrText = c.first;
			cd->clrTextBk = c.second;
		}

		//has dupe color = no matching
		if( SETTING(USE_HIGHLIGHT) && !dl->getIsOwnList() && (ii->type == ItemInfo::DIRECTORY && ii->dir->getDupe() == DUPE_NONE) ) {
				
			ColorList *cList = HighlightManager::getInstance()->getList();
			for(ColorIter i = cList->begin(); i != cList->end(); ++i) {
			ColorSettings* cs = &(*i);
			if(cs->getContext() == HighlightManager::CONTEXT_FILELIST) {
				if(cs->usingRegexp()) {
					try {
						//have to have $Re:
						if(boost::regex_search(ii->dir->getName().begin(), ii->dir->getName().end(), cs->regexp)) {
							if(cs->getHasFgColor()) { cd->clrText = cs->getFgColor(); }
							if(cs->getHasBgColor()) { cd->clrTextBk = cs->getBgColor(); }
							break;
						}
					} catch(...) {}
				} else {
					if (Wildcard::patternMatch(Text::utf8ToAcp(ii->dir->getName()), Text::utf8ToAcp(Text::fromT(cs->getMatch())), '|')){
							if(cs->getHasFgColor()) { cd->clrText = cs->getFgColor(); }
							if(cs->getHasBgColor()) { cd->clrTextBk = cs->getBgColor(); }
							break;
						}
					}
				}
			}
		}
		
		return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
	}

    case CDDS_ITEMPOSTPAINT: {
       if (disabled && ctrlFiles.list.findColumn(cd->iSubItem) == COLUMN_FILENAME) {
           LVITEM rItem;
           int nItem = static_cast<int>(cd->nmcd.dwItemSpec);

           ZeroMemory (&rItem, sizeof(LVITEM) );
           rItem.mask  = LVIF_IMAGE | LVIF_STATE;
           rItem.iItem = nItem;
           ctrlFiles.list.GetItem ( &rItem );

           // Get the rect that holds the item's icon.
           CRect rcIcon;
           ctrlFiles.list.GetItemRect(nItem, &rcIcon, LVIR_ICON);

           // Draw the icon.
           ResourceLoader::getFileImages().DrawEx(rItem.iImage, cd->nmcd.hdc, rcIcon, WinUtil::bgColor, GetSysColor(COLOR_3DDKSHADOW), ILD_BLEND50);
           return CDRF_SKIPDEFAULT;
        }
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
		if(disabled){
			cd->clrTextBk = WinUtil::bgColor;
			cd->clrText = GetSysColor(COLOR_3DDKSHADOW);
			return CDRF_NEWFONT;
		}

		if (SETTING(DUPES_IN_FILELIST) && !dl->getIsOwnList() && !(cd->nmcd.uItemState & CDIS_SELECTED)) {
			DirectoryListing::Directory* dir = reinterpret_cast<DirectoryListing::Directory*>(cd->nmcd.lItemlParam);
			if(dir) {
				auto c = WinUtil::getDupeColors(dir->getDupe());
				cd->clrText = c.first;
				cd->clrTextBk = c.second;
			}
		}
		return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
	}

	default:
		return CDRF_DODEFAULT;
	}
}

LRESULT DirectoryListingFrame::onUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	up();
	return 0;
}

LRESULT DirectoryListingFrame::onForward(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	forward();
	return 0;
}

LRESULT DirectoryListingFrame::onBack(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	back();
	return 0;
}

LRESULT DirectoryListingFrame::onSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	selectItem(Text::toT(history[ctrlPath.GetCurSel()]));
	bHandled= FALSE;
	return 0;
}


void DirectoryListingFrame::onComboSelChanged(bool manual) {
	if (dl->getIsOwnList()) {
		auto token = ShareManager::getInstance()->getProfiles()[selCombo.GetCurSel()]->getToken();
		dl->setFileName(Util::toString(token));
		onReloadPartial(false);
		SettingsManager::getInstance()->set(SettingsManager::LAST_LIST_PROFILE, token);
	} else {
		auto& newHub = hubs[selCombo.GetCurSel()];
		if (manual) {
			auto p = boost::find_if(hubs, [this](const User::UserHubInfo& uhi) { return uhi.hubUrl == dl->getHubUrl(); });
			if (p != hubs.end()) {
				auto& oldHub = *p;
				auto diff = newHub.shared > 0 ? abs(static_cast<double>(oldHub.shared) / static_cast<double>(newHub.shared)) : oldHub.shared;
				if ((diff < 0.95 || diff > 1.05) && MessageBox(CTSTRING_F(LIST_SIZE_DIFF_NOTE, Text::toT(newHub.hubName) % Util::formatBytesW(newHub.shared) % Text::toT(oldHub.hubName) % Util::formatBytesW(oldHub.shared)),
					_T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {

						try {
							QueueManager::getInstance()->addList(HintedUser(dl->getUser(), newHub.hubUrl), (dl->getPartialList() ? QueueItem::FLAG_PARTIAL_LIST : 0) | QueueItem::FLAG_CLIENT_VIEW, Util::emptyString);
						}
						catch (...) { }
				}
			}
		}

		dl->setHintedUser(HintedUser(dl->getUser(), newHub.hubUrl));
	}
}

void DirectoryListingFrame::updateSelCombo() {
	if (dl->getIsOwnList()) {
		auto profiles = ShareManager::getInstance()->getProfiles();
		while (selCombo.GetCount()) {
			selCombo.DeleteString(0);
		}

		for (const auto& p : profiles) {
			if (p->getToken() != SP_HIDDEN) {
				auto idx = selCombo.AddString(Text::toT(p->getPlainName()).c_str());
				if (p->getToken() == Util::toInt(dl->getFileName())) {
					selCombo.SetCurSel(idx);
				}
			}
		}

		if (selCombo.GetCurSel() == -1) {
			// the profile was not found
			selCombo.SetCurSel(0);
			onComboSelChanged(false);
		}
	} else {

		const CID& cid = dl->getUser()->getCID();
		const string& hint = dl->getHubUrl();

		dcassert(!hint.empty() || !dl->getPartialList());

		//get the hub and online status
		auto hubsInfoNew = move(WinUtil::getHubNames(cid));
		if (!hubsInfoNew.second && !online) {
			//nothing to update... probably a delayed event
			return;
		}

		auto tmp = WinUtil::getHubNames(cid);

		auto oldSel = selCombo.GetStyle() & WS_VISIBLE ? selCombo.GetCurSel() : 0;
		StringPair oldHubPair;
		if (!hubs.empty())
			oldHubPair = { hubs[oldSel].hubName, hubs[oldSel].hubUrl }; // cache the old hub name

		hubs.clear();
		ClientManager::getInstance()->getUserInfoList(dl->getUser(), hubs);
		while (selCombo.GetCount()) {
			selCombo.DeleteString(0);
		}

		//General things
		if (hubsInfoNew.second) {
			//the user is online

			hubNames = WinUtil::getHubNames(dl->getHintedUser());
			nicks = WinUtil::getNicks(dl->getHintedUser());
			setDisconnected(false);

			if (!online) {
				//addStatusLine(TSTRING(USER_WENT_ONLINE) + _T(" [") + nicks + _T(" - ") + hubNames + _T("]"));
				//setIcon(userOnline);
			}
		} else {
			setDisconnected(true);
			//setIcon(userOffline);
			//addStatusLine(TSTRING(USER_WENT_OFFLINE) + _T(" [") + hubNames + _T("]"));
		}

		//ADC related changes
		if (hubsInfoNew.second && !dl->getUser()->isNMDC() && !hubs.empty()) {
			if (!(selCombo.GetStyle() & WS_VISIBLE)) {
				showSelCombo(true);
			}

			for (const auto& hub : hubs) {
				auto idx = selCombo.AddString(Text::toT((hub.hubName + " (" + Util::formatBytes(hub.shared) + ")")).c_str());
				if (hub.hubUrl == hint) {
					selCombo.SetCurSel(idx);
				}
			}

			if (selCombo.GetCurSel() == -1) {
				//the hub was not found
				selCombo.SetCurSel(0);
				onComboSelChanged(false);
				//if (!online) //the user came online but not in the previous hub
				//addStatusLine(CTSTRING_F(MESSAGES_SENT_THROUGH, Text::toT(hubs[ctrlHubSel.GetCurSel()].second)));
				//else
				//addStatusLine(CTSTRING_F(USER_OFFLINE_PM_CHANGE, Text::toT(oldHubPair.second) % Text::toT(hubs[0].second)));
			} else if (!oldHubPair.first.empty() && oldHubPair.first != hint) {
				//addStatusLine(CTSTRING_F(MESSAGES_SENT_THROUGH_REMOTE, Text::toT(hubs[ctrlHubSel.GetCurSel()].second)));
			}
			//else if (!ctrlClient.getClient()) {
			//	changeClient();
			//}
		} else {
			showSelCombo(false);
		}

		online = hubsInfoNew.second;
	}

	if (nicks.empty())
		nicks = Text::toT(dl->getNick(false));

	SetWindowText((nicks + (hubNames.empty() ? Util::emptyStringT : _T(" - ") + hubNames)).c_str());
}

void DirectoryListingFrame::showSelCombo(bool show) {
	selCombo.ShowWindow(show);
	selCombo.EnableWindow(show);

	updateStatus();
	//UpdateLayout();
}

LRESULT DirectoryListingFrame::onComboSelChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled) {
	//auto hp = hubs[selCombo.GetCurSel()];
	onComboSelChanged(true);

	updateSelCombo();
	//addStatusLine(CTSTRING_F(MESSAGES_SENT_THROUGH, Text::toT(hp.second)));

	bHandled = FALSE;
	return 0;
}

void DirectoryListingFrame::on(ClientManagerListener::UserConnected, const OnlineUser& aUser, bool /*wasOffline*/) noexcept {
	if (aUser.getUser() == dl->getUser())
		callAsync([this] { updateSelCombo(); });
}

void DirectoryListingFrame::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser, bool /*wentOffline*/) noexcept {
	if (aUser == dl->getUser())
		callAsync([this] { updateSelCombo(); });
}

void DirectoryListingFrame::on(ClientManagerListener::UserUpdated, const UserPtr& aUser) noexcept {
	if (aUser == dl->getUser())
		callAsync([this] { updateSelCombo(); });
}