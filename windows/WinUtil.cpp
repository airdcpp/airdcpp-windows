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

#define COMPILE_MULTIMON_STUBS 1
#include <MultiMon.h>
#include <psapi.h>
#include <powrprof.h>
#include <direct.h>
#include <pdh.h>
#include <WinInet.h>
#include <control.h>
#include <atlwin.h>

#include <strmif.h>	// error with missing ddraw.h, get it from MS DirectX SDK

#include "WinUtil.h"
#include "PrivateFrame.h"
#include "SearchFrm.h"
#include "LineDlg.h"
#include "MainFrm.h"

#include "../client/Util.h"
#include "../client/StringTokenizer.h"
#include "../client/ShareManager.h"
#include "../client/ClientManager.h"
#include "../client/TimerManager.h"
#include "../client/FavoriteManager.h"
#include "../client/ResourceManager.h"
#include "../client/QueueManager.h"
#include "../client/UploadManager.h"
#include "../client/HashManager.h"
#include "../client/LogManager.h"
#include "../client/version.h"
#include "../client/pme.h"
#include "../client/SFVReader.h"

#include "boost/algorithm/string/replace.hpp"
#include "boost/algorithm/string/trim.hpp"
#include "boost/format.hpp"

#include "iTunesCOMInterface.h"
#include "WMPlayerRemoteApi.h"
#include "HubFrame.h"
#include "MagnetDlg.h"
#include "winamp.h"
#include "BarShader.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cstring>

WinUtil::ImageMap WinUtil::fileIndexes;
int WinUtil::fileImageCount;
HBRUSH WinUtil::bgBrush = NULL;
COLORREF WinUtil::textColor = 0;
COLORREF WinUtil::bgColor = 0;
HFONT WinUtil::font = NULL;
int WinUtil::fontHeight = 0;
HFONT WinUtil::boldFont = NULL;
HFONT WinUtil::systemFont = NULL;
HFONT WinUtil::smallBoldFont = NULL;
HFONT WinUtil::tabFont = NULL;
CMenu WinUtil::mainMenu;
OMenu WinUtil::grantMenu;
CImageList WinUtil::fileImages;
CImageList WinUtil::userImages;
CImageList WinUtil::flagImages;
int WinUtil::dirIconIndex = 0;
int WinUtil::dirMaskedIndex = 0;
TStringList WinUtil::lastDirs;
TStringList WinUtil::volumes;
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

static const char* countryNames[] = { "ANDORRA", "UNITED ARAB EMIRATES", "AFGHANISTAN", "ANTIGUA AND BARBUDA", 
"ANGUILLA", "ALBANIA", "ARMENIA", "NETHERLANDS ANTILLES", "ANGOLA", "ANTARCTICA", "ARGENTINA", "AMERICAN SAMOA", 
"AUSTRIA", "AUSTRALIA", "ARUBA", "ALAND", "AZERBAIJAN", "BOSNIA AND HERZEGOVINA", "BARBADOS", "BANGLADESH", 
"BELGIUM", "BURKINA FASO", "BULGARIA", "BAHRAIN", "BURUNDI", "BENIN", "BERMUDA", "BRUNEI DARUSSALAM", "BOLIVIA", 
"BRAZIL", "BAHAMAS", "BHUTAN", "BOUVET ISLAND", "BOTSWANA", "BELARUS", "BELIZE", "CANADA", "COCOS ISLANDS", 
"THE DEMOCRATIC REPUBLIC OF THE CONGO", "CENTRAL AFRICAN REPUBLIC", "CONGO", "SWITZERLAND", "COTE D'IVOIRE", "COOK ISLANDS", 
"CHILE", "CAMEROON", "CHINA", "COLOMBIA", "COSTA RICA", "SERBIA AND MONTENEGRO", "CUBA", "CAPE VERDE", 
"CHRISTMAS ISLAND", "CYPRUS", "CZECH REPUBLIC", "GERMANY", "DJIBOUTI", "DENMARK", "DOMINICA", "DOMINICAN REPUBLIC", 
"ALGERIA", "ECUADOR", "ESTONIA", "EGYPT", "WESTERN SAHARA", "ERITREA", "SPAIN", "ETHIOPIA", "EUROPEAN UNION", "FINLAND", "FIJI", 
"FALKLAND ISLANDS", "MICRONESIA", "FAROE ISLANDS", "FRANCE", "GABON", "UNITED KINGDOM", "GRENADA", "GEORGIA", 
"FRENCH GUIANA", "GUERNSEY", "GHANA", "GIBRALTAR", "GREENLAND", "GAMBIA", "GUINEA", "GUADELOUPE", "EQUATORIAL GUINEA", 
"GREECE", "SOUTH GEORGIA AND THE SOUTH SANDWICH ISLANDS", "GUATEMALA", "GUAM", "GUINEA-BISSAU", "GUYANA", 
"HONG KONG", "HEARD ISLAND AND MCDONALD ISLANDS", "HONDURAS", "CROATIA", "HAITI", "HUNGARY", 
"INDONESIA", "IRELAND", "ISRAEL",  "ISLE OF MAN", "INDIA", "BRITISH INDIAN OCEAN TERRITORY", "IRAQ", "IRAN", "ICELAND", 
"ITALY","JERSEY", "JAMAICA", "JORDAN", "JAPAN", "KENYA", "KYRGYZSTAN", "CAMBODIA", "KIRIBATI", "COMOROS", 
"SAINT KITTS AND NEVIS", "DEMOCRATIC PEOPLE'S REPUBLIC OF KOREA", "SOUTH KOREA", "KUWAIT", "CAYMAN ISLANDS", 
"KAZAKHSTAN", "LAO PEOPLE'S DEMOCRATIC REPUBLIC", "LEBANON", "SAINT LUCIA", "LIECHTENSTEIN", "SRI LANKA", 
"LIBERIA", "LESOTHO", "LITHUANIA", "LUXEMBOURG", "LATVIA", "LIBYAN ARAB JAMAHIRIYA", "MOROCCO", "MONACO", 
"MOLDOVA", "MONTENEGRO", "MADAGASCAR", "MARSHALL ISLANDS", "MACEDONIA", "MALI", "MYANMAR", "MONGOLIA", "MACAO", 
"NORTHERN MARIANA ISLANDS", "MARTINIQUE", "MAURITANIA", "MONTSERRAT", "MALTA", "MAURITIUS", "MALDIVES", 
"MALAWI", "MEXICO", "MALAYSIA", "MOZAMBIQUE", "NAMIBIA", "NEW CALEDONIA", "NIGER", "NORFOLK ISLAND", 
"NIGERIA", "NICARAGUA", "NETHERLANDS", "NORWAY", "NEPAL", "NAURU", "NIUE", "NEW ZEALAND", "OMAN", "PANAMA", 
"PERU", "FRENCH POLYNESIA", "PAPUA NEW GUINEA", "PHILIPPINES", "PAKISTAN", "POLAND", "SAINT PIERRE AND MIQUELON", 
"PITCAIRN", "PUERTO RICO", "PALESTINIAN TERRITORY", "PORTUGAL", "PALAU", "PARAGUAY", "QATAR", "REUNION", 
"ROMANIA", "SERBIA", "RUSSIAN FEDERATION", "RWANDA", "SAUDI ARABIA", "SOLOMON ISLANDS", "SEYCHELLES", "SUDAN", 
"SWEDEN", "SINGAPORE", "SAINT HELENA", "SLOVENIA", "SVALBARD AND JAN MAYEN", "SLOVAKIA", "SIERRA LEONE", 
"SAN MARINO", "SENEGAL", "SOMALIA", "SURINAME", "SAO TOME AND PRINCIPE", "EL SALVADOR", "SYRIAN ARAB REPUBLIC", 
"SWAZILAND", "TURKS AND CAICOS ISLANDS", "CHAD", "FRENCH SOUTHERN TERRITORIES", "TOGO", "THAILAND", "TAJIKISTAN", 
"TOKELAU", "TIMOR-LESTE", "TURKMENISTAN", "TUNISIA", "TONGA", "TURKEY", "TRINIDAD AND TOBAGO", "TUVALU", "TAIWAN", 
"TANZANIA", "UKRAINE", "UGANDA", "UNITED STATES MINOR OUTLYING ISLANDS", "UNITED STATES", "URUGUAY", "UZBEKISTAN", 
"VATICAN", "SAINT VINCENT AND THE GRENADINES", "VENEZUELA", "BRITISH VIRGIN ISLANDS", "U.S. VIRGIN ISLANDS", 
"VIET NAM", "VANUATU", "WALLIS AND FUTUNA", "SAMOA", "YEMEN", "MAYOTTE", "YUGOSLAVIA", "SOUTH AFRICA", "ZAMBIA", 
"ZIMBABWE" };

static const char* countryCodes[] = { 
 "AD", "AE", "AF", "AG", "AI", "AL", "AM", "AN", "AO", "AQ", "AR", "AS", "AT", "AU", "AW", "AX", "AZ", "BA", "BB", 
 "BD", "BE", "BF", "BG", "BH", "BI", "BJ", "BM", "BN", "BO", "BR", "BS", "BT", "BV", "BW", "BY", "BZ", "CA", "CC", 
 "CD", "CF", "CG", "CH", "CI", "CK", "CL", "CM", "CN", "CO", "CR", "CS", "CU", "CV", "CX", "CY", "CZ", "DE", "DJ", 
 "DK", "DM", "DO", "DZ", "EC", "EE", "EG", "EH", "ER", "ES", "ET", "EU", "FI", "FJ", "FK", "FM", "FO", "FR", "GA", 
 "GB", "GD", "GE", "GF", "GG", "GH", "GI", "GL", "GM", "GN", "GP", "GQ", "GR", "GS", "GT", "GU", "GW", "GY", "HK", 
 "HM", "HN", "HR", "HT", "HU", "ID", "IE", "IL", "IM", "IN", "IO", "IQ", "IR", "IS", "IT", "JE", "JM", "JO", "JP", 
 "KE", "KG", "KH", "KI", "KM", "KN", "KP", "KR", "KW", "KY", "KZ", "LA", "LB", "LC", "LI", "LK", "LR", "LS", "LT", 
 "LU", "LV", "LY", "MA", "MC", "MD", "ME", "MG", "MH", "MK", "ML", "MM", "MN", "MO", "MP", "MQ", "MR", "MS", "MT", 
 "MU", "MV", "MW", "MX", "MY", "MZ", "NA", "NC", "NE", "NF", "NG", "NI", "NL", "NO", "NP", "NR", "NU", "NZ", "OM", 
 "PA", "PE", "PF", "PG", "PH", "PK", "PL", "PM", "PN", "PR", "PS", "PT", "PW", "PY", "QA", "RE", "RO", "RS", "RU", 
 "RW", "SA", "SB", "SC", "SD", "SE", "SG", "SH", "SI", "SJ", "SK", "SL", "SM", "SN", "SO", "SR", "ST", "SV", "SY", 
 "SZ", "TC", "TD", "TF", "TG", "TH", "TJ", "TK", "TL", "TM", "TN", "TO", "TR", "TT", "TV", "TW", "TZ", "UA", "UG", 
 "UM", "US", "UY", "UZ", "VA", "VC", "VE", "VG", "VI", "VN", "VU", "WF", "WS", "YE", "YT", "YU", "ZA", "ZM", "ZW" };
	
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

void UserInfoBase::matchQueue(const string& hubHint) {
	if(getUser()) {
		try {
			QueueManager::getInstance()->addList(HintedUser(getUser(), hubHint), QueueItem::FLAG_MATCH_QUEUE);
		} catch(const Exception& e) {
			LogManager::getInstance()->message(e.getError());
		}
	}
}

void UserInfoBase::getList(const string& hubHint) {
	if(getUser()) {
		try {
			QueueManager::getInstance()->addList(HintedUser(getUser(), hubHint), QueueItem::FLAG_CLIENT_VIEW);
		} catch(const Exception& e) {
			LogManager::getInstance()->message(e.getError());		
		}
	}
}
void UserInfoBase::browseList(const string& hubHint) {
	if(!getUser() || getUser()->getCID().isZero())
		return;
	try {
		QueueManager::getInstance()->addList(HintedUser(getUser(), hubHint), QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_PARTIAL_LIST);
	} catch(const Exception& e) {
		LogManager::getInstance()->message(e.getError());		
	}
}
void UserInfoBase::addFav() {
	if(getUser()) {
		FavoriteManager::getInstance()->addFavoriteUser(getUser());
	}
}
void UserInfoBase::pm(const string& hubHint) {
	if(getUser()) {
		PrivateFrame::openWindow(HintedUser(getUser(), hubHint), Util::emptyStringT, NULL);
	}
}
void UserInfoBase::connectFav() {
	if(getUser()) {
		string url = FavoriteManager::getInstance()->getUserURL(getUser());
		if(!url.empty()) {
			HubFrame::openWindow(Text::toT(url));
		}
	}
}
void UserInfoBase::grant(const string& hubHint) {
	if(getUser()) {
		UploadManager::getInstance()->reserveSlot(HintedUser(getUser(), hubHint), 600);
	}
}
void UserInfoBase::removeAll() {
	if(getUser()) {
		QueueManager::getInstance()->removeSource(getUser(), QueueItem::Source::FLAG_REMOVED);
	}
}
void UserInfoBase::grantHour(const string& hubHint) {
	if(getUser()) {
		UploadManager::getInstance()->reserveSlot(HintedUser(getUser(), hubHint), 3600);
	}
}
void UserInfoBase::grantDay(const string& hubHint) {
	if(getUser()) {
		UploadManager::getInstance()->reserveSlot(HintedUser(getUser(), hubHint), 24*3600);
	}
}
void UserInfoBase::grantWeek(const string& hubHint) {
	if(getUser()) {
		UploadManager::getInstance()->reserveSlot(HintedUser(getUser(), hubHint), 7*24*3600);
	}
}
void UserInfoBase::ungrant() {
	if(getUser()) {
		UploadManager::getInstance()->unreserveSlot(getUser());
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

void WinUtil::reLoadImages(){
	userImages.Destroy();
	if(SETTING(USERLIST_IMAGE).empty())
		userImages.CreateFromImage(IDB_USERS, 16, 9, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	else
		userImages.CreateFromImage(Text::toT(SETTING(USERLIST_IMAGE)).c_str(), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED | LR_LOADFROMFILE);
}

void WinUtil::init(HWND hWnd) {
	mainWnd = hWnd;

	mainMenu.CreateMenu();

	CMenuHandle file;
	file.CreatePopupMenu();

	file.AppendMenu(MF_STRING, IDC_OPEN_FILE_LIST, CTSTRING(MENU_OPEN_FILE_LIST));
	file.AppendMenu(MF_STRING, IDC_OPEN_MY_LIST, CTSTRING(MENU_OPEN_OWN_LIST));
	file.AppendMenu(MF_STRING, IDC_MATCH_ALL, CTSTRING(MENU_OPEN_MATCH_ALL));
	file.AppendMenu(MF_STRING, IDC_REFRESH_FILE_LIST, CTSTRING(MENU_REFRESH_FILE_LIST));
	file.AppendMenu(MF_STRING, IDC_SCAN_MISSING, CTSTRING(MENU_SCAN_MISSING));
	file.AppendMenu(MF_STRING, IDC_OPEN_DOWNLOADS, CTSTRING(MENU_OPEN_DOWNLOADS_DIR));
	file.AppendMenu(MF_SEPARATOR);
	file.AppendMenu(MF_STRING, ID_FILE_QUICK_CONNECT, CTSTRING(MENU_QUICK_CONNECT));
	file.AppendMenu(MF_STRING, IDC_FOLLOW, CTSTRING(MENU_FOLLOW_REDIRECT));
	file.AppendMenu(MF_STRING, ID_FILE_RECONNECT, CTSTRING(MENU_RECONNECT));
	file.AppendMenu(MF_SEPARATOR);
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
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, IDC_CDMDEBUG_WINDOW, CTSTRING(MENU_CDMDEBUG_MESSAGES));
	view.AppendMenu(MF_STRING, IDC_NOTEPAD, CTSTRING(MENU_NOTEPAD));
	view.AppendMenu(MF_STRING, IDC_HASH_PROGRESS, CTSTRING(MENU_HASH_PROGRESS));
	view.AppendMenu(MF_SEPARATOR);
	view.AppendMenu(MF_STRING, ID_VIEW_TOOLBAR, CTSTRING(MENU_TOOLBAR));
	view.AppendMenu(MF_STRING, ID_VIEW_STATUS_BAR, CTSTRING(MENU_STATUS_BAR));
	view.AppendMenu(MF_STRING, ID_VIEW_TRANSFER_VIEW, CTSTRING(MENU_TRANSFER_VIEW));
	view.AppendMenu(MF_STRING, ID_TOGGLE_TOOLBAR, CTSTRING(TOGGLE_TOOLBAR));

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
	window.AppendMenu(MF_SEPARATOR);
	window.AppendMenu(MF_STRING, IDC_OPEN_SYSTEMLOG, CTSTRING(MENU_OPEN_SYSTEMLOG));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)window, CTSTRING(MENU_WINDOW));

	CMenuHandle help;
	help.CreatePopupMenu();

	help.AppendMenu(MF_STRING, ID_APP_ABOUT, CTSTRING(MENU_ABOUT));
	help.AppendMenu(MF_STRING, ID_WIZARD, CTSTRING(WIZARD));
	help.AppendMenu(MF_SEPARATOR);
	help.AppendMenu(MF_STRING, IDC_HELP_HOMEPAGE, CTSTRING(MENU_HOMEPAGE));
	help.AppendMenu(MF_STRING, IDC_HELP_GEOIPFILE, CTSTRING(MENU_HELP_GEOIPFILE));
	help.AppendMenu(MF_STRING, IDC_HELP_GUIDES, CTSTRING(MENU_GUIDES));
	help.AppendMenu(MF_STRING, IDC_HELP_DISCUSS, CTSTRING(MENU_DISCUSS));
	help.AppendMenu(MF_STRING, IDC_HELP_CUSTOMIZE, CTSTRING(MENU_CUSTOMIZE));

	mainMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)help, CTSTRING(MENU_HELP));

/** @todo fix this so that the system icon is used for dirs as well (we need
			  to mask it so that incomplete folders appear correct */
#if 0	
	if(BOOLSETTING(USE_SYSTEM_ICONS)) {
		SHFILEINFO fi;
		memzero(&fi, sizeof(SHFILEINFO));
		fileImages.Create(16, 16, ILC_COLOR32 | ILC_MASK, 16, 16);
		::SHGetFileInfo(_T("./"), FILE_ATTRIBUTE_DIRECTORY, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		fileImages.AddIcon(fi.hIcon);
		fileImages.AddIcon(ic);
		::DestroyIcon(fi.hIcon);
	} else {
		fileImages.CreateFromImage(IDB_FOLDERS, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	}
#endif

	fileImages.CreateFromImage(IDB_FOLDERS, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	dirIconIndex = fileImageCount++;
	dirMaskedIndex = fileImageCount++;

	fileImageCount++;

	flagImages.CreateFromImage(IDB_FLAGS, 25, 8, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);

	if(SETTING(USERLIST_IMAGE) == "")
		userImages.CreateFromImage(IDB_USERS, 16, 9, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	else
		userImages.CreateFromImage(Text::toT(SETTING(USERLIST_IMAGE)).c_str(), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED | LR_LOADFROMFILE); 
	
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
	smallBoldFont = ::CreateFontIndirect(&lf);
	tabFont = ::CreateFontIndirect(&lf);
	systemFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);

	if(BOOLSETTING(URL_HANDLER)) {
		registerDchubHandler();
		registerADChubHandler();
		registerADCShubHandler();
		urlDcADCRegistered = true;
	}
	if(BOOLSETTING(MAGNET_REGISTER)) {
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
	fileImages.Destroy();
	userImages.Destroy();
	flagImages.Destroy();
	::DeleteObject(font);
	::DeleteObject(boldFont);
	::DeleteObject(smallBoldFont);
	::DeleteObject(bgBrush);
	::DeleteObject(tabFont);

	mainMenu.DestroyMenu();
	grantMenu.DestroyMenu();

	UnhookWindowsHookEx(hook);	

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
		
		if(target.size() > 0 && target[target.size()-1] != _T('\\'))
			target+=_T('\\');
		
		if(SHGetMalloc(&ma) != E_FAIL) {
			ma->Free(pidl);
			ma->Release();
		}
		return true;
	}
	return false;
}

bool WinUtil::browseFile(tstring& target, HWND owner /* = NULL */, bool save /* = true */, const tstring& initialDir /* = Util::emptyString */, const TCHAR* types /* = NULL */, const TCHAR* defExt /* = NULL */) {
	TCHAR buf[MAX_PATH];
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
	for(StringList::const_iterator i = l.begin(); i != l.end() && k < maxItems; ++i, ++k) {
		array[k] = Util::toInt(*i);
	}
}

bool WinUtil::getUCParams(HWND parent, const UserCommand& uc, StringMap& sm) noexcept {
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
			dlg.line = Text::toT(sm["line:" + name]);

			if(uc.adc()) {
				Util::replace(_T("\\\\"), _T("\\"), dlg.description);
				Util::replace(_T("\\s"), _T(" "), dlg.description);
			}

			if(dlg.DoModal(parent) == IDOK) {
				sm["line:" + name] = Text::fromT(dlg.line);
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
				sm["kickline:" + name] = Text::fromT(dlg.line);
				done[name] = Text::fromT(dlg.line);
			} else {
				return false;
			}
		}
		i = j + 1;
	}
	return true;
}

#ifdef SVNVERSION
#define LINE2 _T("-- http://www.airdcpp.net  <AirDC++ ") _T(VERSIONSTRING) _T(SVNVERSION) _T(" / ") _T(DCVERSIONSTRING) _T(">")
#else
#define LINE2 _T("-- http://www.airdcpp.net  <AirDC++ ") _T(VERSIONSTRING) _T(" / ") _T(DCVERSIONSTRING) _T(">")
#endif

TCHAR *msgs[] = { _T("\r\n-- I'm a happy AirDC++ user. You could be happy too.\r\n") LINE2,
_T("\r\n-- Is it Superman? No, its AirDC++\r\n") LINE2,
_T("\r\n-- My files are burning in my computer...Download are way too fast\r\n") LINE2,
_T("\r\n-- STOP!! My client is too fast, slow down with the writings\r\n") LINE2,
_T("\r\n-- We are always behind the corner waiting to grab your nuts..i mean files\r\n") LINE2,
_T("\r\n-- Knock, knock....We are here to take your files again, we needed backup files too\r\n") LINE2,
_T("\r\n-- Why bother searching when Airdc++ take care of everything?\r\n") LINE2,
_T("\r\n-- the only way to stop me to getting your files is closing the DC++\r\n") LINE2,
_T("\r\n-- We love your files soo much, so we try to get them over and over again...\r\n") LINE2,
_T("\r\n-- Let us thru the waiting line, we download faster than the lightning\r\n") LINE2,
_T("\r\n-- Sometimes we download so fast that we accidently get the whole person on the other side...\r\n") LINE2,
_T("\r\n-- Holy crap, my download speed is sooooo fast that it made a hole in the harddrive\r\n") LINE2,
_T("\r\n-- Once you got access to it, dont let it go...\r\n") LINE2,
_T("\r\n-- Do you feel the wind? Its the download that goes too fast..\r\n") LINE2,
_T("\r\n-- No matter what, no matter where, it's always home if AirDC++ is there\r\n") LINE2,
_T("\r\n-- Knock, knock...We are leaving back the trousers we accidently downloaded...\r\n") LINE2,
_T("\r\n-- Are you blind? you already downloaded that movie 4 times already\r\n") LINE2,
_T("\r\n-- My client has been in jail twice, has  yours?\r\n") LINE2,
_T("\r\n-- Keep your downloads close, but keep your uploads even closer\r\n") LINE2


};

#define MSGS 18

tstring WinUtil::commands = Text::toT("\n\t\t\t\t\tHELP\n\
------------------------------------------------------------------------------------------------------------------------------------------------------------\n\
/refresh\t\t\t\t\t(refresh share)\n\
/rebuild\t\t\t\t\t(rebuild hash data)\n\
/savequeue\t\t\t\t(save Download Queue)\n\
/stop\t\t\t\t\t(stop SFV check)\n\
/save\t\t\t\t\t(save share cache shares.xml)\n\
------------------------------------------------------------------------------------------------------------------------------------------------------------\n\
/search <string>\t\t\t\t(search for...)\n\
/g <searchstring>\t\t\t\t(Google search)\n\
/imdb <imdbquery>\t\t\t\t(search film from IMDB database)\n\
/whois [IP]\t\t\t\t(find info about user from the ip address)\n\
------------------------------------------------------------------------------------------------------------------------------------------------------------\n\
/slots # \t\t\t\t\t(upload slots)\n\
/extraslots # \t\t\t\t(set extra slots)\n\
/smallfilesize # \t\t\t\t(set smallfile size)\n\
/ts \t\t\t\t\t(show timestamp in mainchat)\n\
/connection \t\t\t\t(Show connection settings, IP & ports)\n\
/showjoins \t\t\t\t(show user joins in mainchat)\n\
/shutdown \t\t\t\t(system shutdown)\n\
------------------------------------------------------------------------------------------------------------------------------------------------------------\n\
/AirDC++ \t\t\t\t\t(Show AirDC++ version in mainchat)\n\
------------------------------------------------------------------------------------------------------------------------------------------------------------\n\
/away <msg>\t\t\t\t(set away message)\n\
/winamp, /w\t\t\t\t(Shows Winamp spam in mainchat)\n\
/spotify, /s\t\t\t\t(Shows Spotify spam in mainchat)\n\
/itunes\t\t\t\t\t(Shows iTunes spam in mainchat)\n\
/wmp\t\t\t\t\t(Shows Windows Media Player spam in mainchat)\n\
/mpc\t\t\t\t\t(Shows Media Player Classic spam in mainchat)\n\
/clear,/c\t\t\t\t\t(Clears mainchat)\n\
/speed\t\t\t\t\t(show download/upload speeds in mainchat)\n\
/stats\t\t\t\t\t(Show stats in mainchat)\n\
/prvstats\t\t\t\t\t(View stats visible only to yourself)\n\
/info\t\t\t\t\t(View system info visible only to yourself)\n\
/log system\t\t\t\t(open system log)\n\
/log downloads \t\t\t\t(open downloads log)\n\
/log uploads\t\t\t\t(open uploads log)\n\
/df \t\t\t\t\tShow Disk Space info (Only Visible to yourself)\n\
/dfs \t\t\t\t\tShow Disk Space info (as Public message)\n\
/disks, /di \t\t\t\t\tShow Detailed Disk Info about all mounted disks\n\
/uptime \t\t\t\t\tShow Uptime Info\n\
/topic\t\t\t\t\tShow Topic\n\
/ctopic\t\t\t\t\tOpen Link in Topic\n\
/ratio, /r\t\t\t\t\tShow Ratio in chat\n\n");

bool WinUtil::checkCommand(tstring& cmd, tstring& param, tstring& message, tstring& status, bool& thirdPerson) {
	string::size_type i = cmd.find(' ');
	if(i != string::npos) {
		param = cmd.substr(i+1);
		cmd = cmd.substr(1, i - 1);
	} else {
		cmd = cmd.substr(1);
	}

	if(stricmp(cmd.c_str(), _T("log")) == 0) {
		if(stricmp(param.c_str(), _T("system")) == 0) {
			WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + "system.log")));
		} else if(stricmp(param.c_str(), _T("downloads")) == 0) {
			WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatTime(SETTING(LOG_FILE_DOWNLOAD), time(NULL)))));
		} else if(stricmp(param.c_str(), _T("uploads")) == 0) {
			WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatTime(SETTING(LOG_FILE_UPLOAD), time(NULL)))));
		} else {
			return false;
		}
	} else if(stricmp(cmd.c_str(), _T("stop")) == 0) {
		SFVReaderManager::getInstance()->Stop();
	} else if(stricmp(cmd.c_str(), _T("me")) == 0) {
		message = param;
		thirdPerson = true;
	} else if((stricmp(cmd.c_str(), _T("ratio")) == 0) || (stricmp(cmd.c_str(), _T("r")) == 0)) {
		char ratio[512];
		//thirdPerson = true;
		snprintf(ratio, sizeof(ratio), "Ratio: %s (Uploaded: %s | Downloaded: %s)",
			(SETTING(TOTAL_DOWNLOAD) > 0) ? Util::toString(((double)SETTING(TOTAL_UPLOAD)) / ((double)SETTING(TOTAL_DOWNLOAD))).c_str() : "inf.",
			Util::formatBytes(SETTING(TOTAL_UPLOAD)).c_str(),  Util::formatBytes(SETTING(TOTAL_DOWNLOAD)).c_str());
		message = Text::toT(ratio);

	} else if(stricmp(cmd.c_str(), _T("refresh"))==0) {
		//refresh path
		try {
			ShareManager::getInstance()->setDirty();
			if(!param.empty()) {
				if(stricmp(param.c_str(), _T("incoming"))==0) {
					ShareManager::getInstance()->refreshIncoming();
				
			}else if( ShareManager::REFRESH_PATH_NOT_FOUND == ShareManager::getInstance()->refresh( Text::fromT(param) ) )
					status = TSTRING(DIRECTORY_NOT_FOUND);
			} else {
				ShareManager::getInstance()->refresh(ShareManager::REFRESH_ALL | ShareManager::REFRESH_UPDATE);
			}
		} catch(const ShareException& e) {
			status = Text::toT(e.getError());
		}
	} else if(stricmp(cmd.c_str(), _T("slots"))==0) {
		int j = Util::toInt(Text::fromT(param));
		if(j > 0) {
			SettingsManager::getInstance()->set(SettingsManager::SLOTS, j);
			status = TSTRING(SLOTS_SET);
			ClientManager::getInstance()->infoUpdated();
		} else {
			status = TSTRING(INVALID_NUMBER_OF_SLOTS);
		}
	} else if(stricmp(cmd.c_str(), _T("search")) == 0) {
		if(!param.empty()) {
			SearchFrame::openWindow(param);
		} else {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		}
	} else if(stricmp(cmd.c_str(), _T("airdc++")) == 0) {
		message = msgs[GET_TICK() % MSGS];
	} else if(stricmp(cmd.c_str(), _T("save")) == 0) {
		ShareManager::getInstance()->save();
	} else if(stricmp(cmd.c_str(), _T("away")) == 0) {
		if(Util::getAway() && param.empty()) {
			Util::setAway(false);
			MainFrame::setAwayButton(false);
			status = TSTRING(AWAY_MODE_OFF);
		} else {
			Util::setAway(true);
			MainFrame::setAwayButton(true);
			Util::setAwayMessage(Text::fromT(param));
			
			StringMap sm;
			status = TSTRING(AWAY_MODE_ON) + _T(" ") + Text::toT(Util::getAwayMessage(sm));
		}
		ClientManager::getInstance()->infoUpdated();
	} else if(stricmp(cmd.c_str(), _T("g")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://www.google.com/search?q=") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if(stricmp(cmd.c_str(), _T("imdb")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://www.imdb.com/find?q=") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if(stricmp(cmd.c_str(), _T("u")) == 0) {
		if (!param.empty()) {
			WinUtil::openLink(Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if(stricmp(cmd.c_str(), _T("rebuild")) == 0) {
		LogManager::getInstance()->message(STRING(REBUILD_STARTED));
		HashManager::getInstance()->rebuild();
	} else if(stricmp(cmd.c_str(), _T("shutdown")) == 0) {
		MainFrame::setShutDown(!(MainFrame::getShutDown()));
		if (MainFrame::getShutDown()) {
			status = TSTRING(SHUTDOWN_ON);
		} else {
			status = TSTRING(SHUTDOWN_OFF);
			}
					// Mediaplayer Support
	} else if(stricmp(cmd.c_str(), _T("wmp")) == 0) {
		string spam = WinUtil::getWMPSpam(FindWindow(_T("WMPlayerApp"), NULL));
		if(!spam.empty()) {
			if(spam != "no_media") {
				message = Text::toT(spam);
			} else {
				status = _T("You have no media playing in Windows Media Player");
			}
		} else {
			status = _T("Supported version of Windows Media Player is not running");
		}

	} else if((stricmp(cmd.c_str(), _T("spotify")) == 0) || (stricmp(cmd.c_str(), _T("s")) == 0)) {
		string spam = WinUtil::getSpotifySpam(FindWindow(_T("SpotifyMainWindow"), NULL));
		if(!spam.empty()) {
			if(spam != "no_media") {
				message = Text::toT(spam);
			} else {
				status = _T("You have no media playing in Spotify");
			}
		} else {
			status = _T("Supported version of Spotify is not running");
		}

	} else if(stricmp(cmd.c_str(), _T("itunes")) == 0) {
		string spam = WinUtil::getItunesSpam(FindWindow(_T("iTunes"), _T("iTunes")));
		if(!spam.empty()) {
			if(spam != "no_media") {
				message = Text::toT(spam);
			} else {
				status = _T("You have no media playing in iTunes");
			}
		} else {
			status = _T("Supported version of iTunes is not running");
		}
	} else if(stricmp(cmd.c_str(), _T("mpc")) == 0) {
		string spam = WinUtil::getMPCSpam();
		if(!spam.empty()) {
			message = Text::toT(spam);
		} else {
			status = _T("Supported version of Media Player Classic is not running");
		}


	} else if((stricmp(cmd.c_str(), _T("winamp")) == 0) || (stricmp(cmd.c_str(), _T("w")) == 0)) {
		HWND hwndWinamp = FindWindow(_T("Winamp v1.x"), NULL);
		if (hwndWinamp) {
			StringMap params;
			int waVersion = SendMessage(hwndWinamp,WM_USER, 0, IPC_GETVERSION),
				majorVersion, minorVersion;
			majorVersion = waVersion >> 12;
			if (((waVersion & 0x00F0) >> 4) == 0) {
				minorVersion = ((waVersion & 0x0f00) >> 8) * 10 + (waVersion & 0x000f);
			} else {
				minorVersion = ((waVersion & 0x00f0) >> 4) * 10 + (waVersion & 0x000f);
			}
			params["version"] = Util::toString(majorVersion + minorVersion / 100.0);
			int state = SendMessage(hwndWinamp,WM_USER, 0, IPC_ISPLAYING);
			switch (state) {
				case 0: params["state"] = "stopped";
					break;
				case 1: params["state"] = "playing";
					break;
				case 3: params["state"] = "paused";
			};
			TCHAR titleBuffer[2048];
			int buffLength = sizeof(titleBuffer);
			GetWindowText(hwndWinamp, titleBuffer, buffLength);
			tstring title = titleBuffer;
			params["rawtitle"] = Text::fromT(title);
			// there's some winamp bug with scrolling. wa5.09x and 5.1 or something.. see:
			// http://forums.winamp.com/showthread.php?s=&postid=1768775#post1768775
			int starpos = title.find(_T("***"));
			if (starpos >= 1) {
				tstring firstpart = title.substr(0, starpos - 1);
				if (firstpart == title.substr(title.size() - firstpart.size(), title.size())) {
					// fix title
					title = title.substr(starpos, title.size());
				}
			}
			// fix the title if scrolling is on, so need to put the stairs to the end of it
			tstring titletmp = title.substr(title.find(_T("***")) + 2, title.size());
			title = titletmp + title.substr(0, title.size() - titletmp.size());
			title = title.substr(title.find(_T('.')) + 2, title.size());
			if (title.rfind(_T('-')) != string::npos) {
				params["title"] = Text::fromT(title.substr(0, title.rfind(_T('-')) - 1));
			}
		int listlength = SendMessage(hwndWinamp, WM_WA_IPC, 0, IPC_GETLISTLENGTH);
		int waListPosition = SendMessage(hwndWinamp,WM_WA_IPC,0,IPC_GETLISTPOS);
		//wchar_t *filename = (wchar_t*)SendMessage(hwndWinamp,WM_WA_IPC,0,IPC_GET_PLAYING_FILENAME);
		//wchar_t *filename = (wchar_t*)SendMessage(hwndWinamp, WM_WA_IPC, waListPosition, IPC_GET_PLAYING_FILENAME);
		//std::string folder = std::string(filename);

		//params["directory"] = folder;

		// Could be used like; track %[listpos]/%[listlength]
		params["listlength"] = Util::toString(listlength);
		params["listpos"] = Util::toString(waListPosition);

			int curPos = SendMessage(hwndWinamp,WM_USER, 0, IPC_GETOUTPUTTIME);
			int length = SendMessage(hwndWinamp,WM_USER, 1, IPC_GETOUTPUTTIME);
			if (curPos == -1) {
				curPos = 0;
			} else {
				curPos /= 1000;
			}
			int intPercent;
			if (length > 0 ) {
				intPercent = curPos * 100 / length;
			} else {
				length = 0;
				intPercent = 0;
			}
			params["percent"] = Util::toString(intPercent) + "%";
			params["elapsed"] = Text::fromT(Util::formatSeconds(curPos, true));
			params["length"] = Text::fromT(Util::formatSeconds(length, true));
			int numFront = min(max(intPercent / 10, 0), 10),
				numBack = min(max(10 - 1 - numFront, 0), 10);
			string inFront = string(numFront, '-'),
				inBack = string(numBack, '-');
			params["bar"] = "[" + inFront + (state ? "|" : "-") + inBack + "]";
			int waSampleRate = SendMessage(hwndWinamp,WM_USER, 0, IPC_GETINFO),
				waBitRate = SendMessage(hwndWinamp,WM_USER, 1, IPC_GETINFO),
				waChannels = SendMessage(hwndWinamp,WM_USER, 2, IPC_GETINFO);
			params["bitrate"] = Util::toString(waBitRate) + "kbps";
			params["sample"] = Util::toString(waSampleRate) + "kHz";
			// later it should get some improvement:
			string waChannelName;
			switch (waChannels) {
				case 2:
					waChannelName = "stereo";
					break;
				case 6:
					waChannelName = "5.1 surround";
					break;
				default:
					waChannelName = "mono";
			}
			params["channels"] = waChannelName;
			//params["channels"] = (waChannels==2?"stereo":"mono"); // 3+ channels? 0 channels?
			message = Text::toT(Util::formatParams(SETTING(WINAMP_FORMAT), params, false));
			thirdPerson = strnicmp(message.c_str(), _T("/me "), 4) == 0;
			if(thirdPerson) {
				message = message.substr(4);
			}
		} else {
			status = _T("Supported version of Winamp is not running");
		}
	} else if(stricmp(cmd.c_str(), _T("tvtome")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else
			WinUtil::openLink(_T("http://www.tvtome.com/tvtome/servlet/Search?searchType=all&searchString=") + Text::toT(Util::encodeURI(Text::fromT(param))));
	} else if(stricmp(cmd.c_str(), _T("csfd")) == 0) {
		if(param.empty()) {
			status = TSTRING(SPECIFY_SEARCH_STRING);
		} else {
			WinUtil::openLink(_T("http://www.csfd.cz/search.php?search=") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else {
		return false;
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

tstring WinUtil::getMagnet(const TTHValue& aHash, const string& aFile, int64_t aSize) {
	return _T("magnet:?xt=urn:tree:tiger:") + Text::toT(aHash.toBase32()) + _T("&xl=") + Util::toStringW(aSize) + _T("&dn=") + Text::toT(Util::encodeURI(aFile));
}

void WinUtil::copyMagnet(const TTHValue& aHash, const string& aFile, int64_t aSize) {
	if(!aFile.empty()) {
		setClipboard(getMagnet(aHash, aFile, aSize));
 	}
}

 void WinUtil::searchHash(const TTHValue& aHash) {
	SearchFrame::openWindow(Text::toT(aHash.toBase32()), 0, SearchManager::SIZE_DONTCARE, SearchManager::TYPE_TTH);
 }

 void WinUtil::registerDchubHandler() {
	HKEY hk;
	TCHAR Buf[512];
	tstring app = _T("\"") + Text::toT(getAppName()) + _T("\" %1");
	Buf[0] = 0;

	if(::RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\dchub\\Shell\\Open\\Command"), 0, KEY_WRITE | KEY_READ, &hk) == ERROR_SUCCESS) {
		DWORD bufLen = sizeof(Buf);
		DWORD type;
		::RegQueryValueEx(hk, NULL, 0, &type, (LPBYTE)Buf, &bufLen);
		::RegCloseKey(hk);
	}

	if(stricmp(app.c_str(), Buf) != 0) {
		if (::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\dchub"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL))  {
			LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_DCHUB));
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
		app = Text::toT(getAppName());
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
	 tstring app = _T("\"") + Text::toT(getAppName()) + _T("\" %1");
	 Buf[0] = 0;

	 if(::RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adc\\Shell\\Open\\Command"), 0, KEY_WRITE | KEY_READ, &hk) == ERROR_SUCCESS) {
		 DWORD bufLen = sizeof(Buf);
		 DWORD type;
		 ::RegQueryValueEx(hk, NULL, 0, &type, (LPBYTE)Buf, &bufLen);
		 ::RegCloseKey(hk);
	 }

	 if(stricmp(app.c_str(), Buf) != 0) {
		 if (::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\adc"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL))  {
			 LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_ADC));
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
		 app = Text::toT(getAppName());
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
	tstring app = _T("\"") + Text::toT(getAppName()) + _T("\" %1");
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
			 LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_ADC));
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
		 app = Text::toT(getAppName());
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
	tstring appName = Text::toT(getAppName());
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
	if(BOOLSETTING(MAGNET_REGISTER) && (strnicmp(openCmd, appName, appName.size()) != 0)) {
		SHDeleteKey(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\magnet"));
		if (::RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Classes\\magnet"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL))  {
			LogManager::getInstance()->message(STRING(ERROR_CREATING_REGISTRY_KEY_MAGNET));
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
	if(_strnicmp(Text::fromT(url).c_str(), "magnet:?", 8) == 0) {
		parseMagnetUri(url);
		return;
	}
	if(_strnicmp(Text::fromT(url).c_str(), "dchub://", 8) == 0) {
		parseDchubUrl(url, false);
		return;
	}
	if(_strnicmp(Text::fromT(url).c_str(), "nmdcs://", 8) == 0) {
		parseDchubUrl(url, true);
		return;
	}	
	if(_strnicmp(Text::fromT(url).c_str(), "adc://", 6) == 0) {
		parseADChubUrl(url, false);
		return;
	}
	if(_strnicmp(Text::fromT(url).c_str(), "adcs://", 7) == 0) {
		parseADChubUrl(url, true);
		return;
	}

	boost::wregex reg;
	reg.assign(_T("(?=\\S*[A-Z]\\S*)(([A-Z0-9]|\\w[A-Z0-9])[A-Za-z0-9-]*)(\\.|_|(-(?=\\S*\\d{4}\\S+)))(\\S+)-(\\w{2,})"));
	if(regex_match(url, reg)) {
		WinUtil::search(url, 0, false);
	}

	::ShellExecute(NULL, NULL, url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void WinUtil::parseDchubUrl(const tstring& aUrl, bool secure) {
	uint16_t port = 411;
	string proto, host, file, query, fragment;
	Util::decodeUrl(Text::fromT(aUrl), proto, host, port, file, query, fragment);
	string url = host + ":" + Util::toString(port);
	if(!host.empty()) {
		HubFrame::openWindow((secure ? _T("nmdcs://") : _T("")) + Text::toT(host) + _T(":") + Util::toStringW(port));
	}
	if(!file.empty()) {
		if(file[0] == '/') // Remove any '/' in from of the file
			file = file.substr(1);
		try {
			if(!file.empty()) {
				UserPtr user = ClientManager::getInstance()->findLegacyUser(file);
				if(user)
					QueueManager::getInstance()->addList(HintedUser(user, url), QueueItem::FLAG_CLIENT_VIEW);
			}
			// @todo else report error
		} catch(const Exception&) {
			// ...
		}
	}
}

void WinUtil::parseADChubUrl(const tstring& aUrl, bool secure) {
	string proto, host, file, query, fragment;
	uint16_t port = 0; //make sure we get a port since adc doesn't have a standard one
	Util::decodeUrl(Text::fromT(aUrl), proto, host, port, file, query, fragment);
	if(!host.empty() && port > 0) {
		HubFrame::openWindow((secure ? _T("adcs://") : _T("adc://")) + Text::toT(host) + _T(":") + Util::toStringW(port));
	}
}

void WinUtil::SetIcon(HWND hWnd, long icon, bool big) {
	

	HICON hIconSm = (HICON)LoadImage((HINSTANCE)::GetWindowLong(hWnd, GWLP_HINSTANCE), MAKEINTRESOURCE(icon), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);
	::SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);

	if(big){
		HICON hIcon   = (HICON)LoadImage((HINSTANCE)::GetWindowLong(hWnd, GWLP_HINSTANCE), MAKEINTRESOURCE(icon), IMAGE_ICON, 32, 32, LR_DEFAULTSIZE);
		::SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	}
	
}
void WinUtil::SetIcon(HWND hWnd, tstring file, bool big) {
	tstring path = getIconPath(file);
	HICON hIconSm = (HICON)::LoadImage(NULL, path.c_str(), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_LOADFROMFILE | LR_SHARED);
	::SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);

	if(big){
		HICON hIcon   = (HICON)::LoadImage(NULL, path.c_str(), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR | LR_LOADFROMFILE | LR_SHARED);
		::SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	}
	
}


void WinUtil::parseMagnetUri(const tstring& aUrl, bool /*aOverride*/) {
	// official types that are of interest to us
	//  xt = exact topic
	//  xs = exact substitute
	//  as = acceptable substitute
	//  dn = display name
	//  xl = exact length
	if (strnicmp(aUrl.c_str(), _T("magnet:?"), 8) == 0) {
		LogManager::getInstance()->message(STRING(MAGNET_DLG_TITLE) + ": " + Text::fromT(aUrl));
		StringTokenizer<tstring> mag(aUrl.substr(8), _T('&'));
		typedef map<tstring, tstring> MagMap;
		MagMap hashes;
		tstring fname, fhash, type, param;
		int64_t fsize = 0;
		for(TStringList::const_iterator idx = mag.getTokens().begin(); idx != mag.getTokens().end(); ++idx) {
			// break into pairs
			string::size_type pos = idx->find(_T('='));
			if(pos != string::npos) {
				type = Text::toT(Text::toLower(Util::encodeURI(Text::fromT(idx->substr(0, pos)), true)));
				param = Text::toT(Util::encodeURI(Text::fromT(idx->substr(pos+1)), true));
			} else {
				type = Text::toT(Util::encodeURI(Text::fromT(*idx), true));
				param.clear();
			}
			// extract what is of value
			if(param.length() == 85 && strnicmp(param.c_str(), _T("urn:bitprint:"), 13) == 0) {
				hashes[type] = param.substr(46);
			} else if(param.length() == 54 && strnicmp(param.c_str(), _T("urn:tree:tiger:"), 15) == 0) {
				hashes[type] = param.substr(15);
			} else if(param.length() == 55 && strnicmp(param.c_str(), _T("urn:tree:tiger/:"), 16) == 0) {
				hashes[type] = param.substr(16);
			} else if(param.length() == 59 && strnicmp(param.c_str(), _T("urn:tree:tiger/1024:"), 20) == 0) {
				hashes[type] = param.substr(20);
			} else if(type.length() == 2 && strnicmp(type.c_str(), _T("dn"), 2) == 0) {
				fname = param;
			} else if(type.length() == 2 && strnicmp(type.c_str(), _T("xl"), 2) == 0) {
				fsize = _tstoi64(param.c_str());
			}
		}
		// pick the most authoritative hash out of all of them.
		if(hashes.find(_T("as")) != hashes.end()) {
			fhash = hashes[_T("as")];
		}
		if(hashes.find(_T("xs")) != hashes.end()) {
			fhash = hashes[_T("xs")];
		}
		if(hashes.find(_T("xt")) != hashes.end()) {
			fhash = hashes[_T("xt")];
		}
		if(!fhash.empty() && Encoder::isBase32(Text::fromT(fhash).c_str())){
			// ok, we have a hash, and maybe a filename.
			if(!BOOLSETTING(MAGNET_ASK) && fsize > 0 && fname.length() > 0) {
				switch(SETTING(MAGNET_ACTION)) {
					case SettingsManager::MAGNET_AUTO_DOWNLOAD:
						try {
							QueueManager::getInstance()->add(SETTING(DOWNLOAD_DIRECTORY) + Text::fromT(fname), fsize, TTHValue(Text::fromT(fhash)), HintedUser(UserPtr(), Util::emptyString));
						} catch(const Exception& e) {
							LogManager::getInstance()->message(e.getError());
						}
						break;
					case SettingsManager::MAGNET_AUTO_SEARCH:
						SearchFrame::openWindow(fhash, 0, SearchManager::SIZE_DONTCARE, SearchManager::TYPE_TTH);
						break;
				};
			} else {
			// use aOverride to force the display of the dialog.  used for auto-updating
				MagnetDlg dlg(fhash, fname, fsize);
				dlg.DoModal(mainWnd);
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
bool WinUtil::parseDBLClick(const tstring& aString, string::size_type start, string::size_type end) {
	if( (strnicmp(aString.c_str() + start, _T("http://"), 7) == 0) || 
		(strnicmp(aString.c_str() + start, _T("www."), 4) == 0) ||
		(strnicmp(aString.c_str() + start, _T("ftp://"), 6) == 0) ||
		(strnicmp(aString.c_str() + start, _T("irc://"), 6) == 0) ||
		(strnicmp(aString.c_str() + start, _T("https://"), 8) == 0) ||	
		(strnicmp(aString.c_str() + start, _T("file://"), 7) == 0) ||
		(strnicmp(aString.c_str() + start, _T("mailto:"), 7) == 0) )
	{

		openLink(aString.substr(start, end-start));
		return true;
	} else if(strnicmp(aString.c_str() + start, _T("dchub://"), 8) == 0) {
		parseDchubUrl(aString.substr(start, end-start), false);
		return true;
	} else if(strnicmp(aString.c_str() + start, _T("nmdcs://"), 8) == 0) {
		parseDchubUrl(aString.substr(start, end-start), true);
		return true;	
	} else if(strnicmp(aString.c_str() + start, _T("magnet:?"), 8) == 0) {
		parseMagnetUri(aString.substr(start, end-start));
		return true;
	} else if(strnicmp(aString.c_str() + start, _T("adc://"), 6) == 0) {
		parseADChubUrl(aString.substr(start, end-start), false);
		return true;
	} else if(strnicmp(aString.c_str() + start, _T("adcs://"), 7) == 0) {
		parseADChubUrl(aString.substr(start, end-start), true);
		return true;
	}
	return false;
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

int WinUtil::getIconIndex(const tstring& aFileName) {
	if(BOOLSETTING(USE_SYSTEM_ICONS)) {
		SHFILEINFO fi;
		tstring x = Text::toLower(Util::getFileExt(aFileName));
		if(!x.empty()) {
			ImageIter j = fileIndexes.find(x);
			if(j != fileIndexes.end())
				return j->second;
		}
		tstring fn = Text::toLower(Util::getFileName(aFileName));
		::SHGetFileInfo(fn.c_str(), FILE_ATTRIBUTE_NORMAL, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		fileImages.AddIcon(fi.hIcon);
		::DestroyIcon(fi.hIcon);

		fileIndexes[x] = fileImageCount++;
		return fileImageCount - 1;
	} else {
		return 2;
	}
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

int WinUtil::getOsMajor() {
	OSVERSIONINFOEX ver;
	memzero(&ver, sizeof(OSVERSIONINFOEX));
	if(!GetVersionEx((OSVERSIONINFO*)&ver)) 
	{
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	}
	GetVersionEx((OSVERSIONINFO*)&ver);
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	return ver.dwMajorVersion;
}

int WinUtil::getOsMinor() 
{
	OSVERSIONINFOEX ver;
	memzero(&ver, sizeof(OSVERSIONINFOEX));
	if(!GetVersionEx((OSVERSIONINFO*)&ver)) 
	{
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	}
	GetVersionEx((OSVERSIONINFO*)&ver);
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	return ver.dwMinorVersion;
}

tstring WinUtil::getNicks(const CID& cid, const string& hintUrl) {
	return Text::toT(Util::toString(ClientManager::getInstance()->getNicks(cid, hintUrl)));
}

tstring WinUtil::getNicks(const UserPtr& u, const string& hintUrl) {
	return getNicks(u->getCID(), hintUrl);
}

tstring WinUtil::getNicks(const CID& cid, const string& hintUrl, bool priv) {
	return Text::toT(Util::toString(ClientManager::getInstance()->getNicks(cid, hintUrl, priv)));
}

static pair<tstring, bool> formatHubNames(const StringList& hubs) {
	if(hubs.empty()) {
		return make_pair(CTSTRING(OFFLINE), false);
	} else {
		return make_pair(Text::toT(Util::toString(hubs)), true);
	}
}

pair<tstring, bool> WinUtil::getHubNames(const CID& cid, const string& hintUrl) {
	return formatHubNames(ClientManager::getInstance()->getHubNames(cid, hintUrl));
}

pair<tstring, bool> WinUtil::getHubNames(const UserPtr& u, const string& hintUrl) {
	return getHubNames(u->getCID(), hintUrl);
}

pair<tstring, bool> WinUtil::getHubNames(const CID& cid, const string& hintUrl, bool priv) {
	return formatHubNames(ClientManager::getInstance()->getHubNames(cid, hintUrl, priv));
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
	if(!Util::fileExists(Text::fromT(file)))
		return;

	::ShellExecute(NULL, Text::toT("explore").c_str(), Text::toT(Util::FormatPath(Util::getFilePath(Text::fromT(file)))).c_str(), NULL, NULL, SW_SHOWNORMAL);
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

int WinUtil::SetupPreviewMenu(CMenu &previewMenu, string extension){
	int PreviewAppsSize = 0;
	PreviewApplication::List lst = FavoriteManager::getInstance()->getPreviewApps();
	if(lst.size()>0){		
		PreviewAppsSize = 0;
		for(PreviewApplication::Iter i = lst.begin(); i != lst.end(); i++){
			StringList tok = StringTokenizer<string>((*i)->getExtension(), ';').getTokens();
			bool add = false;
			if(tok.size()==0)add = true;

			
			for(StringIter si = tok.begin(); si != tok.end(); ++si) {
				if(_stricmp(extension.c_str(), si->c_str())==0){
					add = true;
					break;
				}
			}
							
			if (add) previewMenu.AppendMenu(MF_STRING, IDC_PREVIEW_APP + PreviewAppsSize, Text::toT(((*i)->getName())).c_str());
			PreviewAppsSize++;
		}
	}
	return PreviewAppsSize;
}

void WinUtil::RunPreviewCommand(unsigned int index, const string& target) {
	PreviewApplication::List lst = FavoriteManager::getInstance()->getPreviewApps();

	if(index <= lst.size()) {
		string application = lst[index]->getApplication();
		string arguments = lst[index]->getArguments();
		StringMap ucParams;				
	
		ucParams["file"] = "\"" + target + "\"";
		ucParams["dir"] = "\"" + Util::getFilePath(target) + "\"";

		::ShellExecute(NULL, NULL, Text::toT(application).c_str(), Text::toT(Util::formatParams(arguments, ucParams, false)).c_str(), Text::toT(ucParams["dir"]).c_str(), SW_SHOWNORMAL);
	}
}

tstring WinUtil::UselessInfo() {
	tstring result = _T("\n");
	TCHAR buf[255];
	
	MEMORYSTATUSEX mem;
	mem.dwLength = sizeof(MEMORYSTATUSEX);
	if( GlobalMemoryStatusEx(&mem) != 0){
		result += _T("Memory\n");
		result += _T("Physical memory (available/total): ");
		result += Util::formatBytesW( mem.ullAvailPhys ) + _T("/") + Util::formatBytesW( mem.ullTotalPhys );
		result += _T("\n");
		result += _T("Pagefile (available/total): ");
		result += Util::formatBytesW( mem.ullAvailPageFile ) + _T("/") + Util::formatBytesW( mem.ullTotalPageFile );
		result += _T("\n");
		result += _T("Virtual memory (available/total): ");
		result += Util::formatBytesW( mem.ullAvailVirtual ) + _T("/") + Util::formatBytesW( mem.ullTotalVirtual );
		result += _T("\n");
		result += _T("Memory load: ");
		result += Util::toStringW(mem.dwMemoryLoad);
		result += _T("%\n\n");
	}

	CRegKey key;
	ULONG len = 255;

	if(key.Open( HKEY_LOCAL_MACHINE, _T("Hardware\\Description\\System\\CentralProcessor\\0"), KEY_READ) == ERROR_SUCCESS) {
		result += _T("CPU\n");
        if(key.QueryStringValue(_T("ProcessorNameString"), buf, &len) == ERROR_SUCCESS){
			result += _T("Name: ");
			tstring tmp = buf;
            result += tmp.substr( tmp.find_first_not_of(_T(" ")) );
			result += _T("\n");
		}
		DWORD speed;
		if(key.QueryDWORDValue(_T("~MHz"), speed) == ERROR_SUCCESS){
			result += _T("Speed: ");
			result += Util::toStringW(speed);
			result += _T("MHz\n");
		}
		len = 255;
		if(key.QueryStringValue(_T("Identifier"), buf, &len) == ERROR_SUCCESS) {
			result += _T("Identifier: ");
			result += buf;
		}
		result += _T("\n\n");
	}

	result += _T("OS\n");
	result += Text::toT(Util::getOsVersion());
	result += _T("\n");

	result += _T("\nDisk\n");
	result += _T("Disk space(free/total): ");
	result += DiskSpaceInfo(true);
	result += _T("\n\nTransfer\n");
	result += Text::toT("Upload: " + Util::formatBytes(SETTING(TOTAL_UPLOAD)) + ", Download: " + Util::formatBytes(SETTING(TOTAL_DOWNLOAD)));
	
	if(SETTING(TOTAL_DOWNLOAD) > 0) {
		_stprintf(buf, _T("Ratio (up/down): %.2f"), ((double)SETTING(TOTAL_UPLOAD)) / ((double)SETTING(TOTAL_DOWNLOAD)));
		result += _T('\n');
		result += buf;
	}

	return result;
}

tstring WinUtil::DiskSpaceInfo(bool onlyTotal /* = false */) {
	tstring ret = Util::emptyStringT;
	//support XP and higher
	if((LOBYTE(LOWORD(GetVersion())) >= 5 && WinUtil::getOsMinor() >= 1) 
		|| (WinUtil::getOsMajor() >= 6)) {

	int64_t free = 0, totalFree = 0, size = 0, totalSize = 0, netFree = 0, netSize = 0;

   TCHAR   buf[MAX_PATH];  
   HANDLE  hVol;    
   BOOL    found;

   // lookup drive volumes.
   hVol = FindFirstVolume(buf, MAX_PATH);

   if(hVol != INVALID_HANDLE_VALUE)
   {

   found = FindVolume(hVol, buf, MAX_PATH);

   //while we find drive volumes.
   while(found) { 
	   found = FindVolume(hVol, buf, MAX_PATH); 
   }

   found = FindVolumeClose(hVol);
   }

   for(TStringIter i = volumes.begin(); i != volumes.end(); i++) {
	   if(GetDriveType((*i).c_str()) == DRIVE_CDROM || GetDriveType((*i).c_str()) == DRIVE_REMOVABLE)
		   continue;
	   if(GetDiskFreeSpaceEx((*i).c_str(), NULL, (PULARGE_INTEGER)&size, (PULARGE_INTEGER)&free)){
				totalFree += free;
				totalSize += size;
			}
   }

   //check for mounted Network drives
   ULONG drives = _getdrives();
   TCHAR drive[3] = { _T('A'), _T(':'), _T('\0') };
   
	while(drives != 0) {
		if(drives & 1 && ( GetDriveType(drive) != DRIVE_CDROM && GetDriveType(drive) != DRIVE_REMOVABLE && GetDriveType(drive) == DRIVE_REMOTE) ){
			if(GetDiskFreeSpaceEx(drive, NULL, (PULARGE_INTEGER)&size, (PULARGE_INTEGER)&free)){
				netFree += free;
				netSize += size;
			}
		}

		++drive[0];
		drives = (drives >> 1);
	}
   

	
	if(totalSize != 0)
		if( !onlyTotal ) {
			ret += _T("\r\n Local HDD space (free/total): ") + Util::formatBytesW(totalFree) + _T("/") + Util::formatBytesW(totalSize);
		if(netSize != 0) {
				ret +=  _T("\r\n Network HDD space (free/total): ") + Util::formatBytesW(netFree) + _T("/") + Util::formatBytesW(netSize);
				ret +=  _T("\r\n Total HDD space (free/total): ") + Util::formatBytesW((netFree+totalFree)) + _T("/") + Util::formatBytesW(netSize+totalSize);
		}
		}		
		else
			ret += Util::formatBytesW(totalFree) + _T("/") + Util::formatBytesW(totalSize);
		

	volumes.clear();
	}else{
		ret+=_T("Not Supported by OS");
	}
	return ret;
}

BOOL WinUtil::FindVolume(HANDLE hVol, TCHAR *Buf, int bufSize) {

  BOOL  found;

  volumes.push_back(Buf);

  found = FindNextVolume( hVol, Buf, bufSize );

    return (found);
}

tstring WinUtil::diskInfo() {

	tstring result = Util::emptyStringT;
	
	if((LOBYTE(LOWORD(GetVersion())) >= 5 && WinUtil::getOsMinor() >= 1) 
		|| (WinUtil::getOsMajor() >= 6)) {

   TCHAR   buf[MAX_PATH];  
   HANDLE  hVol;    
   BOOL    found;
   TCHAR   buf2[MAX_PATH];
   int64_t free = 0, size = 0 , totalFree = 0, totalSize = 0;
   int disk_count = 0;
   
   std::vector<tstring> results; //add in vector for sorting, nicer to look at :)
   // lookup drive volumes.
   hVol = FindFirstVolume(buf, MAX_PATH);

   if(hVol != INVALID_HANDLE_VALUE)
   {
	  
   found = FindVolume(hVol, buf, MAX_PATH);

   //while we find drive volumes.
   while(found) { 
	   found = FindVolume(hVol, buf, MAX_PATH); 
   }
   
   found = FindVolumeClose(hVol);
   }

   for(TStringIter i = volumes.begin(); i != volumes.end(); i++) {
	   if(GetDriveType((*i).c_str()) == DRIVE_CDROM || GetDriveType((*i).c_str()) == DRIVE_REMOVABLE)
		   continue;
	    
	   if((GetVolumePathNamesForVolumeName((*i).c_str(), buf2,256, NULL) != 0) &&
		(GetDiskFreeSpaceEx((*i).c_str(), NULL, (PULARGE_INTEGER)&size, (PULARGE_INTEGER)&free) !=0)){
			tstring mountpath = buf2; 
			if(!mountpath.empty()) {
				totalFree += free;
				totalSize += size;
				results.push_back((_T("MountPath: ") + mountpath + _T(" Disk Space (free/total) ") + Util::formatBytesW(free) + _T("/") +  Util::formatBytesW(size)));
			}
	   }
   }
      // and a check for mounted Network drives, todo fix a better way for network space
   ULONG drives = _getdrives();
   TCHAR drive[3] = { _T('A'), _T(':'), _T('\0') };
   
	while(drives != 0) {
		if(drives & 1 && ( GetDriveType(drive) != DRIVE_CDROM && GetDriveType(drive) != DRIVE_REMOVABLE && GetDriveType(drive) == DRIVE_REMOTE) ){
			if(GetDiskFreeSpaceEx(drive, NULL, (PULARGE_INTEGER)&size, (PULARGE_INTEGER)&free)){
				totalFree += free;
				totalSize += size;
				results.push_back((_T("Network MountPath: ") + (tstring)drive + _T(" Disk Space (free/total) ") + Util::formatBytesW(free) + _T("/") +  Util::formatBytesW(size)));
			}
		}

		++drive[0];
		drives = (drives >> 1);
	}


   sort(results.begin(), results.end()); //sort it
   for(std::vector<tstring>::iterator i = results.begin(); i != results.end(); ++i) {
	   disk_count++;
	   result += _T("\r\n ") + *i; 
   }
   result +=  _T("\r\n\r\n Total HDD space (free/total): ") + Util::formatBytesW((totalFree)) + _T("/") + Util::formatBytesW(totalSize);
   result += _T("\r\n Total Drives count: ") + Text::toT(Util::toString(disk_count));
   results.clear();
   volumes.clear();
	}else{
	result += _T("Not Supported by OS");
	}
   return result;
}
string WinUtil::formatTime(uint64_t rest) {
	char buf[128];
	string formatedTime;
	uint64_t n, i;
	i = 0;
	n = rest / (24*3600*7);
	rest %= (24*3600*7);
	if(n) {
		if(n >= 2)
			snprintf(buf, sizeof(buf), "%d weeks ", n);
		else
			snprintf(buf, sizeof(buf), "%d week ", n);
		formatedTime += (string)buf;
		i++;
	}
	n = rest / (24*3600);
	rest %= (24*3600);
	if(n) {
		if(n >= 2)
			snprintf(buf, sizeof(buf), "%d days ", n); 
		else
			snprintf(buf, sizeof(buf), "%d day ", n);
		formatedTime += (string)buf;
		i++;
	}
	n = rest / (3600);
	rest %= (3600);
	if(n) {
		if(n >= 2)
			snprintf(buf, sizeof(buf), "%d hours ", n);
		else
			snprintf(buf, sizeof(buf), "%d hour ", n);
		formatedTime += (string)buf;
		i++;
	}
	n = rest / (60);
	rest %= (60);
	if(n) {
		snprintf(buf, sizeof(buf), "%d min ", n);
		formatedTime += (string)buf;
		i++;
	}
	n = rest;
	if(++i <= 3) {
		snprintf(buf, sizeof(buf),"%d sec ", n); 
		formatedTime += (string)buf;
	}
	return formatedTime;
}

uint8_t WinUtil::getFlagIndexByCode(const char* countryCode) {
	// country codes are sorted, use binary search for better performance
	int begin = 0;
	int end = (sizeof(countryCodes) / sizeof(countryCodes[0])) - 1;
		
	while(begin <= end) {
		int mid = (begin + end) / 2;
		int cmp = memcmp(countryCode, countryCodes[mid], 2);

		if(cmp > 0)
			begin = mid + 1;
		else if(cmp < 0)
			end = mid - 1;
		else
			return mid + 1;
	}
	return 0;
}

uint8_t WinUtil::getFlagIndexByName(const char* countryName) {
	// country codes are not sorted, use linear searching (it is not used so often)
	const char** first = countryNames;
	const char** last = countryNames + (sizeof(countryNames) / sizeof(countryNames[0]));
	const char** i = find_if(first, last, [&](const char* cn) { return stricmp(countryName, cn) == 0; });
	if(i != last)
		return i - countryNames + 1;

	return 0;
}
float ProcSpeedCalc() {
#ifndef _WIN64
#define RdTSC __asm _emit 0x0f __asm _emit 0x31
__int64 cyclesStart = 0, cyclesStop = 0;
unsigned __int64 nCtr = 0, nFreq = 0, nCtrStop = 0;
    if(!QueryPerformanceFrequency((LARGE_INTEGER *) &nFreq)) return 0;
    QueryPerformanceCounter((LARGE_INTEGER *) &nCtrStop);
    nCtrStop += nFreq;
    _asm {
		RdTSC
        mov DWORD PTR cyclesStart, eax
        mov DWORD PTR [cyclesStart + 4], edx
    } do {
		QueryPerformanceCounter((LARGE_INTEGER *) &nCtr);
    } while (nCtr < nCtrStop);
    _asm {
		RdTSC
        mov DWORD PTR cyclesStop, eax
        mov DWORD PTR [cyclesStop + 4], edx
    }
	return ((float)cyclesStop-(float)cyclesStart) / 1000000;
#else
	HKEY hKey;
	DWORD dwSpeed;

	// Get the key name
	wchar_t szKey[256];
	_snwprintf(szKey, sizeof(szKey)/sizeof(wchar_t),
		L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\%d\\", 0);

	// Open the key
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,szKey, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
	{
		return 0;
	}

	// Read the value
	DWORD dwLen = 4;
	if(RegQueryValueEx(hKey, L"~MHz", NULL, NULL, (LPBYTE)&dwSpeed, &dwLen) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return 0;
	}

	// Cleanup and return
	RegCloseKey(hKey);
	
	return dwSpeed;
#endif
}

wchar_t arrayutf[42] = { L'Á', L'È', L'Ï', L'É', L'Ì', L'Í', L'¼', L'Ò', L'Ó', L'Ø', L'Š', L'', L'Ú', L'Ù', L'Ý', L'Ž', L'á', L'è', L'ï', L'é', L'ì', L'í', L'¾', L'ò', L'ó', L'ø', L'š', L'', L'ú', L'ù', L'ý', L'ž', L'Ä', L'Ë', L'Ö', L'Ü', L'ä', L'ë', L'ö', L'ü', L'£', L'³' };
wchar_t arraywin[42] = { L'A', L'C', L'D', L'E', L'E', L'I', L'L', L'N', L'O', L'R', L'S', L'T', L'U', L'U', L'Y', L'Z', L'a', L'c', L'd', L'e', L'e', L'i', L'l', L'n', L'o', L'r', L's', L't', L'u', L'u', L'y', L'z', L'A', L'E', L'O', L'U', L'a', L'e', L'o', L'u', L'L', L'l' };

const tstring& WinUtil::disableCzChars(tstring& message) {
	for(size_t j = 0; j < message.length(); j++) {
		for(size_t l = 0; l < (sizeof(arrayutf) / sizeof(arrayutf[0])); l++) {
			if (message[j] == arrayutf[l]) {
				message[j] = arraywin[l];
				break;
			}
		}
	}

	return message;
}

tstring WinUtil::Speedinfo() {
	tstring result = _T("\n");

result += _T("-= ");
result += _T("Downloading: ");
result += Util::formatBytesW(DownloadManager::getInstance()->getRunningAverage()) + _T("/s  [");
result += Util::toStringW(DownloadManager::getInstance()->getDownloadCount()) + _T("]");
result += _T(" =- ");
result += _T(" -= ");
result += _T("Uploading: ");
result += Util::formatBytesW(UploadManager::getInstance()->getRunningAverage()) + _T("/s  [");
result += Util::toStringW(UploadManager::getInstance()->getUploadCount()) + _T("]");
result += _T(" =-");

return result;

}
string WinUtil::getSysUptime(){
			//apexdc
		typedef ULONGLONG (CALLBACK* LPFUNC2)(void);
		LPFUNC2 _GetTickCount64 = (LPFUNC2)GetProcAddress(LoadLibrary(_T("kernel32")), "GetTickCount64");
		time_t sysUptime = (_GetTickCount64 ? _GetTickCount64() : GetTickCount()) / 1000;

		return formatTime(sysUptime);

}


string WinUtil::generateStats() {
	if(LOBYTE(LOWORD(GetVersion())) >= 5) {
		PROCESS_MEMORY_COUNTERS pmc;
		pmc.cb = sizeof(pmc);
		typedef bool (CALLBACK* LPFUNC)(HANDLE Process, PPROCESS_MEMORY_COUNTERS ppsmemCounters, DWORD cb);
		
		LPFUNC _GetProcessMemoryInfo = (LPFUNC)GetProcAddress(LoadLibrary(_T("psapi")), "GetProcessMemoryInfo");
		_GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
		FILETIME tmpa, tmpb, kernelTimeFT, userTimeFT;
		GetProcessTimes(GetCurrentProcess(), &tmpa, &tmpb, &kernelTimeFT, &userTimeFT);
		int64_t kernelTime = kernelTimeFT.dwLowDateTime | (((int64_t)kernelTimeFT.dwHighDateTime) << 32);
		int64_t userTime = userTimeFT.dwLowDateTime | (((int64_t)userTimeFT.dwHighDateTime) << 32);  

		string ret = boost::str(boost::format(
"\r\n\t-=[ %s   http://www.airdcpp.net ]=-\r\n\
\t-=[ Uptime: %s ][ CPU time: %s ]=-\r\n\
\t-=[ Memory usage (peak): %s (%s) ]=-\r\n\
\t-=[ Virtual Memory usage (peak): %s (%s) ]=-\r\n\
\t-=[ Downloaded: %s ][  Uploaded %s ]=-\r\n\
\t-=[ Total download %s ][ Total upload %s ]=-\r\n\
\t-=[ System: %s (Uptime: %s) ]=-\r\n\
\t-=[ CPU: %s ]=-")

			% Text::fromT(COMPLETEVERSIONSTRING)
			% formatTime(Util::getUptime())
			% Text::fromT(Util::formatSeconds((kernelTime + userTime) / (10I64 * 1000I64 * 1000I64)))
			% Util::formatBytes(pmc.WorkingSetSize)
			% Util::formatBytes(pmc.PeakWorkingSetSize)
			% Util::formatBytes(pmc.PagefileUsage)
			% Util::formatBytes(pmc.PeakPagefileUsage)
			% Util::formatBytes(Socket::getTotalDown())
			% Util::formatBytes(Socket::getTotalUp())
			% Util::formatBytes(SETTING(TOTAL_DOWNLOAD))
			% Util::formatBytes(SETTING(TOTAL_UPLOAD))
			% Util::getOsVersion()
			% getSysUptime()
			% CPUInfo());
		return ret;
	} else {
		return "Not supported by OS";
	}

}


string WinUtil::CPUInfo() {
	tstring result = _T("");

	CRegKey key;
	ULONG len = 255;

	if(key.Open( HKEY_LOCAL_MACHINE, _T("Hardware\\Description\\System\\CentralProcessor\\0"), KEY_READ) == ERROR_SUCCESS) {
		TCHAR buf[255];
		if(key.QueryStringValue(_T("ProcessorNameString"), buf, &len) == ERROR_SUCCESS){
			tstring tmp = buf;
			result = tmp.substr( tmp.find_first_not_of(_T(" ")) );
		}
		DWORD speed;
		if(key.QueryDWORDValue(_T("~MHz"), speed) == ERROR_SUCCESS){
			result += _T(" (");
			result += Util::toStringW((uint32_t)speed);
			result += _T(" MHz)");
		}
	}
	return result.empty() ? "Unknown" : Text::fromT(result);
}

string WinUtil::uptimeInfo() {
	if(LOBYTE(LOWORD(GetVersion())) >= 5) {
		char buf[512]; 
		snprintf(buf, sizeof(buf), "\n-=[ Uptime: %s]=-\r\n-=[ System Uptime: %s]=-\r\n", 
		formatTime(Util::getUptime()).c_str(), 
		getSysUptime().c_str());
		return buf;
	} else {
		return "Not supported by OS";
	}
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
			if(LOBYTE(LOWORD(GetVersion())) >= 5) {
				typedef bool (CALLBACK* LPLockWorkStation)(void);
				LPLockWorkStation _d_LockWorkStation = (LPLockWorkStation)GetProcAddress(LoadLibrary(_T("user32")), "LockWorkStation");
				_d_LockWorkStation();
			}
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

string WinUtil::getItunesSpam(HWND playerWnd /*= NULL*/) {
	// If it's not running don't even bother...
	if(playerWnd != NULL) {
		// Pointer
		IiTunes *iITunes;
	
		// Load COM library
		CoInitialize(NULL);

		// Others
		StringMap params;

		// note - CLSID_iTunesApp and IID_IiTunes are defined in iTunesCOMInterface_i.c
		//Create an instance of the top-level object.  iITunes is an interface pointer to IiTunes.  (weird capitalization, but that's how Apple did it)
		if (SUCCEEDED(::CoCreateInstance(CLSID_iTunesApp, NULL, CLSCTX_LOCAL_SERVER, IID_IiTunes, (PVOID *)&iITunes))) {
			long length(0), elapsed;

			//pTrack is a pointer to the track.  This gets passed to other functions to get track data.  wasTrack lets you check if the track was grabbed.
			IITTrack *pTrack;
			//Sanity check -- should never fail if CoCreateInstance succeeded.  You may want to use this for debug output if it does ever fail.
			if (SUCCEEDED(iITunes->get_CurrentTrack(&pTrack)) && pTrack != NULL) {
				//Get album, then call ::COLE2T() to convert the text to array
				BSTR album;
				pTrack->get_Album(&album);
				if (album != NULL) {
					::COLE2T Album(album);
					params["album"] = Text::fromT(Album.m_szBuffer);
				}

				//Same for artist
				BSTR artist;
				pTrack->get_Artist(&artist);
				if(artist != NULL) {
					::COLE2T Artist(artist);
					params["artist"] = Text::fromT(Artist.m_szBuffer);
				}

				//Track name (get_Name is inherited from IITObject, of which IITTrack is derived)
				BSTR name;
				pTrack->get_Name(&name);
				if(name != NULL) {
					::COLE2T Name(name);
					params["title"] = Text::fromT(Name.m_szBuffer);
				}

				// Genre
				BSTR genre;
				pTrack->get_Genre(&genre);
				if(genre != NULL) {
					::COLE2T Genre(name);
					params["genre"] = Text::fromT(Genre.m_szBuffer);
				}

				//Total song time
				pTrack->get_Duration(&length);
				if (length > 0) { params["length"] = Text::fromT(Util::formatSecondsW(length)); } // <--- once more with feeling

				//Bitrate
				long bitrate;
				pTrack->get_BitRate(&bitrate);
				if (bitrate > 0) { params["bitrate"] = Util::toString(bitrate) + "kbps"; } //<--- I'm not gonna play those games.  Mind games.  Board games.  I'm like, come on fhqugads...

				//Frequency
				long frequency;
				pTrack->get_SampleRate(&frequency);
				if (frequency > 0) { params["frequency"] = Util::toString(frequency/1000) + "kHz"; }

				//Year
				long year;
				pTrack->get_Year(&year);
				if (year > 0) { params["year"] = Util::toString(year); }
			
				//Size
				long size;
				pTrack->get_Size(&size);
				if (size > 0) { params["size"] = Util::formatBytes(size); }

				//Release (decrement reference count to 0) track object so it can unload and free itself; otherwise, it's locked in memory.
				pTrack->Release();
			}

			//Player status (stopped, playing, FF, rewind)
			int state(0);
			ITPlayerState pStatus;
			iITunes->get_PlayerState(&pStatus);
			if (pStatus == ITPlayerStateStopped) {
				params["state"] = "stopped";
				state = 1;
			} else if (pStatus == ITPlayerStatePlaying) {
				params["state"] = "playing";
			}

			//Player position (in seconds, you'll want to convert for your output)
			iITunes->get_PlayerPosition(&elapsed);
			if(elapsed > 0) {
				params["elapsed"] = Text::fromT(Util::formatSecondsW(elapsed));
				int intPercent;
				if (length > 0 ) {
					intPercent = elapsed * 100 / length;
				} else {
					length = 0;
					intPercent = 0;
				}
				params["percent"] = Util::toString(intPercent) + "%";
				int numFront = min(max(intPercent / 10, 0), 10),
					numBack = min(max(10 - 1 - numFront, 0), 10);
				string inFront = string(numFront, '-'),
				   inBack = string(numBack, '-');
				params["bar"] = "[" + inFront + (elapsed > 0 ? "|" : "-") + inBack + "]";
			}

			//iTunes version
			BSTR version;
			iITunes->get_Version(&version);
			if(version != NULL) {
				::COLE2T iVersion(version);
				params["version"] = Text::fromT(iVersion.m_szBuffer);
			}

			//Release (decrement reference counter to 0) IiTunes object so it can unload and free itself; otherwise, it's locked in memory
			iITunes->Release();
		}

		//unload COM library -- this is also essential to prevent leaks and to keep it working the next time.
		CoUninitialize();

		// If there is something in title, we have at least partly succeeded 
		if(!params["title"].empty()) {
			return Util::formatParams(SETTING(ITUNES_FORMAT), params, false);
		} else {
			return "no_media";
		}
	} else {
		return "";
	}
}

// mpc = mplayerc = MediaPlayer Classic
// liberally inspired (copied with changes) by the GPLed application mpcinfo by Gabest
string WinUtil::getMPCSpam() {
	StringMap params;
	bool success = false;
	CComPtr<IRunningObjectTable> pROT;
	CComPtr<IEnumMoniker> pEM;
	CComQIPtr<IFilterGraph> pFG;
	if(GetRunningObjectTable(0, &pROT) == S_OK && pROT->EnumRunning(&pEM) == S_OK) {
		CComPtr<IBindCtx> pBindCtx;
		CreateBindCtx(0, &pBindCtx);
		for(CComPtr<IMoniker> pMoniker; pEM->Next(1, &pMoniker, NULL) == S_OK; pMoniker = NULL) {
			LPOLESTR pDispName = NULL;
			if(pMoniker->GetDisplayName(pBindCtx, NULL, &pDispName) != S_OK)
				continue;
			wstring strw(pDispName);
			CComPtr<IMalloc> pMalloc;
			if(CoGetMalloc(1, &pMalloc) != S_OK)
				continue;
			pMalloc->Free(pDispName);
			// Prefix string literals with the L character to indicate a UNCODE string.
			if(strw.find(L"(MPC)") == wstring::npos)
				continue;
			CComPtr<IUnknown> pUnk;
			if(pROT->GetObject(pMoniker, &pUnk) != S_OK)
				continue;
			pFG = pUnk;
			if(!pFG)
				continue;
			success = true;
			break;
		}

		if (success) {
			// file routine (contains size routine)
			CComPtr<IEnumFilters> pEF;
			if(pFG->EnumFilters(&pEF) == S_OK) {
				// from the file routine
				ULONG cFetched = 0;
				for(CComPtr<IBaseFilter> pBF; pEF->Next(1, &pBF, &cFetched) == S_OK; pBF = NULL) {
					if(CComQIPtr<IFileSourceFilter> pFSF = pBF) {
						LPOLESTR pFileName = NULL;
						AM_MEDIA_TYPE mt;
						if(pFSF->GetCurFile(&pFileName, &mt) == S_OK) {
							// second parameter is a AM_MEDIA_TYPE structure, which contains codec info
							//										LPSTR thisIsBad = (LPSTR) pFileName;
							LPSTR test = new char[1000];
							if (test != NULL) {
								WideCharToMultiByte(CP_ACP, 0, pFileName, -1, test, 1000, NULL, NULL);
								string filename(test);
								params["filename"] = Util::getFileName(filename); //otherwise fully qualified
								params["title"] = params["filename"].substr(0, params["filename"].size() - 4);
								params["size"] = Util::formatBytes(File::getSize(filename));
							}
							delete test;
							CoTaskMemFree(pFileName);
							// alternative to FreeMediaType(mt)
							// provided by MSDN DirectX 9 help page for FreeMediaType
							if (mt.cbFormat != 0)
							{
								CoTaskMemFree((PVOID)mt.pbFormat);
								mt.cbFormat = 0;
								mt.pbFormat = NULL;
							}
							if (mt.pUnk != NULL)
							{
								// Unecessary because pUnk should not be used, but safest.
								mt.pUnk->Release();
								mt.pUnk = NULL;
							}
							// end provided by MSDN
							break;
						}
					}
				}
			}

			// paused / stopped / running?
			CComQIPtr<IMediaControl> pMC;
			OAFilterState fs;
			int state = 0;
			if((pMC = pFG) && (pMC->GetState(0, &fs) == S_OK)) {
				switch(fs) {
					case State_Running:
						params["state"] = "playing";
						state = 1;
						break;
					case State_Paused:
						params["state"] = "paused";
						state = 3;
						break;
					case State_Stopped:
						params["state"] = "stopped";
						state = 0;
				};
			}

			// position routine
			CComQIPtr<IMediaSeeking> pMS = pFG;
			REFERENCE_TIME pos, dur;
			if((pMS->GetCurrentPosition(&pos) == S_OK) && (pMS->GetDuration(&dur) == S_OK)) {
				params["elapsed"] =  Text::fromT(Util::formatSecondsW(pos/10000000));
				params["length"] =  Text::fromT(Util::formatSecondsW(dur/10000000));
				int intPercent = 0;
				if (dur != 0)
					intPercent = (int) (pos * 100 / dur);
				params["percent"] = Util::toString(intPercent) + "%";
				int numFront = min(max(intPercent / 10, 0), 10),
					numBack = min(max(10 - 1 - numFront, 0), 10);
				string inFront = string(numFront, '-'),
					   inBack = string(numBack, '-');
				params["bar"] = "[" + inFront + (state ? "|" : "-") + inBack + "]";
			}
			/*
			*	"+me watches: %[filename] (%[elapsed]/%[length]) Size: %[size]"
			*	from mpcinfo.txt
			*	"+me is playing %[filename] %[size] (%[elapsed]/%[length]) Media Player Classic %[version]"
			*	from http://www.faireal.net/articles/6/25/
			*/
		}
	}

	if (success) {
		return Util::formatParams(SETTING(MPLAYERC_FORMAT), params, false);
	} else {
		 return "";
	}
}

string WinUtil::getSpotifySpam(HWND playerWnd /*= NULL*/) {
	if(playerWnd != NULL) {
		StringMap params;
			TCHAR titleBuffer[2048];
			int buffLength = sizeof(titleBuffer);
			GetWindowText(playerWnd, titleBuffer, buffLength);
			tstring title = titleBuffer;
			if (strcmpi(Text::fromT(title).c_str(), "Spotify") == 0)
				return "no_media";
			boost::algorithm::replace_first(title, "Spotify - ", "");
			params["title"] = Text::fromT(title);

			TCHAR appDataPath[MAX_PATH];
			::SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataPath);
			string line;
			boost::regex ritem;
			boost::regex rname;
			boost::regex rhash;

			string path = Text::fromT(appDataPath) + "\\Spotify\\Users\\";
			ifstream guistate;
			StringList userFiles;
			StringIterC i;

			ritem.assign("\"name\":\"([^\"]+)\",\"uri\":\"([^\"]+)");
			rname.assign("(?<=\"name\":\")([^\"]+)");
			rhash.assign("(?<=spotify:track:)([^\"]+)");
			params["link"] = "";

			StringList usersList = File::findFiles(path, "*");

			//find the file containing the latest songs played
			for(i = usersList.begin(); i != usersList.end(); ++i) {
				userFiles = File::findFiles(*i, "guistate");
				for(StringIterC s = userFiles.begin(); s != userFiles.end(); ++s) {
					guistate.open(Text::utf8ToAcp(*s));
					while (getline(guistate, line)) {
						boost::smatch result;
						string::const_iterator  begin = line.begin(), end = line.end();
						//find each song
						while (regex_search(begin, end, result, ritem)) {
							std::string item( result[0].first, result[0].second );
							boost::smatch nresult;
							//compare the name with the song being played now
							if (regex_search(item, nresult, rname)) {
								std::string song( nresult[0].first, nresult[0].second );
								size_t found = Text::fromT(title).find(song);
								if (found != string::npos) {
									//current song found, get the hash
									boost::smatch hresult;
									if (regex_search(item, hresult, rhash)) {
										std::string hash( hresult[0].first, hresult[0].second );
										params["link"] = "spotify:track:" + hash;
										break;
									}
								}
							}
							begin = result[0].second;
						}
					}
				}
			}

			return Util::formatParams(SETTING(SPOTIFY_FORMAT), params, false);
	} else {
		return "";
	}
}

// This works for WMP 9+, but it's little slow, but hey we're talking about an MS app here, so what can you expect from the remote support for it...
 string WinUtil::getWMPSpam(HWND playerWnd /*= NULL*/) {
	// If it's not running don't even bother...
	if(playerWnd != NULL) {
		// Load COM
		CoInitialize(NULL);

		// Pointers 
		CComPtr<IWMPPlayer>					Player;
		CComPtr<IAxWinHostWindow>			Host;
		CComPtr<IObjectWithSite>			HostObj;
		CComObject<WMPlayerRemoteApi>		*WMPlayerRemoteApiCtrl = NULL;
		
		// Other
		HRESULT								hresult;
		CAxWindow *DummyWnd;
		StringMap params;

		// Create hidden window to host the control (if there just was other way to do this... as CoCreateInstance has no access to the current running instance)
		AtlAxWinInit();
		DummyWnd = new CAxWindow();
		hresult = DummyWnd? S_OK : E_OUTOFMEMORY;

		if(SUCCEEDED(hresult)) {
			DummyWnd->Create(mainWnd, NULL, NULL, WS_CHILD | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SIZEBOX | WS_SYSMENU);
			hresult = ::IsWindow(DummyWnd->m_hWnd)? S_OK : E_FAIL;
		}
		
		// Set WMPlayerRemoteApi
		if(SUCCEEDED(hresult)) {
			hresult = DummyWnd->QueryHost(IID_IObjectWithSite, (void **)&HostObj);
			hresult = HostObj.p? hresult : E_FAIL;

			if(SUCCEEDED(hresult)) {
				hresult = CComObject<WMPlayerRemoteApi>::CreateInstance(&WMPlayerRemoteApiCtrl);
				if(WMPlayerRemoteApiCtrl) {
					WMPlayerRemoteApiCtrl->AddRef();
				} else {
					hresult = E_POINTER;
				}
			}

			if(SUCCEEDED(hresult)) {
				hresult = HostObj->SetSite((IWMPRemoteMediaServices *)WMPlayerRemoteApiCtrl);
			}
		}
		
		if(SUCCEEDED(hresult)) {
			hresult = DummyWnd->QueryHost(&Host);
			hresult = Host.p? hresult : E_FAIL;
		}

		// Create WMP Control 
		if(SUCCEEDED(hresult)) {
			hresult = Host->CreateControl(CComBSTR(L"{6BF52A52-394A-11d3-B153-00C04F79FAA6}"), DummyWnd->m_hWnd, 0);
		}
	
		// Now we can finally start to interact with WMP, after we successfully get the "Player"
		if(SUCCEEDED(hresult)) {
			hresult = DummyWnd->QueryControl(&Player);
			hresult = Player.p? hresult : E_FAIL;
		}

		// We've got this far now the grande finale, get the metadata :)
		if(SUCCEEDED(hresult)) {
			CComPtr<IWMPMedia>		Media;
			CComPtr<IWMPControls>	Controls;

			if(SUCCEEDED(Player->get_currentMedia(&Media)) && Media.p != NULL) {
				Player->get_controls(&Controls);

				// Windows Media Player version
				CComBSTR bstrWMPVer;
				Player->get_versionInfo(&bstrWMPVer);
				if(bstrWMPVer != NULL) {
					::COLE2T WMPVer(bstrWMPVer);
					params["fullversion"] = Text::fromT(WMPVer.m_szBuffer);
					params["version"] = params["fullversion"].substr(0, params["fullversion"].find("."));
				}

				// Pre-formatted status message from Windows Media Player
				CComBSTR bstrWMPStatus;
				Player->get_status(&bstrWMPStatus);
				if(bstrWMPStatus != NULL) {
					::COLE2T WMPStatus(bstrWMPStatus);
					params["status"] = Text::fromT(WMPStatus.m_szBuffer);
				}

				// Name of the currently playing media
				CComBSTR bstrMediaName;
				Media->get_name(&bstrMediaName);
				if(bstrMediaName != NULL) {
					::COLE2T MediaName(bstrMediaName);
					params["title"] = Text::fromT(MediaName.m_szBuffer);
				}

				// How much the user has already played 
				// I know this is later duplicated with get_currentPosition() for percent and bar, but for some reason it's overall faster this way 
				CComBSTR bstrMediaPosition;
				Controls->get_currentPositionString(&bstrMediaPosition);
				if(bstrMediaPosition != NULL) {
					::COLE2T MediaPosition(bstrMediaPosition);
					params["elapsed"] = Text::fromT(MediaPosition.m_szBuffer);
				}

				// Full duratiuon of the media
				// I know this is later duplicated with get_duration() for percent and bar, but for some reason it's overall faster this way 
				CComBSTR bstrMediaDuration;
				Media->get_durationString(&bstrMediaDuration);
				if(bstrMediaDuration != NULL) {
					::COLE2T MediaDuration(bstrMediaDuration);
					params["length"] = Text::fromT(MediaDuration.m_szBuffer);
				}

				// Name of the artist (use Author as secondary choice)
				CComBSTR bstrArtistName;
				Media->getItemInfo(CComBSTR(_T("WM/AlbumArtist")), &bstrArtistName);
				if(bstrArtistName != NULL) {
					::COLE2T ArtistName(bstrArtistName);
					params["artist"] = Text::fromT(ArtistName.m_szBuffer);
				} else {
					Media->getItemInfo(CComBSTR(_T("Author")), &bstrArtistName);
					if(bstrArtistName != NULL) {
						::COLE2T ArtistName(bstrArtistName);
						params["artist"] = Text::fromT(ArtistName.m_szBuffer);
					}
				}

				// Name of the album
				CComBSTR bstrAlbumTitle;
				Media->getItemInfo(CComBSTR(_T("WM/AlbumTitle")), &bstrAlbumTitle);
				if(bstrAlbumTitle != NULL) {
					::COLE2T AlbumName(bstrAlbumTitle);
					params["album"] = Text::fromT(AlbumName.m_szBuffer);
				}

				// Genre of the media
				CComBSTR bstrMediaGen;
				Media->getItemInfo(CComBSTR(_T("WM/Genre")), &bstrMediaGen);
				if(bstrMediaGen != NULL) {
					::COLE2T MediaGen(bstrMediaGen);
					params["genre"] = Text::fromT(MediaGen.m_szBuffer);
				}

				// Year of publiciation
				CComBSTR bstrMediaYear;
				Media->getItemInfo(CComBSTR(_T("WM/Year")), &bstrMediaYear);
				if(bstrMediaYear != NULL) {
					::COLE2T MediaYear(bstrMediaYear);
					params["year"] = Text::fromT(MediaYear.m_szBuffer);
				} else {
					Media->getItemInfo(CComBSTR(_T("ReleaseDateYear")), &bstrMediaYear);
					if(bstrMediaYear != NULL) {
						::COLE2T MediaYear(bstrMediaYear);
						params["year"] = Text::fromT(MediaYear.m_szBuffer);
					}
				}

				// Bitrate, displayed as Windows Media Player displays it
				CComBSTR bstrMediaBitrate;
				Media->getItemInfo(CComBSTR(_T("Bitrate")), &bstrMediaBitrate);
				if(bstrMediaBitrate != NULL) {
					::COLE2T MediaBitrate(bstrMediaBitrate);
					double BitrateAsKbps = (Util::toDouble(Text::fromT(MediaBitrate.m_szBuffer))/1000);
					params["bitrate"] = Util::toString(int(BitrateAsKbps)) + "kbps";
				}

				// Size of the file
				CComBSTR bstrMediaSize;
				Media->getItemInfo(CComBSTR(_T("Size")), &bstrMediaSize);
				if(bstrMediaSize != NULL) {
					::COLE2T MediaSize(bstrMediaSize);
					params["size"] = Util::formatBytes(Text::fromT(MediaSize.m_szBuffer));
				}

				// Users rating for this media
				CComBSTR bstrUserRating;
				Media->getItemInfo(CComBSTR(_T("UserRating")), &bstrUserRating);
				if(bstrUserRating != NULL) {
					if(bstrUserRating == "0") {
						params["rating"] = "unrated";
					} else if(bstrUserRating == "1") {
						params["rating"] = "*";
					} else if(bstrUserRating == "25") {
						params["rating"] = "* *";
					} else if(bstrUserRating == "50") {
						params["rating"] = "* * *";
					} else if(bstrUserRating == "75") {
						params["rating"] = "* * * *";
					} else if(bstrUserRating == "99") {
						params["rating"] = "* * * * *";
					} else {
						params["rating"] = "";
					}
				}

				// Bar & percent
				double elapsed;
				double length;
				Controls->get_currentPosition(&elapsed);
				Media->get_duration(&length);
				if(elapsed > 0) {
					int intPercent;
					if (length > 0 ) {
						intPercent = int(elapsed) * 100 / int(length);
					} else {
						length = 0;
						intPercent = 0;
					}
					params["percent"] = Util::toString(intPercent) + "%";
					int numFront = min(max(intPercent / 10, 0), 10),
						numBack = min(max(10 - 1 - numFront, 0), 10);
					string inFront = string(numFront, '-'),
						inBack = string(numBack, '-');
					params["bar"] = "[" + inFront + (elapsed > 0 ? "|" : "-") + inBack + "]";
				} else {
					params["percent"] = "0%";
					params["bar"] = "[|---------]";
				}
			}
		}

		// Release WMPlayerRemoteApi, if it's there
		if(WMPlayerRemoteApiCtrl) {
			WMPlayerRemoteApiCtrl->Release();
		}
			
		// Destroy the hoster window, and unload COM
		DummyWnd->DestroyWindow();
		delete DummyWnd;
		CoUninitialize();
			
		// If there is something in title, we have at least partly succeeded 
		if(!params["title"].empty()) {
			return Util::formatParams(SETTING(WMP_FORMAT), params, false);
		} else {
			return "no_media";
		}
	} else {
		return "";
	}
}


int WinUtil::setButtonPressed(int nID, bool bPressed /* = true */) {
	if (nID == -1)
		return -1;
	if (!MainFrame::getMainFrame()->getToolBar().IsWindow())
		return -1;

	MainFrame::getMainFrame()->getToolBar().CheckButton(nID, bPressed);
	return 0;
}



void WinUtil::search(tstring searchTerm, int searchMode, bool tth) {
	if(!searchTerm.empty()) {
		TCHAR chars[33] = {_T('<'), _T('>'), _T(','), _T(';'), _T('.'), _T(':'), _T('-'), _T('_'), _T('!'),
						_T('\"'), _T('@'), _T('#'), _T('£'), _T('$'), _T('%'), _T('&'), _T('/'), _T('{'),
						_T('('), _T('['), _T(')'), _T(']'), _T('='), _T('}'), _T('?'), _T('+'), _T('´'),
						_T('`'), _T('*'), _T('^'), _T('\\'), _T('\r'), _T('\n')};
		
		size_t length = searchTerm.length()-1;
		try{
			//check for bad chars in the list and remove any matches.
			for(int i = 0; i < 31; ++i) {
				if(searchTerm[0] == chars[i]) {
					searchTerm.erase((tstring::size_type)0, (tstring::size_type)1);
					i = 0;
					--length;
				}
				if(searchTerm[length] == chars[i]) {
					searchTerm.erase(length, length);
					i = 0;
					--length;
				}
			}
		}catch(Exception) {
			return;
		}
		if(!searchTerm.empty()) {
			if(0 == searchMode) {
				if(tth)
					SearchFrame::openWindow(searchTerm, 0, SearchManager::SIZE_ATLEAST, SearchManager::TYPE_TTH);
				else
					SearchFrame::openWindow(searchTerm, 0, SearchManager::SIZE_ATLEAST, SearchManager::TYPE_ANY);
			
				}
			}
		}
	}

tstring WinUtil::getIconPath(const tstring& filename) {

/*	Dont think we need to change it by os anymore.
		if(stricmp(Util::getFileExt(filename), _T(".bmp")) == 0) {
		if (getOsMajor() == 5 && getOsMinor() == 0)
			return _T("icons\\24bpp\\") + filename;
		else
			return _T("icons\\32bpp\\") + filename;
	}
*/
	//but keeping it like this, ToDo different icons sets from iconpaths

	return _T("icons\\") + filename;
}

tstring WinUtil::getTitle(tstring searchTerm) {

		//Remove group name
		string tmp = Text::fromT(searchTerm);
		int spos = tmp.rfind("-");
		tmp = tmp.substr(0,spos);
		searchTerm = Text::toT(tmp);
		searchTerm += ' ';

		//replace . with space
		int pos = 0;
		while ( (pos = searchTerm.find_first_of(_T("._"), pos)) != string::npos) {
			searchTerm.replace(pos, 1, _T(" "));
		}

		searchTerm = Text::toLower(searchTerm);

		//remove words after year/episode
		PME regexp;
		regexp.Init(_T("(((\\[)?((19[0-9]{2})|(20[0-1][0-9]))|(s[0-9]([0-9])?(e|d)[0-9]([0-9])?)|(Season(\\.)[0-9]([0-9])?)).*)"));
		searchTerm = regexp.sub(searchTerm, Util::emptyStringT);

		//remove extra words
		string extrawords[] = {"multisubs","multi","dvdrip","dvdr","real proper","proper","ultimate directors cut","directors cut","dircut","x264","pal","complete","limited","ntsc","bd25",
								"bd50","bdr","bd9","retail","bluray","nordic","720p","1080p","read nfo","dts","hdtv","pdtv","hddvd","repack","internal","custom","subbed","unrated","recut",
								"extended","dts51","finsub","swesub","dksub","nosub","remastered","2disc","rf","fi","swe","stv","r5","festival","anniversary edition","bdrip","ac3", "xvid",
								"ws","int"};
		pos = 0;
		while(pos <= 53) {
			boost::algorithm::replace_all(searchTerm, " " + extrawords[pos] + " ", " ");
			pos++;
		}

		//trim spaces from the end
		boost::algorithm::trim_right(searchTerm);
		return searchTerm;
}

/**
 * @file
 * $Id: WinUtil.cpp 473 2010-01-12 23:17:33Z bigmuscle $
 */
