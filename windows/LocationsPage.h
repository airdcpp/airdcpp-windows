/*
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#if !defined(LOCATIONS_PAGE_H)
#define LOCATIONS_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

class LocationsPage : public CPropertyPage<IDD_LOCATIONSPAGE>, public PropPage
{
public:
	LocationsPage(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_DOWNLOADS) + _T('\\') + TSTRING(SETTINGS_DOWNLOAD_LOCATIONS)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
	}
	~LocationsPage() {
		ctrlDirectories.Detach();
		free(title);
	}

	BEGIN_MSG_MAP_EX(LocationsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		NOTIFY_HANDLER(IDC_FAVORITE_DIRECTORIES, LVN_ITEMCHANGED, onItemChangedDirectories)
		NOTIFY_HANDLER(IDC_FAVORITE_DIRECTORIES, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_FAVORITE_DIRECTORIES, NM_DBLCLK, onDoubleClick)

		COMMAND_ID_HANDLER(IDC_ADD, onClickedAdd)
		COMMAND_ID_HANDLER(IDC_REMOVE, onClickedRemove)
		COMMAND_ID_HANDLER(IDC_RENAME, onClickedRename)

		COMMAND_ID_HANDLER(IDC_BROWSEDIR, onClickedBrowseDir)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onItemChangedDirectories(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onClickedAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedRename(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT onClickedBrowseDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
protected:
	static Item items[];
	static TextItem texts[];
	ExListViewCtrl ctrlDirectories;
	TCHAR* title;
	FavoriteManager::FavoriteDirectoryMap favoriteDirs;
	CComboBox ctrlAutoSelect;

	FavoriteManager::FavoriteDirectoryMap::iterator getByPosition(int aPos) noexcept;

	bool renameDirectory(const string& aPath, const string& aNewName) noexcept;
	bool removeDirectory(const string& aPath) noexcept;
	void addDirectory(const string& aPath, const string& aName) noexcept;

	void fillList();
};

#endif // !defined(FAVORITE_DIR_SPAGE_H)