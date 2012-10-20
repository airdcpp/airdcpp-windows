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

#if !defined(SHARE_PAGE_H)
#define SHARE_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"
#include "WinUtil.h"
#include "FolderTree.h"
#include "../client/SettingsManager.h"
#include "../client/ShareManager.h"

class SharePage : public CPropertyPage<IDD_SHAREPAGE>, public PropPage
{
public:
	SharePage(SettingsManager *s);
	~SharePage();

	BEGIN_MSG_MAP_EX(SharePage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		MESSAGE_HANDLER(WM_DROPFILES, onDropFiles)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		NOTIFY_HANDLER(IDC_DIRECTORIES, LVN_ITEMCHANGED, onItemchangedDirectories)
		NOTIFY_HANDLER(IDC_DIRECTORIES, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_DIRECTORIES, NM_DBLCLK, onDoubleClick)
		NOTIFY_HANDLER(IDC_DIRECTORIES, LVN_COLUMNCLICK, onColumnClick)
		NOTIFY_HANDLER(IDC_DIRECTORIES, NM_CUSTOMDRAW, onCustomDrawList)
		COMMAND_HANDLER(IDC_PROFILE_SEL, CBN_SELCHANGE , onProfileChanged)
		COMMAND_ID_HANDLER(IDC_ADD_DIR, onClickedAddDir)
		COMMAND_ID_HANDLER(IDC_REMOVE_DIR, onClickedRemoveDir)
		COMMAND_ID_HANDLER(IDC_RENAME_DIR, onClickedRenameDir)
		COMMAND_ID_HANDLER(IDC_ADD_PROFILE, onClickedAddProfile)
		COMMAND_ID_HANDLER(IDC_ADD_PROFILE_COPY, onClickedCopyProfile)
		COMMAND_ID_HANDLER(IDC_REMOVE_PROFILE, onClickedRemoveProfile)
		COMMAND_ID_HANDLER(IDC_RENAME_PROFILE, onClickedRenameProfile)
		COMMAND_ID_HANDLER(IDC_APPLY_CHANGES, onApplyChanges)
		COMMAND_ID_HANDLER(IDC_SHOW_TREE, onClickedShowTree)
		COMMAND_ID_HANDLER(IDC_EDIT_TEMPSHARES, onEditTempShares)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onDropFiles(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onItemchangedDirectories(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onClickedAddDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedRemoveDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedRenameDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedAddProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedCopyProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedRemoveProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedRenameProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onProfileChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedShowTree(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedRefreshDisable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onColumnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onEditTempShares(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onApplyChanges(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCustomDrawList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);

	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();

	enum ConfirmOption {
		CONFIRM_REMOVE,
		CONFIRM_LEAVE,
		CONFIRM_ASK
	};
protected:
	ProfileToken curProfile;
	friend class FolderTree;
	static TextItem texts[];
	ExListViewCtrl ctrlDirectories;

	ShareProfilePtr addProfile();

	FolderTree* ft;
	map<string, FolderTree*> ftl;

	void showProfile();
	void applyChanges(bool isQuit);
	void fixControls();
	void removeDir(const string& aPath, ProfileToken aProfile, int8_t& autoRemove, bool checkDupes=true, int remainingItems=0);
	bool addDirectory(const tstring& aPath);
	ShareDirInfo::list removeDirs, newDirs, changedDirs, tempViewItems;

	bool addExcludeFolder(const string& aPath);
	bool removeExcludeFolder(const string& path);
	bool shareFolder(const string& path, ShareDirInfo::list& aShared);

	ShareProfile::set addProfiles;
	ProfileTokenList removeProfiles;
	vector<pair<ShareProfilePtr, string>> renameProfiles;
	ShareProfilePtr getSelectedProfile();
	ShareDirInfo::list getItemsByPath(const string& aPath, bool listRemoved);

	ShareDirInfo::map shareDirs;
	vector<ShareProfilePtr> profiles;
	CComboBox ctrlProfile;

	ShareDirInfo::list getViewItems(ProfileToken aProfile, bool getDiffItems=false);
	StringSet getExcludedDirs();

	ShareProfilePtr getProfile(ProfileToken aProfile);

	ProfileTokenStringList excludedAdd, excludedRemove;

	void deleteDirectoryInfoItems();
	void deleteTempViewItems();
	bool showShareDlg(const ShareProfile::list& spList, ProfileToken curProfile, const tstring& curName, ProfileTokenList& profiles, tstring& newName, bool rename);
};

#endif // !defined(SHARE_PAGE_H)
