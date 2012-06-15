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

#if !defined(USERS_FRAME_H)
#define USERS_FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"
#include "MenuBaseHandlers.h"

#include "../client/FavoriteManager.h"
#include "../client/File.h"
#include "../client/OnlineUser.h"

class UsersFrame : public MDITabChildWindowImpl<UsersFrame>, public StaticFrame<UsersFrame, ResourceManager::FAVORITE_USERS, IDC_FAVUSERS>,
	private FavoriteManagerListener, private ClientManagerListener, public UserInfoBaseHandler<UsersFrame>, private SettingsManagerListener {
public:
	
	UsersFrame() : closed(false), startup(true) { }
	~UsersFrame() { images.Destroy(); }

	DECLARE_FRAME_WND_CLASS_EX(_T("UsersFrame"), IDR_USERS, 0, COLOR_3DFACE);
		
	typedef MDITabChildWindowImpl<UsersFrame> baseClass;
	typedef UserInfoBaseHandler<UsersFrame> uibBase;

	BEGIN_MSG_MAP(UsersFrame)
		NOTIFY_HANDLER(IDC_USERS, LVN_GETDISPINFO, ctrlUsers.onGetDispInfo)
		NOTIFY_HANDLER(IDC_USERS, LVN_COLUMNCLICK, ctrlUsers.onColumnClick)
		NOTIFY_HANDLER(IDC_USERS, LVN_ITEMCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_USERS, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_USERS, NM_DBLCLK, onDoubleClick)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_EDIT, onEdit)
		COMMAND_ID_HANDLER(IDC_OPEN_USER_LOG, onOpenUserLog)
		CHAIN_MSG_MAP(uibBase)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()
		
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);

	void UpdateLayout(BOOL bResizeBars = TRUE);

	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	
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
		COLUMN_NICK = COLUMN_FIRST,
		COLUMN_HUB,
		COLUMN_SEEN,
		COLUMN_DESCRIPTION,
		COLUMN_LAST
	};

	enum {
		USER_UPDATED
	};

	class UserInfo : public UserInfoBase {
	public:
		UserInfo(const FavoriteUser& u) : user(u.getUser()) { 
			update(u);
		}

		inline const TCHAR* getText(int col) const { return columns[col].c_str(); }

		inline static int compareItems(const UserInfo* a, const UserInfo* b, int col) {
			return Util::DefaultSort(a->columns[col].c_str(), b->columns[col].c_str());
		}
		
		int getImageIndex() const { return 2; }

		void remove() { FavoriteManager::getInstance()->removeFavoriteUser(getUser()); }

		void update(const FavoriteUser& u);

		tstring columns[COLUMN_LAST];

		const UserPtr& getUser() const { return user; }
		const string& getHubUrl() const { return Util::emptyString; } //TODO, FIX
		UserPtr user;
	};

	CStatusBarCtrl ctrlStatus;
		
	TypedListViewCtrl<UserInfo, IDC_USERS> ctrlUsers;
	CImageList images;	

	bool closed;
	
	bool startup;
	static int columnSizes[COLUMN_LAST];
	static int columnIndexes[COLUMN_LAST];

	// FavoriteManagerListener
	void on(UserAdded, const FavoriteUser& aUser) noexcept { addUser(aUser); }
	void on(UserRemoved, const FavoriteUser& aUser) noexcept { removeUser(aUser); }
	void on(StatusChanged, const UserPtr& aUser) noexcept { PostMessage(WM_SPEAKER, (WPARAM)USER_UPDATED, (LPARAM)new Identity(aUser, 0)); }

	void on(SettingsManagerListener::Save, SimpleXML&s /*xml*/) noexcept;

	void addUser(const FavoriteUser& aUser);
	void updateUser(const UserPtr& aUser);
	void removeUser(const FavoriteUser& aUser);
};

#endif // !defined(USERS_FRAME_H)

/**
 * @file
 * $Id: UsersFrame.h 568 2011-07-24 18:28:43Z bigmuscle $
 */
