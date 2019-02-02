/*
 * Copyright (C) 2012-2019 AirDC++ Project
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

#include <airdcpp/SettingsManager.h>
#include <airdcpp/ShareManager.h>
#include <airdcpp/ShareDirectoryInfo.h>

class SharePageBase {
public:
	SharePageBase();
	virtual ProfileToken getCurProfile() { return SETTING(DEFAULT_SP); }
	virtual ProfileToken getDefaultProfile() { return SETTING(DEFAULT_SP); }
	virtual ShareProfileInfo::List getProfiles() { return profiles; }
	virtual ShareProfileInfoPtr getProfile(ProfileToken aProfile);
protected:
	ShareProfileInfo::List profiles;
};

class ProfileDirectoryInfo;
typedef shared_ptr<ProfileDirectoryInfo> ProfileDirectoryInfoPtr;

class ProfileDirectoryInfo {
public:
	enum State {
		STATE_NORMAL,
		STATE_ADDED,
		STATE_REMOVED
	};

	//ProfileDirectoryInfo(const ShareDirInfoPtr& aInfo, ProfileToken aNewProfile);
	ProfileDirectoryInfo(ShareDirectoryInfoPtr& aDirectoryInfo, State aState = STATE_NORMAL);

	// item currently exists in the profile
	bool isCurItem() const {
		return state != STATE_REMOVED;
	}

	bool hasProfile(ProfileToken aToken) {
		return dir->profiles.find(aToken) != dir->profiles.end();
	}

	//ProfileToken profile;
	bool found; //used when detecting removed dirs with using dir tree

	State state;

	typedef vector<ProfileDirectoryInfoPtr> List;
	typedef unordered_map<int, List> Map;

	ShareDirectoryInfoPtr dir;

	class PathCompare {
	public:
		PathCompare(const string& compareTo) : a(compareTo) { }
		bool operator()(const ProfileDirectoryInfoPtr& p) const { return Util::stricmp(p->dir->path.c_str(), a.c_str()) == 0; }
		PathCompare& operator=(const PathCompare&) = delete;
	private:
		const string& a;
	};

	class HasProfile {
	public:
		HasProfile(ProfileToken compareTo) : a(compareTo) { }
		bool operator()(const ProfileDirectoryInfoPtr& p) const { return p->dir->profiles.find(a) != p->dir->profiles.end(); }
		HasProfile& operator=(const HasProfile&) = delete;
	private:
		const ProfileToken a;
	};
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

	enum ConfirmOption: uint8_t {
		CONFIRM_REMOVE,
		CONFIRM_LEAVE,
		CONFIRM_ASK
	};

	void onCopyProfile(ProfileToken aNewProfile);
	void onRemoveProfile(ProfileToken aProfile);

	void applyChanges(bool isQuit);
	void showProfile();
protected:
	//ProfileToken curProfile;
	friend class FolderTree;
	static TextItem texts[];
	ExListViewCtrl ctrlDirectories;

	unique_ptr<FolderTree> ft;

	bool showShareDlg(const ShareProfileInfo::List& spList, ProfileToken aCurProfile, const tstring& curName, ProfileTokenSet& profiles, tstring& newName, bool rename);
	void removeDir(const string& aPath, ProfileToken aToken, ConfirmOption& autoRemove, bool checkDupes=true, int remainingItems=0);
	bool addDirectory(const tstring& aPath);

	bool addExcludeFolder(const string& aPath);
	bool removeExcludeFolder(const string& path);
	bool shareFolder(const string& path);

	ProfileDirectoryInfoPtr getItemByPath(const string& aPath, bool listRemoved);

	ProfileDirectoryInfo::List shareDirs;

	ProfileDirectoryInfo::List getCurItems();

	SharePageBase* parent;

	StringSet excludedPaths;
};

#endif // !defined(SHARE_PAGE_H)
