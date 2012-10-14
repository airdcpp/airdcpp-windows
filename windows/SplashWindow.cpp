/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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
#include "SplashWindow.h"

#include <../client/Text.h>
#include <../client/version.h>

#include "resource.h"
#include "WinUtil.h"

SplashWindow::SplashWindow() {
	CRect rc;
	rc.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
	rc.top = (rc.bottom / 2) - 80;

	rc.right = GetSystemMetrics(SM_CXFULLSCREEN);
	rc.left = rc.right / 2 - 85;
	
	
	dummy.Create(NULL, rc, _T(APPNAME) _T(" ") _T(VERSIONSTRING), WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_CENTER | ES_READONLY, WS_EX_STATICEDGE);
	splash.Create(_T("Static"), GetDesktopWindow(), splash.rcDefault, NULL, WS_POPUP | WS_VISIBLE | SS_USERITEM | WS_EX_TOOLWINDOW);
	splash.SetFont((HFONT)GetStockObject(DEFAULT_GUI_FONT));
	
	HDC dc = splash.GetDC();
	rc.right = rc.left + 350;
	rc.bottom = rc.top + 120;
	splash.ReleaseDC(dc);
	splash.HideCaret();
	splash.SetWindowPos(HWND_TOPMOST, &rc, SWP_SHOWWINDOW);
	splash.CenterWindow();

	title = _T(VERSIONSTRING) _T(" ") _T(CONFIGURATION_TYPE);

	splash.SetFocus();
	splash.RedrawWindow();
}

SplashWindow::~SplashWindow() {
	splash.DestroyWindow();
	dummy.DestroyWindow();
}

void SplashWindow::operator()(const string& status) {
	// Get some information
	HDC dc = GetDC(splash.m_hWnd);
	RECT rc;
	GetWindowRect(splash.m_hWnd, &rc);
	OffsetRect(&rc, -rc.left, -rc.top);
	RECT rc2 = rc;
	rc2.top = rc2.bottom - 35; 
	rc2.right = rc2.right - 10;
	::SetBkMode(dc, TRANSPARENT);
		
	// Draw the icon
	HBITMAP hi;
	hi = (HBITMAP)LoadImage(_Module.get_m_hInst(), MAKEINTRESOURCE(IDB_SPLASH), IMAGE_BITMAP, 350, 120, LR_SHARED);
			 
	HDC comp=CreateCompatibleDC(dc);
	SelectObject(comp,hi);	

	BitBlt(dc,0, 0 , 350, 120,comp,0,0,SRCCOPY);

	DeleteObject(hi);
	DeleteDC(comp);
	LOGFONT logFont;
	HFONT hFont;
	GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(logFont), &logFont);
	lstrcpy(logFont.lfFaceName, TEXT("Tahoma"));
	logFont.lfHeight = 15;
	logFont.lfWeight = 700;
	hFont = CreateFontIndirect(&logFont);		
	SelectObject(dc, hFont);
	::SetTextColor(dc, RGB(255,255,255));
	::DrawText(dc, title.c_str(), _tcslen(title.c_str()), &rc2, DT_RIGHT);
	DeleteObject(hFont);

	if(!status.empty()) {
		rc2 = rc;
		rc2.top = rc2.bottom - 15;
		GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(logFont), &logFont);
		lstrcpy(logFont.lfFaceName, TEXT("Tahoma"));
		logFont.lfHeight = 12;
		logFont.lfWeight = 700;
		hFont = CreateFontIndirect(&logFont);		
		SelectObject(dc, hFont);
		::SetTextColor(dc, RGB(255,255,255));
		::DrawText(dc, (_T(".:: ") + Text::toT(status) + _T(" ::.")).c_str(), _tcslen((_T(".:: ") + Text::toT(status) + _T(" ::.")).c_str()), &rc2, DT_CENTER);
		DeleteObject(hFont);
	}

	ReleaseDC(splash.m_hWnd, dc);
}
