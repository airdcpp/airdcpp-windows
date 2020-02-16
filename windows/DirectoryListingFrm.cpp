/* 
 * Copyright (C) 2001-2019 Jacek Sieka, arnetheduck on gmail point com
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

#include "LineDlg.h"
#include "ListFilter.h"
#include "MainFrm.h"
#include "ResourceLoader.h"
#include "ShellContextMenu.h"
#include "TextFrame.h"
#include "Wildcards.h"
#include "WinUtil.h"

#include <airdcpp/ADLSearch.h>
#include <airdcpp/ClientManager.h>
#include <airdcpp/DirectoryListingManager.h>
#include <airdcpp/File.h>
#include <airdcpp/QueueManager.h>
#include <airdcpp/StringTokenizer.h>
#include <airdcpp/User.h>
#include <airdcpp/ViewFileManager.h>

#include <airdcpp/modules/HighlightManager.h>
#include <airdcpp/modules/ShareScannerManager.h>

#include <boost/move/algorithm.hpp>
#include <boost/range/algorithm/copy.hpp>


DirectoryListingFrame::FrameMap DirectoryListingFrame::frames;
int DirectoryListingFrame::columnIndexes[] = { COLUMN_FILENAME, COLUMN_TYPE, COLUMN_EXACTSIZE, COLUMN_SIZE, COLUMN_TTH, COLUMN_DATE };
int DirectoryListingFrame::columnSizes[] = { 300, 60, 100, 100, 200, 130 };

static ResourceManager::Strings columnNames[] = { ResourceManager::FILE, ResourceManager::TYPE, ResourceManager::EXACT_SIZE, ResourceManager::SIZE, ResourceManager::TTH_ROOT, ResourceManager::DATE };
static SettingsManager::BoolSetting filterSettings [] = { SettingsManager::FILTER_FL_SHARED, SettingsManager::FILTER_FL_QUEUED, SettingsManager::FILTER_FL_INVERSED, SettingsManager::FILTER_FL_TOP, SettingsManager::FILTER_FL_PARTIAL_DUPES, SettingsManager::FILTER_FL_RESET_CHANGE };

static ColumnType columnTypes [] = { COLUMN_TEXT, COLUMN_TEXT, COLUMN_SIZE, COLUMN_SIZE, COLUMN_TEXT, COLUMN_TIME };

string DirectoryListingFrame::id = "FileList";


// Open own filelist
void DirectoryListingFrame::openWindow(ProfileToken aProfile, bool aUseADL, const string& aDir) noexcept {
	auto me = HintedUser(ClientManager::getInstance()->getMe(), Util::emptyString);

	auto list = DirectoryListingManager::getInstance()->findList(me);
	if (list) {
		activate(list);
		if (list->getShareProfile() != aProfile) {
			list->addShareProfileChangeTask(aProfile);
		}

		return;
	}

	DirectoryListingManager::getInstance()->openOwnList(aProfile, aUseADL, aDir);
}

// Open local file
void DirectoryListingFrame::openWindow(const HintedUser& aUser, const string& aFile, const string& aDir) noexcept {
	auto list = DirectoryListingManager::getInstance()->findList(aUser);
	if (list) {
		activate(list);
		return;
	}

	DirectoryListingManager::getInstance()->openLocalFileList(aUser, aFile, aDir);
}

// Open remote filelist
void DirectoryListingFrame::openWindow(const HintedUser& aUser, Flags::MaskType aFlags, const string& aInitialDir) noexcept {
	auto list = DirectoryListingManager::getInstance()->findList(aUser);
	if (list) {
		if (aUser.hint != list->getHubUrl()) {
			list->addHubUrlChangeTask(aUser.hint);
		}

		if (aInitialDir != ADC_ROOT_STR) {
			list->addDirectoryChangeTask(aInitialDir, false);
		}

		activate(list);
		return;
	}

	try {
		DirectoryListingManager::getInstance()->openRemoteFileList(aUser, QueueItem::FLAG_CLIENT_VIEW | aFlags, aInitialDir);
	} catch (const Exception& e) {
		MainFrame::getMainFrame()->ShowPopup(Text::toT(e.getError()), _T(""), NIIF_INFO | NIIF_NOSOUND, true);
	}
}

DirectoryListingFrame* DirectoryListingFrame::findFrame(const UserPtr& aUser) noexcept {
	auto f = find_if(frames | map_values, [&](const DirectoryListingFrame* h) { return aUser == h->dl->getUser(); }).base();
	if (f == frames.end()) {
		return nullptr;
	}

	return f->second;
}

void DirectoryListingFrame::activate(const DirectoryListingPtr& aList) noexcept {
	auto frame = findFrame(aList->getUser());
	if (frame) {
		frame->activate();
	}
}

void DirectoryListingFrame::openWindow(const DirectoryListingPtr& aList, const string& aDir, const string& aXML) {
	auto frame = findFrame(aList->getUser());
	if (!frame) {
		frame = new DirectoryListingFrame(aList);
		if (!frame->allowPopup()) {
			WinUtil::hiddenCreateEx(frame);
		} else {
			frame->CreateEx(WinUtil::mdiClient);
		}

		frames.emplace(frame->m_hWnd, frame);
	}

	if (aList->getPartialList()) {
		aList->addPartialListTask(aXML, aDir);
	} else {
		frame->ctrlStatus.SetText(0, CTSTRING(LOADING_FILE_LIST));
		aList->addFullListTask(aDir);
	}
}

bool DirectoryListingFrame::getWindowParams(HWND hWnd, StringMap &params) {
	auto f = frames.find(hWnd);
	if (f != frames.end()) {
		auto dl = f->second->dl;
		params["id"] = DirectoryListingFrame::id;
		params["CID"] = dl->getUser()->getCID().toBase32();
		params["url"] = dl->getHubUrl();
		params["dir"] = f->second->curPath;
		params["file"] = dl->getFileName();
		params["partial"] = Util::toString(dl->getPartialList());
		params["profileToken"] = Util::toString(dl->getIsOwnList() ? dl->getShareProfile() : 0);
		if (!dl->getPartialList())
			QueueManager::getInstance()->noDeleteFileList(dl->getFileName());

		FavoriteManager::getInstance()->addSavedUser(dl->getUser());
		return true;
	}
	return false;
}

bool DirectoryListingFrame::parseWindowParams(StringMap& params) {
	if (params["id"] == DirectoryListingFrame::id) {
		string cid = params["CID"];
		if (cid.empty())
			return false;
		UserPtr u = ClientManager::getInstance()->getUser(CID(cid));
		if (!u)
			return false;

		bool partial = Util::toBool(Util::toInt(params["partial"]));
		string dir = params["dir"];
		if (dir.empty() || dir.front() != ADC_SEPARATOR) {
			// Migrate NMDC paths from older versions
			dir = Util::toAdcFile(dir);
		}

		string file = params["file"];
		string url = params["url"];
		if (u == ClientManager::getInstance()->getMe()) {
			ProfileToken token = Util::toInt(params["profileToken"]);
			DirectoryListingManager::getInstance()->openOwnList(token, false, dir);
		} else if (!file.empty() || partial) {
					
			auto dl = DirectoryListingManager::getInstance()->openLocalFileList(HintedUser(u, url), file, dir, partial);
			if (partial) {
				dl->addDirectoryChangeTask(dir, false, false, true);
			}
		}

		return true;
	}
	return false;
}

bool DirectoryListingFrame::allowPopup() const {
	return dl->getIsOwnList() || (!SETTING(POPUNDER_FILELIST) && !dl->getPartialList()) || (!SETTING(POPUNDER_PARTIAL_LIST) && dl->getPartialList());
}

DirectoryListingFrame::DirectoryListingFrame(const DirectoryListingPtr& aList) :
	treeContainer(WC_TREEVIEW, this, CONTROL_MESSAGE_MAP),
	listContainer(WC_LISTVIEW, this, CONTROL_MESSAGE_MAP),
	browserBar(this, [this](const string& a, bool b) { handleHistoryClick(a, b); }, [this] { up(); }),
		treeRoot(nullptr), skipHits(0), updating(false), dl(aList), 
		UserInfoBaseHandler(true, false), changeType(CHANGE_LIST), 
		ctrlTree(this), statusDirty(false), selComboContainer(WC_COMBOBOX, this, COMBO_SEL_MAP), 
		ctrlFiles(this, COLUMN_LAST, [this] { callAsync([this] { filterList(); }); }, filterSettings, COLUMN_LAST)
{
	dl->addListener(this);
}

DirectoryListingFrame::~DirectoryListingFrame() {
	dl->removeListener(this);
}

void DirectoryListingFrame::disableBrowserLayout(bool redraw) {
	windowState = STATE_DISABLED;
	if (redraw) {
		ctrlFiles.list.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
		ctrlTree.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}

	// can't use EnableWindow as that message seems the get queued for the list view...
	ctrlFiles.SetWindowLongPtr(GWL_STYLE, ctrlFiles.list.GetWindowLongPtr(GWL_STYLE) | WS_DISABLED);
	ctrlTree.SetWindowLongPtr(GWL_STYLE, ctrlTree.GetWindowLongPtr(GWL_STYLE) | WS_DISABLED);
}

void DirectoryListingFrame::enableBrowserLayout(bool redraw) {
	windowState = STATE_ENABLED;
	ctrlFiles.SetWindowLongPtr(GWL_STYLE, ctrlFiles.list.GetWindowLongPtr(GWL_STYLE) & ~WS_DISABLED);
	ctrlTree.SetWindowLongPtr(GWL_STYLE, ctrlTree.GetWindowLongPtr(GWL_STYLE) & ~WS_DISABLED);

	if (redraw) {
		ctrlFiles.list.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
		ctrlTree.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

void DirectoryListingFrame::changeWindowState(bool enable, bool redraw) {
	ctrlToolbar.EnableButton(IDC_MATCH_QUEUE, enable && !dl->isMyCID());
	ctrlToolbar.EnableButton(IDC_MATCH_ADL, enable);
	ctrlToolbar.EnableButton(IDC_FIND, enable && !dl->getIsOwnList());
	ctrlToolbar.EnableButton(IDC_NEXT, enable && dl->curSearch ? TRUE : FALSE);
	ctrlToolbar.EnableButton(IDC_PREV, enable && dl->curSearch ? TRUE : FALSE);
	ctrlToolbar.EnableButton(IDC_FILELIST_DIFF, enable && dl->getPartialList() && !dl->getIsOwnList() ? false : enable);

	browserBar.changeWindowState(enable);
	ctrlFiles.changeFilterState(enable);
	selCombo.EnableWindow(enable);

	if (enable) {
		enableBrowserLayout(redraw);
		ctrlToolbar.EnableButton(IDC_GETLIST, dl->getPartialList() && !dl->isMyCID());
		ctrlToolbar.EnableButton(IDC_RELOAD_DIR, dl->getPartialList());
	} else {
		disableBrowserLayout(redraw);
		ctrlToolbar.EnableButton(IDC_RELOAD_DIR, FALSE);
		ctrlToolbar.EnableButton(IDC_GETLIST, FALSE);
	}
}

void DirectoryListingFrame::updateItemCache(const string& aPath) {
	auto curDir = dl->findDirectory(aPath);
	if (!curDir) {
		dcassert(0);
		return;
	}

	auto& iic = itemInfos[aPath];
	if (!iic) {
		iic = make_unique<ItemInfoCache>();
	}

	for (auto& d : curDir->directories | map_values) {
		iic->directories.emplace(d);
	}

	for (auto& f : curDir->files) {
		iic->files.emplace(f);
	}

	// Check that this directory exists in all parents
	if (aPath != ADC_ROOT_STR) {
		auto parent = Util::getAdcParentDir(aPath);
		auto p = itemInfos.find(parent);
		if (p != itemInfos.end()) {
			auto p2 = p->second->directories.find(ItemInfo(curDir));
			if (p2 != p->second->directories.end()) {
				// no need to update anything
				return;
			}
		}

		updateItemCache(parent);
	}
}

void DirectoryListingFrame::on(DirectoryListingListener::LoadingFinished, int64_t aStart, const string& aDir, bool aBackgroundTask) noexcept {
	// Get ownership of all item infos so that they won't be deleted before we finish loading
	vector<unique_ptr<ItemInfoCache>> removedInfos;
	for (auto i = itemInfos.begin(); i != itemInfos.end();) {
		if (AirUtil::isParentOrExactAdc(aDir, i->first)) {
			removedInfos.push_back(move(i->second));
			i = itemInfos.erase(i);
		} else {
			i++;
		}
	}

	updateItemCache(aDir);

	callAsync([=] {
		if (getActive()) {
			dl->setRead();
		}

		onLoadingFinished(aStart, aDir, aBackgroundTask);
	});
}

void DirectoryListingFrame::onLoadingFinished(int64_t aStart, const string& aDir, bool aBackgroundTask) {
	bool searching = dl->isCurrentSearchPath(aDir);

	if (!dl->getPartialList())
		updateStatus(CTSTRING(UPDATING_VIEW));


	if (searching)
		ctrlFiles.filter.clear();

	refreshTree(aDir, (changeType != CHANGE_TREE_EXPAND && !aBackgroundTask));

	changeWindowState(true);
	if (!searching) {
		int64_t loadTime = (GET_TICK() - aStart) / 1000;
		string msg;
		if (dl->getPartialList()) {
			if (aDir == ADC_ROOT_STR) {
				msg = STRING(PARTIAL_LIST_LOADED);
			}
		} else {
			msg = STRING_F(FILELIST_LOADED_IN, Util::formatSeconds(loadTime, true));
		}

		initStatus();
		updateStatus(Text::toT(msg));

		//notify the user that we've loaded the list
		setDirty();
	} else {
		findSearchHit(true);
		updateStatus(TSTRING_F(X_RESULTS_FOUND, dl->getResultCount()));
	}


	if (getActive()) { //Don't let it steal focus from other windows
		if (changeType == CHANGE_LIST) {
			ctrlFiles.list.SetFocus();
		}
		else {
			ctrlTree.SetFocus();
		}
	}

	changeType = CHANGE_LAST;
}

void DirectoryListingFrame::on(DirectoryListingListener::LoadingFailed, const string& aReason) noexcept {
	if (dl->getClosing()) {
		return;
	}

	callAsync([=] {
		updateStatus(Text::toT(aReason));
		if (!dl->getPartialList()) {
			PostMessage(WM_CLOSE, 0, 0);
		} else {
			changeWindowState(true);
		}
	});
}

void DirectoryListingFrame::on(DirectoryListingListener::LoadingStarted, bool aChangeDir) noexcept {
	// Wait for the GUI to be disabled so we won't access the list items that are
	// possibly going to be deleted
	bool waiting = true;

	callAsync([=, &waiting] { 
		if (aChangeDir) {
			// Don't disable everything to prevent flashing while browsing partial lists
			disableBrowserLayout(false);
		} else {
			changeWindowState(false);
			ctrlStatus.SetText(0, CTSTRING(LOADING_FILE_LIST));
		}

		waiting = false;
	});

	while (waiting) {
		Thread::sleep(20);
	}
}

void DirectoryListingFrame::on(DirectoryListingListener::QueueMatched, const string& aMessage) noexcept {
	callAsync([=] { updateStatus(Text::toT(aMessage)); });
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

void DirectoryListingFrame::on(DirectoryListingListener::ChangeDirectory, const string& aDir, bool aIsSearchChange) noexcept {
	//dcdebug("DirectoryListingListener::ChangeDirectory %s\n", aDir.c_str());
	callAsync([=] {
		if (aIsSearchChange)
			ctrlFiles.filter.clear();

		selectItem(aDir);
		if (aIsSearchChange) {
			updateStatus(TSTRING_F(X_RESULTS_FOUND, dl->getResultCount()));
			findSearchHit(true);
		}
	});

	changeWindowState(true);
}

void DirectoryListingFrame::on(DirectoryListingListener::UpdateStatusMessage, const string& aMessage) noexcept {
	callAsync([=] { updateStatus(Text::toT(aMessage)); });
}

void DirectoryListingFrame::activate() noexcept {
	if (allowPopup()) {
		callAsync([this] { MDIActivate(m_hWnd); });
	}
}

void DirectoryListingFrame::on(DirectoryListingListener::Read) noexcept {
	
}

void DirectoryListingFrame::on(DirectoryListingListener::UserUpdated) noexcept {
	callAsync([this] { updateSelCombo(); });
}

void DirectoryListingFrame::on(DirectoryListingListener::ShareProfileChanged) noexcept {
	callAsync([this] { updateSelCombo(); });
}

void DirectoryListingFrame::createColumns() {
	WinUtil::splitTokens(columnIndexes, SETTING(DIRECTORYLISTINGFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(DIRECTORYLISTINGFRAME_WIDTHS), COLUMN_LAST);
	for (uint8_t j = 0; j < COLUMN_LAST; j++)
	{
		int fmt = ((j == COLUMN_SIZE) || (j == COLUMN_EXACTSIZE) || (j == COLUMN_TYPE)) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlFiles.list.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j, columnTypes[j]);
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

	ctrlTree.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP | TVS_TRACKSELECT | TVS_NOHSCROLL |
		(SETTING(USE_EXPLORER_THEME) ? 0 : TVS_HASLINES), WS_EX_CLIENTEDGE, IDC_DIRECTORIES);
	ctrlTree.SetIndent(10);

	if(SETTING(USE_EXPLORER_THEME)) {
		ctrlTree.SetExtendedStyle(TVS_EX_FADEINOUTEXPANDOS | TVS_EX_DOUBLEBUFFER, TVS_EX_FADEINOUTEXPANDOS | TVS_EX_DOUBLEBUFFER);
		SetWindowTheme(ctrlTree.m_hWnd, L"explorer", NULL);
	}
	
	treeContainer.SubclassWindow(ctrlTree);
	ctrlFiles.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, NULL);
	listContainer.SubclassWindow(ctrlFiles.list);
	ctrlFiles.list.SetBkColor(WinUtil::bgColor);
	ctrlFiles.list.SetTextBkColor(WinUtil::bgColor);
	ctrlFiles.list.SetTextColor(WinUtil::textColor);
	ctrlFiles.list.SetFont(WinUtil::listViewFont);
	
	ctrlTree.SetBkColor(WinUtil::bgColor);
	ctrlTree.SetTextColor(WinUtil::textColor);
	ctrlTree.SetImageList(ResourceLoader::getFileImages(), TVSIL_NORMAL);

	selCombo.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		WS_HSCROLL | WS_VSCROLL | CBS_DROPDOWNLIST, WS_EX_CLIENTEDGE, IDC_HUB);
	selComboContainer.SubclassWindow(selCombo.m_hWnd);
	selCombo.SetFont(WinUtil::systemFont);
	
	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	SetSplitterPanes(ctrlTree.m_hWnd, ctrlFiles.m_hWnd);
	SetSplitterPosPct(25);

	browserBar.Init();
	//Cmd bar
	ctrlToolbar.Create(m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST, 0, ATL_IDW_TOOLBAR);
	ctrlToolbar.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS);
	ctrlToolbar.SetImageList(ResourceLoader::getFilelistTbImages());
	ctrlToolbar.SetButtonStructSize();
	addCmdBarButtons();
	ctrlToolbar.AutoSize();

	AddSimpleReBarBand(ctrlToolbar.m_hWnd, NULL, FALSE, NULL, TRUE);

	//maximize the path field.
	CReBarCtrl rebar = m_hWndToolBar;
	rebar.MaximizeBand(1);
	rebar.LockBands(true);

	createRoot();
	
	memzero(statusSizes, sizeof(statusSizes));

	int desclen = WinUtil::getTextWidth(getComboDesc(), ctrlStatus.m_hWnd);
	statusSizes[STATUS_HUB] = 180 + desclen;

	ctrlStatus.SetParts(STATUS_LAST, statusSizes);

	changeWindowState(false);
	
	SettingsManager::getInstance()->addListener(this);

	CRect rc(SETTING(DIRLIST_LEFT), SETTING(DIRLIST_TOP), SETTING(DIRLIST_RIGHT), SETTING(DIRLIST_BOTTOM));
	if(! (rc.top == 0 && rc.bottom == 0 && rc.left == 0 && rc.right == 0) )
		MoveWindow(rc, TRUE);
	
	WinUtil::SetIcon(m_hWnd, dl->getIsOwnList() ? IDI_OWNLIST : IDI_OPEN_LIST);
	bHandled = FALSE;

	::SetTimer(m_hWnd, 0, 500, 0);

	callAsync([this] { browserBar.updateHistoryCombo(); updateSelCombo(true); });
	return 1;
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

LRESULT DirectoryListingFrame::onGetFullList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	convertToFull();
	return 1;
}

void DirectoryListingFrame::convertToFull() {
	dcassert(!dl->getIsOwnList());

	dl->addAsyncTask([=] {
		try {
			QueueManager::getInstance()->addList(dl->getHintedUser(), QueueItem::FLAG_CLIENT_VIEW, ADC_ROOT_STR);
		} catch (...) {}
	});
}

DirectoryListingFrame::TreeType::ChildrenState DirectoryListingFrame::getChildrenState(const ItemInfo* ii) const {
	auto d = ii->dir;
	if (!d->directories.empty())
		return !d->isComplete() ? TreeType::ChildrenState::CHILDREN_PART_PENDING : TreeType::ChildrenState::CHILDREN_CREATED;

	if (d->getType() == DirectoryListing::Directory::TYPE_INCOMPLETE_CHILD)
		return TreeType::ChildrenState::CHILDREN_ALL_PENDING;

	//if (d->getLoading() && d->getType() == )
	//	return ChildrenState::CHILDREN_LOADING;

	return TreeType::ChildrenState::NO_CHILDREN;
}

void DirectoryListingFrame::expandDir(ItemInfo* ii, bool collapsing) {
	if (changeType != CHANGE_TREE_DOUBLE)
		changeType = collapsing ? CHANGE_TREE_COLLAPSE : CHANGE_TREE_EXPAND;

	if (collapsing || !ii->dir->isComplete()) {
		changeDir(ii);
	} /*else {
		dcdebug("DirectoryListingFrame::expandDir, skip changeDir %s\n", ii->dir->getPath().c_str());
	}*/
}

void DirectoryListingFrame::insertTreeItems(const string& aPath, HTREEITEM aParent) {
	auto p = itemInfos.find(aPath);
	//dcassert(p != itemInfos.end());
	if (p != itemInfos.end()) {
		for (const auto& d : p->second->directories) {
			ctrlTree.insertItem(&d, aParent, d.dir->getAdls());
		}
	} else {
		// We haven't been there before... Fill the cache and try again.
		updateItemCache(aPath);
		insertTreeItems(aPath, aParent);
	}
}

void DirectoryListingFrame::createRoot() {
	auto r = dl->getRoot();
	root.reset(new ItemInfo(r));
	const auto icon = ResourceLoader::DIR_NORMAL;
	treeRoot = ctrlTree.InsertItem(TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM, Text::toT(dl->getNick(true)).c_str(), icon, icon, 0, 0, (LPARAM)root.get(), NULL, NULL);
	browserBar.addHistory(dl->getRoot()->getAdcPath());
	dcassert(treeRoot); 
}

void DirectoryListingFrame::refreshTree(const string& aLoadedDir, bool aSelectDir) {
	ctrlTree.SetRedraw(FALSE);

	//check the root children state
	bool initialChange = !ctrlTree.hasChildren(treeRoot);
	if (initialChange && !dl->getRoot()->directories.empty()) {
		ctrlTree.setHasChildren(treeRoot, true);
	}

	//get the item for our new dir
	HTREEITEM ht = aLoadedDir == ADC_ROOT_STR ? treeRoot : ctrlTree.findItemByPath(treeRoot, Text::toT(aLoadedDir));
	if(!ht) {
		ht = treeRoot;
	}

	auto d = ((ItemInfo*)ctrlTree.GetItemData(ht))->dir;

	bool isExpanded = ctrlTree.IsExpanded(ht);

	// make sure that all tree subitems are removed and expand again if needed
	ctrlTree.Expand(ht, TVE_COLLAPSE | TVE_COLLAPSERESET);
	if (initialChange || isExpanded || changeType == CHANGE_TREE_EXPAND || changeType == CHANGE_TREE_DOUBLE)
		ctrlTree.Expand(ht);

	if (!aSelectDir && curPath == Util::getAdcParentDir(aLoadedDir)) {
		// find the loaded directory and set it as complete
		int j = ctrlFiles.list.GetItemCount();        
		for(int i = 0; i < j; i++) {
			const ItemInfo* ii = ctrlFiles.list.getItemData(i);
			if (ii->type == ii->DIRECTORY && ii->getAdcPath() == aLoadedDir) {
				ctrlFiles.list.SetItem(i, 0, LVIF_IMAGE, NULL, ResourceLoader::DIR_NORMAL, 0, 0, NULL);
				ctrlFiles.list.updateItem(i);
				updateStatus();
				break;
			}
		}
	} else if (aSelectDir || AirUtil::isParentOrExactAdc(aLoadedDir, curPath)) {
		// insert the new items
		ctrlTree.SelectItem(nullptr);

		// tweak to insert the items in the current thread
		curPath = d->getAdcPath();
		updateItems(d);

		selectItem(d->getAdcPath());
	}

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
	if (!ctrlTree.getSelectedItemData()->dir->isComplete()) {
		return;
	}

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
		if(ii->type == ItemInfo::FILE) {
			if(search->matchesFile(ii->file->getName(), ii->file->getSize(), ii->file->getRemoteDate(), ii->file->getTTH())) {
				found = true;
				break;
			}
		} else if(ii->type == ItemInfo::DIRECTORY && search->matchesDirectory(ii->dir->getName())) {
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

	auto s = make_shared<Search>(Priority::HIGH, Util::toString(Util::rand()));

	s->query = dlg.searchStr;
	s->size = dlg.size;
	s->sizeType = dlg.sizeMode;

	s->fileType = dlg.fileType;
	s->exts = dlg.extList;

	s->returnParents = true;
	s->matchType = Search::MATCH_NAME_PARTIAL;
	s->maxResults = 20;
	s->requireReply = true;

	if (dlg.useCurDir) {
		s->path = curPath;
	}

	dl->addSearchTask(s);
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
	auto ii = itemInfos.find(curPath);
	if (ii != itemInfos.end()) {
		return ii->second->directories.size() + ii->second->files.size();
	}

	dcassert(0);
	return 0;
}


void DirectoryListingFrame::updateStatus() noexcept {
	auto currentDir = ctrlTree.getSelectedItemData();
	if (!currentDir) {
		return;
	}

	updateStatus(currentDir->dir);
}

void DirectoryListingFrame::updateStatus(const DirectoryListing::Directory::Ptr& aCurrentDir) noexcept {
	if (updating || !ctrlStatus.IsWindow()) {
		return;
	}

	int selectedCount = ctrlFiles.list.GetSelectedCount();
	int64_t totalFileSize = 0;
	int totalCount = 0;
	auto displayCount = ctrlFiles.list.GetItemCount();

	DirectoryListing::Directory::List directories;
	if (selectedCount > 0) {
		ctrlFiles.list.forEachSelectedT([&](const ItemInfo* ii) {
			if (ii->type == ItemInfo::FILE) {
				totalFileSize += ii->file->getSize();
			} else {
				directories.push_back(ii->dir);
			}
		});
	} else {
		totalCount = getTotalListItemCount();
	}

	auto totalSize = totalFileSize;
	if (selectedCount == 0) {
		totalSize = aCurrentDir->getTotalSize(aCurrentDir != dl->getRoot());
	} else {
		for (const auto& d: directories) {
			totalSize += d->getTotalSize(false);
		}
	}

	updateStatusText(totalCount, totalSize, selectedCount, displayCount, aCurrentDir->getLastUpdateDate());
}

void DirectoryListingFrame::updateStatusText(int aTotalCount, int64_t aTotalSize, int aSelectedCount, int aDisplayCount, time_t aUpdateDate) {
	tstring tmp = TSTRING(ITEMS) + _T(": ");

	if (aSelectedCount != 0) {
		tmp += Util::toStringW(aSelectedCount);
	}
	else if (aDisplayCount != aTotalCount) {
		tmp += Util::toStringW(aDisplayCount) + _T("/") + Util::toStringW(aTotalCount);
	}
	else {
		tmp += Util::toStringW(aDisplayCount);
	}
	bool u = false;

	int w = WinUtil::getTextWidth(tmp, ctrlStatus.m_hWnd);
	if (statusSizes[STATUS_SELECTED_FILES] < w) {
		statusSizes[STATUS_SELECTED_FILES] = w;
		u = true;
	}
	ctrlStatus.SetText(STATUS_SELECTED_FILES, tmp.c_str());

	tmp = TSTRING(SIZE) + _T(": ") + Util::formatBytesW(aTotalSize);
	w = WinUtil::getTextWidth(tmp, ctrlStatus.m_hWnd);
	if (statusSizes[STATUS_SELECTED_SIZE] < w) {
		statusSizes[STATUS_SELECTED_SIZE] = w;
		u = true;
	}
	ctrlStatus.SetText(STATUS_SELECTED_SIZE, tmp.c_str());

	tmp = aUpdateDate > 0 ? TSTRING_F(UPDATED_ON_X, Text::toT(Util::formatTime("%c", aUpdateDate))) : Util::emptyStringT;
	w = WinUtil::getTextWidth(tmp, ctrlStatus.m_hWnd);
	if (statusSizes[STATUS_UPDATED] < w) {
		statusSizes[STATUS_UPDATED] = w;
		u = true;
	}
	ctrlStatus.SetText(STATUS_UPDATED, tmp.c_str());

	if (selCombo.GetStyle() & WS_VISIBLE) {
		tmp = getComboDesc();
	}
	else if (dl->isMyCID()) {
		tmp = TSTRING(OWN_FILELIST);
	}
	else if (!dl->getUser()->isNMDC() || !dl->getUser()->isOnline()) {
		tmp = TSTRING(USER_OFFLINE);
	}
	else {
		tmp = TSTRING(USER_ONLINE);
	}

	ctrlStatus.SetText(STATUS_HUB, tmp.c_str());
	if (u)
		UpdateLayout(TRUE);
}

tstring DirectoryListingFrame::getComboDesc() {
	return (dl->getIsOwnList() ? TSTRING(SHARE_PROFILE) : TSTRING(BROWSE_VIA));
}

void DirectoryListingFrame::initStatus() {
	dl->addAsyncTask([=] {
		size_t totalFiles = 0;
		int64_t totalSize = 0;
		if (dl->getPartialList() && !dl->getHintedUser().user->isNMDC()) {
			dl->getPartialListInfo(totalSize, totalFiles);
		} else {
			totalSize = dl->getTotalListSize();
			totalFiles = dl->getTotalFileCount();
		}

		callAsync([=] {
			tstring tmp = TSTRING_F(TOTAL_SIZE, Util::formatBytesW(totalSize));
			statusSizes[STATUS_TOTAL_SIZE] = WinUtil::getTextWidth(tmp, m_hWnd);
			ctrlStatus.SetText(STATUS_TOTAL_SIZE, tmp.c_str());

			tmp = TSTRING_F(TOTAL_FILES, totalFiles);
			statusSizes[STATUS_TOTAL_FILES] = WinUtil::getTextWidth(tmp, m_hWnd);
			ctrlStatus.SetText(STATUS_TOTAL_FILES, tmp.c_str());

			UpdateLayout(FALSE);
		});
	});
}

LRESULT DirectoryListingFrame::onSelChangedDirectories(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	if (updating) {
		dcdebug("SKIP: DirectoryListingFrame::onSelChangedDirectories, updating\n");
		return 0;
	}

	NMTREEVIEW* p = (NMTREEVIEW*) pnmh;

	if(p->itemNew.state & TVIS_SELECTED) {
		auto ii = (ItemInfo*)p->itemNew.lParam;

		// Don't cause an infinite loop when changing directories in the GUI faster than the
		// list thread is able to handle (= don't send another changeDir for previously queued changes)
		if (curPath != ii->getAdcPath() && dl->getCurrentLocationInfo().directory != ii->dir) {
			if (changeType != CHANGE_HISTORY)
				browserBar.addHistory(ii->getAdcPath());

			curPath = ii->getAdcPath();
			changeDir(ii);
		}
	}

	return 0;
}

LRESULT DirectoryListingFrame::onClickTree(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled) {
	changeType = CHANGE_TREE_SINGLE;
	bHandled = FALSE;
	return 0;
}

void DirectoryListingFrame::on(DirectoryListingListener::RemovedQueue, const string& aDir) noexcept {
	callAsync([=] {
		HTREEITEM ht = ctrlTree.findItemByPath(treeRoot, Text::toT(aDir));
		if (ht) {
			ctrlTree.updateItemImage(ht);
		}

		updateStatus(Text::toT(aDir) + _T(": ") + TSTRING(TARGET_REMOVED));
	});
}

void DirectoryListingFrame::filterList() {
	// in case it's a timed event
	if (windowState != STATE_ENABLED)
		return;

	updating = true;
	ctrlFiles.list.SetRedraw(FALSE);

	insertItems(nullopt);

	ctrlFiles.list.SetRedraw(TRUE);
	updating = false;
	updateStatus();
}

void DirectoryListingFrame::insertItems(const optional<string>& selectedName) {
	ctrlFiles.list.DeleteAllItems();

	int curPos = 0;
	int selectedPos = -1;

	dcassert(itemInfos.find(curPath) != itemInfos.end());
	const auto& dirs = itemInfos[curPath]->directories;
	auto i = dirs.crbegin();

	auto filterInfo = [this, &i](int column) { return (*i).getTextNormal(column); };

	auto numericInfo = [this, &i](int column) -> double {
		switch (column) {
			case COLUMN_SIZE:
			case COLUMN_EXACTSIZE: return i->type == ItemInfo::DIRECTORY ? i->dir->getTotalSize(true) : i->file->getSize();
			case COLUMN_DATE: return i->type == ItemInfo::DIRECTORY ? i->dir->getRemoteDate() : i->file->getRemoteDate();
			default: dcassert(0); return 0;
		}
	};

	auto filterPrep = ctrlFiles.filter.prepare(filterInfo, numericInfo);

	auto doInsert = [&](const decltype(dirs)& map) {
		for (; i != map.crend(); ++i) {
			if (ctrlFiles.checkDupe((*i).getDupe()) && (ctrlFiles.filter.empty() || ctrlFiles.filter.match(filterPrep))) {
				if (selectedName && compare(*selectedName, (*i).getName()) == 0) {
					selectedPos = curPos;
				}

				ctrlFiles.list.insertItem(curPos++, &(*i), (*i).type == ItemInfo::DIRECTORY ? getIconIndex(&(*i)) : (*i).getImageIndex());
			}
		}
	};

	doInsert(dirs);
	const auto& files = itemInfos[curPath]->files;
	i = files.crbegin();
	doInsert(files);

	if (selectedPos != -1) {
		ctrlFiles.list.SelectItem(selectedPos);
	}

	if (ctrlFiles.list.getSortColumn() != COLUMN_FILENAME) {
		ctrlFiles.list.resort();
	}
}

void DirectoryListingFrame::updateItems(const DirectoryListing::Directory::Ptr& d) {
	ctrlFiles.list.SetRedraw(FALSE);
	updating = true;
	if (itemInfos.find(d->getAdcPath()) == itemInfos.end()) {
		updateItemCache(d->getAdcPath());
	}

	optional<string> selectedName;
	if (changeType == CHANGE_HISTORY) {
		auto historyPath = browserBar.getCurSel();
		if (!historyPath.empty() && (!d->getParent() || Util::getAdcParentDir(historyPath) == d->getAdcPath())) {
			selectedName = Util::getLastDir(historyPath);
		}

		changeType = CHANGE_LIST; //reset
	}

	ctrlFiles.onListChanged(false);
	insertItems(selectedName);

	ctrlFiles.list.SetRedraw(TRUE);
	updating = false;
	updateStatus(d);
}

void DirectoryListingFrame::changeDir(const ItemInfo* ii, bool aReloadDir) {
	if (!ii) {
		dcassert(0);
		return;
	}

	if (!aReloadDir) {
		updateItems(ii->dir);
	}

	auto path = ii->getAdcPath();
	//dcdebug("DirectoryListingFrame::changeDir %s\n", path.c_str());

	dl->addDirectoryChangeTask(path, aReloadDir);
}

void DirectoryListingFrame::up() {
	HTREEITEM t = ctrlTree.GetSelectedItem();
	if(t == NULL)
		return;
	t = ctrlTree.GetParentItem(t);
	if(t == NULL)
		return;
	auto aPrevPath = curPath;
	ctrlTree.SelectItem(t);
	
	callAsync([=] { listViewSelectSubDir(aPrevPath, curPath); });
}



void DirectoryListingFrame::handleHistoryClick(const string& aPath, bool byHistory) {
	if(byHistory) 
		changeType = CHANGE_HISTORY;
	auto aPrevPath = curPath;
	selectItem(aPath);
	listViewSelectSubDir(aPrevPath, aPath);

}

void DirectoryListingFrame::listViewSelectSubDir(const string& aSubPath, const string& aParentPath) {
	if (!aSubPath.empty() && AirUtil::isSubAdc(aSubPath, aParentPath)) {
		auto i = ctrlFiles.list.findItem(Text::toT(Util::getAdcLastDir(aSubPath)));
		if (i != -1)
			ctrlFiles.list.SelectItem(i);
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
	auto t = ctrlTree.GetSelectedItem();
	if (!t) {
		dcassert(0);
		return;
	}

	if (ctrlFiles.list.GetSelectedCount() == 1) {
		const ItemInfo* ii = ctrlFiles.list.getItemData(ctrlFiles.list.GetNextItem(-1, LVNI_SELECTED));
		if(ii->type == ItemInfo::FILE) {
			// if we already have it there's no reason to download, try to open instead.
			if (dl->getIsOwnList() || AirUtil::isFinishedDupe(ii->file->getDupe()) || AirUtil::isShareDupe(ii->file->getDupe())) {
				openDupe(ii->file, false);
			} else
				onDownload(SETTING(DOWNLOAD_DIRECTORY), false, false, WinUtil::isShift() ? Priority::HIGHEST : Priority::DEFAULT);
		} else {
			changeType = CHANGE_LIST;
			auto ht = ctrlTree.findItemByName(t, ii->getNameW());
			if (ht) {
				ctrlTree.SelectItem(ht);
			} else {
				dcassert(0);
			}
		}
	} else {
		onDownload(SETTING(DOWNLOAD_DIRECTORY), false, false, WinUtil::isShift() ? Priority::HIGHEST : Priority::DEFAULT);
	}
}

LRESULT DirectoryListingFrame::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	if(kd->wVKey == VK_BACK) {
		up();
	} else if(kd->wVKey == VK_TAB) {
		onTab();
/* alt + left is used for switching tabs, maybe think of another combo for tabs since its used in explorer for back forward
} else if(kd->wVKey == VK_LEFT && WinUtil::isAlt()) {
		browserBar.back();
	} else if(kd->wVKey == VK_RIGHT && WinUtil::isAlt()) {
		browserBar.forward();*/
	} else if(kd->wVKey == VK_RETURN) {
		onListItemAction();
	} else if (checkCommonKey(kd->wVKey)) {
		bHandled = TRUE;
		return 1;
	}
	return 0;
}

LRESULT DirectoryListingFrame::onKeyDownDirs(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMTVKEYDOWN* kd = (NMTVKEYDOWN*) pnmh;
	if(kd->wVKey == VK_DOWN || kd->wVKey == VK_DOWN) {
		changeType = CHANGE_TREE_SINGLE;
	} else if (checkCommonKey(kd->wVKey)) {
		bHandled = TRUE;
		return 1;
	}
	return 0;
}

bool DirectoryListingFrame::checkCommonKey(int key) {
	if (key == VK_TAB) {
		onTab();
		return true;
	}

	return false;
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
		if (WinUtil::browseList(file, m_hWnd)) {
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
	if (!dl->getIsOwnList() && dl->getPartialList() && WinUtil::showQuestionBox(TSTRING(ADL_DL_FULL_LIST), MB_ICONQUESTION)) {
		ctrlStatus.SetText(0, CTSTRING(DOWNLOADING_LIST));

		dl->setMatchADL(true);
		convertToFull();
	} else {
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
			handleCopyDir();
		} else {
			ctrlFiles.list.handleCopy([&](const ItemInfo* ii) { return ii->getText(0); });
		}
		return 1;
	}
		
	bHandled = FALSE;
	return 0;
}

void DirectoryListingFrame::selectItem(const string& aPath) {
	HTREEITEM ht = ctrlTree.findItemByPath(treeRoot, Text::toT(aPath));
	if (!ht) {
		//dcassert(0);
		return;
	}

	if (ctrlTree.GetSelectedItem() != ht) {
		//dcdebug("DirectoryListingFrame::selectItem %s\n", name.c_str());

		if (changeType == CHANGE_LIST)
			ctrlTree.EnsureVisible(ht);
		ctrlTree.SelectItem(ht);
	}
}

LRESULT DirectoryListingFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlFiles.list && ctrlFiles.list.GetSelectedCount() > 0) {
		auto pt = WinUtil::getMenuPosition({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }, ctrlFiles.list);
		if (pt) {
			appendListContextMenu(*pt);
			return TRUE;
		}
	} if(reinterpret_cast<HWND>(wParam) == ctrlTree && ctrlTree.GetSelectedItem() != NULL) { 
		CPoint pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		if (pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlTree, pt);
		}

		ctrlTree.ScreenToClient(&pt);

		UINT a = 0;
		HTREEITEM ht = ctrlTree.HitTest(pt, &a);
		if (ht) {
			// Strange, windows doesn't change the selection on right-click... (!)
			if (ht != ctrlTree.GetSelectedItem())
				ctrlTree.SelectItem(ht);

			ctrlTree.ClientToScreen(&pt);
			changeType = CHANGE_TREE_SINGLE;

			auto dir = ((ItemInfo*) ctrlTree.GetItemData(ht))->dir;

			appendTreeContextMenu(pt, dir);
			return TRUE;
		}
	} 
	
	bHandled = FALSE;
	return FALSE; 
}

void DirectoryListingFrame::appendListContextMenu(CPoint& pt) {
	const ItemInfo* ii = ctrlFiles.list.getItemData(ctrlFiles.list.GetNextItem(-1, LVNI_SELECTED));

	ShellMenu fileMenu;
	fileMenu.CreatePopupMenu();

	int i = -1;
	bool allComplete = true, hasFiles = false;
	while ((i = ctrlFiles.list.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const ItemInfo* iiFiles = ctrlFiles.list.getItemData(i);
		if (iiFiles->type == ItemInfo::DIRECTORY && !iiFiles->dir->isComplete() && iiFiles->dir->getPartialSize() == 0) {
			allComplete = false;
		} else if (iiFiles->type == ItemInfo::FILE) {
			hasFiles = true;
		}
	}

	bool isDupeOrOwnlist = ctrlFiles.list.GetSelectedCount() == 1 && (dl->getIsOwnList() || AirUtil::allowOpenDupe(ii->getDupe()));

	if (!dl->getIsOwnList()) {
		// download menu
		optional<TTHValue> tth;
		optional<string> path;

		if (ii->type == ItemInfo::FILE) {
			if (ctrlFiles.list.GetSelectedCount() == 1)
				tth = ii->file->getTTH();

			path = ii->getAdcPath();
		} else if (ctrlFiles.list.GetSelectedCount() == 1) {
			path = ii->getAdcPath();
		}

		appendDownloadMenu(fileMenu, DownloadBaseHandler::TYPE_PRIMARY, !allComplete, tth, path, true, !isDupeOrOwnlist);
	}

	if (hasFiles)
		fileMenu.appendItem(TSTRING(VIEW_AS_TEXT), [this] { handleViewAsText(); });

	if (hasFiles || !dl->getIsOwnList())
		fileMenu.appendSeparator();

	if (hasFiles) {
		fileMenu.appendItem(TSTRING(OPEN), [this] { handleOpenFile(); }, isDupeOrOwnlist ? OMenu::FLAG_DEFAULT : 0);
	}

	if (!hasFiles && dl->getUser()->isSet(User::ASCH) && !dl->getIsOwnList()) {
		fileMenu.appendItem(TSTRING(VIEW_NFO), [this] { handleViewNFO(false); });
	}

	// searching (client)
	if (ctrlFiles.list.GetSelectedCount() == 1 && ii->type == ItemInfo::FILE)
		fileMenu.appendItem(TSTRING(SEARCH_TTH), [this] { handleSearchByTTH(); });

	fileMenu.appendItem(TSTRING(SEARCH_DIRECTORY), [this] { handleSearchByName(false, true); });
	if (hasFiles)
		fileMenu.appendItem(TSTRING(SEARCH), [this] { handleSearchByName(false, false); });

	fileMenu.appendSeparator();

	// web shortcuts
	WinUtil::appendSearchMenu(fileMenu, [=](const WebShortcut* ws) {
		ctrlFiles.list.forEachSelectedT([=](const ItemInfo* ii) {
			WinUtil::searchSite(ws, ii->getAdcPath());
		});
	});

	fileMenu.appendSeparator();

	// copy menu
	ListBaseType::MenuItemList customItems{
		{ TSTRING(DIRECTORY), &handleCopyDirectory },
		{ TSTRING(MAGNET_LINK), &handleCopyMagnet },
		{ TSTRING(PATH), &handleCopyPath }
	};
	ctrlFiles.list.appendCopyMenu(fileMenu, customItems);


	if (ctrlFiles.list.GetSelectedCount() == 1 && ii->type == ItemInfo::FILE) {
		fileMenu.InsertSeparatorFirst(Text::toT(Util::getFileName(ii->file->getName())));
		if (ii->file->getAdls()) {
			fileMenu.appendItem(TSTRING(GO_TO_DIRECTORY), [this] { handleGoToDirectory(false); });
		}
	} else {
		if (ii->type == ItemInfo::DIRECTORY && ii->dir->getAdls() && ii->dir->getParent() != dl->getRoot().get()) {
			fileMenu.appendItem(TSTRING(GO_TO_DIRECTORY), [this] { handleGoToDirectory(false); });
		}
	}

	// User commands
	prepareMenu(fileMenu, UserCommand::CONTEXT_FILELIST, ClientManager::getInstance()->getHubUrls(dl->getHintedUser().user->getCID()));

	// Share stuff
	if (dl->getIsOwnList() && !hasFiles) {
		fileMenu.appendItem(TSTRING(REFRESH_IN_SHARE), [this] { handleRefreshShare(false); });
	}
	if (dl->getIsOwnList() && !(ii->type == ItemInfo::DIRECTORY && ii->dir->getAdls())) {
		fileMenu.appendItem(TSTRING(SCAN_FOLDER_MISSING), [this] { handleScanShare(false); });
		fileMenu.appendItem(TSTRING(RUN_SFV_CHECK), [this] { handleCheckSfv(false); });
	}

	// shell menu
	if (isDupeOrOwnlist) {
		StringList paths;
		if (getLocalPaths(paths, false, false)) {
			fileMenu.appendShellMenu(paths);
		}
	}

	fileMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
}

void DirectoryListingFrame::appendTreeContextMenu(CPoint& pt, DirectoryListing::Directory::Ptr& dir) {
	ShellMenu directoryMenu;
	directoryMenu.CreatePopupMenu();

	if (!dl->getIsOwnList()) {
		appendDownloadMenu(directoryMenu, DownloadBaseHandler::TYPE_SECONDARY, false, nullopt, dir->getAdcPath());
		directoryMenu.appendSeparator();
	}

	WinUtil::appendSearchMenu(directoryMenu, curPath);
	directoryMenu.appendSeparator();

	if (dir && dir->getAdls() && dir->getParent() != dl->getRoot().get()) {
		directoryMenu.appendItem(TSTRING(GO_TO_DIRECTORY), [this] { handleGoToDirectory(true); });
	}

	directoryMenu.appendItem(TSTRING(COPY_DIRECTORY), [=] { handleCopyDir(); });
	directoryMenu.appendItem(TSTRING(SEARCH), [=] { handleSearchByName(true, false); });

	if (dl->getUser()->isSet(User::ASCH) && !dl->getIsOwnList()) {
		directoryMenu.appendItem(TSTRING(VIEW_NFO), [this] { handleViewNFO(true); });
	}

	if (dl->getPartialList() && !dir->getAdls()) {
		directoryMenu.appendSeparator();
		directoryMenu.appendItem(TSTRING(RELOAD), [=] { handleReloadPartial(); });
	}

	if (dl->getIsOwnList() || (dir && dl->getPartialList() && AirUtil::allowOpenDupe(dir->getDupe()))) {
		StringList paths;
		if (getLocalPaths(paths, true, true)) {
			directoryMenu.appendShellMenu(paths);
			if (!AirUtil::isFinishedDupe(dir->getDupe())) {
				// shared
				directoryMenu.appendSeparator();
				directoryMenu.appendItem(TSTRING(REFRESH_IN_SHARE), [this] { handleRefreshShare(true); });
				directoryMenu.appendItem(TSTRING(SCAN_FOLDER_MISSING), [this] { handleScanShare(true); });
				directoryMenu.appendItem(TSTRING(RUN_SFV_CHECK), [this] { handleCheckSfv(true); });
			}
		}
	}

	directoryMenu.InsertSeparatorFirst(TSTRING(DIRECTORY));
	directoryMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
}

void DirectoryListingFrame::handleItemAction(bool usingTree, std::function<void (const ItemInfo* ii)> aF, bool firstOnly /*false*/) {
	if (usingTree) {
		auto dir = ctrlTree.getSelectedItemData();
		aF(dir);
	} else if (ctrlFiles.list.GetSelectedCount() >= 1) {
		int sel = -1;
		while ((sel = ctrlFiles.list.GetNextItem(sel, LVNI_SELECTED)) != -1) {
			const ItemInfo* ii = ctrlFiles.list.getItemData(sel);
			aF(ii);
			if (firstOnly)
				break;
		}
	}
}

void DirectoryListingFrame::handleDownload(const string& aTarget, Priority aPriority, bool aUsingTree) {
	handleItemAction(aUsingTree, [&](const ItemInfo* ii) {
		if (ii->type == ItemInfo::FILE) {
			WinUtil::addFileDownload(aTarget + (!Util::isDirectoryPath(aTarget) ? Util::emptyString : ii->getName()), ii->file->getSize(), ii->file->getTTH(), dl->getHintedUser(), ii->file->getRemoteDate(),
				0, aPriority);
		} else {
			dl->addAsyncTask([=] {
				try {
					DirectoryListingManager::getInstance()->addDirectoryDownload(dl->getHintedUser(), ii->getName(), ii->getAdcPath(), aTarget, aPriority);
				} catch (const Exception& e) {
					ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
				}
			});
		}
	});
}

void DirectoryListingFrame::handleViewAsText() {
	handleItemAction(false, [this](const ItemInfo* ii) {
		if (ii->type == ItemInfo::FILE) {
			if (dl->getIsOwnList()) {
				ViewFileManager::getInstance()->addLocalFile(ii->file->getTTH(), true);
			} else {
				ViewFileManager::getInstance()->addUserFileNotify(ii->file->getName(), ii->file->getSize(), ii->file->getTTH(), dl->getHintedUser(), true);
			}
		}
	});
}

void DirectoryListingFrame::handleSearchByTTH() {
	handleItemAction(false, [this](const ItemInfo* ii) {
		if (ii->type == ItemInfo::FILE) {
			WinUtil::searchHash(ii->file->getTTH(), ii->file->getName(), ii->file->getSize());
		}
	});
}

void DirectoryListingFrame::handleGoToDirectory(bool usingTree) {
	handleItemAction(usingTree, [this](const ItemInfo* ii) {
		string path;
		if (ii->type == ItemInfo::FILE) {
			if (!ii->file->getAdls())
				return;

			path = Util::getAdcFilePath(ii->getAdcPath());
		} else if (ii->type == ItemInfo::DIRECTORY)	{
			if (!(ii->dir->getAdls() && ii->dir->getParent() != dl->getRoot().get()))
				return;
			path = ((DirectoryListing::AdlDirectory*)ii->dir.get())->getFullAdcPath();
		}

		if (!path.empty()) {
			selectItem(path);
		}
	}, true);
}

void DirectoryListingFrame::handleViewNFO(bool usingTree) {
	handleItemAction(usingTree, [this](const ItemInfo* ii) {
		if (ii->type == ItemInfo::DIRECTORY) {
			WinUtil::findNfo(ii->getAdcPath(), dl->getHintedUser());
		}
	});
}

void DirectoryListingFrame::handleOpenFile() {
	handleItemAction(false, [this](const ItemInfo* ii) {
		if (ii->type != ItemInfo::FILE) {
			return;
		}

		if (dl->getIsOwnList() || AirUtil::allowOpenDupe(ii->file->getDupe())) {
			openDupe(ii->file, false);
		} else {
			WinUtil::openFile(ii->file->getName(), ii->file->getSize(), ii->file->getTTH(), dl->getHintedUser(), false);
		}
	});
}
void DirectoryListingFrame::openDupe(const DirectoryListing::Directory::Ptr& d) {
	try {
		StringList paths;
		dl->getLocalPaths(d, paths);
		if (!paths.empty()) {
			WinUtil::openFolder(Text::toT(paths.front()));
		}
	} catch (const ShareException& e) {
		updateStatus(Text::toT(e.getError()));
	}
}

void DirectoryListingFrame::openDupe(const DirectoryListing::File::Ptr& f, bool openDir) noexcept {
	try {
		StringList paths;
		dl->getLocalPaths(f, paths);

		if (!paths.empty()) {
			auto path = Text::toT(paths.front());
			if (!openDir) {
				WinUtil::openFile(path);
			}
			else {
				WinUtil::openFolder(path);
			}
		} else {
			updateStatus(TSTRING(FILE_NOT_FOUND));
		}
	} catch (const ShareException& e) {
		updateStatus(Text::toT(e.getError()));
	}
}

void DirectoryListingFrame::handleReloadPartial() {
	handleItemAction(true, [=](const ItemInfo* ii) {
		if (ii && !ii->dir->getAdls())
			changeDir(ii, true);
	});
}

void DirectoryListingFrame::handleSearchByName(bool usingTree, bool dirsOnly) {
	bool thisSearched = false;
	handleItemAction(usingTree, [&](const ItemInfo* ii) {
		string name;
		if (ii->type == ItemInfo::FILE) {
			if (dirsOnly) {
				if (thisSearched)
					return;

				name = AirUtil::getAdcReleaseDir(ii->getAdcPath(), true);
				thisSearched = true;
			} else {
				name = ii->getName();
			}
		} else {
			name = ii->getName();
		}

		WinUtil::search(Text::toT(name), dirsOnly);
	});
}

void DirectoryListingFrame::handleCopyDir() {
	handleItemAction(true, [this](const ItemInfo* ii) {
		auto sCopy = Text::toT(ii->dir->getName());
		if (!sCopy.empty()) {
			WinUtil::setClipboard(sCopy);
		}
	});
}

void DirectoryListingFrame::handleRefreshShare(bool usingTree) {
	StringList refresh;
	if (getLocalPaths(refresh, usingTree, true)) {
		ShareManager::getInstance()->refreshPaths(refresh);
	}
}

void DirectoryListingFrame::handleScanShare(bool usingTree) {
	ctrlStatus.SetText(0, CTSTRING(SEE_SYSLOG_FOR_RESULTS));

	StringList scanList;
	if (getLocalPaths(scanList, usingTree, false)) {
		ShareScannerManager::getInstance()->scanShare(scanList);
	}
}

void DirectoryListingFrame::handleCheckSfv(bool usingTree) {
	ctrlStatus.SetText(0, CTSTRING(SEE_SYSLOG_FOR_RESULTS));

	StringList scanList;
	if (getLocalPaths(scanList, usingTree, false)) {
		ShareScannerManager::getInstance()->checkSfv(scanList);
	}
}

bool DirectoryListingFrame::getLocalPaths(StringList& paths_, bool usingTree, bool dirsOnly) {
	string error;
	handleItemAction(usingTree, [&](const ItemInfo* ii) {
		try {
			if (!dirsOnly && ii->type == ItemInfo::FILE) {
				dl->getLocalPaths(ii->file, paths_);
			} else if (ii->type == ItemInfo::DIRECTORY)  {
				dl->getLocalPaths(ii->dir, paths_);
			}
		} catch (ShareException& e) {
			error = e.getError();
		}
	});

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
			ucParams["fileFN"] = [this, ii] { return Util::toNmdcFile(ii->getAdcPath()); };
			ucParams["fileSI"] = [ii] { return Util::toString(ii->file->getSize()); };
			ucParams["fileSIshort"] = [ii] { return Util::formatBytes(ii->file->getSize()); };
			ucParams["fileTR"] = [ii] { return ii->file->getTTH().toBase32(); };
			ucParams["fileMN"] = [ii] { return WinUtil::makeMagnet(ii->file->getTTH(), ii->file->getName(), ii->file->getSize()); };
		} else {
			ucParams["type"] = [] { return "Directory"; };
			ucParams["fileFN"] = [this, ii] { return Util::toNmdcFile(ii->getAdcPath()); };
			ucParams["fileSI"] = [this, ii] { return Util::toString(ii->dir->getTotalSize(ii->dir != dl->getRoot())); };
			ucParams["fileSIshort"] = [ii] { return Util::formatBytes(ii->dir->getTotalSize(true)); };
		}

		// compatibility with 0.674 and earlier
		ucParams["file"] = ucParams["fileFN"];
		ucParams["filesize"] = ucParams["fileSI"];
		ucParams["filesizeshort"] = ucParams["fileSIshort"];
		ucParams["tth"] = ucParams["fileTR"];

		auto tmp = ucParams;
		ClientManager::getInstance()->userCommand(dl->getHintedUser(), uc, tmp, true);
	}
}

tstring DirectoryListingFrame::handleCopyMagnet(const ItemInfo* ii) {
	if (ii->type == ItemInfo::FILE) {
		return Text::toT(WinUtil::makeMagnet(ii->file->getTTH(), ii->file->getName(), ii->file->getSize()));
	} 
		
	return Util::emptyStringT;
}

tstring DirectoryListingFrame::handleCopyPath(const ItemInfo* ii) {
	tstring ret = _T(" ");
	if (ii->type == ItemInfo::FILE) {
		ret += Text::toT(ii->getAdcPath());
	} else {
		if (ii->dir->getAdls() && ii->dir->getParent()->getParent()) { // not root
			ret += Text::toT(((DirectoryListing::AdlDirectory*)ii->dir.get())->getFullAdcPath());
		} else {
			ret += Text::toT(ii->getAdcPath());
		}
	}
	return ret;
}

tstring DirectoryListingFrame::handleCopyDirectory(const ItemInfo* ii) {
	if (ii->type == ItemInfo::FILE) {
		return Text::toT(AirUtil::getAdcReleaseDir(ii->getAdcPath(), true));
	} else {
		return ii->getText(COLUMN_FILENAME);
	}
}

LRESULT DirectoryListingFrame::onXButtonUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /* lParam */, BOOL& /* bHandled */) {
	if(GET_XBUTTON_WPARAM(wParam) & XBUTTON1) {
		browserBar.back();
		return TRUE;
	} else if(GET_XBUTTON_WPARAM(wParam) & XBUTTON2) {
		browserBar.forward();
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
	handleItemAction(isWhole, [&](const ItemInfo* ii) {
		if (ii->type == ItemInfo::FILE) {
			size += ii->file->getSize();
		} else {
			size += ii->dir->getTotalSize(false);
		}
	});

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

bool DirectoryListingFrame::ItemInfo::NameSort::operator()(const ItemInfo& a, const ItemInfo& b) const {
	auto comp = compareItems(&a, &b, DirectoryListingFrame::COLUMN_FILENAME);
	if (comp == 0 && a.isAdl() && b.isAdl()) {
		// there can be multiple items with the same name in a single ADL directory... compare the path instead
		return Util::DefaultSort(Text::toT(a.getAdcPath()).c_str(), Text::toT(b.getAdcPath()).c_str()) > 0;
	}
	return comp > 0;
}

int DirectoryListingFrame::ItemInfo::compareItems(const ItemInfo* a, const ItemInfo* b, uint8_t col) {
	if(a->type == DIRECTORY) {
		if(b->type == DIRECTORY) {
			switch(col) {
				case COLUMN_EXACTSIZE: return compare(a->dir->getTotalSize(true), b->dir->getTotalSize(true));
				case COLUMN_SIZE: return compare(a->dir->getTotalSize(true), b->dir->getTotalSize(true));
				case COLUMN_DATE: return compare(a->dir->getRemoteDate(), b->dir->getRemoteDate());
				case COLUMN_FILENAME: {
					if (a->dir->getType() == DirectoryListing::Directory::TYPE_ADLS && b->dir->getType() != DirectoryListing::Directory::TYPE_ADLS) return -1;
					if (a->dir->getType() != DirectoryListing::Directory::TYPE_ADLS && b->dir->getType() == DirectoryListing::Directory::TYPE_ADLS) return 1;
					return Util::DefaultSort(a->getNameW().c_str(), b->getNameW().c_str());
				}
				case COLUMN_TYPE: {
					return Util::directoryContentSort(a->dir->getContentInfo(), b->dir->getContentInfo());
				}
				default: return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str());
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
			case COLUMN_FILENAME: return Util::DefaultSort(a->getNameW().c_str(), b->getNameW().c_str());
			default: return Util::DefaultSort(a->getText(col).c_str(), b->getText(col).c_str());
		}
	}
}

DirectoryListingFrame::ItemInfo::ItemInfo(const DirectoryListing::File::Ptr& f) : type(FILE), file(f), name(Text::toT(f->getName())) { 
	//dcdebug("ItemInfo (file) %s was created\n", f->getName().c_str());
}

DirectoryListingFrame::ItemInfo::ItemInfo(const DirectoryListing::Directory::Ptr& d) : type(DIRECTORY), dir(d), name(Text::toT(d->getName())) {
	//dcdebug("ItemInfo (directory) %s was created\n", d->getName().c_str());
}

DirectoryListingFrame::ItemInfo::~ItemInfo() {
	//dcdebug("ItemInfo %s was deleted\n", getName().c_str());

	// The member destructor is not called automatically in an union
	if (type == FILE) {
		file.~shared_ptr();
	} else {
		dir.~shared_ptr();
	}
}

const tstring DirectoryListingFrame::ItemInfo::getText(uint8_t col) const {
	switch(col) {
		case COLUMN_FILENAME: return name;
		case COLUMN_TYPE:
			if (type == FILE) {
				return WinUtil::formatFileType(file->getName());
			} else {
				return Text::toT(Util::formatDirectoryContent(dir->getContentInfoRecursive(true)));
			}
		case COLUMN_EXACTSIZE: return type == DIRECTORY ? Util::formatExactSizeW(dir->getTotalSize(true)) : Util::formatExactSizeW(file->getSize());
		case COLUMN_SIZE: return type == DIRECTORY ? Util::formatBytesW(dir->getTotalSize(true)) : Util::formatBytesW(file->getSize());
		case COLUMN_DATE: return Util::getDateTimeW(type == DIRECTORY ? dir->getRemoteDate() : file->getRemoteDate());
		default: return Text::toT(getTextNormal(col));
	}
}

const string DirectoryListingFrame::ItemInfo::getTextNormal(uint8_t col) const {
	switch (col) {
	case COLUMN_FILENAME: return type == DIRECTORY ? dir->getName() : file->getName();
	case COLUMN_TYPE:
		if (type == FILE) {
			return Text::fromT(WinUtil::formatFileType(file->getName()));
		} else {
			return Util::emptyString;
		}
	case COLUMN_TTH: return type == FILE ? file->getTTH().toBase32() : Util::emptyString;
	case COLUMN_EXACTSIZE: return type == DIRECTORY ? Util::formatExactSize(dir->getTotalSize(true)) : Util::formatExactSize(file->getSize());
	case COLUMN_SIZE: return  type == DIRECTORY ? Util::formatBytes(dir->getTotalSize(true)) : Util::formatBytes(file->getSize());
	case COLUMN_DATE: return Util::getDateTime(type == DIRECTORY ? dir->getRemoteDate() : file->getRemoteDate());
	default: return Util::emptyString;
	}
}

int DirectoryListingFrame::ItemInfo::getImageIndex() const {
	/*if(type == DIRECTORY)
		return DirectoryListingFrame::getIconIndex(dir);
	else*/
	return ResourceLoader::getIconIndex(getText(COLUMN_FILENAME));
}

int DirectoryListingFrame::getIconIndex(const ItemInfo* ii) const {
	if (ii->dir->getLoading())
		return ResourceLoader::DIR_LOADING;
	
	if (ii->dir->getType() == DirectoryListing::Directory::TYPE_NORMAL || ii->dir->getType() == DirectoryListing::Directory::TYPE_ADLS || dl->getIsOwnList())
		return ResourceLoader::DIR_NORMAL;

	return ResourceLoader::DIR_INCOMPLETE;
}

void DirectoryListingFrame::closeAll(){
	for (auto f : frames | map_values)
		f->PostMessage(WM_CLOSE, 0, 0);
}

LRESULT DirectoryListingFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!dl->getClosing()) {
		ctrlFiles.list.SetRedraw(FALSE);

		updateStatus(TSTRING(CLOSING_WAIT));

		DirectoryListingManager::getInstance()->removeList(dl->getUser());
		return 0;
	} else {
		SettingsManager::getInstance()->removeListener(this);
		frames.erase(m_hWnd);

		ctrlFiles.list.saveHeaderOrder(SettingsManager::DIRECTORYLISTINGFRAME_ORDER, SettingsManager::DIRECTORYLISTINGFRAME_WIDTHS,
			SettingsManager::DIRECTORYLISTINGFRAME_VISIBLE);

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
	if (!dl->getPartialList() && !dl->isMyCID()) {
		tabMenu.AppendMenu(MF_SEPARATOR);
		tabMenu.AppendMenu(MF_STRING, IDC_RELOAD, CTSTRING(RELOAD));
	}
	tabMenu.AppendMenu(MF_SEPARATOR);
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));

	tabMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt);
	return TRUE;
}

LRESULT DirectoryListingFrame::onReloadList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
	convertToFull();
	return 0;
}

LRESULT DirectoryListingFrame::onReloadDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/) {
	handleReloadPartial();
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
	if (ctrlFiles.list.GetFont() != WinUtil::listViewFont){
		ctrlFiles.list.SetFont(WinUtil::listViewFont);
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
	callAsync([=] { ctrlStatus.SetText(STATUS_TEXT, aMsg.c_str()); });
}

LRESULT DirectoryListingFrame::onCustomDrawList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {

	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;
	switch (cd->nmcd.dwDrawStage) {

	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT: {
		if (windowState != STATE_ENABLED) {
			cd->clrTextBk = WinUtil::bgColor;
			cd->clrText = GetSysColor(COLOR_3DDKSHADOW);
			return CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;
		}

		ItemInfo *ii = reinterpret_cast<ItemInfo*>(cd->nmcd.lItemlParam);
		//dupe colors have higher priority than highlights.
		if (SETTING(DUPES_IN_FILELIST) && !dl->getIsOwnList() && ii) {
			auto c = WinUtil::getDupeColors(ii->type == ItemInfo::FILE ? ii->file->getDupe() : ii->dir->getDupe());
			cd->clrText = c.first;
			cd->clrTextBk = c.second;
		}

		//has dupe color = no matching
		if (SETTING(USE_HIGHLIGHT) && !dl->getIsOwnList() && (ii->type == ItemInfo::DIRECTORY && ii->dir->getDupe() == DUPE_NONE)) {

			ColorList *cList = HighlightManager::getInstance()->getList();
			for (ColorIter i = cList->begin(); i != cList->end(); ++i) {
				ColorSettings* cs = &(*i);
				if (cs->getContext() == HighlightManager::CONTEXT_FILELIST) {
					if (cs->usingRegexp()) {
						try {
							//have to have $Re:
							if (boost::regex_search(Text::toT(ii->dir->getName()), cs->regexp)) {
								if (cs->getHasFgColor()) { cd->clrText = cs->getFgColor(); }
								if (cs->getHasBgColor()) { cd->clrTextBk = cs->getBgColor(); }
								break;
							}
						} catch (...) {}
					} else {
						if (Wildcard::patternMatch(Text::utf8ToAcp(ii->dir->getName()), Text::utf8ToAcp(Text::fromT(cs->getMatch())), '|')) {
							if (cs->getHasFgColor()) { cd->clrText = cs->getFgColor(); }
							if (cs->getHasBgColor()) { cd->clrTextBk = cs->getBgColor(); }
							break;
						}
					}
				}
			}
		}

		return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
	}

	case CDDS_ITEMPOSTPAINT: {
		if (windowState != STATE_ENABLED && ctrlFiles.list.findColumn(cd->iSubItem) == COLUMN_FILENAME) {
			LVITEM rItem;
			int nItem = static_cast<int>(cd->nmcd.dwItemSpec);

			ZeroMemory(&rItem, sizeof(LVITEM));
			rItem.mask = LVIF_IMAGE | LVIF_STATE;
			rItem.iItem = nItem;
			ctrlFiles.list.GetItem(&rItem);

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
		if (SETTING(DUPES_IN_FILELIST) && !dl->getIsOwnList() && !(cd->nmcd.uItemState & CDIS_SELECTED)) {
			auto dir = (reinterpret_cast<ItemInfo*>(cd->nmcd.lItemlParam))->dir;
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

void DirectoryListingFrame::onComboSelChanged(bool aUserChange) {
	if (dl->getIsOwnList()) {
		auto token = ShareManager::getInstance()->getProfiles()[selCombo.GetCurSel()]->getToken();
		dl->addShareProfileChangeTask(token);
	} else {
		auto& newHub = hubs[selCombo.GetCurSel()];
		if (aUserChange) {
			auto p = boost::find_if(hubs, [this](const User::UserHubInfo& uhi) { return uhi.hubUrl == dl->getHubUrl(); });
			if (p != hubs.end()) {
				auto& oldHub = *p;
				auto diff = newHub.shared > 0 ? abs(static_cast<double>(oldHub.shared) / static_cast<double>(newHub.shared)) : oldHub.shared;
				if (diff < 0.95 || diff > 1.05) {
					auto msg = TSTRING_F(LIST_SIZE_DIFF_NOTE, Text::toT(newHub.hubName) % Util::formatBytesW(newHub.shared) % Text::toT(oldHub.hubName) % Util::formatBytesW(oldHub.shared));
					if (WinUtil::showQuestionBox(msg, MB_ICONQUESTION)) {
						dl->addAsyncTask([=] {
							try {
								QueueManager::getInstance()->addList(HintedUser(dl->getUser(), newHub.hubUrl), (dl->getPartialList() ? QueueItem::FLAG_PARTIAL_LIST : 0) | QueueItem::FLAG_CLIENT_VIEW, ADC_ROOT_STR);
							} catch (...) {}
						});
					}
				}
			}
		}

		dl->addHubUrlChangeTask(newHub.hubUrl);
	}
}

void DirectoryListingFrame::updateSelCombo(bool init) {
	if (dl->getIsOwnList()) {
		auto profiles = ShareManager::getInstance()->getProfiles();
		while (selCombo.GetCount()) {
			selCombo.DeleteString(0);
		}

		for (const auto& p : profiles) {
			if (p->getToken() != SP_HIDDEN) { 
				auto idx = selCombo.AddString(Text::toT(p->getPlainName()).c_str());
				if (p->getToken() == dl->getShareProfile()) {
					selCombo.SetCurSel(idx);
				}
			}
		}

		if (selCombo.GetCurSel() == -1) {
			// the profile was not found
			selCombo.SetCurSel(0);
			onComboSelChanged(false);
		}
	} else if (!dl->isMyCID()) {

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

		hubs = ClientManager::getInstance()->getUserInfoList(dl->getUser());
		while (selCombo.GetCount()) {
			selCombo.DeleteString(0);
		}

		//General things
		if (hubsInfoNew.second) {
			//the user is online

			onlineHubNames = WinUtil::getHubNames(dl->getHintedUser());
			onlineNicks = WinUtil::getNicks(dl->getHintedUser());
			setDisconnected(false);

		} else {
			setDisconnected(true);
		}

		//ADC related changes
		if (hubsInfoNew.second && !dl->getUser()->isNMDC() && !hubs.empty()) {
			if (!(selCombo.GetStyle() & WS_VISIBLE)) {
				showSelCombo(true, init);
			}

			//sort the list by share size, try to avoid changing to 0 share hubs...
			sort(hubs.begin(), hubs.end(), [](const User::UserHubInfo& a, const User::UserHubInfo& b) { return a.shared > b.shared; });
			for (const auto& hub : hubs) {
				auto idx = selCombo.AddString(Text::toT(("(" + Util::formatBytes(hub.shared) + ") " + hub.hubName)).c_str());
				if (hub.hubUrl == hint) {
					selCombo.SetCurSel(idx);
				}
			}

			if (selCombo.GetCurSel() == -1) {
				//the hub was not found
				selCombo.SetCurSel(0);
				onComboSelChanged(false);
			}
		} else {
			showSelCombo(false, init);
		}

		online = hubsInfoNew.second;
	} else {
		showSelCombo(false, init);
	}

	if (onlineNicks.empty())
		onlineNicks = Text::toT(dl->getNick(false));

	SetWindowText((onlineNicks + (onlineHubNames.empty() ? Util::emptyStringT : _T(" - ") + onlineHubNames)).c_str());
}

void DirectoryListingFrame::showSelCombo(bool show, bool init) {
	selCombo.ShowWindow(show);
	selCombo.EnableWindow(show);

	if (!init)
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