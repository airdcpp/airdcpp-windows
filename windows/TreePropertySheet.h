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

#if !defined(TREE_PROPERTY_SHEET_H)
#define TREE_PROPERTY_SHEET_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"

class TreePropertySheet : public CPropertySheetImpl<TreePropertySheet> {
public:
	virtual ~TreePropertySheet() { };
	enum { WM_USER_INITDIALOG = WM_APP + 501 };
	enum { TAB_MESSAGE_MAP = 13 };
	TreePropertySheet(ATL::_U_STRINGorID title = (LPCTSTR)NULL, UINT uStartPage = 0, HWND hWndParent = NULL) :
		CPropertySheetImpl<TreePropertySheet>(title, uStartPage, hWndParent), tabContainer(WC_TABCONTROL, this, TAB_MESSAGE_MAP), curPage(0) {

		m_psh.pfnCallback = &PropSheetProc;
		m_psh.dwFlags |= PSH_RTLREADING;
	}
	
	typedef CPropertySheetImpl<TreePropertySheet> baseClass;
	BEGIN_MSG_MAP(TreePropertySheet)
		MESSAGE_HANDLER(WM_COMMAND, baseClass::OnCommand)
		MESSAGE_HANDLER(WM_USER_INITDIALOG, onInitDialog)
		MESSAGE_HANDLER(WM_NOTIFYFORMAT, onNotifyFormat)
		NOTIFY_HANDLER(IDC_PAGE, TVN_SELCHANGED, onSelChanged)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(TAB_MESSAGE_MAP)
		MESSAGE_HANDLER(TCM_SETCURSEL, onSetCurSel)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSetCurSel(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);

	LRESULT onSelChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT onNotifyFormat(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
#ifdef _UNICODE
		return NFR_UNICODE;
#else
		return NFR_ANSI;
#endif
	}

	static int CALLBACK PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam);
	int curPage;
private:

	enum {
		SPACE_MID = 5,
		SPACE_TOP = 10,
		SPACE_BOTTOM = 10,
		SPACE_LEFT = 10,
		SPACE_RIGHT = 10,
		TREE_WIDTH = 210,
	};

	enum {
		MAX_NAME_LENGTH = 256
	};

	void hideTab();
	void addTree();
	void fillTree();

	HTREEITEM createTree(const tstring& str, HTREEITEM parent, int page);
	HTREEITEM findItem(const tstring& str, HTREEITEM start);
	HTREEITEM findItem(int page, HTREEITEM start);

	CTreeViewCtrl ctrlTree;
	CContainedWindow tabContainer;

};

#endif // !defined(TREE_PROPERTY_SHEET_H)
