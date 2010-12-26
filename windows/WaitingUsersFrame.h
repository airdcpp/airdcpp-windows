/* 
 * Copyright (C) 2001-2003 BlackClaw, blackclaw@parsoma.net
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


#if !defined(WAITING_QUEUE_FRAME_H)
#define WAITING_QUEUE_FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"
#include "WinUtil.h"
#include "../client/TimerManager.h"
#include "../client/UploadManager.h"

#define SHOWTREE_MESSAGE_MAP 12

class WaitingUsersFrame : public MDITabChildWindowImpl<WaitingUsersFrame>, public StaticFrame<WaitingUsersFrame, ResourceManager::WAITING_USERS, IDC_UPLOAD_QUEUE>,
	private UploadManagerListener, public CSplitterImpl<WaitingUsersFrame>, private SettingsManagerListener, public UserInfoBaseHandler<WaitingUsersFrame>
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("WaitingUsersFrame"), IDR_UPLOAD_QUEUE, 0, COLOR_3DFACE);

	WaitingUsersFrame() : showTree(true), closed(false), usingUserMenu(false), 
		showTreeContainer(_T("BUTTON"), this, SHOWTREE_MESSAGE_MAP) { }
	
	~WaitingUsersFrame() { }

	enum {
		ADD_ITEM,
		REMOVE,
		REMOVE_ITEM,
		UPDATE_ITEMS
	};

	typedef MDITabChildWindowImpl<WaitingUsersFrame> baseClass;
	typedef CSplitterImpl<WaitingUsersFrame> splitBase;
	typedef UserInfoBaseHandler<WaitingUsersFrame> uibBase;

	// Inline message map
	BEGIN_MSG_MAP(WaitingUsersFrame)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		COMMAND_HANDLER(IDC_REMOVE, BN_CLICKED, onRemove)
		NOTIFY_HANDLER(IDC_UPLOAD_QUEUE, LVN_GETDISPINFO, ctrlList.onGetDispInfo)
		NOTIFY_HANDLER(IDC_UPLOAD_QUEUE, LVN_COLUMNCLICK, ctrlList.onColumnClick)
//		NOTIFY_HANDLER(IDC_UPLOAD_QUEUE, LVN_ITEMCHANGED, onItemChangedQueue)
		NOTIFY_HANDLER(IDC_UPLOAD_QUEUE, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_SELCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_KEYDOWN, onKeyDownDirs)
		NOTIFY_HANDLER(IDC_UPLOAD_QUEUE, NM_CUSTOMDRAW, onCustomDraw)
		CHAIN_COMMANDS(uibBase)
		CHAIN_MSG_MAP(splitBase)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(SHOWTREE_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onShowTree)
	END_MSG_MAP()

	// Message handlers
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);

	LRESULT onShowTree(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		showTree = (wParam == BST_CHECKED);
		UpdateLayout(FALSE);
		DeleteAll();
		LoadAll();
		return 0;
	}
	
	// Update control layouts
	void UpdateLayout(BOOL bResizeBars = TRUE);
	
	struct UserListHandler {
		UserListHandler(TypedListViewCtrl<UploadQueueItem, IDC_UPLOAD_QUEUE>& a, CTreeViewCtrl& b, bool c) : ctrlList(a), ctrlQueued(b), usingUserMenu(c) { }
		
		void forEachSelected(void (UserInfoBase::*func)()) {
			if(usingUserMenu) {
				HTREEITEM selectedItem = ctrlQueued.GetSelectedItem();
				UserItem* ui = reinterpret_cast<UserItem*>(ctrlQueued.GetItemData(selectedItem));
				if(selectedItem && ui)
					(ui->*func)();
			} else {
				ctrlList.forEachSelected(func);
			}
		}
		
		template<class _Function>
		_Function forEachSelectedT(_Function pred) {
			if(usingUserMenu) {
				HTREEITEM selectedItem = ctrlQueued.GetSelectedItem();
				UserItem* ui = reinterpret_cast<UserItem*>(ctrlQueued.GetItemData(selectedItem));
				if(selectedItem && ui)
					pred(ui);
				return pred;
			} else {
				return ctrlList.forEachSelectedT(pred);
			}		
		}
		
	private:
		TypedListViewCtrl<UploadQueueItem, IDC_UPLOAD_QUEUE>& ctrlList;
		bool usingUserMenu;
		CTreeViewCtrl& ctrlQueued;		
	};
	
	UserListHandler getUserList() { return UserListHandler(ctrlList, ctrlQueued, usingUserMenu); }

private:
	static int columnSizes[UploadQueueItem::COLUMN_LAST];
	static int columnIndexes[UploadQueueItem::COLUMN_LAST];

	struct UserItem : UserInfoBase {
		UserPtr u;
		UserItem(UserPtr u) : u(u) { }
		
		const UserPtr& getUser() const { return u; }
	};
		
	const UserPtr getSelectedUser() {
		HTREEITEM selectedItem = ctrlQueued.GetSelectedItem();
		UserItem* ui = reinterpret_cast<UserItem*>(ctrlQueued.GetItemData(selectedItem));
		return (selectedItem && ui) ? ui->u : UserPtr(NULL);
	}

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
		if(kd->wVKey == VK_DELETE) {
			removeSelected();
		}
		return 0;
	}

	LRESULT onKeyDownDirs(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMTVKEYDOWN* kd = (NMTVKEYDOWN*) pnmh;
		if(kd->wVKey == VK_DELETE) {
			removeSelectedUser();
		}
		return 0;
	}

	void removeSelected() {
		int i = -1;
		UserList RemoveUsers;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			// Ok let's cheat here, if you try to remove more users here is not working :(
			RemoveUsers.push_back(((UploadQueueItem*)ctrlList.getItemData(i))->getUser());
		}
		for(UserList::const_iterator i = RemoveUsers.begin(); i != RemoveUsers.end(); ++i) {
			UploadManager::getInstance()->clearUserFiles(*i);
		}
		updateStatus();
	}
	
	void removeSelectedUser() {
		UserPtr User = getSelectedUser();
		if(User) {
			UploadManager::getInstance()->clearUserFiles(User);
		}
		updateStatus();
	}

	// Communication with manager
	void LoadAll();
	void DeleteAll();

	HTREEITEM rootItem;

	// Contained controls
	CButton ctrlShowTree;
	CContainedWindow showTreeContainer;
	
	bool showTree;
	bool closed;
	bool usingUserMenu;
	
	TypedListViewCtrl<UploadQueueItem, IDC_UPLOAD_QUEUE> ctrlList;
	CTreeViewCtrl ctrlQueued;
	
	CStatusBarCtrl ctrlStatus;
	int statusSizes[4];
	
	void AddFile(UploadQueueItem* aUQI);
	void RemoveUser(const UserPtr& aUser);
	
	void addAllFiles(Upload * /*aUser*/);
	void updateStatus();

	// UploadManagerListener
	void on(UploadManagerListener::QueueAdd, UploadQueueItem* aUQI) throw() { PostMessage(WM_SPEAKER, ADD_ITEM, (LPARAM)aUQI); }
	void on(UploadManagerListener::QueueRemove, const UserPtr& aUser) throw() { PostMessage(WM_SPEAKER, REMOVE, (LPARAM)new UserItem(aUser));	}
	void on(UploadManagerListener::QueueItemRemove, UploadQueueItem* aUQI) throw() { aUQI->inc(); PostMessage(WM_SPEAKER, REMOVE_ITEM, (LPARAM)aUQI); }
	void on(UploadManagerListener::QueueUpdate) throw() { PostMessage(WM_SPEAKER, UPDATE_ITEMS, NULL); }

	// SettingsManagerListener
	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw();
};

#endif
