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
#include "../client/DelayedEvents.h"

#include "FlatTabCtrl.h"
#include "WinUtil.h"
#include "UCHandler.h"
#include "EmoticonsDlg.h"
#include "HubFrame.h"

#include "ChatCtrl.h"
#include "ResourceLoader.h"

#define PM_MESSAGE_MAP 8		// This could be any number, really...
#define HUB_SEL_MAP 9

class PrivateFrame : public MDITabChildWindowImpl<PrivateFrame>, 
	private ClientManagerListener, public UCHandler<PrivateFrame>, private SettingsManagerListener
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
		COMMAND_ID_HANDLER(IDC_USER_HISTORY, onOpenUserLog)
		COMMAND_ID_HANDLER(IDC_GETLIST, onGetList)
		COMMAND_ID_HANDLER(IDC_BROWSELIST, onBrowseList)
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
	ALT_MSG_MAP(HUB_SEL_MAP)
		//COMMAND_HANDLER(IDC_HUB, CBN_SELENDOK, onHubChanged)
		//COMMAND_ID_HANDLER(IDC_HUB, onHubChanged)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, onHubChanged)
	ALT_MSG_MAP(PM_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_CHAR, onChar)
		MESSAGE_HANDLER(WM_KEYDOWN, onChar)
		MESSAGE_HANDLER(WM_KEYUP, onChar)
		MESSAGE_HANDLER(IDC_HANDLE_MAGNET, onMagnet)
		MESSAGE_HANDLER(WM_DROPFILES, onDropFiles)
	END_MSG_MAP()

	LRESULT onHubChanged(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBrowseList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onClientEnLink(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) { return ctrlClient.onClientEnLink(uMsg, wParam, lParam, bHandled); }
	LRESULT onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenUserLog(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEmoPackChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  	LRESULT onEmoticons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled);
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
	LRESULT onMagnet(UINT uMsg, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onDropFiles(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	void addMagnet(const tstring& path);
	void fillLogParams(ParamMap& params) const;
	void addLine(const tstring& aLine, CHARFORMAT2& cf);
	void addLine(const Identity&, const tstring& aLine);
	void addLine(const Identity&, const tstring& aLine, CHARFORMAT2& cf);
	void addStatusLine(const tstring& aLine);

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
		//updateTitle();
		updateOnlineStatus();
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
	enum {
		STATUS_TEXT,
		STATUS_HUBSEL_DESC,
		STATUS_HUBSEL_CTRL,
		STATUS_LAST
	};

	PrivateFrame(const HintedUser& replyTo_, Client* c);
	~PrivateFrame() { }

	bool created;
	string getLogPath() const;
	typedef unordered_map<UserPtr, PrivateFrame*, User::Hash> FrameMap;
	typedef FrameMap::const_iterator FrameIter;
	static FrameMap frames;
	ChatCtrl ctrlClient;
	CEdit ctrlMessage;
	CStatusBarCtrl ctrlStatus;
	CComboBox ctrlHubSel;
	CStatic ctrlHubSelDesc;

	void updateOnlineStatus();
	StringPairList hubs;
	string initialHub;
	bool online;
	void changeClient();
	void showHubSelection(bool show);

	int menuItems;
	int lineCount; //ApexDC
	OMenu emoMenu;
	CButton ctrlEmoticons;
	ExCImage hEmoticonBmp;


	HintedUser replyTo;
	
	CContainedWindow ctrlMessageContainer;
	CContainedWindow ctrlClientContainer;
	CContainedWindow ctrlHubSelContainer;

	bool closed;
	tstring hubName;
	ParamMap ucLineParams;

	void updateFrameOnlineStatus(const HintedUser& newUser, Client* c);
	TStringList prevCommands;
	tstring currentCommand;
	TStringList::size_type curCommandPosition;
	
	// ClientManagerListener
	void on(ClientManagerListener::UserUpdated, const OnlineUser& aUser) noexcept;
	void on(ClientManagerListener::UserConnected, const OnlineUser& aUser) noexcept;
	void on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) noexcept;

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;

	void addSpeakerTask();
	void runSpeakerTask();

	DelayedEvents<CID> delayEvents;
};

#endif // !defined(PRIVATE_FRAME_H)

/**
 * @file
 * $Id: PrivateFrame.h 473 2010-01-12 23:17:33Z bigmuscle $
 */
