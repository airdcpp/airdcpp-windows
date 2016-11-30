/*
 * Copyright (C) 2011-2016 AirDC++ Project
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

#ifndef CHATFRAME_BASE_H
#define CHATFRAME_BASE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <airdcpp/concurrency.h>

#include "Async.h"
#include "OMenu.h"
#include "resource.h"
#include "RichTextBox.h"
#include "ResourceLoader.h"
#include "ExCImage.h"
#include "FlatTabCtrl.h"


#define EDIT_MESSAGE_MAP 10		// This could be any number, really...

class ChatFrameBase : public MDITabChildWindowImpl<ChatFrameBase>, public Async<ChatFrameBase> {
public:
	typedef MDITabChildWindowImpl<ChatFrameBase> baseClass;

	BEGIN_MSG_MAP(ChatFrameBase)
		COMMAND_ID_HANDLER(IDC_BMAGNET, onAddMagnet)
		COMMAND_ID_HANDLER(IDC_WINAMP_SPAM, onWinampSpam)
		COMMAND_ID_HANDLER(IDC_EMOT, onEmoticons)
		COMMAND_ID_HANDLER(IDC_SEND_MESSAGE, onSendMessage)
		COMMAND_ID_HANDLER(IDC_RESIZE, onResize)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_FORWARDMSG, OnForwardMsg)
		COMMAND_RANGE_HANDLER(IDC_EMOMENU, IDC_EMOMENU + menuItems, onEmoPackChange)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnForwardMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onDropFiles(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onAddMagnet(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled);

	LRESULT onWinampSpam(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEmoticons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled);
	LRESULT onEmoPackChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onSendMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled);
	LRESULT onResize(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled);

	virtual bool checkFrameCommand(tstring& cmd, tstring& param, tstring& message, tstring& status, bool& thirdPerson) = 0;
	virtual bool sendMessage(const tstring& aMessage, string& error_, bool thirdPerson) = 0;
	virtual void addStatusLine(const tstring& aStatus, uint8_t sev) = 0;
	virtual void addPrivateLine(const tstring& aLine, CHARFORMAT2& cf) = 0;
	virtual void onTab() { };
	virtual void UpdateLayout(BOOL bResizeBars = TRUE) = 0;
protected:
	ChatFrameBase();
	~ChatFrameBase();

	//CContainedWindow ctrlMessageContainer;
	//CContainedWindow clientContainer;

	int menuItems;
	tstring complete;
	bool inHistory = false;
	int lineCount; //ApexDC

	OMenu emoMenu;

	CEdit ctrlMessage;
	CButton ctrlEmoticons;
	CButton ctrlMagnet;
	CButton ctrlResize;
	CButton ctrlSendMessage;
	CToolTipCtrl ctrlTooltips;
	bool resizePressed;

	RichTextBox ctrlClient;

	TStringList prevCommands;
	tstring currentCommand;
	TStringList::size_type curCommandPosition;		//can't use an iterator because StringList is a vector, and vector iterators become invalid after resizing

	void addMagnet(const StringList& aPaths);
	void init(HWND aHWND, RECT aRcDefault);

	void appendTextLine(const tstring& aText, bool addSpace);

	void handleSendMessage();
	static tstring commands;

	void getLineText(tstring& s);

	bool cancelHashing;
	task_group tasks;

	void setStatusText(const tstring& aLine, uint8_t severity);
	void setStatusText(const tstring& aLine, HICON aIcon);

	CStatusBarCtrl ctrlStatus;

	bool sendFrameMessage(const tstring& aMsg, bool thirdPerson = false);
	string getAwayMessage();
private:
	/**
	 * Check if this is a common /-command.
	 * @param cmd The whole text string, will be updated to contain only the command.
	 * @param param Set to any parameters.
	 * @param message Message that should be sent to the chat.
	 * @param status Message that should be shown in the status line.
	 * @return True if the command was processed, false otherwise.
	 */
	bool checkCommand(tstring& cmd, tstring& param, tstring& message, tstring& status, bool& thirdPerson);
	UserPtr getUser() { return ctrlClient.getPmUser(); }
	ClientPtr getClient() { return ctrlClient.getClient(); }
	const tstring& getSendFileTitle();
};

#endif // !defined(CHATFRAME_BASE_H)