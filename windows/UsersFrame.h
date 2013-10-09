/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(USERS_FRAME_H)
#define USERS_FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Async.h"
#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"
#include "UserInfoBaseHandler.h"
#include "RichTextBox.h"
#include "ListFilter.h"

#include "../client/FavoriteManager.h"
#include "../client/UploadManager.h"
#include "../client/QueueManagerListener.h"

#define STATUS_MAP 10

class UsersFrame : public MDITabChildWindowImpl<UsersFrame>, public StaticFrame<UsersFrame, ResourceManager::USERS, IDC_FAVUSERS>,
	public CSplitterImpl<UsersFrame>, private FavoriteManagerListener, private ClientManagerListener, public UserInfoBaseHandler<UsersFrame>, 
	private SettingsManagerListener, private UploadManagerListener, private QueueManagerListener, private Async<UsersFrame> {
public:
	
	UsersFrame();
	~UsersFrame() { images.Destroy(); }

	DECLARE_FRAME_WND_CLASS_EX(_T("UsersFrame"), IDR_USERS, 0, COLOR_3DFACE);
		
	typedef MDITabChildWindowImpl<UsersFrame> baseClass;
	typedef UserInfoBaseHandler<UsersFrame> uibBase;
	typedef CSplitterImpl<UsersFrame> splitBase;

	BEGIN_MSG_MAP(UsersFrame)
		NOTIFY_HANDLER(IDC_USERS, LVN_GETDISPINFO, ctrlUsers.onGetDispInfo)
		NOTIFY_HANDLER(IDC_USERS, LVN_COLUMNCLICK, ctrlUsers.onColumnClick)
		NOTIFY_HANDLER(IDC_USERS, LVN_GETINFOTIP, ctrlUsers.onInfoTip)
		NOTIFY_HANDLER(IDC_USERS, LVN_ITEMCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_USERS, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_USERS, NM_DBLCLK, ctrlUsers.onDoubleClick)
		NOTIFY_HANDLER(IDC_USERS, NM_DBLCLK, onDoubleClick)
		NOTIFY_HANDLER(IDC_USERS, NM_CLICK, ctrlUsers.onClick)
		NOTIFY_HANDLER(IDC_USERS, NM_CUSTOMDRAW, onCustomDrawList)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_OPEN_USER_LOG, onOpenUserLog)
		CHAIN_MSG_MAP(uibBase)
		CHAIN_MSG_MAP(splitBase)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(STATUS_MAP)
		COMMAND_ID_HANDLER(IDC_SHOW_INFO, onShow)
		COMMAND_ID_HANDLER(IDC_SHOW_FAV, onShow)
		COMMAND_ID_HANDLER(IDC_FILTER_QUEUED, onShow)
		COMMAND_ID_HANDLER(IDC_SHOW_ONLINE, onShow)
		CHAIN_MSG_MAP_MEMBER(filter)
	END_MSG_MAP()
		
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onInfoTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onShow(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCustomDrawList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	void UpdateLayout(BOOL bResizeBars = TRUE);
	
	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlUsers.SetFocus();
		return 0;
	}

private:
	class UserInfo;
public:
	TypedListViewCtrl<UserInfo, IDC_USERS>& getUserList() { return ctrlUsers; }
private:
	enum {
		COLUMN_FIRST,
		COLUMN_FAVORITE = COLUMN_FIRST,
		COLUMN_SLOT,
		COLUMN_NICK,
		COLUMN_HUB,
		COLUMN_SEEN,
		COLUMN_QUEUED,
		COLUMN_DESCRIPTION,
		COLUMN_LIMITER,
		COLUMN_LAST
	};

	enum {
		FAVORITE_ON_ICON,
		FAVORITE_OFF_ICON,
		USER_ON_ICON,
		USER_OFF_ICON,
		GRANT_ON_ICON,
		GRANT_OFF_ICON

	};

	enum {
		USER_UPDATED
	};

	class UserInfo : public UserInfoBase {
	public:
		UserInfo(const UserPtr& u, const string& aUrl, bool updateInfo = true);
		~UserInfo(){ }

		inline const tstring& getText(int col) const { return columns[col]; }

		static int compareItems(const UserInfo* a, const UserInfo* b, int col);
		
		int getImageIndex() const { return -1; }

		int getImage(int col) const;

		void remove() { FavoriteManager::getInstance()->removeFavoriteUser(getUser()); }

		void update(const UserPtr& u);

		tstring columns[COLUMN_LAST];

		const UserPtr& getUser() const { return user; }
		GETSET(string, hubUrl, HubUrl);
		UserPtr user;

		bool noLimiter;
		bool grantSlot;
		bool isFavorite;
	};

	CStatusBarCtrl ctrlStatus;
		
	TypedListViewCtrl<UserInfo, IDC_USERS> ctrlUsers;
	CImageList images;
	RichTextBox ctrlInfo;
	CContainedWindow ctrlShowInfoContainer;
	CButton ctrlShowInfo;
	CButton ctrlShowFav;
	CButton ctrlShowQueued;
	CButton ctrlShowOnline;

	CToolTipCtrl ctrlTooltips;

	ListFilter filter;

	std::unordered_map<UserPtr, UserInfo, User::Hash> userInfos;

	bool closed;
	bool showInfo;
	bool startup;
	bool listFav;
	bool filterQueued;
	bool filterOnline;

	void updateInfoText(const UserInfo* ui);
	void updateList();
	bool matches(const UserInfo& ui);
	bool show(const UserPtr &u, bool any) const;
	void addUser(const UserPtr& aUser, const string& aUrl);
	void updateUser(const UserPtr& aUser);
	
	void setImages(const UserInfo* ui, int pos = -1);
	void updateStatus();

	bool handleClickSlot(int row);
	bool handleClickFavorite(int row);
	bool handleClickLimiter(int row);
	bool handleClickDesc(int row);

	static int columnSizes[];
	static int columnIndexes[];

	// FavoriteManagerListener
	void on(UserAdded, const FavoriteUser& aUser) noexcept;
	void on(UserRemoved, const FavoriteUser& aUser) noexcept;
	void on(StatusChanged, const UserPtr& aUser) noexcept;

	// ClientManagerListner
	void on(UserConnected, const OnlineUser& aUser, bool) noexcept;
	void on(ClientManagerListener::UserUpdated, const OnlineUser& aUser) noexcept;
	void on(ClientManagerListener::UserDisconnected, const UserPtr& aUser, bool) noexcept;

	void on(SettingsManagerListener::Save, SimpleXML&s /*xml*/) noexcept;

	void on(UploadManagerListener::SlotsUpdated, const UserPtr& aUser) noexcept;

	void on(QueueManagerListener::SourceFilesUpdated, const UserPtr& aUser) noexcept;
};

#endif // !defined(USERS_FRAME_H)
