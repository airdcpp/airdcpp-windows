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

#if !defined(HUB_FRAME_H)
#define HUB_FRAME_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"
#include "MainFrm.h"
#include "MenuBaseHandlers.h"
#include "ChatFrameBase.h"

#include "../client/Client.h"
#include "../client/User.h"
#include "../client/ClientManager.h"
#include "../client/TimerManager.h"
#include "../client/FastAlloc.h"
#include "../client/DirectoryListing.h"
#include "../client/TaskQueue.h"
#include "IgnoreManager.h"
#include "atlstr.h"
#include "WinUtil.h"
#include "UCHandler.h"

#define FILTER_MESSAGE_MAP 8
#define SHOW_USERS 9
struct CompareItems;
class ChatFrameBase;

class HubFrame : public MDITabChildWindowImpl<HubFrame>, private ClientListener, 
	public CSplitterImpl<HubFrame>, private FavoriteManagerListener, private TimerManagerListener,
	public UCHandler<HubFrame>, public UserInfoBaseHandler<HubFrame>, private SettingsManagerListener, private ChatFrameBase, private FrameMessageBase
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("HubFrame"), IDR_HUB, 0, COLOR_3DFACE);

	typedef CSplitterImpl<HubFrame> splitBase;
	typedef MDITabChildWindowImpl<HubFrame> baseClass;
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
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(FTM_CONTEXTMENU, onTabContextMenu)
		MESSAGE_HANDLER(WM_MOUSEMOVE, onStyleChange)
		MESSAGE_HANDLER(WM_CAPTURECHANGED, onStyleChanged)
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
		COMMAND_ID_HANDLER(IDC_OPEN_OWN_LIST, onOpenMyList)
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
		CHAIN_MSG_MAP(baseClass)
		CHAIN_MSG_MAP(splitBase)
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
	ALT_MSG_MAP(FILTER_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, onCtlColor)
		MESSAGE_HANDLER(WM_CHAR, onFilterChar)
		MESSAGE_HANDLER(WM_KEYUP, onFilterChar)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, onSelChange)
	END_MSG_MAP()

	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
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
	LRESULT onFilterChar(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onFileReconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClientEnLink(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) { return ctrlClient.onClientEnLink(uMsg, wParam, lParam, bHandled); }
	LRESULT onSelectUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPrivateMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
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

	void UpdateLayout(BOOL bResizeBars = TRUE);
	bool sendMessage(const tstring& aMessage, bool isThirdPerson);
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

	static void openWindow(const tstring& server, int chatusersplit = 0, bool userliststate = true, ProfileToken aShareProfile = SP_DEFAULT,
		        string sColumsOrder = Util::emptyString, string sColumsWidth = Util::emptyString, string sColumsVisible = Util::emptyString);
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

	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
		updateStatusBar();
		return 0;
	}

	LRESULT onIgnore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onUnignore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/);

	TypedListViewCtrl<OnlineUser, IDC_USERS>& getUserList() { return ctrlUsers; }

	typedef unordered_set<UserPtr, User::Hash> IgnoreMap;
	static IgnoreMap ignoreList;

	static ResourceManager::Strings columnNames[OnlineUser::COLUMN_LAST];
private:
	enum Tasks { UPDATE_USER_JOIN, UPDATE_USER, REMOVE_USER, ADD_CHAT_LINE,
		ADD_STATUS_LINE, ADD_SILENT_STATUS_LINE, SET_WINDOW_TITLE, GET_PASSWORD, 
		PRIVATE_MESSAGE, STATS, CONNECTED, DISCONNECTED,
		GET_SHUTDOWN, SET_SHUTDOWN, KICK_MSG, UPDATE_TAB_ICONS
	};

	enum FilterModes{
		NONE,
		EQUAL,
		GREATER_EQUAL,
		LESS_EQUAL,
		GREATER,
		LESS,
		NOT_EQUAL
	};

	struct UserTask : public Task {
		UserTask(const OnlineUserPtr& ou) : onlineUser(ou) { }
		~UserTask() { }
		
		const OnlineUserPtr onlineUser;
	};

	struct StatusTask : public StringTask {
		StatusTask(const string& msg, bool _inChat = true) : StringTask(msg), inChat(_inChat) { }
		
		bool inChat;
	};
	
	struct MessageTask : public StringTask {
		MessageTask(const Identity& from_, const string& m) : StringTask(m), from(from_) { }
		MessageTask(const Identity& from_, const OnlineUserPtr& to_, const OnlineUserPtr& replyTo_, const string& m) : StringTask(m),
			from(from_), to(to_->getUser()), replyTo(replyTo_->getUser()), hub(replyTo_->getIdentity().isHub()), bot(replyTo_->getIdentity().isBot()) { }

		const Identity from;
		const UserPtr to;
		const UserPtr replyTo;

		bool hub;
		bool bot;
	};
	
	friend class PrivateFrame;
	
	HubFrame(const tstring& aServer, int chatusersplit, bool userliststate, ProfileToken aShareProfile);
	~HubFrame();

	typedef unordered_map<tstring, HubFrame*> FrameMap;
	typedef FrameMap::const_iterator FrameIter;
	static FrameMap frames;

	tstring redirect;
	bool timeStamps;
	bool showchaticon;
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
	CContainedWindow ctrlFilterContainer;
	CContainedWindow ctrlFilterSelContainer;
	CContainedWindow ctrlMessageContainer;
	CContainedWindow ctrlClientContainer;

	OMenu copyMenu;
	OMenu userMenu;

	CButton ctrlShowUsers;
	CEdit ctrlFilter;
	CComboBox ctrlFilterSel;
	typedef TypedListViewCtrl<OnlineUser, IDC_USERS> CtrlUsers;
	CtrlUsers ctrlUsers;
	CStatusBarCtrl ctrlStatus;
	
	int statusSizes[4];
	
	tstring filter;

	bool closed;
	bool showUsers;

	int countType;

	TStringMap tabParams;
	bool tabMenuShown;

	TaskQueue tasks;
	bool updateUsers;
	bool resort;

	ParamMap ucLineParams;
	string getLogPath(bool status = false) const;

	enum { MAX_CLIENT_LINES = 5 };
	TStringList lastLinesList;
	tstring lastLines;
	CToolTipCtrl ctrlTooltips;
	
	static int columnIndexes[OnlineUser::COLUMN_LAST];
	static int columnSizes[OnlineUser::COLUMN_LAST];
	
	bool updateUser(const UserTask& u);
	void removeUser(const OnlineUserPtr& aUser);

	void updateUserList(OnlineUserPtr ui = NULL);
	bool parseFilter(FilterModes& mode, int64_t& size);
	bool matchFilter(const OnlineUser& ui, int sel, const boost::wregex aReg, bool doSizeCompare = false, FilterModes mode = NONE, int64_t size = 0);

	void addAsFavorite();
	void removeFavoriteHub();

	void clearUserList();
	void clearTaskList();

	int hubchatusersplit;

	bool PreparePopupMenu(CWindow *pCtrl, OMenu& pMenu);
	string sColumsOrder;
    string sColumsWidth;
    string sColumsVisible;

	void updateStatusBar() { if(m_hWnd) speak(STATS); }

	// FavoriteManagerListener
	void on(FavoriteManagerListener::UserAdded, const FavoriteUser& /*aUser*/) noexcept;
	void on(FavoriteManagerListener::UserRemoved, const FavoriteUser& /*aUser*/) noexcept;
	void resortForFavsFirst(bool justDoIt = false);

	// TimerManagerListener
	void on(TimerManagerListener::Second, uint64_t /*aTick*/) noexcept;
	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;

	// ClientListener
	void on(Connecting, const Client*) noexcept;
	void on(Connected, const Client*) noexcept;
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
	void on(ClientListener::SetIcons, const Client*, int aCountType) noexcept;

	void speak(Tasks s) { tasks.add(static_cast<uint8_t>(s), nullptr); PostMessage(WM_SPEAKER); }
	void speak(Tasks s, const string& msg, bool inChat = true) { tasks.add(static_cast<uint8_t>(s), unique_ptr<Task>(new StatusTask(msg, inChat))); PostMessage(WM_SPEAKER); }
	void speak(Tasks s, const OnlineUserPtr& u) { tasks.add(static_cast<uint8_t>(s), unique_ptr<Task>(new UserTask(u))); updateUsers = true; }
	void speak(Tasks s, const Identity& from, const string& line) { tasks.add(static_cast<uint8_t>(s), unique_ptr<Task>(new MessageTask(from, line))); PostMessage(WM_SPEAKER); }
	void speak(Tasks s, const OnlineUserPtr& from, const OnlineUserPtr& to, const OnlineUserPtr& replyTo, const string& line) { tasks.add(static_cast<uint8_t>(s), unique_ptr<Task>(new MessageTask(from->getIdentity(), to, replyTo, line))); PostMessage(WM_SPEAKER); }
	void openLinksInTopic();
};

#endif // !defined(HUB_FRAME_H)
