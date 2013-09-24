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

#if !defined(HUB_FRAME_H)
#define HUB_FRAME_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"
#include "UserInfoBaseHandler.h"
#include "ChatFrameBase.h"

#include "../client/Client.h"
#include "../client/User.h"
#include "../client/ClientManager.h"
#include "../client/FastAlloc.h"
#include "../client/TaskQueue.h"

#include "atlstr.h"
#include "WinUtil.h"
#include "UCHandler.h"
#include "ListFilter.h"

#define SHOW_USERS 9
struct CompareItems;
class ChatFrameBase;

class HubFrame : private ClientListener, public CSplitterImpl<HubFrame>, private FavoriteManagerListener,
	public UCHandler<HubFrame>, public UserInfoBaseHandler<HubFrame>, private SettingsManagerListener, public ChatFrameBase
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("HubFrame"), IDR_HUB, 0, COLOR_3DFACE);

	typedef CSplitterImpl<HubFrame> splitBase;
	typedef UCHandler<HubFrame> ucBase;
	typedef UserInfoBaseHandler<HubFrame> uibBase;
	typedef ChatFrameBase chatBase;
	
	BEGIN_MSG_MAP(HubFrame)
		NOTIFY_HANDLER(IDC_USERS, LVN_GETDISPINFO, ctrlUsers.onGetDispInfo)
		NOTIFY_HANDLER(IDC_USERS, LVN_COLUMNCLICK, ctrlUsers.onColumnClick)
		NOTIFY_HANDLER(IDC_USERS, LVN_GETINFOTIP, ctrlUsers.onInfoTip)
		NOTIFY_HANDLER(IDC_USERS, LVN_KEYDOWN, onKeyDownUsers)
		NOTIFY_HANDLER(IDC_USERS, LVN_ITEMCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_USERS, NM_DBLCLK, onDoubleClickUsers)
		NOTIFY_HANDLER(IDC_USERS, NM_RETURN, onEnterUsers)
		NOTIFY_HANDLER(IDC_USERS, NM_CUSTOMDRAW, onCustomDraw)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFO, onGetToolTip)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(FTM_CONTEXTMENU, onTabContextMenu)
		MESSAGE_HANDLER(WM_MOUSEMOVE, onStyleChange)
		MESSAGE_HANDLER(WM_CAPTURECHANGED, onStyleChanged)
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
		COMMAND_ID_HANDLER(IDC_BROWSE_OWN_LIST, onOpenMyList)
		COMMAND_RANGE_HANDLER(IDC_COPY, IDC_COPY + OnlineUser::COLUMN_LAST, onCopyUserInfo)
		COMMAND_ID_HANDLER(IDC_COPY_HUBNAME, onCopyHubInfo)
		COMMAND_ID_HANDLER(IDC_COPY_HUBADDRESS, onCopyHubInfo)
		COMMAND_ID_HANDLER(IDC_COPY_USER_ALL, onCopyAll)
		COMMAND_ID_HANDLER(IDC_IGNORE, onIgnore)
		COMMAND_ID_HANDLER(IDC_UNIGNORE, onUnignore)
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK, onLButton)
		CHAIN_MSG_MAP(chatBase)
		CHAIN_COMMANDS(ucBase)
		CHAIN_COMMANDS(uibBase)
		CHAIN_MSG_MAP(splitBase)
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
	END_MSG_MAP()

	LRESULT onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onCopyUserInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyAll(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyHubInfo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onDoubleClickUsers(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onShowUsers(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onFollow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEnterUsers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onGetToolTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onFileReconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClientEnLink(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) { return ctrlClient.onClientEnLink(uMsg, wParam, lParam, bHandled); }
	LRESULT onSelectUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPublicMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBanned(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenHubLog(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenUserLog(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onStyleChange(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onStyleChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onKeyDownUsers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSetNotify(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenMyList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onLButton(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);

	LRESULT onFilterChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		//handle focus switch
		if(uMsg == WM_CHAR && wParam == VK_TAB) {
			handleTab(WinUtil::isShift());
			return 0;
		}
		bHandled = FALSE;
		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE);
	bool sendMessage(const tstring& aMessage, string& error_, bool isThirdPerson);
	void addLine(const tstring& aLine);
	void addLine(const tstring& aLine, CHARFORMAT2& cf, bool bUseEmo = true);
	void addLine(const Identity& i, const tstring& aLine, CHARFORMAT2& cf, bool bUseEmo = true);
	void addStatusLine(const tstring& aLine) { addStatus(aLine); }
	void addStatus(const tstring& aLine, CHARFORMAT2& cf = WinUtil::m_ChatTextSystem, bool inChat = true);
	bool checkFrameCommand(tstring& cmd, tstring& param, tstring& message, tstring& status, bool& thirdPerson);
	void onTab();
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

	TypedListViewCtrl<OnlineUser, IDC_USERS>& getUserList() { return ctrlUsers; }

	typedef unordered_set<UserPtr, User::Hash> IgnoreMap;
	static IgnoreMap ignoreList;

	static ResourceManager::Strings columnNames[OnlineUser::COLUMN_LAST];
private:
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

	tstring redirect;
	bool timeStamps;
	static bool shutdown;
	HICON HubOpIcon; 
	HICON HubRegIcon; 
	HICON HubIcon; 

	bool waitingForPW;
	bool extraSort;

	Client* client;
	tstring server;
	string cachedHubname;
	bool wentoffline;
	CContainedWindow ctrlShowUsersContainer;
	CContainedWindow ctrlMessageContainer;
	CContainedWindow ctrlClientContainer;

	CButton ctrlShowUsers;

	typedef TypedListViewCtrl<OnlineUser, IDC_USERS> CtrlUsers;
	CtrlUsers ctrlUsers;
	
	int statusSizes[4];
	
	bool closed;
	bool showUsers;
	bool forceClose;

	int countType;

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
	CToolTipCtrl ctrlTooltips;
	
	static int columnIndexes[OnlineUser::COLUMN_LAST];
	static int columnSizes[OnlineUser::COLUMN_LAST];
	
	bool updateUser(const UserTask& u);
	void removeUser(const OnlineUserPtr& aUser);

	void updateUserList(OnlineUserPtr ui = nullptr);

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

	void onPrivateMessage(const ChatMessage& message);
	void onChatMessage(const ChatMessage& message);
	void onUpdateTabIcons();
	void onPassword();

	void onConnected();
	void onDisconnected(const string& aReason);

	void setWindowTitle(const string& aTitle);

	void execTasks();

	// FavoriteManagerListener
	void on(FavoriteManagerListener::UserAdded, const FavoriteUser& /*aUser*/) noexcept;
	void on(FavoriteManagerListener::UserRemoved, const FavoriteUser& /*aUser*/) noexcept;
	void resortForFavsFirst(bool justDoIt = false);

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;

	// ClientListener
	void on(Connecting, const Client*) noexcept;
	void on(Connected, const Client*) noexcept;
	void on(UserConnected, const Client*, const OnlineUserPtr&) noexcept;
	void on(UserUpdated, const Client*, const OnlineUserPtr&) noexcept;
	void on(UsersUpdated, const Client*, const OnlineUserList&) noexcept;
	void on(ClientListener::UserRemoved, const Client*, const OnlineUserPtr&) noexcept;
	void on(Redirect, const Client*, const string&) noexcept;
	void on(Failed, const string&, const string&) noexcept;
	void on(GetPassword, const Client*) noexcept;
	void on(HubUpdated, const Client*) noexcept;
	void on(Message, const Client*, const ChatMessage&) noexcept;
	void on(StatusMessage, const Client*, const string&, int = ClientListener::FLAG_NORMAL) noexcept;
	void on(NickTaken, const Client*) noexcept;
	void on(SearchFlood, const Client*, const string&) noexcept;	
	void on(HubTopic, const Client*, const string&) noexcept;
	void on(AddLine, const Client*, const string&) noexcept;
	void on(SetIcons, const Client*, int aCountType) noexcept;
	void on(SetActive, const Client*) noexcept;

	void speak(Tasks s, const OnlineUserPtr& u) { tasks.add(static_cast<uint8_t>(s), unique_ptr<Task>(new UserTask(u))); updateUsers = true; }
	void openLinksInTopic();
};

#endif // !defined(HUB_FRAME_H)
