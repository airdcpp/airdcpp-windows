/*
* Copyright (C) 2011-2017 AirDC++ Project
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
#include <commctrl.h>
#include "stdafx.h"
#include "Resource.h"
#include "WinUtil.h"
#include "DynamicTabPage.h"


DynamicTabPage::DynamicTabPage() : loading(true) {}

DynamicTabPage::~DynamicTabPage() { }

LRESULT DynamicTabPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	for (int i = 0; i < 21; i++) {
		addEditConfig(Util::toString(i) + " Test label for CEdit config", Util::toString(i));
	}

	loading = false;
	return TRUE;
}

void DynamicTabPage::resizePage() {
	CRect windowRect;
	GetClientRect(&windowRect);
	if ((prevConfigBottomMargin + 20) >= windowRect.bottom) {
		windowRect.bottom = prevConfigBottomMargin + 70;
		MoveWindow(windowRect);
	}
}

void DynamicTabPage::addEditConfig(const string& aName, const string& aId) {

	auto cfg = make_shared<EditConfig>(aName, aId);
	//label

	cfg->ctrlStatic.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_AUTOHSCROLL, NULL);
	cfg->ctrlStatic.SetFont(WinUtil::systemFont);
	cfg->ctrlStatic.SetWindowText(Text::toT(aName).c_str());

	cfg->ctrlEdit.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);
	cfg->ctrlEdit.SetFont(WinUtil::systemFont);

	CRect rc;

	//CStatic
	rc.left = 100;
	rc.top = prevConfigBottomMargin + configSpacing;
	rc.right = 400;
	rc.bottom = rc.top + WinUtil::getTextHeight(m_hWnd, WinUtil::systemFont);
	cfg->ctrlStatic.MoveWindow(rc);

	//CEdit
	rc.top = rc.bottom + 2;
	rc.bottom = rc.top + editHeight;

	cfg->ctrlEdit.MoveWindow(rc);

	prevConfigBottomMargin = rc.bottom;

	edits.emplace(aId, cfg);

	resizePage();
	
}

LRESULT DynamicTabPage::onCtlColor(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	if (uMsg == WM_CTLCOLORSTATIC) {
		HDC hdc = (HDC)wParam;
		SetBkMode(hdc, TRANSPARENT);
		SetBkColor(hdc, ::GetSysColor(COLOR_3DFACE));
		return (LRESULT)GetStockObject(COLOR_3DFACE);
	}
	else if (uMsg == WM_CTLCOLORDLG) {
		HDC hdc = (HDC)wParam;
		SetBkColor(hdc, ::GetSysColor(COLOR_WINDOW));
		return (LRESULT)GetStockObject(COLOR_WINDOW);
	}

	bHandled = FALSE;
	return FALSE;
}

LRESULT DynamicTabPage::onEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & /*bHandled*/) {
	//background
	WTL::CDCHandle dc(reinterpret_cast<HDC>(wParam));
	RECT rc;
	GetClientRect(&rc);
	dc.FillRect(&rc, GetSysColorBrush(COLOR_WINDOW));

	//draw the left border
	HGDIOBJ oldPen = SelectObject(dc, CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DLIGHT)));
	MoveToEx(dc, rc.left, rc.top, (LPPOINT)NULL);
	LineTo(dc, rc.left, rc.bottom);

	//draw the right border
	MoveToEx(dc, rc.right-1, rc.top, (LPPOINT)NULL);
	LineTo(dc, rc.right-1, rc.bottom);

	//draw the top border
	MoveToEx(dc, rc.left, rc.top, (LPPOINT)NULL);
	LineTo(dc, rc.right, rc.top);

	//draw the bottom border
	MoveToEx(dc, rc.left, rc.bottom - 1, (LPPOINT)NULL);
	LineTo(dc, rc.right, rc.bottom - 1);

	DeleteObject(SelectObject(dc, oldPen));
	return TRUE;
}


