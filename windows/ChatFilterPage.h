/*
 * Copyright (C) 2001-2016 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(CHATFILTER_PAGE_H)
#define CHATFILTER_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropPage.h"

#include "ExListViewCtrl.h"
#include <airdcpp/MessageManager.h>

class ChatFilterPage : public CPropertyPage<IDD_CHATFILTERPAGE>, public PropPage
{
public:
	ChatFilterPage(SettingsManager *s) : PropPage(s), loading(true) {
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_CHATFILTER)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	}
	~ChatFilterPage() { 
		free(title);
		ChatFilterListCtrl.Detach();
	}

	BEGIN_MSG_MAP_EX(ChatFilterPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_IGNORE_ADD, onChatFilterAdd)
		COMMAND_ID_HANDLER(IDC_IGNORE_REMOVE, onChatFilterRemove)
		COMMAND_ID_HANDLER(IDC_IGNORE_CLEAR, onChatFilterClear)
		COMMAND_ID_HANDLER(IDC_IGNORE_EDIT, onChatFilterEdit)
		NOTIFY_HANDLER(IDC_IGNORELIST, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_IGNORELIST, LVN_ITEMCHANGED, onItemchanged)
		NOTIFY_HANDLER(IDC_IGNORELIST, NM_DBLCLK, onDoubleClick)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onChatFilterAdd(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onChatFilterRemove(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onChatFilterClear(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onChatFilterEdit(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onItemchanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
		NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
		switch(kd->wVKey) {
		case VK_DELETE:
			PostMessage(WM_COMMAND, IDC_IGNORE_REMOVE, 0);
			break;
		default:
			bHandled = FALSE;
		}
		return 0;
	}

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
private:

	bool loading;

	bool addChatFilter(const string& aNick, const string& aText, StringMatch::Method aNickMethod = StringMatch::EXACT,
		StringMatch::Method aTextMethod = StringMatch::PARTIAL, bool aMainChat = true, bool aPM = true);
	void removeChatFilter(int pos);
	void clearChatFilters() { ChatFilterItems.clear(); }
	ChatFilterItem getChatFilter(int pos) { return ChatFilterItems[pos]; }
	void updateChatFilter(ChatFilterItem& item, int pos) { ChatFilterItems[pos] = item; }


	static TextItem texts[];
	TCHAR* title;
	ExListViewCtrl ChatFilterListCtrl;

	vector<ChatFilterItem> ChatFilterItems;
};

#endif // !defined(CHATFILTER_PAGE_H)
