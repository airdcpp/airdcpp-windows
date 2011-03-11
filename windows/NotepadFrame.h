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

#if !defined(NOTEPAD_FRAME_H)
#define NOTEPAD_FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "WinUtil.h"

#define NOTEPAD_MESSAGE_MAP 13

class NotepadFrame : public MDITabChildWindowImpl<NotepadFrame>, public StaticFrame<NotepadFrame, ResourceManager::NOTEPAD, IDC_NOTEPAD>, 
	private SettingsManagerListener
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("NotepadFrame"), IDR_NOTEPAD, 0, COLOR_3DFACE);

	NotepadFrame() : dirty(false), closed(false), 
		ctrlClientContainer(_T("edit"), this, NOTEPAD_MESSAGE_MAP) {
		SettingsManager::getInstance()->addListener(this);
	}
	~NotepadFrame() { }
	
	typedef MDITabChildWindowImpl<NotepadFrame> baseClass;
	BEGIN_MSG_MAP(NotepadFrame)
		MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(NOTEPAD_MESSAGE_MAP)
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK, onLButton)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onLButton(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	void UpdateLayout(BOOL bResizeBars = TRUE);
	
	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		HWND hWnd = (HWND)lParam;
		HDC hDC = (HDC)wParam;
		if(hWnd == ctrlPad.m_hWnd) {
			::SetBkColor(hDC, WinUtil::bgColor);
			::SetTextColor(hDC, WinUtil::textColor);
			return (LRESULT)WinUtil::bgBrush;
		}
		bHandled = FALSE;
		return FALSE;
	}
	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}
	
	LRESULT OnFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlPad.SetFocus();
		return 0;
	}
	
private:
	
	bool dirty;
	bool closed;
	CEdit ctrlPad;
	CContainedWindow ctrlClientContainer;

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw();
};

#endif // !defined(NOTEPAD_FRAME_H)

/**
 * @file
 * $Id: NotepadFrame.h 308 2007-07-13 18:57:02Z bigmuscle $
 */
