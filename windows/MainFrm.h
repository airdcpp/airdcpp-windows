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

#if !defined(MAIN_FRM_H)
#define MAIN_FRM_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <airdcpp/concurrency.h>

#include <airdcpp/ActivityManager.h>
#include <airdcpp/ClientManagerListener.h>
#include <airdcpp/DirectoryListingManagerListener.h>
#include <airdcpp/LogManagerListener.h>
#include <airdcpp/PrivateChatManagerListener.h>
#include <airdcpp/QueueManagerListener.h>
#include <airdcpp/TimerManagerListener.h>
#include <airdcpp/UpdateManagerListener.h>
#include <airdcpp/ViewFileManagerListener.h>

#include "Async.h"
#include "Dispatchers.h"
#include "FlatTabCtrl.h"
#include "SingleInstance.h"
#include "OMenu.h"
#include "picturewindow.h"
#include "CProgressCtrlEx.h"

#define STATUS_MESSAGE_MAP 9
#define POPUP_UID 19000

class HashProgressDlg;
class TransferView;

class MainFrame : public CMDIFrameWindowImpl<MainFrame>, public CUpdateUI<MainFrame>,
		public CMessageFilter, public CIdleHandler, public CSplitterImpl<MainFrame>,
		private TimerManagerListener, private QueueManagerListener, public Async<MainFrame>,
		private LogManagerListener, private DirectoryListingManagerListener, private UpdateManagerListener, private ClientManagerListener,
		private PrivateChatManagerListener, private ActivityManagerListener, private ViewFileManagerListener
{
public:
	MainFrame();
	virtual ~MainFrame();
	DECLARE_FRAME_WND_CLASS(_T("AirDC++"), IDR_MAINFRAME)

	CMDICommandBarCtrl m_CmdBar;

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		if (!IsWindow())
			return FALSE;

		if((pMsg->message >= WM_MOUSEFIRST) && (pMsg->message <= WM_MOUSELAST))
			ctrlTooltips.RelayEvent(pMsg);

		if(CMDIFrameWindowImpl<MainFrame>::PreTranslateMessage(pMsg))
			return TRUE;
		
		HWND hWnd = MDIGetActive();
		if(hWnd != NULL)
			return (BOOL)::SendMessage(hWnd, WM_FORWARDMSG, 0, (LPARAM)pMsg);

		return FALSE;
	}
	
	BOOL OnIdle()
	{
		UIUpdateToolBar();
		return FALSE;
	}
	typedef CSplitterImpl<MainFrame> splitterBase;
	BEGIN_MSG_MAP(MainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(FTM_SELECTED, onSelected)
		MESSAGE_HANDLER(FTM_ROWS_CHANGED, onRowsChanged)
		MESSAGE_HANDLER(WM_APP+242, onTrayIcon)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		MESSAGE_HANDLER(WM_QUERYOPEN, onOpen)
		MESSAGE_HANDLER(WM_ENDSESSION, onEndSession)
		MESSAGE_HANDLER(trayMessage, onTray)
		MESSAGE_HANDLER(tbButtonMessage, onTaskbarButton);
		MESSAGE_HANDLER(WM_COPYDATA, onCopyData)
		MESSAGE_HANDLER(WMU_WHERE_ARE_YOU, onWhereAreYou)
		MESSAGE_HANDLER(WM_ACTIVATEAPP, onActivateApp)
		MESSAGE_HANDLER(WM_APPCOMMAND, onAppCommand)
		MESSAGE_HANDLER(IDC_REBUILD_TOOLBAR, OnCreateToolbar)
		MESSAGE_HANDLER(IDC_SET_FONTS, onSetFont)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_FILE_SETTINGS, OnFileSettings)
		COMMAND_ID_HANDLER(IDC_MATCH_ALL, onMatchAll)
		COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_VIEW_TRANSFER_VIEW, OnViewTransferView)
		COMMAND_ID_HANDLER(ID_GET_TTH, onGetTTH)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(ID_WIZARD, OnWizard)
		COMMAND_ID_HANDLER(ID_WINDOW_CASCADE, OnWindowCascade)
		COMMAND_ID_HANDLER(ID_WINDOW_TILE_HORZ, OnWindowTile)
		COMMAND_ID_HANDLER(ID_WINDOW_TILE_VERT, OnWindowTileVert)
		COMMAND_ID_HANDLER(ID_WINDOW_ARRANGE, OnWindowArrangeIcons)
		COMMAND_ID_HANDLER(IDC_RECENTS, onOpenWindows)
		COMMAND_ID_HANDLER(ID_FILE_CONNECT, onOpenWindows)
		COMMAND_ID_HANDLER(ID_FILE_SEARCH, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_FAVORITES, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_FAVUSERS, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_NOTEPAD, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_AUTOSEARCH, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_SYSTEM_LOG, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_QUEUE, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_SEARCH_SPY, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_FILE_ADL_SEARCH, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_FINISHED_UL, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_UPLOAD_QUEUE, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_CDMDEBUG_WINDOW, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_SYSTEM_LOG, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_AWAY, onAway)
		COMMAND_ID_HANDLER(IDC_HELP_HOMEPAGE, onLink)
		COMMAND_ID_HANDLER(IDC_HELP_DONATE, onLink)
		COMMAND_ID_HANDLER(IDC_HELP_CUSTOMIZE, onLink)
		COMMAND_ID_HANDLER(IDC_HELP_GUIDES, onLink)
		COMMAND_ID_HANDLER(IDC_HELP_DISCUSS, onLink)
		COMMAND_ID_HANDLER(IDC_HELP_BUG_TRACKER, onLink)
		COMMAND_ID_HANDLER(IDC_OPEN_FILE_LIST, onOpenFileList)
		COMMAND_ID_HANDLER(IDC_BROWSE_OWN_LIST, onOpenOwnList)
		COMMAND_ID_HANDLER(IDC_TRAY_SHOW, onAppShow)
		COMMAND_ID_HANDLER(ID_TOGGLE_TOOLBAR, OnViewWinampBar)
		COMMAND_ID_HANDLER(ID_TOGGLE_TBSTATUS, OnViewTBStatusBar)
		COMMAND_ID_HANDLER(ID_LOCK_TB, OnLockTB)
		COMMAND_ID_HANDLER(ID_WINDOW_MINIMIZE_ALL, onWindowMinimizeAll)
		COMMAND_ID_HANDLER(ID_WINDOW_RESTORE_ALL, onWindowRestoreAll)
		COMMAND_ID_HANDLER(IDC_WINDOW_MARK_READ, onWindowMarkAsRead)
		COMMAND_ID_HANDLER(IDC_UPDATE, onUpdate)
		COMMAND_ID_HANDLER(IDC_CLOSE_DISCONNECTED, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_CLOSE_ALL_PM, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_CLOSE_ALL_OFFLINE_PM, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_CLOSE_ALL_DIR_LIST, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_CLOSE_ALL_SEARCH_FRAME, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_RECONNECT_DISCONNECTED, onReconnectDisconnected)
		COMMAND_ID_HANDLER(IDC_OPEN_DOWNLOADS, onOpenDir)
		COMMAND_ID_HANDLER(IDC_OPEN_LOG_DIR, onOpenDir)
		COMMAND_ID_HANDLER(IDC_OPEN_CONFIG_DIR, onOpenDir)
		COMMAND_ID_HANDLER(IDC_REFRESH_FILE_LIST, onRefreshFileList)
		COMMAND_ID_HANDLER(IDC_EXTENSIONS, onOpenWindows)
		COMMAND_ID_HANDLER(ID_FILE_QUICK_CONNECT, onQuickConnect)
		COMMAND_ID_HANDLER(IDC_HASH_PROGRESS, onHashProgress)
		COMMAND_ID_HANDLER(IDC_HASH_PROGRESS_AUTO_CLOSE, onHashProgress)
		COMMAND_ID_HANDLER(IDC_OPEN_SYSTEMLOG, onOpenSysLog)
		COMMAND_ID_HANDLER(IDC_RSSFRAME, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_EXTENSIONS, onOpenWindows)
		COMMAND_RANGE_HANDLER(IDC_WINAMP_BACK, IDC_WINAMP_VOL_HALF, onWinampButton)

		MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
		MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)

		COMMAND_ID_HANDLER(IDC_WINAMP_START, onWinampStart)
		NOTIFY_CODE_HANDLER(TBN_DROPDOWN, onRefreshDropDown)
		CHAIN_MDI_CHILD_COMMANDS()
		CHAIN_MSG_MAP(CUpdateUI<MainFrame>)
		CHAIN_MSG_MAP(CSplitterImpl<MainFrame>)
		CHAIN_MSG_MAP(CMDIFrameWindowImpl<MainFrame>)
		ALT_MSG_MAP(STATUS_MESSAGE_MAP)
			NOTIFY_CODE_HANDLER(TTN_GETDISPINFO, onGetToolTip)
			MESSAGE_HANDLER(WM_LBUTTONUP, onStatusBarClick)
			MESSAGE_HANDLER(WM_RBUTTONUP, onStatusBarClick)
			MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
			MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)

	END_MSG_MAP()

	BEGIN_UPDATE_UI_MAP(MainFrame)
		UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_TRANSFER_VIEW, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_TOGGLE_TOOLBAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_TOGGLE_TBSTATUS, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_LOCK_TB, UPDUI_MENUPOPUP)
	END_UPDATE_UI_MAP()

	LRESULT onOpen(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onHashProgress(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onEndSession(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onGetTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMatchAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnWizard(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onLink(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenFileList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenOwnList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onTrayIcon(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewTransferView(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGetToolTip(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onCopyData(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onCloseWindows(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onReconnectDisconnected(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRefreshFileList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onQuickConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onActivateApp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onAppCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onAway(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenWindows(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnViewWinampBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onWinampButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onWinampStart(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onOpenSysLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onRefreshDropDown(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onSetFont(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnViewTBStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnLockTB(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	static DWORD WINAPI stopper(void* p);
	void UpdateLayout(BOOL bResizeBars = TRUE);
	void parseCommandLine(const tstring& cmdLine);

	void openSettings(uint16_t initialPage=0);

	LRESULT onWhereAreYou(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return WMU_WHERE_ARE_YOU;
	}

	LRESULT onTaskbarButton(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onTray(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onRowsChanged(UINT /*uMsg*/, WPARAM /* wParam */, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	void shutdown();

	void TestWrite(bool downloads, bool appPath);

	LRESULT onSelected(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	
	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return 0;
	}

	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		fMenuShutdown = true;
		PostMessage(WM_CLOSE);
		return 0;
	}
	
	LRESULT onOpenDir(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnWindowCascade(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		MDICascade();
		return 0;
	}

	LRESULT OnWindowTile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		MDITile();
		return 0;
	}

	LRESULT OnWindowTileVert(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		MDITile(MDITILE_VERTICAL);
		return 0;
	}

	LRESULT OnWindowArrangeIcons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		MDIIconArrange();
		return 0;
	}

	LRESULT onWindowMinimizeAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onWindowRestoreAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);	

	LRESULT onWindowMarkAsRead(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		ctrlTab.unboldTabs();
		return 0;
	}

	LRESULT onShutDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		setShutDown(!getShutDown());
		return S_OK;
	}
	LRESULT OnCreateToolbar(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		createToolbar();
		return S_OK;
	}

	LRESULT onStatusBarClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	static MainFrame* getMainFrame() { return anyMF; }
	bool getAppMinimized() const { return bAppMinimized; }
	CToolBarCtrl& getToolBar() { return ctrlToolbar; }

	static void setShutDown(bool b);
	static bool getShutDown() { return bShutdown; }
	
	static void setAwayButton(bool check) {
		anyMF->ctrlToolbar.CheckButton(IDC_AWAY, check);
	}

	CImageList ToolbarImages, ToolbarImagesHot;
	void getMagnetForFile();
	
	
	void ShowPopup(tstring szMsg, tstring szTitle, DWORD dwInfoFlags = NIIF_INFO, bool force = false) {
		dcassert(dwInfoFlags != NIIF_USER);
		ShowPopup(szMsg, szTitle, dwInfoFlags, NULL, force);
	}

	void ShowPopup(tstring szMsg, tstring szTitle, HICON hIcon, bool force = false) {
		ShowPopup(szMsg, szTitle, NIIF_USER, hIcon, force);
	}
	void ShowPopup(tstring szMsg, tstring szTitle, DWORD dwInfoFlags = NIIF_INFO, HICON hIcon = NULL, bool force = false);

	void addThreadedTask(std::function<void ()> aF);
	task_group threadedTasks;

	/* Displays a message box that will also display correctly with the splash */
	void showMessageBox(const tstring& aMsg, UINT aFlags, const tstring& aTitle = Util::emptyStringT);
	void onChatMessage(bool pm);

	
private:
	void updateStatus(TStringList* aItems);
	void addStatus(const LogMessagePtr& aMessage);

	void initTooltips();
	void initStatusbar();
	void initCmdBar();

	NOTIFYICONDATA pmicon;
	NOTIFYICONDATA hubicon;
	NOTIFYICONDATA hubPmicon;
	
	unique_ptr<TransferView> transferView;
	static MainFrame* anyMF;
	
	enum { MAX_CLIENT_LINES = 10 };
	deque<tstring> lastLinesList;
	tstring lastLines;
	CToolTipCtrl ctrlTooltips;

	CStatusBarCtrl ctrlStatus;
	FlatTabCtrl ctrlTab;
	CImageList images;
	CImageList winampImages;
	CToolBarCtrl ctrlToolbar;
	CToolBarCtrl ctrlSmallToolbar;
	
	CProgressCtrlEx progress;
	CToolBarCtrl TBStatusCtrl;
	HWND createTBStatusBar();

	void updateTBStatusHashing(const string& aFile, int64_t aSize, size_t aFiles, int64_t aSpeed, int aHashers, bool aPaused);
	void updateTBStatusRefreshing();

	bool refreshing = false;
	int64_t startBytes = 0;
	size_t startFiles = 0;

	CPictureWindow m_PictureWindow;
	string currentPic;

	bool tbarcreated = false;
	bool tbarwinampcreated = false;
	bool bTrayIcon = false;
	bool bAppMinimized = false;
	bool bHasPM = false;
	bool bHasMC = false;
	bool fMenuShutdown = false;
	
	unique_ptr<HashProgressDlg> hashProgress;

	static bool bShutdown;
	static uint64_t iCurrentShutdownTime;
	CIcon hShutdownIcon;
	CIcon slotsIcon;
	CIcon slotsFullIcon;

	CContainedWindow statusContainer;

	static bool isShutdownStatus;

	UINT trayMessage = 0;
	UINT tbButtonMessage = 0;
	UINT trayUID = 0;

	void onTrayMenu();
	void fillLimiterMenu(OMenu* menu, bool upload);

	typedef BOOL (CALLBACK* LPFUNC)(UINT message, DWORD dwFlag);
	LPFUNC _d_ChangeWindowMessageFilter;
	HMODULE user32lib;

	CComPtr<ITaskbarList3> taskbarList;

	/** Was the window maximized when minimizing it? */
	bool maximized = false;
	uint64_t lastMove = 0;
	uint64_t lastUpdate = 0;

	bool forcedShutdown = false;

	bool tabsontop = false;
	bool closing = false;

	int statusSizes[10];

	HANDLE stopperThread = NULL;

	bool settingsWindowOpen = false;
	HWND createToolbar();
	HWND createWinampToolbar();
	void updateTray(bool add = true);
	void updateTooltipRect();
	void updateActivity();
	void onAwayButton();

	void saveOpenWindows();
	void loadOpenWindows();
	enum {
		STATUS_LASTLINES,
		STATUS_AWAY,
		STATUS_SHARED,
		STATUS_HUBS,
		STATUS_SLOTS,
		STATUS_DOWNLOADED,
		STATUS_UPLOADED,
		STATUS_DL_SPEED,
		STATUS_UL_SPEED,
		STATUS_QUEUED,
		STATUS_SHUTDOWN,
		STATUS_LAST
	};
	
	LRESULT onAppShow(WORD /*wNotifyCode*/,WORD /*wParam*/, HWND, BOOL& /*bHandled*/);

	void setTrayIcon(HICON aIcon);

	// LogManagerListener
	void on(LogManagerListener::Message, const LogMessagePtr& aMessage) noexcept;

	// TimerManagerListener
	void on(TimerManagerListener::Second, uint64_t aTick) noexcept;

	// PrivateChatManagerListener
	void on(PrivateChatManagerListener::ChatCreated, const PrivateChatPtr& aChat, bool aMessageReceived) noexcept;

	// QueueManagerListener
	void on(QueueManagerListener::ItemFinished, const QueueItemPtr& qi, const string& dir, const HintedUser& aUser, int64_t aSpeed) noexcept;
	void on(QueueManagerListener::BundleRemoved, const BundlePtr& aBundle) noexcept;

	// DirectoryListingManagerListener
	void on(DirectoryListingManagerListener::OpenListing, const DirectoryListingPtr& aList, const string& aDir, const string& aXML) noexcept;

	//ActivityManagerListener
	void on(ActivityManagerListener::AwayModeChanged, AwayMode aNewMode) noexcept;

	void on(ViewFileManagerListener::FileFinished, const ViewFilePtr& aFile) noexcept;

	void onUpdateAvailable(const string& title, const string& message, const string& aVersionString, const string& infoUrl, bool autoUpdate, int build, const string& autoUpdateUrl) noexcept;
	void onBadVersion(const string& message, const string& url, const string& update, int buildID, bool canAutoUpdate) noexcept;
	void onUpdateComplete(const string& updater) noexcept;

	void on(UpdateManagerListener::UpdateAvailable, const string& title, const string& message, const string& aVersionString, const string& infoUrl, bool autoUpdate, int build, const string& autoUpdateUrl) noexcept;
	void on(UpdateManagerListener::BadVersion, const string& message, const string& url, const string& update, int buildID, bool canAutoUpdate) noexcept;
	void on(UpdateManagerListener::UpdateComplete, const string& updater) noexcept;
	void on(UpdateManagerListener::UpdateFailed, const string& line) noexcept;
	void on(UpdateManagerListener::VersionFileDownloaded, SimpleXML&) noexcept;

	void on(ClientManagerListener::ClientCreated, const ClientPtr&) noexcept;
};

#endif // !defined(MAIN_FRM_H)