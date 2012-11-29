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
	splash.Create(_T("Static"), GetDesktopWindow(), splash.rcDefault, NULL, WS_POPUP | WS_VISIBLE | SS_USERITEM, WS_EX_TOOLWINDOW);
	splash.SetFont((HFONT)GetStockObject(DEFAULT_GUI_FONT));
	
	//load the image
	loadImage();

	//Get the dimensions of the bitmap, this is probobly easier anyway even if we dont allow loading custom splashes.
	BITMAP bm;
	::GetObject( (HBITMAP)img, sizeof( bm ), &bm );
	width = bm.bmWidth;
	height = bm.bmHeight;

	HDC dc = splash.GetDC();
	rc.right = rc.left + width;
	rc.bottom = rc.top + height;
	splash.ReleaseDC(dc);
	splash.HideCaret();
	splash.SetWindowPos(HWND_TOPMOST, &rc, SWP_SHOWWINDOW);
	splash.CenterWindow();

	title = _T(VERSIONSTRING) _T(" ") _T(CONFIGURATION_TYPE);

	splash.SetFocus();
	splash.RedrawWindow();
}
void SplashWindow::loadImage() {
	if(img.Load(_T("splash.png")) != S_OK)
		img.LoadFromResource(IDB_SPLASH, _T("PNG"), _Module.get_m_hInst());
}

SplashWindow::~SplashWindow() {
	if(!img.IsNull())
		img.Destroy();
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
	rc2.top = rc2.bottom - 30; 
	rc2.left = rc2.left + 20;
	::SetBkMode(dc, TRANSPARENT);
	
	HDC memDC = img.GetDC();
	BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
	AlphaBlend(dc, 0, 0, width, height, memDC, 0, 0, width, height, bf);
	
	SelectObject(dc, img);
	img.ReleaseDC();

	LOGFONT logFont;
	HFONT hFont;
	GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(logFont), &logFont);
	lstrcpy(logFont.lfFaceName, TEXT("Tahoma"));
	logFont.lfHeight = 15;
	logFont.lfWeight = 700;
	hFont = CreateFontIndirect(&logFont);		
	SelectObject(dc, hFont);
	//::SetTextColor(dc, RGB(255,255,255)); //white text
	::SetTextColor(dc, RGB(0,0,0)); //black text
	//::SetTextColor(dc, RGB(104,104,104)); //grey text
	::DrawText(dc, title.c_str(), _tcslen(title.c_str()), &rc2, DT_LEFT);
	DeleteObject(hFont);

	if(!status.empty()) {
		rc2 = rc;
		rc2.top = rc2.bottom - 30;
		rc2.right = rc2.right - 20;
		GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(logFont), &logFont);
		lstrcpy(logFont.lfFaceName, TEXT("Tahoma"));
		logFont.lfHeight = 12;
		logFont.lfWeight = 700;
		hFont = CreateFontIndirect(&logFont);		
		SelectObject(dc, hFont);
		//::SetTextColor(dc, RGB(255,255,255)); // white text
		::SetTextColor(dc, RGB(0,0,0)); // black text
		//::SetTextColor(dc, RGB(104,104,104)); //grey text
		::DrawText(dc, (_T(".:: ") + Text::toT(status) + _T(" ::.")).c_str(), _tcslen((_T(".:: ") + Text::toT(status) + _T(" ::.")).c_str()), &rc2, DT_RIGHT);
		DeleteObject(hFont);
	}

	ReleaseDC(splash.m_hWnd, dc);
}
