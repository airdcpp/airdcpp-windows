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

#include <mmsystem.h>
#include <powrprof.h>

#include "WinUtil.h"
#include "PrivateFrame.h"
#include "TextFrame.h"
#include "SearchFrm.h"
#include "LineDlg.h"
#include "MainFrm.h"

#include "../client/ScopedFunctor.h"
#include "../client/Util.h"
#include "../client/Localization.h"
#include "../client/StringTokenizer.h"
#include "../client/ClientManager.h"
#include "../client/TimerManager.h"
#include "../client/FavoriteManager.h"
#include "../client/ResourceManager.h"
#include "../client/QueueManager.h"
#include "../client/UploadManager.h"
#include "../client/LogManager.h"
#include "../client/version.h"
#include "../client/Magnet.h"

#include "boost/algorithm/string/replace.hpp"
#include "boost/algorithm/string/trim.hpp"
#include "boost/format.hpp"

#include "HubFrame.h"
#include "BarShader.h"
#include "ExMessageBox.h"

boost::wregex WinUtil::pathReg;
boost::wregex WinUtil::chatLinkReg;
boost::wregex WinUtil::chatReleaseReg;

PassDlg* WinUtil::passDlg = nullptr;
unique_ptr<SplashWindow> WinUtil::splash;
HBRUSH WinUtil::bgBrush = NULL;
COLORREF WinUtil::textColor = 0;
COLORREF WinUtil::bgColor = 0;
HFONT WinUtil::font = NULL;
int WinUtil::fontHeight = 0;
HFONT WinUtil::boldFont = NULL;
HFONT WinUtil::systemFont = NULL;
HFONT WinUtil::tabFont = NULL;
HFONT WinUtil::OEMFont = NULL;
HFONT WinUtil::progressFont = NULL;
HFONT WinUtil::listViewFont = NULL;
COLORREF WinUtil::TBprogressTextColor = NULL;
CMenu WinUtil::mainMenu;
OMenu WinUtil::grantMenu;
int WinUtil::lastSettingPage = 0;
HWND WinUtil::mainWnd = NULL;
HWND WinUtil::mdiClient = NULL;
FlatTabCtrl* WinUtil::tabCtrl = NULL;
HHOOK WinUtil::hook = NULL;
tstring WinUtil::tth;
bool WinUtil::urlDcADCRegistered = false;
bool WinUtil::urlMagnetRegistered = false;
bool WinUtil::isAppActive = false;
DWORD WinUtil::comCtlVersion = 0;
CHARFORMAT2 WinUtil::m_TextStyleTimestamp;
CHARFORMAT2 WinUtil::m_ChatTextGeneral;
CHARFORMAT2 WinUtil::m_TextStyleMyNick;
CHARFORMAT2 WinUtil::m_ChatTextMyOwn;
CHARFORMAT2 WinUtil::m_ChatTextServer;
CHARFORMAT2 WinUtil::m_ChatTextSystem;
CHARFORMAT2 WinUtil::m_TextStyleBold;
CHARFORMAT2 WinUtil::m_TextStyleFavUsers;
CHARFORMAT2 WinUtil::m_TextStyleOPs;
CHARFORMAT2 WinUtil::m_TextStyleNormUsers;
CHARFORMAT2 WinUtil::m_TextStyleURL;
CHARFORMAT2 WinUtil::m_TextStyleDupe;
CHARFORMAT2 WinUtil::m_TextStyleQueue;
CHARFORMAT2 WinUtil::m_ChatTextPrivate;
CHARFORMAT2 WinUtil::m_ChatTextLog;
HWND WinUtil::findDialog = nullptr;

bool WinUtil::updated;
TStringPair WinUtil::updateCommand;
	
HLSCOLOR RGB2HLS (COLORREF rgb) {
	unsigned char minval = min(GetRValue(rgb), min(GetGValue(rgb), GetBValue(rgb)));
	unsigned char maxval = max(GetRValue(rgb), max(GetGValue(rgb), GetBValue(rgb)));
	float mdiff  = float(maxval) - float(minval);
	float msum   = float(maxval) + float(minval);

	float luminance = msum / 510.0f;
	float saturation = 0.0f;
	float hue = 0.0f; 

	if ( maxval != minval ) { 
		float rnorm = (maxval - GetRValue(rgb) ) / mdiff;      
		float gnorm = (maxval - GetGValue(rgb) ) / mdiff;
		float bnorm = (maxval - GetBValue(rgb) ) / mdiff;   

		saturation = (luminance <= 0.5f) ? (mdiff / msum) : (mdiff / (510.0f - msum));

		if (GetRValue(rgb) == maxval) hue = 60.0f * (6.0f + bnorm - gnorm);
		if (GetGValue(rgb) == maxval) hue = 60.0f * (2.0f + rnorm - bnorm);
		if (GetBValue(rgb) == maxval) hue = 60.0f * (4.0f + gnorm - rnorm);
		if (hue > 360.0f) hue = hue - 360.0f;
	}
	return HLS ((hue*255)/360, luminance*255, saturation*255);
}

static BYTE _ToRGB (float rm1, float rm2, float rh) {
	if      (rh > 360.0f) rh -= 360.0f;
	else if (rh <   0.0f) rh += 360.0f;

	if      (rh <  60.0f) rm1 = rm1 + (rm2 - rm1) * rh / 60.0f;   
	else if (rh < 180.0f) rm1 = rm2;
	else if (rh < 240.0f) rm1 = rm1 + (rm2 - rm1) * (240.0f - rh) / 60.0f;      

	return (BYTE)(rm1 * 255);
}

COLORREF HLS2RGB (HLSCOLOR hls) {
	float hue        = ((int)HLS_H(hls)*360)/255.0f;
	float luminance  = HLS_L(hls)/255.0f;
	float saturation = HLS_S(hls)/255.0f;

	if ( saturation == 0.0f ) {
		return RGB (HLS_L(hls), HLS_L(hls), HLS_L(hls));
	}
	float rm1, rm2;

	if ( luminance <= 0.5f ) rm2 = luminance + luminance * saturation;  
	else                     rm2 = luminance + saturation - luminance * saturation;
	rm1 = 2.0f * luminance - rm2;   
	BYTE red   = _ToRGB (rm1, rm2, hue + 120.0f);   
	BYTE green = _ToRGB (rm1, rm2, hue);
	BYTE blue  = _ToRGB (rm1, rm2, hue - 120.0f);

	return RGB (red, green, blue);
}

COLORREF HLS_TRANSFORM (COLORREF rgb, int percent_L, int percent_S) {
	HLSCOLOR hls = RGB2HLS (rgb);
	BYTE h = HLS_H(hls);
	BYTE l = HLS_L(hls);
	BYTE s = HLS_S(hls);

	if ( percent_L > 0 ) {
		l = BYTE(l + ((255 - l) * percent_L) / 100);
	} else if ( percent_L < 0 )	{
		l = BYTE((l * (100+percent_L)) / 100);
	}
	if ( percent_S > 0 ) {
		s = BYTE(s + ((255 - s) * percent_S) / 100);
	} else if ( percent_S < 0 ) {
		s = BYTE((s * (100+percent_S)) / 100);
	}
	return HLS2RGB (HLS(h, l, s));
}

double WinUtil::getFontFactor() {
	return static_cast<float>(::GetDeviceCaps(::GetDC(reinterpret_cast<HWND>(0)), LOGPIXELSX )) / 96.0;
}

void WinUtil::playSound(const tstring& sound) {
	if(sound == _T("beep")) {
		::MessageBeep(MB_OK);
	} else {
		::PlaySound(sound.c_str(), 0, SND_FILENAME | SND_ASYNC);
	}
}

void WinUtil::PM::operator()(UserPtr aUser, const string& aUrl) const {
	if (aUser)
		PrivateFrame::openWindow(HintedUser(aUser, aUrl), Util::emptyStringT, ClientManager::getInstance()->getClient(aUrl));
}

void WinUtil::MatchQueue::operator()(UserPtr aUser, const string& aUrl) const {
	if(aUser) {
		try {
			QueueManager::getInstance()->addList(HintedUser(aUser, aUrl), QueueItem::FLAG_MATCH_QUEUE);
		} catch(const Exception& e) {
			LogManager::getInstance()->message(e.getError(), LogManager::LOG_ERROR);
		}
	}
}

void WinUtil::GetList::operator()(UserPtr aUser, const string& aUrl) const {
	if(aUser) {
		try {
			QueueManager::getInstance()->addList(HintedUser(aUser, aUrl), QueueItem::FLAG_CLIENT_VIEW);
		} catch(const Exception& e) {
			LogManager::getInstance()->message(e.getError(), LogManager::LOG_ERROR);		
		}
	}
}
void WinUtil::BrowseList::operator()(UserPtr aUser, const string& aUrl) const {
	if(!aUser)
		return;

	try {
		QueueManager::getInstance()->addList(HintedUser(aUser, aUrl), QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_PARTIAL_LIST);
	} catch(const Exception& e) {
		LogManager::getInstance()->message(e.getError(), LogManager::LOG_ERROR);		
	}
}
void WinUtil::GetBrowseList::operator()(UserPtr aUser, const string& aUrl) const {
	if(!aUser)
		return;

	if (aUser->isSet(User::NMDC) || ClientManager::getInstance()->getShareInfo(HintedUser(aUser, aUrl)).second < SETTING(FULL_LIST_DL_LIMIT))
		GetList()(aUser, aUrl);
	else
		BrowseList()(aUser, aUrl);
}

void WinUtil::ConnectFav::operator()(UserPtr aUser, const string& aUrl) const {
	if(aUser) {
		if(!aUrl.empty()) {
			HubFrame::openWindow(Text::toT(aUrl));
		}
	}
}


void UserInfoBase::pm() {
	WinUtil::PM()(getUser(), getHubUrl());
}

void UserInfoBase::matchQueue() {
	WinUtil::MatchQueue()(getUser(), getHubUrl());
}

void UserInfoBase::getList() {
	WinUtil::GetList()(getUser(), getHubUrl());
}
void UserInfoBase::browseList() {
	WinUtil::BrowseList()(getUser(), getHubUrl());
}
void UserInfoBase::getBrowseList() {
	WinUtil::GetBrowseList()(getUser(), getHubUrl());
}

void UserInfoBase::connectFav() {
	WinUtil::ConnectFav()(getUser(), getHubUrl());
}

void UserInfoBase::addFav() {
	if(getUser()) {
		FavoriteManager::getInstance()->addFavoriteUser(HintedUser(getUser(), getHubUrl()));
	}
}
void UserInfoBase::grant() {
	if(getUser()) {
		UploadManager::getInstance()->reserveSlot(HintedUser(getUser(), getHubUrl()), 600);
	}
}

void UserInfoBase::grantTimeless() {
	if(getUser()) {
		UploadManager::getInstance()->reserveSlot(HintedUser(getUser(), getHubUrl()), 0);
	}
}
void UserInfoBase::removeAll() {
	MainFrame::getMainFrame()->addThreadedTask([=] { 
		if(getUser()) {
			QueueManager::getInstance()->removeSource(getUser(), QueueItem::Source::FLAG_REMOVED);
		}
	});
}
void UserInfoBase::grantHour() {
	if(getUser()) {
		UploadManager::getInstance()->reserveSlot(HintedUser(getUser(), getHubUrl()), 3600);
	}
}
void UserInfoBase::grantDay() {
	if(getUser()) {
		UploadManager::getInstance()->reserveSlot(HintedUser(getUser(), getHubUrl()), 24*3600);
	}
}
void UserInfoBase::grantWeek() {
	if(getUser()) {
		UploadManager::getInstance()->reserveSlot(HintedUser(getUser(), getHubUrl()), 7*24*3600);
	}
}
void UserInfoBase::ungrant() {
	if(getUser()) {
		UploadManager::getInstance()->unreserveSlot(getUser());
	}
}
bool UserInfoBase::hasReservedSlot() {
	if(getUser()) {
		return UploadManager::getInstance()->hasReservedSlot(getUser());
	}
	return false;
}

bool WinUtil::getVersionInfo(OSVERSIONINFOEX& ver) {
	memzero(&ver, sizeof(OSVERSIONINFOEX));
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if(!GetVersionEx((OSVERSIONINFO*)&ver)) {
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if(!GetVersionEx((OSVERSIONINFO*)&ver)) {
			return false;
		}
	}
	return true;
}

static LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
	if(code == HC_ACTION) {
		if(wParam == VK_CONTROL && LOWORD(lParam) == 1) {
			if(lParam & 0x80000000) {
				WinUtil::tabCtrl->endSwitch();
			} else {
				WinUtil::tabCtrl->startSwitch();
			}
		}
	}
	return CallNextHookEx(WinUtil::hook, code, wParam, lParam);
}



void WinUtil::preInit() {
	ResourceLoader::loadFlagImages();
	updated = false;
}

void WinUtil::init(HWND hWnd) {
	pathReg.assign(_T("((?<=\\s)(([A-Za-z0-9]:)|(\\\\))(\\\\[^\\\\:]+)(\\\\([^\\s:])([^\\\\:])*)*((\\.[a-z0-9]{2,10})|(\\\\))(?=(\\s|$|:|,)))"));
	chatReleaseReg.assign(Text::toT(AirUtil::getReleaseRegLong(true)));
	chatLinkReg.assign(Text::toT(AirUtil::getLinkUrl()), boost::regex_constants::icase);

	mainWnd = hWnd;

	mainMenu.CreateMenu();

	CMenuHandle file;
	file.CreatePopupMenu();

	file.AppendMenu(MF_STRING, IDC_OPEN_FILE_LIST, CTSTRING(MENU_OPEN_FILE_LIST));
	file.AppendMenu(MF_STRING, IDC_BROWSE_OWN_LIST, CTSTRING(MENU_BROWSE_OWN_LIST));
	if (SETTING(SETTINGS_PROFILE) == SettingsManager::PROFILE_RAR)
		file.AppendMenu(MF_STRING, IDC_OWN_LIST_ADL, CTSTRING(OWN_LIST_ADL));

	file.AppendMenu(MF_STRING, IDC_MATCH_ALL, CTSTRING(MENU_OPEN_MATCH_ALL));
	file.AppendMenu(MF_STRING, IDC_REFRESH_FILE_LIST, CTSTRING(MENU_REFRESH_FILE_LIST));
	file.AppendMenu(MF_STRING, IDC_SCAN_MISSING, CTSTRING(MENU_SCAN_MISSING));
	file.AppendMenu(MF_SEPARATOR);
	file.AppendMenu(MF_STRING, IDC_OPEN_DOWNLOADS, CTSTRING(MENU_OPEN_DOWNLOADS_DIR));
	file.AppendMenu(MF_STRING, IDC_OPEN_LOG_DIR, CTSTRING(OPEN_LOG_DIR));
	file.AppendMenu(MF_SEPARATOR);
	file.AppendMenu(MF_STRING, ID_FILE_QUICK_CONNECT, CTSTRING(MENU_QUICK_CONNECT));
	file.AppendMenu(MF_STRING, IDC_FOLLOW, CTSTRING(MENU_FOLLOW_REDIRECT));
	file.AppendMenu(MF_STRING, ID_FILE_RECONNECT, CTSTRING(MENU_RECONNECT));
	file.AppendMenu(MF_SEPARATOR);
	file.AppendMenu(MF_STRING, ID_WIZARD, CTSTRING(WIZARD));
	file.AppendMenu(MF_STRING, ID_FILE_SETTINGS, CTSTRING(MENU_SETTINGS));
	file.AppendMenu(MF_STRING, ID_GET_TTH, CTSTRING(MENU_TTH));
	file.AppendMenu(MF_STRING, IDC_UPDATE, CTSTRING(UPDATE_CHECK));
	file.AppendMenu(MF_SEPARATOR);
	file.AppendMenu(MF_STRING, ID_APP_EXIT, CTSTRING(MENU_EXIT));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)file, CTSTRING(MENU_FILE));

	CMenuHandle view;
	view.CreatePopupMenu();

	view.AppendMenu(MF_STRING, ID_FILE_CONNECT, CTSTRING(MENU_PUBLIC_HUBS));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, IDC_RECENTS, CTSTRING(MENU_FILE_RECENT_HUBS));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, IDC_FAVORITES, CTSTRING(MENU_FAVORITE_HUBS));
	view.AppendMenu(MF_STRING, IDC_FAVUSERS, CTSTRING(MENU_FAVORITE_USERS));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, ID_FILE_SEARCH, CTSTRING(MENU_SEARCH));
	view.AppendMenu(MF_STRING, IDC_FILE_ADL_SEARCH, CTSTRING(MENU_ADL_SEARCH));
	view.AppendMenu(MF_STRING, IDC_SEARCH_SPY, CTSTRING(MENU_SEARCH_SPY));
	view.AppendMenu(MF_STRING, IDC_AUTOSEARCH, CTSTRING(AUTO_SEARCH));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, IDC_CDMDEBUG_WINDOW, CTSTRING(MENU_CDMDEBUG_MESSAGES));
	view.AppendMenu(MF_STRING, IDC_NOTEPAD, CTSTRING(MENU_NOTEPAD));
	view.AppendMenu(MF_STRING, IDC_SYSTEM_LOG, CTSTRING(SYSTEM_LOG));
	view.AppendMenu(MF_STRING, IDC_HASH_PROGRESS, CTSTRING(MENU_HASH_PROGRESS));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, ID_VIEW_TOOLBAR, CTSTRING(MENU_TOOLBAR));
	view.AppendMenu(MF_STRING, ID_VIEW_STATUS_BAR, CTSTRING(MENU_STATUS_BAR));
	view.AppendMenu(MF_STRING, ID_VIEW_TRANSFER_VIEW, CTSTRING(MENU_TRANSFER_VIEW));
	view.AppendMenu(MF_STRING, ID_TOGGLE_TOOLBAR, CTSTRING(TOGGLE_TOOLBAR));
	view.AppendMenu(MF_STRING, ID_TOGGLE_TBSTATUS, CTSTRING(TOGGLE_TBSTATUS));
	view.AppendMenu(MF_STRING, ID_LOCK_TB, CTSTRING(LOCK_TB));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)view, CTSTRING(MENU_VIEW));

	CMenuHandle transfers;
	transfers.CreatePopupMenu();

	transfers.AppendMenu(MF_STRING, IDC_QUEUE, CTSTRING(MENU_DOWNLOAD_QUEUE));
	transfers.AppendMenu(MF_STRING, IDC_FINISHED, CTSTRING(FINISHED_DOWNLOADS));
	transfers.AppendMenu(MF_SEPARATOR);
	transfers.AppendMenu(MF_STRING, IDC_UPLOAD_QUEUE, CTSTRING(UPLOAD_QUEUE));
	transfers.AppendMenu(MF_STRING, IDC_FINISHED_UL, CTSTRING(FINISHED_UPLOADS));
	transfers.AppendMenu(MF_SEPARATOR);
	transfers.AppendMenu(MF_STRING, IDC_NET_STATS, CTSTRING(MENU_NETWORK_STATISTICS));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)transfers, CTSTRING(MENU_TRANSFERS));

	CMenuHandle window;
	window.CreatePopupMenu();

	window.AppendMenu(MF_STRING, ID_WINDOW_CASCADE, CTSTRING(MENU_CASCADE));
	window.AppendMenu(MF_STRING, ID_WINDOW_TILE_HORZ, CTSTRING(MENU_HORIZONTAL_TILE));
	window.AppendMenu(MF_STRING, ID_WINDOW_TILE_VERT, CTSTRING(MENU_VERTICAL_TILE));
	window.AppendMenu(MF_STRING, ID_WINDOW_ARRANGE, CTSTRING(MENU_ARRANGE));
	window.AppendMenu(MF_STRING, ID_WINDOW_MINIMIZE_ALL, CTSTRING(MENU_MINIMIZE_ALL));
	window.AppendMenu(MF_STRING, ID_WINDOW_RESTORE_ALL, CTSTRING(MENU_RESTORE_ALL));
	window.AppendMenu(MF_SEPARATOR);
	window.AppendMenu(MF_STRING, IDC_CLOSE_DISCONNECTED, CTSTRING(MENU_CLOSE_DISCONNECTED));
	window.AppendMenu(MF_STRING, IDC_CLOSE_ALL_PM, CTSTRING(MENU_CLOSE_ALL_PM));
	window.AppendMenu(MF_STRING, IDC_CLOSE_ALL_OFFLINE_PM, CTSTRING(MENU_CLOSE_ALL_OFFLINE_PM));
	window.AppendMenu(MF_STRING, IDC_CLOSE_ALL_DIR_LIST, CTSTRING(MENU_CLOSE_ALL_DIR_LIST));
	window.AppendMenu(MF_STRING, IDC_CLOSE_ALL_SEARCH_FRAME, CTSTRING(MENU_CLOSE_ALL_SEARCHFRAME));
	window.AppendMenu(MF_STRING, IDC_RECONNECT_DISCONNECTED, CTSTRING(MENU_RECONNECT_DISCONNECTED));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)window, CTSTRING(MENU_WINDOW));

	CMenuHandle help;
	help.CreatePopupMenu();

	help.AppendMenu(MF_STRING, ID_APP_ABOUT, CTSTRING(MENU_ABOUT));
	help.AppendMenu(MF_SEPARATOR);
	help.AppendMenu(MF_STRING, IDC_HELP_HOMEPAGE, CTSTRING(MENU_HOMEPAGE));
	help.AppendMenu(MF_STRING, IDC_HELP_GUIDES, CTSTRING(MENU_GUIDES));
	help.AppendMenu(MF_STRING, IDC_HELP_DISCUSS, CTSTRING(MENU_DISCUSS));
	help.AppendMenu(MF_STRING, IDC_HELP_CUSTOMIZE, CTSTRING(MENU_CUSTOMIZE));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)help, CTSTRING(MENU_HELP));

	LOGFONT lf, lf2;
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
	SettingsManager::getInstance()->setDefault(SettingsManager::TEXT_FONT, Text::fromT(encodeFont(lf)));
	decodeFont(Text::toT(SETTING(TEXT_FONT)), lf);
	::GetObject((HFONT)GetStockObject(ANSI_FIXED_FONT), sizeof(lf2), &lf2);
	
	lf2.lfHeight = lf.lfHeight;
	lf2.lfWeight = lf.lfWeight;
	lf2.lfItalic = lf.lfItalic;

	bgBrush = CreateSolidBrush(SETTING(BACKGROUND_COLOR));
	textColor = SETTING(TEXT_COLOR);
	bgColor = SETTING(BACKGROUND_COLOR);
	font = ::CreateFontIndirect(&lf);
	fontHeight = WinUtil::getTextHeight(mainWnd, font);
	lf.lfWeight = FW_BOLD;
	boldFont = ::CreateFontIndirect(&lf);
	lf.lfHeight *= 5;
	lf.lfHeight /= 6;
	tabFont = ::CreateFontIndirect(&lf);
	systemFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	
	OEMFont = (HFONT)::GetStockObject(OEM_FIXED_FONT);

	LOGFONT lf3;
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(lf3), &lf3);
	decodeFont(Text::toT(SETTING(TB_PROGRESS_FONT)), lf3);
	progressFont = CreateFontIndirect(&lf3);
	TBprogressTextColor = SETTING(TB_PROGRESS_TEXT_COLOR);

	//default to system theme Font like it was before, only load if changed...
	//should we use this as systemFont?
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
	listViewFont = CreateFontIndirect(&(ncm.lfMessageFont)); 

	if(!SETTING(LIST_VIEW_FONT).empty()){
		::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(lf3), &lf3);
		decodeFont(Text::toT(SETTING(LIST_VIEW_FONT)), lf3);
		listViewFont = CreateFontIndirect(&lf3);
	}

	if(SETTING(URL_HANDLER)) {
		registerDchubHandler();
		registerADChubHandler();
		registerADCShubHandler();
		urlDcADCRegistered = true;
	}
	if(SETTING(MAGNET_REGISTER)) {
		registerMagnetHandler();
		urlMagnetRegistered = true; 
	}

	DWORD dwMajor = 0, dwMinor = 0;
	if(SUCCEEDED(ATL::AtlGetCommCtrlVersion(&dwMajor, &dwMinor))) {
		comCtlVersion = MAKELONG(dwMinor, dwMajor);
	}
	
	hook = SetWindowsHookEx(WH_KEYBOARD, &KeyboardProc, NULL, GetCurrentThreadId());
	
	grantMenu.CreatePopupMenu();
	grantMenu.InsertSeparatorFirst(CTSTRING(GRANT_SLOTS_MENU));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CTSTRING(GRANT_EXTRA_SLOT));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_HOUR, CTSTRING(GRANT_EXTRA_SLOT_HOUR));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_DAY, CTSTRING(GRANT_EXTRA_SLOT_DAY));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_WEEK, CTSTRING(GRANT_EXTRA_SLOT_WEEK));
	grantMenu.AppendMenu(MF_SEPARATOR);
	grantMenu.AppendMenu(MF_STRING, IDC_UNGRANTSLOT, CTSTRING(REMOVE_EXTRA_SLOT));

	initColors();
}

void WinUtil::setFonts() {
		
	LOGFONT lf, lf2;
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
	decodeFont(Text::toT(SETTING(TEXT_FONT)), lf);
	::GetObject((HFONT)GetStockObject(ANSI_FIXED_FONT), sizeof(lf2), &lf2);
	
	lf2.lfHeight = lf.lfHeight;
	lf2.lfWeight = lf.lfWeight;
	lf2.lfItalic = lf.lfItalic;

	font = ::CreateFontIndirect(&lf);
	fontHeight = WinUtil::getTextHeight(mainWnd, font);
	lf.lfWeight = FW_BOLD;
	boldFont = ::CreateFontIndirect(&lf);
	lf.lfHeight *= 5;
	lf.lfHeight /= 6;
	tabFont = ::CreateFontIndirect(&lf);
	systemFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	
	OEMFont = (HFONT)::GetStockObject(OEM_FIXED_FONT);

}
void WinUtil::initColors() {
	bgBrush = CreateSolidBrush(SETTING(BACKGROUND_COLOR));
	textColor = SETTING(TEXT_COLOR);
	bgColor = SETTING(BACKGROUND_COLOR);

	CHARFORMAT2 cf;
	memzero(&cf, sizeof(CHARFORMAT2));
	cf.cbSize = sizeof(cf);
	cf.dwReserved = 0;
	cf.dwMask = CFM_BACKCOLOR | CFM_COLOR | CFM_BOLD | CFM_ITALIC;
	cf.dwEffects = 0;
	cf.crBackColor = SETTING(BACKGROUND_COLOR);
	cf.crTextColor = SETTING(TEXT_COLOR);

	m_TextStyleTimestamp = cf;
	m_TextStyleTimestamp.crBackColor = SETTING(TEXT_TIMESTAMP_BACK_COLOR);
	m_TextStyleTimestamp.crTextColor = SETTING(TEXT_TIMESTAMP_FORE_COLOR);
	if(SETTING(TEXT_TIMESTAMP_BOLD))
		m_TextStyleTimestamp.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_TIMESTAMP_ITALIC))
		m_TextStyleTimestamp.dwEffects |= CFE_ITALIC;

	m_ChatTextGeneral = cf;
	m_ChatTextGeneral.crBackColor = SETTING(TEXT_GENERAL_BACK_COLOR);
	m_ChatTextGeneral.crTextColor = SETTING(TEXT_GENERAL_FORE_COLOR);
	if(SETTING(TEXT_GENERAL_BOLD))
		m_ChatTextGeneral.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_GENERAL_ITALIC))
		m_ChatTextGeneral.dwEffects |= CFE_ITALIC;

	m_TextStyleBold = m_ChatTextGeneral;
	m_TextStyleBold.dwEffects = CFE_BOLD;
	
	m_TextStyleMyNick = cf;
	m_TextStyleMyNick.crBackColor = SETTING(TEXT_MYNICK_BACK_COLOR);
	m_TextStyleMyNick.crTextColor = SETTING(TEXT_MYNICK_FORE_COLOR);
	if(SETTING(TEXT_MYNICK_BOLD))
		m_TextStyleMyNick.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_MYNICK_ITALIC))
		m_TextStyleMyNick.dwEffects |= CFE_ITALIC;

	m_ChatTextMyOwn = cf;
	m_ChatTextMyOwn.crBackColor = SETTING(TEXT_MYOWN_BACK_COLOR);
	m_ChatTextMyOwn.crTextColor = SETTING(TEXT_MYOWN_FORE_COLOR);
	if(SETTING(TEXT_MYOWN_BOLD))
		m_ChatTextMyOwn.dwEffects       |= CFE_BOLD;
	if(SETTING(TEXT_MYOWN_ITALIC))
		m_ChatTextMyOwn.dwEffects       |= CFE_ITALIC;

	m_ChatTextPrivate = cf;
	m_ChatTextPrivate.crBackColor = SETTING(TEXT_PRIVATE_BACK_COLOR);
	m_ChatTextPrivate.crTextColor = SETTING(TEXT_PRIVATE_FORE_COLOR);
	if(SETTING(TEXT_PRIVATE_BOLD))
		m_ChatTextPrivate.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_PRIVATE_ITALIC))
		m_ChatTextPrivate.dwEffects |= CFE_ITALIC;

	m_ChatTextSystem = cf;
	m_ChatTextSystem.crBackColor = SETTING(TEXT_SYSTEM_BACK_COLOR);
	m_ChatTextSystem.crTextColor = SETTING(TEXT_SYSTEM_FORE_COLOR);
	if(SETTING(TEXT_SYSTEM_BOLD))
		m_ChatTextSystem.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_SYSTEM_ITALIC))
		m_ChatTextSystem.dwEffects |= CFE_ITALIC;

	m_ChatTextServer = cf;
	m_ChatTextServer.crBackColor = SETTING(TEXT_SERVER_BACK_COLOR);
	m_ChatTextServer.crTextColor = SETTING(TEXT_SERVER_FORE_COLOR);
	if(SETTING(TEXT_SERVER_BOLD))
		m_ChatTextServer.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_SERVER_ITALIC))
		m_ChatTextServer.dwEffects |= CFE_ITALIC;

	m_ChatTextLog = m_ChatTextGeneral;
	m_ChatTextLog.crTextColor = OperaColors::blendColors(SETTING(TEXT_GENERAL_BACK_COLOR), SETTING(TEXT_GENERAL_FORE_COLOR), 0.4);

	m_TextStyleFavUsers = cf;
	m_TextStyleFavUsers.crBackColor = SETTING(TEXT_FAV_BACK_COLOR);
	m_TextStyleFavUsers.crTextColor = SETTING(TEXT_FAV_FORE_COLOR);
	if(SETTING(TEXT_FAV_BOLD))
		m_TextStyleFavUsers.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_FAV_ITALIC))
		m_TextStyleFavUsers.dwEffects |= CFE_ITALIC;

	m_TextStyleOPs = cf;
	m_TextStyleOPs.crBackColor = SETTING(TEXT_OP_BACK_COLOR);
	m_TextStyleOPs.crTextColor = SETTING(TEXT_OP_FORE_COLOR);
	if(SETTING(TEXT_OP_BOLD))
		m_TextStyleOPs.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_OP_ITALIC))
		m_TextStyleOPs.dwEffects |= CFE_ITALIC;

	m_TextStyleNormUsers = cf;
	m_TextStyleNormUsers.crBackColor = SETTING(TEXT_NORM_BACK_COLOR);
	m_TextStyleNormUsers.crTextColor = SETTING(TEXT_NORM_FORE_COLOR);
	if(SETTING(TEXT_NORM_BOLD))
		m_TextStyleNormUsers.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_NORM_ITALIC))
		m_TextStyleNormUsers.dwEffects |= CFE_ITALIC;

	m_TextStyleURL = cf;
	m_TextStyleURL.crBackColor = SETTING(TEXT_URL_BACK_COLOR);
	m_TextStyleURL.crTextColor = SETTING(TEXT_URL_FORE_COLOR);
	m_TextStyleURL.dwEffects = CFE_LINK | CFE_UNDERLINE;
	if(SETTING(TEXT_URL_BOLD))
		m_TextStyleURL.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_URL_ITALIC))
		m_TextStyleURL.dwEffects |= CFE_ITALIC;
	if(SETTING(UNDERLINE_LINKS))
		m_TextStyleURL.dwMask |= CFE_UNDERLINE;

	m_TextStyleDupe = cf;
	m_TextStyleDupe.crBackColor = SETTING(TEXT_DUPE_BACK_COLOR);
	m_TextStyleDupe.crTextColor = SETTING(DUPE_COLOR);
	m_TextStyleDupe.dwEffects = CFE_LINK | CFE_UNDERLINE;
	if(SETTING(TEXT_DUPE_BOLD))
		m_TextStyleDupe.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_DUPE_ITALIC))
		m_TextStyleDupe.dwEffects |= CFE_ITALIC;
	if(SETTING(UNDERLINE_DUPES))
		m_TextStyleDupe.dwMask |= CFE_UNDERLINE;

	m_TextStyleQueue = cf;
	m_TextStyleQueue.crBackColor = SETTING(TEXT_QUEUE_BACK_COLOR);
	m_TextStyleQueue.crTextColor = SETTING(QUEUE_COLOR);
	m_TextStyleQueue.dwEffects = CFE_LINK | CFE_UNDERLINE;
	if(SETTING(TEXT_QUEUE_BOLD))
		m_TextStyleQueue.dwEffects |= CFE_BOLD;
	if(SETTING(TEXT_QUEUE_ITALIC))
		m_TextStyleQueue.dwEffects |= CFE_ITALIC;
	if(SETTING(UNDERLINE_QUEUE))
		m_TextStyleQueue.dwMask |= CFE_UNDERLINE;
}

void WinUtil::uninit() {
	::DeleteObject(font);
	::DeleteObject(boldFont);
	::DeleteObject(bgBrush);
	::DeleteObject(tabFont);

	mainMenu.DestroyMenu();
	grantMenu.DestroyMenu();
	::DeleteObject(OEMFont);
	::DeleteObject(systemFont);

	UnhookWindowsHookEx(hook);	
	auto files = File::findFiles(Util::getOpenPath(), "*");
	for_each(files.begin(), files.end(), &File::deleteFile);
}

void WinUtil::decodeFont(const tstring& setting, LOGFONT &dest) {
	StringTokenizer<tstring> st(setting, _T(','));
	TStringList &sl = st.getTokens();
	
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(dest), &dest);
	tstring face;
	if(sl.size() == 4)
	{
		face = sl[0];
		dest.lfHeight = Util::toInt(Text::fromT(sl[1]));
		dest.lfWeight = Util::toInt(Text::fromT(sl[2]));
		dest.lfItalic = (BYTE)Util::toInt(Text::fromT(sl[3]));
	}
	
	if(!face.empty()) {
		memzero(dest.lfFaceName, LF_FACESIZE);
		_tcscpy(dest.lfFaceName, face.c_str());
	}
}

int CALLBACK WinUtil::browseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lp*/, LPARAM pData) {
	switch(uMsg) {
	case BFFM_INITIALIZED: 
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
		break;
	}
	return 0;
}

bool WinUtil::browseDirectory(tstring& target, HWND owner /* = NULL */) {
	TCHAR buf[UNC_MAX_PATH];
	BROWSEINFO bi;
	LPMALLOC ma;
	
	memzero(&bi, sizeof(bi));
	
	bi.hwndOwner = owner;
	bi.pszDisplayName = buf;
	bi.lpszTitle = CTSTRING(CHOOSE_FOLDER);
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
	bi.lParam = (LPARAM)target.c_str();
	bi.lpfn = &browseCallbackProc;
	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if(pidl != NULL) {
		SHGetPathFromIDList(pidl, buf);
		target = buf;
		
		if(target.size() > 0 && target[target.size()-1] != PATH_SEPARATOR)
			target += PATH_SEPARATOR;
		
		if(SHGetMalloc(&ma) != E_FAIL) {
			ma->Free(pidl);
			ma->Release();
		}
		return true;
	}
	return false;
}
bool WinUtil::MessageBoxConfirm(SettingsManager::BoolSetting i, const tstring& txt){
	UINT ret = IDYES;
	UINT bCheck = SettingsManager::getInstance()->get(i) ? BST_UNCHECKED : BST_CHECKED;
	if(bCheck == BST_UNCHECKED)
		ret = ::MessageBox(WinUtil::mainWnd, txt.c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), CTSTRING(DONT_ASK_AGAIN), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2, bCheck);
		
	SettingsManager::getInstance()->set(i, bCheck != BST_CHECKED);
	return ret == IDYES;
}

void WinUtil::ShowMessageBox(SettingsManager::BoolSetting i, const tstring& txt) {
	if (SettingsManager::getInstance()->get(i)) {
		UINT bCheck = BST_UNCHECKED;
		::MessageBox(WinUtil::mainWnd, txt.c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), CTSTRING(DONT_SHOW_AGAIN), MB_OK | MB_ICONWARNING | MB_DEFBUTTON2, bCheck);
		if (bCheck == BST_CHECKED)
			SettingsManager::getInstance()->set(i, false);
	}
}

bool WinUtil::browseFile(tstring& target, HWND owner /* = NULL */, bool save /* = true */, const tstring& initialDir /* = Util::emptyString */, const TCHAR* types /* = NULL */, const TCHAR* defExt /* = NULL */) {
	TCHAR buf[UNC_MAX_PATH];
	OPENFILENAME ofn = { 0 };       // common dialog box structure
	target = Text::toT(Util::validateFileName(Text::fromT(target)));
	_tcscpy(buf, target.c_str());
	// Initialize OPENFILENAME
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner = owner;
	ofn.lpstrFile = buf;
	ofn.lpstrFilter = types;
	ofn.lpstrDefExt = defExt;
	ofn.nFilterIndex = 1;

	if(!initialDir.empty()) {
		ofn.lpstrInitialDir = initialDir.c_str();
	}
	ofn.nMaxFile = sizeof(buf);
	ofn.Flags = (save ? 0: OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST);
	
	// Display the Open dialog box. 
	if ( (save ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn) ) ==TRUE) {
		target = ofn.lpstrFile;
		return true;
	}
	return false;
}

tstring WinUtil::encodeFont(LOGFONT const& font)
{
	tstring res(font.lfFaceName);
	res += L',';
	res += Util::toStringW(font.lfHeight);
	res += L',';
	res += Util::toStringW(font.lfWeight);
	res += L',';
	res += Util::toStringW(font.lfItalic);
	return res;
}

void WinUtil::setClipboard(const tstring& str) {
	if(!::OpenClipboard(mainWnd)) {
		return;
	}

	EmptyClipboard();

#ifdef UNICODE	
	OSVERSIONINFOEX ver;
	if( WinUtil::getVersionInfo(ver) ) {
		if( ver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ) {
			string tmp = Text::wideToAcp(str);

			HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (tmp.size() + 1) * sizeof(char)); 
			if (hglbCopy == NULL) { 
				CloseClipboard(); 
				return; 
			} 

			// Lock the handle and copy the text to the buffer. 
			char* lptstrCopy = (char*)GlobalLock(hglbCopy); 
			strcpy(lptstrCopy, tmp.c_str());
			GlobalUnlock(hglbCopy);

			SetClipboardData(CF_TEXT, hglbCopy);

			CloseClipboard();

			return;
		}
	}
#endif

	// Allocate a global memory object for the text. 
	HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (str.size() + 1) * sizeof(TCHAR)); 
	if (hglbCopy == NULL) { 
		CloseClipboard(); 
		return; 
	} 

	// Lock the handle and copy the text to the buffer. 
	TCHAR* lptstrCopy = (TCHAR*)GlobalLock(hglbCopy); 
	_tcscpy(lptstrCopy, str.c_str());
	GlobalUnlock(hglbCopy); 

	// Place the handle on the clipboard. 
#ifdef UNICODE
	SetClipboardData(CF_UNICODETEXT, hglbCopy); 
#else
	SetClipboardData(CF_TEXT hglbCopy);
#endif

	CloseClipboard();
}

void WinUtil::splitTokens(int* array, const string& tokens, int maxItems /* = -1 */) noexcept {
	StringTokenizer<string> t(tokens, _T(','));
	StringList& l = t.getTokens();
	if(maxItems == -1)
		maxItems = l.size();
	
	int k = 0;
	for(auto i = l.begin(); i != l.end() && k < maxItems; ++i, ++k) {
		array[k] = Util::toInt(*i);
	}
}

bool WinUtil::getUCParams(HWND parent, const UserCommand& uc, ParamMap& params) noexcept {
	string::size_type i = 0;
	StringMap done;

	while( (i = uc.getCommand().find("%[line:", i)) != string::npos) {
		i += 7;
		string::size_type j = uc.getCommand().find(']', i);
		if(j == string::npos)
			break;

		string name = uc.getCommand().substr(i, j-i);
		if(done.find(name) == done.end()) {
			LineDlg dlg;
			dlg.title = Text::toT(Util::toString(" > ", uc.getDisplayName()));
			dlg.description = Text::toT(name);
			dlg.line = Text::toT(boost::get<string>(params["line:" + name]));

			if(uc.adc()) {
				Util::replace(_T("\\\\"), _T("\\"), dlg.description);
				Util::replace(_T("\\s"), _T(" "), dlg.description);
			}

			if(dlg.DoModal(parent) == IDOK) {
				params["line:" + name] = Text::fromT(dlg.line);
				done[name] = Text::fromT(dlg.line);
			} else {
				return false;
			}
		}
		i = j + 1;
	}
	i = 0;
	while( (i = uc.getCommand().find("%[kickline:", i)) != string::npos) {
		i += 11;
		string::size_type j = uc.getCommand().find(']', i);
		if(j == string::npos)
			break;

		string name = uc.getCommand().substr(i, j-i);
		if(done.find(name) == done.end()) {
			KickDlg dlg;
			dlg.title = Text::toT(Util::toString(" > ", uc.getDisplayName()));
			dlg.description = Text::toT(name);

			if(uc.adc()) {
				Util::replace(_T("\\\\"), _T("\\"), dlg.description);
				Util::replace(_T("\\s"), _T(" "), dlg.description);
			}

			if(dlg.DoModal(parent) == IDOK) {
				params["kickline:" + name] = Text::fromT(dlg.line);
				done[name] = Text::fromT(dlg.line);
			} else {
				return false;
			}
		}
		i = j + 1;
	}
	return true;
}

void WinUtil::bitziLink(const TTHValue& aHash) {
	// to use this free service by bitzi, we must not hammer or request information from bitzi
	// except when the user requests it (a mass lookup isn't acceptable), and (if we ever fetch
	// this data within DC++, we must identify the client/mod in the user agent, so abuse can be 
	// tracked down and the code can be fixed
	openLink(_T("http://bitzi.com/lookup/tree:tiger:") + Text::toT(aHash.toBase32()));
}

void WinUtil::copyMagnet(const TTHValue& aHash, const string& aFile, int64_t aSize) {
	if(!aFile.empty()) {
		setClipboard(Text::toT(makeMagnet(aHash, aFile, aSize)));
 	}
}

string WinUtil::makeMagnet(const TTHValue& aHash, const string& aFile, int64_t size) {
	string ret = "magnet:?xt=urn:tree:tiger:" + aHash.toBase32();
	if(size > 0)
		ret += "&xl=" + Util::toString(size);
	return ret + "&dn=" + Util::encodeURI(aFile);
}

 void WinUtil::searchHash(const TTHValue& aHash, const string& aFileName, int64_t aSize) {
	if (SettingsManager::lanMode)
		SearchFrame::openWindow(Text::toT(aFileName), aSize, SearchManager::SIZE_EXACT, SEARCH_TYPE_ANY);
	else
		SearchFrame::openWindow(Text::toT(aHash.toBase32()), 0, SearchManager::SIZE_DONTCARE, SEARCH_TYPE_TTH);
 }

 void WinUtil::registerDchubHandler() {
	HKEY hk;
	TCHAR Buf[512];
	tstring app = _T("\"") + Text::toT(Util::getAppName()) + _T("\" \"%1\"");
	Buf[0] = 0;

	if(::RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\dchub\\Shell\\Open\\Command"), 0, KEY_WRITE | KEY_READ, &hk) == ERROR_SUCCESS) {
		DWORD bufLen = sizeof(Buf);
		DWORD type;
		::RegQueryValueEx(hk, NULL, 0, &type, (LPBYTE)Buf, &bufLen);
		::RegCloseKey(hk);
	}

	if(stricmp(app.c_str(), Buf) != 0) {
		if (::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\dchub"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL))  {
			LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_DCHUB), LogManager::LOG_ERROR);
			return;
		}
	
		TCHAR* tmp = _T("URL:Direct Connect Protocol");
		::RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)tmp, sizeof(TCHAR) * (_tcslen(tmp) + 1));
		::RegSetValueEx(hk, _T("URL Protocol"), 0, REG_SZ, (LPBYTE)_T(""), sizeof(TCHAR));
		::RegCloseKey(hk);

		::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\dchub\\Shell\\Open\\Command"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		::RegCloseKey(hk);

		::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\dchub\\DefaultIcon"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		app = Text::toT(Util::getAppName());
		::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		::RegCloseKey(hk);
	}
}

 void WinUtil::unRegisterDchubHandler() {
	SHDeleteKey(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\dchub"));
 }

 void WinUtil::registerADChubHandler() {
	 HKEY hk;
	 TCHAR Buf[512];
	 tstring app = _T("\"") + Text::toT(Util::getAppName()) + _T("\" %1");
	 Buf[0] = 0;

	 if(::RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adc\\Shell\\Open\\Command"), 0, KEY_WRITE | KEY_READ, &hk) == ERROR_SUCCESS) {
		 DWORD bufLen = sizeof(Buf);
		 DWORD type;
		 ::RegQueryValueEx(hk, NULL, 0, &type, (LPBYTE)Buf, &bufLen);
		 ::RegCloseKey(hk);
	 }

	 if(stricmp(app.c_str(), Buf) != 0) {
		 if (::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adc"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL))  {
			 LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_ADC), LogManager::LOG_ERROR);
			 return;
		 }

		 TCHAR* tmp = _T("URL:Direct Connect Protocol");
		 ::RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)tmp, sizeof(TCHAR) * (_tcslen(tmp) + 1));
		 ::RegSetValueEx(hk, _T("URL Protocol"), 0, REG_SZ, (LPBYTE)_T(""), sizeof(TCHAR));
		 ::RegCloseKey(hk);

		 ::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adc\\Shell\\Open\\Command"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		 ::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		 ::RegCloseKey(hk);

		 ::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adc\\DefaultIcon"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		 app = Text::toT(Util::getAppName());
		 ::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		 ::RegCloseKey(hk);
	 }
 }

 void WinUtil::unRegisterADChubHandler() {
	SHDeleteKey(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adc"));
 }
void WinUtil::registerADCShubHandler() {
	HKEY hk;
	TCHAR Buf[512];
	tstring app = _T("\"") + Text::toT(Util::getAppName()) + _T("\" %1");
	Buf[0] = 0;
 
	
	
	if(::RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adcs\\Shell\\Open\\Command"), 0, KEY_WRITE | KEY_READ, &hk) == ERROR_SUCCESS)
	{
		DWORD bufLen = sizeof(Buf);
		DWORD type;
		::RegQueryValueEx(hk, NULL, 0, &type, (LPBYTE) Buf, &bufLen);
		::RegCloseKey(hk);
		}
	
	
	
	if(stricmp(app.c_str(), Buf) != 0) {
		if(::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adcs"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL))  {
			 LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_ADC), LogManager::LOG_ERROR);
			return;
			}

			 TCHAR* tmp = _T("URL:Direct Connect Protocol");
		 ::RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)tmp, sizeof(TCHAR) * (_tcslen(tmp) + 1));
		 ::RegSetValueEx(hk, _T("URL Protocol"), 0, REG_SZ, (LPBYTE)_T(""), sizeof(TCHAR));
		 ::RegCloseKey(hk);

		 ::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adcs\\Shell\\Open\\Command"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		 ::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		 ::RegCloseKey(hk);

		 ::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adcs\\DefaultIcon"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		 app = Text::toT(Util::getAppName());
		 ::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE)app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		 ::RegCloseKey(hk);
	    }

}

void WinUtil::unRegisterADCShubHandler() {
SHDeleteKey(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adcs"));
}

void WinUtil::registerMagnetHandler() {
	HKEY hk;
	TCHAR buf[512];
	tstring openCmd;
	tstring appName = Text::toT(Util::getAppName());
	buf[0] = 0;

	// what command is set up to handle magnets right now?
	if(::RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\magnet\\shell\\open\\command"), 0, KEY_READ, &hk) == ERROR_SUCCESS) {
		DWORD bufLen = sizeof(TCHAR) * sizeof(buf);
		::RegQueryValueEx(hk, NULL, NULL, NULL, (LPBYTE)buf, &bufLen);
		::RegCloseKey(hk);
	}
	openCmd = buf;
	buf[0] = 0;

	// (re)register the handler if magnet.exe isn't the default, or if DC++ is handling it
	if(SETTING(MAGNET_REGISTER) && (strnicmp(openCmd, appName, appName.size()) != 0)) {
		SHDeleteKey(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\magnet"));
		if (::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\magnet"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL))  {
			LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_MAGNET), LogManager::LOG_ERROR);
			return;
		}
		::RegSetValueEx(hk, NULL, NULL, REG_SZ, (LPBYTE)CTSTRING(MAGNET_SHELL_DESC), sizeof(TCHAR)*(TSTRING(MAGNET_SHELL_DESC).length()+1));
		::RegSetValueEx(hk, _T("URL Protocol"), NULL, REG_SZ, NULL, NULL);
		::RegCloseKey(hk);
		::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\magnet\\DefaultIcon"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		::RegSetValueEx(hk, NULL, NULL, REG_SZ, (LPBYTE)appName.c_str(), sizeof(TCHAR)*(appName.length()+1));
		::RegCloseKey(hk);
		appName += _T(" %1");
		::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\magnet\\shell\\open\\command"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);
		::RegSetValueEx(hk, NULL, NULL, REG_SZ, (LPBYTE)appName.c_str(), sizeof(TCHAR)*(appName.length()+1));
		::RegCloseKey(hk);
	}
}

void WinUtil::unRegisterMagnetHandler() {
	SHDeleteKey(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\magnet"));
}

void WinUtil::openLink(const tstring& url) {
	parseDBLClick(url);
}

bool WinUtil::parseDBLClick(const tstring& str) {
	auto url = Text::fromT(str);
	string proto, host, port, file, query, fragment;
	Util::decodeUrl(url, proto, host, port, file, query, fragment);

	if(Util::stricmp(proto.c_str(), "adc") == 0 ||
		Util::stricmp(proto.c_str(), "adcs") == 0 ||
		Util::stricmp(proto.c_str(), "dchub") == 0 )
	{
		if(!host.empty()) {
			HubFrame::openWindow(Text::toT(url));
		}

		if(!file.empty()) {
			if(file[0] == '/') {
				// Remove any '/' in from of the file
				file = file.substr(1);
				if(file.empty()) return true;
			}
			try {
				UserPtr user = ClientManager::getInstance()->findLegacyUser(file);
				if(user)
					QueueManager::getInstance()->addList(HintedUser(user, url), QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_PARTIAL_LIST);
				// @todo else report error
			} catch (const Exception&) {
				// ...
			}
		}

		return true;
	} else if(host == "magnet") {
		parseMagnetUri(str, HintedUser());
		return true;
	}

	boost::regex reg;
	reg.assign(AirUtil::getReleaseRegLong(false));
	if(regex_match(url, reg)) {
		WinUtil::searchAny(Text::toT(url));
		return true;
	} else {
		::ShellExecute(NULL, NULL, Text::toT(url).c_str(), NULL, NULL, SW_SHOWNORMAL);
		return true;
	}
}

void WinUtil::SetIcon(HWND hWnd, int aDefault, bool big) {
	int size = big ? ::GetSystemMetrics(SM_CXICON) : ::GetSystemMetrics(SM_CXSMICON);
	HICON hIcon = ResourceLoader::loadIcon(aDefault, size);
	::SendMessage(hWnd, WM_SETICON, big ? ICON_BIG : ICON_SMALL, (LPARAM)hIcon);
}

void WinUtil::parseMagnetUri(const tstring& aUrl, const HintedUser& aUser, RichTextBox* ctrlEdit /*nullptr*/) {
	if (strnicmp(aUrl.c_str(), _T("magnet:?"), 8) == 0) {
		//LogManager::getInstance()->message(STRING(MAGNET_DLG_TITLE) + ": " + Text::fromT(aUrl), LogManager::LOG_INFO);
		Magnet m = Magnet(Text::fromT(aUrl));
		if(!m.hash.empty() && Encoder::isBase32(m.hash.c_str())){
			auto sel = SETTING(MAGNET_ACTION);
			BOOL remember = false;
			if (SETTING(MAGNET_ASK)) {
				CTaskDialog taskdlg;

				tstring msg = CTSTRING_F(MAGNET_INFOTEXT, Text::toT(m.fname) % Util::formatBytesW(m.fsize));
				taskdlg.SetContentText(msg.c_str());
				TASKDIALOG_BUTTON buttons[] =
				{
					{ IDC_MAGNET_OPEN, CTSTRING(DOWNLOAD_OPEN), },
					{ IDC_MAGNET_QUEUE, CTSTRING(SAVE_DEFAULT), },
					{ IDC_MAGNET_SEARCH, CTSTRING(MAGNET_DLG_SEARCH), },
				};
				taskdlg.ModifyFlags(0, TDF_ALLOW_DIALOG_CANCELLATION | TDF_USE_COMMAND_LINKS);
				taskdlg.SetWindowTitle(CTSTRING(MAGNET_DLG_TITLE));
				taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);


				taskdlg.SetButtons(buttons, _countof(buttons));
				//taskdlg.SetExpandedInformationText(_T("More information"));
				taskdlg.SetMainIcon(IDI_MAGNET);
				taskdlg.SetVerificationText(CTSTRING(MAGNET_DLG_REMEMBER));

                taskdlg.DoModal(mainWnd, &sel, 0, &remember);
				if (sel == IDCANCEL) {
					return;
				}

				sel = sel - IDC_MAGNET_SEARCH;
				if (remember) {
					SettingsManager::getInstance()->set(SettingsManager::MAGNET_ASK,  false);
					SettingsManager::getInstance()->set(SettingsManager::MAGNET_ACTION, sel);
				}
			}

			try {
				if (sel == SettingsManager::MAGNET_SEARCH) {
					WinUtil::searchHash(m.getTTH(), m.fname, m.fsize);
				} else if (sel == SettingsManager::MAGNET_DOWNLOAD) {
					if (ctrlEdit) {
						ctrlEdit->handleDownload(SETTING(DOWNLOAD_DIRECTORY), QueueItem::DEFAULT, false, TargetUtil::TARGET_PATH, false);
					} else {
						addFileDownload(SETTING(DOWNLOAD_DIRECTORY) + m.fname, m.fsize, m.getTTH(), aUser, 0);
					}
				} else if (sel == SettingsManager::MAGNET_OPEN) {
					QueueManager::getInstance()->addOpenedItem(m.fname, m.fsize, m.getTTH(), aUser, false);
				}
			} catch(const Exception& e) {
				LogManager::getInstance()->message(e.getError(), LogManager::LOG_ERROR);
			}
		} else {
			MessageBox(mainWnd, CTSTRING(MAGNET_DLG_TEXT_BAD), CTSTRING(MAGNET_DLG_TITLE), MB_OK | MB_ICONEXCLAMATION);
		}
	}
}

int WinUtil::textUnderCursor(POINT p, CEdit& ctrl, tstring& x) {
	
	int i = ctrl.CharFromPos(p);
	int line = ctrl.LineFromChar(i);
	int c = LOWORD(i) - ctrl.LineIndex(line);
	int len = ctrl.LineLength(i) + 1;
	if(len < 3) {
		return 0;
	}

	x.resize(len);
	ctrl.GetLine(line, &x[0], len);

	string::size_type start = x.find_last_of(_T(" <\t\r\n"), c);
	if(start == string::npos)
		start = 0;
	else
		start++;

	return start;
}
int WinUtil::textUnderCursor(POINT p, CRichEditCtrl& ctrl, tstring& x) {
	
	int i = ctrl.CharFromPos(p);
	int line = ctrl.LineFromChar(i);
	int c = i - ctrl.LineIndex(line);
	int len = ctrl.LineLength(i);
	
	if(len < 3) {
		return 0;
	}

	x.resize(len+1);
	x.resize(ctrl.GetLine(line, &x[0], len+1));

	string::size_type start = x.find_last_of(_T(" <\t\r\n"), c);
	if(start == string::npos)
		start = 0;
	else
		start++;

	return start;
}

void WinUtil::saveHeaderOrder(CListViewCtrl& ctrl, SettingsManager::StrSetting order, 
							  SettingsManager::StrSetting widths, int n, 
							  int* indexes, int* sizes) noexcept {
	string tmp;

	ctrl.GetColumnOrderArray(n, indexes);
	int i;
	for(i = 0; i < n; ++i) {
		tmp += Util::toString(indexes[i]);
		tmp += ',';
	}
	tmp.erase(tmp.size()-1, 1);
	SettingsManager::getInstance()->set(order, tmp);
	tmp.clear();
	int nHeaderItemsCount = ctrl.GetHeader().GetItemCount();
	for(i = 0; i < n; ++i) {
		sizes[i] = ctrl.GetColumnWidth(i);
		if (i >= nHeaderItemsCount) // Not exist column
			sizes[i] = 0;
		tmp += Util::toString(sizes[i]);
		tmp += ',';
	}
	tmp.erase(tmp.size()-1, 1);
	SettingsManager::getInstance()->set(widths, tmp);
}

double WinUtil::toBytes(TCHAR* aSize) {
	double bytes = _tstof(aSize);

	if (_tcsstr(aSize, CTSTRING(PiB))) {
		return bytes * 1024.0 * 1024.0 * 1024.0 * 1024.0 * 1024.0;
	} else if (_tcsstr(aSize, CTSTRING(TiB))) {
		return bytes * 1024.0 * 1024.0 * 1024.0 * 1024.0;
	} else if (_tcsstr(aSize, CTSTRING(GiB))) {
		return bytes * 1024.0 * 1024.0 * 1024.0;
	} else if (_tcsstr(aSize, CTSTRING(MiB))) {
		return bytes * 1024.0 * 1024.0;
	} else if (_tcsstr(aSize, CTSTRING(KiB))) {
		return bytes * 1024.0;
	} else {
		return bytes;
	}
}

bool WinUtil::isDesktopOs() {
	OSVERSIONINFOEX ver;
	memzero(&ver, sizeof(OSVERSIONINFOEX));
	if(!GetVersionEx((OSVERSIONINFO*)&ver)) 
	{
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	}
	GetVersionEx((OSVERSIONINFO*)&ver);
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	return ver.wProductType == VER_NT_WORKSTATION;
}

tstring WinUtil::getNicks(const CID& cid) {
	return Text::toT(Util::listToString(ClientManager::getInstance()->getNicks(cid)));
}

tstring WinUtil::getNicks(const HintedUser& user) {
	return Text::toT(ClientManager::getInstance()->getFormatedNicks(user));
}

static pair<tstring, bool> formatHubNames(const StringList& hubs) {
	if(hubs.empty()) {
		return make_pair(CTSTRING(OFFLINE), false);
	} else {
		return make_pair(Text::toT(Util::listToString(hubs)), true);
	}
}

pair<tstring, bool> WinUtil::getHubNames(const CID& cid) {
	return formatHubNames(ClientManager::getInstance()->getHubNames(cid));
}

tstring WinUtil::getHubNames(const HintedUser& aUser) {
	return Text::toT(ClientManager::getInstance()->getFormatedHubNames(aUser));
}

void WinUtil::getContextMenuPos(CListViewCtrl& aList, POINT& aPt) {
	int pos = aList.GetNextItem(-1, LVNI_SELECTED | LVNI_FOCUSED);
	if(pos >= 0) {
		CRect lrc;
		aList.GetItemRect(pos, &lrc, LVIR_LABEL);
		aPt.x = lrc.left;
		aPt.y = lrc.top + (lrc.Height() / 2);
	} else {
		aPt.x = aPt.y = 0;
	}
	aList.ClientToScreen(&aPt);
}

void WinUtil::getContextMenuPos(CTreeViewCtrl& aTree, POINT& aPt) {
	CRect trc;
	HTREEITEM ht = aTree.GetSelectedItem();
	if(ht) {
		aTree.GetItemRect(ht, &trc, TRUE);
		aPt.x = trc.left;
		aPt.y = trc.top + (trc.Height() / 2);
	} else {
		aPt.x = aPt.y = 0;
	}
	aTree.ClientToScreen(&aPt);
}
void WinUtil::getContextMenuPos(CEdit& aEdit, POINT& aPt) {
	CRect erc;
	aEdit.GetRect(&erc);
	aPt.x = erc.Width() / 2;
	aPt.y = erc.Height() / 2;
	aEdit.ClientToScreen(&aPt);
}

void WinUtil::openFolder(const tstring& file) {
	if(file.empty() )
		return;

	MainFrame::getMainFrame()->addThreadedTask([=] {
		if(!Util::fileExists(Text::fromT(file)))
			return;
		::ShellExecute(NULL, Text::toT("explore").c_str(), Text::toT(Util::FormatPath(Util::getFilePath(Text::fromT(file)))).c_str(), NULL, NULL, SW_SHOWNORMAL);
	});
}
void WinUtil::FlashWindow() {
	if( GetForegroundWindow() != WinUtil::mainWnd ) {
		DWORD flashCount;
		SystemParametersInfo(SPI_GETFOREGROUNDFLASHCOUNT, 0, &flashCount, 0);
		FLASHWINFO flash;
		flash.cbSize = sizeof(FLASHWINFO);
		flash.dwFlags = FLASHW_ALL;
		flash.uCount = flashCount;
		flash.hwnd = WinUtil::mainWnd;
		flash.dwTimeout = 0;

		FlashWindowEx(&flash);
	}
}
void WinUtil::ClearPreviewMenu(OMenu &previewMenu){
	while(previewMenu.GetMenuItemCount() > 0) {
		previewMenu.RemoveMenu(0, MF_BYPOSITION);
	}
}

void WinUtil::loadReBarSettings(HWND bar) {
	CReBarCtrl rebar = bar;
	
	REBARBANDINFO rbi = { 0 };
	ZeroMemory(&rbi, sizeof(rbi));
	rbi.cbSize = Util::getOsMajor() >= 6 ? REBARBANDINFOW_V6_SIZE : REBARBANDINFOW_V3_SIZE;
	rbi.fMask = RBBIM_ID | RBBIM_SIZE | RBBIM_STYLE;
	
	StringTokenizer<string> st(SETTING(TOOLBAR_POS), ';');
	StringList &sl = st.getTokens();
	
	for(StringList::const_iterator i = sl.begin(); i != sl.end(); i++)
	{
		StringTokenizer<string> stBar(*i, ',');
		StringList &slBar = stBar.getTokens();
		
		rebar.MoveBand(rebar.IdToIndex(Util::toUInt32(slBar[1])), i - sl.begin());
		rebar.GetBandInfo(i - sl.begin(), &rbi);
		
		rbi.cx = Util::toUInt32(slBar[0]);
		
		if(slBar[2] == "1") {
			rbi.fStyle |= RBBS_BREAK;
		} else {
			rbi.fStyle &= ~RBBS_BREAK;
		}
		
		rebar.SetBandInfo(i - sl.begin(), &rbi);		
	}
}

void WinUtil::saveReBarSettings(HWND bar) {
	string toolbarSettings;
	CReBarCtrl rebar = bar;
	
	REBARBANDINFO rbi = { 0 };
	ZeroMemory(&rbi, sizeof(rbi));
	rbi.cbSize = Util::getOsMajor() >= 6 ? REBARBANDINFOW_V6_SIZE : REBARBANDINFOW_V3_SIZE;
	rbi.fMask = RBBIM_ID | RBBIM_SIZE | RBBIM_STYLE;
	
	for(unsigned int i = 0; i < rebar.GetBandCount(); i++)
	{
		rebar.GetBandInfo(i, &rbi);
		toolbarSettings += Util::toString(rbi.cx) + "," + Util::toString(rbi.wID) + "," + Util::toString((int)(rbi.fStyle & RBBS_BREAK)) + ";";
	}
	
	SettingsManager::getInstance()->set(SettingsManager::TOOLBAR_POS, toolbarSettings);
}


void WinUtil::appendPreviewMenu(OMenu& parent, const string& aTarget) {
	auto previewMenu = parent.createSubMenu(TSTRING(PREVIEW_MENU), true);

	auto lst = FavoriteManager::getInstance()->getPreviewApps();
	auto ext = Util::getFileExt(aTarget);
	if (ext.empty()) return;

	ext = ext.substr(1); //remove the dot

	for(auto& i: lst){
		auto tok = StringTokenizer<string>(i->getExtension(), ';').getTokens();

		if (tok.size() == 0 || any_of(tok.begin(), tok.end(), [&ext](const string& si) { return _stricmp(ext.c_str(), si.c_str())==0; })) {

			string application = i->getApplication();
			string arguments = i->getArguments();

			previewMenu->appendItem(Text::toT((i->getName())).c_str(), [=] {
				string tempTarget = QueueManager::getInstance()->getTempTarget(aTarget);
				ParamMap ucParams;				
	
				ucParams["file"] = "\"" + tempTarget + "\"";
				ucParams["dir"] = "\"" + Util::getFilePath(tempTarget) + "\"";

				::ShellExecute(NULL, NULL, Text::toT(application).c_str(), Text::toT(Util::formatParams(arguments, ucParams)).c_str(), Text::toT(Util::getFilePath(tempTarget)).c_str(), SW_SHOWNORMAL);
			});
		}
	}
}

template<typename T> 
static void appendPrioMenu(OMenu& aParent, const vector<T>& aBase, bool isBundle, function<void (QueueItemBase::Priority aPrio)> prioF, function<void ()> autoPrioF) {
	if (aBase.empty())
		return;

	tstring text;

	if (isBundle) {
		text = aBase.size() == 1 ? TSTRING(SET_BUNDLE_PRIORITY) : TSTRING(SET_BUNDLE_PRIORITIES);
	} else {
		text = aBase.size() == 1 ? TSTRING(SET_FILE_PRIORITY) : TSTRING(SET_FILE_PRIORITIES);
	}

	QueueItemBase::Priority p = QueueItemBase::DEFAULT;
	for (auto& aItem: aBase) {
		if (aItem->getPriority() != p && p != QueueItemBase::DEFAULT) {
			p = QueueItemBase::DEFAULT;
			break;
		}

		p = aItem->getPriority();
	}

	auto priorityMenu = aParent.createSubMenu(text, true);

	int curItem = 0;
	auto appendItem = [=, &curItem](const tstring& aString, QueueItemBase::Priority aPrio) {
		curItem++;
		priorityMenu->appendItem(aString, [=] { 		
			prioF(aPrio);
		}, OMenu::FLAG_THREADED | (p == aPrio ? OMenu::FLAG_CHECKED /*| OMenu::FLAG_DISABLED*/ : 0));
	};

	if (isBundle)
		appendItem(TSTRING(PAUSED_FORCED), QueueItemBase::PAUSED_FORCE);
	appendItem(TSTRING(PAUSED), QueueItemBase::PAUSED);
	appendItem(TSTRING(LOWEST), QueueItemBase::LOWEST);
	appendItem(TSTRING(LOW), QueueItemBase::LOW);
	appendItem(TSTRING(NORMAL), QueueItemBase::NORMAL);
	appendItem(TSTRING(HIGH), QueueItemBase::HIGH);
	appendItem(TSTRING(HIGHEST), QueueItemBase::HIGHEST);

	curItem++;
	//priorityMenu->appendSeparator();

	auto usingAutoPrio = all_of(aBase.begin(), aBase.end(), [](const T& item) { return item->getAutoPriority(); });
	priorityMenu->appendItem(TSTRING(AUTO), autoPrioF, OMenu::FLAG_THREADED | (usingAutoPrio ? OMenu::FLAG_CHECKED : 0));
}
	
void WinUtil::appendBundlePrioMenu(OMenu& aParent, const BundleList& aBundles) {
	auto prioF = [=](QueueItemBase::Priority aPrio) {
		for (auto& b: aBundles)
			QueueManager::getInstance()->setBundlePriority(b->getToken(), aPrio);
	};

	auto autoPrioF = [=] {
		for (auto& b: aBundles)
			QueueManager::getInstance()->setBundleAutoPriority(b->getToken());
	};

	appendPrioMenu<BundlePtr>(aParent, aBundles, true, prioF, autoPrioF);
}

void WinUtil::appendFilePrioMenu(OMenu& aParent, const QueueItemList& aFiles) {
	auto prioF = [=](QueueItemBase::Priority aPrio) {
		for (auto& qi: aFiles)
			QueueManager::getInstance()->setQIPriority(qi->getTarget(), aPrio);
	};

	auto autoPrioF = [=] {
		for (auto& qi: aFiles)
			QueueManager::getInstance()->setQIAutoPriority(qi->getTarget());
	};

	appendPrioMenu<QueueItemPtr>(aParent, aFiles, false, prioF, autoPrioF);
}

bool WinUtil::shutDown(int action) {
	// Prepare for shutdown
	UINT iForceIfHung = 0;
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	if (GetVersionEx(&osvi) != 0 && osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
		iForceIfHung = 0x00000010;
		HANDLE hToken;
		OpenProcessToken(GetCurrentProcess(), (TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY), &hToken);

		LUID luid;
		LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &luid);
		
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		tp.Privileges[0].Luid = luid;
		AdjustTokenPrivileges(hToken, FALSE, &tp, 0, (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL);
		CloseHandle(hToken);
	}

	// Shutdown
	switch(action) {
		case 0: { action = EWX_POWEROFF; break; }
		case 1: { action = EWX_LOGOFF; break; }
		case 2: { action = EWX_REBOOT; break; }
		case 3: { SetSuspendState(false, false, false); return true; }
		case 4: { SetSuspendState(true, false, false); return true; }
		case 5: { 
			typedef bool (CALLBACK* LPLockWorkStation)(void);
			LPLockWorkStation _d_LockWorkStation = (LPLockWorkStation)GetProcAddress(LoadLibrary(_T("user32")), "LockWorkStation");
			_d_LockWorkStation();
			return true;
		}
	}

	if (ExitWindowsEx(action | iForceIfHung, 0) == 0) {
		return false;
	} else {
		return true;
	}
}

int WinUtil::getFirstSelectedIndex(CListViewCtrl& list) {
	for(int i = 0; i < list.GetItemCount(); ++i) {
		if (list.GetItemState(i, LVIS_SELECTED) == LVIS_SELECTED) {
			return i;
		}
	}
	return -1;
}

int WinUtil::setButtonPressed(int nID, bool bPressed /* = true */) {
	if (nID == -1)
		return -1;
	if (!MainFrame::getMainFrame()->getToolBar().IsWindow())
		return -1;

	MainFrame::getMainFrame()->getToolBar().CheckButton(nID, bPressed);
	return 0;
}



void WinUtil::searchAny(const tstring& aSearch) {
	tstring searchTerm = aSearch;
	searchTerm.erase(std::remove(searchTerm.begin(), searchTerm.end(), '\r'), searchTerm.end());
	searchTerm.erase(std::remove(searchTerm.begin(), searchTerm.end(), '\n'), searchTerm.end());
	if(!searchTerm.empty()) {
		SearchFrame::openWindow(searchTerm, 0, SearchManager::SIZE_ATLEAST, SEARCH_TYPE_ANY);
	}
}

void WinUtil::appendSearchMenu(OMenu& aParent, function<void (const WebShortcut* ws)> f, bool appendTitle /*true*/) {
	OMenu* searchMenu = aParent.createSubMenu(TSTRING(SEARCH_SITES), appendTitle);
	for(auto ws: WebShortcuts::getInstance()->list) {
		searchMenu->appendItem(ws->name, [=] { f(ws); });
	}
}

void WinUtil::appendSearchMenu(OMenu& aParent, const string& aPath, bool getReleaseDir /*true*/, bool appendTitle /*true*/) {
	appendSearchMenu(aParent, [=](const WebShortcut* ws) { searchSite(ws, aPath, getReleaseDir); }, appendTitle);
}

void WinUtil::searchSite(const WebShortcut* ws, const string& aSearchTerm, bool getReleaseDir) {
	if(ws == NULL)
		return;

	tstring searchTerm = Text::toT(getReleaseDir ? Util::getReleaseDir(aSearchTerm, true) : Util::getLastDir(aSearchTerm));

	if(ws->clean && !searchTerm.empty()) {
		searchTerm = WinUtil::getTitle(searchTerm);
	}
	
	if(!searchTerm.empty())
		if(ws->url.find(_T("%s")) != tstring::npos){
			WinUtil::openLink(Text::toT(str(boost::format(Text::fromT(ws->url)) %Util::encodeURI(Text::fromT(searchTerm)))));
		} else {
			WinUtil::openLink((ws->url) + Text::toT(Util::encodeURI(Text::fromT(searchTerm))));
		}
	else
		WinUtil::openLink(ws->url);
}

void WinUtil::removeBundle(const string& aBundleToken) {
	BundlePtr aBundle = QueueManager::getInstance()->findBundle(aBundleToken);
	if (aBundle) {
		int finishedFiles = QueueManager::getInstance()->getFinishedItemCount(aBundle);
		bool moveFinished = true;
		if(::MessageBox(0, CTSTRING_F(CONFIRM_REMOVE_DIR_BUNDLE, Text::toT(aBundle->getName())), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) {
			return;
		} else {
			if (finishedFiles > 0) {
				if(::MessageBox(mainWnd, CTSTRING_F(CONFIRM_REMOVE_DIR_FINISHED_BUNDLE, finishedFiles), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) {
					moveFinished = false;
				}
			}
		}

		MainFrame::getMainFrame()->addThreadedTask([=] {
			auto b = aBundle;
			QueueManager::getInstance()->removeBundle(b, false, moveFinished);
		});
	}
}

/* Only returns the text color */
COLORREF WinUtil::getDupeColor(DupeType aType) {
	if (aType == SHARE_DUPE) {
		return SETTING(DUPE_COLOR);
	} else if (aType == FINISHED_DUPE) {
		return blendColors(SETTING(QUEUE_COLOR), SETTING(BACKGROUND_COLOR));
	} else if (aType == QUEUE_DUPE) {
		return SETTING(QUEUE_COLOR);
	} else if(aType == PARTIAL_SHARE_DUPE) {
		return blendColors(SETTING(DUPE_COLOR), SETTING(BACKGROUND_COLOR));
	} else if(aType == PARTIAL_QUEUE_DUPE) {
		return blendColors(SETTING(QUEUE_COLOR), SETTING(BACKGROUND_COLOR));
	} else if(aType == SHARE_QUEUE_DUPE) {
		return blendColors(SETTING(QUEUE_COLOR), SETTING(DUPE_COLOR));
	}

	return SETTING(TEXT_COLOR);
}

/* Text + the background color */
pair<COLORREF, COLORREF> WinUtil::getDupeColors(DupeType aType) {
	if (aType == SHARE_DUPE) {
		return make_pair(SETTING(DUPE_COLOR), SETTING(TEXT_DUPE_BACK_COLOR));
	} else if (aType == FINISHED_DUPE) {
		return make_pair(blendColors(SETTING(QUEUE_COLOR), SETTING(BACKGROUND_COLOR)), SETTING(TEXT_QUEUE_BACK_COLOR));
	} else if (aType == QUEUE_DUPE) {
		return make_pair(SETTING(QUEUE_COLOR), SETTING(TEXT_QUEUE_BACK_COLOR));
	} else if(aType == PARTIAL_SHARE_DUPE) {
		return make_pair(blendColors(SETTING(DUPE_COLOR), SETTING(TEXT_DUPE_BACK_COLOR)), SETTING(TEXT_DUPE_BACK_COLOR));
	} else if(aType == PARTIAL_QUEUE_DUPE) {
		return make_pair(blendColors(SETTING(QUEUE_COLOR), SETTING(TEXT_QUEUE_BACK_COLOR)), SETTING(TEXT_QUEUE_BACK_COLOR));
	} else if(aType == SHARE_QUEUE_DUPE) {
		return make_pair(blendColors(SETTING(QUEUE_COLOR), SETTING(DUPE_COLOR)), SETTING(TEXT_DUPE_BACK_COLOR));
	}

	return make_pair(SETTING(TEXT_COLOR), SETTING(BACKGROUND_COLOR));
}

COLORREF WinUtil::blendColors(COLORREF aForeGround, COLORREF aBackGround) {
	BYTE r, b, g;

	r = static_cast<BYTE>(( static_cast<DWORD>(GetRValue(aForeGround)) + static_cast<DWORD>(GetRValue(aBackGround)) ) / 2);
	g = static_cast<BYTE>(( static_cast<DWORD>(GetGValue(aForeGround)) + static_cast<DWORD>(GetGValue(aBackGround)) ) / 2);
	b = static_cast<BYTE>(( static_cast<DWORD>(GetBValue(aForeGround)) + static_cast<DWORD>(GetBValue(aBackGround)) ) / 2);
					
	return RGB(r, g, b);
}

tstring WinUtil::getTitle(const tstring& searchTerm) {
	tstring ret = Text::toLower(searchTerm);

	//Remove group name
	size_t pos = ret.rfind(_T("-"));
	if (pos != string::npos)
		ret = ret.substr(0, pos);

	//replace . with space
	pos = 0;
	while ((pos = ret.find_first_of(_T("._"), pos)) != string::npos) {
		ret.replace(pos, 1, _T(" "));
	}

	//remove words after year/episode
	boost::wregex reg;
	reg.assign(_T("(((\\[)?((19[0-9]{2})|(20[0-1][0-9]))|(s[0-9]([0-9])?(e|d)[0-9]([0-9])?)|(Season(\\.)[0-9]([0-9])?)).*)"));

	boost::match_results<tstring::const_iterator> result;
	tstring::const_iterator start = ret.begin();
	tstring::const_iterator end = ret.end();

	if (boost::regex_search(start, end, result, reg, boost::match_default)) {
		ret = ret.substr(0, result.position());
	}

	//boost::regex_replace(ret, reg, Util::emptyStringT, boost::match_default | boost::format_sed);

	//remove extra words
	string extrawords[] = {"multisubs","multi","dvdrip","dvdr","real proper","proper","ultimate directors cut","directors cut","dircut","x264","pal","complete","limited","ntsc","bd25",
							"bd50","bdr","bd9","retail","bluray","nordic","720p","1080p","read nfo","dts","hdtv","pdtv","hddvd","repack","internal","custom","subbed","unrated","recut",
							"extended","dts51","finsub","swesub","dksub","nosub","remastered","2disc","rf","fi","swe","stv","r5","festival","anniversary edition","bdrip","ac3", "xvid",
							"ws","int"};
	pos = 0;
	ret += ' ';
	auto arrayLength = sizeof ( extrawords ) / sizeof ( *extrawords );
	while(pos < arrayLength) {
		boost::replace_all(ret, " " + extrawords[pos] + " ", " ");
		pos++;
	}

	//trim spaces from the end
	boost::trim_right(ret);
	return ret;
}

void WinUtil::viewLog(const string& path, bool aHistory /*false*/) {
	if (aHistory) {
		TextFrame::openWindow(Text::toT(path), TextFrame::HISTORY);
	} else if(SETTING(OPEN_LOGS_INTERNAL)) {
		TextFrame::openWindow(Text::toT(path), TextFrame::LOG);
	} else {
		ShellExecute(NULL, NULL, Text::toT(path).c_str(), NULL, NULL, SW_SHOWNORMAL);
	}
}

time_t WinUtil::fromSystemTime(const SYSTEMTIME * pTime) {
	struct tm tm;
	memset(&tm, 0, sizeof(tm));

	tm.tm_year = pTime->wYear - 1900;
	tm.tm_mon = pTime->wMonth - 1;
	tm.tm_mday = pTime->wDay;

	tm.tm_hour = pTime->wHour;
	tm.tm_min = pTime->wMinute;
	tm.tm_sec = pTime->wSecond;
	tm.tm_wday = pTime->wDayOfWeek;

	return mktime(&tm);
}

void WinUtil::toSystemTime(const time_t aTime, SYSTEMTIME* sysTime) {
	tm _tm;
	localtime_s(&_tm, &aTime);

	sysTime->wYear = _tm.tm_year + 1900;
	sysTime->wMonth = _tm.tm_mon + 1;
	sysTime->wDay = _tm.tm_mday;

	sysTime->wHour = _tm.tm_hour;

	sysTime->wMinute = _tm.tm_min;
	sysTime->wSecond = _tm.tm_sec;
	sysTime->wDayOfWeek = _tm.tm_wday;
	sysTime->wMilliseconds = 0;
}

void WinUtil::appendLanguageMenu(CComboBoxEx& ctrlLanguage) {
	ctrlLanguage.SetImageList(ResourceLoader::flagImages);
	int count = 0;
	
	for(auto l: Localization::languageList){
		COMBOBOXEXITEM cbli =  {CBEIF_TEXT|CBEIF_IMAGE|CBEIF_SELECTEDIMAGE};
		CString str = Text::toT(l.languageName).c_str();
		cbli.iItem = count;
		cbli.pszText = (LPTSTR)(LPCTSTR) str;
		cbli.cchTextMax = str.GetLength();

		auto flagIndex = Localization::getFlagIndexByCode(l.countryFlagCode);
		cbli.iImage = flagIndex;
		cbli.iSelectedImage = flagIndex;
		ctrlLanguage.InsertItem(&cbli);

		count = count++;
	}

	ctrlLanguage.SetCurSel(Localization::curLanguage);
}

void WinUtil::appendHistory(CComboBox& ctrlCombo, SettingsManager::HistoryType aType) {
	// Add new items to the history dropdown list
	while (ctrlCombo.GetCount())
		ctrlCombo.DeleteString(0);

	const auto& lastSearches = SettingsManager::getInstance()->getHistory(aType);
	for(const auto& s: lastSearches) {
		ctrlCombo.InsertString(0, s.c_str());
	}
}

string WinUtil::addHistory(CComboBox& ctrlCombo, SettingsManager::HistoryType aType) {
	string ret;
	TCHAR *buf = new TCHAR[ctrlCombo.GetWindowTextLength()+1];
	ctrlCombo.GetWindowText(buf, ctrlCombo.GetWindowTextLength()+1);
	ret = Text::fromT(buf);
	if(!ret.empty() && SettingsManager::getInstance()->addToHistory(buf, aType))
		appendHistory(ctrlCombo, aType);

	delete[] buf;
	return ret;
}

void WinUtil::addCue(HWND hwnd, LPCWSTR text, BOOL drawFocus) {
	if (Util::getOsMajor() == 6)
		Edit_SetCueBannerTextFocused(hwnd, text, drawFocus);
}

void WinUtil::addUpdate(const string& aUpdater) {
	updated = true;
	auto path = Util::getFilePath(Util::getAppName());

	if(path[path.size() - 1] == PATH_SEPARATOR)
		path.insert(path.size() - 1, "\\");

	auto updateCmd = Text::toT("/update \"" +  path + "\"");
	if (isElevated()) {
		updateCmd += _T(" /elevation");
	}

	updateCommand = make_pair(Text::toT(aUpdater), updateCmd);
}

void WinUtil::runPendingUpdate() {
	if(updated && !updateCommand.first.empty()) {
		auto cmd = updateCommand.second + Util::getParams(false);
		ShellExecute(NULL, Util::getOsMajor() >= 6 ? _T("runas") : NULL, updateCommand.first.c_str(), cmd.c_str(), NULL, SW_SHOWNORMAL);
	}
}

void WinUtil::showPopup(tstring szMsg, tstring szTitle, HICON hIcon, bool force) {
	MainFrame::getMainFrame()->ShowPopup(szMsg, szTitle, hIcon, force); 
}

void WinUtil::showPopup(tstring szMsg, tstring szTitle, DWORD dwInfoFlags, bool force) {
	MainFrame::getMainFrame()->ShowPopup(szMsg, szTitle, dwInfoFlags, force); 
}

bool WinUtil::isElevated() {
	BOOL fRet = FALSE;
	HANDLE hToken = NULL;
	if( OpenProcessToken( GetCurrentProcess( ),TOKEN_QUERY,&hToken ) ) {
		TOKEN_ELEVATION Elevation;
		DWORD cbSize = sizeof( TOKEN_ELEVATION );
		if( GetTokenInformation( hToken, TokenElevation, &Elevation, sizeof( Elevation ), &cbSize ) ) {
			fRet = Elevation.TokenIsElevated;
		}
	}
	if( hToken ) {
		CloseHandle( hToken );
	}
	return fRet ? true : false;
}

bool WinUtil::onConnSpeedChanged(WORD wNotifyCode, WORD /*wID*/, HWND hWndCtl) {
	tstring speed;
	speed.resize(1024);
	speed.resize(::GetWindowText(hWndCtl, &speed[0], 1024));
	if (!speed.empty() && wNotifyCode != CBN_SELENDOK) {
		boost::wregex reg;
		if(speed[speed.size() -1] == '.')
			reg.assign(_T("(\\d+\\.)"));
		else
			reg.assign(_T("(\\d+(\\.\\d+)?)"));
		if (!regex_match(speed, reg)) {
			CComboBox tmp;
			tmp.Attach(hWndCtl);
			DWORD dwSel;
			if ((dwSel = tmp.GetEditSel()) != CB_ERR) {
				tstring::iterator it = speed.begin() +  HIWORD(dwSel)-1;
				speed.erase(it);
				tmp.SetEditSel(0,-1);
				tmp.SetWindowText(speed.c_str());
				tmp.SetEditSel(HIWORD(dwSel)-1, HIWORD(dwSel)-1);
				tmp.Detach();
				return false;
			}
		}
	}

	return true;
}

void WinUtil::appendSpeedCombo(CComboBox& aCombo, SettingsManager::StrSetting aSetting) {
	auto curSpeed = SettingsManager::getInstance()->get(aSetting);
	bool found=false;

	//add the speed strings	
	for(const auto& speed: SettingsManager::connectionSpeeds) {
		if (Util::toDouble(curSpeed) < Util::toDouble(speed) && !found) {
			aCombo.AddString(Text::toT(curSpeed).c_str());
			found=true;
		} else if (curSpeed == speed) {
			found=true;
		}
		aCombo.AddString(Text::toT(speed).c_str());
	}

	//set current upload speed setting
	aCombo.SetCurSel(aCombo.FindString(0, Text::toT(curSpeed).c_str()));
}

void WinUtil::setUserFieldLimits(HWND hWnd) {
	CEdit tmp;
	tmp.Attach(GetDlgItem(hWnd, IDC_NICK));
	tmp.LimitText(35);
	tmp.Detach();

	tmp.Attach(GetDlgItem(hWnd, IDC_USERDESC));
	tmp.LimitText(35);
	tmp.Detach();

	tmp.Attach(GetDlgItem(hWnd, IDC_EMAIL));
	tmp.LimitText(35);
	tmp.Detach();
}

LRESULT WinUtil::onUserFieldChar(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/) {
	TCHAR buf[1024];

	::GetWindowText(hWndCtl, buf, 1024);
	tstring old = buf;


	// Strip '$', '|', '<', '>' and ' ' from text
	TCHAR *b = buf, *f = buf, c;
	while( (c = *b++) != 0 )
	{
		if(c != '$' && c != '|' && (wID == IDC_USERDESC || c != ' ') && ( (wID != IDC_NICK && wID != IDC_USERDESC && wID != IDC_EMAIL) || (c != '<' && c != '>') ) )
			*f++ = c;
	}

	*f = '\0';

	if(old != buf)
	{
		// Something changed; update window text without changing cursor pos
		CEdit tmp;
		tmp.Attach(hWndCtl);
		int start, end;
		tmp.GetSel(start, end);
		tmp.SetWindowText(buf);
		if(start > 0) start--;
		if(end > 0) end--;
		tmp.SetSel(start, end);
		tmp.Detach();
	}

	return TRUE;
}

void WinUtil::getProfileConflicts(HWND aParent, int aProfile, SettingItem::List& conflicts) {
	conflicts.clear();

	// a custom set value that differs from the one used by the profile? don't replace those without confirmation
	for (const auto& newSetting: SettingsManager::profileSettings[aProfile]) {
		if (newSetting.isSet() && !newSetting.isProfileCurrent()) {
			conflicts.push_back(newSetting);
		}
	}

	if (!conflicts.empty()) {
		string msg;
		for (const auto& setting: conflicts) {
			msg += STRING_F(SETTING_NAME_X, setting.getName()) + "\r\n";
			msg += STRING_F(CURRENT_VALUE_X, setting.currentToString()) + "\r\n";
			msg += STRING_F(PROFILE_VALUE_X, setting.profileToString()) + "\r\n\r\n";
		}

		CTaskDialog taskdlg;

		tstring tmp1 = TSTRING_F(MANUALLY_CONFIGURED_MSG, conflicts.size() % Text::toT(SettingsManager::getInstance()->getProfileName(aProfile)).c_str());
		taskdlg.SetContentText(tmp1.c_str());

		auto tmp2 = Text::toT(msg);
		taskdlg.SetExpandedInformationText(tmp2.c_str());
		taskdlg.SetExpandedControlText(CTSTRING(SHOW_CONFLICTING));
		TASKDIALOG_BUTTON buttons[] =
		{
			{ IDC_USE_PROFILE, CTSTRING(USE_PROFILE_SETTINGS), },
			{ IDC_USE_OWN, CTSTRING(USE_CURRENT_SETTINGS), },
		};
		taskdlg.ModifyFlags(0, TDF_USE_COMMAND_LINKS | TDF_EXPAND_FOOTER_AREA);
		taskdlg.SetWindowTitle(CTSTRING(MANUALLY_CONFIGURED_DETECTED));
		taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
		taskdlg.SetMainIcon(TD_INFORMATION_ICON);

		taskdlg.SetButtons(buttons, _countof(buttons));

		int sel = 0;
		taskdlg.DoModal(aParent, &sel, 0, 0);
		if (sel == IDC_USE_OWN) {
			conflicts.clear();
		}
	}
}

void WinUtil::addFileDownload(const string& aTarget, int64_t aSize, const TTHValue& aTTH, const HintedUser& aUser, time_t aDate, Flags::MaskType aFlags /*0*/, int8_t prio /*0*/) {
	MainFrame::getMainFrame()->addThreadedTask([=] {
		try {
			QueueManager::getInstance()->createFileBundle(aTarget, aSize, aTTH, aUser, aDate, aFlags, (QueueItemBase::Priority)prio);
		} catch (...) {
			//...
		}
	});
}

bool WinUtil::checkClientPassword() {
	if(passDlg)
		return false;

	passDlg = new PassDlg;
	ScopedFunctor([] { delete passDlg; passDlg = nullptr; });

	passDlg->description = TSTRING(PASSWORD_DESC);
	passDlg->title = TSTRING(PASSWORD_TITLE);
	passDlg->ok = TSTRING(UNLOCK);
	if(passDlg->DoModal(NULL) == IDOK) {
		if (passDlg->line != Text::toT(Util::base64_decode(SETTING(PASSWORD)))) {
			MessageBox(mainWnd, CTSTRING(INVALID_PASSWORD), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK | MB_ICONERROR);
			return false;
		}
	}

	return true;
}

/*void WinUtil::addFileDownloads(BundleFileList& aFiles, const HintedUser& aUser, Flags::MaskType aFlags 0, bool addBad true) {
	MainFrame::getMainFrame()->addThreadedTask([=] {
		for (auto& bfi: aFiles)
			QueueManager::getInstance()->createFileBundle(bfi.file, bfi.size, bfi.tth, aUser, bfi.date, aFlags, addBad, bfi.prio);
	});
}*/