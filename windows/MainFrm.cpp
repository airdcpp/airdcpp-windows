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

#include "MainFrm.h"
#include "AboutDlg.h"
#include "HubFrame.h"
#include "SearchFrm.h"
#include "PublicHubsFrm.h"
#include "PropertiesDlg.h"
#include "UsersFrame.h"
#include "DirectoryListingFrm.h"
#include "RecentsFrm.h"
#include "FavoritesFrm.h"
#include "NotepadFrame.h"
#include "QueueFrame.h"
#include "SpyFrame.h"
#include "FinishedFrame.h"
#include "ADLSearchFrame.h"
#include "FinishedULFrame.h"
#include "TextFrame.h"
#include "UpdateDlg.h"
#include "StatsFrame.h"
#include "UploadQueueFrame.h"
#include "LineDlg.h"
#include "PrivateFrame.h"
#include "WinUtil.h"
#include "CDMDebugFrame.h"
#include "InputBox.h"
#include "PopupManager.h"
#include "Wizard.h"
#include "AutoSearchFrm.h"

#include "Winamp.h"
#include "Players.h"
#include "iTunesCOMInterface.h"
#include "SystemFrame.h"

#include "../client/ConnectionManager.h"
#include "../client/ConnectivityManager.h"
#include "../client/DownloadManager.h"
#include "../client/UploadManager.h"
#include "../client/StringTokenizer.h"
#include "../client/ShareManager.h"
#include "../client/LogManager.h"
#include "../client/FavoriteManager.h"
#include "../client/MappingManager.h"
#include "../client/AirUtil.h"
#include "../client/DirectoryListingManager.h"
#include "../client/UpdateManager.h"
#include "../client/GeoManager.h"
#include "../client/ThrottleManager.h"
#include "../client/version.h"

MainFrame* MainFrame::anyMF = NULL;
bool MainFrame::bShutdown = false;
uint64_t MainFrame::iCurrentShutdownTime = 0;
bool MainFrame::isShutdownStatus = false;

#define ICON_SPACE 24

//static HICON mainIcon(ResourceLoader::loadIcon(IDR_MAINFRAME, ::GetSystemMetrics(SM_CXICON)));
//static HICON mainSmallIcon(ResourceLoader::loadIcon(IDR_MAINFRAME, ::GetSystemMetrics(SM_CXSMICON)));

MainFrame::MainFrame() : trayMessage(0), maximized(false), lastUpload(-1), lastUpdate(0), 
lastUp(0), lastDown(0), oldshutdown(false), stopperThread(NULL),
closing(false), awaybyminimize(false), missedAutoConnect(false), lastTTHdir(Util::emptyStringT), tabsontop(false),
bTrayIcon(false), bAppMinimized(false), bIsPM(false), hashProgress(false), trayUID(0),
statusContainer(STATUSCLASSNAME, this, STATUS_MESSAGE_MAP)


{ 
	if(Util::getOsMajor() >= 6) {
		user32lib = LoadLibrary(_T("user32"));
		_d_ChangeWindowMessageFilter = (LPFUNC)GetProcAddress(user32lib, "ChangeWindowMessageFilter");
	}

	memzero(statusSizes, sizeof(statusSizes));
	anyMF = this;
}

LRESULT MainFrame::onOpenDownloads(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	WinUtil::openFile(Text::toT(SETTING(DOWNLOAD_DIRECTORY)));
	return 0;
}

LRESULT MainFrame::onOpenLogDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	WinUtil::openFolder(Text::toT(SETTING(LOG_DIRECTORY)));
	return 0;
}

MainFrame::~MainFrame() {
	m_CmdBar.m_hImageList = NULL;

	images.Destroy();
	ToolbarImages.Destroy();
	ToolbarImagesHot.Destroy();
	winampImages.Destroy();

	WinUtil::uninit();
	ResourceLoader::unload();

	if((Util::getOsMajor() >= 6) && user32lib)
		FreeLibrary(user32lib);
}

void MainFrame::addThreadedTask(std::function<void ()> aF) {
	threadedTasks.run(aF);
}

DWORD WINAPI MainFrame::stopper(void* p) {
	MainFrame* mf = (MainFrame*)p;
	HWND wnd, wnd2 = NULL;

	mf->threadedTasks.wait();
	while( (wnd=::GetWindow(mf->m_hWndMDIClient, GW_CHILD)) != NULL) {
		if(wnd == wnd2)
			Sleep(100);
		else { 
			::PostMessage(wnd, WM_CLOSE, 0, 0);
			wnd2 = wnd;
		}
	}

	mf->PostMessage(WM_CLOSE);	

	return 0;
}

class ListMatcher : public Thread {
public:
	ListMatcher(StringList files_) : files(files_) {

	}
	int run() {
		for(const auto& i: files) {
			UserPtr u = DirectoryListing::getUserFromFilename(i);
			if(!u)
				continue;
				
			HintedUser user(u, Util::emptyString);
			unique_ptr<DirectoryListing> dl(new DirectoryListing(user, false, i, false, false));
			try {
				dl->loadFile();
				int matches=0, newFiles=0;
				BundleList bundles;
				QueueManager::getInstance()->matchListing(*dl, matches, newFiles, bundles);
				LogManager::getInstance()->message(dl->getNick(false) + ": " + AirUtil::formatMatchResults(matches, newFiles, bundles, false), LogManager::LOG_INFO);
			} catch(const Exception&) {

			}
		}
		delete this;
		return 0;
	}
	StringList files;
};

LRESULT MainFrame::onMatchAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ListMatcher* matcher = new ListMatcher(File::findFiles(Util::getListPath(), "*.xml*"));
	try {
		matcher->start();
	} catch(const ThreadException&) {
		///@todo add error message
		delete matcher;
	}
	
	return 0;
}

void MainFrame::terminate() {
	oldshutdown = true;
	PostMessage(WM_CLOSE);
}

LRESULT MainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	TimerManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);
	LogManager::getInstance()->addListener(this);
	DirectoryListingManager::getInstance()->addListener(this);
	UpdateManager::getInstance()->addListener(this);
	ShareScannerManager::getInstance()->addListener(this);

	WinUtil::init(m_hWnd);
	ResourceLoader::load();

	trayMessage = RegisterWindowMessage(_T("TaskbarCreated"));

	if(Util::getOsMajor() >= 6) {
		// 1 == MSGFLT_ADD
		_d_ChangeWindowMessageFilter(trayMessage, 1);
		_d_ChangeWindowMessageFilter(WMU_WHERE_ARE_YOU, 1);

		if(Util::getOsMajor() > 6 || Util::getOsMinor() >= 1) {
			tbButtonMessage = RegisterWindowMessage(_T("TaskbarButtonCreated"));
			_d_ChangeWindowMessageFilter(tbButtonMessage, 1);
			_d_ChangeWindowMessageFilter(WM_COMMAND, 1);
		}
	}

	TimerManager::getInstance()->start();

	// Set window name
	SetWindowText(COMPLETEVERSIONSTRING);

	// Load images
	// create command bar window
	HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
	
	m_hMenu = WinUtil::mainMenu;

	hShutdownIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SHUTDOWN), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

	// attach menu
	m_CmdBar.AttachMenu(m_hMenu);

	// load command bar images
	ResourceLoader::loadCmdBarImageList(images);
	m_CmdBar.m_hImageList = images;

	m_CmdBar.m_arrCommand.Add(ID_FILE_CONNECT);
	m_CmdBar.m_arrCommand.Add(ID_FILE_RECONNECT);
	m_CmdBar.m_arrCommand.Add(IDC_FOLLOW);
	m_CmdBar.m_arrCommand.Add(IDC_RECENTS);
	m_CmdBar.m_arrCommand.Add(IDC_FAVORITES);
	m_CmdBar.m_arrCommand.Add(IDC_FAVUSERS);
	m_CmdBar.m_arrCommand.Add(IDC_QUEUE);
	m_CmdBar.m_arrCommand.Add(IDC_FINISHED);
	m_CmdBar.m_arrCommand.Add(IDC_UPLOAD_QUEUE);
	m_CmdBar.m_arrCommand.Add(IDC_FINISHED_UL);
	m_CmdBar.m_arrCommand.Add(ID_FILE_SEARCH);
	m_CmdBar.m_arrCommand.Add(IDC_FILE_ADL_SEARCH);
	m_CmdBar.m_arrCommand.Add(IDC_SEARCH_SPY);
	m_CmdBar.m_arrCommand.Add(IDC_OPEN_FILE_LIST);
	m_CmdBar.m_arrCommand.Add(IDC_BROWSE_OWN_LIST);
	m_CmdBar.m_arrCommand.Add(IDC_OWN_LIST_ADL);
	m_CmdBar.m_arrCommand.Add(IDC_MATCH_ALL);
	m_CmdBar.m_arrCommand.Add(IDC_REFRESH_FILE_LIST);
	m_CmdBar.m_arrCommand.Add(IDC_SCAN_MISSING);
	m_CmdBar.m_arrCommand.Add(IDC_OPEN_DOWNLOADS);
	m_CmdBar.m_arrCommand.Add(ID_FILE_QUICK_CONNECT);
	m_CmdBar.m_arrCommand.Add(ID_FILE_SETTINGS);
	m_CmdBar.m_arrCommand.Add(ID_GET_TTH);
	m_CmdBar.m_arrCommand.Add(IDC_UPDATE);
	m_CmdBar.m_arrCommand.Add(ID_APP_EXIT);
	m_CmdBar.m_arrCommand.Add(IDC_NOTEPAD);
	m_CmdBar.m_arrCommand.Add(IDC_NET_STATS);
	m_CmdBar.m_arrCommand.Add(IDC_CDMDEBUG_WINDOW);
	m_CmdBar.m_arrCommand.Add(IDC_SYSTEM_LOG);
	m_CmdBar.m_arrCommand.Add(IDC_AUTOSEARCH);
	m_CmdBar.m_arrCommand.Add(IDC_HASH_PROGRESS);
	m_CmdBar.m_arrCommand.Add(ID_APP_ABOUT);
	m_CmdBar.m_arrCommand.Add(ID_WIZARD);
	m_CmdBar.m_arrCommand.Add(IDC_OPEN_LOG_DIR);

	// use Vista-styled menus on Vista/Win7
	if(Util::getOsMajor() >= 6)
		m_CmdBar._AddVistaBitmapsFromImageList(0, m_CmdBar.m_arrCommand.GetSize());

	// remove old menu
	SetMenu(NULL);

	tbarcreated = false;
	tbarwinampcreated = false;
	HWND hWndToolBar = createToolbar();
	HWND hWndWinampBar = createWinampToolbar();
	HWND hWndTBStatusBar = createTBStatusBar();

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndCmdBar);
	AddSimpleReBarBand(hWndToolBar, NULL, TRUE);
	AddSimpleReBarBand(hWndWinampBar, NULL, TRUE);
	AddSimpleReBarBand(hWndTBStatusBar, NULL, FALSE, 212, TRUE);

	CreateSimpleStatusBar();
	
	RECT toolRect = {0};
	::GetWindowRect(hWndToolBar, &toolRect);

	ctrlStatus.Attach(m_hWndStatusBar);
	ctrlStatus.SetSimple(FALSE);
	int w[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	ctrlStatus.SetParts(11, w);
	//statusSizes[0] = 4;
	statusContainer.SubclassWindow(ctrlStatus.m_hWnd);

	ctrlTooltips.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON, WS_EX_TOPMOST);
	ctrlTooltips.SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	// last lines and away are different, the tooltip text changes, onGetToolTip handles the text shown.
	CToolInfo ti_lastlines(TTF_SUBCLASS, ctrlStatus.m_hWnd, STATUS_LASTLINES+POPUP_UID, 0, LPSTR_TEXTCALLBACK);
	CToolInfo ti_away(TTF_SUBCLASS, ctrlStatus.m_hWnd, STATUS_AWAY+POPUP_UID, 0, LPSTR_TEXTCALLBACK);
	CToolInfo ti_shared(TTF_SUBCLASS, ctrlStatus.m_hWnd, STATUS_SHARED+POPUP_UID, 0, (LPWSTR)CTSTRING(SHARED));
	CToolInfo ti_dlSpeed(TTF_SUBCLASS, ctrlStatus.m_hWnd, STATUS_DL_SPEED+POPUP_UID, 0, (LPWSTR)CTSTRING(DL_STATUS_POPUP));
	CToolInfo ti_ulSpeed(TTF_SUBCLASS, ctrlStatus.m_hWnd, STATUS_UL_SPEED+POPUP_UID, 0, (LPWSTR)CTSTRING(UL_STATUS_POPUP));
	CToolInfo ti_queueSize(TTF_SUBCLASS, ctrlStatus.m_hWnd, STATUS_QUEUED+POPUP_UID, 0, (LPWSTR)CTSTRING(QUEUE_SIZE));
	CToolInfo ti_slots(TTF_SUBCLASS, ctrlStatus.m_hWnd, STATUS_SLOTS+POPUP_UID, 0, (LPWSTR)CTSTRING(SLOTS));  //TODO: Click to adjust
	CToolInfo ti_hubs(TTF_SUBCLASS, ctrlStatus.m_hWnd, STATUS_HUBS+POPUP_UID, 0, (LPWSTR)CTSTRING(HUBS));
	CToolInfo ti_downloaded(TTF_SUBCLASS, ctrlStatus.m_hWnd, STATUS_DOWNLOADED+POPUP_UID, 0, (LPWSTR)CTSTRING(DOWNLOADED));
	CToolInfo ti_uploaded(TTF_SUBCLASS, ctrlStatus.m_hWnd, STATUS_UPLOADED+POPUP_UID, 0, (LPWSTR)CTSTRING(UPLOADED));

	ctrlTooltips.AddTool(&ti_lastlines);
	ctrlTooltips.AddTool(&ti_away);
	ctrlTooltips.AddTool(&ti_shared);
	ctrlTooltips.AddTool(&ti_dlSpeed);
	ctrlTooltips.AddTool(&ti_ulSpeed);
	ctrlTooltips.AddTool(&ti_queueSize);
	ctrlTooltips.AddTool(&ti_slots);
	ctrlTooltips.AddTool(&ti_hubs);
	ctrlTooltips.AddTool(&ti_downloaded);
	ctrlTooltips.AddTool(&ti_uploaded);
	ctrlTooltips.SetDelayTime(TTDT_AUTOPOP, 15000);

	CreateMDIClient();
	m_CmdBar.SetMDIClient(m_hWndMDIClient);
	WinUtil::mdiClient = m_hWndMDIClient;

	ctrlTab.Create(m_hWnd, rcDefault);
	WinUtil::tabCtrl = &ctrlTab;
	tabsontop = SETTING(TABS_ON_TOP);

	transferView.Create(m_hWnd);

	SetSplitterPanes(m_hWndMDIClient, transferView.m_hWnd);
	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	m_nProportionalPos = SETTING(TRANSFER_SPLIT_SIZE);
	UIAddToolBar(hWndToolBar);
	UIAddToolBar(hWndWinampBar);
	UIAddToolBar(hWndTBStatusBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);
	UISetCheck(ID_VIEW_TRANSFER_VIEW, 1);
	UISetCheck(ID_TOGGLE_TOOLBAR, 1);
	UISetCheck(ID_TOGGLE_TBSTATUS, 1);
	UISetCheck(ID_LOCK_TB, 0);

	WinUtil::loadReBarSettings(m_hWndToolBar);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	if(SETTING(OPEN_PUBLIC)) PostMessage(WM_COMMAND, ID_FILE_CONNECT);
	if(SETTING(OPEN_FAVORITE_HUBS)) PostMessage(WM_COMMAND, IDC_FAVORITES);
	if(SETTING(OPEN_FAVORITE_USERS)) PostMessage(WM_COMMAND, IDC_FAVUSERS);
	if(SETTING(OPEN_QUEUE)) PostMessage(WM_COMMAND, IDC_QUEUE);
	if(SETTING(OPEN_FINISHED_DOWNLOADS)) PostMessage(WM_COMMAND, IDC_FINISHED);
	if(SETTING(OPEN_WAITING_USERS)) PostMessage(WM_COMMAND, IDC_UPLOAD_QUEUE);
	if(SETTING(OPEN_FINISHED_UPLOADS)) PostMessage(WM_COMMAND, IDC_FINISHED_UL);
	if(SETTING(OPEN_SEARCH_SPY)) PostMessage(WM_COMMAND, IDC_SEARCH_SPY);
	if(SETTING(OPEN_NETWORK_STATISTICS)) PostMessage(WM_COMMAND, IDC_NET_STATS);
	if(SETTING(OPEN_NOTEPAD)) PostMessage(WM_COMMAND, IDC_NOTEPAD);

	if(!SETTING(SHOW_STATUSBAR)) PostMessage(WM_COMMAND, ID_VIEW_STATUS_BAR);
	if(!SETTING(SHOW_TOOLBAR)) PostMessage(WM_COMMAND, ID_VIEW_TOOLBAR);
	if(!SETTING(SHOW_TRANSFERVIEW))	PostMessage(WM_COMMAND, ID_VIEW_TRANSFER_VIEW);

	if(!SETTING(SHOW_WINAMP_CONTROL)) PostMessage(WM_COMMAND, ID_TOGGLE_TOOLBAR);
	if(!SETTING(SHOW_TBSTATUS)) PostMessage(WM_COMMAND, ID_TOGGLE_TBSTATUS);
	if(SETTING(LOCK_TB)) PostMessage(WM_COMMAND, ID_LOCK_TB);
	if(SETTING(OPEN_SYSTEM_LOG)) PostMessage(WM_COMMAND, IDC_SYSTEM_LOG);

	if(!WinUtil::isShift() && !Util::hasParam("/noautoconnect"))
		callAsync([=] { autoConnect(FavoriteManager::getInstance()->getFavoriteHubs()); });

	callAsync([=] { parseCommandLine(GetCommandLine()); });

	try {
		File::ensureDirectory(SETTING(LOG_DIRECTORY));
	} catch (const FileException) {	}

	try {
		ConnectivityManager::getInstance()->setup(true, true);
	} catch (const Exception& e) {
		showPortsError(e.getError());
	}

	UpdateManager::getInstance()->init(Util::getAppName());
		
	WinUtil::SetIcon(m_hWnd, IDR_MAINFRAME, true);
	WinUtil::SetIcon(m_hWnd, IDR_MAINFRAME);
	pmicon.hIcon = ResourceLoader::loadIcon(IDR_TRAY_PM, 16);
	hubicon.hIcon = ResourceLoader::loadIcon(IDR_TRAY_HUB, 16);

	updateTray(true);

	AirUtil::setAway(SETTING(AWAY) ? AWAY_MANUAL : AWAY_OFF);
	ctrlToolbar.CheckButton(IDC_AWAY,SETTING(AWAY));
	ctrlToolbar.CheckButton(IDC_DISABLE_SOUNDS, SETTING(SOUNDS_DISABLED));

	if(SETTING(NICK).empty()) {
		PostMessage(WM_COMMAND, ID_FILE_SETTINGS);
	}
	
	slotsIcon = ResourceLoader::loadIcon(IDI_SLOTS, 16);
	slotsFullIcon = ResourceLoader::loadIcon(IDI_SLOTSFULL, 16);
	const int i = UserInfoBase::USER_ICON_AWAY * (UserInfoBase::USER_ICON_LAST - UserInfoBase::USER_ICON_MOD_START) * (UserInfoBase::USER_ICON_LAST - UserInfoBase::USER_ICON_MOD_START);
	awayIconON = ResourceLoader::getUserImages().GetIcon(i);
	awayIconOFF = ResourceLoader::getUserImages().GetIcon(0);
	infoIcon = ResourceLoader::loadIcon(IDI_INFO, 16);
	warningIcon = ResourceLoader::loadIcon(IDI_IWARNING, 16);
	errorIcon = ResourceLoader::loadIcon(IDI_IERROR, 16);;

	ctrlStatus.SetIcon(STATUS_AWAY, AirUtil::getAway() ? awayIconON : awayIconOFF);
	ctrlStatus.SetIcon(STATUS_SHARED, ResourceLoader::loadIcon(IDI_SHARED, 16));
	ctrlStatus.SetIcon(STATUS_SLOTS, slotsIcon);
	ctrlStatus.SetIcon(STATUS_HUBS, ResourceLoader::loadIcon(IDI_HUB, 16));
	ctrlStatus.SetIcon(STATUS_DOWNLOADED, ResourceLoader::loadIcon(IDI_TOTAL_DOWN, 16));
	ctrlStatus.SetIcon(STATUS_UPLOADED, ResourceLoader::loadIcon(IDI_TOTAL_UP, 16));
	ctrlStatus.SetIcon(STATUS_DL_SPEED, ResourceLoader::loadIcon(IDI_DOWNLOAD, 16));
	ctrlStatus.SetIcon(STATUS_UL_SPEED, ResourceLoader::loadIcon(IDI_UPLOAD, 16));
	ctrlStatus.SetIcon(STATUS_QUEUED, ResourceLoader::loadIcon(IDI_QUEUE, 16));

	//background image
	if(!SETTING(BACKGROUND_IMAGE).empty()) {
		m_PictureWindow.SubclassWindow(m_hWndMDIClient);
		m_PictureWindow.m_nMessageHandler = CPictureWindow::BackGroundPaint;
		currentPic = SETTING(BACKGROUND_IMAGE);
		m_PictureWindow.Load(Text::toT(currentPic).c_str());
	}

	if (Util::getOsMajor() >= 6 && Util::getOsMinor() >= 2 && WinUtil::isDesktopOs() && WinUtil::isElevated()) {
		WinUtil::ShowMessageBox(SettingsManager::WARN_ELEVATED, TSTRING(ELEVATED_WARNING));
	}

	if(SETTING(TESTWRITE)) {
		TestWrite(true, true, Util::usingLocalMode());
	}

	WinUtil::splash.reset();

	// We want to pass this one on to the splitter...hope it get's there...
	bHandled = FALSE;
	return 0;
}

LRESULT MainFrame::onTaskbarButton(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if(Util::getOsMajor() < 6 || (Util::getOsMajor() == 6 && Util::getOsMinor() < 1))
		return 0;

	taskbarList.Release();
	taskbarList.CoCreateInstance(CLSID_TaskbarList);
	taskbarList->SetOverlayIcon(m_hWnd, NULL, NULL);

	THUMBBUTTON buttons[2];
	buttons[0].dwMask = THB_ICON | THB_TOOLTIP | THB_FLAGS;
	buttons[0].iId = IDC_OPEN_DOWNLOADS;
	buttons[0].hIcon = ResourceLoader::loadIcon(IDI_OPEN_DOWNLOADS, 16);
	wcscpy(buttons[0].szTip, CWSTRING(MENU_OPEN_DOWNLOADS_DIR));
	buttons[0].dwFlags = THBF_ENABLED;

	buttons[1].dwMask = THB_ICON | THB_TOOLTIP | THB_FLAGS;
	buttons[1].iId = ID_FILE_SETTINGS;
	buttons[1].hIcon = ResourceLoader::loadIcon(IDI_SETTINGS, 16);
	wcscpy(buttons[1].szTip, CWSTRING(SETTINGS));
	buttons[1].dwFlags = THBF_ENABLED;

	taskbarList->ThumbBarAddButtons(m_hWnd, sizeof(buttons)/sizeof(THUMBBUTTON), buttons);

	for(int i = 0; i < sizeof(buttons)/sizeof(THUMBBUTTON); ++i)
		DestroyIcon(buttons[i].hIcon);

	return 0;
}


HWND MainFrame::createTBStatusBar() {
	
	TBStatusCtrl.Create(m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT , 0, ATL_IDW_TOOLBAR);

	TBBUTTON tb[1];
	memzero(&tb, sizeof(tb));
	tb[0].iBitmap = 210;
	tb[0].fsStyle = TBSTYLE_SEP;

	TBStatusCtrl.SetButtonStructSize();
	TBStatusCtrl.AddButtons(1, tb);
	TBStatusCtrl.AutoSize();

	CRect rect;
	TBStatusCtrl.GetItemRect(0, &rect);
	rect.left += 2;

	progress.Create(TBStatusCtrl.m_hWnd, rect, NULL, WS_CHILD | PBS_SMOOTH | WS_VISIBLE);
	progress.SetRange(0, 10000);

	startBytes, startFiles = 0;
	refreshing = false;

	return TBStatusCtrl.m_hWnd;
}


void MainFrame::showPortsError(const string& port) {
	showMessageBox(Text::toT(STRING_F(PORT_BYSY, port)), MB_OK | MB_ICONEXCLAMATION);
}

HWND MainFrame::createWinampToolbar() {
	if(!tbarwinampcreated){
		ctrlSmallToolbar.Create(m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST, 0, ATL_IDW_TOOLBAR);
		ctrlSmallToolbar.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS);

		//we want to support old themes also so check if there is the old .bmp
		if(Util::fileExists(Text::fromT(ResourceLoader::getIconPath(_T("mediatoolbar.bmp"))))){
			winampImages.CreateFromImage(ResourceLoader::getIconPath(_T("mediatoolbar.bmp")).c_str(), 20, 20, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED | LR_LOADFROMFILE);
		} else {
			ResourceLoader::loadWinampToolbarIcons(winampImages);
		}
		
		ctrlSmallToolbar.SetImageList(winampImages);
		tbarwinampcreated = true;
	}
	
	while(ctrlSmallToolbar.GetButtonCount()>0)
		ctrlSmallToolbar.DeleteButton(0);

	ctrlSmallToolbar.SetButtonStructSize();
	StringTokenizer<string> t(SETTING(MEDIATOOLBAR), ',');
	StringList& l = t.getTokens();

	int buttonsCount = sizeof(WinampToolbarButtons) / sizeof(WinampToolbarButtons[0]);
	for(StringList::const_iterator k = l.begin(); k != l.end(); ++k) {
		int i = Util::toInt(*k);		
		
		TBBUTTON nTB;
		memzero(&nTB, sizeof(TBBUTTON));

		if(i == -1) {
			nTB.fsStyle = TBSTYLE_SEP;			
		} else if(i >= 0 && i < buttonsCount) {
			nTB.iBitmap = WinampToolbarButtons[i].image;
			nTB.idCommand = WinampToolbarButtons[i].id;
			nTB.fsState = TBSTATE_ENABLED;
			nTB.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
			nTB.iString = ctrlSmallToolbar.AddStrings(CTSTRING_I((ResourceManager::Strings)WinampToolbarButtons[i].tooltip));
		} else {
			continue;
		}

		ctrlSmallToolbar.AddButtons(1, &nTB);
	}	

	ctrlSmallToolbar.AutoSize();

	return ctrlSmallToolbar.m_hWnd;
}

LRESULT MainFrame::onWinampButton(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(SETTING(MEDIA_PLAYER) == 0) {
		HWND hwndWinamp = FindWindow(_T("Winamp v1.x"), NULL);
		if (::IsWindow(hwndWinamp)) {
			switch(wID) {
				case IDC_WINAMP_BACK: SendMessage(hwndWinamp, WM_COMMAND, WINAMP_BUTTON1, 0); break;
				case IDC_WINAMP_PLAY: SendMessage(hwndWinamp, WM_COMMAND, WINAMP_BUTTON2, 0); break;
				case IDC_WINAMP_STOP: SendMessage(hwndWinamp, WM_COMMAND, WINAMP_BUTTON4, 0); break;
				case IDC_WINAMP_PAUSE: SendMessage(hwndWinamp, WM_COMMAND, WINAMP_BUTTON3, 0); break;	
				case IDC_WINAMP_NEXT: SendMessage(hwndWinamp, WM_COMMAND, WINAMP_BUTTON5, 0); break;
				case IDC_WINAMP_VOL_UP: SendMessage(hwndWinamp, WM_COMMAND, WINAMP_VOLUMEUP, 0); break;
				case IDC_WINAMP_VOL_DOWN: SendMessage(hwndWinamp, WM_COMMAND, WINAMP_VOLUMEDOWN, 0); break;
				case IDC_WINAMP_VOL_HALF: SendMessage(hwndWinamp, WM_WA_IPC, 255/2, IPC_SETVOLUME); break;
			}
		}

	} else if(SETTING(MEDIA_PLAYER) == 1) {
		// Since i couldn't find out the appropriate window messages, we doing this à la COM
		HWND hwndiTunes = FindWindow(_T("iTunes"), _T("iTunes"));
		if (::IsWindow(hwndiTunes)) {
			IiTunes *iITunes;
			CoInitialize(NULL);
			if (SUCCEEDED(::CoCreateInstance(CLSID_iTunesApp, NULL, CLSCTX_LOCAL_SERVER, IID_IiTunes, (PVOID *)&iITunes))) {
				long currVol;
				switch(wID) {
					case IDC_WINAMP_BACK: iITunes->PreviousTrack(); break;
					case IDC_WINAMP_PLAY: iITunes->Play(); break;
					case IDC_WINAMP_STOP: iITunes->Stop();  break;
					case IDC_WINAMP_PAUSE: iITunes->Pause(); break;	
					case IDC_WINAMP_NEXT: iITunes->NextTrack(); break;
					case IDC_WINAMP_VOL_UP: iITunes->get_SoundVolume(&currVol); iITunes->put_SoundVolume(currVol+10); break;
					case IDC_WINAMP_VOL_DOWN: iITunes->get_SoundVolume(&currVol); iITunes->put_SoundVolume(currVol-10); break;
					case IDC_WINAMP_VOL_HALF: iITunes->put_SoundVolume(50); break;
				}
			}
			iITunes->Release();
			CoUninitialize();
		}
	} else if(SETTING(MEDIA_PLAYER) == 2) {
		HWND hwndMPC = FindWindow(_T("MediaPlayerClassicW"), NULL);
		if (::IsWindow(hwndMPC)) {
			switch(wID) {
				case IDC_WINAMP_BACK: SendMessage(hwndMPC, WM_COMMAND, MPC_PREV, 0); break;
				case IDC_WINAMP_PLAY: SendMessage(hwndMPC, WM_COMMAND, MPC_PLAY, 0); break;
				case IDC_WINAMP_STOP: SendMessage(hwndMPC, WM_COMMAND, MPC_STOP, 0); break;
				case IDC_WINAMP_PAUSE: SendMessage(hwndMPC, WM_COMMAND, MPC_PAUSE, 0); break;	
				case IDC_WINAMP_NEXT: SendMessage(hwndMPC, WM_COMMAND, MPC_NEXT, 0); break;
				case IDC_WINAMP_VOL_UP: SendMessage(hwndMPC, WM_COMMAND, MPC_VOLUP, 0); break;
				case IDC_WINAMP_VOL_DOWN: SendMessage(hwndMPC, WM_COMMAND, MPC_VOLDOWN, 0); break;
				case IDC_WINAMP_VOL_HALF: SendMessage(hwndMPC, WM_COMMAND, MPC_MUTE, 0); break;
			}
		}
	}
	 else if(SETTING(MEDIA_PLAYER) == 3) {
		HWND hwndWMP = FindWindow(_T("WMPlayerApp"), NULL);
		if (::IsWindow(hwndWMP)) {
			switch(wID) {
				case IDC_WINAMP_BACK: SendMessage(hwndWMP, WM_COMMAND, WMP_PREV, 0); break;
				case IDC_WINAMP_PLAY: SendMessage(hwndWMP, WM_COMMAND, WMP_PLAY, 0); break;
				case IDC_WINAMP_STOP: SendMessage(hwndWMP, WM_COMMAND, WMP_STOP, 0); break;
				case IDC_WINAMP_PAUSE: SendMessage(hwndWMP, WM_COMMAND, WMP_PLAY, 0); break;	
				case IDC_WINAMP_NEXT: SendMessage(hwndWMP, WM_COMMAND, WMP_NEXT, 0); break;
				case IDC_WINAMP_VOL_UP: SendMessage(hwndWMP, WM_COMMAND, WMP_VOLUP, 0); break;
				case IDC_WINAMP_VOL_DOWN: SendMessage(hwndWMP, WM_COMMAND, WMP_VOLDOWN, 0); break;
				case IDC_WINAMP_VOL_HALF: SendMessage(hwndWMP, WM_COMMAND, WMP_MUTE, 0); break;
			}
		}
	 }
	 else if(SETTING(MEDIA_PLAYER) == 4) {
		HWND hwndSpotify = FindWindow(_T("SpotifyMainWindow"), NULL);
		if (::IsWindow(hwndSpotify)) {
			switch(wID) {
				case IDC_WINAMP_BACK: SendMessage(hwndSpotify, 0x319, 0, 0xc0000L); break;
				case IDC_WINAMP_PLAY: SendMessage(hwndSpotify, 0x319, 0, 0xe0000L); break;
				case IDC_WINAMP_STOP: SendMessage(hwndSpotify, 0x319, 0, 0xd0000L); break;
				case IDC_WINAMP_PAUSE: SendMessage(hwndSpotify, 0x319, 0, 0xe0000L); break;	
				case IDC_WINAMP_NEXT: SendMessage(hwndSpotify, 0x319, 0, 0xb0000L); break;
				case IDC_WINAMP_VOL_UP: SendMessage(hwndSpotify, 0x319, 0, 0xa0000L); break;
				case IDC_WINAMP_VOL_DOWN: SendMessage(hwndSpotify, 0x319, 0, 0x90000L); break;
				case IDC_WINAMP_VOL_HALF: SendMessage(hwndSpotify, 0x319, 0, 0x80000L); break;
			}
		}
	 }
       return 0;
}

HWND MainFrame::createToolbar() {
	if(!tbarcreated) {

		ctrlToolbar.Create(m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST, 0, ATL_IDW_TOOLBAR);
		ctrlToolbar.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS);

		if(!(SETTING(TOOLBARIMAGE) == "") && Util::fileExists(SETTING(TOOLBARIMAGE)) ) { //we expect to have the toolbarimage set before setting the Hot image.
			ToolbarImages.CreateFromImage(Text::toT(SETTING(TOOLBARIMAGE)).c_str(), 0/*SETTING(TB_IMAGE_SIZE)*/, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED | LR_LOADFROMFILE);

			if(!(SETTING(TOOLBARHOTIMAGE) == "")) { 
				ToolbarImagesHot.CreateFromImage(Text::toT(SETTING(TOOLBARHOTIMAGE)).c_str(), 0/*SETTING(TB_IMAGE_SIZE_HOT)*/, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED | LR_LOADFROMFILE);
				ctrlToolbar.SetHotImageList(ToolbarImagesHot);
			}
		} else { //default ones are .ico
			int i = 0;
			const int size = SETTING(TB_IMAGE_SIZE);
			int buttonsCount = sizeof(ToolbarButtons) / sizeof(ToolbarButtons[0]);
			ToolbarImages.Create(size, size, ILC_COLOR32 | ILC_MASK,  0, buttonsCount+1);
			ToolbarImagesHot.Create(size, size, ILC_COLOR32 | ILC_MASK,  0, buttonsCount+1);
			while(i < buttonsCount){
				HICON icon = ResourceLoader::loadIcon(ToolbarButtons[i].nIcon, size);
				ToolbarImages.AddIcon(icon);
				ToolbarImagesHot.AddIcon(ResourceLoader::convertGrayscaleIcon(icon));
				i++;
			}
		}
		ctrlToolbar.SetImageList(ToolbarImages);
		ctrlToolbar.SetHotImageList(ToolbarImagesHot);
		tbarcreated = true;
	}

	while(ctrlToolbar.GetButtonCount()>0)
		ctrlToolbar.DeleteButton(0);

	ctrlToolbar.SetButtonStructSize();
	StringTokenizer<string> t(SETTING(TOOLBAR), ',');
	StringList& l = t.getTokens();
	
	int buttonsCount = sizeof(ToolbarButtons) / sizeof(ToolbarButtons[0]);
	for(StringList::const_iterator k = l.begin(); k != l.end(); ++k) {
		int i = Util::toInt(*k);		
		
		TBBUTTON nTB;
		memzero(&nTB, sizeof(TBBUTTON));

		if(i == -1) {
			nTB.fsStyle = TBSTYLE_SEP;			
		} else if(i >= 0 && i < buttonsCount) {
			nTB.iBitmap = ToolbarButtons[i].image;
			nTB.idCommand = ToolbarButtons[i].id;
			nTB.fsState = TBSTATE_ENABLED;
			nTB.fsStyle = TBSTYLE_AUTOSIZE | ((ToolbarButtons[i].check == true)? TBSTYLE_CHECK : TBSTYLE_BUTTON);
			nTB.iString = ctrlToolbar.AddStrings(CTSTRING_I((ResourceManager::Strings)ToolbarButtons[i].tooltip));
		} else {
			continue;
		}
		ctrlToolbar.AddButtons(1, &nTB);
	}	

	ctrlToolbar.AutoSize();

// This way correct button height is preserved...
	TBBUTTONINFO tbi;
	memzero(&tbi, sizeof(TBBUTTONINFO));
	tbi.cbSize = sizeof(TBBUTTONINFO);
	tbi.dwMask = TBIF_STYLE;

	if(ctrlToolbar.GetButtonInfo(IDC_REFRESH_FILE_LIST, &tbi) != -1) {
		tbi.fsStyle |= BTNS_WHOLEDROPDOWN;
		ctrlToolbar.SetButtonInfo(IDC_REFRESH_FILE_LIST, &tbi);
	}

	return ctrlToolbar.m_hWnd;
}

LRESULT MainFrame::onWindowMinimizeAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	HWND tmpWnd = GetWindow(GW_CHILD); //getting client window
	tmpWnd = ::GetWindow(tmpWnd, GW_CHILD); //getting first child window
	while (tmpWnd!=NULL) {
		::CloseWindow(tmpWnd);
		tmpWnd = ::GetWindow(tmpWnd, GW_HWNDNEXT);
	}
	return 0;
}

LRESULT MainFrame::onSelected(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	HWND hWnd = (HWND)wParam;
	if(MDIGetActive() != hWnd) {
		MDIActivate(hWnd);
	} else if(SETTING(TOGGLE_ACTIVE_WINDOW)) {
		::SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
		MDINext(hWnd);
		hWnd = MDIGetActive();
	}
	if(::IsIconic(hWnd))
		::ShowWindow(hWnd, SW_RESTORE);
	return 0;
}

LRESULT MainFrame::onWindowRestoreAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	HWND tmpWnd = GetWindow(GW_CHILD); //getting client window
	HWND ClientWnd = tmpWnd; //saving client window handle
	tmpWnd = ::GetWindow(tmpWnd, GW_CHILD); //getting first child window
	BOOL bmax;
	while (tmpWnd!=NULL) {
		::ShowWindow(tmpWnd, SW_RESTORE);
		::SendMessage(ClientWnd,WM_MDIGETACTIVE,NULL,(LPARAM)&bmax);
		if(bmax)break; //bmax will be true if active child 
		//window is maximized, so if bmax then break
		tmpWnd = ::GetWindow(tmpWnd, GW_HWNDNEXT);
	}
	return 0;
}

LRESULT MainFrame::onTray(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { 
	updateTray(false);
	trayUID++;
	updateTray(true); 
	return 0;
}

LRESULT MainFrame::onRowsChanged(UINT /*uMsg*/, WPARAM /* wParam */, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	UpdateLayout();
	Invalidate();
	return 0;
}

void MainFrame::updateStatus(TStringList* aList) {
	checkAwayIdle();
	if(ctrlStatus.IsWindow()) {
		const auto& str = *aList;
		bool u = false;
		ctrlStatus.SetIcon(STATUS_AWAY, AirUtil::getAway() ? awayIconON : awayIconOFF);
		auto pos = 0;
		for(int i = STATUS_SHARED; i < STATUS_SHUTDOWN; i++) {

			const int w = WinUtil::getTextWidth(str[pos], ctrlStatus.m_hWnd);

			if(i == STATUS_SLOTS) {
				if(str[pos][0] == '0') // a hack, do this some other way.
					ctrlStatus.SetIcon(STATUS_SLOTS, slotsFullIcon);
				else
					ctrlStatus.SetIcon(STATUS_SLOTS, slotsIcon);
			}

			if(statusSizes[i-1] < w || statusSizes[i-1] > (w + 50) ) {
				statusSizes[i-1] = w;
				u = true;
			}

			ctrlStatus.SetText(i, str[pos].c_str());
			pos++;
		}

		if(u)
			UpdateLayout(TRUE);

		if (bShutdown) {
			uint64_t iSec = GET_TICK() / 1000;
			if(!isShutdownStatus) {
				ctrlStatus.SetIcon(STATUS_SHUTDOWN, hShutdownIcon);
				isShutdownStatus = true;
			}
			if (DownloadManager::getInstance()->getDownloadCount() > 0) {
				iCurrentShutdownTime = iSec;
				ctrlStatus.SetText(STATUS_SHUTDOWN, _T(""));
			} else {
				int64_t timeLeft = SETTING(SHUTDOWN_TIMEOUT) - (iSec - iCurrentShutdownTime);
				ctrlStatus.SetText(STATUS_SHUTDOWN, (_T(" ") + Util::formatSecondsW(timeLeft, timeLeft < 3600)).c_str(), SBT_POPOUT);
				if (iCurrentShutdownTime + SETTING(SHUTDOWN_TIMEOUT) <= iSec) {
					bool bDidShutDown = false;
					bDidShutDown = WinUtil::shutDown(SETTING(SHUTDOWN_ACTION));
					if (bDidShutDown) {
						// Should we go faster here and force termination?
						// We "could" do a manual shutdown of this app...
					} else {
						LogManager::getInstance()->message(STRING(FAILED_TO_SHUTDOWN), LogManager::LOG_ERROR);
						ctrlStatus.SetText(STATUS_SHUTDOWN, _T(""));
					}
					// We better not try again. It WON'T work...
					bShutdown = false;
				}
			}
		} else {
			if(isShutdownStatus) {
				ctrlStatus.SetIcon(STATUS_SHUTDOWN, NULL);
				isShutdownStatus = false;
			}
			ctrlStatus.SetText(STATUS_SHUTDOWN, _T(""));
		}
	}

	delete aList;
}

void MainFrame::on(LogManagerListener::Message, time_t t, const string& m, uint8_t sev) noexcept {
	callAsync([=] { addStatus(m, t, sev); });
}

void MainFrame::addStatus(const string& aMsg, time_t aTime, uint8_t severity) {
	if(ctrlStatus.IsWindow()) {
		tstring line = Text::toT("[" + Util::getTimeStamp(aTime) + "] " + aMsg);

		ctrlStatus.SetText(STATUS_LASTLINES, line.c_str());
		while(lastLinesList.size() + 1 > MAX_CLIENT_LINES)
			lastLinesList.pop_front();

		if (_tcschr(line.c_str(), _T('\r')) == NULL) {
			lastLinesList.push_back(line);
		} else {
			lastLinesList.push_back(line.substr(0, line.find(_T('\r'))));
		}

		switch(severity) {
		case LogManager::LOG_INFO:
			ctrlStatus.SetIcon(STATUS_LASTLINES, infoIcon);
			break;
		case LogManager::LOG_WARNING:
			ctrlStatus.SetIcon(STATUS_LASTLINES, warningIcon);
			break;
		case LogManager::LOG_ERROR:
			ctrlStatus.SetIcon(STATUS_LASTLINES, errorIcon);
			break;
		default:
			break;
		}
	}
}

void MainFrame::onChatMessage(bool pm) {
	if(!bIsPM && (!WinUtil::isAppActive || bAppMinimized)) { //using IsPM to avoid the 2 icons getting mixed up
		bIsPM = true;
		if (!pm) {
			if(taskbarList) {
				taskbarList->SetOverlayIcon(m_hWnd, hubicon.hIcon, NULL);
			}
			if(bTrayIcon) {
				NOTIFYICONDATA nid;
				ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
				nid.cbSize = sizeof(NOTIFYICONDATA);
				nid.hWnd = m_hWnd;
				nid.uID = trayUID;
				nid.uFlags = NIF_ICON;
				nid.hIcon = hubicon.hIcon;
				::Shell_NotifyIcon(NIM_MODIFY, &nid);
			}
		} else {
			if(taskbarList) {
				taskbarList->SetOverlayIcon(m_hWnd, pmicon.hIcon, NULL);
			}
			if(bTrayIcon) {
				NOTIFYICONDATA nid;
				ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
				nid.cbSize = sizeof(NOTIFYICONDATA);
				nid.hWnd = m_hWnd;
				nid.uID = trayUID;
				nid.uFlags = NIF_ICON;
				nid.hIcon = pmicon.hIcon;
				::Shell_NotifyIcon(NIM_MODIFY, &nid);
			}
		}
	}
}

void MainFrame::parseCommandLine(const tstring& cmdLine)
{
	string::size_type i = 0;
	string::size_type j;

	if( (j = cmdLine.find(_T("dchub://"), i)) != string::npos ||
		(j = cmdLine.find(_T("adc://"), i)) != string::npos ||
		(j = cmdLine.find(_T("adcs://"), i)) != string::npos ||
		(j = cmdLine.find(_T("magnet:?"), i)) != string::npos )
	{
		WinUtil::parseDBLClick(cmdLine.substr(j));
	}
}

LRESULT MainFrame::onCopyData(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	tstring cmdLine = (LPCTSTR) (((COPYDATASTRUCT *)lParam)->lpData);
	parseCommandLine(Text::toT(Util::getAppName() + " ") + cmdLine);
	return true;
}

LRESULT MainFrame::onHashProgress(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	//HashProgressDlg(false).DoModal(m_hWnd);
	switch(wID) {
		case IDC_HASH_PROGRESS:
			if( !hashProgress.IsWindow() ){
				hashProgress.setAutoClose(false);
				hashProgress.Create( m_hWnd );
				hashProgress.ShowWindow( SW_SHOW );
			}
			break;
		case IDC_HASH_PROGRESS_AUTO_CLOSE:
			if( !hashProgress.IsWindow() ){
				hashProgress.setAutoClose(true);
				hashProgress.Create( m_hWnd );
				hashProgress.ShowWindow( SW_SHOW );
			}
			break;
		default: break;
	}
	return 0;
}

LRESULT MainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	AboutDlg dlg;
	dlg.DoModal(m_hWnd);


	return 0;
}
LRESULT MainFrame::OnWizard(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	SetupWizard dlg;
	if (dlg.DoModal(m_hWnd) == IDOK) {
		PropPage::TaskList tasks;
		dlg.deletePages(tasks);
		if (!tasks.empty()) {
			addThreadedTask([=] {
				for(auto& t: tasks) {
					t.first();
					delete t.second;
				}
			});
		}
	}
	return 0;
}
LRESULT MainFrame::onOpenWindows(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	switch(wID) {
		case ID_FILE_SEARCH: SearchFrame::openWindow(); break;
		case ID_FILE_CONNECT: PublicHubsFrame::openWindow(); break;
		case IDC_FAVORITES: FavoriteHubsFrame::openWindow(); break;
		case IDC_FAVUSERS: UsersFrame::openWindow(); break;
		case IDC_NOTEPAD: NotepadFrame::openWindow(); break;
		case IDC_QUEUE: QueueFrame::openWindow(); break;
		case IDC_SEARCH_SPY: SpyFrame::openWindow(); break;
		case IDC_FILE_ADL_SEARCH: ADLSearchFrame::openWindow(); break;
		case IDC_NET_STATS: StatsFrame::openWindow(); break; 
		case IDC_FINISHED: FinishedFrame::openWindow(); break;
		case IDC_FINISHED_UL: FinishedULFrame::openWindow(); break;
		case IDC_UPLOAD_QUEUE: UploadQueueFrame::openWindow(); break;
		case IDC_CDMDEBUG_WINDOW: CDMDebugFrame::openWindow(); break;
		case IDC_RECENTS: RecentHubsFrame::openWindow(); break;
		case IDC_SYSTEM_LOG: SystemFrame::openWindow(); break;
		case IDC_AUTOSEARCH: AutoSearchFrame::openWindow(); break;

	
		default: dcassert(0); break;
	}
	return 0;
}

LRESULT MainFrame::OnFileSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	openSettings(WinUtil::lastSettingPage);
	return 0;
}

void MainFrame::openSettings(uint16_t initialPage /*0*/) {
	PropertiesDlg dlg(m_hWnd, SettingsManager::getInstance(), initialPage);

	auto prevTCP = SETTING(TCP_PORT);
	auto prevUDP = SETTING(UDP_PORT);
	auto prevTLS = SETTING(TLS_PORT);

	auto prevConn4 = SETTING(INCOMING_CONNECTIONS);
	auto prevConn6 = SETTING(INCOMING_CONNECTIONS6);
	auto prevMapper = SETTING(MAPPER);
	auto prevBind = SETTING(BIND_ADDRESS);
	auto prevBind6 = SETTING(BIND_ADDRESS6);
	auto prevProxy = CONNSETTING(OUTGOING_CONNECTIONS);


	auto prevGeo = SETTING(GET_USER_COUNTRY);
	auto prevGeoFormat = SETTING(COUNTRY_FORMAT);

	bool lastSortFavUsersFirst = SETTING(SORT_FAVUSERS_FIRST);

	auto prevHighPrio = SETTING(HIGH_PRIO_FILES);
	auto prevHighPrioRegex = SETTING(HIGHEST_PRIORITY_USE_REGEXP);

	auto prevShareSkiplist = SETTING(SKIPLIST_SHARE);
	auto prevShareSkiplistRegex = SETTING(SHARE_SKIPLIST_USE_REGEXP);

	auto prevDownloadSkiplist = SETTING(SKIPLIST_DOWNLOAD);
	auto prevDownloadSkiplistRegex = SETTING(DOWNLOAD_SKIPLIST_USE_REGEXP);

	auto prevFreeSlotMatcher = SETTING(FREE_SLOTS_EXTENSIONS);
	auto prevTranslation = SETTING(LANGUAGE_FILE);

	if(dlg.DoModal(m_hWnd) == IDOK) 
	{
		PropPage::TaskList tasks;
		dlg.deletePages(tasks);

		addThreadedTask([=] {
			for(auto& t: tasks) {
				t.first();
				delete t.second;
			}

			SettingsManager::getInstance()->save();

			if (prevTranslation != SETTING(LANGUAGE_FILE)) {
				UpdateManager::getInstance()->checkLanguage();
			}

			bool v4Changed = SETTING(INCOMING_CONNECTIONS) != prevConn4 ||
				SETTING(TCP_PORT) != prevTCP || SETTING(UDP_PORT) != prevUDP || SETTING(TLS_PORT) != prevTLS ||
				SETTING(MAPPER) != prevMapper || SETTING(BIND_ADDRESS) != prevBind || SETTING(BIND_ADDRESS6) != prevBind6;

			bool v6Changed = SETTING(INCOMING_CONNECTIONS6) != prevConn6 ||
				SETTING(TCP_PORT) != prevTCP || SETTING(UDP_PORT) != prevUDP || SETTING(TLS_PORT) != prevTLS ||
				/*SETTING(MAPPER) != prevMapper ||*/ SETTING(BIND_ADDRESS6) != prevBind6;

			try {
				ConnectivityManager::getInstance()->setup(v4Changed, v6Changed);
			} catch (const Exception& e) {
				showPortsError(e.getError());
			}

			auto outConns = CONNSETTING(OUTGOING_CONNECTIONS);
			if(outConns != prevProxy || outConns == SettingsManager::OUTGOING_SOCKS5) {
				Socket::socksUpdated();
			}


			ClientManager::getInstance()->infoUpdated();


			if (prevHighPrio != SETTING(HIGH_PRIO_FILES) || prevHighPrioRegex != SETTING(HIGHEST_PRIORITY_USE_REGEXP) || prevDownloadSkiplist != SETTING(SKIPLIST_DOWNLOAD) ||
				prevDownloadSkiplistRegex != SETTING(DOWNLOAD_SKIPLIST_USE_REGEXP)) {
			
					QueueManager::getInstance()->setMatchers();
			}

			if (prevShareSkiplist != SETTING(SKIPLIST_SHARE) || prevShareSkiplistRegex != SETTING(SHARE_SKIPLIST_USE_REGEXP)) {
				ShareManager::getInstance()->setSkipList();
			}

			if (prevFreeSlotMatcher != SETTING(FREE_SLOTS_EXTENSIONS)) {
				UploadManager::getInstance()->setFreeSlotMatcher();
			}

			bool rebuildGeo = prevGeo && SETTING(COUNTRY_FORMAT) != prevGeoFormat;
			if(SETTING(GET_USER_COUNTRY) != prevGeo) {
				if(SETTING(GET_USER_COUNTRY)) {
					GeoManager::getInstance()->init();
					UpdateManager::getInstance()->checkGeoUpdate();
				} else {
					GeoManager::getInstance()->close();
					rebuildGeo = false;
				}
			}
			if(rebuildGeo) {
				GeoManager::getInstance()->rebuild();
			}
		});

		if(SETTING(DISCONNECT_SPEED) < 1) {
			SettingsManager::getInstance()->set(SettingsManager::DISCONNECT_SPEED, 1);
		}
 
		if(SETTING(SORT_FAVUSERS_FIRST) != lastSortFavUsersFirst)
			HubFrame::resortUsers();

		if(SETTING(URL_HANDLER)) {
			WinUtil::registerDchubHandler();
			WinUtil::registerADChubHandler();
		WinUtil::registerADCShubHandler();
			WinUtil::urlDcADCRegistered = true;
		} else if(WinUtil::urlDcADCRegistered) {
			WinUtil::unRegisterDchubHandler();
			WinUtil::unRegisterADChubHandler();
			WinUtil::unRegisterADCShubHandler();
			WinUtil::urlDcADCRegistered = false;
		}
		if(SETTING(MAGNET_REGISTER)) {
			WinUtil::registerMagnetHandler();
			WinUtil::urlMagnetRegistered = true; 
		} else if(WinUtil::urlMagnetRegistered) {
			WinUtil::unRegisterMagnetHandler();
			WinUtil::urlMagnetRegistered = false;
		}



		if(AirUtil::getAway()) ctrlToolbar.CheckButton(IDC_AWAY, true);
		else ctrlToolbar.CheckButton(IDC_AWAY, false);

		if(getShutDown()) ctrlToolbar.CheckButton(IDC_SHUTDOWN, true);
		else ctrlToolbar.CheckButton(IDC_SHUTDOWN, false);
	
		if(tabsontop != SETTING(TABS_ON_TOP)) {
			tabsontop = SETTING(TABS_ON_TOP);
			UpdateLayout();
		}

		if(missedAutoConnect && !SETTING(NICK).empty()) {
			autoConnect(FavoriteManager::getInstance()->getFavoriteHubs());
		}
	}
}

LRESULT MainFrame::onGetToolTip(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMTTDISPINFO pDispInfo = (LPNMTTDISPINFO)pnmh;
	pDispInfo->szText[0] = 0;

	if(idCtrl == STATUS_LASTLINES+POPUP_UID) {
		lastLines.clear();
		for(const auto& i : lastLinesList) {
			lastLines += i;
			lastLines += _T("\r\n");
		}
		if(lastLines.size() > 2) {
			lastLines.erase(lastLines.size() - 2);
		}
		pDispInfo->lpszText = const_cast<TCHAR*>(lastLines.c_str());

	} else if(idCtrl == STATUS_AWAY+POPUP_UID) {
		pDispInfo->lpszText = AirUtil::getAway() ? (LPWSTR)CTSTRING(AWAY_ON) : (LPWSTR)CTSTRING(AWAY_OFF);
	}
	return 0;
}

void MainFrame::autoConnect(const FavoriteHubEntry::List& fl) {
		
	missedAutoConnect = false;
	for(const auto entry: fl) {
		if(entry->getConnect()) {
 			if(!SETTING(NICK).empty()) {
				RecentHubEntry r;
				r.setName(entry->getName());
				r.setDescription(entry->getDescription());
				r.setUsers("*");
				r.setShared("*");
				r.setServer(entry->getServers()[0].first);
				FavoriteManager::getInstance()->addRecent(r);
				HubFrame::openWindow(Text::toT(entry->getServers()[0].first), entry->getChatUserSplit(), entry->getUserListState());
 			} else
 				missedAutoConnect = true;
 		}				
	}
}

void MainFrame::updateTray(bool add /* = true */) {
	if(add) {
		if(!bTrayIcon) {
			NOTIFYICONDATA nid;
			ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = m_hWnd;
			nid.uID = trayUID;
			nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
			nid.uCallbackMessage = WM_APP + 242;
			nid.hIcon = GetIcon(false);
			//nid.hIcon = mainSmallIcon;
			_tcsncpy(nid.szTip, _T(APPNAME), 64);
			nid.szTip[63] = '\0';
			lastMove = GET_TICK() - 1000;
			bTrayIcon = (::Shell_NotifyIcon(NIM_ADD, &nid) != FALSE);
		}
	} else {
		if(bTrayIcon) {
			NOTIFYICONDATA nid;
			ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = m_hWnd;
			nid.uID = trayUID;
			nid.uFlags = 0;
			::Shell_NotifyIcon(NIM_DELETE, &nid);
			//ShowWindow(SW_SHOW);
			bTrayIcon = false;
		}
	}
}

LRESULT MainFrame::onOpen(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if(SETTING(MINIMIZE_TRAY) && SETTING(PASSWD_PROTECT_TRAY) && bAppMinimized && !WinUtil::checkClientPassword()) {
		ShowWindow(SW_HIDE);
		return FALSE;
	}

	return TRUE;
}

LRESULT MainFrame::onSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	if(wParam == SIZE_MINIMIZED) {
		/*maybe make this an option? with large amount of ram this is kinda obsolete,
		will look good in taskmanager ram usage tho :) */
		if(SETTING(DECREASE_RAM)) {
			if(!SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1))
				LogManager::getInstance()->message("Minimize Process WorkingSet Failed: "+ Util::translateError(GetLastError()), LogManager::LOG_WARNING);
		}

		if(SETTING(AUTO_AWAY) && (bAppMinimized == false) ) {
			
			if(AirUtil::getAwayMode() < AWAY_MANUAL) {
				AirUtil::setAway(AWAY_MINIMIZE);
				setAwayButton(true);
			}
		}
		bAppMinimized = true; //set this here, on size is called twice if minimize to tray.

		if(SETTING(MINIMIZE_TRAY) != WinUtil::isShift()) {
			ShowWindow(SW_HIDE);
			SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
		}
		
		maximized = IsZoomed() > 0;
	} else if( (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED) ) {
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
		if(SETTING(AUTO_AWAY)) {
			if(AirUtil::getAwayMode() < AWAY_MANUAL) {
				AirUtil::setAway(AWAY_OFF);
				setAwayButton(false);
			}
		}
		if(bIsPM) {
			bIsPM = false;
			if(taskbarList) {
				taskbarList->SetOverlayIcon(m_hWnd, NULL , NULL);
			}

			if (bTrayIcon) {
				NOTIFYICONDATA nid;
				ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
				nid.cbSize = sizeof(NOTIFYICONDATA);
				nid.hWnd = m_hWnd;
				nid.uID = trayUID;
				nid.uFlags = NIF_ICON;
				nid.hIcon = GetIcon(false);
				//nid.hIcon = mainSmallIcon;
				::Shell_NotifyIcon(NIM_MODIFY, &nid);
			}
		}
		bAppMinimized = false;
	}

	bHandled = FALSE;
	return 0;
}

LRESULT MainFrame::onEndSession(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	GetWindowPlacement(&wp);
	
	CRect rc;
	GetWindowRect(rc);
	
	if(wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWNORMAL) {
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_POS_X, rc.left);
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_POS_Y, rc.top);
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_SIZE_X, rc.Width());
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_SIZE_Y, rc.Height());
	}
	if(wp.showCmd == SW_SHOWNORMAL || wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_MAXIMIZE)
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_STATE, (int)wp.showCmd);
	
	QueueManager::getInstance()->saveQueue(false);
	SettingsManager::getInstance()->save();
	HashManager::getInstance()->closeDB();

	return 0;
}

void MainFrame::showMessageBox(const tstring& aMsg, UINT aFlags, const tstring& aTitle) {
	::MessageBox(WinUtil::splash ? WinUtil::splash->getHWND() : m_hWnd, aMsg.c_str(), (!aTitle.empty() ? aTitle.c_str() : _T(APPNAME) _T(" ") _T(VERSIONSTRING)), aFlags);
}

LRESULT MainFrame::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closing) {
		if(UpdateManager::getInstance()->isUpdating()) {
			showMessageBox(CTSTRING(UPDATER_IN_PROGRESS), MB_OK | MB_ICONINFORMATION);

			bHandled = TRUE;
			return 0;
		}
		
		if( oldshutdown || WinUtil::MessageBoxConfirm(SettingsManager::CONFIRM_EXIT, TSTRING(REALLY_EXIT)) ) {
			
			HubFrame::ShutDown();

			WinUtil::splash = unique_ptr<SplashWindow>(new SplashWindow());
			(*WinUtil::splash)(STRING(CLOSING_WINDOWS));

			updateTray(false);

			WinUtil::saveReBarSettings(m_hWndToolBar);

			if( hashProgress.IsWindow() )
				hashProgress.DestroyWindow();

			WINDOWPLACEMENT wp;
			wp.length = sizeof(wp);
			GetWindowPlacement(&wp);

			CRect rc;
			GetWindowRect(rc);
			if(SETTING(SHOW_TRANSFERVIEW)) {
				SettingsManager::getInstance()->set(SettingsManager::TRANSFER_SPLIT_SIZE, m_nProportionalPos);
			}
			if(wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWNORMAL) {
				SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_POS_X, rc.left);
				SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_POS_Y, rc.top);
				SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_SIZE_X, rc.Width());
				SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_SIZE_Y, rc.Height());
			}
			if(wp.showCmd == SW_SHOWNORMAL || wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_MAXIMIZE)
				SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_STATE, (int)wp.showCmd);

			ShowWindow(SW_HIDE);
			transferView.prepareClose();
			
			SearchManager::getInstance()->disconnect();
			ConnectionManager::getInstance()->disconnect();

			DWORD id;
			stopperThread = CreateThread(NULL, 0, stopper, this, 0, &id);
			closing = true;
		}
		bHandled = TRUE;
	} else {
		// This should end immediately, as it only should be the stopper that sends another WM_CLOSE
		WaitForSingleObject(stopperThread, 60*1000);
		CloseHandle(stopperThread);
		stopperThread = NULL;
		DestroyIcon(hShutdownIcon); 	
		DestroyIcon(pmicon.hIcon);
		DestroyIcon(hubicon.hIcon);
		DestroyIcon(slotsIcon);
		DestroyIcon(slotsFullIcon);
		bHandled = FALSE;
	}

	return 0;
}

LRESULT MainFrame::onSwitchWindow(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlTab.SwitchWindow(wID - IDC_SWITCH_WINDOW_1);
	return 0;
}

LRESULT MainFrame::onSetFont(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	HubFrame::updateFonts();
	ctrlTab.redraw();
	UpdateLayout();
	return 0;
}

LRESULT MainFrame::onLink(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	tstring site;
	bool isFile = false;
	switch(wID) {
		case IDC_HELP_HOMEPAGE: site = Text::toT(UpdateManager::getInstance()->links.homepage); break;
		case IDC_HELP_GUIDES: site = Text::toT(UpdateManager::getInstance()->links.guides); break;
		case IDC_HELP_DISCUSS: site = Text::toT(UpdateManager::getInstance()->links.discuss); break;
		case IDC_HELP_CUSTOMIZE: site = Text::toT(UpdateManager::getInstance()->links.customize); break;
		default: dcassert(0);
	}

	if(isFile)
		WinUtil::openFile(site);
	else
		WinUtil::openLink(site);

	return 0;
}

void MainFrame::getMagnetForFile() {
	tstring file;
	if(WinUtil::browseFile(file, m_hWnd, false, lastTTHdir) == IDOK) {
		WinUtil::mainMenu.EnableMenuItem(ID_GET_TTH, MF_GRAYED);

		auto path = Text::fromT(file);
		TTHValue tth;
		try {
			auto size = File::getSize(path);
			auto sizeLeft = size;
			HashManager::getInstance()->getFileTTH(path, size, false, tth, sizeLeft, closing);
			if (closing)
				return;

			string magnetlink = WinUtil::makeMagnet(tth, Util::getFileName(path), size);

			CInputBox ibox(m_hWnd);
			ibox.DoModal(_T("Tiger Tree Hash"), file.c_str(), Text::toT(tth.toBase32()).c_str(), Text::toT(magnetlink).c_str());
		} catch(...) { }
		WinUtil::mainMenu.EnableMenuItem(ID_GET_TTH, MF_ENABLED);
	}
}

LRESULT MainFrame::onGetTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	addThreadedTask([this] { getMagnetForFile(); });
	return 0;
}

void MainFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */)
{
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	
	if(ctrlStatus.IsWindow() && ctrlTooltips.IsWindow()) {
		CRect sr;
		int w[11];
		ctrlStatus.GetClientRect(sr);
		w[10] = sr.right - 20;
		w[9] = w[10] - 20;
#define setw(x) w[x] = max(w[x+1] - (statusSizes[x]+ICON_SPACE), 0)
		setw(8); setw(7); setw(6); setw(5); setw(4); setw(3); setw(2); setw(1); setw(0);

		ctrlStatus.SetParts(11, w);
		ctrlTooltips.SetMaxTipWidth(w[0]);
		
		updateTooltipRect();
	}
	CRect rc = rect;
	if(tabsontop == false)
		rc.top = rc.bottom - ctrlTab.getHeight();
	else
		rc.bottom = rc.top + ctrlTab.getHeight();
	if(ctrlTab.IsWindow())
		ctrlTab.MoveWindow(rc);
	

	CRect rc2 = rect;
	if(tabsontop == false)
		rc2.bottom = rc.top;
	else
		rc2.top = rc.bottom;
	SetSplitterRect(rc2);
}

LRESULT MainFrame::onOpenOwnList(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ProfileToken profile = SETTING(LAST_LIST_PROFILE);
	auto profiles = ShareManager::getInstance()->getProfiles();
	if (boost::find_if(profiles, [profile](const ShareProfilePtr& aProfile) { return aProfile->getToken() == profile; }) == profiles.end())
		profile = SETTING(DEFAULT_SP);

	DirectoryListingManager::getInstance()->openOwnList(profile, wID == IDC_OWN_LIST_ADL);
	return 0;
}

LRESULT MainFrame::onOpenFileList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	const TCHAR types[] = _T("File Lists\0*.DcLst;*.xml.bz2\0All Files\0*.*\0");

	tstring file;
	if (WinUtil::browseFile(file, m_hWnd, false, Text::toT(Util::getListPath()), TSTRING(OPEN_FILE_LIST), types)) {
		UserPtr u = DirectoryListing::getUserFromFilename(Text::fromT(file));
		if(u) {
			DirectoryListingManager::getInstance()->openFileList(HintedUser(u, Util::emptyString), Text::fromT(file));
		} else {
			MessageBox(CTSTRING(INVALID_LISTNAME), _T(APPNAME) _T(" ") _T(VERSIONSTRING));
		}
	}
	return 0;
}

LRESULT MainFrame::onRefreshFileList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	addThreadedTask([] { ShareManager::getInstance()->refresh(false, ShareManager::TYPE_MANUAL); });
	return 0;
}

LRESULT MainFrame::onScanMissing(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	addThreadedTask([] { ShareScannerManager::getInstance()->scan(); });
	return 0;
}

LRESULT MainFrame::onTrayIcon(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	if (lParam == WM_LBUTTONUP) {
		if(bAppMinimized) {
			ShowWindow(SW_SHOW);
			ShowWindow(maximized ? SW_MAXIMIZE : SW_RESTORE);
		} else {
			ShowWindow(SW_HIDE);
			ShowWindow(SW_MINIMIZE);
			//SetForegroundWindow(m_hWnd);
		}
	} else if(lParam == WM_MOUSEMOVE && ((lastMove + 1000) < GET_TICK()) ) {
		NOTIFYICONDATA nid;
		ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = m_hWnd;
		nid.uID = trayUID;
		nid.uFlags = NIF_TIP;
		_tcsncpy(nid.szTip, (_T("D: ") + Util::formatBytesW(DownloadManager::getInstance()->getRunningAverage()) + _T("/s (") + 
			Util::toStringW(DownloadManager::getInstance()->getDownloadCount()) + _T(")\r\nU: ") +
			Util::formatBytesW(UploadManager::getInstance()->getRunningAverage()) + _T("/s (") + 
			Util::toStringW(UploadManager::getInstance()->getUploadCount()) + _T(")")
			+ _T("\r\nUptime: ") + Util::formatSecondsW(Util::getUptime())
			).c_str(), 64);
		
		::Shell_NotifyIcon(NIM_MODIFY, &nid);
		lastMove = GET_TICK();
	} else if (lParam == WM_RBUTTONUP) {
		onTrayMenu();
	}
	return 0;
}

void MainFrame::onTrayMenu() {
	OMenu trayMenu;
	trayMenu.CreatePopupMenu();
	trayMenu.AppendMenu(MF_STRING, IDC_TRAY_SHOW, bAppMinimized ? CTSTRING(MENU_SHOW) : CTSTRING(MENU_HIDE));
	trayMenu.AppendMenu(MF_STRING, IDC_OPEN_DOWNLOADS, CTSTRING(MENU_OPEN_DOWNLOADS_DIR));
	trayMenu.AppendMenu(MF_SEPARATOR);
	trayMenu.AppendMenu(MF_STRING, IDC_REFRESH_FILE_LIST, CTSTRING(MENU_REFRESH_FILE_LIST));
	trayMenu.AppendMenu(MF_SEPARATOR);

	fillLimiterMenu(trayMenu.createSubMenu(CTSTRING(DOWNLOAD_LIMIT)), false);
	fillLimiterMenu(trayMenu.createSubMenu(CTSTRING(UPLOAD_LIMIT)), true);

	trayMenu.AppendMenu(MF_STRING, ID_APP_ABOUT, CTSTRING(MENU_ABOUT));
	trayMenu.AppendMenu(MF_STRING, ID_APP_EXIT, CTSTRING(MENU_EXIT));
	trayMenu.SetMenuDefaultItem(IDC_TRAY_SHOW);

	CPoint pt(GetMessagePos());
	SetForegroundWindow(m_hWnd);
	trayMenu.open(m_hWnd);
	PostMessage(WM_NULL, 0, 0);
}

void MainFrame::fillLimiterMenu(OMenu* limiterMenu, bool upload) {
	const auto title = upload ? CTSTRING(UPLOAD_LIMIT) : CTSTRING(DOWNLOAD_LIMIT);
	limiterMenu->InsertSeparatorFirst(title);
	//menu->setTitle(title, menuIcon);

	const auto setting = ThrottleManager::getCurSetting(
		upload ? SettingsManager::MAX_UPLOAD_SPEED_MAIN : SettingsManager::MAX_DOWNLOAD_SPEED_MAIN);
	auto x = SettingsManager::getInstance()->get(setting);
	bool disabled = x > 0 ? false : true;
	auto lineSpeed = upload ? Util::toDouble(SETTING(UPLOAD_SPEED)) * 1000 / 8 : Util::toDouble(SETTING(DOWNLOAD_SPEED)) * 1000 / 8;
	if(!x) {
		x = lineSpeed;
	}

	int arr[] = { 0 /* disabled */, x /* current value */,
		x + 1, x + 2, x + 5, x + 10, x + 20, x + 50, x + 100,
		x - 1, x - 2, x - 5, x - 10, x - 20, x - 50, x - 100,
		x * 11 / 10, x * 6 / 5, x * 3 / 2, x * 2, x * 3, x * 4, x * 5, x * 10, x * 100,
		x * 10 / 11, x * 5 / 6, x * 2 / 3, x / 2, x / 3, x / 4, x / 5, x / 10, x / 100 };

	// set ensures ordered unique members; remove_if performs range and relevancy checking.
	auto minDelta = (x >= 1000) ? (20 * pow(1000, floor(log(static_cast<float>(x)) / log(1000.)) - 1)) :
		(x >= 100) ? 5 : 0; // aint 5KB/s accurate enough?
	
	set<int> values(arr, std::remove_if(arr, arr + sizeof(arr) / sizeof(int), [x, minDelta, lineSpeed](int i) {
		return i < 0 || 
			i > ThrottleManager::MAX_LIMIT || 
			(x == lineSpeed && (i > (lineSpeed*1.2))) || // dont show over 1.2 * connectionspeed as default
			(minDelta && i != x && abs(i - x) < minDelta); // not too close to the original value
	}));

	for(auto value: values) {
		auto enabled = (disabled && !value) || (!disabled && value == x);
		auto formatted = Util::formatBytesW(Util::convertSize(value, Util::KB));
		limiterMenu->appendItem(value ? formatted + _T("/s") : CTSTRING(DISABLED),
			[setting, value] { ThrottleManager::setSetting(setting, value); }, enabled ? OMenu::FLAG_CHECKED | OMenu::FLAG_DISABLED : 0);

		if(!value)
			limiterMenu->AppendMenu(MF_SEPARATOR);
	}

	limiterMenu->AppendMenu(MF_SEPARATOR);
	limiterMenu->appendItem(CTSTRING(CUSTOM), [this, title, setting, x] {
		LineDlg dlg;
		dlg.title = title;
		dlg.description = CTSTRING(NEW_LIMIT);
		if(dlg.DoModal() == IDOK) {
			ThrottleManager::setSetting(setting, Util::toUInt(Text::fromT(dlg.line)));
		}
	});
}

LRESULT MainFrame::onStatusBarClick(UINT uMsg, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	OMenu menu;
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	CRect DLrect;
	CRect ULrect;
	CRect Awayrect;
	/* Currently the menu appears by clicking anywhere on the speed regions
	it could be narrowed down by limiting the rect, or using the icons as a dummy hwnds */
	ctrlStatus.GetRect(STATUS_DL_SPEED, DLrect);
	ctrlStatus.GetRect(STATUS_UL_SPEED, ULrect);
	ctrlStatus.GetRect(STATUS_AWAY, Awayrect);
	if(PtInRect(&DLrect, pt)) {
		menu.CreatePopupMenu();
		fillLimiterMenu(&menu, false);
		menu.open(ctrlStatus.m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON);
	} else if(PtInRect(&ULrect, pt)) {
		menu.CreatePopupMenu();
		fillLimiterMenu(&menu, true);
		menu.open(ctrlStatus.m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON);
	} else if(PtInRect(&Awayrect, pt)) {
		if(uMsg == WM_RBUTTONUP) {
			LineDlg awaymsg;
			awaymsg.title = CTSTRING(AWAY);
			awaymsg.description = CTSTRING(SET_AWAY_MESSAGE);
			awaymsg.line = Text::toT(SETTING(DEFAULT_AWAY_MESSAGE));
			if(awaymsg.DoModal(m_hWnd) == IDOK) {
				string msg = Text::fromT(awaymsg.line);
				if(!msg.empty())
					SettingsManager::getInstance()->set(SettingsManager::DEFAULT_AWAY_MESSAGE, msg); 
			
				//set the away mode on if its not already.
				if(!AirUtil::getAway()) {
					setAwayButton(true);
					AirUtil::setAway(AWAY_MANUAL);
					ctrlStatus.SetIcon(STATUS_AWAY, awayIconON);
				}
			}
		} else {
			if(AirUtil::getAway()) { 
				setAwayButton(false);
				AirUtil::setAway(AWAY_OFF);
				ctrlStatus.SetIcon(STATUS_AWAY, awayIconOFF);
			} else {
				setAwayButton(true);
				AirUtil::setAway(AWAY_MANUAL);
				ctrlStatus.SetIcon(STATUS_AWAY, awayIconON);
			}
		}
	}
	bHandled = TRUE;
	return 0;
}

LRESULT MainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_TOOLBAR, bVisible ? true : false);
	return 0;
}

LRESULT MainFrame::OnViewWinampBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 2);	// toolbar is 3rd added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_TOGGLE_TOOLBAR, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_WINAMP_CONTROL, bVisible ? true : false);
	return 0;
}
LRESULT MainFrame::OnViewTBStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 3);	// toolbar is 4th added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_TOGGLE_TBSTATUS, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_TBSTATUS, bVisible ? true : false);
	return 0;
}
LRESULT MainFrame::OnLockTB(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL locked = FALSE;	// initially not locked
	locked = !locked;
	CReBarCtrl rebar = m_hWndToolBar;
	rebar.LockBands(locked? true : false);

	UISetCheck(ID_LOCK_TB, locked);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::LOCK_TB, locked ? true : false);
	return 0;
}

LRESULT MainFrame::onWinampStart(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	if(SETTING(WINAMP_PATH) != Util::emptyString) {
	::ShellExecute(NULL, NULL, Text::toT(SETTING(WINAMP_PATH)).c_str(), NULL , NULL, SW_SHOWNORMAL);
	}
	return 0;
}

void MainFrame::updateTBStatusHashing(const string& /*aFile*/, int64_t aSize, size_t aFiles, int64_t aSpeed, int /*aHashers*/, bool aPaused) {

	if(refreshing) {
		refreshing = false;
		progress.SetMarquee(FALSE);
		progress.ModifyStyle(PBS_MARQUEE, NULL);
		progress.SetText(_T(""));
		progress.SetPos(0);
		progress.Invalidate();
	}

	if(aSize > startBytes)
		startBytes = aSize;

	if(aFiles > startFiles)
		startFiles = aFiles;

	tstring tmp;
	if(aFiles == 0 || aSize == 0 || aPaused) {
		if(aPaused) {
			tmp += 	_T(" ( ") + TSTRING(PAUSED) + _T(" )");
		} else {
			tmp = _T("");
		}
	} else {
		int64_t filestat = aSpeed > 0 ? ((startBytes - (startBytes - aSize)) / aSpeed) : 0;

		tmp = Util::formatBytesW((int64_t)aSpeed) + _T("/s, ") + Util::formatBytesW(aSize) + _T(" ") + TSTRING(LEFT);
		
		if(filestat == 0 || aSpeed == 0) {
			tmp += Text::toT(", -:--:-- " + STRING(LEFT));	
		} else {
			tmp += _T(", ") + Util::formatSecondsW(filestat) + _T(" ") + TSTRING(LEFT);
		}
	}

	if(startFiles == 0 || startBytes == 0 || aFiles == 0) {
		if(startFiles != 0 && aFiles == 0) {
			progress.SetPosWithText((int)progress.GetRangeLimit(0), _T("Finished...")); //show 100% for one second.
		} else if(progress.GetPos() != 0 || aPaused || progress.GetTextLen() > 0) {//skip unnecessary window updates...
			progress.SetPosWithText(0, tmp);
		}
		startBytes = 0;
		startFiles = 0;
	} else {
		progress.SetPosWithText(((int)(progress.GetRangeLimit(0) * ((0.5 * (startFiles - aFiles)/startFiles) + 0.5 * (startBytes - aSize) / startBytes))), tmp);
	}
}

void MainFrame::updateTBStatusRefreshing() {
	if(refreshing)
		return;

	refreshing = true;
	progress.ModifyStyle(NULL, PBS_MARQUEE);
	progress.SetMarquee(TRUE, 10);
	progress.SetText(_T("Refreshing...")); // Dont translate, someone might make it too long to fit.
}

LRESULT MainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_STATUSBAR, bVisible ? true : false);
	return 0;
}

LRESULT MainFrame::OnViewTransferView(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !transferView.IsWindowVisible();
	if(!bVisible) {	
		if(GetSinglePaneMode() == SPLIT_PANE_NONE)
			SetSinglePaneMode(SPLIT_PANE_TOP);
	} else { 
		if(GetSinglePaneMode() != SPLIT_PANE_NONE)
			SetSinglePaneMode(SPLIT_PANE_NONE);
	}
	UISetCheck(ID_VIEW_TRANSFER_VIEW, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_TRANSFERVIEW, bVisible ? true : false);
	return 0;
}

LRESULT MainFrame::onCloseWindows(WORD , WORD wID, HWND , BOOL& ) {
	switch(wID) {
	case IDC_CLOSE_DISCONNECTED:		HubFrame::closeDisconnected();		break;
	case IDC_CLOSE_ALL_PM:				PrivateFrame::closeAll();			break;
	case IDC_CLOSE_ALL_OFFLINE_PM:		PrivateFrame::closeAllOffline();	break;
	case IDC_CLOSE_ALL_DIR_LIST:		DirectoryListingFrame::closeAll();	break;
	case IDC_CLOSE_ALL_SEARCH_FRAME:	SearchFrame::closeAll();			break;
	}
	return 0;
}
LRESULT MainFrame::onOpenSysLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	string filename = LogManager::getInstance()->getPath(LogManager::SYSTEM);
	if(Util::fileExists(filename)){
		WinUtil::viewLog(filename);
	} else {
		MessageBox(CTSTRING(NO_LOG_FOR_HUB),CTSTRING(NO_LOG_FOR_HUB), MB_OK );	  
	}
	
	return 0; 
}

LRESULT MainFrame::onReconnectDisconnected(WORD , WORD , HWND , BOOL& ) {
	HubFrame::reconnectDisconnected();
	return 0;
}

LRESULT MainFrame::onQuickConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	ConnectDlg dlg;
	dlg.title = TSTRING(QUICK_CONNECT);
	if(dlg.DoModal(m_hWnd) == IDOK){
		if(SETTING(NICK).empty())
			return 0;

		RecentHubEntry r;
		r.setName("*");
		r.setDescription("*");
		r.setUsers("*");
		r.setShared("*");
		r.setServer(Text::fromT(dlg.address));
		FavoriteManager::getInstance()->addRecent(r);
		HubFrame::openWindow(dlg.address, 0, true, dlg.curProfile);
	}
	return 0;
}

void MainFrame::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
	if(aTick == lastUpdate)	// FIXME: temp fix for new TimerManager
		return;

	Util::increaseUptime();
	uint64_t queueSize = QueueManager::getInstance()->getTotalQueueSize();
	int64_t totalDown = Socket::getTotalDown();
	int64_t totalUp = Socket::getTotalUp();

	int64_t diff = (int64_t)((lastUpdate == 0) ? aTick - 1000 : aTick - lastUpdate);
	int64_t updiff = totalUp - lastUp;
	int64_t downdiff = totalDown - lastDown;

	TStringList* str = new TStringList();
	str->push_back(Util::formatBytesW(ShareManager::getInstance()->getSharedSize()));
	str->push_back(Text::toT(Client::getCounts()));
	str->push_back(Util::toStringW(UploadManager::getInstance()->getFreeSlots()) + _T('/') + Util::toStringW(UploadManager::getInstance()->getSlots()) + _T(" (") + Util::toStringW(UploadManager::getInstance()->getFreeExtraSlots()) + _T('/') + Util::toStringW(SETTING(EXTRA_SLOTS)) + _T(")"));

	str->push_back(Util::formatBytesW(totalDown));
	str->push_back(Util::formatBytesW(totalUp));

	tstring down = _T("[") + Util::toStringW(DownloadManager::getInstance()->getDownloadCount()) + _T("][");
	tstring up = _T("[") + Util::toStringW(UploadManager::getInstance()->getUploadCount()) + _T("][");

	auto dl = ThrottleManager::getDownLimit();
	if (dl > 0)
		down += Util::formatBytesW(Util::convertSize(dl, Util::KB));
	else
		down += _T("-");

	auto ul = ThrottleManager::getUpLimit();
	if (ul > 0)
		up += Util::formatBytesW(Util::convertSize(ul, Util::KB));
	else
		up += _T("-");

	str->push_back(down + _T("] ") + Util::formatBytesW(downdiff*1000I64/diff) + _T("/s"));
	str->push_back(up + _T("] ") + Util::formatBytesW(updiff*1000I64/diff) + _T("/s"));
	str->push_back(Util::formatBytesW(queueSize < 0 ? 0 : queueSize));

	callAsync([=] { updateStatus(str); });
	SettingsManager::getInstance()->set(SettingsManager::TOTAL_UPLOAD, SETTING(TOTAL_UPLOAD) + updiff);
	SettingsManager::getInstance()->set(SettingsManager::TOTAL_DOWNLOAD, SETTING(TOTAL_DOWNLOAD) + downdiff);
	lastUpdate = aTick;
	lastUp = totalUp;
	lastDown = totalDown;

	if(TBStatusCtrl.IsWindowVisible()){
		if(ShareManager::getInstance()->isRefreshing()){
			callAsync([=] { updateTBStatusRefreshing(); });
		} else {
			string file;
			int64_t bytes = 0;
			size_t files = 0;
			int64_t speed = 0;
			int hashers = 0;
			bool paused = HashManager::getInstance()->isHashingPaused();
			HashManager::getInstance()->getStats(file, bytes, files, speed, hashers);
			callAsync([=] { updateTBStatusHashing(file, bytes, files, speed, hashers, paused); });
		}
	}

	if(currentPic != SETTING(BACKGROUND_IMAGE)) {
		currentPic = SETTING(BACKGROUND_IMAGE);
		callAsync([=] { m_PictureWindow.Load(Text::toT(currentPic).c_str()); });
	}

	time_t currentTime;
	time(&currentTime);
}

void MainFrame::on(QueueManagerListener::Finished, const QueueItemPtr& qi, const string& /*dir*/, const HintedUser& /*aUser*/, int64_t /*aSpeed*/) noexcept {
	if(!qi->isSet(QueueItem::FLAG_USER_LIST)) {
		// Finished file sound
		if(!SETTING(FINISHFILE).empty() && !SETTING(SOUNDS_DISABLED))
			WinUtil::playSound(Text::toT(SETTING(FINISHFILE)));
	}

	if(qi->isSet(QueueItem::FLAG_CLIENT_VIEW) && qi->isSet(QueueItem::FLAG_TEXT)) {
		callAsync([=] {
			TextFrame::openWindow(Text::toT(qi->getTarget()), TextFrame::NORMAL);
			File::deleteFile(qi->getTarget());
		});
	}
}

void MainFrame::on(ScannerManagerListener::ScanFinished, const string& aText, const string& aTitle) noexcept {
	callAsync([=] { TextFrame::openWindow(Text::toT(aTitle), Text::toT(aText), TextFrame::REPORT); });
}

void MainFrame::on(DirectoryListingManagerListener::OpenListing, DirectoryListing* aList, const string& aDir, const string& aXML) noexcept {
	callAsync([=] { DirectoryListingFrame::openWindow(aList, aDir, aXML); });
}

void MainFrame::on(DirectoryListingManagerListener::PromptAction, completionF aF, const string& aMessage) noexcept {
	bool accept = !SETTING(FREE_SPACE_WARN) || ::MessageBox(m_hWnd, Text::toT(aMessage).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES;
	//bool accept = WinUtil::MessageBoxConfirm(SettingsManager::FREE_SPACE_WARN, Text::toT(aMessage));
	//DirectoryListingManager::getInstance()->handleSizeConfirmation(aName, accept);
	aF(accept);
}

LRESULT MainFrame::onActivateApp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	bHandled = FALSE;
	WinUtil::isAppActive = (wParam == 1);	//wParam == TRUE if window is activated, FALSE if deactivated
	if(wParam == 1) {
		if(bIsPM) {
			bIsPM = false;

			if(taskbarList) {
				taskbarList->SetOverlayIcon(m_hWnd, NULL, NULL);
			}

			if (bTrayIcon == true) {
				NOTIFYICONDATA nid;
				ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
				nid.cbSize = sizeof(NOTIFYICONDATA);
				nid.hWnd = m_hWnd;
				nid.uID = trayUID;
				nid.uFlags = NIF_ICON;
				nid.hIcon = GetIcon(false);
				//nid.hIcon = mainSmallIcon;
				::Shell_NotifyIcon(NIM_MODIFY, &nid);
			}
		}
	}
	return 0;
}

LRESULT MainFrame::onAppCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	if(GET_APPCOMMAND_LPARAM(lParam) == APPCOMMAND_BROWSER_FORWARD) {
		ctrlTab.SwitchTo();
	} else if(GET_APPCOMMAND_LPARAM(lParam) == APPCOMMAND_BROWSER_BACKWARD) {
		ctrlTab.SwitchTo(false);
	} else {
		bHandled = FALSE;
	}
	
	return FALSE;
}

LRESULT MainFrame::onAway(WORD , WORD , HWND, BOOL& ) {
	if(AirUtil::getAway()) { 
		setAwayButton(false);
		AirUtil::setAway(AWAY_OFF);
	} else {
		setAwayButton(true);
		AirUtil::setAway(AWAY_MANUAL);
	}
	return 0;
}

LRESULT MainFrame::onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UpdateManager::getInstance()->checkVersion(true);
	return S_OK;
}

void MainFrame::TestWrite( bool downloads, bool incomplete, bool AppPath) {
	
	tstring error = Util::emptyStringT;
	string filename = (Util::getPath(Util::PATH_USER_CONFIG) + "testwrite.tmp");
	bool ready = false;
	File* f = NULL;
	//Create the test file first, we should allways be able to write Settings folder
	try {

	f = new File((filename), File::WRITE, File::OPEN | File::CREATE | File::TRUNCATE);

	} catch (const Exception&) {
		        //if cant write to settings no need to test anything more
				downloads = false;
				incomplete = false;
				AppPath = false;
				error += _T("Error: test write to Settings folder failed. \r\n");
	}

	if(downloads) {
		try {

			File::ensureDirectory(SETTING(DOWNLOAD_DIRECTORY));
			File::copyFile((filename), (SETTING(DOWNLOAD_DIRECTORY) + "testwrite.tmp"));
			if (Util::fileExists(SETTING(DOWNLOAD_DIRECTORY) + "testwrite.tmp")) {
				File::deleteFile(SETTING(DOWNLOAD_DIRECTORY) + "testwrite.tmp");
				ready = true;
			}
				} catch (const Exception&) {
				error += _T("Error: test write to Downloads folder failed. \r\n");
			}
	}

	if(incomplete && (SETTING(TEMP_DOWNLOAD_DIRECTORY).find("%[targetdrive]") == string::npos)) {
		try {
			File::ensureDirectory(SETTING(TEMP_DOWNLOAD_DIRECTORY));
			File::copyFile((filename), (SETTING(TEMP_DOWNLOAD_DIRECTORY) + "testwrite.tmp"));
			if (Util::fileExists(SETTING(TEMP_DOWNLOAD_DIRECTORY) + "testwrite.tmp")) {
				File::deleteFile(SETTING(TEMP_DOWNLOAD_DIRECTORY) + "testwrite.tmp");
				ready = true;
			}
				} catch (const Exception&) {
				error += _T("Error: test write to Incomplete Downloads folder failed. \r\n");
			}
	}

	if(AppPath) {
		try {
			File::copyFile((filename), (Util::getPath(Util::PATH_RESOURCES) + "testwrite.tmp"));
			if (Util::fileExists(Util::getPath(Util::PATH_RESOURCES) + "testwrite.tmp")) {
				File::deleteFile(Util::getPath(Util::PATH_RESOURCES) + "testwrite.tmp");
				ready = true;
			}
		} catch (const Exception&) {
			error += _T("Error: test write to Application folder failed. \r\n");
		}
	}



	//Delete Temp file
	
	if( f != NULL) {
		try {
			f->flush();
			delete f;
			f = NULL;
		} catch(const FileException&) { }
	}


	if (Util::fileExists(filename)) {
		File::deleteFile(filename);
		ready = true;
	}

	//report errors if any
	if( error != Util::emptyStringT) {
		error += _T("Check Your User Privileges or try running AirDC++ as administrator. \r\n");
		showMessageBox(error, MB_ICONWARNING | MB_OK);
	}
}


LRESULT MainFrame::onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	LogManager::getInstance()->removeListener(this);
	QueueManager::getInstance()->removeListener(this);
	TimerManager::getInstance()->removeListener(this);
	DirectoryListingManager::getInstance()->removeListener(this);
	UpdateManager::getInstance()->removeListener(this);
	ShareScannerManager::getInstance()->removeListener(this);

	//if(bTrayIcon) {
		updateTray(false);
	//}
	bHandled = FALSE;
	return 0;
}

LRESULT MainFrame::onRefreshDropDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMTOOLBAR tb = (LPNMTOOLBAR)pnmh;
	OMenu dropMenu;
	dropMenu.CreatePopupMenu();
	dropMenu.appendItem(CTSTRING(ALL), [] { ShareManager::getInstance()->refresh(false, ShareManager::TYPE_MANUAL); }, OMenu::FLAG_THREADED);
	dropMenu.appendItem(CTSTRING(INCOMING), [] { ShareManager::getInstance()->refresh(true, ShareManager::TYPE_MANUAL); }, OMenu::FLAG_THREADED);
	dropMenu.appendSeparator();

	auto l = ShareManager::getInstance()->getGroupedDirectories();
	for(auto& i: l) {
		if (i.second.size() > 1) {
			auto vMenu = dropMenu.createSubMenu(Text::toT(i.first).c_str(), true);
			vMenu->appendItem(CTSTRING(ALL), [=] { ShareManager::getInstance()->refresh(i.first); }, OMenu::FLAG_THREADED);
			vMenu->appendSeparator();
			for(const auto& s: i.second) {
				vMenu->appendItem(Text::toT(s).c_str(), [=] { ShareManager::getInstance()->refresh(s); }, OMenu::FLAG_THREADED);
			}
		} else {
			dropMenu.appendItem(Text::toT(i.first).c_str(), [=] { ShareManager::getInstance()->refresh(i.first); }, OMenu::FLAG_THREADED);
		}
	}

	POINT pt;
	pt.x = tb->rcButton.right;
	pt.y = tb->rcButton.bottom;
	ClientToScreen(&pt);
	
	dropMenu.open(m_hWnd, TPM_LEFTALIGN, pt);
	return TBDDRET_DEFAULT;
}

LRESULT MainFrame::onAppShow(WORD /*wNotifyCode*/,WORD /*wParam*/, HWND, BOOL& /*bHandled*/) {
	if (::IsIconic(m_hWnd)) {
		ShowWindow(SW_SHOW);
		ShowWindow(maximized ? SW_MAXIMIZE : SW_RESTORE);
	} else {
		ShowWindow(SW_HIDE);
		ShowWindow(SW_MINIMIZE);
	}
	return 0;
}
void MainFrame::checkAwayIdle() {
	if(SETTING(AWAY_IDLE_TIME) && (AirUtil::getAwayMode() <= AWAY_IDLE)) {
		LASTINPUTINFO info = { sizeof(LASTINPUTINFO) };
		bool awayIdle = (AirUtil::getAwayMode() == AWAY_IDLE);
		if((::GetLastInputInfo(&info) && static_cast<int>(::GetTickCount() - info.dwTime) > SETTING(AWAY_IDLE_TIME) * 60 * 1000) ^ awayIdle) {
			awayIdle ? AirUtil::setAway(AWAY_OFF) : AirUtil::setAway(AWAY_IDLE);
		}
	}
}

void MainFrame::on(UpdateManagerListener::UpdateFailed, const string& line) noexcept {
	//MessageBox(Text::toT(line).c_str());
	ShowPopup(Text::toT(line), CTSTRING(UPDATER), NIIF_ERROR, true);
}

void MainFrame::on(UpdateManagerListener::UpdateAvailable, const string& title, const string& message, const string& version, const string& infoUrl, bool autoUpdate, int build, const string& autoUpdateUrl) noexcept {
	callAsync([=] { onUpdateAvailable(title, message, version, infoUrl, autoUpdate, build, autoUpdateUrl); });
}

void MainFrame::on(UpdateManagerListener::BadVersion, const string& message, const string& infoUrl, const string& updateUrl, int buildID, bool autoUpdate) noexcept {
	callAsync([=] { onBadVersion(message, infoUrl, updateUrl, buildID, autoUpdate); });
}

void MainFrame::on(UpdateManagerListener::UpdateComplete, const string& aUpdater) noexcept {
	callAsync([=] { onUpdateComplete(aUpdater); });
}

void MainFrame::onUpdateAvailable(const string& title, const string& message, const string& version, const string& infoUrl, bool autoUpdate, int build, const string& autoUpdateUrl) noexcept {
	UpdateDlg dlg(title, message, version, infoUrl, autoUpdate, build, autoUpdateUrl);
	if (dlg.DoModal(m_hWnd)  == IDC_UPDATE_DOWNLOAD) {
		UpdateManager::getInstance()->downloadUpdate(autoUpdateUrl, build, true);
		ShowPopup(CTSTRING(UPDATER_START), CTSTRING(UPDATER), NIIF_INFO, true);
	}
}

void MainFrame::onBadVersion(const string& message, const string& infoUrl, const string& updateUrl, int buildID, bool autoUpdate) noexcept {
	bool canAutoUpdate = autoUpdate && UpdateDlg::canAutoUpdate(updateUrl);

	tstring title = Text::toT(STRING(MANDATORY_UPDATE) + " - " APPNAME " " VERSIONSTRING);
	showMessageBox(Text::toT(message + "\r\n\r\n" + (canAutoUpdate ? STRING(ATTEMPT_AUTO_UPDATE) : STRING(MANUAL_UPDATE_MSG))).c_str(), MB_OK | MB_ICONEXCLAMATION, title);

	if(!canAutoUpdate) {
		if(!infoUrl.empty())
			WinUtil::openLink(Text::toT(infoUrl));

		oldshutdown = true;
		PostMessage(WM_CLOSE);
	} else {
		if(!UpdateManager::getInstance()->isUpdating()) {
			UpdateManager::getInstance()->downloadUpdate(updateUrl, buildID, true);
			ShowPopup(CTSTRING(UPDATER_START), CTSTRING(UPDATER), NIIF_INFO, true);
		} else {
			showMessageBox(CTSTRING(UPDATER_IN_PROGRESS), MB_OK | MB_ICONINFORMATION);
		}
	}
}

void MainFrame::onUpdateComplete(const string& aUpdater) noexcept {
	if(::MessageBox(m_hWnd, CTSTRING(UPDATER_RESTART), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1) == IDYES) {
		WinUtil::addUpdate(aUpdater);

		oldshutdown = true;
		PostMessage(WM_CLOSE);
	}
}

void MainFrame::ShowPopup(tstring szMsg, tstring szTitle, DWORD dwInfoFlags, HICON hIcon, bool force) {
	callAsync([=] { PopupManager::getInstance()->Show(szMsg, szTitle, dwInfoFlags, hIcon, force); });
}
void MainFrame::updateTooltipRect() {
	CRect sr;
	for(uint8_t i = STATUS_LASTLINES; i != STATUS_SHUTDOWN; i++) {
		ctrlStatus.GetRect(i, sr);
		ctrlTooltips.SetToolRect(ctrlStatus.m_hWnd, i+POPUP_UID, sr);
	}
}