#pragma once
/*
* Copyright (C) 2011-2021 AirDC++ Project
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

#ifndef TABBED_DLG_H
#define TABBED_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <atlcrack.h>

#define ATTACH(id, var) var.Attach(GetDlgItem(id))


//Wrapper around different dialog classes
class TabPage
{
public:
	TabPage() {}
	~TabPage() {}

	virtual bool write() = 0;
	virtual string getName() = 0;

	virtual void moveWindow(CRect& rc) = 0;
	virtual void create(HWND aParent) = 0;
	virtual void showWindow(BOOL aShow) = 0;

};


class TabbedDialog : public CDialogImpl<TabbedDialog> {
public:

	enum {
		IDD = IDD_TABBED_DIALOG
	};

	TabbedDialog(const string& aTitle);
	~TabbedDialog();

	BEGIN_MSG_MAP_EX(TabbedDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		NOTIFY_HANDLER(IDC_TAB1, TCN_SELCHANGE, onTabChanged)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		END_MSG_MAP()


	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled);
	LRESULT onTabChanged(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/);
	
	template<class T>
	void addPage(shared_ptr<T>&& t) {
		pages.push_back(move(t));
	}

private:

	vector<shared_ptr<TabPage>> pages;
	string wTitle;
	void showPage(int aPage);
	CTabCtrl cTab;

};

#endif
