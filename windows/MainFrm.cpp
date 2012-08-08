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
#include "../client/HashManager.h"
#include "../client/UploadManager.h"
#include "../client/StringTokenizer.h"
#include "../client/SimpleXML.h"
#include "../client/ShareManager.h"
#include "../client/LogManager.h"
#include "../client/Thread.h"
#include "../client/FavoriteManager.h"
#include "../client/MappingManager.h"
#include "../client/ShareScannerManager.h"
#include "../client/AirUtil.h"
#include "../client/ScopedFunctor.h"
#include "../client/format.h"
#include "../client/HttpDownload.h"

MainFrame* MainFrame::anyMF = NULL;
bool MainFrame::bShutdown = false;
uint64_t MainFrame::iCurrentShutdownTime = 0;
bool MainFrame::isShutdownStatus = false;

MainFrame::MainFrame() : trayMessage(0), maximized(false), lastUpload(-1), lastUpdate(0), 
lastUp(0), lastDown(0), oldshutdown(false), stopperThread(NULL),
closing(false), awaybyminimize(false), missedAutoConnect(false), lastTTHdir(Util::emptyStringT), tabsontop(false),
bTrayIcon(false), bAppMinimized(false), bIsPM(false), hasPassdlg(false), hashProgress(false)


{ 
		if(WinUtil::getOsMajor() >= 6) {
			user32lib = LoadLibrary(_T("user32"));
			_d_ChangeWindowMessageFilter = (LPFUNC)GetProcAddress(user32lib, "ChangeWindowMessageFilter");
		}


		memzero(statusSizes, sizeof(statusSizes));
		anyMF = this;
}

MainFrame::~MainFrame() {
	m_CmdBar.m_hImageList = NULL;

	images.Destroy();
	ToolbarImages.Destroy();
	ToolbarImagesHot.Destroy();
	winampImages.Destroy();

	WinUtil::uninit();
	if(WinUtil::getOsMajor() >= 6)
		FreeLibrary(user32lib);
}

DWORD WINAPI MainFrame::stopper(void* p) {
	MainFrame* mf = (MainFrame*)p;
	HWND wnd, wnd2 = NULL;

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
		for(StringIter i = files.begin(); i != files.end(); ++i) {
			UserPtr u = DirectoryListing::getUserFromFilename(*i);
			if(!u)
				continue;
				
			HintedUser user(u, Util::emptyString);
			DirectoryListing* dl = new DirectoryListing(user, false);
			try {
				dl->loadFile(*i, false);
				int matches=0, newFiles=0;
				BundleList bundles;
				QueueManager::getInstance()->matchListing(*dl, matches, newFiles, bundles);
				LogManager::getInstance()->message(Util::toString(ClientManager::getInstance()->getNicks(user)) + ": " + 
					AirUtil::formatMatchResults(matches, newFiles, bundles, false), LogManager::LOG_INFO);
			} catch(const Exception&) {

			}
			delete dl;
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
	hasUpdate = false; // avoid update question if user double downloads update
	oldshutdown = true;
	PostMessage(WM_CLOSE);
}

LRESULT MainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	links.homepage = _T("http://www.airdcpp.net/");
	links.downloads = links.homepage + _T("download/");
	links.geoip6 = _T("http://geoip6.airdcpp.net");
	links.geoip4 = _T("http://geoip4.airdcpp.net");
	links.guides = links.homepage + _T("guides/");
	links.customize = links.homepage + _T("c/customizations/");
	links.discuss = links.homepage + _T("forum/");

	TimerManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);
	LogManager::getInstance()->addListener(this);

	hasUpdate = false; 
	conns[CONN_VERSION].reset(new HttpDownload(VERSION_URL,
		[this] { postMessageFW(WM_SPEAKER, HTTP_COMPLETED, (LPARAM)CONN_VERSION); }, false));

	WinUtil::init(m_hWnd);

	trayMessage = RegisterWindowMessage(_T("TaskbarCreated"));

	if(WinUtil::getOsMajor() >= 6) {
		// 1 == MSGFLT_ADD
		_d_ChangeWindowMessageFilter(trayMessage, 1);
		_d_ChangeWindowMessageFilter(WMU_WHERE_ARE_YOU, 1);

		if(WinUtil::getOsMajor() > 6 || WinUtil::getOsMinor() >= 1) {
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
	images.Create(16, 16, ILC_COLOR32 | ILC_MASK,  0, 30);
	loadCmdBarImageList(images);
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
	m_CmdBar.m_arrCommand.Add(IDC_OPEN_MY_LIST);
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
	m_CmdBar.m_arrCommand.Add(IDC_OPEN_SYSTEMLOG);
	m_CmdBar.m_arrCommand.Add(IDC_AUTOSEARCH);
	m_CmdBar.m_arrCommand.Add(IDC_HASH_PROGRESS);
	m_CmdBar.m_arrCommand.Add(ID_APP_ABOUT);
	m_CmdBar.m_arrCommand.Add(ID_WIZARD);

	// use Vista-styled menus on Vista/Win7
	if(WinUtil::getOsMajor() >= 6)
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
	AddSimpleReBarBand(hWndTBStatusBar, NULL, FALSE, 205, TRUE);

	CreateSimpleStatusBar();
	
	RECT toolRect = {0};
	::GetWindowRect(hWndToolBar, &toolRect);

	ctrlStatus.Attach(m_hWndStatusBar);
	ctrlStatus.SetSimple(FALSE);
	int w[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	ctrlStatus.SetParts(11, w);
	statusSizes[0] = WinUtil::getTextWidth(TSTRING(AWAY), ctrlStatus.m_hWnd); // for "AWAY" segment

	CToolInfo ti(TTF_SUBCLASS, ctrlStatus.m_hWnd);

	ctrlLastLines.Create(ctrlStatus.m_hWnd, rcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON, WS_EX_TOPMOST);
	ctrlLastLines.SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	ctrlLastLines.AddTool(&ti);
	ctrlLastLines.SetDelayTime(TTDT_AUTOPOP, 15000);

	CreateMDIClient();
	m_CmdBar.SetMDIClient(m_hWndMDIClient);
	WinUtil::mdiClient = m_hWndMDIClient;

	ctrlTab.Create(m_hWnd, rcDefault);
	WinUtil::tabCtrl = &ctrlTab;
	tabsontop = BOOLSETTING(TABS_ON_TOP);

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

	trayMenu.CreatePopupMenu();
	trayMenu.AppendMenu(MF_STRING, IDC_TRAY_SHOW, CTSTRING(MENU_SHOW));
	trayMenu.AppendMenu(MF_STRING, IDC_OPEN_DOWNLOADS, CTSTRING(MENU_OPEN_DOWNLOADS_DIR));
	trayMenu.AppendMenu(MF_SEPARATOR);
	trayMenu.AppendMenu(MF_STRING, IDC_REFRESH_FILE_LIST, CTSTRING(MENU_REFRESH_FILE_LIST));
	trayMenu.AppendMenu(MF_SEPARATOR);
	trayMenu.AppendMenu(MF_STRING, ID_APP_ABOUT, CTSTRING(MENU_ABOUT));
	trayMenu.AppendMenu(MF_STRING, ID_APP_EXIT, CTSTRING(MENU_EXIT));

	if(BOOLSETTING(OPEN_PUBLIC)) PostMessage(WM_COMMAND, ID_FILE_CONNECT);
	if(BOOLSETTING(OPEN_FAVORITE_HUBS)) PostMessage(WM_COMMAND, IDC_FAVORITES);
	if(BOOLSETTING(OPEN_FAVORITE_USERS)) PostMessage(WM_COMMAND, IDC_FAVUSERS);
	if(BOOLSETTING(OPEN_QUEUE)) PostMessage(WM_COMMAND, IDC_QUEUE);
	if(BOOLSETTING(OPEN_FINISHED_DOWNLOADS)) PostMessage(WM_COMMAND, IDC_FINISHED);
	if(BOOLSETTING(OPEN_WAITING_USERS)) PostMessage(WM_COMMAND, IDC_UPLOAD_QUEUE);
	if(BOOLSETTING(OPEN_FINISHED_UPLOADS)) PostMessage(WM_COMMAND, IDC_FINISHED_UL);
	if(BOOLSETTING(OPEN_SEARCH_SPY)) PostMessage(WM_COMMAND, IDC_SEARCH_SPY);
	if(BOOLSETTING(OPEN_NETWORK_STATISTICS)) PostMessage(WM_COMMAND, IDC_NET_STATS);
	if(BOOLSETTING(OPEN_NOTEPAD)) PostMessage(WM_COMMAND, IDC_NOTEPAD);

	if(!BOOLSETTING(SHOW_STATUSBAR)) PostMessage(WM_COMMAND, ID_VIEW_STATUS_BAR);
	if(!BOOLSETTING(SHOW_TOOLBAR)) PostMessage(WM_COMMAND, ID_VIEW_TOOLBAR);
	if(!BOOLSETTING(SHOW_TRANSFERVIEW))	PostMessage(WM_COMMAND, ID_VIEW_TRANSFER_VIEW);

	if(!BOOLSETTING(SHOW_WINAMP_CONTROL)) PostMessage(WM_COMMAND, ID_TOGGLE_TOOLBAR);
	if(!BOOLSETTING(SHOW_TBSTATUS)) PostMessage(WM_COMMAND, ID_TOGGLE_TBSTATUS);
	if(BOOLSETTING(LOCK_TB)) PostMessage(WM_COMMAND, ID_LOCK_TB);
	if(BOOLSETTING(OPEN_SYSTEM_LOG)) PostMessage(WM_COMMAND, IDC_SYSTEM_LOG);

	if(!WinUtil::isShift())
		PostMessage(WM_SPEAKER, AUTO_CONNECT);

	PostMessage(WM_SPEAKER, PARSE_COMMAND_LINE);

	try {
		File::ensureDirectory(SETTING(LOG_DIRECTORY));
	} catch (const FileException) {	}

	try {
		ConnectivityManager::getInstance()->setup(true);
	} catch (const Exception& e) {
		showPortsError(e.getError());
	}

	auto prevGeo = BOOLSETTING(GET_USER_COUNTRY);
	auto prevGeoFormat = SETTING(COUNTRY_FORMAT);

	bool rebuildGeo = prevGeo && SETTING(COUNTRY_FORMAT) != prevGeoFormat;
	//if(BOOLSETTING(GET_USER_COUNTRY) != prevGeo) {
		if(BOOLSETTING(GET_USER_COUNTRY)) {
			GeoManager::getInstance()->init();
			checkGeoUpdate();
		} else {
			GeoManager::getInstance()->close();
			rebuildGeo = false;
		}
	//}
	if(rebuildGeo) {
		GeoManager::getInstance()->rebuild();
	}
	
	// Different app icons for different instances (APEX)
	HICON trayIcon = NULL;
	if(Util::fileExists(Text::fromT(WinUtil::getIconPath(_T("AirDCPlusPlus.ico")).c_str()))) {
		//HICON appIcon = (HICON)::LoadImage(NULL, WinUtil::getIconPath(_T("AirDCPlusPlus.ico")).c_str(), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR | LR_LOADFROMFILE);
		trayIcon = (HICON)::LoadImage(NULL, WinUtil::getIconPath(_T("AirDCPlusPlus.ico")).c_str(), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_LOADFROMFILE);
		
		//if(WinUtil::getOsMajor() > 6 || WinUtil::getOsMinor() >= 1) {
		WinUtil::SetIcon(m_hWnd, _T("AirDCPlusPlus.ico"), true);
		//}

		//if(WinUtil::getOsMajor() < 6 || (WinUtil::getOsMajor() == 6 && WinUtil::getOsMinor() < 1))
			//DestroyIcon((HICON)SetClassLongPtr(m_hWnd, GCLP_HICON, (LONG_PTR)appIcon));

		DestroyIcon((HICON)SetClassLongPtr(m_hWnd, GCLP_HICONSM, (LONG_PTR)trayIcon));
		
	}

	normalicon.hIcon = (!trayIcon) ? (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR) : trayIcon;
	pmicon.hIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAY_PM), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	hubicon.hIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAY_HUB), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

	uploadIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_UPLOAD), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	downloadIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DOWNLOAD), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);


	//updateTray( BOOLSETTING( MINIMIZE_TRAY ) );
	updateTray(true);

	Util::setAway(BOOLSETTING(AWAY));

	ctrlToolbar.CheckButton(IDC_AWAY,BOOLSETTING(AWAY));
	ctrlToolbar.CheckButton(IDC_DISABLE_SOUNDS, BOOLSETTING(SOUNDS_DISABLED));

	if(SETTING(NICK).empty()) {
		PostMessage(WM_COMMAND, ID_FILE_SETTINGS);
	}
	
	ctrlStatus.SetIcon(5, downloadIcon);
	ctrlStatus.SetIcon(6, uploadIcon);

	//background image
	if(!SETTING(BACKGROUND_IMAGE).empty()) {
		m_PictureWindow.SubclassWindow(m_hWndMDIClient);
		m_PictureWindow.m_nMessageHandler = CPictureWindow::BackGroundPaint;
		currentPic = SETTING(BACKGROUND_IMAGE);
		m_PictureWindow.Load(Text::toT(currentPic).c_str());
	}
	if(BOOLSETTING(TESTWRITE)) {
		TestWrite(true, true, true);
	}
	// We want to pass this one on to the splitter...hope it get's there...
	bHandled = FALSE;
	return 0;
}
void MainFrame::loadCmdBarImageList(CImageList& images){
	images.AddIcon(WinUtil::createIcon(IDI_PUBLICHUBS));
	images.AddIcon(WinUtil::createIcon(IDI_RECONNECT));
	images.AddIcon(WinUtil::createIcon(IDI_FOLLOW));
	images.AddIcon(WinUtil::createIcon(IDI_RECENTS));
	images.AddIcon(WinUtil::createIcon(IDI_FAVORITEHUBS));
	images.AddIcon(WinUtil::createIcon(IDI_FAVORITE_USERS));
	images.AddIcon(WinUtil::createIcon(IDI_QUEUE));
	images.AddIcon(WinUtil::createIcon(IDI_FINISHED_DL));
	images.AddIcon(WinUtil::createIcon(IDI_UPLOAD_QUEUE));
	images.AddIcon(WinUtil::createIcon(IDI_FINISHED_UL));
	images.AddIcon(WinUtil::createIcon(IDI_SEARCH));
	images.AddIcon(WinUtil::createIcon(IDI_ADLSEARCH));
	images.AddIcon(WinUtil::createIcon(IDI_SEARCHSPY));
	images.AddIcon(WinUtil::createIcon(IDI_OPEN_LIST));
	images.AddIcon(WinUtil::createIcon(IDI_OWNLIST)); 
	images.AddIcon(WinUtil::createIcon(IDI_MATCHLIST)); 
	images.AddIcon(WinUtil::createIcon(IDI_REFRESH));
	images.AddIcon(WinUtil::createIcon(IDI_SCAN));
	images.AddIcon(WinUtil::createIcon(IDI_OPEN_DOWNLOADS));
	images.AddIcon(WinUtil::createIcon(IDI_QCONNECT));
	images.AddIcon(WinUtil::createIcon(IDI_SETTINGS));
	images.AddIcon(WinUtil::createIcon(IDI_GET_TTH));
	images.AddIcon(WinUtil::createIcon(IDR_UPDATE));
	images.AddIcon(WinUtil::createIcon(IDI_SHUTDOWN));
	images.AddIcon(WinUtil::createIcon(IDI_NOTEPAD));
	images.AddIcon(WinUtil::createIcon(IDI_NETSTATS));
	images.AddIcon(WinUtil::createIcon(IDI_CDM));
	images.AddIcon(WinUtil::createIcon(IDI_LOGS));
	images.AddIcon(WinUtil::createIcon(IDI_AUTOSEARCH));
	images.AddIcon(WinUtil::createIcon(IDI_INDEXING));
	images.AddIcon(WinUtil::createIcon(IDR_MAINFRAME));
	images.AddIcon(WinUtil::createIcon(IDI_WIZARD));
}

HWND MainFrame::createTBStatusBar() {
	
	TBStatusCtrl.Create(m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT, 0, ATL_IDW_TOOLBAR);

	TBBUTTON tb[1];
	memzero(&tb, sizeof(tb));
	tb[0].iBitmap = 205;
	tb[0].fsStyle = TBSTYLE_SEP;

	TBStatusCtrl.SetButtonStructSize();
	TBStatusCtrl.AddButtons(1, tb);
	TBStatusCtrl.AutoSize();

	CRect rect;
	TBStatusCtrl.GetItemRect(0, &rect);
	rect.bottom += 100;
	rect.left += 2;

	progress.Create(TBStatusCtrl.m_hWnd, rect , NULL, WS_BORDER | PBS_SMOOTH  | WS_CHILD | WS_VISIBLE);
	progress.SetRange(0, 10000);

	startBytes, startFiles = 0;
	refreshing = false;
	//updateTBStatus();

	return TBStatusCtrl.m_hWnd;
}


void MainFrame::showPortsError(const string& port) {
	MessageBox(Text::toT(str(boost::format(STRING(PORT_BYSY)) % port)).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONEXCLAMATION);
	//MessageBox(CTSTRING_F(PORT_BYSY, port), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONEXCLAMATION);
}

HWND MainFrame::createWinampToolbar() {
	if(!tbarwinampcreated){
		ctrlSmallToolbar.Create(m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST, 0, ATL_IDW_TOOLBAR);
		ctrlSmallToolbar.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS);

		//we want to support old themes also so check if there is the old .bmp
		if(Util::fileExists(Text::fromT(WinUtil::getIconPath(_T("mediatoolbar.bmp"))))){
			winampImages.CreateFromImage(WinUtil::getIconPath(_T("mediatoolbar.bmp")).c_str(), 20, 20, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED | LR_LOADFROMFILE);
		} else /*if(Util::fileExists(Text::fromT(WinUtil::getIconPath(_T("MediaToolbar\\")))))*/{
			int size = SETTING(WTB_IMAGE_SIZE);
			winampImages.Create(size, size, ILC_COLOR32 | ILC_MASK,  0, 11);
			winampImages.AddIcon(WinUtil::createIcon(WinUtil::getIconPath(_T("MediaToolbar\\start.ico")), size));
			winampImages.AddIcon(WinUtil::createIcon(WinUtil::getIconPath(_T("MediaToolbar\\spam.ico")), size));
			winampImages.AddIcon(WinUtil::createIcon(WinUtil::getIconPath(_T("MediaToolbar\\back.ico")), size));
			winampImages.AddIcon(WinUtil::createIcon(WinUtil::getIconPath(_T("MediaToolbar\\play.ico")), size));
			winampImages.AddIcon(WinUtil::createIcon(WinUtil::getIconPath(_T("MediaToolbar\\pause.ico")), size));
			winampImages.AddIcon(WinUtil::createIcon(WinUtil::getIconPath(_T("MediaToolbar\\next.ico")), size));
			winampImages.AddIcon(WinUtil::createIcon(WinUtil::getIconPath(_T("MediaToolbar\\stop.ico")), size));
			winampImages.AddIcon(WinUtil::createIcon(WinUtil::getIconPath(_T("MediaToolbar\\up.ico")), size));
			winampImages.AddIcon(WinUtil::createIcon(WinUtil::getIconPath(_T("MediaToolbar\\volume50.ico")), size));
			winampImages.AddIcon(WinUtil::createIcon(WinUtil::getIconPath(_T("MediaToolbar\\down.ico")), size));
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

		if(!(SETTING(TOOLBARIMAGE) == "")) { //we expect to have the toolbarimage set before setting the Hot image.
			ToolbarImages.CreateFromImage(Text::toT(SETTING(TOOLBARIMAGE)).c_str(), 0/*SETTING(TB_IMAGE_SIZE)*/, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED | LR_LOADFROMFILE);

			if(!(SETTING(TOOLBARHOTIMAGE) == "")) { 
				ToolbarImagesHot.CreateFromImage(Text::toT(SETTING(TOOLBARHOTIMAGE)).c_str(), 0/*SETTING(TB_IMAGE_SIZE_HOT)*/, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED | LR_LOADFROMFILE);
				ctrlToolbar.SetHotImageList(ToolbarImagesHot);
			}
		} else { //default ones are .ico
			int i = 0;
			int buttonsCount = sizeof(ToolbarButtons) / sizeof(ToolbarButtons[0]);
			ToolbarImages.Create(SETTING(TB_IMAGE_SIZE), SETTING(TB_IMAGE_SIZE), ILC_COLOR32 | ILC_MASK,  0, buttonsCount+1);
			while(i < buttonsCount){
				ToolbarImages.AddIcon(WinUtil::createIcon(ToolbarButtons[i].nIcon));
				i++;
			}
		}
		ctrlToolbar.SetImageList(ToolbarImages);
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



LRESULT MainFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
		
	if(wParam == DOWNLOAD_LISTING) {
		auto_ptr<DirectoryListInfo> i(reinterpret_cast<DirectoryListInfo*>(lParam));
		DirectoryListingFrame::openWindow(i->file, i->dir, i->user, i->speed);
	} else if(wParam == BROWSE_LISTING) {
		auto_ptr<DirectoryBrowseInfo> i(reinterpret_cast<DirectoryBrowseInfo*>(lParam));
		DirectoryListingFrame::openWindow(i->user, i->text, 0);
	} else if(wParam == VIEW_FILE_AND_DELETE) {
		auto_ptr<tstring> file(reinterpret_cast<tstring*>(lParam));
		TextFrame::openWindow(*file, TextFrame::NORMAL);
		File::deleteFile(Text::fromT(*file));
	} else if(wParam == STATS) {
		auto_ptr<TStringList> pstr(reinterpret_cast<TStringList*>(lParam));
		const TStringList& str = *pstr;
		if(ctrlStatus.IsWindow()) {
			bool u = false;
			ctrlStatus.SetText(1, str[0].c_str());
			for(int i = 1; i < 9; i++) {
				int w = WinUtil::getTextWidth(str[i], ctrlStatus.m_hWnd);
				//make room for the icons
				if(i == 4 || i == 5)
					w = w+17;

				if(statusSizes[i] < w) {
					statusSizes[i] = w;
					u = true;
				}
				ctrlStatus.SetText(i+1, str[i].c_str());
			}

			if(u)
				UpdateLayout(TRUE);

			if (bShutdown) {
				uint64_t iSec = GET_TICK() / 1000;
				if(!isShutdownStatus) {
					ctrlStatus.SetIcon(10, hShutdownIcon);
					isShutdownStatus = true;
				}
				if (DownloadManager::getInstance()->getDownloadCount() > 0) {
					iCurrentShutdownTime = iSec;
					ctrlStatus.SetText(10, _T(""));
				} else {
					int64_t timeLeft = SETTING(SHUTDOWN_TIMEOUT) - (iSec - iCurrentShutdownTime);
					ctrlStatus.SetText(10, (_T(" ") + Util::formatSeconds(timeLeft, timeLeft < 3600)).c_str(), SBT_POPOUT);
					if (iCurrentShutdownTime + SETTING(SHUTDOWN_TIMEOUT) <= iSec) {
						bool bDidShutDown = false;
						bDidShutDown = WinUtil::shutDown(SETTING(SHUTDOWN_ACTION));
						if (bDidShutDown) {
							// Should we go faster here and force termination?
							// We "could" do a manual shutdown of this app...
						} else {
							LogManager::getInstance()->message(STRING(FAILED_TO_SHUTDOWN), LogManager::LOG_ERROR);
							ctrlStatus.SetText(10, _T(""));
						}
						// We better not try again. It WON'T work...
						bShutdown = false;
					}
				}
			} else {
				if(isShutdownStatus) {
					ctrlStatus.SetIcon(10, NULL);
					isShutdownStatus = false;
				}
				ctrlStatus.SetText(10, _T(""));
			}
		}
	} else if(wParam == AUTO_CONNECT) {
		autoConnect(FavoriteManager::getInstance()->getFavoriteHubs());
	} else if(wParam == PARSE_COMMAND_LINE) {
		parseCommandLine(GetCommandLine());
	} else if(wParam == STATUS_MESSAGE) {
		//tstring* msg = (tstring*)lParam;
		auto_ptr<pair<time_t, tstring> > msg((pair<time_t, tstring>*)lParam);
		if(ctrlStatus.IsWindow()) {
			//tstring line = Text::toT("[" + Util::getShortTimeString() + "] ") + *msg;
			tstring line = Text::toT("[" + Util::getTimeStamp(msg->first) + "] ") + msg->second;

			ctrlStatus.SetText(0, line.c_str());
			while(lastLinesList.size() + 1 > MAX_CLIENT_LINES)
				lastLinesList.erase(lastLinesList.begin());
			if (_tcschr(line.c_str(), _T('\r')) == NULL) {
				lastLinesList.push_back(line);
			} else {
				lastLinesList.push_back(line.substr(0, line.find(_T('\r'))));
			}
		}
		//delete msg;
	} else if(wParam == SHOW_POPUP) {
		Popup* msg = (Popup*)lParam;
		PopupManager::getInstance()->Show(msg->Message, msg->Title, msg->Icon);
		delete msg;
	} else if(wParam == WM_CLOSE) {
		PopupManager::getInstance()->Remove((int)lParam);
	} else if(wParam == REMOVE_POPUP){
		PopupManager::getInstance()->AutoRemove();
	} else if(wParam == SET_PM_TRAY_ICON) {
		if(bIsPM == false && (!WinUtil::isAppActive || bAppMinimized)) {
			bIsPM = true;
			if(taskbarList) {
				taskbarList->SetOverlayIcon(m_hWnd, pmicon.hIcon, NULL);
			}
			if(bTrayIcon == true) {
				NOTIFYICONDATA nid;
				nid.cbSize = sizeof(NOTIFYICONDATA);
				nid.hWnd = m_hWnd;
				nid.uID = 0;
				nid.uFlags = NIF_ICON;
				nid.hIcon = pmicon.hIcon;
				::Shell_NotifyIcon(NIM_MODIFY, &nid);
			}
		}
    } else if(wParam == SET_HUB_TRAY_ICON) {
		if(bIsPM == false && (!WinUtil::isAppActive || bAppMinimized)) { //using IsPM to avoid the 2 icons getting mixed up
			bIsPM = true;
			if(taskbarList) {
				taskbarList->SetOverlayIcon(m_hWnd, hubicon.hIcon, NULL);
			}
			if(bTrayIcon == true) {
				NOTIFYICONDATA nid;
				nid.cbSize = sizeof(NOTIFYICONDATA);
				nid.hWnd = m_hWnd;
				nid.uID = 0;
				nid.uFlags = NIF_ICON;
				nid.hIcon = hubicon.hIcon;
				::Shell_NotifyIcon(NIM_MODIFY, &nid);
			}
		}
	} else if(wParam == UPDATE_TBSTATUS_HASHING) {
		HashInfo *tmp = (HashInfo*)lParam;
		updateTBStatusHashing(*tmp);
		delete tmp;
	} else if(wParam == UPDATE_TBSTATUS_REFRESHING) {
		updateTBStatusRefreshing();
    } else if(wParam == HTTP_COMPLETED) {
		if (lParam == CONN_VERSION) {
			completeVersionUpdate();
		} else if (lParam == CONN_GEO_V6) {
			completeGeoUpdate(true);
		} else if (lParam == CONN_GEO_V4) {
			completeGeoUpdate(false);
		};
	}

	return 0;
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
	parseCommandLine(Text::toT(WinUtil::getAppName() + " ") + cmdLine);
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
	WizardDlg dlg;
	dlg.DoModal(m_hWnd);
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
	openSettings();
	return 0;
}

void MainFrame::openSettings(uint16_t initialPage /*0*/) {
	PropertiesDlg dlg(m_hWnd, SettingsManager::getInstance(), initialPage);

	auto prevTCP = SETTING(TCP_PORT);
	auto prevUDP = SETTING(UDP_PORT);
	auto prevTLS = SETTING(TLS_PORT);

	auto prevConn = SETTING(INCOMING_CONNECTIONS);
	auto prevMapper = SETTING(MAPPER);
	auto prevBind = SETTING(BIND_ADDRESS);
	auto prevBind6 = SETTING(BIND_ADDRESS6);
	auto prevProxy = CONNSETTING(OUTGOING_CONNECTIONS);


	auto prevGeo = BOOLSETTING(GET_USER_COUNTRY);
	auto prevGeoFormat = SETTING(COUNTRY_FORMAT);

	bool lastSortFavUsersFirst = BOOLSETTING(SORT_FAVUSERS_FIRST);

	auto prevDownloadOrder = SETTING(DOWNLOAD_ORDER);

	/*auto prevHighPrio = SETTING(HIGH_PRIO_FILES);
	auto prevHighPrioRegex = SETTING(HIGHEST_PRIORITY_USE_REGEXP);

	auto prevShareSkiplist = SETTING(SKIPLIST_SHARE);
	auto prevShareSkiplistRegex = SETTING(SHARE_SKIPLIST_USE_REGEXP);

	auto prevDownloadSkiplist = SETTING(SKIPLIST_DOWNLOAD);
	auto prevDownloadSkiplistRegex = SETTING(DOWNLOAD_SKIPLIST_USE_REGEXP);*/

	if(dlg.DoModal(m_hWnd) == IDOK) 
	{
		SettingsManager::getInstance()->save();
		if(missedAutoConnect && !SETTING(NICK).empty()) {
			PostMessage(WM_SPEAKER, AUTO_CONNECT);
		}

		try {
			ConnectivityManager::getInstance()->setup(SETTING(INCOMING_CONNECTIONS) != prevConn ||
				SETTING(TCP_PORT) != prevTCP || SETTING(UDP_PORT) != prevUDP || SETTING(TLS_PORT) != prevTLS ||
				SETTING(MAPPER) != prevMapper || SETTING(BIND_ADDRESS) != prevBind || SETTING(BIND_ADDRESS6) != prevBind6);
		} catch (const Exception& e) {
			showPortsError(e.getError());
		}

		auto outConns = CONNSETTING(OUTGOING_CONNECTIONS);
		if(outConns != prevProxy || outConns == SettingsManager::OUTGOING_SOCKS5) {
			Socket::socksUpdated();
		}


		ClientManager::getInstance()->infoUpdated();

		if (prevDownloadOrder != SETTING(DOWNLOAD_ORDER)) {
			QueueManager::getInstance()->onChangeDownloadOrder();
		}

		bool rebuildGeo = prevGeo && SETTING(COUNTRY_FORMAT) != prevGeoFormat;
		if(BOOLSETTING(GET_USER_COUNTRY) != prevGeo) {
			if(BOOLSETTING(GET_USER_COUNTRY)) {
				GeoManager::getInstance()->init();
				checkGeoUpdate();
			} else {
				GeoManager::getInstance()->close();
				rebuildGeo = false;
			}
		}
		if(rebuildGeo) {
			GeoManager::getInstance()->rebuild();
		}
 
		if(BOOLSETTING(SORT_FAVUSERS_FIRST) != lastSortFavUsersFirst)
			HubFrame::resortUsers();

		if(BOOLSETTING(URL_HANDLER)) {
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
		if(BOOLSETTING(MAGNET_REGISTER)) {
			WinUtil::registerMagnetHandler();
			WinUtil::urlMagnetRegistered = true; 
		} else if(WinUtil::urlMagnetRegistered) {
			WinUtil::unRegisterMagnetHandler();
			WinUtil::urlMagnetRegistered = false;
		}



		if(Util::getAway()) ctrlToolbar.CheckButton(IDC_AWAY, true);
		else ctrlToolbar.CheckButton(IDC_AWAY, false);

		if(getShutDown()) ctrlToolbar.CheckButton(IDC_SHUTDOWN, true);
		else ctrlToolbar.CheckButton(IDC_SHUTDOWN, false);
	
		if(tabsontop != BOOLSETTING(TABS_ON_TOP)) {
			tabsontop = BOOLSETTING(TABS_ON_TOP);
			UpdateLayout();
		}
	}
}

LRESULT MainFrame::onGetToolTip(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMTTDISPINFO pDispInfo = (LPNMTTDISPINFO)pnmh;
	pDispInfo->szText[0] = 0;

	if(!((idCtrl != 0) && !(pDispInfo->uFlags & TTF_IDISHWND))) {
		// if we're really in the status bar, this should be detected intelligently
		lastLines.clear();
		for(TStringIter i = lastLinesList.begin(); i != lastLinesList.end(); ++i) {
			lastLines += *i;
			lastLines += _T("\r\n");
		}
		if(lastLines.size() > 2) {
			lastLines.erase(lastLines.size() - 2);
		}
		pDispInfo->lpszText = const_cast<TCHAR*>(lastLines.c_str());
	}
	return 0;
}

void MainFrame::autoConnect(const FavoriteHubEntry::List& fl) {
		
	int left = SETTING(OPEN_FIRST_X_HUBS);
	auto& fh = FavoriteManager::getInstance()->getFavoriteHubs();
	if(left > static_cast<int>(fh.size())) {
		left = fh.size();
	}
	missedAutoConnect = false;
	for(auto i = fl.begin(); i != fl.end(); ++i) {
		FavoriteHubEntry* entry = *i;
		if(entry->getConnect()) {
 			if(!entry->getNick().empty() || !SETTING(NICK).empty()) {
				RecentHubEntry r;
				r.setName(entry->getName());
				r.setDescription(entry->getDescription());
				r.setUsers("*");
				r.setShared("*");
				r.setServer(entry->getServer());
				FavoriteManager::getInstance()->addRecent(r);
				HubFrame::openWindow(Text::toT(entry->getServer()), entry->getChatUserSplit(), entry->getUserListState());
 			} else
 				missedAutoConnect = true;
 		}				
	}
}

void MainFrame::updateTray(bool add /* = true */) {
	if(add) {
		if(bTrayIcon == false) {
			NOTIFYICONDATA nid;
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = m_hWnd;
			nid.uID = 0;
			nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
			nid.uCallbackMessage = WM_APP + 242;
			nid.hIcon = normalicon.hIcon;
			_tcsncpy(nid.szTip, _T(APPNAME), 64);
			nid.szTip[63] = '\0';
			lastMove = GET_TICK() - 1000;
			//::Shell_NotifyIcon(NIM_ADD, &nid);
			bTrayIcon = (::Shell_NotifyIcon(NIM_ADD, &nid) != FALSE);
		}
	} else {
		if(bTrayIcon) {
			NOTIFYICONDATA nid;
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = m_hWnd;
			nid.uID = 0;
			nid.uFlags = 0;
			::Shell_NotifyIcon(NIM_DELETE, &nid);
			//ShowWindow(SW_SHOW);
			bTrayIcon = false;
		}
	}
}

LRESULT MainFrame::onSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	if(wParam == SIZE_MINIMIZED) {
		/*maybe make this an option? with large amount of ram this is kinda obsolete,
		will look good in taskmanager ram usage tho :) */
		if(BOOLSETTING(DECREASE_RAM)) {
			if(!SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1))
				LogManager::getInstance()->message("Minimize Process WorkingSet Failed: "+ Util::translateError(GetLastError()), LogManager::LOG_WARNING);
		}

		if(BOOLSETTING(AUTO_AWAY) && (bAppMinimized == false) ) {
			
			if(Util::getAway()) {
				awaybyminimize = false;
			} else {
				awaybyminimize = true;
				Util::setAway(true, true);
				setAwayButton(true);
				ClientManager::getInstance()->infoUpdated();
			}
		}
		bAppMinimized = true; //set this here, on size is called twice if minimize to tray.

		if(BOOLSETTING(MINIMIZE_TRAY) != WinUtil::isShift()) {
			ShowWindow(SW_HIDE);
			SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
		}
		
		maximized = IsZoomed() > 0;
	} else if( (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED) ) {
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
		if(BOOLSETTING(AUTO_AWAY)) {
			if(awaybyminimize == true) {
				awaybyminimize = false;
				Util::setAway(false, true);
				setAwayButton(false);
				ClientManager::getInstance()->infoUpdated();
			}
		}
		if(bIsPM) {
			bIsPM = false;
			if(taskbarList) {
				taskbarList->SetOverlayIcon(m_hWnd, NULL , NULL);
			}

			if (bTrayIcon == true) {
				NOTIFYICONDATA nid;
				nid.cbSize = sizeof(NOTIFYICONDATA);
				nid.hWnd = m_hWnd;
				nid.uID = 0;
				nid.uFlags = NIF_ICON;
				nid.hIcon = normalicon.hIcon;
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
	
	QueueManager::getInstance()->saveQueue(true);
	SettingsManager::getInstance()->save();
	
	return 0;
}

LRESULT MainFrame::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(hasUpdate) {
		if (Util::fileExists(Util::getPath(Util::PATH_RESOURCES) + INSTALLER)) {
			//string file = (Util::getPath(Util::PATH_RESOURCES) + "AirDC_Installer.exe");
			if(MessageBox(CTSTRING(UPDATE_FILE_DETECTED), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
				::ShellExecute(NULL, NULL, Text::toT(Util::getPath(Util::PATH_RESOURCES) + INSTALLER).c_str(), NULL, NULL, SW_SHOWNORMAL);
				terminate();
			}
		}
		hasUpdate = false;
	}
	if(!closing) {
		if( oldshutdown ||(!BOOLSETTING(CONFIRM_EXIT)) || (MessageBox(CTSTRING(REALLY_EXIT), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) ) {
			updateTray(false);
			string tmp1;
			string tmp2;

			WinUtil::saveReBarSettings(m_hWndToolBar);

			if( hashProgress.IsWindow() )
				hashProgress.DestroyWindow();

			WINDOWPLACEMENT wp;
			wp.length = sizeof(wp);
			GetWindowPlacement(&wp);

			CRect rc;
			GetWindowRect(rc);
			if(BOOLSETTING(SHOW_TRANSFERVIEW)) {
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
			listQueue.shutdown();

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
		DestroyIcon(normalicon.hIcon);
		DestroyIcon(hShutdownIcon); 	
		DestroyIcon(pmicon.hIcon);
		DestroyIcon(hubicon.hIcon);
		DestroyIcon(uploadIcon);
		DestroyIcon(downloadIcon);
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
		case IDC_HELP_HOMEPAGE: site = links.homepage; break;
		case IDC_HELP_GUIDES: site = links.guides; break;
		case IDC_HELP_DISCUSS: site = links.discuss; break;
		case IDC_HELP_CUSTOMIZE: site = links.customize; break;
		default: dcassert(0);
	}

	if(isFile)
		WinUtil::openFile(site);
	else
		WinUtil::openLink(site);

	return 0;
}

int MainFrame::run() {
	tstring file;
	if(WinUtil::browseFile(file, m_hWnd, false, lastTTHdir) == IDOK) {
		WinUtil::mainMenu.EnableMenuItem(ID_GET_TTH, MF_GRAYED);
		Thread::setThreadPriority(Thread::LOW);
		lastTTHdir = Util::getFilePath(file);

		string TTH;
		TTH.resize(192*8/(5*8)+1);

		boost::scoped_array<char> buf(new char[512 * 1024]);

		try {
			File f(Text::fromT(file), File::READ, File::OPEN);
			TigerTree tth(TigerTree::calcBlockSize(f.getSize(), 1));

			if(f.getSize() > 0) {
				size_t n = 512*1024;
				while( (n = f.read(&buf[0], n)) > 0) {
					tth.update(&buf[0], n);
					n = 512*1024;
				}
			} else {
				tth.update("", 0);
			}
			tth.finalize();

			strcpy(&TTH[0], tth.getRoot().toBase32().c_str());

			CInputBox ibox(m_hWnd);

			string magnetlink = "magnet:?xt=urn:tree:tiger:"+ TTH +"&xl="+Util::toString(f.getSize())+"&dn="+Util::encodeURI(Text::fromT(Util::getFileName(file)));
			f.close();
			
			ibox.DoModal(_T("Tiger Tree Hash"), file.c_str(), Text::toT(TTH).c_str(), Text::toT(magnetlink).c_str());
		} catch(...) { }
		Thread::setThreadPriority(Thread::NORMAL);
		WinUtil::mainMenu.EnableMenuItem(ID_GET_TTH, MF_ENABLED);
	}
	return 0;
}

LRESULT MainFrame::onGetTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Thread::start();
	return 0;
}

void MainFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */)
{
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	
	if(ctrlStatus.IsWindow() && ctrlLastLines.IsWindow()) {
		CRect sr;
		int w[11];
		ctrlStatus.GetClientRect(sr);
		w[10] = sr.right - 20;
		w[9] = w[10] - 20;
		w[8] = w[9] - 120;
#define setw(x) w[x] = max(w[x+1] - statusSizes[x], 0)
		setw(7); setw(6); setw(5); setw(4); setw(3); setw(2); setw(1); setw(0);

		ctrlStatus.SetParts(11, w);
		ctrlLastLines.SetMaxTipWidth(w[0]);
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

static const TCHAR types[] = _T("File Lists\0*.DcLst;*.xml.bz2\0All Files\0*.*\0");

LRESULT MainFrame::onOpenFileList(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	tstring file = Util::emptyStringT;
	
	if(wID == IDC_OPEN_MY_LIST){
		string flname;
		auto profiles = ShareManager::getInstance()->getProfiles();
		if (profiles.size() > 2) {
			StringList tmpList;
			for(auto j = profiles.begin(); j != profiles.end(); j++) {
				if ((*j)->getToken() != SP_HIDDEN)
					tmpList.push_back((*j)->getDisplayName());
			}

			ComboDlg dlg;
			dlg.setList(tmpList);
			dlg.description = CTSTRING(SHARE_PROFILE);
			dlg.title = CTSTRING(MENU_OPEN_OWN_LIST);
			if(dlg.DoModal() == IDOK) {
				flname = profiles[dlg.curSel]->getProfileList()->getProfile();
			} else {
				return 0;
			}
		} else {
			flname = SP_DEFAULT;
		}
		DirectoryListingFrame::openWindow(Text::toT(flname), Text::toT(Util::emptyString), HintedUser(ClientManager::getInstance()->getMe(), Util::emptyString), 0, true);
		return 0;
	}

	if(WinUtil::browseFile(file, m_hWnd, false, Text::toT(Util::getListPath()), types)) {
		UserPtr u = DirectoryListing::getUserFromFilename(Text::fromT(file));
		if(u) {
			DirectoryListingFrame::openWindow(file, Text::toT(Util::emptyString), HintedUser(u, Util::emptyString), 0);
		} else {
			MessageBox(CTSTRING(INVALID_LISTNAME), _T(APPNAME) _T(" ") _T(VERSIONSTRING));
		}
	}
	return 0;
}

LRESULT MainFrame::onRefreshFileList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ShareManager::getInstance()->refresh();
	return 0;
}

LRESULT MainFrame::onScanMissing(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

		ShareScannerManager::getInstance()->scan();
		return 0;
}

LRESULT MainFrame::onTrayIcon(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	if (lParam == WM_LBUTTONUP) {
		if(bAppMinimized) {
	
	if(BOOLSETTING(PASSWD_PROTECT_TRAY)) {
		if(hasPassdlg) //prevent dialog from showing twice, findwindow doesnt seem to work with this??
			return 0;

		hasPassdlg = true;
		PassDlg passdlg;
		passdlg.description = TSTRING(PASSWORD_DESC);
		passdlg.title = TSTRING(PASSWORD_TITLE);
		passdlg.ok = TSTRING(UNLOCK);
		if(passdlg.DoModal(/*m_hWnd*/) == IDOK){
			tstring tmp = passdlg.line;
			if (tmp == Text::toT(Util::base64_decode(SETTING(PASSWORD)))) {
			ShowWindow(SW_SHOW);
			ShowWindow(maximized ? SW_MAXIMIZE : SW_RESTORE);
			}
			hasPassdlg = false;
		}
	} else {
			ShowWindow(SW_SHOW);
			ShowWindow(maximized ? SW_MAXIMIZE : SW_RESTORE);
	}
		} else {
			SetForegroundWindow(m_hWnd);
		}
	} else if(lParam == WM_MOUSEMOVE && ((lastMove + 1000) < GET_TICK()) ) {
		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = m_hWnd;
		nid.uID = 0;
		nid.uFlags = NIF_TIP;
		_tcsncpy(nid.szTip, (_T("D: ") + Util::formatBytesW(DownloadManager::getInstance()->getRunningAverage()) + _T("/s (") + 
			Util::toStringW(DownloadManager::getInstance()->getDownloadCount()) + _T(")\r\nU: ") +
			Util::formatBytesW(UploadManager::getInstance()->getRunningAverage()) + _T("/s (") + 
			Util::toStringW(UploadManager::getInstance()->getUploadCount()) + _T(")")
			+ _T("\r\nUptime: ") + Util::formatSeconds(Util::getUptime())
			).c_str(), 64);
		
		::Shell_NotifyIcon(NIM_MODIFY, &nid);
		lastMove = GET_TICK();
	} else if (lParam == WM_RBUTTONUP) {
		CPoint pt(GetMessagePos());		
		SetForegroundWindow(m_hWnd);
		trayMenu.TrackPopupMenu(TPM_RIGHTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);		
		PostMessage(WM_NULL, 0, 0);
	}
	return 0;
}

void MainFrame::ShowBalloonTip(tstring szMsg, tstring szTitle, DWORD dwInfoFlags) {
	Popup* p = new Popup;
	p->Title = szTitle;
	p->Message = szMsg;
	p->Icon = dwInfoFlags;

	PostMessage(WM_SPEAKER, SHOW_POPUP, (LPARAM)p);
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
	SettingsManager::getInstance()->set(SettingsManager::SHOW_TOOLBAR, bVisible);
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
	SettingsManager::getInstance()->set(SettingsManager::SHOW_WINAMP_CONTROL, bVisible);
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
	SettingsManager::getInstance()->set(SettingsManager::SHOW_TBSTATUS, bVisible);
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
	SettingsManager::getInstance()->set(SettingsManager::LOCK_TB, locked);
	return 0;
}

LRESULT MainFrame::onWinampStart(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	if(SETTING(WINAMP_PATH) != Util::emptyString) {
	::ShellExecute(NULL, NULL, Text::toT(SETTING(WINAMP_PATH)).c_str(), NULL , NULL, SW_SHOWNORMAL);
	}
	return 0;
}

void MainFrame::updateTBStatusHashing(HashInfo m_HashInfo) {

	refreshing = false;
	progress.SetMarquee(FALSE);
	progress.ModifyStyle(PBS_MARQUEE, NULL);
	progress.SetRedraw(TRUE);

	string file = m_HashInfo.file;
	int64_t bytes = m_HashInfo.size;
	size_t files = m_HashInfo.files;
	int64_t speed = m_HashInfo.speed;

	if(bytes > startBytes)
		startBytes = bytes;

	if(files > startFiles)
		startFiles = files;

	bool paused = m_HashInfo.paused;
	tstring tmp = _T("");
	if(files == 0 || bytes == 0 || paused) {
		if(paused) {
			tmp += 	_T(" ( ") + TSTRING(PAUSED) + _T(" )");
		} else {
			tmp = _T("");
		}
	} else {
		int64_t filestat = speed > 0 ? ((startBytes - (startBytes - bytes)) / speed) : 0;

		tmp = Util::formatBytesW((int64_t)speed) + _T("/s, ") + Util::formatBytesW(bytes) + _T(" ") + TSTRING(LEFT);
		
		if(filestat == 0 || speed == 0) {
			tmp += Text::toT(", -:--:-- " + STRING(LEFT));	
		} else {
			tmp += _T(", ") + Util::formatSeconds(filestat) + _T(" ") + TSTRING(LEFT);
		}
	}

	if(startFiles == 0 || startBytes == 0 || files == 0) {
		progress.SetPos(0);
		//setProgressText(_T("TEST text"));
		startBytes = 0;
		startFiles = 0;
	} else {
		progress.SetPos((int)(progress.GetRangeLimit(0) * ((0.5 * (startFiles - files)/startFiles) + 0.5 * (startBytes - bytes) / startBytes)));
		progress.RedrawWindow();
		setProgressText(tmp);
		progress.SetRedraw(FALSE);
	}
}
void MainFrame::setProgressText(const tstring& text){
	CPoint ptStart;
	CRect prc;
	progress.GetClientRect(&prc);

	HDC progressTextDC = progress.GetDC();

	::SetBkMode(progressTextDC, TRANSPARENT);
	::SetTextColor(progressTextDC, WinUtil::TBprogressTextColor );
	::SelectObject(progressTextDC, WinUtil::progressFont);
	
	//ptStart.x = prc.left + 2;
	//ptStart.y = prc.top + 5; 
	//::ExtTextOut(progressTextDC, ptStart.x, ptStart.y, ETO_CLIPPED, &prc, text.c_str(), _tcslen(text.c_str()), NULL);
	prc.top += 5;
	::DrawText(progressTextDC, text.c_str(), text.length(), prc, DT_CENTER | DT_VCENTER );
	progress.ReleaseDC(progressTextDC);
}

void MainFrame::updateTBStatusRefreshing() {
	if(refreshing)
		return;

	refreshing = true;
	progress.SetRedraw(TRUE);
	progress.ModifyStyle(NULL, PBS_MARQUEE);
	progress.SetMarquee(TRUE, 10);
}

LRESULT MainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_STATUSBAR, bVisible);
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
	SettingsManager::getInstance()->set(SettingsManager::SHOW_TRANSFERVIEW, bVisible);
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

		tstring tmp = dlg.address;
		// Strip out all the spaces
		string::size_type i;
		while((i = tmp.find(' ')) != string::npos)
			tmp.erase(i, 1);

		RecentHubEntry r;
		r.setName("*");
		r.setDescription("*");
		r.setUsers("*");
		r.setShared("*");
		r.setServer(Text::fromT(tmp));
		FavoriteManager::getInstance()->addRecent(r);
		HubFrame::openWindow(tmp, 0, true, dlg.curProfile);
	}
	return 0;
}

void MainFrame::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
	if(aTick == lastUpdate)	// FIXME: temp fix for new TimerManager
		return;

	Util::increaseUptime();
	int64_t diff = (int64_t)((lastUpdate == 0) ? aTick - 1000 : aTick - lastUpdate);
	int64_t updiff = Socket::getTotalUp() - lastUp;
	int64_t downdiff = Socket::getTotalDown() - lastDown;
	uint64_t queueSize = QueueManager::getInstance()->fileQueue.getTotalQueueSize();

	TStringList* str = new TStringList();
	str->push_back(Util::getAway() ? TSTRING(AWAY) : _T(""));
	str->push_back(TSTRING(SHARED) + _T(": ") + Util::formatBytesW(ShareManager::getInstance()->getSharedSize()));
	str->push_back(_T("H: ") + Text::toT(Client::getCounts()));
	str->push_back(TSTRING(SLOTS) + _T(": ") + Util::toStringW(UploadManager::getInstance()->getFreeSlots()) + _T('/') + Util::toStringW(UploadManager::getInstance()->getSlots()) + _T(" (") + Util::toStringW(UploadManager::getInstance()->getFreeExtraSlots()) + _T('/') + Util::toStringW(SETTING(EXTRA_SLOTS)) + _T(")"));
	str->push_back(_T("D: ") + Util::formatBytesW(Socket::getTotalDown()));
	str->push_back(_T("U: ") + Util::formatBytesW(Socket::getTotalUp()));
	str->push_back(_T("D: [") + Util::toStringW(DownloadManager::getInstance()->getDownloadCount()) + _T("]") + Util::formatBytesW(downdiff*1000I64/diff) + _T("/s"));
	str->push_back(_T("U: [") + Util::toStringW(UploadManager::getInstance()->getUploadCount()) + _T("]") + Util::formatBytesW(updiff*1000I64/diff) + _T("/s"));
	str->push_back(TSTRING(QUEUE_SIZE) + _T(": ")  + Util::formatBytesW(queueSize < 0 ? 0 : queueSize));

	PostMessage(WM_SPEAKER, STATS, (LPARAM)str);
	SettingsManager::getInstance()->set(SettingsManager::TOTAL_UPLOAD, SETTING(TOTAL_UPLOAD) + updiff);
	SettingsManager::getInstance()->set(SettingsManager::TOTAL_DOWNLOAD, SETTING(TOTAL_DOWNLOAD) + downdiff);
	lastUpdate = aTick;
	lastUp = Socket::getTotalUp();
	lastDown = Socket::getTotalDown();

	if(SETTING(DISCONNECT_SPEED) < 1) {
		SettingsManager::getInstance()->set(SettingsManager::DISCONNECT_SPEED, 1);
	}
	if(TBStatusCtrl.IsWindowVisible()){
		if(ShareManager::getInstance()->isRefreshing()){
			PostMessage(WM_SPEAKER, UPDATE_TBSTATUS_REFRESHING, 0);
		} else {
			string file = "";
			int64_t bytes = 0;
			size_t files = 0;
			int64_t speed = 0;
			bool paused = HashManager::getInstance()->isHashingPaused();
			HashManager::getInstance()->getStats(file, bytes, files, speed);
			HashInfo *hashinfo = new HashInfo(file, bytes, files, speed, paused);
			PostMessage(WM_SPEAKER, UPDATE_TBSTATUS_HASHING, LPARAM(hashinfo));
		}
	}

	if(currentPic != SETTING(BACKGROUND_IMAGE)) {
		currentPic = SETTING(BACKGROUND_IMAGE);
		m_PictureWindow.Load(Text::toT(currentPic).c_str());
	}

	time_t currentTime;
	time(&currentTime);
}

void MainFrame::on(PartialList, const HintedUser& aUser, const string& text) noexcept {
	PostMessage(WM_SPEAKER, BROWSE_LISTING, (LPARAM)new DirectoryBrowseInfo(aUser, text));
}

void MainFrame::on(QueueManagerListener::Finished, const QueueItemPtr qi, const string& dir, const HintedUser& aUser, int64_t aSpeed) noexcept {
	if(qi->isSet(QueueItem::FLAG_CLIENT_VIEW)) {
		if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
			// This is a file listing, show it...
			DirectoryListInfo* i = new DirectoryListInfo(aUser, Text::toT(qi->getListName()), Text::toT(dir), static_cast<int64_t>(aSpeed));

			PostMessage(WM_SPEAKER, DOWNLOAD_LISTING, (LPARAM)i);
		} else if(qi->isSet(QueueItem::FLAG_TEXT)) {
			PostMessage(WM_SPEAKER, VIEW_FILE_AND_DELETE, (LPARAM) new tstring(Text::toT(qi->getTarget())));
		}
	} else if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
		DirectoryListInfo* i = new DirectoryListInfo(aUser, Text::toT(qi->getListName()), Text::toT(dir), static_cast<int64_t>(aSpeed));
		
		if(listQueue.stop) {
			listQueue.stop = false;
			listQueue.start();
		}
		{
			Lock l(listQueue.cs);
			listQueue.fileLists.push_back(i);
		}
		listQueue.s.signal();
	}	
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
				nid.cbSize = sizeof(NOTIFYICONDATA);
				nid.hWnd = m_hWnd;
				nid.uID = 0;
				nid.uFlags = NIF_ICON;
				nid.hIcon = normalicon.hIcon;
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
	if(Util::getAway()) { 
		setAwayButton(false);
		Util::setAway(false);
	} else {
		setAwayButton(true);
		Util::setAway(true);
	}
	ClientManager::getInstance()->infoUpdated();
	return 0;
}

LRESULT MainFrame::onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UpdateDlg dlg;
	dlg.DoModal();
	return S_OK;
}

int MainFrame::FileListQueue::run() {
	setThreadPriority(Thread::LOW);

	while(true) {
		s.wait(15000);
		if(stop || fileLists.empty()) {
			break;
		}

		DirectoryListInfo* i;
		{
			Lock l(cs);
			i = fileLists.front();
			fileLists.pop_front();
		}
		if(Util::fileExists(Text::fromT(i->file))) {
			DirectoryListing* dl = new DirectoryListing(i->user, false);
			try {
				dl->loadFile(Text::fromT(i->file), false);
			} catch(...) {
			}
			delete dl;
		}
		delete i;
	}
	stop = true;
	return 0;
}

void MainFrame::TestWrite( bool downloads, bool incomplete, bool AppPath) {
	
	tstring error = Util::emptyStringT;
	string filename = (Util::getPath(Util::PATH_USER_CONFIG) + "testwrite.tmp");
	bool ready = false;
	File* f = NULL;
	//Create the test file first, we should allways be able to write Settings folder
	try {

	f = new File((filename), File::WRITE, File::OPEN | File::CREATE);

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
		MessageBox((error.c_str()), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONWARNING | MB_OK);
	} /*else if(ready) { //dont need this but leave it for now.
		LogManager::getInstance()->message("Test write to AirDC++ common folders succeeded.", LogManager::LOG_WARNING);
	}*/

}


LRESULT MainFrame::onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	LogManager::getInstance()->removeListener(this);
	QueueManager::getInstance()->removeListener(this);
	TimerManager::getInstance()->removeListener(this);

	//if(bTrayIcon) {
		updateTray(false);
	//}
	bHandled = FALSE;
	return 0;
}

LRESULT MainFrame::onDropDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMTOOLBAR tb = (LPNMTOOLBAR)pnmh;
	CMenu dropMenu;
	dropMenu.CreatePopupMenu();

	auto l = ShareManager::getInstance()->getGroupedDirectories();
	
	dropMenu.AppendMenu(MF_STRING, IDC_REFRESH_MENU, CTSTRING(ALL));
	dropMenu.AppendMenu(MF_SEPARATOR);
	int virtualCounter=1, subCounter=0;
	for(auto i = l.begin(); i != l.end(); ++i, ++virtualCounter) {
		if (i->second.size() > 1) {
			CMenu pathMenu;
			pathMenu.CreatePopupMenu();
			pathMenu.AppendMenu(MF_STRING, IDC_REFRESH_MENU + virtualCounter, CTSTRING(ALL));
			pathMenu.AppendMenu(MF_SEPARATOR);
			for(auto s = i->second.begin(); s != i->second.end(); ++s) {
				pathMenu.AppendMenu(MF_STRING, IDC_REFRESH_MENU_SUBDIRS + subCounter, Text::toT(*s).c_str());
				subCounter++;
			}
			dropMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)pathMenu, Text::toT(i->first).c_str());
		} else {
			dropMenu.AppendMenu(MF_STRING, IDC_REFRESH_MENU + virtualCounter, Text::toT(i->first).c_str());
		}
	}
	
	POINT pt;
	pt.x = tb->rcButton.right;
	pt.y = tb->rcButton.bottom;
	ClientToScreen(&pt);
	dropMenu.TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, m_hWnd);

	return TBDDRET_DEFAULT;
}

LRESULT MainFrame::onRefreshMenu(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/) {

	try {
		auto l = ShareManager::getInstance()->getGroupedDirectories();
		if(wID == IDC_REFRESH_MENU){
			ShareManager::getInstance()->refresh();
		} else if (wID < IDC_REFRESH_MENU_SUBDIRS) {
			int id = wID-IDC_REFRESH_MENU-1;
			ShareManager::getInstance()->refresh(l[id].first);
		} else {
			int id = wID-IDC_REFRESH_MENU_SUBDIRS, counter=0;
			for(auto i = l.begin(); i != l.end(); ++i) {
				if (i->second.size() > 1) {
					for(auto s = i->second.begin(); s != i->second.end(); ++s) {
						if (counter == id) {
							ShareManager::getInstance()->refresh(*s);
							return 0;
						}
						counter++;
					}
				}
			}
		}
	} catch(ShareException) {
		//...
	}

	return 0;
}

void MainFrame::checkGeoUpdate() {
	checkGeoUpdate(true);
	checkGeoUpdate(false);
}

void MainFrame::checkGeoUpdate(bool v6) {
	// update when the database is non-existent or older than 25 days (GeoIP updates every month).
	try {
		File f(GeoManager::getDbPath(v6) + ".gz", File::READ, File::OPEN);
		if(f.getSize() > 0 && static_cast<time_t>(f.getLastModified()) > GET_TIME() - 3600 * 24 * 25) {
			return;
		}
	} catch(const FileException&) { }
	updateGeo(v6);
}

void MainFrame::updateGeo() {
	if(BOOLSETTING(GET_USER_COUNTRY)) {
		updateGeo(true);
		updateGeo(false);
	} else {
		//dwt::MessageBox(this).show(T_("IP -> country mappings are disabled. Turn them back on via Settings > Appearance."),
			//_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
	}
}

void MainFrame::updateGeo(bool v6) {
	auto& conn = conns[v6 ? CONN_GEO_V6 : CONN_GEO_V4];
	if(conn)
		return;

	LogManager::getInstance()->message(str(boost::format("Updating the %1% GeoIP database...") % (v6 ? "IPv6" : "IPv4")), LogManager::LOG_INFO);
	conn.reset(new HttpDownload(Text::fromT(v6 ? links.geoip6 : links.geoip4),
		[this, v6] { postMessageFW(WM_SPEAKER, HTTP_COMPLETED, (LPARAM)(v6 ? CONN_GEO_V6 : CONN_GEO_V4)); }, false));
}

void MainFrame::completeGeoUpdate(bool v6) {
	auto& conn = conns[v6 ? CONN_GEO_V6 : CONN_GEO_V4];
	if(!conn) { return; }
	ScopedFunctor([&conn] { conn.reset(); });

	if(!conn->buf.empty()) {
		try {
			File(GeoManager::getDbPath(v6) + ".gz", File::WRITE, File::CREATE | File::TRUNCATE).write(conn->buf);
			GeoManager::getInstance()->update(v6);
			LogManager::getInstance()->message(str(boost::format("The %1% GeoIP database has been successfully updated") % (v6 ? "IPv6" : "IPv4")), LogManager::LOG_INFO);
			return;
		} catch(const FileException&) { }
	}
	LogManager::getInstance()->message(str(boost::format("The %1% GeoIP database could not be updated") % (v6 ? "IPv6" : "IPv4")), LogManager::LOG_WARNING);
}


LRESULT MainFrame::onAppShow(WORD /*wNotifyCode*/,WORD /*wParam*/, HWND, BOOL& /*bHandled*/) {
	if (::IsIconic(m_hWnd)) {
		if(BOOLSETTING(PASSWD_PROTECT_TRAY)) {
			if(hasPassdlg)
				return 0;

			hasPassdlg = true;
			PassDlg passdlg;
			passdlg.description = TSTRING(PASSWORD_DESC);
			passdlg.title = TSTRING(PASSWORD_TITLE);
			passdlg.ok = TSTRING(UNLOCK);
			if(passdlg.DoModal(/*m_hWnd*/) == IDOK){
				tstring tmp = passdlg.line;
				if (tmp == Text::toT(Util::base64_decode(SETTING(PASSWORD)))) {
					ShowWindow(SW_SHOW);
					ShowWindow(maximized ? SW_MAXIMIZE : SW_RESTORE);
				}
				hasPassdlg = false;
			}
		} else {
			ShowWindow(SW_SHOW);
			ShowWindow(maximized ? SW_MAXIMIZE : SW_RESTORE);
		}

	} else {
		SetForegroundWindow(m_hWnd);	
	}
	return 0;
}

void MainFrame::completeVersionUpdate() {

	if(!conns[CONN_VERSION]) { return; }
	try {
		SimpleXML xml;
		xml.fromXML(conns[CONN_VERSION]->buf);
		xml.stepIn();

		double LangVersion;

		string url;
#		ifdef _WIN64
			if(xml.findChild("URL64")) {
			url = xml.getChildData();
		}
#		else
		if(xml.findChild("URL")) {
			url = xml.getChildData();
		}
#		endif

		//ApexDC
		xml.resetCurrentChild();
		if(!BOOLSETTING(AUTO_DETECT_CONNECTION)) {
			if(BOOLSETTING(IP_UPDATE) && xml.findChild("IP")) {
				string ip = xml.getChildData();
				SettingsManager::getInstance()->set(SettingsManager::EXTERNAL_IP, (!ip.empty() ? ip : AirUtil::getLocalIp()));
			} else if(BOOLSETTING(IP_UPDATE)) {
				SettingsManager::getInstance()->set(SettingsManager::EXTERNAL_IP, AirUtil::getLocalIp());
			}
			xml.resetCurrentChild();
		}

		if(xml.findChild("Version")) {
			double remoteVer = Util::toDouble(xml.getChildData());
			xml.resetCurrentChild();
			double ownVersion = Util::toDouble(VERSIONFLOAT);

#ifdef SVNVERSION
			if (xml.findChild("SVNrev")) {
				remoteVer = Util::toDouble(xml.getChildData());
				xml.resetCurrentChild();
			}
			string tmp = SVNVERSION;
			ownVersion = Util::toDouble(tmp.substr(1, tmp.length()-1));
#endif

			if(remoteVer > ownVersion) {
				xml.resetCurrentChild();
				if(xml.findChild("Title")) {
					const string& title = xml.getChildData();
					xml.resetCurrentChild();
					if(xml.findChild("Message") && !BOOLSETTING(DONT_ANNOUNCE_NEW_VERSIONS)) {
						if(url.empty()) {
							const string& msg = xml.getChildData();
							MessageBox(Text::toT(msg).c_str(), Text::toT(title).c_str(), MB_OK);
						} else {
							string msg = xml.getChildData() + "\r\n" + STRING(OPEN_DOWNLOAD_PAGE);
							UpdateDlg dlg;
							dlg.DoModal();
						}
					}
				}
				xml.resetCurrentChild();
				if(xml.findChild("VeryOldVersion")) {
					if(Util::toDouble(xml.getChildData()) >= Util::toDouble(VERSIONFLOAT)) {
						string msg = xml.getChildAttrib("Message", "Your version of AirDC++ contains a serious bug that affects all users of the DC network or the security of your computer.");
						MessageBox(Text::toT(msg + "\r\nPlease get a new one at " + url).c_str());
						oldshutdown = true;
						PostMessage(WM_CLOSE);
					}
				}
				xml.resetCurrentChild();
				if(xml.findChild("BadVersion")) {
					xml.stepIn();
					while(xml.findChild("BadVersion")) {
						double v = Util::toDouble(xml.getChildAttrib("Version"));
						if(v == Util::toDouble(VERSIONFLOAT)) {
							string msg = xml.getChildAttrib("Message", "Your version of DC++ contains a serious bug that affects all users of the DC network or the security of your computer.");
							MessageBox(Text::toT(msg + "\r\nPlease get a new one at " + url).c_str(), _T("Bad DC++ version"), MB_OK | MB_ICONEXCLAMATION);
							oldshutdown = true;
							PostMessage(WM_CLOSE);
						}
					}
				}
			} else if((!SETTING(LANGUAGE_FILE).empty() && !BOOLSETTING(DONT_ANNOUNCE_NEW_VERSIONS))) {
				if (xml.findChild(CSTRING(AAIRDCPP_LANGUAGE_FILE))) {
					string version = xml.getChildData();
					LangVersion = Util::toDouble(version);
					xml.resetCurrentChild();
					if (xml.findChild("LANGURL")) {
						if (LangVersion > Util::toDouble(CSTRING(AAIRDCPP_LANGUAGE_VERSION))){
							string msg = xml.getChildData()+ "\r\n" + STRING(OPEN_DOWNLOAD_PAGE);
							xml.resetCurrentChild();
							UpdateDlg dlg;
							dlg.DoModal();
						}
						xml.resetCurrentChild();
					}
				}
			}
		}
	} catch (const Exception&) {
		// ...
	}

	conns[CONN_VERSION].reset();

	// check after the version.xml download in case it contains updated GeoIP links.
	/*if(BOOLSETTING(GET_USER_COUNTRY)) {
		checkGeoUpdate();
	} */
}

/**
 * @file
 * $Id: MainFrm.cpp,v 1.20 2004/07/21 13:15:15 bigmuscle Exp
 */