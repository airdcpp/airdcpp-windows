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

#include "stdafx.h"
#include "Resource.h"

#include "DynamicTabPage.h"
#include "WinUtil.h"


DynamicTabPage::DynamicTabPage() : loading(true) 
{ };

DynamicTabPage::~DynamicTabPage() 
{ };

LRESULT DynamicTabPage::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	for (auto cfg : configs) {
		cfg->Create(m_hWnd);
	}
	loading = false;
	return TRUE;
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

void DynamicTabPage::resizePage(CRect& windowRect) {

	if ((m_prevConfigBottomMargin + 5) >= windowRect.bottom) {
		windowRect.bottom = m_prevConfigBottomMargin + 70;
		MoveWindow(windowRect);
	}
}

void DynamicTabPage::updateLayout(CRect& windowRect) {
	for (auto cfg : configs) {
		m_prevConfigBottomMargin = cfg->updateLayout(m_hWnd, m_prevConfigBottomMargin, m_configSpacing);
		resizePage(windowRect);
	}
}


