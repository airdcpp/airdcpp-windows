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


#ifndef DCPP_WINGUI_FAVORITE_HUBS_FRAME
#define DCPP_WINGUI_FAVORITE_HUBS_FRAME

#include <windows/frames/StaticFrame.h>
#include <windows/components/ExListViewCtrl.h>

#include <airdcpp/favorites/FavoriteManagerListener.h>
#include <airdcpp/hub/ClientManagerListener.h>
#include <airdcpp/hub/Client.h>

#define SERVER_MESSAGE_MAP 7

namespace wingui {
class FavoriteHubsFrame : public MDITabChildWindowImpl<FavoriteHubsFrame>, public StaticFrame<FavoriteHubsFrame, ResourceManager::FAVORITE_HUBS, IDC_FAVORITES>,
	private Async<FavoriteHubsFrame>, private FavoriteManagerListener, private SettingsManagerListener, private ClientManagerListener
{
public:
	typedef MDITabChildWindowImpl<FavoriteHubsFrame> baseClass;

	FavoriteHubsFrame() : nosave(true), closed(false) { }
	~FavoriteHubsFrame() {  }

	DECLARE_FRAME_WND_CLASS_EX(_T("FavoriteHubsFrame"), IDR_FAVORITES, 0, COLOR_3DFACE);
		
	BEGIN_MSG_MAP(FavoriteHubsFrame)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		COMMAND_ID_HANDLER(IDC_CONNECT, onClickedConnect)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_EDIT, onEdit)
		COMMAND_ID_HANDLER(IDC_NEWFAV, onNew)
		COMMAND_ID_HANDLER(IDC_MOVE_UP, onMoveUp);
		COMMAND_ID_HANDLER(IDC_MOVE_DOWN, onMoveDown);
		COMMAND_ID_HANDLER(IDC_OPEN_HUB_LOG, onOpenHubLog)
		COMMAND_ID_HANDLER(IDC_MANAGE_GROUPS, onManageGroups)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		NOTIFY_HANDLER(IDC_HUBLIST, NM_DBLCLK, onDoubleClickHublist)
		NOTIFY_HANDLER(IDC_HUBLIST, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_HUBLIST, LVN_ITEMCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_HUBLIST, LVN_COLUMNCLICK, onColumnClickHublist)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()
		
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onDoubleClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onNew(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onMoveUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onMoveDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onOpenHubLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onManageGroups(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onColumnClickHublist(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	bool checkNick();
	void UpdateLayout(BOOL bResizeBars = TRUE);
	
	LRESULT onEnter(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
		openSelected();
		return 0;
	}

	LRESULT onClickedConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		openSelected();
		return 0;
	}
	
	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}
	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlHubs.SetFocus();
		return 0;
	}
	
	static string id;

private:
	class ItemInfo {
	public:
		explicit ItemInfo(const FavoriteHubEntryPtr& aHub) : hub(aHub) {}
		~ItemInfo() = default;

		FavoriteHubEntryPtr hub = nullptr;
	};

	using ItemInfoMap = unordered_map<FavoriteHubToken, unique_ptr<ItemInfo>>;
	ItemInfoMap itemInfos;

	enum {
		COLUMN_FIRST,
		COLUMN_NAME = COLUMN_FIRST,
		COLUMN_DESCRIPTION,
		COLUMN_NICK,
		COLUMN_PASSWORD,
		COLUMN_SERVER,
		COLUMN_USERDESCRIPTION,
		COLUMN_SHAREPROFILE,
		COLUMN_LAST
	};
	
	struct StateKeeper {
		StateKeeper(ExListViewCtrl& hubs_, bool ensureVisible_ = true);
		~StateKeeper();

		const FavoriteHubEntryList& getSelection() const;

	private:
		ExListViewCtrl& hubs;
		bool ensureVisible;
		FavoriteHubEntryList selected;
		int scroll;
	};

	CButton ctrlConnect;
	CButton ctrlRemove;
	CButton ctrlNew;
	CButton ctrlProps;
	CButton ctrlUp;
	CButton ctrlDown;
	CButton ctrlManageGroups;

	CImageListManaged onlineStatusImg;
	StringList onlineHubs;
	bool isOnline(const string& hubUrl) {
		return find(onlineHubs.begin(), onlineHubs.end(), hubUrl) != onlineHubs.end();
	}

	ExListViewCtrl ctrlHubs;

	bool nosave;
	bool closed;
	
	static int columnSizes[COLUMN_LAST];
	static int columnIndexes[COLUMN_LAST];

	void addEntry(const FavoriteHubEntryPtr& entry, int pos, int groupIndex);
	void handleMove(bool up);
	TStringList getSortedGroups() const;
	void fillList();
	void openSelected();

	void on(FavoriteManagerListener::FavoriteHubAdded, const FavoriteHubEntryPtr& /*e*/)  noexcept override;
	void on(FavoriteManagerListener::FavoriteHubRemoved, const FavoriteHubEntryPtr& e) noexcept override;
	void on(FavoriteManagerListener::FavoriteHubsUpdated) noexcept override;

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept override;

	void on(ClientManagerListener::ClientConnected, const ClientPtr& c) noexcept override;
	void on(ClientManagerListener::ClientDisconnected, const string& aHubUrl) noexcept override;
};
}

#endif // !defined(FAVORITE_HUBS_FRM_H)