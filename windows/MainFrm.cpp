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
#include "WaitingUsersFrame.h"
#include "LineDlg.h"
#include "HashProgressDlg.h"
#include "PrivateFrame.h"
#include "WinUtil.h"
#include "CDMDebugFrame.h"
#include "InputBox.h"
#include "PopupManager.h"
#include "Wizard.h"

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
#include "../client/UPnPManager.h"
#include "../client/SFVReader.h"


MainFrame* MainFrame::anyMF = NULL;
bool MainFrame::bShutdown = false;
uint64_t MainFrame::iCurrentShutdownTime = 0;
bool MainFrame::isShutdownStatus = false;

MainFrame::MainFrame() : trayMessage(0), maximized(false), lastUpload(-1), lastUpdate(0), 
lastUp(0), lastDown(0), oldshutdown(false), stopperThread(NULL), c(new HttpConnection()), 
closing(false), awaybyminimize(false), missedAutoConnect(false), lastTTHdir(Util::emptyStringT), tabsontop(false),
bTrayIcon(false), bAppMinimized(false), bIsPM(false) 

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
	largeImages.Destroy();
	largeImagesHot.Destroy();
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
			DirectoryListing dl(user);
			try {
				dl.loadFile(*i);
				string tmp;
				tmp.resize(STRING(MATCHED_FILES).size() + 16);
				tmp.resize(snprintf(&tmp[0], tmp.size(), CSTRING(MATCHED_FILES), QueueManager::getInstance()->matchListing(dl)));
				LogManager::getInstance()->message(Util::toString(ClientManager::getInstance()->getNicks(user)) + ": " + tmp);
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

void MainFrame::Terminate() {
	HasUpdate = false; // avoid update question if user double downloads update
	oldshutdown = true;
	PostMessage(WM_CLOSE);
}
LRESULT MainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	TimerManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);
	LogManager::getInstance()->addListener(this);

	HasUpdate = false; 
	
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

	hShutdownIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SHUTDOWN), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

	// attach menu
	m_CmdBar.AttachMenu(m_hMenu);
	// load command bar images
	images.CreateFromImage(IDB_TOOLBAR, 16, 16, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
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
	m_CmdBar.m_arrCommand.Add(ID_FILE_SETTINGS);
	m_CmdBar.m_arrCommand.Add(IDC_NOTEPAD);
	m_CmdBar.m_arrCommand.Add(IDC_NET_STATS);
	m_CmdBar.m_arrCommand.Add(IDC_CDMDEBUG_WINDOW);
	m_CmdBar.m_arrCommand.Add(IDC_SYSTEM_LOG);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_CASCADE);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_TILE_HORZ);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_TILE_VERT);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_MINIMIZE_ALL);
	m_CmdBar.m_arrCommand.Add(ID_WINDOW_RESTORE_ALL);
	m_CmdBar.m_arrCommand.Add(ID_GET_TTH);		
	// use Vista-styled menus on Vista/Win7
	if(WinUtil::getOsMajor() >= 6)
		m_CmdBar._AddVistaBitmapsFromImageList(0, m_CmdBar.m_arrCommand.GetSize());

	// remove old menu
	SetMenu(NULL);

	tbarcreated = false;
	tbarwinampcreated = false;
	HWND hWndToolBar = createToolbar();
	HWND hWndWinampBar = createWinampToolbar();

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndCmdBar);
	AddSimpleReBarBand(hWndToolBar, NULL, TRUE);
	AddSimpleReBarBand(hWndWinampBar, NULL, TRUE);
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
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);
	UISetCheck(ID_VIEW_TRANSFER_VIEW, 1);
	UISetCheck(ID_TOGGLE_TOOLBAR, 1);


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

	c->addListener(this);
	c->setCoralizeState(HttpConnection::CST_NOCORALIZE);
	c->downloadFile((VERSION_URL));

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
	if(BOOLSETTING(OPEN_SYSTEM_LOG)) PostMessage(WM_COMMAND, IDC_SYSTEM_LOG);

	if(!WinUtil::isShift())
		PostMessage(WM_SPEAKER, AUTO_CONNECT);

	PostMessage(WM_SPEAKER, PARSE_COMMAND_LINE);

	try {
		File::ensureDirectory(SETTING(LOG_DIRECTORY));
	} catch (const FileException) {	}

	try {
		ConnectivityManager::getInstance()->setup(true, SettingsManager::INCOMING_DIRECT);
	} catch (const Exception& e) {
		// TODO showPortsError(e.getError());
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

	//updateTray( BOOLSETTING( MINIMIZE_TRAY ) );
	updateTray(true);

	Util::setAway(BOOLSETTING(AWAY));

	ctrlToolbar.CheckButton(IDC_AWAY,BOOLSETTING(AWAY));
	ctrlToolbar.CheckButton(IDC_DISABLE_SOUNDS, BOOLSETTING(SOUNDS_DISABLED));

	if(SETTING(NICK).empty()) {
		PostMessage(WM_COMMAND, ID_FILE_SETTINGS);
	}
	

	//background image
	m_PictureWindow.SubclassWindow(m_hWndMDIClient);
	m_PictureWindow.m_nMessageHandler = CPictureWindow::BackGroundPaint;
	currentPic = SETTING(BACKGROUND_IMAGE);
	m_PictureWindow.Load(Text::toT(currentPic).c_str());
	
	if(BOOLSETTING(TESTWRITE)) {
	TestWrite(true, true, true);
	}
	// We want to pass this one on to the splitter...hope it get's there...
	bHandled = FALSE;
	return 0;
}

HWND MainFrame::createWinampToolbar() {
	if(!tbarwinampcreated){
		winampImages.CreateFromImage(WinUtil::getIconPath(_T("mediatoolbar.bmp")).c_str(), 20, 20, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED | LR_LOADFROMFILE);
		ctrlSmallToolbar.Create(m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST, 0, ATL_IDW_TOOLBAR);
		ctrlSmallToolbar.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS);
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
       return 0;
}

HWND MainFrame::createToolbar() {
	if(!tbarcreated) {
		if(SETTING(TOOLBARIMAGE) == "")
			largeImages.CreateFromImage(IDB_TOOLBAR20, 22, 22, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
		else
			largeImages.CreateFromImage(Text::toT(SETTING(TOOLBARIMAGE)).c_str(), SETTING(TB_IMAGE_SIZE), 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED | LR_LOADFROMFILE);

		if(SETTING(TOOLBARHOTIMAGE) == "")
			largeImagesHot.CreateFromImage(IDB_TOOLBAR20_HOT, 22, 22, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
		else
			largeImagesHot.CreateFromImage(Text::toT(SETTING(TOOLBARHOTIMAGE)).c_str(), SETTING(TB_IMAGE_SIZE_HOT), 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED | LR_LOADFROMFILE);

		ctrlToolbar.Create(m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST, 0, ATL_IDW_TOOLBAR);
		ctrlToolbar.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS);
		ctrlToolbar.SetImageList(largeImages);
		ctrlToolbar.SetHotImageList(largeImagesHot);
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
		tbi.fsStyle |= TBSTYLE_DROPDOWN;
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
		TextFrame::openWindow(*file, false, false);
		File::deleteFile(Text::fromT(*file));
	} else if(wParam == STATS) {
		auto_ptr<TStringList> pstr(reinterpret_cast<TStringList*>(lParam));
		const TStringList& str = *pstr;
		if(ctrlStatus.IsWindow()) {
			bool u = false;
			ctrlStatus.SetText(1, str[0].c_str());
			for(int i = 1; i < 9; i++) {
				int w = WinUtil::getTextWidth(str[i], ctrlStatus.m_hWnd);
				
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
							LogManager::getInstance()->message(STRING(FAILED_TO_SHUTDOWN));
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
    }

	return 0;
}

void MainFrame::parseCommandLine(const tstring& cmdLine)
{
	string::size_type i = 0;
	string::size_type j;

	if( (j = cmdLine.find(_T("dchub://"), i)) != string::npos) {
		WinUtil::parseDchubUrl(cmdLine.substr(j), false);
	}
	if( (j = cmdLine.find(_T("nmdcs://"), i)) != string::npos) {
		WinUtil::parseDchubUrl(cmdLine.substr(j), true);
	}	
	if( (j = cmdLine.find(_T("adc://"), i)) != string::npos) {
		WinUtil::parseADChubUrl(cmdLine.substr(j), false);
	}
	if( (j = cmdLine.find(_T("adcs://"), i)) != string::npos) {
		WinUtil::parseADChubUrl(cmdLine.substr(j), true);
	}
	if( (j = cmdLine.find(_T("magnet:?"), i)) != string::npos) {
		WinUtil::parseMagnetUri(cmdLine.substr(j));
	}
}

LRESULT MainFrame::onCopyData(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	tstring cmdLine = (LPCTSTR) (((COPYDATASTRUCT *)lParam)->lpData);
	parseCommandLine(Text::toT(WinUtil::getAppName() + " ") + cmdLine);
	return true;
}

LRESULT MainFrame::onHashProgress(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	HashProgressDlg(false).DoModal(m_hWnd);
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
		case IDC_UPLOAD_QUEUE: WaitingUsersFrame::openWindow(); break;
		case IDC_CDMDEBUG_WINDOW: CDMDebugFrame::openWindow(); break;
		case IDC_RECENTS: RecentHubsFrame::openWindow(); break;
		case IDC_SYSTEM_LOG: SystemFrame::openWindow(); break;

	
		default: dcassert(0); break;
	}
	return 0;
}

LRESULT MainFrame::OnFileSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	PropertiesDlg dlg(m_hWnd, SettingsManager::getInstance());

	unsigned short lastTCP = static_cast<unsigned short>(SETTING(TCP_PORT));
	unsigned short lastUDP = static_cast<unsigned short>(SETTING(UDP_PORT));
	unsigned short lastTLS = static_cast<unsigned short>(SETTING(TLS_PORT));

	int lastConn = SETTING(INCOMING_CONNECTIONS);

	bool lastSortFavUsersFirst = BOOLSETTING(SORT_FAVUSERS_FIRST);

	if(dlg.DoModal(m_hWnd) == IDOK) 
	{
		SettingsManager::getInstance()->save();
		if(missedAutoConnect && !SETTING(NICK).empty()) {
			PostMessage(WM_SPEAKER, AUTO_CONNECT);
		}

		try {
			ConnectivityManager::getInstance()->setup(SETTING(INCOMING_CONNECTIONS) != lastConn || SETTING(TCP_PORT) != lastTCP || SETTING(UDP_PORT) != lastUDP || SETTING(TLS_PORT) != lastTLS, lastConn );
		} catch (const Exception& e) {
			// TODO: showPortsError(e.getError());
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
		ClientManager::getInstance()->infoUpdated();
	}
	return 0;
}

void MainFrame::on(HttpConnectionListener::Complete, HttpConnection* /*aConn*/, const string&, bool /*fromCoral*/) throw() {
	try {
		SimpleXML xml;
		xml.fromXML(versionInfo);
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
		if(BOOLSETTING(IP_UPDATE) && xml.findChild("IP")) {
			string ip = xml.getChildData();
			SettingsManager::getInstance()->set(SettingsManager::EXTERNAL_IP, (!ip.empty() ? ip : Util::getLocalIp()));
		} else if(BOOLSETTING(IP_UPDATE)) {
			SettingsManager::getInstance()->set(SettingsManager::EXTERNAL_IP, Util::getLocalIp());
		}
		xml.resetCurrentChild();
		if(xml.findChild("Version")) {
			if(Util::toDouble(xml.getChildData()) > VERSIONFLOAT) {
				xml.resetCurrentChild();
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
					if(Util::toDouble(xml.getChildData()) >= VERSIONFLOAT) {
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
						if(v == VERSIONFLOAT) {
							string msg = xml.getChildAttrib("Message", "Your version of DC++ contains a serious bug that affects all users of the DC network or the security of your computer.");
							MessageBox(Text::toT(msg + "\r\nPlease get a new one at " + url).c_str(), _T("Bad DC++ version"), MB_OK | MB_ICONEXCLAMATION);
							oldshutdown = true;
							PostMessage(WM_CLOSE);
						}
					}
				}
			} else 
				if((!SETTING(LANGUAGE_FILE).empty() && !BOOLSETTING(DONT_ANNOUNCE_NEW_VERSIONS))) {
					if (xml.findChild(Text::fromT(TSTRING(AAIRDCPP_LANGUAGE_FILE)))) {
						string version = xml.getChildData();
						LangVersion = Util::toDouble(version);
						xml.resetCurrentChild();
						if (xml.findChild("LANGURL")) {
							if (LangVersion > Util::toDouble(Text::fromT(TSTRING(AAIRDCPP_LANGUAGE_VERSION)))){
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
		
	if(BOOLSETTING(FIRST_RUN)) {
		FavoriteHubEntry e;
			
		e.setName("FXR Team AirDC++ support hub");
		e.setDescription("AirDC++");
		e.setServer("fxr.airdcpp.net");
			FavoriteManager::getInstance()->addFavorite(e);
		SettingsManager::getInstance()->set(SettingsManager::FIRST_RUN, false);
	}

	int left = SETTING(OPEN_FIRST_X_HUBS);
	FavoriteHubEntry::List& fh = FavoriteManager::getInstance()->getFavoriteHubs();
	if(left > (int)fh.size()) {
		left = fh.size();
	}
	missedAutoConnect = false;
	for(FavoriteHubEntry::List::const_iterator i = fl.begin(); i != fl.end(); ++i) {
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
		SetProcessWorkingSetSize(GetCurrentProcess(), 0xffffffff, 0xffffffff);
		if(BOOLSETTING(AUTO_AWAY)) {
			if(bAppMinimized == false)
			if(Util::getAway() == true) {
				awaybyminimize = false;
			} else {
				awaybyminimize = true;
				Util::setAway(true);
				setAwayButton(true);
				ClientManager::getInstance()->infoUpdated();
			}
		}

		if(BOOLSETTING(MINIMIZE_TRAY) != WinUtil::isShift()) {
			ShowWindow(SW_HIDE);
			SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
		}
		bAppMinimized = true;
		maximized = IsZoomed() > 0;
	} else if( (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED) ) {
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
		if(BOOLSETTING(AUTO_AWAY)) {
			if(awaybyminimize == true) {
				awaybyminimize = false;
				Util::setAway(false);
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
	if(c != NULL) {
		c->removeListener(this);
		delete c;
		c = NULL;
	}

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
	
	QueueManager::getInstance()->saveQueue();
	SettingsManager::getInstance()->save();
	
	return 0;
}

LRESULT MainFrame::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(c != NULL) {
		c->removeListener(this);
		delete c;
		c = NULL;
	}
		if(HasUpdate) {
					if (Util::fileExists(Util::getPath(Util::PATH_RESOURCES) + INSTALLER)) {
						//string file = (Util::getPath(Util::PATH_RESOURCES) + "AirDC_Installer.exe");
					if(MessageBox(CTSTRING(UPDATE_FILE_DETECTED), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
						::ShellExecute(NULL, NULL, Text::toT(Util::getPath(Util::PATH_RESOURCES) + INSTALLER).c_str(), NULL, NULL, SW_SHOWNORMAL);
						Terminate();
					}
			}
			HasUpdate = false;
	}
	if(!closing) {
		if( oldshutdown ||(!BOOLSETTING(CONFIRM_EXIT)) || (MessageBox(CTSTRING(REALLY_EXIT), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) ) {
			updateTray(false);
			string tmp1;
			string tmp2;

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
		bHandled = FALSE;
	}

	return 0;
}

LRESULT MainFrame::onSwitchWindow(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlTab.SwitchWindow(wID - IDC_SWITCH_WINDOW_1);
	return 0;
}

LRESULT MainFrame::onLink(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	tstring site;
	bool isFile = false;
	switch(wID) {
		case IDC_HELP_HOMEPAGE: site = _T("http://www.airdcpp.net"); break;
		case IDC_HELP_GEOIPFILE: site = _T("http://www.maxmind.com/download/geoip/database/GeoIPCountryCSV.zip"); break;
		case IDC_HELP_TRANSLATIONS: site = _T("http://www.airdcpp.net"); break;
		case IDC_HELP_FAQ: site = _T("http://www.airdcpp.net"); break;
		case IDC_HELP_DISCUSS: site = _T("http://www.airdcpp.net"); break;
		case IDC_HELP_DONATE: site = _T("http://www.airdcpp.net"); break;
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
	tstring file;
	
	if(wID == IDC_OPEN_MY_LIST){
		if(!ShareManager::getInstance()->getOwnListFile().empty()){
			DirectoryListingFrame::openWindow(Text::toT(ShareManager::getInstance()->getOwnListFile()), Text::toT(Util::emptyString), HintedUser(ClientManager::getInstance()->getMe(), Util::emptyString), 0);
		}
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
	ShareManager::getInstance()->setDirty();
	ShareManager::getInstance()->refresh(true);
	return 0;
}

LRESULT MainFrame::onScanMissing(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

		SFVReaderManager::getInstance()->scan();
		
		return 0;
}

LRESULT MainFrame::onTrayIcon(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	if (lParam == WM_LBUTTONUP) {
		if(bAppMinimized) {
	if(BOOLSETTING(PASSWD_PROTECT_TRAY)) {
		PassDlg dlg;
		dlg.description = TSTRING(PASSWORD_DESC);
		dlg.title = TSTRING(PASSWORD_TITLE);
		dlg.ok = TSTRING(UNLOCK);
		if(dlg.DoModal(/*m_hWnd*/) == IDOK){
			tstring tmp = dlg.line;
			if (tmp == Text::toT(Util::base64_decode(SETTING(PASSWORD)))) {
			ShowWindow(SW_SHOW);
			ShowWindow(maximized ? SW_MAXIMIZE : SW_RESTORE);
			}
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
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 2);	// toolbar is 2nd added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_TOGGLE_TOOLBAR, bVisible);
	UpdateLayout();
	SettingsManager::getInstance()->set(SettingsManager::SHOW_WINAMP_CONTROL, bVisible);
	return 0;
}

LRESULT MainFrame::onWinampStart(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

if(SETTING(WINAMP_PATH) != Util::emptyString) {
::ShellExecute(NULL, NULL, Text::toT(SETTING(WINAMP_PATH)).c_str(), NULL , NULL, SW_SHOWNORMAL);

}
return 0;
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
LRESULT MainFrame::Opensyslog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{

tstring filename = Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatTime(SETTING(LOG_FILE_SYSTEM), time(NULL))));
	if(Util::fileExists(Text::fromT(filename))){
			if(BOOLSETTING(OPEN_LOGS_INTERNAL) == false) {
			ShellExecute(NULL, NULL, filename.c_str(), NULL, NULL, SW_SHOWNORMAL);
		} else {
			TextFrame::openWindow(filename, true, false);
		}
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
	LineDlg dlg;
	dlg.description = TSTRING(HUB_ADDRESS);
	dlg.title = TSTRING(QUICK_CONNECT);
	if(dlg.DoModal(m_hWnd) == IDOK){
		if(SETTING(NICK).empty())
			return 0;

		tstring tmp = dlg.line;
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
		HubFrame::openWindow(tmp);
	}
	return 0;
}

void MainFrame::on(TimerManagerListener::Second, uint64_t aTick) throw() {
		if(aTick == lastUpdate)	// FIXME: temp fix for new TimerManager
		return;

	Util::increaseUptime();
		int64_t diff = (int64_t)((lastUpdate == 0) ? aTick - 1000 : aTick - lastUpdate);
		int64_t updiff = Socket::getTotalUp() - lastUp;
		int64_t downdiff = Socket::getTotalDown() - lastDown;

		TStringList* str = new TStringList();
		str->push_back(Util::getAway() ? TSTRING(AWAY) : _T(""));
		str->push_back(TSTRING(SHARED) + _T(": ") + Util::formatBytesW(ShareManager::getInstance()->getSharedSize()));
		str->push_back(_T("H: ") + Text::toT(Client::getCounts()));
		str->push_back(TSTRING(SLOTS) + _T(": ") + Util::toStringW(UploadManager::getInstance()->getFreeSlots()) + _T('/') + Util::toStringW(UploadManager::getInstance()->getSlots()) + _T(" (") + Util::toStringW(UploadManager::getInstance()->getFreeExtraSlots()) + _T('/') + Util::toStringW(SETTING(EXTRA_SLOTS)) + _T(")"));
		str->push_back(_T("D: ") + Util::formatBytesW(Socket::getTotalDown()));
		str->push_back(_T("U: ") + Util::formatBytesW(Socket::getTotalUp()));
		str->push_back(_T("D: [") + Util::toStringW(DownloadManager::getInstance()->getDownloadCount()) + _T("]") + Util::formatBytesW(downdiff*1000I64/diff) + _T("/s"));
		str->push_back(_T("U: [") + Util::toStringW(UploadManager::getInstance()->getUploadCount()) + _T("]") + Util::formatBytesW(updiff*1000I64/diff) + _T("/s"));
		str->push_back(TSTRING(QUEUE_SIZE) + _T(": ")  + Util::formatBytesW(QueueManager::getInstance()->fileQueue.getTotalQueueSize()));

		PostMessage(WM_SPEAKER, STATS, (LPARAM)str);
		SettingsManager::getInstance()->set(SettingsManager::TOTAL_UPLOAD, SETTING(TOTAL_UPLOAD) + updiff);
		SettingsManager::getInstance()->set(SettingsManager::TOTAL_DOWNLOAD, SETTING(TOTAL_DOWNLOAD) + downdiff);
		lastUpdate = aTick;
		lastUp = Socket::getTotalUp();
		lastDown = Socket::getTotalDown();

			if(SETTING(DISCONNECT_SPEED) < 1) {
			SettingsManager::getInstance()->set(SettingsManager::DISCONNECT_SPEED, 1);
		}
		
		if(currentPic != SETTING(BACKGROUND_IMAGE)) {
				currentPic = SETTING(BACKGROUND_IMAGE);
				m_PictureWindow.Load(Text::toT(currentPic).c_str());
		}

			time_t currentTime;
			time(&currentTime);
			

			
}

void MainFrame::on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const uint8_t* buf, size_t len) throw() {
	versionInfo += string((const char*)buf, len);
}

void MainFrame::on(HttpConnectionListener::Retried, HttpConnection* /*conn*/, const bool Connected) throw() {
 	if (Connected)
 		versionInfo = Util::emptyString;
}
void MainFrame::on(PartialList, const HintedUser& aUser, const string& text) throw() {
	PostMessage(WM_SPEAKER, BROWSE_LISTING, (LPARAM)new DirectoryBrowseInfo(aUser, text));
}

void MainFrame::on(QueueManagerListener::Finished, const QueueItem* qi, const string& dir, const Download* download) throw() {
	if(qi->isSet(QueueItem::FLAG_CLIENT_VIEW)) {
		if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
			// This is a file listing, show it...
			DirectoryListInfo* i = new DirectoryListInfo(qi->getDownloads()[0]->getHintedUser(), Text::toT(qi->getListName()), Text::toT(dir), static_cast<int64_t>(download->getAverageSpeed()));

			PostMessage(WM_SPEAKER, DOWNLOAD_LISTING, (LPARAM)i);
		} else if(qi->isSet(QueueItem::FLAG_TEXT)) {
			PostMessage(WM_SPEAKER, VIEW_FILE_AND_DELETE, (LPARAM) new tstring(Text::toT(qi->getTarget())));
		}
	} else if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
		DirectoryListInfo* i = new DirectoryListInfo(qi->getDownloads()[0]->getHintedUser(), Text::toT(qi->getListName()), Text::toT(dir), static_cast<int64_t>(download->getAverageSpeed()));
		
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
			DirectoryListing* dl = new DirectoryListing(i->user);
			try {
				dl->loadFile(Text::fromT(i->file));
				if(BOOLSETTING(USE_ADLS)) {
				ADLSearchManager::getInstance()->matchListing(*dl);
				}
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
		if(incomplete) {
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
	} else if(ready) { //dont need this but leave it for now.
		LogManager::getInstance()->message("Test write to AirDC++ common folders succeeded." );
	}

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

	//Set the MNS_NOTIFYBYPOS flag to receive WM_MENUCOMMAND
	MENUINFO inf;
	inf.cbSize = sizeof(MENUINFO);
	inf.fMask = MIM_STYLE;
	inf.dwStyle = MNS_NOTIFYBYPOS;
	dropMenu.SetMenuInfo(&inf);

	StringList l = ShareManager::getInstance()->getVirtualNames();
	
	dropMenu.AppendMenu(MF_STRING, IDC_REFRESH_MENU, CTSTRING(ALL));
	dropMenu.AppendMenu(MF_SEPARATOR);
	int j = 1;
	for(StringIter i = l.begin(); i != l.end(); ++i, ++j)
		dropMenu.AppendMenu(MF_STRING, IDC_REFRESH_MENU, Text::toT( *i ).c_str());
	
	POINT pt;
	pt.x = tb->rcButton.right;
	pt.y = tb->rcButton.bottom;
	ClientToScreen(&pt);
	dropMenu.TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, m_hWnd);

	return TBDDRET_DEFAULT;
}

LRESULT MainFrame::onRefreshMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	try
	{
		ShareManager::getInstance()->setDirty();
		if(wParam == 0){
			ShareManager::getInstance()->refresh( ShareManager::REFRESH_ALL | ShareManager::REFRESH_UPDATE );
		} else if(wParam > 1){
			int id = wParam - 2;
			StringList l = ShareManager::getInstance()->getVirtualNames();
			ShareManager::getInstance()->refresh( l[id] );
		}
	} catch(ShareException) {
		//...
	}

	return 0;
}

/**
 * @file
 * $Id: MainFrm.cpp,v 1.20 2004/07/21 13:15:15 bigmuscle Exp
 */