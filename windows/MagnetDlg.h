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

#if !defined(MAGNET_DLG_H)
#define MAGNET_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/QueueManager.h"

// (Modders) Enjoy my liberally commented out source code.  The plan is to enable the
// magnet link add an entry to the download queue, with just the hash (if that is the
// only information the magnet contains).  DC++ has to find sources for the file anyway,
// and can take filename, size, etc. values from there.
//                                                        - GargoyleMT

class MagnetDlg : public CDialogImpl<MagnetDlg > {
public:
	enum { IDD = IDD_MAGNET };

	MagnetDlg(const string& aHash, const tstring& aFileName, const int64_t aSize) : mHash(aHash), mFileName(aFileName), mSize(aSize), remember(false), sel(0) { }
	~MagnetDlg() { }

	BEGIN_MSG_MAP(MagnetDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDOK, onCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, onCloseCmd)
		COMMAND_ID_HANDLER(IDC_MAGNET_QUEUE, onRadioButton)
		COMMAND_ID_HANDLER(IDC_MAGNET_NOTHING, onRadioButton)
		COMMAND_ID_HANDLER(IDC_MAGNET_SEARCH, onRadioButton)
	END_MSG_MAP();

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRadioButton(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	bool remember;
	int sel;
private:
	tstring mFileName;
	string mHash;
	int64_t mSize;
};

#endif // !defined(MAGNET_DLG_H)