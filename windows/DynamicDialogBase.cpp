/*
* Copyright (C) 2011-2017 AirDC++ Project
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
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

#include "DynamicDialogBase.h"


namespace wingui {
DynamicDialogBase::DynamicDialogBase(const tstring& aName) : name(aName) {
	//Create a dummy page to fill in dialog items
	m_page = make_shared<DynamicTabPage>(DynamicTabPage());
}

LRESULT DynamicDialogBase::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	m_page->Create(m_hWnd);

	// save the original size
	GetClientRect(&m_rcOriginal);

	updateLayout();

	CenterWindow(GetParent());
	SetWindowText(name.c_str());

	::SetWindowText(GetDlgItem(IDCANCEL), CTSTRING(CANCEL));

	return TRUE;
}

void DynamicDialogBase::updateLayout() {
	CRect rcOKbtn;
	GetDlgItem(IDOK).GetClientRect(&rcOKbtn);

	CRect rcDialog;
	GetClientRect(&rcDialog); // current base dialog rect

	//Size the page, exclude OK button area by button left edge
	CRect rcPage;
	m_page->GetClientRect(&rcPage);
	rcPage.top += m_pageSpacing;
	rcPage.right = rcDialog.right - (rcOKbtn.Width() + m_pageSpacing); //OK button left
	rcPage.left = rcDialog.left + m_pageSpacing;
	rcPage.bottom = rcDialog.bottom;
	rcPage.bottom -= m_pageSpacing;
	m_page->MoveWindow(rcPage);

	//update page setting items layout and resize the page accordingly. Updates the rect.bottom.
	m_page->updateLayout(rcPage);


	m_rcClip.left = rcPage.left;
	m_rcClip.top = 0;
	m_rcClip.right = rcPage.right;
	m_rcClip.bottom = rcPage.bottom;

	//Set the scrolling area.
	m_rcScroll = rcPage;
	m_rcScroll.top = -(rcPage.bottom);

	//Set scrollbar dimensions.
	SCROLLINFO si = { 0 };
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(SB_VERT, &si);
	si.nMax = m_rcScroll.bottom + m_pageSpacing;
	si.nPage = rcDialog.bottom;
	SetScrollInfo(SB_VERT, &si);
}

LRESULT DynamicDialogBase::onMouseWheel(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	auto i = GET_WHEEL_DELTA_WPARAM(wParam);
	if (i > 0) {
		PostMessage(WM_VSCROLL, MAKELONG(SB_LINEUP, 0), 0);
	} else if (i < 0) {
		PostMessage(WM_VSCROLL, MAKELONG(SB_LINEDOWN, 0), 0);
	}

	bHandled = FALSE;
	return 0;
}

LRESULT DynamicDialogBase::onScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	int nDelta = 0;
	int nPos = HIWORD(wParam);

	SCROLLINFO si = { 0 };
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(SB_VERT, &si);
	int nMaxPos = si.nMax - si.nPage;
	int nMinPos = si.nMin;

	switch (LOWORD(wParam)) {
	case SB_THUMBTRACK:
		nDelta = nPos - m_curScrollPos;
		break;
	case SB_THUMBPOSITION:
		nDelta = nPos - m_curScrollPos;
		break;
	case SB_LINEDOWN:
		if (m_curScrollPos >= nMaxPos)
			return 0;
		nDelta = min(30, nMaxPos - m_curScrollPos);
		break;
	case SB_LINEUP:
		if (m_curScrollPos <= nMinPos)
			return 0;
		nDelta = -min(30, m_curScrollPos);
		break;

	default:
		return 0;
	}

	m_curScrollPos += nDelta;
	SetScrollPos(SB_VERT, m_curScrollPos);
	ScrollWindowEx(0, -nDelta, &m_rcScroll, &m_rcClip, NULL, NULL, SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN);
	UpdateWindow();
	bHandled = TRUE;
	return 0;
}

LRESULT DynamicDialogBase::onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	m_page->SetFocus();
	return 0;
}

LRESULT DynamicDialogBase::onSize(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	updateLayout();
	return 0;
}

LRESULT DynamicDialogBase::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled) {
	if (wID == IDOK) {
		if (!m_page->write()) {
			bHandled = FALSE;
			return 0;
		}
	}
	EndDialog(wID);
	return 0;
}
}