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

#if !defined(PUBLIC_HUBS_FRM_H)
#define PUBLIC_HUBS_FRM_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Async.h"
#include "FlatTabCtrl.h"
#include "ExListViewCtrl.h"
#include "Resource.h"

#include <airdcpp/modules/HublistEntry.h>
#include <airdcpp/modules/HublistManagerListener.h>

#include <airdcpp/SettingsManagerListener.h>

#define FILTER_MESSAGE_MAP 8
class PublicHubsFrame : public MDITabChildWindowImpl<PublicHubsFrame>, public StaticFrame<PublicHubsFrame, ResourceManager::PUBLIC_HUBS, ID_FILE_CONNECT>, 
	private HublistManagerListener, private SettingsManagerListener, public Async<PublicHubsFrame>
{
public:
	PublicHubsFrame() : users(0), hubs(0), closed(false), filter(Util::emptyString),
		filterContainer(WC_EDIT, this, FILTER_MESSAGE_MAP) {
	}

	~PublicHubsFrame() { }

	DECLARE_FRAME_WND_CLASS_EX(_T("PublicHubsFrame"), IDR_PUBLICHUBS, 0, COLOR_3DFACE);
		
	typedef MDITabChildWindowImpl<PublicHubsFrame> baseClass;
	BEGIN_MSG_MAP(PublicHubsFrame)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, onCtlColor)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		COMMAND_ID_HANDLER(IDC_FILTER_FOCUS, onFilterFocus)
		COMMAND_ID_HANDLER(IDC_ADD, onAdd)
		COMMAND_ID_HANDLER(IDC_REFRESH, onClickedRefresh)
		COMMAND_ID_HANDLER(IDC_PUB_LIST_CONFIG, onClickedConfigure)
		COMMAND_ID_HANDLER(IDC_CONNECT, onClickedConnect)
		COMMAND_ID_HANDLER(IDC_COPY_HUB, onCopyHub);
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		NOTIFY_HANDLER(IDC_HUBLIST, LVN_COLUMNCLICK, onColumnClickHublist)
		NOTIFY_HANDLER(IDC_HUBLIST, NM_RETURN, onEnter)
		NOTIFY_HANDLER(IDC_HUBLIST, NM_DBLCLK, onDoubleClickHublist)
		COMMAND_HANDLER(IDC_PUB_LIST_DROPDOWN, CBN_SELCHANGE, onListSelChanged)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(FILTER_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CHAR, onFilterChar)
	END_MSG_MAP()
		
	LRESULT onFilterChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onDoubleClickHublist(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onEnter(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onFilterFocus(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onClickedRefresh(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onClickedConfigure(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onClickedConnect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);	
	LRESULT onCopyHub(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onListSelChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onColumnClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE);
	bool checkNick();
	
	LRESULT onCtlColor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	
	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlHubs.SetFocus();
		return 0;
	}
	static string id;
private:

	void connectHub(int pos);
	enum {
		COLUMN_FIRST,
		COLUMN_NAME = COLUMN_FIRST,
		COLUMN_DESCRIPTION,
		COLUMN_USERS,
		COLUMN_SERVER,
		COLUMN_COUNTRY,
		COLUMN_SHARED,
		COLUMN_MINSHARE,
		COLUMN_MINSLOTS,
		COLUMN_MAXHUBS,
		COLUMN_MAXUSERS,
		COLUMN_RELIABILITY,
		COLUMN_RATING,
		COLUMN_LAST
	};

	enum FilterModes{
		NONE,
		EQUAL,
		GREATER_EQUAL,
		LESS_EQUAL,
		GREATER,
		LESS,
		NOT_EQUAL
	};

	int visibleHubs;
	int users;
	CStatusBarCtrl ctrlStatus;
	CButton ctrlConfigure;
	CButton ctrlRefresh;
	CButton ctrlLists;
	CButton ctrlFilterDesc;
	CEdit ctrlFilter;
	CMenu hubsMenu;

	CContainedWindow filterContainer;
	CComboBox ctrlPubLists;
	CComboBox ctrlFilterSel;
	ExListViewCtrl ctrlHubs;

	HublistEntry::List hubs;
	string filter;

	bool closed;
	
	static int columnIndexes[];
	static int columnSizes[];
	
	void onFinished(const tstring& aStatus);
	void setStatusText(const tstring& aStr);
	void updateStatus();
	void updateList();
	void updateDropDown();

	bool parseFilter(FilterModes& mode, double& size);
	bool matchFilter(const HublistEntry& entry, const int& sel, bool doSizeCompare, const FilterModes& mode, const double& size);

	void on(HublistManagerListener::DownloadStarting, const string& l) noexcept;
	void on(HublistManagerListener::DownloadFailed, const string& l) noexcept;
	void on(HublistManagerListener::DownloadFinished, const string& l, bool /*fromCoral*/) noexcept;
	void on(HublistManagerListener::LoadedFromCache, const string& l, const string& /*d*/) noexcept;
	void on(HublistManagerListener::Corrupted, const string& l) noexcept;

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;

};

#endif // !defined(PUBLIC_HUBS_FRM_H)
