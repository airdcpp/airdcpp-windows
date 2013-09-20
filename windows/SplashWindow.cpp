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
#include "SplashWindow.h"

#include <../client/Text.h>
#include <../client/version.h>

#include "resource.h"
#include "WinUtil.h"

LRESULT SplashWindow::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if (closing) {
		bHandled = FALSE;
		return 0;
	}

	ExitProcess(0);
}

SplashWindow::SplashWindow() {
}

void SplashWindow::create() {
	WinUtil::splash = new SplashWindow();
	WinUtil::splash->Create(NULL, rcDefault, _T("Static"), WS_POPUP | WS_VISIBLE | SS_USERITEM, 0);
}

void SplashWindow::OnFinalMessage(HWND /*hWnd*/) {
	WinUtil::splash = nullptr;
	delete this; 
}

LRESULT SplashWindow::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	CRect rc;
	rc.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
	rc.top = (rc.bottom / 2) - 80;

	rc.right = GetSystemMetrics(SM_CXFULLSCREEN);
	rc.left = rc.right / 2 - 85;

	dummy.Create(GetDesktopWindow(), rc, _T(APPNAME) _T(" ") _T(VERSIONSTRING), WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		ES_CENTER | ES_READONLY, WS_EX_STATICEDGE);
	SetFont((HFONT) GetStockObject(DEFAULT_GUI_FONT));

	//load the image
	loadImage();
	width = img.GetWidth();
	height = img.GetHeight();

	HDC dc = GetDC();
	rc.right = rc.left + width;
	rc.bottom = rc.top + height;
	ReleaseDC(dc);
	HideCaret();
	SetWindowPos(dummy.m_hWnd, &rc, SWP_SHOWWINDOW);
	CenterWindow();

	title = _T(VERSIONSTRING) _T(" ") _T(CONFIGURATION_TYPE);

	SetFocus();

	LOGFONT logFont;
	GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(logFont), &logFont);
	lstrcpy(logFont.lfFaceName, TEXT("Tahoma"));

	logFont.lfHeight = 12;
	logFont.lfWeight = 700;
	hFontStatus = CreateFontIndirect(&logFont);

	logFont.lfHeight = 15;
	logFont.lfWeight = 700;
	hFontTitle = CreateFontIndirect(&logFont);

	bHandled = FALSE;
	return TRUE;
}

void SplashWindow::loadImage() {
	if(img.Load(_T("splash.png")) != S_OK)
		img.LoadFromResource(IDB_SPLASH, _T("PNG"), _Module.get_m_hInst());
}

SplashWindow::~SplashWindow() {
	if(!img.IsNull())
		img.Destroy();

	DeleteObject(hFontStatus);
	DeleteObject(hFontTitle);
}

void SplashWindow::destroy() {
	closing = true;
	PostMessage(WM_CLOSE, 0, 0);
}

void SplashWindow::operator()(const string& status) {
	this->status = Text::toT(status);
	progress = 0;
	callAsync([this] { RedrawWindow(); });
}

void SplashWindow::operator()(float progress) {
	if (this->progress == 0.00 || progress - this->progress >= 0.01) {
		this->progress = progress;
		callAsync([this] { RedrawWindow(); });
	}
}

LRESULT SplashWindow::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	draw();
	return 0;
}


LRESULT SplashWindow::onEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	bHandled = TRUE;
	return TRUE;
}

void SplashWindow::draw() {
	// Get some information
	PAINTSTRUCT ps;
	HDC dc = BeginPaint(&ps);
	RECT rc;
	GetWindowRect(&rc);
	OffsetRect(&rc, -rc.left, -rc.top);
	RECT rc2 = rc;
	rc2.top = rc2.bottom - 20; 
	rc2.left = rc2.left + 20;
	::SetBkMode(dc, TRANSPARENT);
	
	//HDC memDC = img.GetDC();
	//BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
	//AlphaBlend(dc, 0, 0, width, height, memDC, 0, 0, width, height, bf);
	//img.ReleaseDC();
	HDC comp = CreateCompatibleDC(dc);
	SelectObject(comp, img);
	BitBlt(dc, 0, 0 , width, height, comp, 0, 0, SRCCOPY);
	DeleteDC(comp);

	SelectObject(dc, hFontTitle);
	//::SetTextColor(dc, RGB(255,255,255)); //white text
	//::SetTextColor(dc, RGB(0,0,0)); //black text
	::SetTextColor(dc, RGB(104,104,104)); //grey text
	::DrawText(dc, title.c_str(), _tcslen(title.c_str()), &rc2, DT_LEFT);

	if(!status.empty()) {
		rc2 = rc;
		rc2.top = rc2.bottom - 20;
		rc2.right = rc2.right - 20;	
		SelectObject(dc, hFontStatus);
		::SetTextColor(dc, RGB(104,104,104)); //grey text

		tstring tmp = (_T(".:: ") + status + (progress > 0 ? _T(" (") + Util::toStringW(static_cast<int>(progress*100.00)) + _T("%)") : Util::emptyStringT) + _T(" ::."));
		::DrawText(dc, tmp.c_str(), _tcslen((tmp).c_str()), &rc2, DT_RIGHT);
	}

	EndPaint(&ps);
}
