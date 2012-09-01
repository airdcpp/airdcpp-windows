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

#if !defined(WIN_UTIL_H)
#define WIN_UTIL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/Util.h"
#include "../client/SettingsManager.h"
#include "../client/User.h"
#include "../client/MerkleTree.h"
#include "../client/HintedUser.h"
#include "../client/WebShortcuts.h"
#include "../client/TargetUtil.h"
#include "../client/AirUtil.h"

#include <boost/bind.hpp>

#include "resource.h"
#include "OMenu.h"
#include <atlcomtime.h>

class ChatCtrl;

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
	virtual ~StaticFrame() { frame = NULL; }

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
			} else if(BOOLSETTING(TOGGLE_ACTIVE_WINDOW)) {
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
};

template<class T, int title, int ID>
T* StaticFrame<T, title, ID>::frame = NULL;

struct toolbarButton {
	int id, image;
	int nIcon;
	bool check;
	ResourceManager::Strings tooltip;
};

static const toolbarButton ToolbarButtons[] = {
	{ID_FILE_CONNECT, 0, IDI_PUBLICHUBS, true, ResourceManager::MENU_PUBLIC_HUBS},
	{ID_FILE_RECONNECT, 1, IDI_RECONNECT, false, ResourceManager::MENU_RECONNECT},
	{IDC_FOLLOW, 2, IDI_FOLLOW, false, ResourceManager::MENU_FOLLOW_REDIRECT},
	{IDC_FAVORITES, 3,IDI_FAVORITEHUBS, true, ResourceManager::MENU_FAVORITE_HUBS},
	{IDC_FAVUSERS, 4, IDI_FAVORITE_USERS, true, ResourceManager::MENU_FAVORITE_USERS},
	{IDC_RECENTS, 5, IDI_RECENTS, true, ResourceManager::MENU_FILE_RECENT_HUBS},
	{IDC_QUEUE, 6, IDI_QUEUE, true, ResourceManager::MENU_DOWNLOAD_QUEUE},
	{IDC_FINISHED, 7, IDI_FINISHED_DL, true, ResourceManager::FINISHED_DOWNLOADS},
	{IDC_UPLOAD_QUEUE, 8, IDI_UPLOAD_QUEUE, true, ResourceManager::UPLOAD_QUEUE},
	{IDC_FINISHED_UL, 9, IDI_FINISHED_UL, true, ResourceManager::FINISHED_UPLOADS},
	{ID_FILE_SEARCH, 10, IDI_SEARCH,false, ResourceManager::MENU_SEARCH},
	{IDC_FILE_ADL_SEARCH, 11, IDI_ADLSEARCH, true, ResourceManager::MENU_ADL_SEARCH},
	{IDC_SEARCH_SPY, 12, IDI_SEARCHSPY, true, ResourceManager::MENU_SEARCH_SPY},
	{IDC_NET_STATS, 13, IDI_NETSTATS, true, ResourceManager::NETWORK_STATISTICS},
	{IDC_OPEN_FILE_LIST, 14, IDI_OPEN_LIST, false, ResourceManager::MENU_OPEN_FILE_LIST},
	{ID_FILE_SETTINGS, 15, IDI_SETTINGS, false, ResourceManager::MENU_SETTINGS},
	{IDC_NOTEPAD, 16, IDI_NOTEPAD, true, ResourceManager::MENU_NOTEPAD},
	{IDC_AWAY, 17, IDI_AWAY, true, ResourceManager::AWAY},
	{IDC_SHUTDOWN, 18, IDI_SHUTDOWN, true, ResourceManager::SHUTDOWN},
	{IDC_OPEN_DOWNLOADS, 19, IDI_OPEN_DOWNLOADS,false, ResourceManager::MENU_OPEN_DOWNLOADS_DIR},
	{IDC_REFRESH_FILE_LIST, 20, IDI_REFRESH,false, ResourceManager::REFRESH_FILE_LIST},
	{IDC_SYSTEM_LOG, 21, IDI_LOGS, true, ResourceManager::SYSTEM_LOG},
	{IDC_SCAN_MISSING, 22, IDI_SCAN,false, ResourceManager::MENU_SCAN_MISSING},
	{IDC_AUTOSEARCH, 23,  IDI_AUTOSEARCH, false, ResourceManager::AUTOSEARCH},
};


struct winamptoolbarButton {
	int id, image;
	ResourceManager::Strings tooltip;
};


static const winamptoolbarButton WinampToolbarButtons[] = {
	{IDC_WINAMP_START, 0, ResourceManager::WINAMP_START},
	{IDC_WINAMP_SPAM, 1, ResourceManager::WINAMP_SPAM},
	{IDC_WINAMP_BACK, 2, ResourceManager::WINAMP_BACK},
	{IDC_WINAMP_PLAY, 3, ResourceManager::WINAMP_PLAY},
	{IDC_WINAMP_PAUSE, 4, ResourceManager::WINAMP_PAUSE},
	{IDC_WINAMP_NEXT, 5, ResourceManager::WINAMP_NEXT},
	{IDC_WINAMP_STOP, 6, ResourceManager::WINAMP_STOP},
	{IDC_WINAMP_VOL_UP, 7, ResourceManager::WINAMP_VOL_UP},
	{IDC_WINAMP_VOL_HALF, 8, ResourceManager::WINAMP_VOL_HALF},
	{IDC_WINAMP_VOL_DOWN, 9, ResourceManager::WINAMP_VOL_DOWN},
};


class WinUtil {
public:
	static CImageList searchImages;
	static CImageList fileImages;
	static int fileImageCount;
	static CImageList userImages;
	static CImageList flagImages;
	static CImageList settingsTreeImages;

	struct TextItem {
		WORD itemID;
		ResourceManager::Strings translatedString;
	};

	typedef std::map<tstring, int> ImageMap;
	typedef ImageMap::const_iterator ImageIter;
	static ImageMap fileIndexes;
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
	static CMenu mainMenu;
	static OMenu grantMenu;
	static int dirIconIndex;
	static int dirMaskedIndex;
	static HWND mainWnd;
	static HWND mdiClient;
	static FlatTabCtrl* tabCtrl;
	static tstring commands;
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
	static DWORD comCtlVersion;
	static tstring m_IconPath;

	static void preInit(); // init required for the wizard
	static void init(HWND hWnd);
	static void uninit();
	static void initColors();
	static void setFonts();
	static void reLoadImages(); // User Icon Begin / User Icon End
	static void FlashWindow();
	static void searchAny(const tstring& aSearch);
	static void SetIcon(HWND hWnd, int aDefault, bool big = false);
	static tstring getTitle(const tstring& searchTerm);
	static void SearchSite(WebShortcut* ws, tstring strSearchString);
	static void AppendSearchMenu(OMenu& menu, int x = 0);
	static void loadReBarSettings(HWND bar);
	static void saveReBarSettings(HWND bar);

	static tstring getIconPath(const tstring& filename);

	static string getAppName() {
		TCHAR buf[MAX_PATH+1];
		DWORD x = GetModuleFileName(NULL, buf, MAX_PATH);
		return Text::fromT(tstring(buf, x));
	}

	static void decodeFont(const tstring& setting, LOGFONT &dest);

	static bool getVersionInfo(OSVERSIONINFOEX& ver);

	static HICON loadDefaultIcon(int icon) {
		return (HICON)::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(icon));
	}
	static HICON createIcon(int aDefault, int size = 0);

	static tstring getIconName(int aDefault);
	static HBITMAP getBitmapFromIcon(const tstring& aFile, COLORREF crBgColor, long defaultIcon = 0, int xSize = 0, int ySize = 0);
	static COLORREF getDupeColor(DupeType aType);
	static pair<COLORREF, COLORREF> getDupeColors(DupeType aType);
	static COLORREF blendColors(COLORREF aForeGround, COLORREF aBackGround);

	/**
	 * Check if this is a common /-command.
	 * @param cmd The whole text string, will be updated to contain only the command.
	 * @param param Set to any parameters.
	 * @param message Message that should be sent to the chat.
	 * @param status Message that should be shown in the status line.
	 * @return True if the command was processed, false otherwise.
	 */
	static bool checkCommand(tstring& cmd, tstring& param, tstring& message, tstring& status, bool& thirdPerson);

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

	static tstring encodeFont(LOGFONT const& font);
	
	static bool browseFile(tstring& target, HWND owner = NULL, bool save = true, const tstring& initialDir = Util::emptyStringW, const TCHAR* types = NULL, const TCHAR* defExt = NULL);
	static bool browseDirectory(tstring& target, HWND owner = NULL);

	// Hash related
	static void bitziLink(const TTHValue& /*aHash*/);
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

	static bool parseDBLClick(const tstring& aString, HWND hWnd = NULL);
	static void parseMagnetUri(const tstring& /*aUrl*/, bool aOverride = false, const UserPtr& aUser = UserPtr(), const string& hubHint = Util::emptyString);

	static bool urlDcADCRegistered;
	static bool urlMagnetRegistered;
	static int textUnderCursor(POINT p, CEdit& ctrl, tstring& x);
	static int textUnderCursor(POINT p, CRichEditCtrl& ctrl, tstring& x);
	static void openLink(const tstring& url, HWND hWnd = NULL);
	static void openFile(const tstring& file) {
	if(Util::fileExists(Text::fromT(file)))
		::ShellExecute(NULL, NULL, file.c_str(), NULL, NULL, SW_SHOWNORMAL);
	}
	static void openFolder(const tstring& file);

	static int getIconIndex(const tstring& aFileName);

	static int getDirIconIndex() { return dirIconIndex; }
	static int getDirMaskedIndex() { return dirMaskedIndex; }
	
	static double toBytes(TCHAR* aSize);
	
	static int getOsMajor();
	static int getOsMinor();
	
	//returns the position where the context menu should be
	//opened if it was invoked from the keyboard.
	//aPt is relative to the screen not the control.
	static void getContextMenuPos(CListViewCtrl& aList, POINT& aPt);
	static void getContextMenuPos(CTreeViewCtrl& aTree, POINT& aPt);
	static void getContextMenuPos(CEdit& aEdit,			POINT& aPt);
	
	static bool getUCParams(HWND parent, const UserCommand& cmd, ParamMap& params) noexcept;

	static tstring getNicks(const CID& cid, const string& hintUrl);
	static tstring getNicks(const UserPtr& u, const string& hintUrl);
	static tstring getNicks(const CID& cid, const string& hintUrl, bool priv);
	static tstring getNicks(const HintedUser& user) { return getNicks(user.user->getCID(), user.hint); }

	/** @return Pair of hubnames as a string and a bool representing the user's online status */
	static pair<tstring, bool> getHubNames(const CID& cid, const string& hintUrl);
	static pair<tstring, bool> getHubNames(const UserPtr& u, const string& hintUrl);
	static pair<tstring, bool> getHubNames(const CID& cid, const string& hintUrl, bool priv);
	static pair<tstring, bool> getHubNames(const HintedUser& user) { return getHubNames(user.user->getCID(), user.hint); }
	
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
	static int SetupPreviewMenu(CMenu &previewMenu, string extension);
	static void RunPreviewCommand(unsigned int index, const string& target);
	static string formatTime(uint64_t rest);
	static string getSysUptime();
	static tstring diskInfo();
	static string generateStats();
	static string uptimeInfo();
	static const tstring& disableCzChars(tstring& message);
	static bool shutDown(int action);
	static int getFirstSelectedIndex(CListViewCtrl& list);
	static int setButtonPressed(int nID, bool bPressed = true);
	static tstring UselessInfo();
	static tstring Speedinfo();
	static tstring DiskSpaceInfo(bool onlyTotal = false);

	static void appendSearchTypeCombo(CComboBoxEx& ctrlSearchType, const string& aSelection);
	static void loadSettingsTreeIcons();
	static void loadSearchTypeIcons();

	static void appendLanguageMenu(CComboBoxEx& ctrlLanguage);

	static string getReport(const Identity& identity, HWND hwnd);

	static string CPUInfo();
	
	static TStringList FindVolumes();

	static void viewLog(const string& path, bool aHistory=false);

	static string getCompileDate() {
		COleDateTime tCompileDate; 
		tCompileDate.ParseDateTime( _T( __DATE__ ), LOCALE_NOUSEROVERRIDE, 1033 );
		return Text::fromT(tCompileDate.Format(_T("%d.%m.%Y")).GetString());
	}

	static time_t fromSystemTime(const SYSTEMTIME* pTime);
	static void toSystemTime(const time_t aTime, SYSTEMTIME* sysTime);
	static void addCue(HWND hwnd, LPCWSTR text, BOOL drawFocus);

	static HWND findDialog;
private:
	static int CALLBACK browseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lp*/, LPARAM pData);

};


#endif // !defined(WIN_UTIL_H)

/**
 * @file
 * $Id: WinUtil.h 471 2010-01-09 23:17:42Z bigmuscle $
 */
