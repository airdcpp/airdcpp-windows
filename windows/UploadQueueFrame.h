/* 
 * Copyright (C) 2001-2003 BlackClaw, blackclaw@parsoma.net
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


#if !defined(UPLOAD_QUEUE_FRAME_H)
#define UPLOAD_QUEUE_FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"
#include "UserInfoBaseHandler.h"

#include <airdcpp/UploadManager.h>

#define SHOWTREE_MESSAGE_MAP 12

class UploadQueueFrame : public MDITabChildWindowImpl<UploadQueueFrame>, public StaticFrame<UploadQueueFrame, ResourceManager::UPLOAD_QUEUE, IDC_UPLOAD_QUEUE>,
	private UploadManagerListener, public CSplitterImpl<UploadQueueFrame>, private SettingsManagerListener, public UserInfoBaseHandler<UploadQueueFrame>
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("UploadQueueFrame"), IDR_UPLOAD_QUEUE, 0, COLOR_3DFACE);

	UploadQueueFrame() : showTree(true), closed(false), usingUserMenu(false), 
		showTreeContainer(_T("BUTTON"), this, SHOWTREE_MESSAGE_MAP) { }
	
	~UploadQueueFrame() { }

	enum {
		ADD_ITEM,
		REMOVE,
		REMOVE_ITEM,
		UPDATE_ITEMS
	};

	typedef MDITabChildWindowImpl<UploadQueueFrame> baseClass;
	typedef CSplitterImpl<UploadQueueFrame> splitBase;
	typedef UserInfoBaseHandler<UploadQueueFrame> uibBase;

	// Inline message map
	BEGIN_MSG_MAP(UploadQueueFrame)
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

	static string id;
private:

	static int columnSizes[UploadQueueItem::COLUMN_LAST];
	static int columnIndexes[UploadQueueItem::COLUMN_LAST];

	struct UserItem : UserInfoBase {
		HintedUser u;
		UserItem(HintedUser u) : u(u) { }
		
		const UserPtr& getUser() const { return u.user; }
		const string& getHubUrl() const { return u.hint; }
	};
		
	const UserPtr getSelectedUser() {
		HTREEITEM selectedItem = ctrlQueued.GetSelectedItem();
		UserItem* ui = reinterpret_cast<UserItem*>(ctrlQueued.GetItemData(selectedItem));
		return (selectedItem && ui->u.user) ? ui->u.user : UserPtr();
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

	void removeSelected();
	void removeSelectedUser();

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
	void on(UploadManagerListener::QueueAdd, UploadQueueItem* aUQI) noexcept { PostMessage(WM_SPEAKER, ADD_ITEM, (LPARAM)aUQI); }
	void on(UploadManagerListener::QueueRemove, const UserPtr& aUser) noexcept { PostMessage(WM_SPEAKER, REMOVE, (LPARAM)new UserItem(HintedUser(aUser, Util::emptyString)));	}
	void on(UploadManagerListener::QueueItemRemove, UploadQueueItem* aUQI) noexcept { aUQI->inc(); PostMessage(WM_SPEAKER, REMOVE_ITEM, (LPARAM)aUQI); }
	void on(UploadManagerListener::QueueUpdate) noexcept { PostMessage(WM_SPEAKER, UPDATE_ITEMS, NULL); }

	// SettingsManagerListener
	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;
};

#endif
