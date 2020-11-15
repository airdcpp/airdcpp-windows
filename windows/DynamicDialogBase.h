#pragma once
#pragma once
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

#ifndef DYNAMIC_DIALOG_BASE_H
#define DYNAMIC_DIALOG_BASE_H

#include <atlcrack.h>
#include "stdafx.h"

#include "DynamicTabPage.h"


class DynamicDialogBase : public CDialogImpl<DynamicDialogBase> {
public:

	enum { IDD = IDD_DYNAMIC_DIALOG };

	DynamicDialogBase(const tstring& aName);
	~DynamicDialogBase() {

	}

	BEGIN_MSG_MAP_EX(DynamicDialogBase)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_MOUSEWHEEL, onMouseWheel)
		MESSAGE_HANDLER(WM_VSCROLL, onScroll)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	shared_ptr<DynamicTabPage> getPage() { return m_page; }

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	void updateLayout();

	LRESULT onMouseWheel(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onSize(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled);

	// dialog size in the resource view (original size)
	CRect m_rcOriginal;

	// actual scroll position
	int m_curScrollPos = 0;
	int m_pageSpacing = 20;

	CEdit ctrlEdit;

	tstring name;
	CRect m_rcScroll;
	CRect m_rcClip;

	shared_ptr<DynamicTabPage> m_page;

};
#endif