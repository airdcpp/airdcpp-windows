/*
 * Copyright (C) 2001-2014 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef PRIVATE_FRAME_H
#define PRIVATE_FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/User.h"
#include "../client/ClientManagerListener.h"
#include "../client/ConnectionManagerListener.h"
#include "../client/DelayedEvents.h"
#include "../client/UserInfoBase.h"

#include "UserInfoBaseHandler.h"
#include "ChatFrameBase.h"
#include "UCHandler.h"

#define HUB_SEL_MAP 9
#define STATUS_MSG_MAP 19

class PrivateFrame : public UserInfoBaseHandler<PrivateFrame>, public UserInfoBase,
	private ClientManagerListener, private ConnectionManagerListener, public UCHandler<PrivateFrame>, private SettingsManagerListener, public ChatFrameBase
{
public:
	static bool gotMessage(const ChatMessage& aMessage , Client* c);
	static void openWindow(const HintedUser& replyTo, const tstring& aMessage = Util::emptyStringT, Client* c = NULL);
	static bool isOpen(const UserPtr& u) { return frames.find(u) != frames.end(); }
	static void closeAll();
	static void closeAllOffline();

	enum {
		USER_UPDATED
	};

	DECLARE_FRAME_WND_CLASS_EX(_T("PrivateFrame"), IDR_PRIVATE, 0, COLOR_3DFACE);

	typedef UCHandler<PrivateFrame> ucBase;
	typedef ChatFrameBase chatBase;
	typedef UserInfoBaseHandler<PrivateFrame> uibBase;

	BEGIN_MSG_MAP(PrivateFrame)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(FTM_CONTEXTMENU, onTabContextMenu)
		MESSAGE_HANDLER(WM_FORWARDMSG, OnRelayMsg)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFO, onGetToolTip)
		COMMAND_ID_HANDLER(IDC_OPEN_USER_LOG, onOpenUserLog)
		COMMAND_ID_HANDLER(IDC_USER_HISTORY, onOpenUserLog)
		COMMAND_ID_HANDLER(IDC_UNGRANTSLOT, onGrantSlot)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		COMMAND_ID_HANDLER(ID_EDIT_CLEAR_ALL, onEditClearAll)
		COMMAND_ID_HANDLER(IDC_PUBLIC_MESSAGE, onPublicMessage)
		CHAIN_MSG_MAP(chatBase)
		CHAIN_COMMANDS(ucBase)
		CHAIN_COMMANDS(uibBase)
	ALT_MSG_MAP(EDIT_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CHAR, onChar)
		MESSAGE_HANDLER(WM_KEYDOWN, onChar)
		MESSAGE_HANDLER(WM_KEYUP, onChar)
		MESSAGE_HANDLER(WM_CUT, onChar) //ApexDC
		MESSAGE_HANDLER(WM_PASTE, onChar) //ApexDC
		MESSAGE_HANDLER(WM_DROPFILES, onDropFiles)
	ALT_MSG_MAP(HUB_SEL_MAP)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, onHubChanged)
	ALT_MSG_MAP(STATUS_MSG_MAP)
	NOTIFY_CODE_HANDLER(TTN_GETDISPINFO, onGetToolTip)
		MESSAGE_HANDLER(WM_LBUTTONUP, onStatusBarClick)
	END_MSG_MAP()

	LRESULT onHubChanged(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onClientEnLink(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) { return ctrlClient.onClientEnLink(uMsg, wParam, lParam, bHandled); }
	LRESULT onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenUserLog(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPublicMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGetToolTip(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onStatusBarClick(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	LRESULT OnRelayMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);

	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);

	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}

	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);


	void fillLogParams(ParamMap& params) const;
	void addLine(const tstring& aLine, CHARFORMAT2& cf);
	void addLine(const Identity&, const tstring& aLine);
	void addLine(const Identity&, const tstring& aLine, CHARFORMAT2& cf);
	void addStatusLine(const tstring& aLine, uint8_t severity);

	bool checkFrameCommand(tstring& cmd, tstring& param, tstring& message, tstring& status, bool& thirdPerson);
	void UpdateLayout(BOOL bResizeBars = TRUE);	
	void runUserCommand(UserCommand& uc);
	void readLog();
	
	void addClientLine(const tstring& aLine, uint8_t severity);
	bool sendMessage(const tstring& msg, string& error_, bool thirdPerson = false);

	struct UserListHandler {
		UserListHandler(PrivateFrame* _pf) : pf(_pf) { }
		
		void forEachSelected(void (UserInfoBase::*func)()) {
			(pf->*func)();
		}
		
		template<class _Function>
		_Function forEachSelectedT(_Function pred) {
			pred(pf);
			return pred;
		}
		
	private:
		PrivateFrame* pf;	
	};
	
	UserListHandler getUserList() { return UserListHandler(this); }

private:
	enum {
		STATUS_TEXT,
		STATUS_CC,
		STATUS_HUBSEL,
		STATUS_LAST
	};

	PrivateFrame(const HintedUser& replyTo_, Client* c);
	~PrivateFrame() { }
	
	bool nmdcUser;
	bool created;

	int CCPMattempts;
	int failedCCPMattempts;
	uint64_t lastCCPMconnect;
	bool allowAutoCCPM;

	string getLogPath() const;
	typedef unordered_map<UserPtr, PrivateFrame*, User::Hash> FrameMap;
	typedef FrameMap::const_iterator FrameIter;
	static FrameMap frames;
	CComboBoxEx ctrlHubSel;

	void fillHubSelection();

	void updateOnlineStatus(bool ownChange = false);
	StringPairList hubs;
	bool online;
	void changeClient();
	void showHubSelection(bool show);

	HintedUser replyTo;
	const UserPtr& getUser() const { return replyTo.user; }	
	const string& getHubUrl() const { return replyTo.hint; }	
	
	CContainedWindow ctrlMessageContainer;
	CContainedWindow ctrlClientContainer;
	CContainedWindow ctrlHubSelContainer;
	CContainedWindow ctrlStatusContainer;

	bool closed;
	tstring nicks;
	tstring hubNames;
	ParamMap ucLineParams;
	
	CIcon tabIcon;
	CIcon userOffline;
	CIcon iCCReady;
	CIcon iStartCC;

	void startCC(bool silent = false);
	void closeCC(bool silent = false);
	bool ccReady() const;

	void checkClientChanged(const HintedUser& newUser, Client* c, bool ownChange);
	void updateTabIcon(bool offline);
	TStringList prevCommands;
	tstring currentCommand;
	TStringList::size_type curCommandPosition;
	
	// ClientManagerListener
	void on(ClientManagerListener::UserConnected, const OnlineUser& aUser, bool wasOffline) noexcept;
	void on(ClientManagerListener::UserDisconnected, const UserPtr& aUser, bool wentOffline) noexcept;
	void on(ClientManagerListener::UserUpdated, const OnlineUser& aUser) noexcept;

	// ConnectionManagerListener
	virtual void on(ConnectionManagerListener::Connected, const ConnectionQueueItem* cqi, UserConnection* uc) noexcept;
	virtual void on(ConnectionManagerListener::Removed, const ConnectionQueueItem* cqi) noexcept;

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;

	void addSpeakerTask(bool addDelay);
	void runSpeakerTask();

	void checkAllwaysCCPM();
	void handleNotifications(bool newWindow, const tstring& aMessage, const Identity& from);

	DelayedEvents<CID> delayEvents;
};

#endif // !defined(PRIVATE_FRAME_H)
