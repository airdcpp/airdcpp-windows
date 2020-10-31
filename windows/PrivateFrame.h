/*
 * Copyright (C) 2001-2021 Jacek Sieka, arnetheduck on gmail point com
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

#include <airdcpp/typedefs.h>

#include <airdcpp/User.h>
#include <airdcpp/UserInfoBase.h>
#include <airdcpp/PrivateChatListener.h>

#include "UserInfoBaseHandler.h"
#include "ChatFrameBase.h"
#include "UCHandler.h"
#include "UserUtil.h"

#define HUB_SEL_MAP 9
#define STATUS_MSG_MAP 19

class PrivateFrame : public UserInfoBaseHandler<PrivateFrame>, public UserInfoBase,
private PrivateChatListener, public UCHandler<PrivateFrame>, private SettingsManagerListener, public ChatFrameBase
{
public:
	static void openWindow(const HintedUser& replyTo, bool aMessageReceived = false);
	static bool getWindowParams(HWND hWnd, StringMap& params);
	static bool parseWindowParams(StringMap& params);

	DECLARE_FRAME_WND_CLASS_EX(_T("PrivateFrame"), IDR_PRIVATE, 0, COLOR_3DFACE);

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
		MESSAGE_HANDLER(WM_TIMER, onTimer)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFO, onGetToolTip)
		COMMAND_ID_HANDLER(IDC_OPEN_USER_LOG, onOpenUserLog)
		COMMAND_ID_HANDLER(IDC_USER_HISTORY, onOpenUserLog)
		COMMAND_ID_HANDLER(IDC_UNGRANTSLOT, onGrantSlot)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		COMMAND_ID_HANDLER(ID_EDIT_CLEAR_ALL, onEditClearAll)
		COMMAND_ID_HANDLER(IDC_PUBLIC_MESSAGE, onPublicMessage)
		COMMAND_CODE_HANDLER(EN_CHANGE, onEditChange)
		COMMAND_RANGE_HANDLER(IDC_COPY, IDC_COPY + UserUtil::COLUMN_LAST, onCopyUserInfo)
		CHAIN_MSG_MAP(chatBase)
		CHAIN_COMMANDS(uibBase)
	ALT_MSG_MAP(EDIT_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocusMsg)
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
	LRESULT onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onEditChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL & bHandled);

	LRESULT OnRelayMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);

	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);

	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}
	LRESULT onCopyUserInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onFocusMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);

	void addLine(const tstring& aLine, CHARFORMAT2& cf);
	void addLine(const Identity&, const tstring& aLine);
	void addLine(const Identity&, const tstring& aLine, CHARFORMAT2& cf);
	void addPrivateLine(const tstring& aLine, CHARFORMAT2& cf) { addLine(aLine, cf); }
	void addStatusLine(const tstring& aLine, uint8_t aSeverity);

	bool checkFrameCommand(tstring& cmd, tstring& param, tstring& message, tstring& status, bool& thirdPerson);
	void UpdateLayout(BOOL bResizeBars = TRUE);	
	void runUserCommand(UserCommand& uc);
	
	void addClientLine(const tstring& aLine, uint8_t severity);
	bool sendMessageHooked(const OutgoingChatMessage& aMessage, string& error_);

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

	static string id;

private:
	typedef unordered_map<UserPtr, PrivateFrame*, User::Hash> FrameMap;
	static FrameMap frames;

	enum {
		STATUS_TEXT,
		STATUS_CC,
		STATUS_HUBSEL,
		STATUS_AWAY,
		STATUS_COUNTRY,
		STATUS_LAST
	};


	PrivateFrame(const HintedUser& replyTo_);
	~PrivateFrame() { }

	PrivateChatPtr chat;
	
	bool nmdcUser;
	bool created;

	CComboBoxEx ctrlHubSel;
	CIcon tabIcon;

	void fillHubSelection();
	StringPairList hubs;
	void showHubSelection(bool show);
	void updateOnlineStatus();


	const UserPtr& getUser() const;
	const string& getHubUrl() const;
	
	CContainedWindow ctrlMessageContainer;
	CContainedWindow ctrlClientContainer;
	CContainedWindow ctrlHubSelContainer;
	CContainedWindow ctrlStatusContainer;

	bool closed;
	bool windowLoaded;
	ParamMap ucLineParams;
	
	tstring lastCCPMError;
	tstring countryPopup;

	void closeCC(bool silent = false);
	bool ccReady() const;

	bool isTyping;
	bool userTyping;
	bool userAway;

	void updatePMInfo(uint8_t aType);
	void addStatus(const tstring& aLine, HICON aIcon);
	deque<tstring> lastLinesList;
	tstring lastLines;

	pair<tstring, HICON> lastStatus;

	void updateTabIcon(bool offline);
	void setCountryFlag();
	void setAway();

	TStringList prevCommands;
	tstring currentCommand;
	TStringList::size_type curCommandPosition;

	void activate() noexcept;
	
	void on(PrivateChatListener::StatusMessage, PrivateChat*, const LogMessagePtr& aMessage) noexcept;
	void on(PrivateChatListener::PrivateMessage, PrivateChat*, const ChatMessagePtr& aMessage) noexcept;
	void on(PrivateChatListener::UserUpdated, PrivateChat*) noexcept;
	void on(PrivateChatListener::PMStatus, PrivateChat*, uint8_t aType) noexcept;
	void on(PrivateChatListener::Close, PrivateChat*) noexcept;

	void on(PrivateChatListener::CCPMStatusUpdated, PrivateChat* aChat) noexcept{
		on(UserUpdated(), aChat);
	}

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;

	void handleNotifications(bool newWindow, const tstring& aMessage);
	void updateStatusBar();

	void onChatMessage(const ChatMessagePtr& aMessage) noexcept;
	void onStatusMessage(const LogMessagePtr& aMessage) noexcept;
};

#endif // !defined(PRIVATE_FRAME_H)
