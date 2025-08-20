/* 
 * Copyright (C) 2001-2003 BlackClaw, blackclaw@parsoma.net
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


#ifndef DCPP_WINGUI_UPLOAD_QUEUE_FRAME
#define DCPP_WINGUI_UPLOAD_QUEUE_FRAME

#include <windows/frames/StaticFrame.h>
#include <windows/components/TypedListViewCtrl.h>
#include <windows/UserInfoBaseHandler.h>

#include <airdcpp/user/UserInfoBase.h>
#include <airdcpp/transfer/upload/UploadQueueManagerListener.h>

#define SHOWTREE_MESSAGE_MAP 12

namespace wingui {
class UploadQueueFrame : public MDITabChildWindowImpl<UploadQueueFrame>, private Async<UploadQueueFrame>, public StaticFrame<UploadQueueFrame, ResourceManager::UPLOAD_QUEUE, IDC_UPLOAD_QUEUE>,
	private UploadQueueManagerListener, public CSplitterImpl<UploadQueueFrame>, private SettingsManagerListener, public UserInfoBaseHandler<UploadQueueFrame>
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("UploadQueueFrame"), IDR_UPLOAD_QUEUE, 0, COLOR_3DFACE);

	UploadQueueFrame();
	
	~UploadQueueFrame() { }

	typedef MDITabChildWindowImpl<UploadQueueFrame> baseClass;
	typedef CSplitterImpl<UploadQueueFrame> splitBase;
	typedef UserInfoBaseHandler<UploadQueueFrame> uibBase;

	// Inline message map
	BEGIN_MSG_MAP(UploadQueueFrame)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		COMMAND_HANDLER(IDC_REMOVE, BN_CLICKED, onRemove)
		NOTIFY_HANDLER(IDC_UPLOAD_QUEUE, LVN_GETDISPINFO, ctrlList.onGetDispInfo)
		NOTIFY_HANDLER(IDC_UPLOAD_QUEUE, LVN_COLUMNCLICK, ctrlList.onColumnClick)
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

	LRESULT onShowTree(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		showTree = (wParam == BST_CHECKED);
		UpdateLayout(FALSE);
		DeleteAll();
		LoadAll();
		return 0;
	}

	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}
	
	// Update control layouts
	void UpdateLayout(BOOL bResizeBars = TRUE);

private:
	class ItemInfo : public UserInfoBase {
	public:
		explicit ItemInfo(const UploadQueueItemPtr& aUQI) : uqi(aUQI) {}
		~ItemInfo() = default;

		static int compareItems(const ItemInfo* a, const ItemInfo* b, uint8_t col) noexcept;

		enum {
			COLUMN_FIRST,
			COLUMN_FILE = COLUMN_FIRST,
			COLUMN_PATH,
			COLUMN_NICK,
			COLUMN_HUB,
			COLUMN_TRANSFERRED,
			COLUMN_SIZE,
			COLUMN_ADDED,
			COLUMN_WAITING,
			COLUMN_LAST
		};

		const tstring getText(uint8_t col) const noexcept;
		int getImageIndex() const noexcept;

		const UserPtr& getUser() const noexcept override { return uqi->getHintedUser().user; }
		const string& getHubUrl() const noexcept override { return uqi->getHintedUser().hint; }

		UploadQueueItemPtr uqi = nullptr;
	};

public:

	
	using ListType = TypedListViewCtrl<ItemInfo, IDC_UPLOAD_QUEUE>;

	struct UserListHandler {
		UserListHandler(ListType& a, CTreeViewCtrl& b, bool c) : ctrlList(a), ctrlQueued(b), usingUserMenu(c) { }
		
		void forEachSelected(void (UserInfoBase::*func)()) {
			if(usingUserMenu) {
				HTREEITEM selectedItem = ctrlQueued.GetSelectedItem();
				auto ui = reinterpret_cast<UserItem*>(ctrlQueued.GetItemData(selectedItem));
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
				auto ui = reinterpret_cast<UserItem*>(ctrlQueued.GetItemData(selectedItem));
				if(selectedItem && ui)
					pred(ui);
				return pred;
			} else {
				return ctrlList.forEachSelectedT(pred);
			}		
		}
		
	private:
		ListType& ctrlList;
		bool usingUserMenu;
		CTreeViewCtrl& ctrlQueued;		
	};
	
	UserListHandler getUserList() { return UserListHandler(ctrlList, ctrlQueued, usingUserMenu); }

	static string id;
private:
	using ItemInfoMap = unordered_map<UploadQueueItemToken, unique_ptr<ItemInfo>>;
	ItemInfoMap itemInfos;

	static int columnSizes[ItemInfo::COLUMN_LAST];
	static int columnIndexes[ItemInfo::COLUMN_LAST];

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
	
	bool showTree = true;
	bool closed = false;
	bool usingUserMenu = true;
	
	ListType ctrlList;
	CTreeViewCtrl ctrlQueued;
	
	CStatusBarCtrl ctrlStatus;
	int statusSizes[4];
	
	void AddFile(const UploadQueueItemPtr& aUQI);
	void RemoveUser(const UserPtr& aUser);

	void updateStatus();

	// UploadManagerListener
	void on(UploadQueueManagerListener::QueueAdd, const UploadQueueItemPtr& aUQI) noexcept override;
	void on(UploadQueueManagerListener::QueueUserRemove, const UserPtr& aUser) noexcept override;
	void on(UploadQueueManagerListener::QueueItemRemove, const UploadQueueItemPtr& aUQI) noexcept override;
	void on(UploadQueueManagerListener::QueueUpdate) noexcept override;

	// SettingsManagerListener
	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept override;
};
}

#endif
