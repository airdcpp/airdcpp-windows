/* 
 * Copyright (C) 2012-2016 AirDC++ team
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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>

class TempShareDlg : public CDialogImpl<TempShareDlg>
{
public:

	enum { IDD = IDD_TEMPSHARE_DLG };
	
	ExListViewCtrl ctrlFiles;

	BEGIN_MSG_MAP(TempShareDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_TMP_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_TMP_REMOVE_ALL, onRemoveAll)
	END_MSG_MAP()
	
	TempShareDlg() { }
	~TempShareDlg() { }
	

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};

