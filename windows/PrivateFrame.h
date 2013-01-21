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

#ifndef PRIVATE_FRAME_H
#define PRIVATE_FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/User.h"
#include "../client/ClientManagerListener.h"
#include "../client/DelayedEvents.h"
#include "../client/UserInfoBase.h"

#include "ChatFrameBase.h"
#include "FlatTabCtrl.h"
#include "UCHandler.h"

#define HUB_SEL_MAP 9

class PrivateFrame : public MDITabChildWindowImpl<PrivateFrame>, public UserInfoBaseHandler<PrivateFrame>, public UserInfoBase,
	private ClientManagerListener, public UCHandler<PrivateFrame>, private SettingsManagerListener, private ChatFrameBase, private FrameMessageBase
{
public:
	static void gotMessage(const Identity& from, const UserPtr& to, const UserPtr& replyTo, const tstring& aMessage, Client* c);
	static void openWindow(const HintedUser& replyTo, const tstring& aMessage = Util::emptyStringT, Client* c = NULL);
	static bool isOpen(const UserPtr& u) { return frames.find(u) != frames.end(); }
	static void closeAll();
	static void closeAllOffline();

	enum {
		USER_UPDATED
	};

	DECLARE_FRAME_WND_CLASS_EX(_T("PrivateFrame"), IDR_PRIVATE, 0, COLOR_3DFACE);

	typedef MDITabChildWindowImpl<PrivateFrame> baseClass;
	typedef UCHandler<PrivateFrame> ucBase;
	typedef ChatFrameBase chatBase;
	typedef UserInfoBaseHandler<PrivateFrame> uibBase;

	BEGIN_MSG_MAP(PrivateFrame)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(FTM_CONTEXTMENU, onTabContextMenu)
		MESSAGE_HANDLER(WM_FORWARDMSG, OnRelayMsg)
		COMMAND_ID_HANDLER(IDC_OPEN_USER_LOG, onOpenUserLog)
		COMMAND_ID_HANDLER(IDC_USER_HISTORY, onOpenUserLog)
		COMMAND_ID_HANDLER(IDC_UNGRANTSLOT, onGrantSlot)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		COMMAND_ID_HANDLER(ID_EDIT_CLEAR_ALL, onEditClearAll)
		COMMAND_ID_HANDLER(IDC_PUBLIC_MESSAGE, onPublicMessage)
		CHAIN_MSG_MAP(chatBase)
		CHAIN_COMMANDS(ucBase)
		CHAIN_COMMANDS(uibBase)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(EDIT_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CHAR, onChar)
		MESSAGE_HANDLER(WM_KEYDOWN, onChar)
		MESSAGE_HANDLER(WM_KEYUP, onChar)
		MESSAGE_HANDLER(WM_CUT, onChar) //ApexDC
		MESSAGE_HANDLER(WM_PASTE, onChar) //ApexDC
		MESSAGE_HANDLER(WM_DROPFILES, onDropFiles)
	ALT_MSG_MAP(HUB_SEL_MAP)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, onHubChanged)
	END_MSG_MAP()

	LRESULT onHubChanged(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onClientEnLink(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) { return ctrlClient.onClientEnLink(uMsg, wParam, lParam, bHandled); }
	LRESULT onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenUserLog(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPublicMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnRelayMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);

	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);

	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}

	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */);
	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);


	void fillLogParams(ParamMap& params) const;
	void addLine(const tstring& aLine, CHARFORMAT2& cf);
	void addLine(const Identity&, const tstring& aLine);
	void addLine(const Identity&, const tstring& aLine, CHARFORMAT2& cf);
	void addStatusLine(const tstring& aLine);

	bool checkFrameCommand(tstring& cmd, tstring& param, tstring& message, tstring& status, bool& thirdPerson);
	void UpdateLayout(BOOL bResizeBars = TRUE);	
	void runUserCommand(UserCommand& uc);
	void readLog();
	
	void addClientLine(const tstring& aLine);
	bool sendMessage(const tstring& msg, bool thirdPerson = false);

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
		STATUS_HUBSEL,
		STATUS_LAST
	};

	PrivateFrame(const HintedUser& replyTo_, Client* c);
	~PrivateFrame() { }
	
	bool nmdcUser;
	bool created;
	string getLogPath() const;
	typedef unordered_map<UserPtr, PrivateFrame*, User::Hash> FrameMap;
	typedef FrameMap::const_iterator FrameIter;
	static FrameMap frames;
	CComboBox ctrlHubSel;

	void updateOnlineStatus();
	StringPairList hubs;
	bool online;
	void changeClient();
	void showHubSelection(bool show);

	HICON userOffline;
	HICON userOnline;

	HintedUser replyTo;
	const UserPtr& getUser() const { return replyTo.user; }	
	const string& getHubUrl() const { return replyTo.hint; }	
	
	CContainedWindow ctrlMessageContainer;
	CContainedWindow ctrlClientContainer;
	CContainedWindow ctrlHubSelContainer;

	bool closed;
	tstring hubNames;
	ParamMap ucLineParams;

	void updateFrameOnlineStatus(const HintedUser& newUser, Client* c);
	TStringList prevCommands;
	tstring currentCommand;
	TStringList::size_type curCommandPosition;
	
	// ClientManagerListener
	void on(ClientManagerListener::UserConnected, const OnlineUser& aUser, bool wasOffline) noexcept;
	void on(ClientManagerListener::UserDisconnected, const UserPtr& aUser, bool wentOffline) noexcept;

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;

	void addSpeakerTask(bool addDelay);
	void runSpeakerTask();

	DelayedEvents<CID> delayEvents;
};

#endif // !defined(PRIVATE_FRAME_H)
