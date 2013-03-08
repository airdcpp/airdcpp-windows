/* 
 * Copyright (C) 2012-2013 AirDC++ Project
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

#include "PopupDlg.h"
#include "Resource.h"
#include "WinUtil.h"
#include "MainFrm.h"

PopupWnd::PopupWnd(const tstring& aMsg, const tstring& aTitle, CRect rc, uint32_t aId, HBITMAP hBmp): visible(GET_TICK()), id(aId), msg(aMsg), title(aTitle), bmp(hBmp) {
	if((SETTING(POPUP_TYPE) == BALLOON) || (SETTING(POPUP_TYPE) == SPLASH))
		Create(NULL, rc, NULL, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW );
	else if((SETTING(POPUP_TYPE) == CUSTOM) && (bmp != NULL))
		Create(NULL, rc, NULL, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW );
	else
		Create(NULL, rc, NULL, WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW );

	WinUtil::decodeFont(Text::toT(SETTING(POPUP_FONT)), logFont);
	font = ::CreateFontIndirect(&logFont);

	WinUtil::decodeFont(Text::toT(SETTING(POPUP_TITLE_FONT)), myFont);
	titlefont = ::CreateFontIndirect(&myFont);

}

PopupWnd::~PopupWnd(){
	DeleteObject(font);
	DeleteObject(titlefont);
}

LRESULT PopupWnd::onLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled){
	MainFrame::getMainFrame()->callAsync([=] { PopupManager::getInstance()->Remove(id); });
	bHandled = TRUE;
	return 0;
}

LRESULT PopupWnd::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled){
	if(bmp != NULL && (SETTING(POPUP_TYPE) == CUSTOM)) {
		bHandled = FALSE;
		return 1;
	}

	::SetClassLongPtr(m_hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)::GetSysColorBrush(COLOR_INFOTEXT));
	CRect rc;
	GetClientRect(rc);

	rc.top += 1;
	rc.left += 1;
	rc.right -= 1;
	if((SETTING(POPUP_TYPE) == BALLOON) || (SETTING(POPUP_TYPE) == CUSTOM) || (SETTING(POPUP_TYPE) == SPLASH))
		rc.bottom /= 3;
	else
		rc.bottom /= 4;

	label.Create(m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		SS_CENTER | SS_NOPREFIX);

	rc.top += rc.bottom - 1;
	if((SETTING(POPUP_TYPE) == BALLOON) || (SETTING(POPUP_TYPE) == CUSTOM) || (SETTING(POPUP_TYPE) == SPLASH)) 
		rc.bottom *= 3;
	else
		rc.bottom = (rc.bottom * 4) + 1;

	label1.Create(m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		SS_CENTER | SS_NOPREFIX);

	if(SETTING(POPUP_TYPE) == BALLOON) {
		label.SetFont(WinUtil::boldFont);
		label.SetWindowText(title.c_str());
		label1.SetFont(WinUtil::font);
		label1.SetWindowText(msg.c_str());
		bHandled = false;
		return 1;
	} else if(SETTING(POPUP_TYPE) == CUSTOM || (SETTING(POPUP_TYPE) == SPLASH)) {
		label.SetFont(WinUtil::boldFont);
		label.SetWindowText(title.c_str());
	} else
		SetWindowText(title.c_str());

	label1.SetFont(font);
	label1.SetWindowText(msg.c_str());


	bHandled = false;
	return 1;
}

LRESULT PopupWnd::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled){
	if(bmp == NULL || (SETTING(POPUP_TYPE) != CUSTOM)){
		label.DestroyWindow();
		label.Detach();
		label1.DestroyWindow();
		label1.Detach();
	}
	DestroyWindow();

	bHandled = false;
	return 1;
}

LRESULT PopupWnd::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	HWND hWnd = (HWND)lParam;
	HDC hDC = (HDC)wParam;
	::SetBkColor(hDC, SETTING(POPUP_BACKCOLOR));
	::SetTextColor(hDC, SETTING(POPUP_TEXTCOLOR));
	if(hWnd == label1.m_hWnd)
		::SelectObject(hDC, font);
	return (LRESULT)CreateSolidBrush(SETTING(POPUP_BACKCOLOR));
}

LRESULT PopupWnd::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(bmp == NULL || (SETTING(POPUP_TYPE) != CUSTOM)){
		bHandled = FALSE;
		return 0;
	}

	PAINTSTRUCT ps;
	HDC hdc = ::BeginPaint(m_hWnd,&ps);

	HDC hdcMem = CreateCompatibleDC(NULL);
	HBITMAP hbmT = (HBITMAP)::SelectObject(hdcMem,bmp);

	BITMAP bm;
	GetObject(bmp,sizeof(bm),&bm);

	//Move selected bitmap to the background
	BitBlt(hdc,0,0,bm.bmWidth,bm.bmHeight,hdcMem,0,0,SRCCOPY);

	SelectObject(hdcMem,hbmT);
	DeleteDC(hdcMem);

	//Cofigure border
	::SetBkMode(hdc, TRANSPARENT);

	int xBorder = bm.bmWidth / 10;
	int yBorder = bm.bmHeight / 10;
	CRect rc(xBorder, yBorder, bm.bmWidth - xBorder, bm.bmHeight - yBorder);

	//Draw the Title and Message with selected font and color
	tstring pmsg = _T("\r\n\r\n") + msg;
	HFONT oldTitleFont = (HFONT)SelectObject(hdc, titlefont);
	::SetTextColor(hdc, SETTING(POPUP_TITLE_TEXTCOLOR));
	::DrawText(hdc, title.c_str(), title.length(), rc, DT_SINGLELINE | DT_TOP | DT_CENTER);

	HFONT oldFont = (HFONT)SelectObject(hdc, font);
	::SetTextColor(hdc, SETTING(POPUP_TEXTCOLOR));
	::DrawText(hdc, pmsg.c_str(), pmsg.length(), rc, DT_LEFT | DT_WORDBREAK);

	SelectObject(hdc, oldTitleFont);
	SelectObject(hdc, oldFont);
	::EndPaint(m_hWnd,&ps);

	return 0;
}