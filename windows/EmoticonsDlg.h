/* 
 * Copyright (C) Rm.
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

#ifndef __EMOTICONS_DLG
#define __EMOTICONS_DLG

#include "../client/typedefs.h"

class EmoticonsDlg : public CDialogImpl<EmoticonsDlg>
{
public:
	enum { IDD = IDD_EMOTICONS_DLG };


	BEGIN_MSG_MAP(EmoticonsDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		MESSAGE_HANDLER(WM_RBUTTONUP, onClose)
		MESSAGE_HANDLER(WM_LBUTTONUP, onClose)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		COMMAND_CODE_HANDLER(BN_CLICKED, onEmoticonClick)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onEmoticonClick(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		EndDialog(0);
		return 0;
	}

	tstring result;
	CRect pos;

private:
	CToolTipCtrl ctrlToolTip;
	static vector<HBITMAP> bitmapList;

	static WNDPROC m_MFCWndProc;
	static EmoticonsDlg* m_pDialog;
	static LRESULT CALLBACK NewWndProc( HWND, UINT, WPARAM, LPARAM );

};

#endif // __EMOTICONS_DLG