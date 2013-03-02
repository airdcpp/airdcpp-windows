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

#if !defined(SYSTEM_FRAME_H)
#define SYSTEM_FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "../client/LogManager.h"
#include "../client/TaskQueue.h"

#define SYSTEM_LOG_MESSAGE_MAP 42

class SystemFrame : public MDITabChildWindowImpl<SystemFrame>, public StaticFrame<SystemFrame, ResourceManager::SYSTEM_LOG, IDC_SYSTEM_LOG>,
	private LogManagerListener, private SettingsManagerListener
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("SystemFrame"), IDR_SYSTEM_LOG, 0, COLOR_3DFACE);

	SystemFrame() : ctrlClientContainer(_T("edit"), this, SYSTEM_LOG_MESSAGE_MAP), errorNotified(false), lButtonDown(false), hasMessages(false) { 
		hbError = NULL;
		hbInfo = NULL;
		hbWarning = NULL;
	}
	~SystemFrame() { }
	
	typedef MDITabChildWindowImpl<SystemFrame> baseClass;
	BEGIN_MSG_MAP(SystemFrame)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(FTM_CONTEXTMENU, onTabContextMenu)
		MESSAGE_HANDLER(WM_REFRESH_SETTINGS, onRefreshSettings)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, onEditCopy)
		COMMAND_ID_HANDLER(IDC_COPY_DIR, onCopyDir)
		COMMAND_ID_HANDLER(IDC_OPEN_SYSTEM_LOG, onSystemLog)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		COMMAND_ID_HANDLER(IDC_SEARCH, onSearchFile)
		COMMAND_ID_HANDLER(IDC_SEARCHDIR, onSearchDir)
		COMMAND_ID_HANDLER(ID_EDIT_SELECT_ALL, onEditSelectAll)
		COMMAND_ID_HANDLER(ID_EDIT_CLEAR_ALL, onEditClearAll)
		COMMAND_ID_HANDLER(IDC_OPEN_FOLDER, onOpenFolder)
		COMMAND_ID_HANDLER(IDC_DELETE_FILE, onDeleteFile)
		COMMAND_ID_HANDLER(IDC_ADD_AUTO_SEARCH_FILE, onAddAutoSearchFile)
		COMMAND_ID_HANDLER(IDC_ADD_AUTO_SEARCH_DIR, onAddAutoSearchDir)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(SYSTEM_LOG_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK, onLButton)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, onLButtonDown)
		MESSAGE_HANDLER(WM_LBUTTONUP, onLButtonUp)
	END_MSG_MAP()

	enum Tasks { ADD_LINE };

	void UpdateLayout(BOOL bResizeBars = TRUE);

	LRESULT OnRButtonDown(POINT pt);
	LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onLButton(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onSystemLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRefreshSettings(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onSearchFile(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchDir(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDeleteFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAddAutoSearchFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAddAutoSearchDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		lButtonDown = true;
		bHandled = FALSE;
		return 0;
	}

	LRESULT onLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		lButtonDown = false;
		if(hasMessages)
			PostMessage(WM_SPEAKER);

		hasMessages = false;
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlPad.SetFocus();
		return 0;
	}
	
	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}

	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		HWND hWnd = (HWND)lParam;
		HDC hDC = (HDC)wParam;
		if(hWnd == ctrlPad.m_hWnd) {
			::SetBkColor(hDC, WinUtil::bgColor);
			::SetTextColor(hDC, WinUtil::textColor);
			DeleteDC(hDC);
			return (LRESULT)WinUtil::bgBrush;
		}
		DeleteDC(hDC);
		bHandled = FALSE;
		return FALSE;
	}

private:
	
	struct MessageTask : public StringTask {
		MessageTask(const string& msg, LogManager::MessageData _data) : StringTask(msg), data(_data) { }

		LogManager::MessageData data;
	};


	CContainedWindow ctrlClientContainer;

	CRichEditCtrl ctrlPad;
	CMenu tabMenu;
	virtual void on(Message, time_t t, const string& message, uint8_t sev);
	virtual void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept;
	void addLine(LogManager::MessageData, const tstring& msg);

	void scrollToEnd();

	void Colorize(const tstring& line, LONG Begin);
	boost::wregex reg;

	HBITMAP  hbInfo;
	HBITMAP  hbWarning;
	HBITMAP  hbError;

	HICON tabError;
	HICON tabNormal;

	bool errorNotified;
	bool lButtonDown;
	bool hasMessages;
	TaskQueue messages;

	bool scrollIsAtEnd();
	tstring selWord;
	tstring WordFromPos(const POINT& p);

	/* add the messages as tasks, if the user is currently making a selection don't add text to the window
	before mouse button is up and selection is complete */
	void speak(Tasks s, const string& msg, LogManager::MessageData mdata) { 
		messages.add(static_cast<uint8_t>(s), unique_ptr<Task>(new MessageTask(msg, mdata)));
		if(!lButtonDown)
			PostMessage(WM_SPEAKER); 
		else
			hasMessages = true;
	}

};

#endif // !defined(SYSTEM_FRAME_H)