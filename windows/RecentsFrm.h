/* 
 * 
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

#if !defined(__RECENTS_FRAME_H__)
#define __RECENTS_FRAME_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "ExListViewCtrl.h"

#include <airdcpp/RecentManagerListener.h>

class RecentHubsFrame : public MDITabChildWindowImpl<RecentHubsFrame>, public StaticFrame<RecentHubsFrame, ResourceManager::RECENT_HUBS, IDC_RECENTS>, 
	private RecentManagerListener, private SettingsManagerListener
{
public:
	typedef MDITabChildWindowImpl<RecentHubsFrame> baseClass;
		
	RecentHubsFrame() : closed(false) { };
	~RecentHubsFrame() { };

	DECLARE_FRAME_WND_CLASS_EX(_T("RecentHubsFrame"), IDR_RECENTS, 0, COLOR_3DFACE);
		
	void OnFinalMessage(HWND /*hWnd*/) {
		delete this;
	}

	BEGIN_MSG_MAP(RecentHubsFrame)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		COMMAND_ID_HANDLER(IDC_CONNECT, onClickedConnect)
		COMMAND_ID_HANDLER(IDC_ADD, onAdd)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_REMOVE_ALL, onRemoveAll)
		COMMAND_ID_HANDLER(IDC_EDIT, onEdit)
		NOTIFY_HANDLER(IDC_RECENTS, LVN_COLUMNCLICK, onColumnClickHublist)
		NOTIFY_HANDLER(IDC_RECENTS, NM_DBLCLK, onDoubleClickHublist)
		NOTIFY_HANDLER(IDC_RECENTS, NM_RETURN, onEnter)
		NOTIFY_HANDLER(IDC_RECENTS, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_RECENTS, LVN_ITEMCHANGED, onItemchangedDirectories)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()
		
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onDoubleClickHublist(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onEnter(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onClickedConnect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onRemoveAll(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onItemchangedDirectories(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	void UpdateLayout(BOOL bResizeBars = TRUE);
	
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL & /*bHandled*/);
	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/);
	LRESULT onColumnClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL & /*bHandled*/);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL & /*bHandled*/);
private:
	void connectSelected();
	enum {
		COLUMN_FIRST,
		COLUMN_NAME = COLUMN_FIRST,
		COLUMN_DESCRIPTION,
		COLUMN_USERS,
		COLUMN_SHARED,
		COLUMN_SERVER,
		COLUMN_LAST
	};
	
	CButton ctrlConnect;
	CButton ctrlRemove;
	CButton ctrlRemoveAll;
	CMenu hubsMenu;
	
	ExListViewCtrl ctrlHubs;

	bool closed;
	
	static int columnSizes[COLUMN_LAST];
	static int columnIndexes[COLUMN_LAST];
	
	void updateList(const RecentHubEntryList& fl);
	void addEntry(const RecentHubEntryPtr& entry, int pos);

	
	void on(RecentManagerListener::RecentHubAdded, const RecentHubEntryPtr& entry) noexcept { addEntry(entry, ctrlHubs.GetItemCount()); }
	void on(RecentManagerListener::RecentHubRemoved, const RecentHubEntryPtr& entry) noexcept { ctrlHubs.DeleteItem(ctrlHubs.find((LPARAM)entry.get())); }
	void on(RecentManagerListener::RecentHubUpdated, const RecentHubEntryPtr& entry) noexcept;

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;
};

#endif