/*
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(SPY_FRAME_H)
#define SPY_FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <airdcpp/search/SearchManagerListener.h>
#include <airdcpp/core/timer/TimerManagerListener.h>

#include <windows/StaticFrame.h>
#include <windows/ExListViewCtrl.h>

#define IGNORETTH_MESSAGE_MAP 7

namespace wingui {
class SpyFrame : public MDITabChildWindowImpl<SpyFrame>, public StaticFrame<SpyFrame, ResourceManager::SEARCH_SPY, IDC_SEARCH_SPY>,
	private SearchManagerListener, private TimerManagerListener, private SettingsManagerListener
{
public:
	SpyFrame() : ignoreTth(SETTING(SPY_FRAME_IGNORE_TTH_SEARCHES)), ignoreTthContainer(WC_BUTTON, this, IGNORETTH_MESSAGE_MAP) {
		memzero(perSecond, sizeof(perSecond));
	}

	~SpyFrame() { }

	enum {
		COLUMN_FIRST,
		COLUMN_STRING = COLUMN_FIRST,
		COLUMN_COUNT,
		COLUMN_TIME,
		COLUMN_LAST
	};

	static int columnIndexes[COLUMN_LAST];
	static int columnSizes[COLUMN_LAST];

	DECLARE_FRAME_WND_CLASS_EX(_T("SpyFrame"), IDR_SPY, 0, COLOR_3DFACE)

	typedef MDITabChildWindowImpl<SpyFrame> baseClass;
	BEGIN_MSG_MAP(SpyFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		COMMAND_ID_HANDLER(IDC_SEARCH, onSearch)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		NOTIFY_HANDLER(IDC_RESULTS, LVN_COLUMNCLICK, onColumnClickResults)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(IGNORETTH_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onIgnoreTth)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onColumnClickResults(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}
	void UpdateLayout(BOOL bResizeBars = TRUE);

	LRESULT onIgnoreTth(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		ignoreTth = (wParam == BST_CHECKED);
		return 0;
	}

	static string id;
	
private:
	struct IncomingSearchInfo {
		tstring str;
		bool hasResults;
	};

	void updateStatus() noexcept;
	void addList(const tstring& aText) noexcept;

	enum { AVG_TIME = 60 };
	enum {
		INCOMING_SEARCH,
		OUTGOING_RESPONSES,
		TICK_AVG
	};

	ExListViewCtrl ctrlSearches;
	CStatusBarCtrl ctrlStatus;
	CContainedWindow ignoreTthContainer;
	CButton ctrlIgnoreTth;
	int total = 0;
	int hits = 0;
	int perSecond[AVG_TIME];
	int cur = 0;
	tstring searchString;

	bool closed = false;
	bool ignoreTth;

	// SearchManagerListener
	void on(SearchManagerListener::IncomingSearch, Client* aClient, const OnlineUserPtr& aAdcUser, const SearchQuery& aQuery, const SearchResultList& aResults, bool) noexcept override;
	
	// TimerManagerListener
	void on(TimerManagerListener::Second, uint64_t) noexcept override;
	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept override;
};

}

#endif // !defined(SPY_FRAME_H)
