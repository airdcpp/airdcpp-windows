/*
 * Copyright (C) 2001-2016 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef WIN_UTIL_H
#define WIN_UTIL_H

#include "resource.h"

#include "OMenu.h"
#include "SplashWindow.h"

#include <airdcpp/DupeType.h>
#include <airdcpp/HintedUser.h>
#include <airdcpp/MerkleTree.h>
#include <airdcpp/QueueItemBase.h>
#include <airdcpp/SettingItem.h>
#include <airdcpp/SettingsManager.h>
#include <airdcpp/User.h>
#include <airdcpp/Util.h>
#include <airdcpp/modules/WebShortcuts.h>

/* Work around DBTYPE name conflict with Berkeley DB */
#define DBTYPE MS_DBTYPE
#include <atlcomtime.h>
#undef DBTYPE

class RichTextBox;

// Some utilities for handling HLS colors, taken from Jean-Michel LE FOL's codeproject
// article on WTL OfficeXP Menus
typedef DWORD HLSCOLOR;
#define HLS(h,l,s) ((HLSCOLOR)(((BYTE)(h)|((WORD)((BYTE)(l))<<8))|(((DWORD)(BYTE)(s))<<16)))
#define HLS_H(hls) ((BYTE)(hls))
#define HLS_L(hls) ((BYTE)(((WORD)(hls)) >> 8))
#define HLS_S(hls) ((BYTE)((hls)>>16))

HLSCOLOR RGB2HLS (COLORREF rgb);
COLORREF HLS2RGB (HLSCOLOR hls);

COLORREF HLS_TRANSFORM (COLORREF rgb, int percent_L, int percent_S);

class FlatTabCtrl;

template<class T, int title, int ID = -1>
class StaticFrame {
public:
	virtual ~StaticFrame() { 
		frame = NULL; 
	}

	static T* frame;
	static void openWindow() {
		if(frame == NULL) {
			frame = new T();
			frame->CreateEx(WinUtil::mdiClient, frame->rcDefault, CTSTRING_I(ResourceManager::Strings(title)));
			WinUtil::setButtonPressed(ID, true);
		} else {
			// match the behavior of MainFrame::onSelected()
			HWND hWnd = frame->m_hWnd;
			if(isMDIChildActive(hWnd)) {
				::PostMessage(hWnd, WM_CLOSE, NULL, NULL);
			} else if(frame->MDIGetActive() != hWnd) {
				MainFrame::getMainFrame()->MDIActivate(hWnd);
				WinUtil::setButtonPressed(ID, true);
			} else if(SETTING(TOGGLE_ACTIVE_WINDOW)) {
				::SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
				frame->MDINext(hWnd);
				hWnd = frame->MDIGetActive();
				WinUtil::setButtonPressed(ID, true);
			}
			if(::IsIconic(hWnd))
				::ShowWindow(hWnd, SW_RESTORE);
		}
	}
	static bool isMDIChildActive(HWND hWnd) {
		HWND wnd = MainFrame::getMainFrame()->MDIGetActive();
		dcassert(wnd != NULL);
		return (hWnd == wnd);
	}

	static bool parseWindowParams(StringMap& params) {
		if (params["id"] == T::id) {
			MainFrame::getMainFrame()->callAsync([=] { T::openWindow(); });
			return true;
		}
		return false;
	}

	static bool getWindowParams(HWND hWnd, StringMap& params) {
		if (frame != NULL && hWnd == frame->m_hWnd) {
			params["id"] = T::id;
			return true;
		}
		return false;
	}
};

template<class T, int title, int ID> T* StaticFrame<T, title, ID>::frame = NULL;

struct toolbarButton {
	int id, image;
	int nIcon;
	bool check;
	ResourceManager::Strings tooltip;
};

static const toolbarButton ToolbarButtons[] = {
	
	{ ID_FILE_CONNECT, 0, IDI_PUBLICHUBS, true, ResourceManager::MENU_PUBLIC_HUBS },
	//separator
	{ ID_FILE_RECONNECT, 1, IDI_RECONNECT, false, ResourceManager::MENU_RECONNECT },
	{ IDC_FOLLOW, 2, IDI_FOLLOW, false, ResourceManager::MENU_FOLLOW_REDIRECT },
	//separator
	{ IDC_FAVORITES, 3, IDI_FAVORITEHUBS, true, ResourceManager::MENU_FAVORITE_HUBS },
	{ IDC_FAVUSERS, 4, IDI_USERS, true, ResourceManager::MENU_FAVORITE_USERS },
	{ IDC_RECENTS, 5, IDI_RECENTS, true, ResourceManager::MENU_FILE_RECENT_HUBS },
	//separator
	{ IDC_QUEUE, 6, IDI_QUEUE, true, ResourceManager::MENU_DOWNLOAD_QUEUE },
	{ IDC_UPLOAD_QUEUE, 7, IDI_UPLOAD_QUEUE, true, ResourceManager::UPLOAD_QUEUE },
	{ IDC_FINISHED_UL, 8, IDI_FINISHED_UL, true, ResourceManager::FINISHED_UPLOADS },
	//separator
	{ ID_FILE_SEARCH, 9, IDI_SEARCH, false, ResourceManager::MENU_SEARCH },
	{ IDC_FILE_ADL_SEARCH, 10, IDI_ADLSEARCH, true, ResourceManager::MENU_ADL_SEARCH },
	{ IDC_SEARCH_SPY, 11, IDI_SEARCHSPY, true, ResourceManager::MENU_SEARCH_SPY },
	{ IDC_AUTOSEARCH, 12, IDI_AUTOSEARCH, false, ResourceManager::AUTO_SEARCH },
	//separator
	{ IDC_NOTEPAD, 13, IDI_NOTEPAD, true, ResourceManager::MENU_NOTEPAD },
	{ IDC_SYSTEM_LOG, 14, IDI_LOGS, true, ResourceManager::SYSTEM_LOG },
	//separator
	{ IDC_REFRESH_FILE_LIST, 15, IDI_REFRESH, false, ResourceManager::REFRESH_FILE_LIST },
	{ IDC_SCAN_MISSING, 16, IDI_SCAN, false, ResourceManager::MENU_SCAN_MISSING },
	//separator
	{ IDC_OPEN_FILE_LIST, 17, IDI_OPEN_LIST, false, ResourceManager::MENU_OPEN_FILE_LIST },
	{ IDC_OPEN_DOWNLOADS, 18, IDI_OPEN_DOWNLOADS, false, ResourceManager::MENU_OPEN_DOWNLOADS_DIR },
	//separator
	{ IDC_AWAY, 19, IDI_AWAY, true, ResourceManager::AWAY },
	//separator
	{ ID_FILE_SETTINGS, 20, IDI_SETTINGS, false, ResourceManager::MENU_SETTINGS },
	{ IDC_RSSFRAME, 21, IDI_RSS, true, ResourceManager::RSS_FEEDS },
};


struct winamptoolbarButton {
	int id, image;
	ResourceManager::Strings tooltip;
};


static const winamptoolbarButton WinampToolbarButtons[] = {
	{IDC_WINAMP_START, 0, ResourceManager::WINAMP_START},
	{IDC_WINAMP_SPAM, 1, ResourceManager::WINAMP_SPAM},
	{IDC_WINAMP_BACK, 2, ResourceManager::BACK},
	{IDC_WINAMP_PLAY, 3, ResourceManager::WINAMP_PLAY},
	{IDC_WINAMP_PAUSE, 4, ResourceManager::PAUSE},
	{IDC_WINAMP_NEXT, 5, ResourceManager::NEXT},
	{IDC_WINAMP_STOP, 6, ResourceManager::STOP},
	{IDC_WINAMP_VOL_UP, 7, ResourceManager::WINAMP_VOL_UP},
	{IDC_WINAMP_VOL_HALF, 8, ResourceManager::WINAMP_VOL_HALF},
	{IDC_WINAMP_VOL_DOWN, 9, ResourceManager::WINAMP_VOL_DOWN},
};


class WinUtil {
public:
	static SplashWindow* splash;

	struct TextItem {
		WORD itemID;
		ResourceManager::Strings translatedString;
	};

	static boost::wregex pathReg;
	static boost::wregex chatLinkReg;
	static boost::wregex chatReleaseReg;

	static bool hasPassDlg;
	static HBRUSH bgBrush;
	static COLORREF textColor;
	static COLORREF bgColor;
	static HFONT font;
	static int fontHeight;
	static HFONT boldFont;
	static HFONT systemFont;
	static HFONT tabFont;
	static HFONT OEMFont;
	static HFONT progressFont;
	static HFONT listViewFont;
	static CMenu mainMenu;
	static OMenu grantMenu;
	static int lastSettingPage;
	static HWND mainWnd;
	static HWND mdiClient;
	static FlatTabCtrl* tabCtrl;
	static HHOOK hook;
	static tstring tth;
	static DWORD helpCookie;	
	static bool isAppActive;
	static COLORREF TBprogressTextColor;
	static CHARFORMAT2 m_TextStyleTimestamp;
	static CHARFORMAT2 m_ChatTextGeneral;
	static CHARFORMAT2 m_TextStyleMyNick;
	static CHARFORMAT2 m_ChatTextMyOwn;
	static CHARFORMAT2 m_ChatTextServer;
	static CHARFORMAT2 m_ChatTextSystem;
	static CHARFORMAT2 m_TextStyleBold;
	static CHARFORMAT2 m_TextStyleFavUsers;
	static CHARFORMAT2 m_TextStyleOPs;
	static CHARFORMAT2 m_TextStyleNormUsers;
	static CHARFORMAT2 m_TextStyleURL;
	static CHARFORMAT2 m_TextStyleDupe;
	static CHARFORMAT2 m_TextStyleQueue;
	static CHARFORMAT2 m_ChatTextPrivate;
	static CHARFORMAT2 m_ChatTextLog;
	static bool mutesounds;

	static double getFontFactor();
	static bool updated;
	static TStringPair updateCommand;

	static bool isElevated();
	static void addUpdate(const string& aUpdaterFile, bool aTesting = false) noexcept;
	static bool runPendingUpdate() noexcept;
	static void preInit(); // init required for the wizard
	static void init(HWND hWnd);
	static void uninit();
	static void initColors();
	static void setFonts();
	static void FlashWindow();
	static void search(const tstring& aSearch, bool searchDirectory = false);
	static void SetIcon(HWND hWnd, int aDefault, bool big = false);

	static void searchSite(const WebShortcut* ws, const string& strSearchString, bool getReleaseDir = true);

	static void appendSearchMenu(OMenu& aParent, function<void (const WebShortcut* ws)> f, bool appendTitle = true);
	static void appendSearchMenu(OMenu& aParent, const string& aPath, bool getReleaseDir = true, bool appendTitle = true);

	static void loadReBarSettings(HWND bar);
	static void saveReBarSettings(HWND bar);
	static void playSound(const tstring& sound);

	static bool checkClientPassword();

	static bool MessageBoxConfirm(SettingsManager::BoolSetting i, const tstring& txt);
	static void ShowMessageBox(SettingsManager::BoolSetting i, const tstring& txt);

	static void showMessageBox(const tstring& aText, int icon = MB_ICONINFORMATION);
	static bool showQuestionBox(const tstring& aText, int icon = MB_ICONQUESTION, int defaultButton = MB_DEFBUTTON2);

	struct ConnectFav {
		void operator()(UserPtr aUser, const string& aUrl) const;
	};

	struct GetList {
		void operator()(UserPtr aUser, const string& aUrl) const;
	};

	struct BrowseList {
		void operator()(UserPtr aUser, const string& aUrl) const;
	};

	struct GetBrowseList {
		void operator()(UserPtr aUser, const string& aUrl) const;
	};

	struct MatchQueue {
		void operator()(UserPtr aUser, const string& aUrl) const;
	};

	struct PM {
		void operator()(UserPtr aUser, const string& aUrl) const;
	};

	/*void getList(UserPtr aUser, const string& aUrl);
	void browseList(UserPtr aUser, const string& aUrl);
	void getBrowseList(UserPtr aUser, const string& aUrl);
	void matchQueue(UserPtr aUser, const string& aUrl);
	void pm(UserPtr aUser, const string& aUrl);*/

	static void decodeFont(const tstring& setting, LOGFONT &dest);

	static bool getVersionInfo(OSVERSIONINFOEX& ver);

	static COLORREF getDupeColor(DupeType aType);
	static pair<COLORREF, COLORREF> getDupeColors(DupeType aType);
	static COLORREF blendColors(COLORREF aForeGround, COLORREF aBackGround);

	static void showPopup(tstring szMsg, tstring szTitle, HICON hIcon, bool force = false);
	static void showPopup(tstring szMsg, tstring szTitle, DWORD dwInfoFlags = NIIF_INFO, bool force = false);
	static void drawProgressBar(HDC& drawDC, CRect& rc, COLORREF crl, COLORREF textclr, COLORREF backclr, const tstring& aText, 
		double size, double done, bool odcStyle, bool colorOverride, int depth, int lighten, DWORD tAlign = DT_LEFT);

	static int getTextWidth(const tstring& str, HWND hWnd) {
		HDC dc = ::GetDC(hWnd);
		HFONT hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
		HGDIOBJ old = ::SelectObject(dc, hFont);
		
		SIZE sz = { 0, 0 };
		::GetTextExtentPoint32(dc, str.c_str(), str.length(), &sz);
		::SelectObject(dc, old);
		::ReleaseDC(mainWnd, dc);
		
		return (sz.cx == 0) ? 0 : (sz.cx + 10);
	}

	static int getTextWidth(const tstring& str, HDC dc) {
		SIZE sz = { 0, 0 };
		::GetTextExtentPoint32(dc, str.c_str(), str.length(), &sz);
		return (sz.cx == 0) ? 0 : (sz.cx + 10);
	}

	static int WinUtil::getTextWidth(HWND wnd, HFONT fnt) {
	HDC dc = ::GetDC(wnd);
	HGDIOBJ old = ::SelectObject(dc, fnt);
	TEXTMETRIC tm;
	::GetTextMetrics(dc, &tm);
	::SelectObject(dc, old);
	::ReleaseDC(wnd, dc);
	return tm.tmAveCharWidth;
}
	static int getTextHeight(HWND wnd, HFONT fnt) {
		HDC dc = ::GetDC(wnd);
		HGDIOBJ old = ::SelectObject(dc, fnt);
		int h = getTextHeight(dc);
		::SelectObject(dc, old);
		::ReleaseDC(wnd, dc);
		return h;
	}

	static int getTextHeight(HDC dc) {
		TEXTMETRIC tm;
		::GetTextMetrics(dc, &tm);
		return tm.tmHeight;
	}

	static void setClipboard(const tstring& str);

	static uint32_t percent(int32_t x, uint8_t percent) {
		return x*percent/100;
	}

	static tstring encodeFont(LOGFONT const& aFont);

	static bool browseList(tstring& target, HWND aOwner);
	static bool browseApplication(tstring& target, HWND aOwner);

	// Hash related
	static void copyMagnet(const TTHValue& /*aHash*/, const string& /*aFile*/, int64_t);
	static void searchHash(const TTHValue& aHash, const string& aFileName, int64_t aSize);
	static string makeMagnet(const TTHValue& aHash, const string& aFile, int64_t size);

	// URL related
	static void registerDchubHandler();
	static void registerADChubHandler();
	static void registerADCShubHandler();
	static void registerMagnetHandler();
	static void unRegisterDchubHandler();
	static void unRegisterADChubHandler();
	static void unRegisterADCShubHandler();
	static void unRegisterMagnetHandler();

	static bool parseDBLClick(const tstring& aString);
	static void parseMagnetUri(const tstring& aUrl, const HintedUser& aUser, RichTextBox* ctrlEdit = nullptr);

	static bool urlDcADCRegistered;
	static bool urlMagnetRegistered;
	static int textUnderCursor(POINT p, CEdit& ctrl, tstring& x);
	static int textUnderCursor(POINT p, CRichEditCtrl& ctrl, tstring& x);

	static void openLink(const tstring& url);
	static void openFile(const tstring& file);
	static void openFolder(const tstring& file);
	static bool openFile(const string& aFileName, int64_t aSize, const TTHValue& aTTH, const HintedUser& aUser, bool aIsClientView) noexcept;
	
	static double toBytes(TCHAR* aSize);

	static void appendBundlePrioMenu(OMenu& aParent, const BundleList& aBundles);
	static void appendBundlePauseMenu(OMenu& aParent, const BundleList& aBundles);
	static void appendFilePrioMenu(OMenu& aParent, const QueueItemList& aFiles);
	
	template<typename T1>
	static optional<CPoint> getMenuPosition(CPoint pt, T1& aWindow) {
		if (pt.x == -1 && pt.y == -1) {
			getContextMenuPos(aWindow, pt);
			return pt;
		}

		// did we click on a scrollbar?
		if (WinUtil::isOnScrollbar(aWindow.m_hWnd, pt)) {
			return boost::none;
		}

		// get the current one instead of the pos where we clicked
		// TODO: fix all menus then
		//return GetMessagePos();
		return pt;
	}

	//returns the position where the context menu should be
	//opened if it was invoked from the keyboard.
	//aPt is relative to the screen not the control.
	static void getContextMenuPos(CListViewCtrl& aList, POINT& aPt);
	static void getContextMenuPos(CTreeViewCtrl& aTree, POINT& aPt);
	static void getContextMenuPos(CEdit& aEdit,			POINT& aPt);

	static bool isOnScrollbar(HWND aHWND, POINT& aPt);
	
	static bool getUCParams(HWND parent, const UserCommand& cmd, ParamMap& params) noexcept;

	static tstring getNicks(const CID& cid);
	static tstring getNicks(const HintedUser& user);

	/** @return Pair of hubnames as a string and a bool representing the user's online status */
	static pair<tstring, bool> getHubNames(const CID& cid);
	static pair<tstring, bool> getHubNames(const UserPtr& u) { return getHubNames(u->getCID()); }
	static tstring getHubNames(const HintedUser& user);
	
	static void splitTokens(int* array, const string& tokens, int maxItems = -1) noexcept;
	static void saveHeaderOrder(CListViewCtrl& ctrl, SettingsManager::StrSetting order, 
		SettingsManager::StrSetting widths, int n, int* indexes, int* sizes) noexcept;


	
	static bool isShift() { return (GetKeyState(VK_SHIFT) & 0x8000) > 0; }
	static bool isAlt() { return (GetKeyState(VK_MENU) & 0x8000) > 0; }
	static bool isCtrl() { return (GetKeyState(VK_CONTROL) & 0x8000) > 0; }

	static tstring escapeMenu(tstring str) { 
		string::size_type i = 0;
		while( (i = str.find(_T('&'), i)) != string::npos) {
			str.insert(str.begin()+i, 1, _T('&'));
			i += 2;
		}
		return str;
	}
	template<class T> static HWND hiddenCreateEx(T& p) noexcept {
		HWND active = (HWND)::SendMessage(mdiClient, WM_MDIGETACTIVE, 0, 0);
		::LockWindowUpdate(mdiClient);
		HWND ret = p.CreateEx(mdiClient);
		if(active && ::IsWindow(active))
			::SendMessage(mdiClient, WM_MDIACTIVATE, (WPARAM)active, 0);
		::LockWindowUpdate(0);
		return ret;
	}
	template<class T> static HWND hiddenCreateEx(T* p) noexcept {
		return hiddenCreateEx(*p);
	}
	
	static void translate(HWND page, TextItem* textItems) 
	{
		if (textItems != NULL) {
			for(int i = 0; textItems[i].itemID != 0; i++) {
				::SetDlgItemText(page, textItems[i].itemID,
					Text::toT(ResourceManager::getInstance()->getString(textItems[i].translatedString)).c_str());
			}
		}
	}

	static void ClearPreviewMenu(OMenu &previewMenu);
	static void appendPreviewMenu(OMenu& parent, const string& aTarget);
	static bool shutDown(int action);
	static int getFirstSelectedIndex(CListViewCtrl& list);
	static int setButtonPressed(int nID, bool bPressed = true);

	static void appendLanguageMenu(CComboBoxEx& ctrlLanguage);
	static void appendHistory(CComboBox& ctrlExcluded, SettingsManager::HistoryType aType);
	static string addHistory(CComboBox& ctrlExcluded, SettingsManager::HistoryType aType);

	static void viewLog(const string& path, bool aHistory=false);

	static string getCompileDate() {
		COleDateTime tCompileDate; 
		tCompileDate.ParseDateTime( _T( __DATE__ ), LOCALE_NOUSEROVERRIDE, 1033 );
		return Text::fromT(tCompileDate.Format(_T("%d.%m.%Y")).GetString());
	}

	static time_t fromSystemTime(const SYSTEMTIME* pTime);
	static void toSystemTime(const time_t aTime, SYSTEMTIME* sysTime);
	static void addCue(HWND hwnd, LPCWSTR text, BOOL drawFocus);

	static void removeBundle(QueueToken aBundleToken);

	static HWND findDialog;
	static LRESULT onUserFieldChar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	static LRESULT onAddressFieldChar(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	static bool onConnSpeedChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/);
	static void setUserFieldLimits(HWND hWnd);

	static void getProfileConflicts(HWND aParent, int aProfile, ProfileSettingItem::List& conflicts);

	static void appendSpeedCombo(CComboBox& aCombo, SettingsManager::StrSetting aSetting);
	static void appendDateUnitCombo(CComboBox& aCombo, int aSel = 1);
	static time_t parseDate(CEdit& aDate, CComboBox& aCombo);

	static void appendSizeCombos(CComboBox& aUnitCombo, CComboBox& aModeCombo, int aUnitSel = 2, int aModeSel = 1);
	static int64_t parseSize(CEdit& aSize, CComboBox& aSizeUnit);

	static tstring getEditText(CEdit& edit);

	static void handleTab(HWND aCurFocus, HWND* ctrlHwnds, int hwndCount);
	static void addFileDownload(const string& aTarget, int64_t aSize, const TTHValue& aTTH, const HintedUser& aUser, time_t aDate, Flags::MaskType aFlags = 0, Priority aPrio = Priority::DEFAULT);
	//static void addFileDownloads(BundleFileList& aFiles, const HintedUser& aUser, Flags::MaskType aFlags = 0, bool addBad = true);

	static void connectHub(const string& aUrl);
	static tstring formatFolderContent(const DirectoryContentInfo& aContentInfo);
	static tstring formatFileType(const string& aFileName);
};


#endif // !defined(WIN_UTIL_H)