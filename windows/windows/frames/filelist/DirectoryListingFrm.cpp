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

#include <windows/stdafx.h>

#include <windows/Resource.h>

#include <windows/frames/filelist/DirectoryListingFrm.h>
#include <windows/frames/filelist/DirectoryListingDlg.h>

#include <windows/components/ListFilter.h>
#include <windows/MainFrm.h>
#include <windows/ResourceLoader.h>
#include <windows/components/ShellContextMenu.h>
#include <windows/frames/text/TextFrame.h>
#include <windows/util/Wildcards.h>
#include <windows/util/WinUtil.h>
#include <windows/util/ActionUtil.h>
#include <windows/util/FormatUtil.h>

#include <airdcpp/hub/ClientManager.h>
#include <airdcpp/filelist/DirectoryListingManager.h>
#include <airdcpp/util/DupeUtil.h>
#include <airdcpp/favorites/FavoriteUserManager.h>
#include <airdcpp/core/io/File.h>
#include <airdcpp/util/PathUtil.h>
#include <airdcpp/queue/QueueManager.h>
#include <airdcpp/search/SearchQuery.h>
#include <airdcpp/share/profiles/ShareProfile.h>
#include <airdcpp/util/text/StringTokenizer.h>
#include <airdcpp/user/User.h>
#include <airdcpp/util/ValueGenerator.h>
#include <airdcpp/viewed_files/ViewFileManager.h>

#include <airdcpp/modules/ADLSearch.h>
#include <airdcpp/modules/HighlightManager.h>

#include <web-server/ContextMenuManager.h>
#include <web-server/WebServerManager.h>


namespace wingui {
DirectoryListingFrame::FrameMap DirectoryListingFrame::frames;
int DirectoryListingFrame::columnIndexes[] = { COLUMN_FILENAME, COLUMN_TYPE, COLUMN_EXACTSIZE, COLUMN_SIZE, COLUMN_TTH, COLUMN_DATE };
int DirectoryListingFrame::columnSizes[] = { 300, 60, 100, 100, 200, 130 };

static ResourceManager::Strings columnNames[] = { ResourceManager::FILE, ResourceManager::TYPE, ResourceManager::EXACT_SIZE, ResourceManager::SIZE, ResourceManager::TTH_ROOT, ResourceManager::DATE };
static SettingsManager::BoolSetting filterSettings [] = { SettingsManager::FILTER_FL_SHARED, SettingsManager::FILTER_FL_QUEUED, SettingsManager::FILTER_FL_INVERSED, SettingsManager::FILTER_FL_TOP, SettingsManager::FILTER_FL_PARTIAL_DUPES, SettingsManager::FILTER_FL_RESET_CHANGE };

static ColumnType columnTypes [] = { COLUMN_TEXT, COLUMN_TEXT, COLUMN_SIZE, COLUMN_SIZE, COLUMN_TEXT, COLUMN_TIME };

string DirectoryListingFrame::id = "FileList";


// Open own filelist
void DirectoryListingFrame::openWindow(ProfileToken aProfile, const string& aDir) noexcept {
	auto me = HintedUser(ClientManager::getInstance()->getMe(), Util::emptyString);

	auto list = DirectoryListingManager::getInstance()->findList(me);
	if (list) {
		activate(list);
		if (*list->getShareProfile() != aProfile) {
			list->addShareProfileChangeTask(aProfile);
		}

		return;
	}

	DirectoryListingManager::getInstance()->openOwnList(aProfile, aDir);
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
			list->addDirectoryChangeTask(aInitialDir, DirectoryListing::DirectoryLoadType::CHANGE_NORMAL);
		}

		activate(list);
		return;
	}

	MainFrame::getMainFrame()->addThreadedTask([=] {
		try {
			auto listData = FilelistAddData(aUser, MainFrame::getMainFrame(), aInitialDir);
			DirectoryListingManager::getInstance()->openRemoteFileListHookedThrow(listData, QueueItem::FLAG_CLIENT_VIEW | aFlags);
		} catch (const Exception& e) {
			MainFrame::getMainFrame()->ShowPopup(Text::toT(e.getError()), _T(""), NIIF_INFO | NIIF_NOSOUND, true);
		}
	});
}

DirectoryListingFrame* DirectoryListingFrame::findFrame(const UserPtr& aUser) noexcept {
	auto f = ranges::find_if(frames | views::values, [&](const DirectoryListingFrame* h) { return aUser == h->dl->getUser(); }).base();
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
		aList->addPartialListLoadTask(aXML, aDir);
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
		params["dir"] = f->second->getCurrentListPath();
		params["file"] = dl->getFileName();
		params["partial"] = Util::toString(dl->getPartialList());

		auto profileToken = dl->getShareProfile();
		if (profileToken) {
			params["profileToken"] = Util::toString(*profileToken);
		}

		if (!dl->getPartialList())
			QueueManager::getInstance()->noDeleteFileList(dl->getFileName());

		FavoriteUserManager::getInstance()->addSavedUser(dl->getUser());
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
			dir = PathUtil::toAdcFile(dir);
		}

		string file = params["file"];
		string url = params["url"];
		if (u == ClientManager::getInstance()->getMe()) {
			auto token = Util::toInt(params["profileToken"]);
			DirectoryListingManager::getInstance()->openOwnList(token, dir);
		} else if (!file.empty() || partial) {
					
			auto dl = DirectoryListingManager::getInstance()->openLocalFileList(HintedUser(u, url), file, dir, partial);
			if (partial) {
				dl->addDirectoryChangeTask(dir, DirectoryListing::DirectoryLoadType::CHANGE_NORMAL, true);
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
	UserInfoBaseHandler(true, false),
	treeContainer(WC_TREEVIEW, this, CONTROL_MESSAGE_MAP),
	listContainer(WC_LISTVIEW, this, CONTROL_MESSAGE_MAP),
	ctrlTree(this), 
	ctrlFiles(this, COLUMN_LAST, [this] { callAsync([this] { filterList(); }); }, filterSettings, COLUMN_LAST), 
	browserBar(this, [this](const string& a, bool b) { handleHistoryClick(a, b); }, [this] { up(); }), dl(aList), 
	selComboContainer(WC_COMBOBOX, this, COMBO_SEL_MAP),
	matchADL(SETTING(USE_ADLS) && !aList->getPartialList())
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

void DirectoryListingFrame::changeWindowState(bool aEnabled, bool redraw) {
	ctrlToolbar.EnableButton(IDC_MATCH_QUEUE, aEnabled && !dl->isMyCID());
	ctrlToolbar.EnableButton(IDC_MATCH_ADL, aEnabled);
	ctrlToolbar.EnableButton(IDC_FIND, aEnabled /* && !dl->getIsOwnList()*/);
	ctrlToolbar.EnableButton(IDC_NEXT, aEnabled && search ? TRUE : FALSE);
	ctrlToolbar.EnableButton(IDC_PREV, aEnabled && search ? TRUE : FALSE);
	ctrlToolbar.EnableButton(IDC_FILELIST_DIFF, aEnabled && dl->getPartialList() && !dl->getIsOwnList() ? false : aEnabled);

	browserBar.changeWindowState(aEnabled);
	ctrlFiles.changeFilterState(aEnabled);
	selCombo.EnableWindow(aEnabled);

	if (aEnabled) {
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
	auto curDir = dl->findDirectoryUnsafe(aPath);
	if (!curDir) {
		dcassert(0);
		return;
	}

	auto& iic = itemInfos[aPath];
	if (!iic) {
		iic = make_unique<ItemInfoCache>();
	}

	for (auto& d : curDir->directories | views::values) {
		iic->directories.emplace(d);
	}

	for (auto& f : curDir->files) {
		iic->files.emplace(f);
	}

	// Check that this directory exists in all parents
	if (aPath != ADC_ROOT_STR) {
		auto parent = PathUtil::getAdcParentDir(aPath);
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

void DirectoryListingFrame::on(DirectoryListingListener::LoadingFinished, int64_t aStart, const string& aDir, uint8_t aType) noexcept {
	if (matchADL) {
		ctrlStatus.SetText(0, CTSTRING(MATCHING_ADL));
		ADLSearchManager::getInstance()->matchListing(*dl.get());
	}

	onLoadingFinished(aStart, aDir, aType);

	// Get ownership of all item infos so that they won't be deleted before we finish loading
	/*vector<unique_ptr<ItemInfoCache>> removedInfos;
	for (auto i = itemInfos.begin(); i != itemInfos.end();) {
		if (PathUtil::isParentOrExactAdc(aDir, i->first)) {
			removedInfos.push_back(std::move(i->second));
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
	});*/
}

bool DirectoryListingFrame::checkSearchDirectoryLoading(const string& aPath) noexcept {
	auto searching = search && search->isCurrentSearchPath(aPath);
	if (searching) {
		ctrlFiles.filter.clear();
	} else if (search) {
		search.reset();
	}

	return searching;
}

void DirectoryListingFrame::onSearchDirectoryLoaded() noexcept {
	findSearchHit(true);
	updateStatus(TSTRING_F(X_RESULTS_FOUND, search->getResultCount()));
}

void DirectoryListingFrame::onLoadingFinished(int64_t aStart, const string& aPath, uint8_t aType) {

	// Get ownership of all item infos so that they won't be deleted before we finish loading
	vector<unique_ptr<ItemInfoCache>> removedInfos;
	for (auto i = itemInfos.begin(); i != itemInfos.end();) {
		if (PathUtil::isParentOrExactAdc(aPath, i->first)) {
			removedInfos.push_back(std::move(i->second));
			i = itemInfos.erase(i);
		} else {
			i++;
		}
	}

	// Update the current location in case of reload
	updateItemCache(getCurrentListPath());

	callAsync([=] {
		if (getActive()) {
			dl->setRead();
		}

		if (!dl->getPartialList())
			updateStatus(CTSTRING(UPDATING_VIEW));

		auto searching = checkSearchDirectoryLoading(aPath);

		refreshTree(aPath, static_cast<DirectoryListing::DirectoryLoadType>(aType) != DirectoryListing::DirectoryLoadType::LOAD_CONTENT);

		changeWindowState(true);
		if (!searching) {
			int64_t loadTime = (GET_TICK() - aStart) / 1000;
			string msg;
			if (dl->getPartialList()) {
				if (aPath == ADC_ROOT_STR) {
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
			onSearchDirectoryLoaded();
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
	});
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

void DirectoryListingFrame::onSearchFailed(bool aTimedOut) noexcept {
	callAsync([=] {
		tstring msg;
		if (aTimedOut) {
			msg = dl->supportsASCH() ? TSTRING(SEARCH_TIMED_OUT) : TSTRING(NO_RESULTS_SPECIFIED_TIME);
		}
		else {
			msg = TSTRING(NO_RESULTS_FOUND);
		}
		updateStatus(msg);
	});

	search.reset();
	changeWindowState(true);
}

void DirectoryListingFrame::on(DirectoryListingListener::ChangeDirectory, const string& aNewPath, uint8_t /*aChangeType*/) noexcept {
	//dcdebug("DirectoryListingListener::ChangeDirectory %s\n", aDir.c_str());

	callAsync([=] {
		auto searching = checkSearchDirectoryLoading(aNewPath);

		auto loadedTreeItem = selectItem(aNewPath);
		if (loadedTreeItem) {
			auto loadedDirectory = ((ItemInfo*)ctrlTree.GetItemData(loadedTreeItem))->dir;
			updateItems(loadedDirectory);
		}

		if (searching) {
			onSearchDirectoryLoaded();
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

void DirectoryListingFrame::createColumns() noexcept {
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
	ctrlTree.SetFont(WinUtil::listViewFont);

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

	int desclen = WinUtil::getStatusTextWidth(getComboDesc(), ctrlStatus.m_hWnd);
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
			auto listData = FilelistAddData(dl->getHintedUser(), this, ADC_ROOT_STR);
			QueueManager::getInstance()->addListHooked(listData, QueueItem::FLAG_CLIENT_VIEW);
		} catch (const Exception& e) {
			ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
		}
	});
}

DirectoryListingFrame::TreeType::ChildrenState DirectoryListingFrame::getChildrenState(const ItemInfo* ii) const noexcept {
	auto d = ii->dir;
	if (!d->directories.empty())
		return !d->isComplete() ? TreeType::ChildrenState::CHILDREN_PART_PENDING : TreeType::ChildrenState::CHILDREN_CREATED;

	if (d->getType() == DirectoryListing::Directory::TYPE_INCOMPLETE_CHILD)
		return TreeType::ChildrenState::CHILDREN_ALL_PENDING;

	//if (d->getLoading() && d->getType() == )
	//	return ChildrenState::CHILDREN_LOADING;

	return TreeType::ChildrenState::NO_CHILDREN;
}

void DirectoryListingFrame::expandDir(ItemInfo* ii, bool collapsing) noexcept {
	if (changeType != CHANGE_TREE_DOUBLE)
		changeType = collapsing ? CHANGE_TREE_COLLAPSE : CHANGE_TREE_EXPAND;

	if (collapsing || !ii->dir->isComplete()) {
		loadPath(ii, DirectoryListing::DirectoryLoadType::LOAD_CONTENT);
	} /*else {
		dcdebug("DirectoryListingFrame::expandDir, skip changeDir %s\n", ii->dir->getPath().c_str());
	}*/
}

void DirectoryListingFrame::insertTreeItems(const string& aPath, HTREEITEM aParent) noexcept {
	auto p = itemInfos.find(aPath);
	//dcassert(p != itemInfos.end());
	if (p != itemInfos.end()) {
		for (const auto& d : p->second->directories) {
			ctrlTree.insertItem(&d, aParent, d.dir->isVirtual());
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
	browserBar.addHistory(dl->getRoot()->getAdcPathUnsafe());
	dcassert(treeRoot); 
}

const string DirectoryListingFrame::getCurrentListPath() const noexcept {
	/*auto ii = ctrlTree.getSelectedItemData();
	if (!ii) {
		return ADC_ROOT_STR;
	}

	return ii->dir->getAdcPath();*/

	const auto ret = dl->getCurrentLocationInfo().directory ? dl->getCurrentLocationInfo().directory->getAdcPathUnsafe() : ADC_ROOT_STR;
	return ret;
}

void DirectoryListingFrame::refreshTree(const string& aLoadedPath, bool aSelectDir) {
	ctrlTree.SetRedraw(FALSE);

	// Check the root children state
	auto initialChange = !ctrlTree.hasChildren(treeRoot);
	if (initialChange && !dl->getRoot()->directories.empty()) {
		ctrlTree.setHasChildren(treeRoot, true);
	}

	// Get the item for our newly loaded directory
	auto loadedTreeItem = ctrlTree.findItemByPath(treeRoot, Text::toT(aLoadedPath));
	dcassert(loadedTreeItem);
	if (!loadedTreeItem) {
		loadedTreeItem = treeRoot;
	}

	auto loadedDirectory = ((ItemInfo*)ctrlTree.GetItemData(loadedTreeItem))->dir;

	{
		auto alwaysExpand = initialChange || changeType == CHANGE_TREE_EXPAND || changeType == CHANGE_TREE_DOUBLE;
		auto refreshItemChildState = loadedTreeItem != treeRoot;
		ctrlTree.refreshItem(loadedTreeItem, refreshItemChildState, alwaysExpand);
	}

	if (!aSelectDir && getCurrentListPath() == PathUtil::getAdcParentDir(aLoadedPath)) {
		// Find the loaded directory and set it as complete
		int j = ctrlFiles.list.GetItemCount();        
		for(int i = 0; i < j; i++) {
			const ItemInfo* ii = ctrlFiles.list.getItemData(i);
			if (ii->type == ii->DIRECTORY && ii->getAdcPath() == aLoadedPath) {
				ctrlFiles.list.SetItem(i, 0, LVIF_IMAGE, NULL, ResourceLoader::DIR_NORMAL, 0, 0, NULL);
				ctrlFiles.list.updateItem(i);
				updateStatus();
				break;
			}
		}
	} else if (aSelectDir || PathUtil::isParentOrExactAdc(aLoadedPath, getCurrentListPath())) {
		// insert the new items
		ctrlTree.SelectItem(nullptr);

		// tweak to insert the items in the current thread
		if (aSelectDir) {
			updateItems(loadedDirectory);
			selectItem(loadedDirectory->getAdcPathUnsafe());
		} else {
			// Subdirectory of the loaded directory is selected
			auto currentTreeItem = ctrlTree.findItemByPath(treeRoot, Text::toT(getCurrentListPath()));
			dcassert(currentTreeItem);

			auto currentDirectory = ((ItemInfo*)ctrlTree.GetItemData(currentTreeItem))->dir;

			updateItems(currentDirectory);
			selectItem(currentDirectory->getAdcPathUnsafe());
		}
	}

	ctrlTree.SetRedraw(TRUE);

#ifdef _DEBUG
	validateTreeDebug();
#endif
}

#ifdef _DEBUG
void DirectoryListingFrame::validateTreeDebug() {
	ctrlTree.getTreeItemPaths(treeRoot);

	StringList paths;
	for (const auto& cacheItem : itemInfos | views::values) {
		for (const auto& d : cacheItem->directories) {
			paths.push_back(d.getAdcPath());
		}

		for (const auto& f : cacheItem->files) {
			paths.push_back(f.getAdcPath());
		}
	}

}
#endif

LRESULT DirectoryListingFrame::onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
	changeType = CHANGE_LIST;
	statusDirty = true;
	return 0;
}

void DirectoryListingFrame::findSearchHit(bool aNewDir /*false*/) {
	if (!search) {
		return;
	}

	const auto& searchQuery = search->curSearch;
	if (!searchQuery)
		return;

	{
		auto dir = ctrlTree.getSelectedItemData()->dir;
		if (!dir->isComplete()) {
			return;
		}

		if (!search->isCurrentSearchPath(dir->getAdcPathUnsafe())) {
			//auto searchPath = search->getCurrentSearchPath();
			//if (!searchPath.empty()) {
			//	selectItem(searchPath);
			//}
			return;
		}
	}

	// set the starting position for the search
	if (aNewDir) {
		searchPos = gotoPrev ? ctrlFiles.list.GetItemCount()-1 : 0;
	} else if (gotoPrev) {
		searchPos--;
	} else {
		searchPos++;
	}

	bool found = false;

	// Check file names in list pane
	while(searchPos < ctrlFiles.list.GetItemCount() && searchPos >= 0) {
		const ItemInfo* ii = ctrlFiles.list.getItemData(searchPos);
		if(ii->type == ItemInfo::FILE) {
			if(searchQuery->matchesFile(ii->file->getName(), ii->file->getSize(), ii->file->getRemoteDate(), ii->file->getTTH())) {
				found = true;
				break;
			}
		} else if(ii->type == ItemInfo::DIRECTORY && searchQuery->matchesDirectory(ii->dir->getName())) {
			if (searchQuery->matchesSize(ii->dir->getTotalSize(false))) {
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
		dcassert(!aNewDir);
		if (!search->nextResult(gotoPrev)) {
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

	auto s = make_shared<Search>(Priority::HIGH, Util::toString(ValueGenerator::rand()));

	s->query = dlg.searchStr;

	s->setLegacySize(dlg.size, dlg.sizeMode);

	s->fileType = dlg.fileType;
	s->exts = dlg.extList;

	s->returnParents = true;
	s->matchType = Search::MATCH_NAME_PARTIAL;
	s->maxResults = 20;
	s->requireReply = true;

	if (dlg.useCurDir) {
		s->path = getCurrentListPath();
	}

	updateStatus(TSTRING(SEARCHING));
	changeWindowState(false);

	search = make_unique<DirectoryListingSearch>(dl, std::bind_front(&DirectoryListingFrame::onSearchFailed, this));
	search->addSearchTask(s);
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

size_t DirectoryListingFrame::getTotalListItemCount() const noexcept {
	auto ii = ctrlTree.getSelectedItemData();
	if (!ii) {
		return 0;
	}

	auto dir = ii->dir;
	return dir->directories.size() + dir->files.size();
}


void DirectoryListingFrame::updateStatus() noexcept {
	auto ii = ctrlTree.getSelectedItemData();
	if (!ii) {
		return;
	}

	updateStatus(ii->dir);
}

void DirectoryListingFrame::updateStatus(const DirectoryListing::DirectoryPtr& aCurrentDir) noexcept {
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
		tmp += WinUtil::toStringW(aSelectedCount);
	}
	else if (aDisplayCount != aTotalCount) {
		tmp += WinUtil::toStringW(aDisplayCount) + _T("/") + WinUtil::toStringW(aTotalCount);
	}
	else {
		tmp += WinUtil::toStringW(aDisplayCount);
	}
	bool u = false;

	int w = WinUtil::getStatusTextWidth(tmp, ctrlStatus.m_hWnd);
	if (statusSizes[STATUS_SELECTED_FILES] < w) {
		statusSizes[STATUS_SELECTED_FILES] = w;
		u = true;
	}
	ctrlStatus.SetText(STATUS_SELECTED_FILES, tmp.c_str());

	tmp = TSTRING(SIZE) + _T(": ") + Util::formatBytesW(aTotalSize);
	w = WinUtil::getStatusTextWidth(tmp, ctrlStatus.m_hWnd);
	if (statusSizes[STATUS_SELECTED_SIZE] < w) {
		statusSizes[STATUS_SELECTED_SIZE] = w;
		u = true;
	}
	ctrlStatus.SetText(STATUS_SELECTED_SIZE, tmp.c_str());

	tmp = aUpdateDate > 0 ? TSTRING_F(UPDATED_ON_X, FormatUtil::formatDateTimeW(aUpdateDate)) : Util::emptyStringT;
	w = WinUtil::getStatusTextWidth(tmp, ctrlStatus.m_hWnd);
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
			totalSize = dl->getTotalListSizeUnsafe();
			totalFiles = dl->getTotalFileCountUnsafe();
		}

		callAsync([=] {
			tstring tmp = TSTRING_F(TOTAL_SIZE, Util::formatBytesW(totalSize));
			statusSizes[STATUS_TOTAL_SIZE] = WinUtil::getStatusTextWidth(tmp, m_hWnd);
			ctrlStatus.SetText(STATUS_TOTAL_SIZE, tmp.c_str());

			tmp = TSTRING_F(TOTAL_FILES, totalFiles);
			statusSizes[STATUS_TOTAL_FILES] = WinUtil::getStatusTextWidth(tmp, m_hWnd);
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
		auto selectedItemInfo = (ItemInfo*)p->itemNew.lParam;

		// Don't cause an infinite loop when changing directories in the GUI faster than the
		// list thread is able to handle (= don't send another changeDir for previously queued changes)
		if (getCurrentListPath() != selectedItemInfo->getAdcPath() && dl->getCurrentLocationInfo().directory != selectedItemInfo->dir) {
			if (changeType != CHANGE_HISTORY)
				browserBar.addHistory(selectedItemInfo->getAdcPath());

			// curPath = ii->getAdcPath();
			loadPath(selectedItemInfo, DirectoryListing::DirectoryLoadType::CHANGE_NORMAL);
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

	insertItems(getCurrentListPath(), nullopt);

	ctrlFiles.list.SetRedraw(TRUE);
	updating = false;
	updateStatus();
}

void DirectoryListingFrame::insertItems(const string& aPath, const optional<string>& aSelectedName) {
	ctrlFiles.list.DeleteAllItems();

	int curPos = 0;
	int selectedPos = -1;

	dcassert(itemInfos.find(aPath) != itemInfos.end());
	const auto& dirs = itemInfos[aPath]->directories;
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
				if (aSelectedName && compare(*aSelectedName, (*i).getName()) == 0) {
					selectedPos = curPos;
				}

				ctrlFiles.list.insertItem(curPos++, &(*i), (*i).type == ItemInfo::DIRECTORY ? getIconIndex(&(*i)) : (*i).getImageIndex());
			}
		}
	};

	doInsert(dirs);
	const auto& files = itemInfos[aPath]->files;
	i = files.crbegin();
	doInsert(files);

	if (selectedPos != -1) {
		ctrlFiles.list.SelectItem(selectedPos);
	}

	if (ctrlFiles.list.getSortColumn() != COLUMN_FILENAME) {
		ctrlFiles.list.resort();
	}
}

void DirectoryListingFrame::updateItems(const DirectoryListing::DirectoryPtr& d) {
	ctrlFiles.list.SetRedraw(FALSE);
	updating = true;
	if (!itemInfos.contains(d->getAdcPathUnsafe())) {
		updateItemCache(d->getAdcPathUnsafe());
	}

	optional<string> selectedName;
	if (changeType == CHANGE_HISTORY) {
		auto historyPath = browserBar.getCurSel();
		if (!historyPath.empty() && (!d->getParent() || PathUtil::getAdcParentDir(historyPath) == d->getAdcPathUnsafe())) {
			selectedName = PathUtil::getLastDir(historyPath);
		}

		changeType = CHANGE_LIST; //reset
	}

	ctrlFiles.onListChanged(false);
	insertItems(d->getAdcPathUnsafe(), selectedName);

	ctrlFiles.list.SetRedraw(TRUE);
	updating = false;
	updateStatus(d);
}

void DirectoryListingFrame::loadPath(const ItemInfo* ii, DirectoryListing::DirectoryLoadType aType) {
	if (!ii) {
		dcassert(0);
		return;
	}

	auto path = ii->getAdcPath();
	//dcdebug("DirectoryListingFrame::changeDir %s\n", path.c_str());

	dl->addDirectoryChangeTask(path, aType);
}

void DirectoryListingFrame::up() {
	auto t = ctrlTree.GetSelectedItem();
	if (!t) {
		return;
	}

	t = ctrlTree.GetParentItem(t);
	if (!t)
		return;

	auto aPrevPath = getCurrentListPath();
	ctrlTree.SelectItem(t);
	
	callAsync([=] { 
		listViewSelectSubDir(aPrevPath, getCurrentListPath()); 
	});
}



void DirectoryListingFrame::handleHistoryClick(const string& aPath, bool aByHistory) {
	if (aByHistory)
		changeType = CHANGE_HISTORY;

	auto prevPath = getCurrentListPath();
	selectItem(aPath);
	listViewSelectSubDir(prevPath, aPath);
}

void DirectoryListingFrame::listViewSelectSubDir(const string& aSubPath, const string& aParentPath) {
	if (!aSubPath.empty() && PathUtil::isSubAdc(aSubPath, aParentPath)) {
		auto i = ctrlFiles.list.findItem(Text::toT(PathUtil::getAdcLastDir(aSubPath)));
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
			if (dl->getIsOwnList() || DupeUtil::isFinishedDupe(ii->file->getDupe()) || DupeUtil::isShareDupe(ii->file->getDupe())) {
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
	if(kd->wVKey == VK_DOWN || kd->wVKey == VK_UP) {
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
		if (ActionUtil::browseList(file, m_hWnd)) {
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

		matchADL = true;
		convertToFull();
	} else if (!dl->getPartialList() || dl->getIsOwnList()) {
		ctrlStatus.SetText(0, CTSTRING(MATCHING_ADL));

		matchADL = true;
		dl->addFullListTask(ADC_ROOT_STR);
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

HTREEITEM DirectoryListingFrame::selectItem(const string& aPath) {
	HTREEITEM ht = ctrlTree.findItemByPath(treeRoot, Text::toT(aPath));
	if (!ht) {
		// dcassert(0);
		return nullptr;
	}

	if (ctrlTree.GetSelectedItem() != ht) {
		//dcdebug("DirectoryListingFrame::selectItem %s\n", name.c_str());

		if (changeType == CHANGE_LIST)
			ctrlTree.EnsureVisible(ht);
		ctrlTree.SelectItem(ht);
	}

	return ht;
}

LRESULT DirectoryListingFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlFiles.list && ctrlFiles.list.GetSelectedCount() > 0) {
		auto pt = WinUtil::getMenuPosition({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }, ctrlFiles.list);
		if (pt) {
			appendListContextMenu(*pt);
			return TRUE;
		}
	} 
	
	if(reinterpret_cast<HWND>(wParam) == ctrlTree && ctrlTree.GetSelectedItem() != NULL) { 
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

			appendTreeContextMenu(pt, ht);
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

	bool isDupeOrOwnlist = ctrlFiles.list.GetSelectedCount() == 1 && allowOpen(ii);

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

	{
		vector<DirectoryListingItemToken> tokens;
		ctrlFiles.list.forEachSelectedT([&tokens](const ItemInfo* ii) {
			tokens.push_back(ii->getToken());
		});

		EXT_CONTEXT_MENU_ENTITY(fileMenu, FilelistItem, tokens, dl);
	}

	// copy menu
	ListBaseType::MenuItemList customItems{
		{ TSTRING(DIRECTORY), &handleCopyDirectory },
		{ TSTRING(MAGNET_LINK), &handleCopyMagnet },
		{ TSTRING(PATH), &handleCopyPath }
	};
	ctrlFiles.list.appendCopyMenu(fileMenu, customItems);


	if (ctrlFiles.list.GetSelectedCount() == 1 && ii->type == ItemInfo::FILE) {
		fileMenu.InsertSeparatorFirst(Text::toT(PathUtil::getFileName(ii->file->getName())));
		if (ii->file->getOwner()) {
			fileMenu.appendItem(TSTRING(GO_TO_DIRECTORY), [this] { handleGoToDirectory(false); });
		}
	} else {
		if (ii->type == ItemInfo::DIRECTORY && ii->dir->isVirtual() && ii->dir->getParent() != dl->getRoot().get()) {
			fileMenu.appendItem(TSTRING(GO_TO_DIRECTORY), [this] { handleGoToDirectory(false); });
		}
	}

	// User commands
	prepareMenu(fileMenu, UserCommand::CONTEXT_FILELIST, ClientManager::getInstance()->getHubUrls(dl->getHintedUser().user->getCID()));

	// Share stuff
	if (dl->getIsOwnList() && !hasFiles && !ii->isAdl()) {
		fileMenu.appendItem(TSTRING(REFRESH_IN_SHARE), [this] { handleRefreshShare(false); });
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

void DirectoryListingFrame::appendTreeContextMenu(CPoint& pt, const HTREEITEM& aTreeItem) {
	auto ii = ((ItemInfo*)ctrlTree.GetItemData(aTreeItem));
	auto dir = ii->dir;

	ShellMenu directoryMenu;
	directoryMenu.CreatePopupMenu();

	if (!dl->getIsOwnList()) {
		appendDownloadMenu(directoryMenu, DownloadBaseHandler::TYPE_SECONDARY, false, nullopt, dir->getAdcPathUnsafe());
		directoryMenu.appendSeparator();
	}

	directoryMenu.appendSeparator();

	{
		vector<DirectoryListingItemToken> tokens({ ii->getToken() });
		EXT_CONTEXT_MENU_ENTITY(directoryMenu, FilelistItem, tokens, dl);
	}

	if (dir && dir->isVirtual() && dir->getParent() != dl->getRoot().get()) {
		directoryMenu.appendItem(TSTRING(GO_TO_DIRECTORY), [this] { handleGoToDirectory(true); });
	}

	directoryMenu.appendItem(TSTRING(COPY_DIRECTORY), [=] { handleCopyDir(); });
	directoryMenu.appendItem(TSTRING(SEARCH), [=] { handleSearchByName(true, false); });

	if (dl->getUser()->isSet(User::ASCH) && !dl->getIsOwnList()) {
		directoryMenu.appendItem(TSTRING(VIEW_NFO), [this] { handleViewNFO(true); });
	}

	if (dl->getPartialList() && !dir->isVirtual()) {
		directoryMenu.appendSeparator();
		directoryMenu.appendItem(TSTRING(RELOAD), [=] { handleReloadPartial(); });
	}

	if (allowOpen(ii)) {
		StringList paths;
		if (getLocalPaths(paths, true, true)) {
			directoryMenu.appendShellMenu(paths);
			if (DupeUtil::isShareDupe(dir->getDupe())) {
				// shared
				directoryMenu.appendSeparator();
				directoryMenu.appendItem(TSTRING(REFRESH_IN_SHARE), [this] { handleRefreshShare(true); });
			}
		}
	}

	directoryMenu.InsertSeparatorFirst(TSTRING(DIRECTORY));
	directoryMenu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
}

void DirectoryListingFrame::handleItemAction(bool aUsingTree, std::function<void (const ItemInfo* ii)> aF, bool aFirstOnly /*false*/) {
	if (aUsingTree) {
		auto dir = ctrlTree.getSelectedItemData();
		aF(dir);
	} else if (ctrlFiles.list.GetSelectedCount() >= 1) {
		int sel = -1;
		while ((sel = ctrlFiles.list.GetNextItem(sel, LVNI_SELECTED)) != -1) {
			const ItemInfo* ii = ctrlFiles.list.getItemData(sel);
			aF(ii);
			if (aFirstOnly)
				break;
		}
	}
}

void DirectoryListingFrame::handleDownload(const string& aTarget, Priority aPriority, bool aUsingTree) {
	handleItemAction(aUsingTree, [&](const ItemInfo* ii) {
		if (ii->type == ItemInfo::FILE) {
			auto target = aTarget + (!PathUtil::isDirectoryPath(aTarget) ? Util::emptyString : ii->getName());
			decltype(auto) f = ii->file;
			ActionUtil::addFileDownload(target, f->getSize(), f->getTTH(), dl->getDownloadSourceUser(), f->getRemoteDate(), 0, aPriority);
		} else {
			dl->addAsyncTask([
				listPath = ii->getAdcPath(),
				name = ii->getName(),
				this, aTarget, aPriority
			] {
				try {
					auto listData = FilelistAddData(dl->getHintedUser(), this, listPath);
					DirectoryListingManager::getInstance()->addDirectoryDownloadHookedThrow(listData, name, aTarget, aPriority, DirectoryDownload::ErrorMethod::LOG);
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
				ViewFileManager::getInstance()->addLocalFileNotify(ii->file->getTTH(), true, ii->getName());
			} else {
				ActionUtil::openTextFile(ii->file->getName(), ii->file->getSize(), ii->file->getTTH(), dl->getHintedUser(), true);
			}
		}
	});
}

void DirectoryListingFrame::handleSearchByTTH() {
	handleItemAction(false, [this](const ItemInfo* ii) {
		if (ii->type == ItemInfo::FILE) {
			ActionUtil::searchHash(ii->file->getTTH(), ii->file->getName(), ii->file->getSize());
		}
	});
}

void DirectoryListingFrame::handleGoToDirectory(bool usingTree) {
	handleItemAction(usingTree, [this](const ItemInfo* ii) {
		string path;
		if (ii->type == ItemInfo::FILE) {
			if (!ii->file->getOwner())
				return;

			path = PathUtil::getAdcFilePath(ii->getAdcPath());
		} else if (ii->type == ItemInfo::DIRECTORY)	{
			if (!(ii->dir->isVirtual() && ii->dir->getParent() != dl->getRoot().get()))
				return;
			path = ((DirectoryListing::VirtualDirectory*)ii->dir.get())->getFullAdcPath();
		}

		if (!path.empty()) {
			selectItem(path);
		}
	}, true);
}

void DirectoryListingFrame::handleViewNFO(bool usingTree) {
	handleItemAction(usingTree, [this](const ItemInfo* ii) {
		if (ii->type == ItemInfo::DIRECTORY) {
			ActionUtil::findNfo(ii->getAdcPath(), dl->getHintedUser());
		}
	});
}

void DirectoryListingFrame::handleOpenFile() {
	handleItemAction(false, [this](const ItemInfo* ii) {
		if (ii->type != ItemInfo::FILE) {
			return;
		}

		if (allowOpen(ii)) {
			openDupe(ii->file, false);
		} else {
			ActionUtil::openTextFile(ii->file->getName(), ii->file->getSize(), ii->file->getTTH(), dl->getHintedUser(), false);
		}
	});
}
void DirectoryListingFrame::openDupe(const DirectoryListing::DirectoryPtr& d) {
	try {
		StringList paths;
		dl->getLocalPathsUnsafe(d, paths);
		if (!paths.empty()) {
			ActionUtil::openFolder(Text::toT(paths.front()));
		}
	} catch (const ShareException& e) {
		updateStatus(Text::toT(e.getError()));
	}
}

void DirectoryListingFrame::openDupe(const DirectoryListing::FilePtr& f, bool openDir) noexcept {
	try {
		StringList paths;
		dl->getLocalPathsUnsafe(f, paths);

		if (!paths.empty()) {
			auto path = Text::toT(paths.front());
			if (!openDir) {
				ActionUtil::openFile(path);
			}
			else {
				ActionUtil::openFolder(path);
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
		if (ii && !ii->dir->isVirtual()) {
			loadPath(ii, DirectoryListing::DirectoryLoadType::CHANGE_RELOAD);
		}
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

				name = DupeUtil::getAdcReleaseDir(ii->getAdcPath(), true);
				thisSearched = true;
			} else {
				name = ii->getName();
			}
		} else {
			name = ii->getName();
		}

		ActionUtil::search(Text::toT(name), dirsOnly);
	});
}

void DirectoryListingFrame::handleCopyDir() {
	handleItemAction(true, [](const ItemInfo* ii) {
		auto sCopy = Text::toT(ii->dir->getName());
		if (!sCopy.empty()) {
			WinUtil::setClipboard(sCopy);
		}
	});
}

void DirectoryListingFrame::handleRefreshShare(bool aUsingTree) {
	StringList refreshPaths;
	if (getLocalPaths(refreshPaths, aUsingTree, true)) {
		string displayName = Util::emptyString;
		if (refreshPaths.size() > 1 && aUsingTree) {
			displayName = ctrlTree.getSelectedItemData()->getName();
		}

		dl->addAsyncTask([=] {
			ShareManager::getInstance()->refreshPathsHooked(ShareRefreshPriority::MANUAL, refreshPaths, this, displayName);
		});
	}
}

bool DirectoryListingFrame::getLocalPaths(StringList& paths_, bool aUsingTree, bool aDirsOnly) {
	string error;
	handleItemAction(aUsingTree, [&](const ItemInfo* ii) {
		try {
			if (!aDirsOnly && ii->type == ItemInfo::FILE) {
				dl->getLocalPathsUnsafe(ii->file, paths_);
			} else if (ii->type == ItemInfo::DIRECTORY)  {
				dl->getLocalPathsUnsafe(ii->dir, paths_);
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
	if (!ActionUtil::getUCParams(m_hWnd, uc, ucLineParams))
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
			ucParams["fileFN"] = [this, ii] { return PathUtil::toNmdcFile(ii->getAdcPath()); };
			ucParams["fileSI"] = [ii] { return Util::toString(ii->file->getSize()); };
			ucParams["fileSIshort"] = [ii] { return Util::formatBytes(ii->file->getSize()); };
			ucParams["fileTR"] = [ii] { return ii->file->getTTH().toBase32(); };
			ucParams["fileMN"] = [ii] { return ActionUtil::makeMagnet(ii->file->getTTH(), ii->file->getName(), ii->file->getSize()); };
		} else {
			ucParams["type"] = [] { return "Directory"; };
			ucParams["fileFN"] = [this, ii] { return PathUtil::toNmdcFile(ii->getAdcPath()); };
			ucParams["fileSI"] = [this, ii] { return Util::toString(ii->dir->getTotalSize(ii->dir != dl->getRoot())); };
			ucParams["fileSIshort"] = [ii] { return Util::formatBytes(ii->dir->getTotalSize(true)); };
		}

		// compatibility with 0.674 and earlier
		ucParams["file"] = ucParams["fileFN"];
		ucParams["filesize"] = ucParams["fileSI"];
		ucParams["filesizeshort"] = ucParams["fileSIshort"];
		ucParams["tth"] = ucParams["fileTR"];

		auto tmp = ucParams;
		UserCommandManager::getInstance()->userCommand(dl->getHintedUser(), uc, tmp, true);
	}
}

tstring DirectoryListingFrame::handleCopyMagnet(const ItemInfo* ii) {
	if (ii->type == ItemInfo::FILE) {
		return Text::toT(ActionUtil::makeMagnet(ii->file->getTTH(), ii->file->getName(), ii->file->getSize()));
	} 
		
	return Util::emptyStringT;
}

tstring DirectoryListingFrame::handleCopyPath(const ItemInfo* ii) {
	tstring ret = _T(" ");
	if (ii->type == ItemInfo::FILE) {
		ret += Text::toT(ii->getAdcPath());
	} else {
		if (ii->dir->isVirtual() && ii->dir->getParent()->getParent()) { // not root
			ret += Text::toT(((DirectoryListing::VirtualDirectory*)ii->dir.get())->getFullAdcPath());
		} else {
			ret += Text::toT(ii->getAdcPath());
		}
	}
	return ret;
}

tstring DirectoryListingFrame::handleCopyDirectory(const ItemInfo* ii) {
	if (ii->type == ItemInfo::FILE) {
		return Text::toT(DupeUtil::getAdcReleaseDir(ii->getAdcPath(), true));
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
			int desclen = WinUtil::getStatusTextWidth(getComboDesc(), ctrlStatus.m_hWnd);
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

bool DirectoryListingFrame::ItemInfo::NameSort::operator()(const ItemInfo& a, const ItemInfo& b) const noexcept {
	auto comp = compareItems(&a, &b, DirectoryListingFrame::COLUMN_FILENAME);
	if (comp == 0 && a.isAdl() && b.isAdl()) {
		// there can be multiple items with the same name in a single ADL directory... compare the path instead
		return Util::DefaultSort(Text::toT(a.getAdcPath()).c_str(), Text::toT(b.getAdcPath()).c_str()) > 0;
	}
	return comp > 0;
}

int DirectoryListingFrame::ItemInfo::compareItems(const ItemInfo* a, const ItemInfo* b, uint8_t col) noexcept {
	if(a->type == DIRECTORY) {
		if(b->type == DIRECTORY) {
			switch(col) {
				case COLUMN_EXACTSIZE: return compare(a->dir->getTotalSize(true), b->dir->getTotalSize(true));
				case COLUMN_SIZE: return compare(a->dir->getTotalSize(true), b->dir->getTotalSize(true));
				case COLUMN_DATE: return compare(a->dir->getRemoteDate(), b->dir->getRemoteDate());
				case COLUMN_FILENAME: {
					if (a->dir->isVirtual() != b->dir->isVirtual()) {
						return a->dir->isVirtual() ? -1 : 1;
					}

					return Util::DefaultSort(a->getNameW().c_str(), b->getNameW().c_str());
				}
				case COLUMN_TYPE: {
					return DirectoryContentInfo::Sort(a->dir->getContentInfo(), b->dir->getContentInfo());
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

DirectoryListingFrame::ItemInfo::ItemInfo(const DirectoryListing::FilePtr& f) : type(FILE), file(f), name(Text::toT(f->getName())) { 
	//dcdebug("ItemInfo (file) %s was created\n", f->getName().c_str());
}

DirectoryListingFrame::ItemInfo::ItemInfo(const DirectoryListing::DirectoryPtr& d) : type(DIRECTORY), dir(d), name(Text::toT(d->getName())) {
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

const tstring DirectoryListingFrame::ItemInfo::getText(uint8_t col) const noexcept {
	switch(col) {
		case COLUMN_FILENAME: return name;
		case COLUMN_TYPE:
			if (type == FILE) {
				return WinUtil::formatFileType(file->getName());
			} else {
				return Text::toT(Util::formatDirectoryContent(dir->getContentInfoRecursive(true)));
			}
		case COLUMN_EXACTSIZE: return type == DIRECTORY ? FormatUtil::formatExactSizeW(dir->getTotalSize(true)) : FormatUtil::formatExactSizeW(file->getSize());
		case COLUMN_SIZE: return type == DIRECTORY ? Util::formatBytesW(dir->getTotalSize(true)) : Util::formatBytesW(file->getSize());
		case COLUMN_DATE: return WinUtil::formatDateTimeW(type == DIRECTORY ? dir->getRemoteDate() : file->getRemoteDate());
		default: return Text::toT(getTextNormal(col));
	}
}

const string DirectoryListingFrame::ItemInfo::getTextNormal(uint8_t col) const noexcept {
	switch (col) {
	case COLUMN_FILENAME: return type == DIRECTORY ? dir->getName() : file->getName();
	case COLUMN_TYPE:
		if (type == FILE) {
			return Text::fromT(WinUtil::formatFileType(file->getName()));
		} else {
			return Util::emptyString;
		}
	case COLUMN_TTH: return type == FILE ? file->getTTH().toBase32() : Util::emptyString;
	case COLUMN_EXACTSIZE: return type == DIRECTORY ? FormatUtil::formatExactSize(dir->getTotalSize(true)) : FormatUtil::formatExactSize(file->getSize());
	case COLUMN_SIZE: return  type == DIRECTORY ? Util::formatBytes(dir->getTotalSize(true)) : Util::formatBytes(file->getSize());
	case COLUMN_DATE: return FormatUtil::formatDateTime(type == DIRECTORY ? dir->getRemoteDate() : file->getRemoteDate());
	default: return Util::emptyString;
	}
}

int DirectoryListingFrame::ItemInfo::getImageIndex() const noexcept {
	/*if(type == DIRECTORY)
		return DirectoryListingFrame::getIconIndex(dir);
	else*/
	return ResourceLoader::getIconIndex(getText(COLUMN_FILENAME));
}

bool DirectoryListingFrame::ItemInfo::isAdl() const noexcept {
	return type == DIRECTORY ? dir->isVirtual() : !!file->getOwner(); 
}

bool DirectoryListingFrame::allowOpen(const ItemInfo* ii) const noexcept {
	if (dl->getIsOwnList()) {
		return true;
	}
	
	return ii->type == ItemInfo::DIRECTORY ? DupeUtil::allowOpenDirectoryDupe(ii->getDupe()) : DupeUtil::allowOpenFileDupe(ii->getDupe());
}

int DirectoryListingFrame::getIconIndex(const ItemInfo* ii) const noexcept {
	if (ii->dir->getLoading() != DirectoryListing::DirectoryLoadType::NONE) {
		return ResourceLoader::DIR_LOADING;
	}
	
	if (ii->dir->isComplete() || dl->getIsOwnList()) {
		return ResourceLoader::DIR_NORMAL;
	}

	return ResourceLoader::DIR_INCOMPLETE;
}

void DirectoryListingFrame::closeAll(){
	for (auto f : frames | views::values)
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

	EXT_CONTEXT_MENU(tabMenu, Filelist, vector<CID>({ dl->getUser()->getCID() }));

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
		const auto& newHub = hubs[selCombo.GetCurSel()];
		if (aUserChange) {
			auto p = ranges::find_if(hubs, [this](const User::UserHubInfo& uhi) { return uhi.hubUrl == dl->getHubUrl(); });
			if (p != hubs.end()) {
				auto& oldHub = *p;
				auto diff = newHub.shared > 0 ? abs(static_cast<double>(oldHub.shared) / static_cast<double>(newHub.shared)) : oldHub.shared;
				if (diff < 0.95 || diff > 1.05) {
					auto msg = TSTRING_F(LIST_SIZE_DIFF_NOTE, Text::toT(newHub.hubName) % Util::formatBytesW(newHub.shared) % Text::toT(oldHub.hubName) % Util::formatBytesW(oldHub.shared));
					if (WinUtil::showQuestionBox(msg, MB_ICONQUESTION)) {
						dl->addAsyncTask([=] {
							try {
								auto flags = (dl->getPartialList() ? QueueItem::FLAG_PARTIAL_LIST : 0) | QueueItem::FLAG_CLIENT_VIEW;
								auto listData = FilelistAddData(HintedUser(dl->getUser(), newHub.hubUrl), this, ADC_ROOT_STR);
								QueueManager::getInstance()->addListHooked(listData, flags);
							} catch (const Exception& e) {
								ctrlStatus.SetText(STATUS_TEXT, Text::toT(e.getError()).c_str());
							}
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
		auto [_, onlineNew] = FormatUtil::getHubNames(cid);
		if (!onlineNew && !online) {
			//nothing to update... probably a delayed event
			return;
		}

		auto oldSel = selCombo.GetStyle() & WS_VISIBLE ? selCombo.GetCurSel() : 0;
		StringPair oldHubPair;
		if (!hubs.empty())
			oldHubPair = { hubs[oldSel].hubName, hubs[oldSel].hubUrl }; // cache the old hub name

		hubs = ClientManager::getInstance()->getUserInfoList(dl->getUser());
		while (selCombo.GetCount()) {
			selCombo.DeleteString(0);
		}

		//General things
		if (onlineNew) {
			//the user is online

			onlineHubNames = FormatUtil::getHubNames(dl->getHintedUser());
			onlineNicks = FormatUtil::getNicks(dl->getHintedUser());
			setDisconnected(false);

		} else {
			setDisconnected(true);
		}

		//ADC related changes
		if (onlineNew && !dl->getUser()->isNMDC() && !hubs.empty()) {
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

		online = onlineNew;
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
}