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
#include "ChatCtrl.h"
#include "MainFrm.h"
#include "EmoticonsDlg.h"

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

#define EDIT_MESSAGE_MAP 10		// This could be any number, really...
#define FILTER_MESSAGE_MAP 8
struct CompareItems;

class HubFrame : public MDITabChildWindowImpl<HubFrame>, private ClientListener, 
	public CSplitterImpl<HubFrame>, private FavoriteManagerListener, private TimerManagerListener,
	public UCHandler<HubFrame>, public UserInfoBaseHandler<HubFrame>, private SettingsManagerListener
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("HubFrame"), IDR_HUB, 0, COLOR_3DFACE);

	typedef CSplitterImpl<HubFrame> splitBase;
	typedef MDITabChildWindowImpl<HubFrame> baseClass;
	typedef UCHandler<HubFrame> ucBase;
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
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFO, onGetToolTip)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_FORWARDMSG, OnForwardMsg)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(FTM_CONTEXTMENU, onTabContextMenu)
		MESSAGE_HANDLER(WM_MOUSEMOVE, onStyleChange)
		MESSAGE_HANDLER(WM_CAPTURECHANGED, onStyleChanged)
		COMMAND_ID_HANDLER(ID_FILE_RECONNECT, onFileReconnect)
		COMMAND_ID_HANDLER(IDC_REFRESH, onRefresh)
		COMMAND_ID_HANDLER(IDC_FOLLOW, onFollow)
		COMMAND_ID_HANDLER(IDC_SEND_MESSAGE, onSendMessage)
		COMMAND_ID_HANDLER(IDC_ADD_AS_FAVORITE, onAddAsFavorite)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		COMMAND_ID_HANDLER(IDC_SELECT_USER, onSelectUser)
		COMMAND_ID_HANDLER(IDC_PUBLIC_MESSAGE, onPublicMessage)
		COMMAND_ID_HANDLER(IDC_OPEN_HUB_LOG, onOpenHubLog)
		COMMAND_ID_HANDLER(IDC_HISTORY, onHistory)
		COMMAND_ID_HANDLER(IDC_OPEN_USER_LOG, onOpenUserLog)
		COMMAND_ID_HANDLER(IDC_USER_HISTORY, onUserHistory)
		COMMAND_ID_HANDLER(IDC_WINAMP_SPAM, onWinampSpam)
		COMMAND_ID_HANDLER(IDC_EMOT, onEmoticons)
		COMMAND_ID_HANDLER(IDC_NOTIFY, onSetNotify)
		COMMAND_ID_HANDLER(ID_EDIT_CLEAR_ALL, onEditClearAll)
		COMMAND_RANGE_HANDLER(IDC_EMOMENU, IDC_EMOMENU + menuItems, onEmoPackChange)
		COMMAND_RANGE_HANDLER(IDC_COPY, IDC_COPY + OnlineUser::COLUMN_LAST, onCopyUserInfo)
		COMMAND_ID_HANDLER(IDC_COPY_HUBNAME, onCopyHubInfo)
		COMMAND_ID_HANDLER(IDC_COPY_HUBADDRESS, onCopyHubInfo)
		COMMAND_ID_HANDLER(IDC_COPY_USER_ALL, onCopyAll)
		COMMAND_ID_HANDLER(IDC_IGNORE, onIgnore)
		COMMAND_ID_HANDLER(IDC_UNIGNORE, onUnignore)
		CHAIN_COMMANDS(ucBase)
		CHAIN_COMMANDS(uibBase)
		CHAIN_MSG_MAP(baseClass)
		CHAIN_MSG_MAP(splitBase)
	ALT_MSG_MAP(EDIT_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CHAR, onChar)
		MESSAGE_HANDLER(WM_KEYDOWN, onChar)
		MESSAGE_HANDLER(WM_KEYUP, onChar)
		MESSAGE_HANDLER(BM_SETCHECK, onShowUsers)
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK, onLButton)
		MESSAGE_HANDLER(WM_CUT, onChar) //ApexDC
		MESSAGE_HANDLER(WM_PASTE, onChar) //ApexDC
		//MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
	ALT_MSG_MAP(FILTER_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, onCtlColor)
		MESSAGE_HANDLER(WM_CHAR, onFilterChar)
		MESSAGE_HANDLER(WM_KEYUP, onFilterChar)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, onSelChange)
	END_MSG_MAP()

	LRESULT OnForwardMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onCopyUserInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyAll(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyHubInfo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onDoubleClickUsers(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onShowUsers(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onFollow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onLButton(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
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
	LRESULT onOpenHubLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onHistory(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onUserHistory(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onStyleChange(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onStyleChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onEmoPackChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onKeyDownUsers(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSetNotify(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onWinampSpam(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		tstring cmd, param, message, status;
		bool thirdPerson;
		if(SETTING(MEDIA_PLAYER) == 0) {
			cmd = _T("/winamp");
		} else if(SETTING(MEDIA_PLAYER) == 1) {
			cmd = _T("/itunes");
		} else if(SETTING(MEDIA_PLAYER) == 2) {
			cmd = _T("/mpc");
		} else if(SETTING(MEDIA_PLAYER) == 3) {
			cmd = _T("/wmp");
		} else if(SETTING(MEDIA_PLAYER) == 4) {
			cmd = _T("/spotify");
		} else {
			addStatus(CTSTRING(NO_MEDIA_SPAM));
			return 0;
		}
		if(WinUtil::checkCommand(cmd, param, message, status, thirdPerson)){
			if(!message.empty()) {
				client->hubMessage(Text::fromT(message));
			}
			if(!status.empty()) {
				addStatus(status);
			}
		}
		return 0;
	}

	LRESULT onEmoticons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled) {
		if (hWndCtl != ctrlEmoticons.m_hWnd) {
			bHandled = false;
			return 0;
		}

		EmoticonsDlg dlg;
		ctrlEmoticons.GetWindowRect(dlg.pos);
		dlg.DoModal(m_hWnd);
		if (!dlg.result.empty()) {
			TCHAR* message = new TCHAR[ctrlMessage.GetWindowTextLength()+1];
			ctrlMessage.GetWindowText(message, ctrlMessage.GetWindowTextLength()+1);
			tstring s(message, ctrlMessage.GetWindowTextLength());
			delete[] message;
			ctrlMessage.SetWindowText((s + dlg.result).c_str());
			ctrlMessage.SetFocus();
			ctrlMessage.SetSel( ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength() );
		}
		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE);
	void addLine(const tstring& aLine);
	void addLine(const tstring& aLine, CHARFORMAT2& cf, bool bUseEmo = true);
	void addLine(const Identity& i, const tstring& aLine, CHARFORMAT2& cf, bool bUseEmo = true);
	void addStatus(const tstring& aLine, CHARFORMAT2& cf = WinUtil::m_ChatTextSystem, bool inChat = true);
	void onEnter();
	void onTab();
	void handleTab(bool reverse);
	void runUserCommand(::UserCommand& uc);

	static void openWindow(const tstring& server, int chatusersplit = 0, bool userliststate = true,
		        string sColumsOrder = Util::emptyString, string sColumsWidth = Util::emptyString, string sColumsVisible = Util::emptyString);
	static void resortUsers();	
	static void closeDisconnected();
	static void reconnectDisconnected();

	static HubFrame* getHub(Client* aClient) {
		for(FrameIter i = frames.begin() ; i != frames.end() ; i++) {
			HubFrame* hubFrame = i->second;
			if(hubFrame->client == aClient) {
				return hubFrame;
			}
		}
		return NULL;
	}

	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlMessage.SetFocus();
		return 0;
	}

	LRESULT onSendMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		onEnter();
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

	LRESULT onRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(client->isConnected()) {
			clearUserList();
			client->refreshUserList(false);
		}
		return 0;
	}

	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
		updateStatusBar();
		return 0;
	}

	LRESULT onIgnore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/) {
		int i=-1;
		if(client->isConnected()) {
			while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
				ignoreList.insert(((OnlineUser*)ctrlUsers.getItemData(i))->getUser());
				IgnoreManager::getInstance()->storeIgnore(((OnlineUser*)ctrlUsers.getItemData(i))->getUser());
			}
		}
		return 0;
	}

	LRESULT onUnignore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/) {
		int i=-1;
		if(client->isConnected()) {
			while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
				ignoreList.erase(((OnlineUser*)ctrlUsers.getItemData(i))->getUser());
					IgnoreManager::getInstance()->removeIgnore(((OnlineUser*)ctrlUsers.getItemData(i))->getUser());
			}
		}
		return 0;
	}

	TypedListViewCtrl<OnlineUser, IDC_USERS>& getUserList() { return ctrlUsers; }

	typedef unordered_set<UserPtr, User::Hash> IgnoreMap;
	static IgnoreMap ignoreList;

	static ResourceManager::Strings columnNames[OnlineUser::COLUMN_LAST];

private:
	enum Tasks { UPDATE_USER_JOIN, UPDATE_USER, REMOVE_USER, ADD_CHAT_LINE,
		ADD_STATUS_LINE, ADD_SILENT_STATUS_LINE, SET_WINDOW_TITLE, GET_PASSWORD, 
		PRIVATE_MESSAGE, STATS, CONNECTED, DISCONNECTED,
		GET_SHUTDOWN, SET_SHUTDOWN, KICK_MSG
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
	
	HubFrame(const tstring& aServer, int chatusersplit, bool userliststate) : 
		waitingForPW(false), extraSort(false), server(aServer), closed(false), 
		showUsers(BOOLSETTING(GET_USER_INFO)), updateUsers(false), resort(false),
		curCommandPosition(0), timeStamps(BOOLSETTING(TIME_STAMPS)),
		hubchatusersplit(chatusersplit), menuItems(0), currentNeedlePos(-1),
		lineCount(1), //ApexDC
		ctrlMessageContainer(WC_EDIT, this, EDIT_MESSAGE_MAP), 
		showUsersContainer(WC_BUTTON, this, EDIT_MESSAGE_MAP),
		clientContainer(WC_EDIT, this, EDIT_MESSAGE_MAP),
		ctrlFilterContainer(WC_EDIT, this, FILTER_MESSAGE_MAP),
		ctrlFilterSelContainer(WC_COMBOBOX, this, FILTER_MESSAGE_MAP)
	{
		client = ClientManager::getInstance()->getClient(Text::fromT(aServer));
		client->addListener(this);

		if(FavoriteManager::getInstance()->getFavoriteHubEntry(Text::fromT(server)) != NULL) {
			showUsers = userliststate;
		} else {
			showUsers = BOOLSETTING(GET_USER_INFO);
		}
		
		memset(statusSizes, 0, sizeof(statusSizes));
	}

	~HubFrame() {
		ClientManager::getInstance()->putClient(client);

		dcassert(frames.find(server) != frames.end());
		dcassert(frames[server] == this);
		frames.erase(server);

		clearTaskList();
	}

	typedef unordered_map<tstring, HubFrame*> FrameMap;
	typedef FrameMap::const_iterator FrameIter;
	static FrameMap frames;

	tstring redirect;
	bool timeStamps;
	bool showJoins;
	bool favShowJoins;
	bool hubshowjoins; //favorite hub show joins
	bool logMainChat;
	bool showchaticon;
	tstring complete;
	int menuItems;
	int lineCount; //ApexDC

	bool waitingForPW;
	bool extraSort;


	TStringList prevCommands;
	tstring currentCommand;
	TStringList::size_type curCommandPosition;		//can't use an iterator because StringList is a vector, and vector iterators become invalid after resizing

	tstring currentNeedle;		// search in chat window
	long currentNeedlePos;		// search in chat window
	void findText(tstring const& needle) noexcept;
	tstring findTextPopup();

	CFindReplaceDialog findDlg;

	Client* client;
	tstring server;
	string cachedHubname;
	bool wentoffline;
	CContainedWindow ctrlMessageContainer;
	CContainedWindow clientContainer;
	CContainedWindow showUsersContainer;
	CContainedWindow ctrlFilterContainer;
	CContainedWindow ctrlFilterSelContainer;

	OMenu copyMenu;
	OMenu userMenu;
	OMenu emoMenu;

	CButton ctrlShowUsers;
	CButton ctrlEmoticons;
	ChatCtrl ctrlClient;
	CEdit ctrlMessage;
	CEdit ctrlFilter;
	CComboBox ctrlFilterSel;
	typedef TypedListViewCtrl<OnlineUser, IDC_USERS> CtrlUsers;
	CtrlUsers ctrlUsers;
	CStatusBarCtrl ctrlStatus;
	
	int statusSizes[4];
	
	tstring filter;

	HBITMAP hEmoticonBmp;
	bool closed;
	bool showUsers;

	TStringMap tabParams;
	bool tabMenuShown;

	TaskQueue tasks;
	bool updateUsers;
	bool resort;

	StringMap ucLineParams;

	enum { MAX_CLIENT_LINES = 5 };
	TStringList lastLinesList;
	tstring lastLines;
	CToolTipCtrl ctrlLastLines;
	
	static int columnIndexes[OnlineUser::COLUMN_LAST];
	static int columnSizes[OnlineUser::COLUMN_LAST];
	
	bool updateUser(const UserTask& u);
	void removeUser(const OnlineUserPtr& aUser);

	void updateUserList(OnlineUserPtr ui = NULL);
	bool parseFilter(FilterModes& mode, int64_t& size);
	bool matchFilter(const OnlineUser& ui, int sel, bool doSizeCompare = false, FilterModes mode = NONE, int64_t size = 0);

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
	void on(Failed, const Client*, const string&) noexcept;
	void on(GetPassword, const Client*) noexcept;
	void on(HubUpdated, const Client*) noexcept;
	void on(Message, const Client*, const ChatMessage&) noexcept;
	void on(StatusMessage, const Client*, const string&, int = ClientListener::FLAG_NORMAL) noexcept;
	void on(NickTaken, const Client*) noexcept;
	void on(SearchFlood, const Client*, const string&) noexcept;	
	void on(HubTopic, const Client*, const string&) noexcept;
	void on(AddLine, const Client*, const string&) noexcept;
	void on(HubCounts, const Client*) noexcept;

	void speak(Tasks s) { tasks.add(static_cast<uint8_t>(s), 0); PostMessage(WM_SPEAKER); }
	void speak(Tasks s, const string& msg, bool inChat = true) { tasks.add(static_cast<uint8_t>(s), unique_ptr<Task>(new StatusTask(msg, inChat))); PostMessage(WM_SPEAKER); }
	void speak(Tasks s, const OnlineUserPtr& u) { tasks.add(static_cast<uint8_t>(s), unique_ptr<Task>(new UserTask(u))); updateUsers = true; }
	void speak(Tasks s, const Identity& from, const string& line) { tasks.add(static_cast<uint8_t>(s), unique_ptr<Task>(new MessageTask(from, line))); PostMessage(WM_SPEAKER); }
	void speak(Tasks s, const OnlineUserPtr& from, const OnlineUserPtr& to, const OnlineUserPtr& replyTo, const string& line) { tasks.add(static_cast<uint8_t>(s), unique_ptr<Task>(new MessageTask(from->getIdentity(), to, replyTo, line))); PostMessage(WM_SPEAKER); }
	void openLinksInTopic();
};

#endif // !defined(HUB_FRAME_H)

/**
 * @file
 * $Id: HubFrame.h 482 2010-02-13 10:49:30Z bigmuscle $
 */
