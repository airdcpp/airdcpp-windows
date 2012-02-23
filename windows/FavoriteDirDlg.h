/* 
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

#ifndef FAVORITEDIR_DLG_H
#define FAVORITEDIR_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <atlcrack.h>

#include "../client/Util.h"
#include "../client/ResourceManager.h"

#include "ExListViewCtrl.h"

class FavoriteDirDlg : public CDialogImpl<FavoriteDirDlg> {
public:

	enum { IDD = IDD_FAVORITEDIR };

	FavoriteDirDlg();
	~FavoriteDirDlg();

	BEGIN_MSG_MAP_EX(FavoriteDirDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_FAVDIR_ADD, onAddPath)
		COMMAND_ID_HANDLER(IDC_FAVDIR_REMOVE, onRemovePath)
		COMMAND_HANDLER(IDC_BROWSEFAV, BN_CLICKED, onBrowse)
		COMMAND_CODE_HANDLER(EN_CHANGE, onEditChange)
		NOTIFY_HANDLER(IDC_DIRPATHS, LVN_ITEMCHANGED, onItemchangedDirectories)
		MESSAGE_HANDLER(WM_DROPFILES, onDropFiles)
	END_MSG_MAP()


	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlName.SetFocus();
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onEditChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemchangedDirectories(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onAddPath(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onRemovePath(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onDropFiles(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	tstring vName;
	StringList paths;
private:
	CEdit ctrlName;
	ExListViewCtrl pathListCtrl;
	void addPath(const tstring& aPath);
};
#endif