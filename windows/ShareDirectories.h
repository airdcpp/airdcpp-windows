/*
 * Copyright (C) 2012-2013 AirDC++ Project
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

#ifndef SHARE_DIRECTORIES_H
#define SHARE_DIRECTORIES_H

#include <atlcrack.h>

#include "ExListViewCtrl.h"
#include "FolderTree.h"
#include "PropPage.h"

#include "../client/SettingsManager.h"
#include "../client/ShareManager.h"

class SharePageBase {
public:
	virtual ProfileToken getCurProfile() { return SP_DEFAULT; }
	virtual const vector<ShareProfilePtr>& getProfiles() { return ShareManager::getInstance()->getProfiles(); }
	virtual ShareProfilePtr getProfile(ProfileToken aProfile) { return ShareManager::getInstance()->getProfile(aProfile); }
};

class ShareDirectories : public SettingTab, public CDialogImpl<ShareDirectories>
{
public:
	ShareDirectories(SharePageBase* aParent, SettingsManager *s);
	~ShareDirectories();

	enum { IDD = IDD_SHAREDIRS };

	BEGIN_MSG_MAP_EX(ShareDirectories)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		MESSAGE_HANDLER(WM_DROPFILES, onDropFiles)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		NOTIFY_HANDLER(IDC_DIRECTORIES, LVN_ITEMCHANGED, onItemchangedDirectories)
		NOTIFY_HANDLER(IDC_DIRECTORIES, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_DIRECTORIES, NM_DBLCLK, onDoubleClick)
		NOTIFY_HANDLER(IDC_DIRECTORIES, LVN_COLUMNCLICK, onColumnClick)
		NOTIFY_HANDLER(IDC_DIRECTORIES, NM_CUSTOMDRAW, onCustomDrawList)
		COMMAND_ID_HANDLER(IDC_ADD_DIR, onClickedAddDir)
		COMMAND_ID_HANDLER(IDC_REMOVE_DIR, onClickedRemoveDir)
		COMMAND_ID_HANDLER(IDC_RENAME_DIR, onClickedRenameDir)
		COMMAND_ID_HANDLER(IDC_SHOW_TREE, onClickedShowTree)
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
	LRESULT onClickedShowTree(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedRefreshDisable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onColumnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onCustomDrawList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	// Common PropPage interface
	void write();
	Dispatcher::F getThreadedTask();

	enum ConfirmOption {
		CONFIRM_REMOVE,
		CONFIRM_LEAVE,
		CONFIRM_ASK
	};

	void onCopyProfile(ProfileToken aNewProfile);
	void onRemoveProfile();

	bool hasChanged();
	void applyChanges(bool isQuit);
	void showProfile();
protected:
	//ProfileToken curProfile;
	friend class FolderTree;
	static TextItem texts[];
	ExListViewCtrl ctrlDirectories;

	unique_ptr<FolderTree> ft;

	bool showShareDlg(const ShareProfile::List& spList, ProfileToken aCurProfile, const tstring& curName, ProfileTokenList& profiles, tstring& newName, bool rename);
	void removeDir(const string& aPath, ProfileToken aProfile, int8_t& autoRemove, bool checkDupes=true, int remainingItems=0);
	bool addDirectory(const tstring& aPath);

	ShareDirInfo::List removeDirs, changedDirs, tempViewItems;
	ShareDirInfo::List newDirs;

	bool addExcludeFolder(const string& aPath);
	bool removeExcludeFolder(const string& path);
	bool shareFolder(const string& path, ShareDirInfo::List& aShared);

	ShareDirInfo::List getItemsByPath(const string& aPath, bool listRemoved);

	ShareDirInfo::Map shareDirs;

	ShareDirInfo::List getViewItems(ProfileToken aProfile, bool getDiffItems=false);
	StringSet getExcludedDirs();

	ProfileTokenStringList excludedAdd, excludedRemove;

	void deleteTempViewItems();

	SharePageBase* parent; 
};

#endif // !defined(SHARE_PAGE_H)
