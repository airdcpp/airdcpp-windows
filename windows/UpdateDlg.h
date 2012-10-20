/* 
 * Copyright (C) 2002-2004 "Opera", <opera at home dot se>
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

#ifndef __UPDATE_DLG
#define __UPDATE_DLG

#include <atlctrlx.h>
#include "RichTextBox.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class UpdateDlg : public CDialogImpl<UpdateDlg> {
	CEdit ctrlCurrentVersion;
	CEdit ctrlLatestVersion;
	CEdit ctrlLatestLanguage;
	CButton ctrlDownload;
	CButton ctrlClose;
public:

	enum { IDD = IDD_UPDATE };

	enum {
		UPDATE_CURRENT_VERSION,
		UPDATE_LATEST_VERSION,
		UPDATE_STATUS,
		UPDATE_CONTENT
	};

	BEGIN_MSG_MAP(UpdateDlg)
		NOTIFY_HANDLER(IDC_UPDATE_HISTORY_TEXT, EN_LINK, onClientEnLink)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		COMMAND_ID_HANDLER(IDC_UPDATE_DOWNLOAD, OnDownload)
		COMMAND_ID_HANDLER(IDCLOSE, OnCloseCmd)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)	
	END_MSG_MAP()

	UpdateDlg(const string& aTitle, const string& aMessage, const string& aVersion, const string& infoUrl, bool autoUpdate, int build, const string& autoUpdateUrl);
	~UpdateDlg();

	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlClose.SetFocus();
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDownload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		EndDialog(0);
		return 0;
	}

	LRESULT onClientEnLink(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) { 
		return m_Changelog.handleLink(*(ENLINK*)pnmh);
	}

	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		EndDialog(wID);
		return 0;
	}
	LRESULT onCtlColor(UINT, WPARAM, LPARAM, BOOL&);
	
	static bool canAutoUpdate(const string& url);
private:
	CHyperLink url;
	CStatic ctrlUpdateStatus;
	bool versionAvailable;

	string message;
	string downloadURL;
	string infoLink;
	string title;
	string autoUpdateUrl;
	int buildID;

	string version;
	bool autoUpdate;
	HICON m_hIcon;

	RichTextBox m_Changelog;
};

#endif // __UPDATE_DLG
