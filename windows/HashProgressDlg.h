/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(HASH_PROGRESS_DLG_H)
#define HASH_PROGESS_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"

class HashProgressDlg : public CDialogImpl<HashProgressDlg>
{
public:
	enum { IDD = IDD_HASH_PROGRESS };
	enum { WM_VERSIONDATA = WM_APP + 53 };

	HashProgressDlg(bool aAutoClose = false);
	virtual ~HashProgressDlg() { }

	BEGIN_MSG_MAP(HashProgressDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_TIMER, onTimer)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		// KUL - hash progress dialog patch
		COMMAND_HANDLER(IDC_MAX_HASH_SPEED, EN_CHANGE ,onMaxHashSpeed)
		COMMAND_ID_HANDLER(IDC_PAUSE, onPause)
		COMMAND_ID_HANDLER(IDC_CLEAR, onClear)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD/* wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);


	// KUL - hash progress dialog patch (begin)
	LRESULT onMaxHashSpeed(WORD /*wNotifyCode*/, WORD, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	// KUL - hash progress dialog patch (end)
		
	LRESULT onClear(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	void updateStats();
	void setAutoClose(bool val);

private:
	bool autoClose;
	int64_t startBytes;
	size_t startFiles;
	CProgressBarCtrl progress;
	bool init;
	int hashers;
};

#endif // !defined(HASH_PROGRESS_DLG_H)