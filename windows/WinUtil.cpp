/* 
 * Copyright (C) 2001-2021 Jacek Sieka, arnetheduck on gmail point com
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

#include "WinUtil.h"

#include "LineDlg.h"
#include "MainFrm.h"
#include "OMenu.h"
#include "ResourceLoader.h"
#include "SystemUtil.h"

#include "BarShader.h"
#include "ExMessageBox.h"
#include "SplashWindow.h"

#include <airdcpp/AirUtil.h>
#include <airdcpp/File.h>
#include <airdcpp/LogManager.h>
#include <airdcpp/ResourceManager.h>
#include <airdcpp/ScopedFunctor.h>
#include <airdcpp/StringTokenizer.h>
#include <airdcpp/Util.h>

#include <airdcpp/version.h>

#include <boost/format.hpp>

#include <mmsystem.h>


boost::wregex WinUtil::pathReg;
boost::wregex WinUtil::chatLinkReg;
boost::wregex WinUtil::chatReleaseReg;

bool WinUtil::hasPassDlg = false;
SplashWindow* WinUtil::splash = nullptr;
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
int WinUtil::lastSettingPage = 0;
HWND WinUtil::mainWnd = NULL;
HWND WinUtil::mdiClient = NULL;
FlatTabCtrl* WinUtil::tabCtrl = NULL;
HHOOK WinUtil::hook = NULL;
tstring WinUtil::tth;
const string WinUtil::ownerId = "windows_gui";
bool WinUtil::urlDcADCRegistered = false;
bool WinUtil::urlMagnetRegistered = false;
bool WinUtil::isAppActive = false;
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
string WinUtil::paths[WinUtil::PATH_LAST];
	
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
	if (sound == _T("beep")) {
		::MessageBeep(MB_OK);
	}
	else {
		::PlaySound(sound.c_str(), 0, SND_FILENAME | SND_ASYNC);
	}
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
		if (WinUtil::isAlt() && (lParam & 0x80000000) && LOWORD(lParam) == 1) {
			if (wParam == VK_LEFT)
					WinUtil::tabCtrl->SwitchTo(false);
				else if (wParam == VK_RIGHT)
					WinUtil::tabCtrl->SwitchTo(true);
		}
	}
	return CallNextHookEx(WinUtil::hook, code, wParam, lParam);
}



void WinUtil::preInit() {
	ResourceLoader::loadFlagImages();
	updated = false;
}

void WinUtil::init(HWND hWnd) {
	paths[PATH_NOTEPAD] = Util::getPath(Util::PATH_USER_CONFIG) + "Notepad.txt";
	paths[PATH_EMOPACKS] = Util::getPath(Util::PATH_RESOURCES) + "EmoPacks" PATH_SEPARATOR_STR;
	paths[PATH_THEMES] = Util::getPath(Util::PATH_RESOURCES) + "Themes" PATH_SEPARATOR_STR;

	File::ensureDirectory(paths[PATH_THEMES]);

	pathReg.assign(_T("((?<=\\s)(([A-Za-z0-9]:)|(\\\\))(\\\\[^\\\\:]+)(\\\\([^\\s:])([^\\\\:])*)*((\\.[a-z0-9]{2,10})|(\\\\))(?=(\\s|$|:|,)))"));
	chatReleaseReg.assign(Text::toT(AirUtil::getReleaseRegLong(true)));
	chatLinkReg.assign(Text::toT(AirUtil::getUrlReg()), boost::regex_constants::icase);

	mainWnd = hWnd;

	if (SETTING(URL_HANDLER)) {
		registerDchubHandler();
		registerADChubHandler();
		registerADCShubHandler();
		urlDcADCRegistered = true;
	}

	if (SETTING(MAGNET_REGISTER)) {
		registerMagnetHandler();
		urlMagnetRegistered = true;
	}

	hook = SetWindowsHookEx(WH_KEYBOARD, &KeyboardProc, NULL, GetCurrentThreadId());

	setFonts();
	initColors();
	initMenus();
}


void WinUtil::initMenus() {
	// Main menu
	mainMenu.CreateMenu();

	CMenuHandle file;
	file.CreatePopupMenu();

	file.AppendMenu(MF_STRING, IDC_OPEN_FILE_LIST, CTSTRING(MENU_OPEN_FILE_LIST));
	file.AppendMenu(MF_STRING, IDC_BROWSE_OWN_LIST, CTSTRING(MENU_BROWSE_OWN_LIST));

	file.AppendMenu(MF_STRING, IDC_MATCH_ALL, CTSTRING(MENU_OPEN_MATCH_ALL));
	file.AppendMenu(MF_STRING, IDC_REFRESH_FILE_LIST, CTSTRING(MENU_REFRESH_FILE_LIST));
	file.AppendMenu(MF_SEPARATOR);
	file.AppendMenu(MF_STRING, IDC_OPEN_DOWNLOADS, CTSTRING(MENU_OPEN_DOWNLOADS_DIR));
	file.AppendMenu(MF_STRING, IDC_OPEN_LOG_DIR, CTSTRING(OPEN_LOG_DIR));
	file.AppendMenu(MF_STRING, IDC_OPEN_CONFIG_DIR, CTSTRING(OPEN_SETTINGS_DIR));
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
	view.AppendMenu(MF_STRING, IDC_RECENTS, CTSTRING(RECENTS));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, IDC_FAVORITES, CTSTRING(MENU_FAVORITE_HUBS));
	view.AppendMenu(MF_STRING, IDC_FAVUSERS, CTSTRING(MENU_FAVORITE_USERS));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, ID_FILE_SEARCH, CTSTRING(MENU_SEARCH));
	view.AppendMenu(MF_STRING, IDC_FILE_ADL_SEARCH, CTSTRING(MENU_ADL_SEARCH));
	view.AppendMenu(MF_STRING, IDC_SEARCH_SPY, CTSTRING(MENU_SEARCH_SPY));
	view.AppendMenu(MF_STRING, IDC_AUTOSEARCH, CTSTRING(AUTO_SEARCH));
	view.AppendMenu(MF_STRING, IDC_EXTENSIONS, CTSTRING(EXTENSIONS));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, IDC_CDMDEBUG_WINDOW, CTSTRING(MENU_CDMDEBUG_MESSAGES));
	view.AppendMenu(MF_STRING, IDC_NOTEPAD, CTSTRING(MENU_NOTEPAD));
	view.AppendMenu(MF_STRING, IDC_SYSTEM_LOG, CTSTRING(SYSTEM_LOG));
	view.AppendMenu(MF_STRING, IDC_RSSFRAME, CTSTRING(RSS_FEEDS));
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
	transfers.AppendMenu(MF_SEPARATOR);
	transfers.AppendMenu(MF_STRING, IDC_UPLOAD_QUEUE, CTSTRING(UPLOAD_QUEUE));
	transfers.AppendMenu(MF_STRING, IDC_FINISHED_UL, CTSTRING(FINISHED_UPLOADS));

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
	window.AppendMenu(MF_STRING, IDC_WINDOW_MARK_READ, CTSTRING(MENU_MARK_AS_READ));

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
}

void WinUtil::setFonts() {
	// System fonts
	systemFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	OEMFont = (HFONT)::GetStockObject(OEM_FIXED_FONT);
	
	// Default app font with variants
	{
		LOGFONT lf;
		::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
		SettingsManager::getInstance()->setDefault(SettingsManager::TEXT_FONT, Text::fromT(encodeFont(lf)));
		decodeFont(Text::toT(SETTING(TEXT_FONT)), lf);

		font = ::CreateFontIndirect(&lf);
		fontHeight = WinUtil::getTextHeight(mainWnd, font);
		lf.lfWeight = FW_BOLD;
		boldFont = ::CreateFontIndirect(&lf);
		lf.lfHeight *= 5;
		lf.lfHeight /= 6;
		tabFont = ::CreateFontIndirect(&lf);
	}

	// Toolbar font
	{
		LOGFONT lf3;
		::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(lf3), &lf3);
		decodeFont(Text::toT(SETTING(TB_PROGRESS_FONT)), lf3);
		progressFont = CreateFontIndirect(&lf3);
		TBprogressTextColor = SETTING(TB_PROGRESS_TEXT_COLOR);
	}

	// List view font
	{
		//default to system theme Font like it was before, only load if changed...
		//should we use this as systemFont?
		NONCLIENTMETRICS ncm;
		ncm.cbSize = sizeof(ncm);
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
		listViewFont = CreateFontIndirect(&(ncm.lfMessageFont));
	}

	if (!SETTING(LIST_VIEW_FONT).empty()) {
		LOGFONT lf3;
		::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(lf3), &lf3);
		decodeFont(Text::toT(SETTING(LIST_VIEW_FONT)), lf3);
		listViewFont = CreateFontIndirect(&lf3);
	}

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
	// Menus
	mainMenu.DestroyMenu();

	// Fonts
	::DeleteObject(font);
	::DeleteObject(boldFont);
	::DeleteObject(bgBrush);
	::DeleteObject(tabFont);

	::DeleteObject(OEMFont);
	::DeleteObject(systemFont);
	::DeleteObject(progressFont);
	::DeleteObject(listViewFont);

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

bool WinUtil::MessageBoxConfirm(SettingsManager::BoolSetting i, const tstring& txt){
	UINT ret = IDYES;
	UINT bCheck = SettingsManager::getInstance()->get(i) ? BST_UNCHECKED : BST_CHECKED;
	if(bCheck == BST_UNCHECKED)
		ret = ::MessageBox(WinUtil::mainWnd, txt.c_str(), Text::toT(shortVersionString).c_str(), CTSTRING(DONT_ASK_AGAIN), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2, bCheck);
		
	SettingsManager::getInstance()->set(i, bCheck != BST_CHECKED);
	return ret == IDYES;
}

void WinUtil::ShowMessageBox(SettingsManager::BoolSetting i, const tstring& txt) {
	if (SettingsManager::getInstance()->get(i)) {
		UINT bCheck = BST_UNCHECKED;
		::MessageBox(WinUtil::mainWnd, txt.c_str(), Text::toT(shortVersionString).c_str(), CTSTRING(DONT_SHOW_AGAIN), MB_OK | MB_ICONWARNING | MB_DEFBUTTON2, bCheck);
		if (bCheck == BST_CHECKED)
			SettingsManager::getInstance()->set(i, false);
	}
}

void WinUtil::showMessageBox(const tstring& aText, int icon) {
	::MessageBox(WinUtil::splash ? WinUtil::splash->m_hWnd : WinUtil::mainWnd, aText.c_str(), Text::toT(shortVersionString).c_str(), icon | MB_OK);
}

bool WinUtil::showQuestionBox(const tstring& aText, int icon, int defaultButton /*= MB_DEFBUTTON2*/) {
	return ::MessageBox(WinUtil::splash ? WinUtil::splash->m_hWnd : WinUtil::mainWnd, aText.c_str(), Text::toT(shortVersionString).c_str(), MB_YESNO | icon | defaultButton) == IDYES;
}

tstring WinUtil::encodeFont(LOGFONT const& aFont)
{
	tstring res(aFont.lfFaceName);
	res += L',';
	res += Util::toStringW((int64_t)aFont.lfHeight);
	res += L',';
	res += Util::toStringW((int64_t)aFont.lfWeight);
	res += L',';
	res += Util::toStringW(aFont.lfItalic);
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

 void WinUtil::registerDchubHandler() {
	HKEY hk;
	TCHAR Buf[512];
	tstring app = _T("\"") + Text::toT(Util::getAppPath()) + _T("\" \"%1\"");
	Buf[0] = 0;

	if(::RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\dchub\\Shell\\Open\\Command"), 0, KEY_WRITE | KEY_READ, &hk) == ERROR_SUCCESS) {
		DWORD bufLen = sizeof(Buf);
		DWORD type;
		::RegQueryValueEx(hk, NULL, 0, &type, (LPBYTE)Buf, &bufLen);
		::RegCloseKey(hk);
	}

	if(stricmp(app.c_str(), Buf) != 0) {
		if (::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\dchub"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL))  {
			LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_DCHUB), LogMessage::SEV_ERROR, STRING(APPLICATION));
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
		app = Text::toT(Util::getAppPath());
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
	 tstring app = _T("\"") + Text::toT(Util::getAppPath()) + _T("\" %1");
	 Buf[0] = 0;

	 if(::RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adc\\Shell\\Open\\Command"), 0, KEY_WRITE | KEY_READ, &hk) == ERROR_SUCCESS) {
		 DWORD bufLen = sizeof(Buf);
		 DWORD type;
		 ::RegQueryValueEx(hk, NULL, 0, &type, (LPBYTE)Buf, &bufLen);
		 ::RegCloseKey(hk);
	 }

	 if(stricmp(app.c_str(), Buf) != 0) {
		 if (::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adc"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL))  {
			 LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_ADC), LogMessage::SEV_ERROR, STRING(APPLICATION));
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
		 app = Text::toT(Util::getAppPath());
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
	tstring app = _T("\"") + Text::toT(Util::getAppPath()) + _T("\" %1");
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
			 LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_ADC), LogMessage::SEV_ERROR, STRING(APPLICATION));
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
		 app = Text::toT(Util::getAppPath());
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
	tstring appName = Text::toT(Util::getAppPath());
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
			LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_MAGNET), LogMessage::SEV_ERROR, STRING(APPLICATION));
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

void WinUtil::SetIcon(HWND hWnd, int aDefault, bool big) {
	int size = big ? ::GetSystemMetrics(SM_CXICON) : ::GetSystemMetrics(SM_CXSMICON);
	::SendMessage(hWnd, WM_SETICON, big ? ICON_BIG : ICON_SMALL, (LPARAM)GET_ICON(aDefault, size));
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
	int c = LOWORD(i) - ctrl.LineIndex(line);
	int len = ctrl.LineLength(i);
	
	if(len < 3) {
		return 0;
	}

	x.resize(len);
	x.resize(ctrl.GetLine(line, &x[0], len));

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

bool WinUtil::isOnScrollbar(HWND m_hWnd, POINT& pt) {
	SCROLLBARINFO sbi;
	memset(&sbi, 0, sizeof(SCROLLBARINFO));
	sbi.cbSize = sizeof(SCROLLBARINFO);

	GetScrollBarInfo(m_hWnd, OBJID_VSCROLL, &sbi);
	if (!(sbi.rgstate[0] & STATE_SYSTEM_INVISIBLE) && sbi.rcScrollBar.left <= pt.x && sbi.rcScrollBar.right >= pt.x) {
		//let the system handle those
		return true;
	}

	GetScrollBarInfo(m_hWnd, OBJID_HSCROLL, &sbi);
	if (!(sbi.rgstate[0] & STATE_SYSTEM_INVISIBLE) && sbi.rcScrollBar.top <= pt.y && sbi.rcScrollBar.bottom >= pt.y) {
		//let the system handle those
		return true;
	}

	return false;
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

void WinUtil::loadReBarSettings(HWND bar) {
	CReBarCtrl rebar = bar;
	
	REBARBANDINFO rbi = { 0 };
	ZeroMemory(&rbi, sizeof(rbi));
	rbi.cbSize = REBARBANDINFOW_V6_SIZE;
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
	rbi.cbSize = REBARBANDINFOW_V6_SIZE;
	rbi.fMask = RBBIM_ID | RBBIM_SIZE | RBBIM_STYLE;
	
	for(unsigned int i = 0; i < rebar.GetBandCount(); i++)
	{
		rebar.GetBandInfo(i, &rbi);
		toolbarSettings += Util::toString(rbi.cx) + "," + Util::toString(rbi.wID) + "," + Util::toString((int)(rbi.fStyle & RBBS_BREAK)) + ";";
	}
	
	SettingsManager::getInstance()->set(SettingsManager::TOOLBAR_POS, toolbarSettings);
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

tstring WinUtil::escapeMenu(tstring str) {
	string::size_type i = 0;
	while ((i = str.find(_T('&'), i)) != string::npos) {
		str.insert(str.begin() + i, 1, _T('&'));
		i += 2;
	}
	return str;
}

void WinUtil::translate(HWND page, TextItem* textItems) {
	if (textItems != NULL) {
		for (int i = 0; textItems[i].itemID != 0; i++) {
			::SetDlgItemText(page, textItems[i].itemID,
				Text::toT(ResourceManager::getInstance()->getString(textItems[i].translatedString)).c_str());
		}
	}
}

int WinUtil::getStatusTextWidth(const tstring& aStr, HWND hWnd) {
	auto width = getTextWidth(aStr, hWnd);
	return (width == 0) ? 0 : (width + 10); // Status bar adds an extra margin
}

int WinUtil::getTextWidth(const tstring& aStr, HWND hWnd) {
	HDC dc = ::GetDC(hWnd);
	HFONT hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
	HGDIOBJ old = ::SelectObject(dc, hFont);

	SIZE sz = { 0, 0 };
	::GetTextExtentPoint32(dc, aStr.c_str(), aStr.length(), &sz);
	::SelectObject(dc, old);
	::ReleaseDC(mainWnd, dc);

	return sz.cx;
}

int WinUtil::getTextWidth(const tstring& aStr, HDC dc) {
	SIZE sz = { 0, 0 };
	::GetTextExtentPoint32(dc, aStr.c_str(), aStr.length(), &sz);
	return sz.cx;
}

int WinUtil::getTextWidth(HWND wnd, HFONT fnt) {
	HDC dc = ::GetDC(wnd);
	HGDIOBJ old = ::SelectObject(dc, fnt);
	TEXTMETRIC tm;
	::GetTextMetrics(dc, &tm);
	::SelectObject(dc, old);
	::ReleaseDC(wnd, dc);
	return tm.tmAveCharWidth;
}

int WinUtil::getTextHeight(HWND wnd, HFONT fnt) {
	HDC dc = ::GetDC(wnd);
	HGDIOBJ old = ::SelectObject(dc, fnt);
	int h = getTextHeight(dc);
	::SelectObject(dc, old);
	::ReleaseDC(wnd, dc);
	return h;
}

int WinUtil::getTextHeight(HDC dc) {
	TEXTMETRIC tm;
	::GetTextMetrics(dc, &tm);
	return tm.tmHeight;
}

void WinUtil::drawProgressBar(HDC& drawDC, CRect& rc, COLORREF clr, COLORREF textclr, COLORREF backclr, const tstring& aText, 
	double size, double done, bool odcStyle, bool colorOverride, int depth, int lighten, DWORD tAlign/*DT_LEFT*/) {
	// fixes issues with double border
	rc.top -= 1;
	// Real rc, the original one.
	CRect real_rc = rc;
	// We need to offset the current rc to (0, 0) to paint on the New dc
	rc.MoveToXY(0, 0);

	CDC cdc;
	cdc.CreateCompatibleDC(drawDC);

	HBITMAP pOldBmp = cdc.SelectBitmap(CreateCompatibleBitmap(drawDC, real_rc.Width(), real_rc.Height()));
	HDC& dc = cdc.m_hDC;

	CFont oldFont = (HFONT)SelectObject(dc, WinUtil::font);
	SetBkMode(dc, TRANSPARENT);

	COLORREF oldcol;
	if (odcStyle) {
		// New style progressbar tweaks the current colors
		HLSTRIPLE hls_bk = OperaColors::RGB2HLS(bgColor);

		// Create pen (ie outline border of the cell)
		HPEN penBorder = ::CreatePen(PS_SOLID, 1, OperaColors::blendColors(bgColor, clr, (hls_bk.hlstLightness > 0.75) ? 0.6 : 0.4));
		HGDIOBJ pOldPen = ::SelectObject(dc, penBorder);

		// Draw the outline (but NOT the background) using pen
		HBRUSH hBrOldBg = CreateSolidBrush(bgColor);
		hBrOldBg = (HBRUSH)::SelectObject(dc, hBrOldBg);
		::Rectangle(dc, rc.left, rc.top, rc.right, rc.bottom);
		DeleteObject(::SelectObject(dc, hBrOldBg));

		// Set the background color, by slightly changing it
		HBRUSH hBrDefBg = CreateSolidBrush(OperaColors::blendColors(bgColor, clr, (hls_bk.hlstLightness > 0.75) ? 0.85 : 0.70));
		HGDIOBJ oldBg = ::SelectObject(dc, hBrDefBg);

		// Draw the outline AND the background using pen+brush
		::Rectangle(dc, rc.left, rc.top, rc.left + (LONG)(rc.Width() * 1 + 0.5), rc.bottom);

		// Reset pen
		DeleteObject(::SelectObject(dc, pOldPen));
		// Reset bg (brush)
		DeleteObject(::SelectObject(dc, oldBg));

		// Draw the text over the entire item
		oldcol = ::SetTextColor(dc, colorOverride ? textclr : clr);

		//Want to center the text, DT_CENTER wont work with the changing text colours so center with the drawing rect..
		CRect textRect = rc;
		if (tAlign & DT_CENTER) {
			int textWidth = WinUtil::getTextWidth(aText, dc);
			textRect.left += (textRect.right / 2) - (textWidth / 2);
			tAlign &= ~DT_CENTER;
		} else {
			textRect.left += 6;
		}
		::DrawText(dc, aText.c_str(), aText.length(), textRect, tAlign | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);

		rc.right = rc.left + size > 0 ? (rc.Width() * done / size) : 0;

		COLORREF a, b;
		OperaColors::EnlightenFlood(clr, a, b, lighten);
		OperaColors::FloodFill(cdc, rc.left + 1, rc.top + 1, rc.right, rc.bottom - 1, a, b);

		// Draw the text only over the bar and with correct color
		::SetTextColor(dc, colorOverride ? textclr :
			OperaColors::TextFromBackground(clr));

		textRect.right = textRect.left > rc.right ? textRect.left : rc.right;
		::DrawText(dc, aText.c_str(), aText.length(), textRect, tAlign | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
	} else {
		CBarShader statusBar(rc.bottom - rc.top, rc.right - rc.left, backclr, size);
		statusBar.FillRange(0, done, clr);
		statusBar.Draw(cdc, rc.top, rc.left, depth);

		// Get the color of this text bar
		oldcol = ::SetTextColor(dc, colorOverride ? textclr :
			OperaColors::TextFromBackground(clr));

		::DrawText(dc, aText.c_str(), aText.length(), rc, tAlign | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
	}
	SelectObject(dc, oldFont);
	::SetTextColor(dc, oldcol);

	// New way:
	BitBlt(drawDC, real_rc.left, real_rc.top, real_rc.Width(), real_rc.Height(), dc, 0, 0, SRCCOPY);
	DeleteObject(cdc.SelectBitmap(pOldBmp));

}

/* Only returns the text color */
COLORREF WinUtil::getDupeColor(DupeType aType) {
	if (aType == DUPE_SHARE_FULL) {
		return SETTING(DUPE_COLOR);
	} else if (aType == DUPE_FINISHED_FULL) {
		return blendColors(SETTING(QUEUE_COLOR), SETTING(BACKGROUND_COLOR));
	} else if (aType == DUPE_FINISHED_PARTIAL) {
		return blendColors(SETTING(QUEUE_COLOR), SETTING(BACKGROUND_COLOR));
	} else if (aType == DUPE_QUEUE_FULL) {
		return SETTING(QUEUE_COLOR);
	} else if(aType == DUPE_SHARE_PARTIAL) {
		return blendColors(SETTING(DUPE_COLOR), SETTING(BACKGROUND_COLOR));
	} else if(aType == DUPE_QUEUE_PARTIAL) {
		return blendColors(SETTING(QUEUE_COLOR), SETTING(BACKGROUND_COLOR));
	} else if(aType == DUPE_SHARE_QUEUE) {
		return blendColors(SETTING(QUEUE_COLOR), SETTING(DUPE_COLOR));
	}

	return SETTING(TEXT_COLOR);
}

/* Text + the background color */
pair<COLORREF, COLORREF> WinUtil::getDupeColors(DupeType aType) {
	if (aType == DUPE_SHARE_FULL) {
		return make_pair(SETTING(DUPE_COLOR), SETTING(TEXT_DUPE_BACK_COLOR));
	} else if (aType == DUPE_FINISHED_FULL) {
		return make_pair(blendColors(SETTING(QUEUE_COLOR), SETTING(BACKGROUND_COLOR)), SETTING(TEXT_QUEUE_BACK_COLOR));
	} else if (aType == DUPE_FINISHED_PARTIAL) {
		return make_pair(blendColors(SETTING(QUEUE_COLOR), SETTING(BACKGROUND_COLOR)), SETTING(TEXT_QUEUE_BACK_COLOR));
	} else if (aType == DUPE_QUEUE_FULL) {
		return make_pair(SETTING(QUEUE_COLOR), SETTING(TEXT_QUEUE_BACK_COLOR));
	} else if(aType == DUPE_SHARE_PARTIAL) {
		return make_pair(blendColors(SETTING(DUPE_COLOR), SETTING(TEXT_DUPE_BACK_COLOR)), SETTING(TEXT_DUPE_BACK_COLOR));
	} else if(aType == DUPE_QUEUE_PARTIAL) {
		return make_pair(blendColors(SETTING(QUEUE_COLOR), SETTING(TEXT_QUEUE_BACK_COLOR)), SETTING(TEXT_QUEUE_BACK_COLOR));
	} else if(aType == DUPE_SHARE_QUEUE) {
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

void WinUtil::addCue(HWND hwnd, LPCWSTR text, BOOL drawFocus) {
	Edit_SetCueBannerTextFocused(hwnd, text, drawFocus);
}

void WinUtil::addUpdate(const string& aUpdater, bool aTesting) noexcept {
	updated = true;
	auto appPath = Util::getAppFilePath();

	auto updateCmd = Text::toT("/update \"" + appPath + "\\\""); // The extra end slash is required!
	if (SystemUtil::isElevated()) {
		updateCmd += _T(" /elevation");
	}

	if (aTesting) {
		updateCmd += _T(" /test");
	}

	updateCommand = make_pair(Text::toT(aUpdater), updateCmd);
}

bool WinUtil::runPendingUpdate() noexcept {
	if(updated && !updateCommand.first.empty()) {
		auto cmd = updateCommand.second + Text::toT(Util::getStartupParams(false));
		ShellExecute(NULL, _T("runas"), updateCommand.first.c_str(), cmd.c_str(), NULL, SW_SHOWNORMAL);
		return true;
	}

	return false;
}

void WinUtil::showPopup(tstring szMsg, tstring szTitle, HICON hIcon, bool force) {
	MainFrame::getMainFrame()->ShowPopup(szMsg, szTitle, hIcon, force); 
}

void WinUtil::showPopup(tstring szMsg, tstring szTitle, DWORD dwInfoFlags, bool force) {
	MainFrame::getMainFrame()->ShowPopup(szMsg, szTitle, dwInfoFlags, force); 
}

time_t WinUtil::parseDate(CEdit& ctrlDate, CComboBox& ctrlDateUnit) {
	tstring date(ctrlDate.GetWindowTextLength() + 1, _T('\0'));
	ctrlDate.GetWindowText(&date[0], date.size());
	date.resize(date.size() - 1);

	time_t ldate = Util::toUInt32(Text::fromT(date));
	if (ldate > 0) {
		switch (ctrlDateUnit.GetCurSel()) {
		case 0:
			ldate *= 60 * 60; break;
		case 1:
			ldate *= 60 * 60 * 24; break;
		case 2:
			ldate *= 60 * 60 * 24 * 7; break;
		case 3:
			ldate *= 60 * 60 * 24 * 30; break;
		case 4:
			ldate *= 60 * 60 * 24 * 365; break;
		}

		ldate = GET_TIME() - ldate;
	}

	return ldate;
}

int64_t WinUtil::parseSize(CEdit& ctrlSize, CComboBox& ctrlSizeMode) {
	tstring size(ctrlSize.GetWindowTextLength() + 1, _T('\0'));
	ctrlSize.GetWindowText(&size[0], size.size());
	size.resize(size.size() - 1);

	double lsize = Util::toDouble(Text::fromT(size));
	switch (ctrlSizeMode.GetCurSel()) {
	case 1:
		lsize = Util::convertSize(lsize, Util::KB); break;
	case 2:
		lsize = Util::convertSize(lsize, Util::MB); break;
	case 3:
		lsize = Util::convertSize(lsize, Util::GB); break;
	}

	return static_cast<int64_t>(lsize);
}

tstring WinUtil::getEditText(CEdit& edit) {
	tstring tmp;
	tmp.resize(edit.GetWindowTextLength());
	tmp.resize(edit.GetWindowText(&tmp[0], tmp.size() + 1));
	return tmp;
}

tstring WinUtil::getComboText(CComboBox& aCombo, WORD wNotifyCode) {
	tstring tmp;
	if (wNotifyCode == CBN_SELENDOK) {
		tmp.resize(aCombo.GetLBTextLen(aCombo.GetCurSel()));
		tmp.resize(aCombo.GetLBText(aCombo.GetCurSel(), &tmp[0]));
	} else {
		tmp.resize(aCombo.GetWindowTextLength());
		tmp.resize(aCombo.GetWindowText(&tmp[0], tmp.size() + 1));
	}
	return tmp;
}


void WinUtil::handleTab(HWND focus, HWND* ctrlHwnds, int hwndCount) {
	bool shift = isShift();

	int i;
	for (i = 0; i < hwndCount; i++) {
		if (ctrlHwnds[i] == focus)
			break;
	}

	// don't focus to a disabled control
	for (auto s = 0; s < hwndCount; ++s) {
		i = (i + (shift ? -1 : 1)) % hwndCount;
		if (::IsWindowEnabled(ctrlHwnds[i])) {
			::SetFocus(ctrlHwnds[i]);
			break;
		}
	}
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

bool WinUtil::checkClientPassword() {
	if (hasPassDlg)
		return false;

	auto passDlg = PassDlg();
	ScopedFunctor([] { hasPassDlg = false; });

	passDlg.description = TSTRING(PASSWORD_DESC);
	passDlg.title = TSTRING(PASSWORD_TITLE);
	passDlg.ok = TSTRING(UNLOCK);
	if(passDlg.DoModal(NULL) == IDOK) {
		if (passDlg.line != Text::toT(Util::base64_decode(SETTING(PASSWORD)))) {
			MessageBox(mainWnd, CTSTRING(INVALID_PASSWORD), Text::toT(shortVersionString).c_str(), MB_OK | MB_ICONERROR);
			return false;
		}
	}

	return true;
}

tstring WinUtil::formatFolderContent(const DirectoryContentInfo& aContentInfo) {
	return Text::toT(Util::formatDirectoryContent(aContentInfo));
}

tstring WinUtil::formatFileType(const string& aFileName) {
	auto type = Util::getFileExt(aFileName);
	if (type.size() > 0 && type[0] == '.')
		type.erase(0, 1);
	return Text::toT(type);
}

void WinUtil::insertBindAddresses(const AdapterInfoList& aBindAdapters, CComboBox& combo_, const string& aCurValue) noexcept {
	for (const auto& addr : aBindAdapters) {
		auto caption = Text::toT(addr.adapterName);
		if (!addr.ip.empty()) {
			caption += _T(" (") + Text::toT(addr.ip) + _T(")");
		}

		combo_.AddString(caption.c_str());
	}

	// Select the current address
	auto curItem = boost::find_if(aBindAdapters, [&aCurValue](const AdapterInfo& aInfo) { return aInfo.ip == aCurValue; });
	combo_.SetCurSel(distance(aBindAdapters.begin(), curItem));
}
