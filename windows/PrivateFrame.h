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

#if !defined(PRIVATE_FRAME_H)
#define PRIVATE_FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/User.h"
#include "../client/ClientManagerListener.h"
#include "../client/ResourceManager.h"

#include "FlatTabCtrl.h"
#include "WinUtil.h"
#include "UCHandler.h"
#include "EmoticonsDlg.h"
#include "HubFrame.h"

#include "ChatCtrl.h"
#include "ResourceLoader.h"

#define PM_MESSAGE_MAP 8		// This could be any number, really...

class PrivateFrame : public MDITabChildWindowImpl<PrivateFrame>, 
	private ClientManagerListener, public UCHandler<PrivateFrame>, private SettingsManagerListener
{
public:
	static void gotMessage(const Identity& from, const UserPtr& to, const UserPtr& replyTo, const tstring& aMessage, Client* c);
	static void openWindow(const HintedUser& replyTo, const tstring& aMessage = Util::emptyStringT, Client* c = NULL);
	static bool isOpen(const UserPtr u) { return frames.find(u) != frames.end(); }
	static void closeAll();
	static void closeAllOffline();

	enum {
		USER_UPDATED
	};

	DECLARE_FRAME_WND_CLASS_EX(_T("PrivateFrame"), IDR_PRIVATE, 0, COLOR_3DFACE);

	typedef MDITabChildWindowImpl<PrivateFrame> baseClass;
	typedef UCHandler<PrivateFrame> ucBase;

	BEGIN_MSG_MAP(PrivateFrame)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(FTM_CONTEXTMENU, onTabContextMenu)
		COMMAND_ID_HANDLER(IDC_OPEN_USER_LOG, onOpenUserLog)
		COMMAND_ID_HANDLER(IDC_USER_HISTORY, onUserHistory)
		COMMAND_ID_HANDLER(IDC_GETLIST, onGetList)
		COMMAND_ID_HANDLER(IDC_MATCH_QUEUE, onMatchQueue)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT, onGrantSlot)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT_HOUR, onGrantSlot)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT_DAY, onGrantSlot)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT_WEEK, onGrantSlot)
		COMMAND_ID_HANDLER(IDC_UNGRANTSLOT, onGrantSlot)
		COMMAND_ID_HANDLER(IDC_ADD_TO_FAVORITES, onAddToFavorites)
		COMMAND_ID_HANDLER(IDC_SEND_MESSAGE, onSendMessage)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		COMMAND_ID_HANDLER(ID_EDIT_CLEAR_ALL, onEditClearAll)
		COMMAND_ID_HANDLER(IDC_EMOT, onEmoticons)
		COMMAND_ID_HANDLER(IDC_WINAMP_SPAM, onWinampSpam)
		COMMAND_ID_HANDLER(IDC_PUBLIC_MESSAGE, onPublicMessage)
		COMMAND_RANGE_HANDLER(IDC_EMOMENU, IDC_EMOMENU + menuItems, onEmoPackChange);
		CHAIN_COMMANDS(ucBase)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(PM_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CHAR, onChar)
		MESSAGE_HANDLER(WM_KEYDOWN, onChar)
		MESSAGE_HANDLER(WM_KEYUP, onChar)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onClientEnLink(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) { return ctrlClient.onClientEnLink(uMsg, wParam, lParam, bHandled); }
	LRESULT onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEmoPackChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  	LRESULT onEmoticons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled);
	LRESULT onUserHistory(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  	LRESULT onWinampSpam(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		tstring cmd, param, message, status;
		bool thirdPerson;
		if(SETTING(MEDIA_PLAYER) == 0) {
			cmd = _T("/winamp");
		} else if(SETTING(MEDIA_PLAYER) == 1) {
			cmd = _T("/wmp");
		} else if(SETTING(MEDIA_PLAYER) == 2) {
			cmd = _T("/itunes");
		} else if(SETTING(MEDIA_PLAYER) == 3) {
			cmd = _T("/mpc");
		} else {
			addClientLine(CTSTRING(NO_MEDIA_SPAM));
			return 0;
		}
		if(WinUtil::checkCommand(cmd, param, message, status, thirdPerson)){
			if(!message.empty()) {
				sendMessage(message);
			}
			if(!status.empty()) {
				addClientLine(status);
			}
		}
		return 0;
	}
	LRESULT onPublicMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void addLine(const tstring& aLine, CHARFORMAT2& cf);
	void addLine(const Identity&, const tstring& aLine);
	void addLine(const Identity&, const tstring& aLine, CHARFORMAT2& cf);
	void onEnter();
	void UpdateLayout(BOOL bResizeBars = TRUE);	
	void runUserCommand(UserCommand& uc);
	void readLog();
	
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
		
	LRESULT onSendMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		onEnter();
		return 0;
	}

	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}

	LRESULT PrivateFrame::onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) {
		updateTitle();
		return 0;
	}
	
	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		HWND hWnd = (HWND)lParam;
		HDC hDC = (HDC)wParam;
		if(hWnd == ctrlClient.m_hWnd || hWnd == ctrlMessage.m_hWnd) {
			::SetBkColor(hDC, WinUtil::bgColor);
			::SetTextColor(hDC, WinUtil::textColor);
			return (LRESULT)WinUtil::bgBrush;
		}
		bHandled = FALSE;
		return FALSE;
	}

	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlMessage.SetFocus();
		return 0;
	}
	
	void addClientLine(const tstring& aLine) {
		if(!created) {
			CreateEx(WinUtil::mdiClient);
		}
		ctrlStatus.SetText(0, (_T("[") + Text::toT(Util::getShortTimeString()) + _T("] ") + aLine).c_str());
		if (BOOLSETTING(BOLD_PM)) {
			setDirty();
		}
	}
	
	void sendMessage(const tstring& msg, bool thirdPerson = false);

private:
	PrivateFrame(const HintedUser& replyTo_, Client* c) : replyTo(replyTo_),
		priv(FavoriteManager::getInstance()->isPrivate(replyTo_.hint)),
		created(false), closed(false), isoffline(false), curCommandPosition(0),  
		ctrlMessageContainer(WC_EDIT, this, PM_MESSAGE_MAP),
		ctrlClientContainer(WC_EDIT, this, PM_MESSAGE_MAP), menuItems(0)
	{
		ctrlClient.setClient(c);
	}
	
	~PrivateFrame() { }

	bool created;
	typedef unordered_map<UserPtr, PrivateFrame*, User::Hash> FrameMap;
	typedef FrameMap::const_iterator FrameIter;
	static FrameMap frames;
	ChatCtrl ctrlClient;
	CEdit ctrlMessage;
	CStatusBarCtrl ctrlStatus;

	int menuItems;
	int lineCount; //ApexDC
	OMenu emoMenu;
	CButton ctrlEmoticons;
	ExCImage hEmoticonBmp;


	HintedUser replyTo;
	const bool priv;
	
	CContainedWindow ctrlMessageContainer;
	CContainedWindow ctrlClientContainer;

	bool closed;
	bool isoffline;
	tstring hubName;
	StringMap ucLineParams;

	void updateTitle();
	
	TStringList prevCommands;
	tstring currentCommand;
	TStringList::size_type curCommandPosition;
	
	// ClientManagerListener
	void on(ClientManagerListener::UserUpdated, const OnlineUser& aUser) noexcept {
		if(aUser.getUser() == replyTo.user) {
			ctrlClient.setClient(const_cast<Client*>(&aUser.getClient()));
			PostMessage(WM_SPEAKER, USER_UPDATED);
		}
	}
	void on(ClientManagerListener::UserConnected, const UserPtr& aUser) noexcept {
		if(aUser == replyTo.user)
			PostMessage(WM_SPEAKER, USER_UPDATED);
	}
	void on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) noexcept {
		if(aUser == replyTo.user) {
			ctrlClient.setClient(NULL);
			PostMessage(WM_SPEAKER, USER_UPDATED);
		}
	}
	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;
};

#endif // !defined(PRIVATE_FRAME_H)

/**
 * @file
 * $Id: PrivateFrame.h 473 2010-01-12 23:17:33Z bigmuscle $
 */
