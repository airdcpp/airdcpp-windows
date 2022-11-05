/*
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(HUB_FRAME_H)
#define HUB_FRAME_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"
#include "UserInfoBaseHandler.h"
#include "ChatFrameBase.h"
#include "UserUtil.h"

#include <airdcpp/Client.h>
#include <airdcpp/FastAlloc.h>
#include <airdcpp/TaskQueue.h>

#include "atlstr.h"
#include "WinUtil.h"
#include "UCHandler.h"
#include "ListFilter.h"

#define SHOW_USERS 9
#define STATUS_MSG 52
class ChatFrameBase;

class HubFrame : private ClientListener, public CSplitterImpl<HubFrame>, private FavoriteManagerListener,
	public UCHandler<HubFrame>, public UserInfoBaseHandler<HubFrame>, private SettingsManagerListener, public ChatFrameBase
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("HubFrame"), IDR_HUB, 0, COLOR_3DFACE);

	typedef ChatFrameBase chatBase;
	typedef CSplitterImpl<HubFrame> splitBase;
	typedef UserInfoBaseHandler<HubFrame> uibBase;
	
	BEGIN_MSG_MAP(HubFrame)
		NOTIFY_HANDLER(IDC_USERS, LVN_GETDISPINFO, ctrlUsers.onGetDispInfo)
		NOTIFY_HANDLER(IDC_USERS, LVN_COLUMNCLICK, ctrlUsers.onColumnClick)
		NOTIFY_HANDLER(IDC_USERS, LVN_GETINFOTIP, ctrlUsers.onInfoTip)
		NOTIFY_HANDLER(IDC_USERS, LVN_KEYDOWN, onKeyDownUsers)
		NOTIFY_HANDLER(IDC_USERS, LVN_ITEMCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_USERS, NM_DBLCLK, onDoubleClickUsers)
		NOTIFY_HANDLER(IDC_USERS, NM_RETURN, onEnterUsers)
		NOTIFY_HANDLER(IDC_USERS, NM_CUSTOMDRAW, onCustomDraw)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(FTM_CONTEXTMENU, onTabContextMenu)
		MESSAGE_HANDLER(WM_MOUSEMOVE, onStyleChange)
		MESSAGE_HANDLER(WM_CAPTURECHANGED, onStyleChanged)
		MESSAGE_HANDLER(WM_FORWARDMSG, OnRelayMsg)
		MESSAGE_HANDLER(WM_TIMER, onTimer)
		COMMAND_ID_HANDLER(ID_FILE_RECONNECT, onFileReconnect)
		COMMAND_ID_HANDLER(IDC_FOLLOW, onFollow)
		COMMAND_ID_HANDLER(IDC_ADD_AS_FAVORITE, onAddAsFavorite)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		COMMAND_ID_HANDLER(IDC_SELECT_USER, onSelectUser)
		COMMAND_ID_HANDLER(IDC_PUBLIC_MESSAGE, onPublicMessage)
		COMMAND_ID_HANDLER(IDC_OPEN_HUB_LOG, onOpenHubLog)
		COMMAND_ID_HANDLER(IDC_HISTORY, onOpenHubLog)
		COMMAND_ID_HANDLER(IDC_OPEN_USER_LOG, onOpenUserLog)
		COMMAND_ID_HANDLER(IDC_USER_HISTORY, onOpenUserLog)
		COMMAND_ID_HANDLER(IDC_NOTIFY, onSetNotify)
		COMMAND_ID_HANDLER(ID_EDIT_CLEAR_ALL, onEditClearAll)
		COMMAND_ID_HANDLER(IDC_IGNORE, onIgnore)
		COMMAND_ID_HANDLER(IDC_UNIGNORE, onUnignore)
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK, onLButton)
		CHAIN_COMMANDS(uibBase)
		CHAIN_MSG_MAP(splitBase)
		CHAIN_MSG_MAP(chatBase)
		CHAIN_MSG_MAP_MEMBER(filter)
	ALT_MSG_MAP(EDIT_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CHAR, onChar)
		MESSAGE_HANDLER(WM_KEYDOWN, onChar)
		MESSAGE_HANDLER(WM_KEYUP, onChar)
		MESSAGE_HANDLER(WM_CUT, onChar) //ApexDC
		MESSAGE_HANDLER(WM_PASTE, onChar) //ApexDC
		MESSAGE_HANDLER(WM_DROPFILES, onDropFiles)
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK, onLButton)
	ALT_MSG_MAP(SHOW_USERS)
		MESSAGE_HANDLER(BM_SETCHECK, onShowUsers)
	ALT_MSG_MAP(STATUS_MSG)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFO, onGetToolTip)
	END_MSG_MAP()

	LRESULT onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onDoubleClickUsers(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onShowUsers(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onFollow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEnterUsers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onGetToolTip(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onFileReconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClientEnLink(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) { return ctrlClient.onClientEnLink(uMsg, wParam, lParam, bHandled); }
	LRESULT onSelectUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPublicMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenHubLog(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenUserLog(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onStyleChange(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onStyleChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onKeyDownUsers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSetNotify(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onLButton(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	
	LRESULT OnRelayMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		LPMSG pMsg = (LPMSG)lParam;
		if (ctrlTooltips.m_hWnd != NULL && pMsg->message >= WM_MOUSEFIRST && pMsg->message <= WM_MOUSELAST)
			ctrlTooltips.RelayEvent(pMsg);
		return 0;
	}

	void handleOpenOwnList();

	LRESULT onFilterChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		//handle focus switch
		if(uMsg == WM_CHAR && wParam == VK_TAB) {
			handleTab(WinUtil::isShift());
			return 0;
		}
		bHandled = FALSE;
		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE) override;
	bool sendMessageHooked(const OutgoingChatMessage& aMessage, string& error_) override;
	void addLine(const tstring& aLine, CHARFORMAT2& cf = WinUtil::m_ChatTextGeneral, bool bUseEmo = true);
	void addMessage(const Message& aMessage, CHARFORMAT2& cf, bool bUseEmo = true);
	void addPrivateLine(const tstring& aLine, CHARFORMAT2& cf) override { addLine(aLine, cf, false); }
	void addStatusMessage(const LogMessagePtr& aMessage, int aFlags) override;
	void addStatus(const LogMessagePtr& aMessage, CHARFORMAT2& cf = WinUtil::m_ChatTextSystem, bool aInChat = true);
	bool checkFrameCommand(const tstring& aCmd, const tstring& aParam, tstring& message_, tstring& status_, bool& thirdPerson_) override;
	void onTab() override;
	void handleTab(bool reverse);
	void runUserCommand(::UserCommand& uc);
	static void ShutDown() {
		shutdown = true;
	}

	static void openWindow(const tstring& server);
	static void resortUsers();	
	static void closeDisconnected();
	static void reconnectDisconnected();
	static void updateFonts();
	static bool getWindowParams(HWND hWnd, StringMap& params);
	static bool parseWindowParams(StringMap& params);


	void setFonts();

	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlMessage.SetFocus();
		return 0;
	}
	
	LRESULT onAddAsFavorite(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		addAsFavorite();
		return 0;
	}

	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}

	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	LRESULT onIgnore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onUnignore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/);

	static ResourceManager::Strings columnNames[UserUtil::COLUMN_LAST];
	static string id;
private:
	friend class UserInfoBaseHandler<HubFrame>;

	class ItemInfo : public FastAlloc<ItemInfo>, public UserInfoBase {
	public:
		ItemInfo(const OnlineUserPtr& aUser);

		const tstring getText(uint8_t col) const;

		static int compareItems(const ItemInfo* a, const ItemInfo* b, uint8_t col);

		int getImageIndex() const;

		bool update(int sortCol, const tstring& oldText = Util::emptyStringT) noexcept;


		const UserPtr& getUser() const { return onlineUser->getUser(); }
		const string& getHubUrl() const { return onlineUser->getHubUrl(); }

		const OnlineUserPtr onlineUser;
	private:
	};

	std::unordered_map<OnlineUserPtr, ItemInfo, OnlineUser::Hash> userInfos;

	ItemInfo* findUserInfo(const OnlineUserPtr& aUser) noexcept;
	ItemInfo* findUserInfo(const string& aNick) noexcept;


	TypedListViewCtrl<ItemInfo, IDC_USERS>& getUserList() { return ctrlUsers; }

	enum Tasks { UPDATE_USER_JOIN, UPDATE_USER, REMOVE_USER, ADD_SILENT_STATUS_LINE, 
		GET_SHUTDOWN, SET_SHUTDOWN, KICK_MSG, UPDATE_TAB_ICONS, ADD_STATUS_LINE
	};

	struct UserTask : public Task {
		UserTask(const OnlineUserPtr& ou) : onlineUser(ou) { }
		~UserTask() { }
		
		const OnlineUserPtr onlineUser;
	};
	
	friend class PrivateFrame;
	
	HubFrame(const tstring& aServer);
	~HubFrame();

	typedef unordered_map<tstring, HubFrame*> FrameMap;
	typedef FrameMap::const_iterator FrameIter;
	static FrameMap frames;

	static bool shutdown;

	bool waitingForPW;
	bool extraSort;

	ClientPtr client;
	tstring server;
	string cachedHubname;
	bool wentoffline = true;
	CContainedWindow ctrlShowUsersContainer;
	CContainedWindow ctrlMessageContainer;
	CContainedWindow ctrlClientContainer;
	CContainedWindow ctrlStatusContainer;

	CButton ctrlShowUsers;

	typedef TypedListViewCtrl<ItemInfo, IDC_USERS> CtrlUsers;
	CtrlUsers ctrlUsers;
	
	int statusSizes[4];
	
	bool closed;
	bool showUsers;
	bool forceClose;

	Client::CountType countType;

	TStringMap tabParams;
	bool tabMenuShown;

	TaskQueue tasks;
	bool updateUsers;
	bool resort;
	bool statusDirty;

	ParamMap ucLineParams;
	string getLogPath(bool status = false) const;

	enum { MAX_CLIENT_LINES = 5 };
	deque<tstring> lastLinesList;
	tstring lastLines;

	CIcon tabIcon;
	tstring cipherPopupTxt;
	
	static int columnIndexes[UserUtil::COLUMN_LAST];
	static int columnSizes[UserUtil::COLUMN_LAST];
	
	bool updateUser(const UserTask& u);
	void removeUser(const OnlineUserPtr& aUser);

	void updateUserList(ItemInfo* ii = nullptr);

	ListFilter filter;

	void addAsFavorite();
	void removeFavoriteHub();

	void clearUserList();
	void clearTaskList();

	int hubchatusersplit;

	string sColumsOrder;
    string sColumsWidth;
    string sColumsVisible;

	void updateStatusBar();
	void onChatMessage(const ChatMessagePtr& message);
	void onUpdateTabIcons();
	void setTabIcons();
	void onPassword();

	void onConnected();
	void onDisconnected(const string& aReason);

	void setWindowTitle(const string& aTitle);

	void execTasks();

	// FavoriteManagerListener
	void on(FavoriteManagerListener::FavoriteUserAdded, const FavoriteUser& /*aUser*/) noexcept override;
	void on(FavoriteManagerListener::FavoriteUserRemoved, const FavoriteUser& /*aUser*/) noexcept override;
	void resortForFavsFirst(bool justDoIt = false);

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept override;

	// ClientListener
	void on(ClientListener::Connecting, const Client*) noexcept override;
	void on(ClientListener::Connected, const Client*) noexcept override;
	void on(ClientListener::UserConnected, const Client*, const OnlineUserPtr&) noexcept override;
	void on(ClientListener::UserUpdated, const Client*, const OnlineUserPtr&) noexcept override;
	void on(ClientListener::UsersUpdated, const Client*, const OnlineUserList&) noexcept override;
	void on(ClientListener::UserRemoved, const Client*, const OnlineUserPtr&) noexcept override;
	void on(ClientListener::Redirect, const Client*, const string&) noexcept override;
	void on(ClientListener::Disconnected, const string&, const string&) noexcept override;
	void on(ClientListener::GetPassword, const Client*) noexcept override;
	void on(ClientListener::HubUpdated, const Client*) noexcept override;
	void on(ClientListener::ChatMessage, const Client*, const ChatMessagePtr&) noexcept override;
	void on(ClientListener::StatusMessage, const Client*, const LogMessagePtr&, int = ClientListener::FLAG_NORMAL) noexcept override;
	void on(ClientListener::NickTaken, const Client*) noexcept override;
	void on(ClientListener::SearchFlood, const Client*, const string&) noexcept override;
	void on(ClientListener::HubTopic, const Client*, const string&) noexcept override;
	void on(ClientListener::SetActive, const Client*) noexcept override;
	void on(ClientListener::Close, const Client*) noexcept override;
	void on(ClientListener::Redirected, const string&, const ClientPtr& aNewClient) noexcept override;
	void on(ClientListener::MessagesRead, const Client*) noexcept override;
	void on(ClientListener::KeyprintMismatch, const Client*) noexcept override;

	void speak(Tasks s, const OnlineUserPtr& u) { tasks.add(static_cast<uint8_t>(s), unique_ptr<Task>(new UserTask(u))); updateUsers = true; }
	void openLinksInTopic();
};

#endif // !defined(HUB_FRAME_H)
