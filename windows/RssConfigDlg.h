#pragma once
/*
* Copyright (C) 2012-2016 AirDC++ Project
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

#ifndef RSS_DLG_H
#define RSS_DLG_H

#pragma once

#include <atlcrack.h>
#include "ExListViewCtrl.h"
#include <airdcpp/RSSManager.h>

class RssDlg : public CDialogImpl<RssDlg> {
public:

	enum { IDD = IDD_RSS_DLG };

	RssDlg();
	~RssDlg();

	BEGIN_MSG_MAP_EX(RssDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_RSS_ADD, onAdd)
		COMMAND_ID_HANDLER(IDC_RSS_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_RSS_BROWSE, onBrowse)
		COMMAND_ID_HANDLER(IDC_RSS_UPDATE, onUpdate)
		NOTIFY_HANDLER(IDC_RSS_LIST, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_RSS_LIST, LVN_ITEMCHANGED, onSelectionChanged)
		END_MSG_MAP()


	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		//ctrlSearch.SetFocus();
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onSelectionChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
		NMLVKEYDOWN* kd = (NMLVKEYDOWN*)pnmh;
		switch (kd->wVKey) {
		case VK_DELETE:
			PostMessage(WM_COMMAND, IDC_REMOVE, 0);
			break;
		default:
			bHandled = FALSE;
		}
		return 0;
	}
private:

	class RSSConfigItem {
	public:

		RSSConfigItem(const RSSPtr& aRss) noexcept :
			url(aRss->getUrl()), categories(aRss->getCategories()), autoSearchFilter(aRss->getAutoSearchFilter()), downloadTarget(aRss->getDownloadTarget())
		{
		}
		RSSConfigItem(const string& aUrl, const string& aCategory, const string& aAutoSearchFilter, const string& aDownloadTarget) noexcept :
			url(aUrl), categories(aCategory), autoSearchFilter(aAutoSearchFilter), downloadTarget(aDownloadTarget) {}

		~RSSConfigItem() {};

		GETSET(string, url, Url);
		GETSET(string, categories, Categories);
		GETSET(string, autoSearchFilter, AutoSearchFilter);
		GETSET(string, downloadTarget, DownloadTarget);
	};


	CEdit ctrlUrl;
	CEdit ctrlCategorie;
	CEdit ctrlAutoSearchPattern;
	CEdit ctrlTarget;

	ExListViewCtrl ctrlRssList;

	deque<RSSConfigItem> rssList;
	StringList removeList;

	void fillList();

	void remove();
	bool add();
	void update();

	void restoreSelection(const tstring& curSel);
};
#endif