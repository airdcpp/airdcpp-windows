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
#include "../client/FavoriteManager.h"

class RecentHubsFrame : public MDITabChildWindowImpl<RecentHubsFrame>, public StaticFrame<RecentHubsFrame, ResourceManager::RECENT_HUBS, IDC_RECENTS>, 
	private FavoriteManagerListener, private SettingsManagerListener
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
	
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
		if(reinterpret_cast<HWND>(wParam) == ctrlHubs && ctrlHubs.GetSelectedCount() > 0) {
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 

			CRect rc;
			ctrlHubs.GetHeader().GetWindowRect(&rc);
			if (PtInRect(&rc, pt)) {
				return 0;
			}

	        if(pt.x == -1 && pt.y == -1) {
			    WinUtil::getContextMenuPos(ctrlHubs, pt);
            }	
		
			hubsMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

			return TRUE; 
		}
		
		return FALSE; 
	}
	
	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlHubs.SetFocus();
		return 0;
	}

	LRESULT onColumnClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLISTVIEW* l = (NMLISTVIEW*)pnmh;
		if(l->iSubItem == ctrlHubs.getSortColumn()) {
			if (!ctrlHubs.isAscending())
				ctrlHubs.setSort(-1, ctrlHubs.getSortType());
			else
				ctrlHubs.setSortDirection(false);
		} else {
			if(l->iSubItem == 2 || l->iSubItem == 3) {
				ctrlHubs.setSort(l->iSubItem, ExListViewCtrl::SORT_INT);
			} else {
				ctrlHubs.setSort(l->iSubItem, ExListViewCtrl::SORT_STRING_NOCASE);
			}
		}
		return 0;
	}
private:
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
	
	void updateList(const RecentHubEntry::List& fl) {
		ctrlHubs.SetRedraw(FALSE);
		for(RecentHubEntry::List::const_iterator i = fl.begin(); i != fl.end(); ++i) {
			addEntry(*i, ctrlHubs.GetItemCount());
		}
		ctrlHubs.SetRedraw(TRUE);
		ctrlHubs.Invalidate();
	}

	void addEntry(const RecentHubEntry* entry, int pos) {
		TStringList l;
		l.push_back(Text::toT(entry->getName()));
		l.push_back(Text::toT(entry->getDescription()));
		l.push_back(Text::toT(entry->getUsers()));
		l.push_back(Text::toT(Util::formatBytes(entry->getShared())));
		l.push_back(Text::toT(entry->getServer()));

		ctrlHubs.insert(pos, l, 0, (LPARAM)entry);
	}
	
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
		if(kd->wVKey == VK_DELETE) {
			int i = -1;
			while( (i = ctrlHubs.GetNextItem(-1, LVNI_SELECTED)) != -1) {
				FavoriteManager::getInstance()->removeRecent((RecentHubEntry*)ctrlHubs.GetItemData(i));
			}
		}
		return 0;
	}

	
	void on(RecentAdded, const RecentHubEntry* entry) noexcept { addEntry(entry, ctrlHubs.GetItemCount()); }
	void on(RecentRemoved, const RecentHubEntry* entry) noexcept { ctrlHubs.DeleteItem(ctrlHubs.find((LPARAM)entry)); }
	void on(RecentUpdated, const RecentHubEntry* entry) noexcept {
		int i = -1;
		if((i = ctrlHubs.find((LPARAM)entry)) != -1) {
			ctrlHubs.SetItemText(i, COLUMN_NAME, Text::toT(entry->getName()).c_str());
			ctrlHubs.SetItemText(i, COLUMN_DESCRIPTION, Text::toT(entry->getDescription()).c_str());
			ctrlHubs.SetItemText(i, COLUMN_USERS, Text::toT(entry->getUsers()).c_str());
			ctrlHubs.SetItemText(i, COLUMN_SHARED, Text::toT(Util::formatBytes(entry->getShared())).c_str());
			ctrlHubs.SetItemText(i, COLUMN_SERVER, Text::toT(entry->getServer()).c_str());
		}
	}
	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;
};

#endif