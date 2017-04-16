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

	//add CEdit test item
	RECT r = { 100, 80, 350, 130 };
	ctrlEdit.Create(m_hWnd, r, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE);
	ctrlEdit.SetFont(WinUtil::systemFont);

	loading = false; //loading done.
	return TRUE;
}


