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

#include "stdafx.h"

#include "../client/DCPlusPlus.h"
#include "../client/Util.h"
#include "../client/SettingsManager.h"
#include "../client/version.h"

#include "Resource.h"
#include "SharePage.h"
#include "WinUtil.h"
#include "HashProgressDlg.h"
#include "LineDlg.h"
#include "PropertiesDlg.h"
#include "TempShare_dlg.h"

#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/remove_if.hpp>
#include <boost/range/algorithm/find_if.hpp>

PropPage::TextItem SharePage::texts[] = {
	{ IDC_SETTINGS_SHARED_DIRECTORIES, ResourceManager::SETTINGS_SHARED_DIRECTORIES },
	{ IDC_ADD_PROFILE, ResourceManager::ADD_PROFILE },
	{ IDC_ADD_PROFILE_COPY, ResourceManager::ADD_PROFILE_COPY },
	{ IDC_REMOVE_PROFILE, ResourceManager::REMOVE_PROFILE },
	{ IDC_SETTINGS_SHARE_PROFILES, ResourceManager::SHARE_PROFILES },
	{ IDC_SHAREHIDDEN, ResourceManager::SETTINGS_SHARE_HIDDEN },
	{ IDC_REMOVE_DIR, ResourceManager::REMOVE },
	{ IDC_ADD_DIR, ResourceManager::SETTINGS_ADD_FOLDER },
	{ IDC_RENAME_DIR, ResourceManager::SETTINGS_RENAME_FOLDER },
	{ IDC_SETTINGS_ONLY_HASHED, ResourceManager::SETTINGS_ONLY_HASHED },
	{ IDC_SETTINGS_AUTO_REFRESH_TIME, ResourceManager::SETTINGS_AUTO_REFRESH_TIME },
	{ IDC_SETTINGS_INCOMING_REFRESH_TIME, ResourceManager::SETTINGS_INCOMING_REFRESH_TIME },
	{ IDC_SETTINGS_MAX_HASH_SPEED, ResourceManager::SETTINGS_MAX_HASH_SPEED },
	{ IDC_SETTINGS_MBS, ResourceManager::MBPS },
	{ IDC_SHARE_SAVE, ResourceManager::SAVE_SHARE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item SharePage::items[] = {
	{ IDC_SHAREHIDDEN, SettingsManager::SHARE_HIDDEN, PropPage::T_BOOL },
	{ IDC_AUTO_REFRESH_TIME, SettingsManager::AUTO_REFRESH_TIME, PropPage::T_INT },
	{ IDC_INCOMING_REFRESH_TIME, SettingsManager::INCOMING_REFRESH_TIME, PropPage::T_INT },
	{ IDC_MAX_HASH_SPEED, SettingsManager::MAX_HASH_SPEED, PropPage::T_INT },
	{ IDC_SHARE_SAVE_TIME, SettingsManager::SHARE_SAVE_TIME, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

SharePage::SharePage(SettingsManager *s) : PropPage(s), ft(nullptr) {
	SetTitle(CTSTRING(SETTINGS_SHARINGPAGE));
	m_psp.dwFlags |= PSP_RTLREADING;
}

LRESULT SharePage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);

	CheckDlgButton(IDC_SHOW_TREE, BOOLSETTING(USE_OLD_SHARING_UI) ? BST_UNCHECKED : BST_CHECKED);

	GetDlgItem(IDC_TREE1).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_DIRECTORIES).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_ADD_DIR).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_REMOVE_DIR).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_RENAME_DIR).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);

	GetDlgItem(IDC_EDIT_TEMPSHARES).EnableWindow((ShareManager::getInstance()->hasTempShares()) ? 1 : 0);

	ctrlDirectories.Attach(GetDlgItem(IDC_DIRECTORIES));
		ctrlDirectories.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES);

	PropPage::read((HWND)*this, items);

	CUpDownCtrl updown;
	updown.Attach(GetDlgItem(IDC_REFRESH_SPIN));
	updown.SetRange32(0, 3000); 
	updown.Detach();

	updown.Attach(GetDlgItem(IDC_INCOMING_SPIN));
	updown.SetRange32(0, 3000); 
	updown.Detach();

	updown.Attach(GetDlgItem(IDC_HASH_SPIN));
	updown.SetRange32(0, 999);
	updown.Detach();
	
	updown.Attach(GetDlgItem(IDC_SAVE_SPIN));
	updown.SetRange32(0, 3000); 
	updown.Detach();

	curProfile = SP_DEFAULT;
	if(BOOLSETTING(USE_OLD_SHARING_UI)) {
		// Prepare shared dir list
		ctrlDirectories.InsertColumn(0, CTSTRING(VIRTUAL_NAME), LVCFMT_LEFT, 80, 0);
		ctrlDirectories.InsertColumn(1, CTSTRING(DIRECTORY), LVCFMT_LEFT, 197, 1);
		ctrlDirectories.InsertColumn(2, CTSTRING(SIZE), LVCFMT_RIGHT, 90, 2);
	}

	ShareManager::getInstance()->getShares(shareDirs);
	showProfile();

	ctrlProfile.Attach(GetDlgItem(IDC_PROFILE_SEL));
	auto profileList = ShareManager::getInstance()->getProfiles();
	int n = 0;
	for(auto j = profileList.begin(); j != profileList.end(); j++) {
		if ((*j)->getToken() != SP_HIDDEN) {
			ctrlProfile.InsertString(n, Text::toT((*j)->getDisplayName()).c_str());
			n++;
			profiles.push_back((*j));
		}
	}

	ctrlProfile.SetCurSel(0);
	return TRUE;
}

ShareProfilePtr SharePage::getSelectedProfile() {
	return profiles[ctrlProfile.GetCurSel()];
}

LRESULT SharePage::onProfileChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto p = getSelectedProfile();
	curProfile = p->getToken();
	showProfile();
	fixControls();
	return 0;
}

ShareDirInfo::list SharePage::getViewItems() {
	ShareDirInfo::list ret;

	auto& dirs = shareDirs[curProfile];
	for(auto i = dirs.begin(); i != dirs.end(); i++) {
		if (find(removeDirs.begin(), removeDirs.end(), *i) == removeDirs.end()) {
			ret.push_back(*i);
		}
	}

	for(auto j = newDirs.begin(); j != newDirs.end(); j++) {
		if (j->profile == curProfile) {
			//auto sdi = ShareDirInfo(j->vname, curProfile, j->path, j->incoming);
			//ret.push_back(sdi);
			ret.push_back(*j);
		}
	}
	return ret;
}

StringSet SharePage::getExcludedDirs() {
	StringList excludes;
	StringSet ret;
	ShareManager::getInstance()->getExcludes(curProfile, excludes);
	for (auto j = excludes.begin(); j != excludes.end(); j++) {
		if (excludedRemove[curProfile].find(*j) == excludedRemove[curProfile].end()) {
			ret.insert(*j);
		}
	}

	if (excludedAdd.find(curProfile) != excludedAdd.end())
		ret.insert(excludedAdd[curProfile].begin(), excludedAdd[curProfile].end());
	return ret;
}

void SharePage::showProfile() {
	if (ft) {
		ft->Clear();
		ft->UnsubclassWindow(true);
		ft->DestroyWindow();
		delete ft;
		ft = nullptr;
	} else {
		ctrlDirectories.DeleteAllItems();
	}

	GetDlgItem(IDC_TREE1).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_DIRECTORIES).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_ADD_DIR).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_REMOVE_DIR).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_RENAME_DIR).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);

	if(BOOLSETTING(USE_OLD_SHARING_UI)) {
		// Prepare shared dir list
		auto shares = getViewItems();
		for(auto j = shares.begin(); j != shares.end(); j++) {
			int i = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), Text::toT(j->vname));
			ctrlDirectories.SetItemText(i, 1, Text::toT(j->path).c_str());
			ctrlDirectories.SetItemText(i, 2, Util::formatBytesW(ShareManager::getInstance()->getShareSize(j->path, curProfile)).c_str());
			ctrlDirectories.SetCheckState(i, j->incoming);
		}
		ctrlDirectories.setSort(0, ExListViewCtrl::SORT_STRING);
	} else {
		ft = new FolderTree(this);
		ft->SubclassWindow(GetDlgItem(IDC_TREE1));
		ft->PopulateTree();
	}
}

LRESULT SharePage::onClickedShowTree(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto useOldInterface = IsDlgButtonChecked(IDC_SHOW_TREE) != BST_CHECKED;
	SettingsManager::getInstance()->set(SettingsManager::USE_OLD_SHARING_UI, useOldInterface);
	if(useOldInterface) {
		// Prepare shared dir list
		ctrlDirectories.InsertColumn(0, CTSTRING(VIRTUAL_NAME), LVCFMT_LEFT, 80, 0);
		ctrlDirectories.InsertColumn(1, CTSTRING(DIRECTORY), LVCFMT_LEFT, 197, 1);
		ctrlDirectories.InsertColumn(2, CTSTRING(SIZE), LVCFMT_RIGHT, 90, 2);
	} else {
		ctrlDirectories.DeleteColumn(0);
		ctrlDirectories.DeleteColumn(0);
		ctrlDirectories.DeleteColumn(0);
	}

	showProfile();
	return 0;
}

LRESULT SharePage::onEditTempShares(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TempShareDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT SharePage::onColumnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	
	NMLISTVIEW* l = (NMLISTVIEW*)pnmh;
	
	if(l->iSubItem == ctrlDirectories.getSortColumn()) {
		if(!ctrlDirectories.isAscending()) { //change the direction of sorting by clicking on a already sorted column
			ctrlDirectories.setSort(-1, ctrlDirectories.getSortType());
		} else {
			ctrlDirectories.setSortDirection(false);
		}
	} else {
		if(l->iSubItem == 0 || l->iSubItem == 1) //realpath or vname
			ctrlDirectories.setSort(l->iSubItem, ExListViewCtrl::SORT_STRING);
		else if(l->iSubItem == 2) //size
			ctrlDirectories.setSort(l->iSubItem, ExListViewCtrl::SORT_BYTES);
	}
	return 0;
}

LRESULT SharePage::onDropFiles(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/){
	HDROP drop = (HDROP)wParam;
	TCHAR buf[MAX_PATH];
	UINT nrFiles;
	
	nrFiles = DragQueryFile(drop, (UINT)-1, NULL, 0);
	
	for(UINT i = 0; i < nrFiles; ++i){
		if(DragQueryFile(drop, i, buf, MAX_PATH)){
			if(PathIsDirectory(buf))
				addDirectory(buf);
		}
	}

	DragFinish(drop);

	return 0;
}

void SharePage::write() {
	PropPage::write((HWND)*this, items);
	applyChanges(true);
	//boost::for_each(ftl, [](pair<string, FolderTree*> ftp) { delete ftp.second;  });
}

LRESULT SharePage::onItemchangedDirectories(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	NM_LISTVIEW* lv = (NM_LISTVIEW*) pnmh;
	::EnableWindow(GetDlgItem(IDC_REMOVE_DIR), (lv->uNewState & LVIS_FOCUSED));
	::EnableWindow(GetDlgItem(IDC_RENAME_DIR), (lv->uNewState & LVIS_FOCUSED));

	NMITEMACTIVATE* itemActivate = (NMITEMACTIVATE*)pnmh;
	if((itemActivate->uChanged & LVIF_STATE) == 0)
		return 0;
	if((itemActivate->uOldState & INDEXTOSTATEIMAGEMASK(0xf)) == 0)
		return 0;
	if((itemActivate->uNewState & INDEXTOSTATEIMAGEMASK(0xf)) == 0)
		return 0;

	/* The checkbox state has changed */
	if(itemActivate->iItem >= 0) {
		TCHAR buf[MAX_PATH];
		LVITEM item;
		memzero(&item, sizeof(item));
		item.mask = LVIF_TEXT;
		item.cchTextMax = sizeof(buf);
		item.pszText = buf;
		item.iItem = itemActivate->iItem;

		item.iSubItem = 1;
		ctrlDirectories.GetItem(&item);
		string rPath = Text::fromT(buf);
		bool checked = ctrlDirectories.GetCheckState(item.iItem) > 0;

		auto dirItems = getItemsByPath(rPath);
		boost::for_each(dirItems, [checked](ShareDirInfo* aDir) { aDir->incoming = checked; });

		auto p = boost::find_if(dirItems, [rPath, this](ShareDirInfo* aDir) { return aDir->path == rPath && aDir->profile == curProfile; });
		dcassert(p != dirItems.end());

		ShareDirInfo& dir = *(*p);

		auto s = find(newDirs.begin(), newDirs.end(), dir);
		if (s == newDirs.end()) {
			auto s = find(changedDirs.begin(), changedDirs.end(), dir);
			if (s == changedDirs.end()) {
				dcassert(checked);
				changedDirs.push_back(dir);
			} else if (!checked) {
				changedDirs.erase(s);
			}
		}
	}

	return 0;		
}

vector<ShareDirInfo*> SharePage::getItemsByPath(const string& aPath) {
	vector<ShareDirInfo*> dirItems;
	for(auto i = shareDirs.begin(); i != shareDirs.end(); i++) {
		auto& dirs = i->second;
		for(auto i = dirs.begin(); i != dirs.end(); i++) {
			if (i->path == aPath && find(removeDirs.begin(), removeDirs.end(), *i) == removeDirs.end()) {
				dirItems.push_back(&(*i));
			}
		}
	}

	for(auto j = newDirs.begin(); j != newDirs.end(); j++) {
		if (j->path == aPath)
			dirItems.push_back(&(*j));
	}
	return dirItems;
}

SharePage::~SharePage() {
	if (ft) {
		ft->UnsubclassWindow(true);
		delete ft;
	}
	ctrlDirectories.Detach();
}

LRESULT SharePage::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	switch(kd->wVKey) {
	case VK_INSERT:
		PostMessage(WM_COMMAND, IDC_ADD_DIR, 0);
		break;
	case VK_DELETE:
		PostMessage(WM_COMMAND, IDC_REMOVE_DIR, 0);
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}

LRESULT SharePage::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

	if(item->iItem >= 0) {
		PostMessage(WM_COMMAND, IDC_RENAME_DIR, 0);
	} else if(item->iItem == -1) {
		PostMessage(WM_COMMAND, IDC_ADD_DIR, 0);
	}

	return 0;
}

LRESULT SharePage::onClickedAddProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto newProfile = addProfile();
	if (newProfile) {
		curProfile = newProfile->getToken();
		showProfile();
		fixControls();
	}
	return 0;
}

ShareProfilePtr SharePage::addProfile() {
	LineDlg virt;
	virt.title = TSTRING(NAME);
	virt.description = TSTRING(PROFILE_NAME_DESC);
	if(virt.DoModal(m_hWnd) == IDOK) {
		string name = Text::fromT(virt.line);
		auto spNew = ShareProfilePtr(new ShareProfile(name));
		ctrlProfile.AddString(Text::toT(name).c_str());
		ctrlProfile.SetCurSel(ctrlProfile.GetCount()-1);
		profiles.push_back(spNew);
		addProfiles.insert(spNew);
		return spNew;
	}
	return nullptr;
}

LRESULT SharePage::onClickedCopyProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto newProfile = addProfile();
	if (!newProfile)
		return 0;

	/* Add all items from the current profile */
	auto dirs = getViewItems();
	for(auto j = dirs.begin(); j != dirs.end(); j++) {
		auto sdi = ShareDirInfo(j->vname, newProfile->getToken(), j->path);
		newDirs.push_back(sdi);
	}

	curProfile = newProfile->getToken();
	showProfile();
	fixControls();
	return 0;
}

LRESULT SharePage::onClickedRemoveProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto p = getSelectedProfile();
	//can't remove the default profile
	if (p->getToken() == SP_DEFAULT) {
		return 0;
	}

	/* Remove the profile */
	auto n = find(addProfiles.begin(), addProfiles.end(), p);
	if (n != addProfiles.end()) {
		addProfiles.erase(n);
	} else {
		removeProfiles.push_back(p->getToken());
	}
	renameProfiles.erase(remove_if(renameProfiles.begin(), renameProfiles.end(), CompareFirst<ShareProfilePtr, string>(p)), renameProfiles.end());

	/* Undo all current directory modifications to this profile */
	auto hasProfile = [p](const ShareDirInfo& sdi) -> bool {
		return sdi.profile == p->getToken();
	};

	removeDirs.erase(boost::remove_if(removeDirs, hasProfile), removeDirs.end());
	newDirs.erase(boost::remove_if(newDirs, hasProfile), newDirs.end());
	changedDirs.erase(boost::remove_if(changedDirs, hasProfile), changedDirs.end());

	excludedAdd.erase(p->getToken());
	excludedRemove.erase(p->getToken());


	/* Add all dirs in this profile to the list of removed dirs */
	auto& profileDirs = shareDirs[curProfile];
	removeDirs.insert(removeDirs.end(), profileDirs.begin(), profileDirs.end());

	/* List excludes */
	StringList excluded;
	ShareManager::getInstance()->getExcludes(p->getToken(), excluded);
	boost::for_each(excluded, [&](const string& aString) { excludedRemove[p->getToken()].insert(aString); });


	/* Switch to default profile */
	ctrlProfile.DeleteString(ctrlProfile.GetCurSel());
	curProfile = SP_DEFAULT;
	showProfile();
	ctrlProfile.SetCurSel(0);
	fixControls();
	return 0;
}

LRESULT SharePage::onClickedRenameProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto profile = profiles[ctrlProfile.GetCurSel()];
	auto name = Text::toT(profile->getPlainName());
	auto p = find_if(renameProfiles.begin(), renameProfiles.end(), CompareFirst<ShareProfilePtr, string>(profile));
	if (p != renameProfiles.end()) {
		name = Text::toT(p->second);
	}

	LineDlg virt;
	virt.title = TSTRING(VIRTUAL_NAME);
	virt.description = TSTRING(VIRTUAL_NAME_LONG);
	virt.line = name;
	if(virt.DoModal(m_hWnd) != IDOK || virt.line.empty())
		return 0;
	if (name == virt.line)
		return 0;

	// Update the list
	auto pos = ctrlProfile.GetCurSel();
	ctrlProfile.DeleteString(pos);
	tstring displayName = virt.line + (pos == 0 ? (_T(" (") + TSTRING(DEFAULT) + _T(")")) : Util::emptyStringT);
	ctrlProfile.InsertString(pos, displayName.c_str());
	ctrlProfile.SetCurSel(pos);

	if (p != renameProfiles.end()) {
		p->second = Text::fromT(virt.line);
		return 0;
	}

	renameProfiles.push_back(make_pair(profile, Text::fromT(virt.line)));

	fixControls();
	return 0;
}

LRESULT SharePage::onClickedAddDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	tstring target;
	if(WinUtil::browseDirectory(target, m_hWnd)) {
		addDirectory(target);
	}
	
	return 0;
}

LRESULT SharePage::onClickedRemoveDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR buf[MAX_PATH];
	int i;
	while((i = ctrlDirectories.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		ctrlDirectories.GetItemText(i, 1, buf, MAX_PATH);
		removeDir(Text::fromT(buf), curProfile);
		ctrlDirectories.DeleteItem(i);
	}
	
	return 0;
}

void SharePage::removeDir(const string& rPath, const string& aProfile, bool checkDupes /*true*/) {
	auto shareDir = ShareDirInfo(Util::emptyString, aProfile, rPath);
	auto p = find(newDirs.begin(), newDirs.end(), shareDir);
	if (p != newDirs.end()) {
		newDirs.erase(p);
		return;
	}

	removeDirs.push_back(shareDir);
	if (checkDupes) {
		auto dirItems = getItemsByPath(rPath);
		if (!dirItems.empty() && MessageBox(CTSTRING_F(X_PROFILE_DIRS_EXISTS, (int)dirItems.size()), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
			boost::for_each(dirItems, [this](ShareDirInfo* aDir) { removeDir(aDir->path, aDir->profile, false); });
		}
		fixControls();
	}
}

LRESULT SharePage::onClickedRenameDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR buf[MAX_PATH];
	LVITEM item;
	memzero(&item, sizeof(item));
	item.mask = LVIF_TEXT;
	item.cchTextMax = sizeof(buf);
	item.pszText = buf;

	int i = -1;
	while((i = ctrlDirectories.GetNextItem(i, LVNI_SELECTED)) != -1) {
		item.iItem = i;
		item.iSubItem = 0;
		ctrlDirectories.GetItem(&item);
		tstring vName = buf;
		item.iSubItem = 1;
		ctrlDirectories.GetItem(&item);
		string rPath = Text::fromT(buf);
		LineDlg virt;
		virt.title = TSTRING(VIRTUAL_NAME);
		virt.description = TSTRING(VIRTUAL_NAME_LONG);
		virt.line = vName;
		if(virt.DoModal(m_hWnd) == IDOK) {
			if (stricmp(buf, virt.line) != 0) {
				ctrlDirectories.SetItemText(i, 0, virt.line.c_str());

				/* Is this a newly added dir? */
				auto p = boost::find_if(newDirs, [rPath, this](const ShareDirInfo& sdi) { return sdi.path == rPath && sdi.profile == curProfile; });
				if (p != newDirs.end()) {
					p->vname = Text::fromT(virt.line);
					return 0;
				}
					
				/* Existing shared dir? */
				auto& dirs = shareDirs[curProfile];
				auto k = boost::find_if(dirs, [rPath, this](const ShareDirInfo& sdi) { return sdi.path == rPath && sdi.profile == curProfile; });
				if (k == dirs.end()) {
					dcassert(0); //this should never happen
					return 0;
				}

				auto& shareDir = *k;
				shareDir.vname = Text::fromT(virt.line);
				auto s = find(changedDirs.begin(), changedDirs.end(), shareDir);
				if (s == changedDirs.end()) {
					changedDirs.push_back(shareDir);
				} else {
					s->vname = Text::fromT(virt.line);
				}
			} else {
				MessageBox(CTSTRING(SKIP_RENAME), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONINFORMATION | MB_OK);
			}
		}
	}

	fixControls();
	return 0;
}

bool SharePage::addDirectory(const tstring& aPath){
	tstring path = aPath;
	if(path[path.length()-1] != PATH_SEPARATOR)
		path += PATH_SEPARATOR;

	/* Check if we are trying to share a directory which exists already in this profile */
	auto dirs = getViewItems();
	for(auto j = dirs.begin(); j != dirs.end(); j++) {
		if (AirUtil::isParentOrExact(j->path, Text::fromT(aPath))) {
			MessageBox(CTSTRING(DIRECTORY_SHARED), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONINFORMATION | MB_OK);
			return false;
		} else if (AirUtil::isSub(j->path, Text::fromT(aPath))) {
			if (ft) {
				//Remove the subdir but only from this profile
				removeDir(j->path, j->profile, false);
				//break;
			} else {
				//Maybe we should find and remove the correct item from the list instead...
				MessageBox(CTSTRING(DIRECTORY_PARENT), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONINFORMATION | MB_OK);
				return false;
			}
		}
	}

	LineDlg virt;
	virt.title = TSTRING(VIRTUAL_NAME);
	virt.description = TSTRING(VIRTUAL_NAME_LONG);
	virt.line = Text::toT(ShareManager::getInstance()->validateVirtual(Util::getLastDir(Text::fromT(path))));
	if(virt.DoModal(m_hWnd) == IDOK) {
		string vPath = Text::fromT(virt.line), rPath = Text::fromT(path);
		ShareManager::getInstance()->validatePath(rPath, vPath);
		auto dir = ShareDirInfo(vPath, curProfile, rPath);
		if (find(newDirs.begin(), newDirs.end(), dir) != newDirs.end())
			return false;
		newDirs.push_back(dir);
		if(BOOLSETTING(USE_OLD_SHARING_UI)) {
			int i = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), virt.line );
			ctrlDirectories.SetItemText(i, 1, path.c_str());
			ctrlDirectories.SetItemText(i, 2, _T("New"));
		}
	}

	fixControls();
	ctrlDirectories.resort();
	return true;
}

LRESULT SharePage::onApplyChanges(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	applyChanges(false);
	return 0;
}

void SharePage::fixControls() {
	bool hasChanged = (!addProfiles.empty() || !removeProfiles.empty() || !renameProfiles.empty() || !newDirs.empty() || !removeDirs.empty() || 
		changedDirs.empty() || !excludedAdd.empty() || !excludedRemove.empty());
	::EnableWindow(GetDlgItem(IDC_APPLY_CHANGES), hasChanged);
	::EnableWindow(GetDlgItem(IDC_REMOVE_PROFILE), curProfile != SP_DEFAULT);
}

void SharePage::applyChanges(bool isQuit) {
	if (!addProfiles.empty()) {
		ShareManager::getInstance()->addProfiles(addProfiles);
		addProfiles.clear();
	}

	if (!removeProfiles.empty()) {
		int favReset = 0;
		ClientManager::getInstance()->resetProfiles(removeProfiles, *profiles.begin()); //reset for currently connected hubs
		favReset += FavoriteManager::getInstance()->resetProfiles(removeProfiles, *profiles.begin()); //reset for favorite hubs
		ShareManager::getInstance()->removeProfiles(removeProfiles); //remove from profiles
		if (favReset > 0)
			MessageBox(CWSTRING_F(X_FAV_PROFILES_RESET, favReset), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONINFORMATION | MB_OK);

		removeProfiles.clear();
	}

	if (!renameProfiles.empty()) {
		for(auto j = renameProfiles.begin(); j != renameProfiles.end(); j++) {
			j->first->setPlainName(j->second);
		}
		FavoriteManager::getInstance()->onProfilesRenamed();
		renameProfiles.clear();
	}


	if (!newDirs.empty()) {
		ShareManager::getInstance()->addDirectories(newDirs);
		newDirs.clear();
	}

	if (!removeDirs.empty()) {
		ShareManager::getInstance()->removeDirectories(removeDirs);
		removeDirs.clear();
	}

	if (!changedDirs.empty()) {
		ShareManager::getInstance()->changeDirectories(changedDirs);
		changedDirs.clear();
	}

	if (!excludedRemove.empty() || !excludedAdd.empty()) {
		//ShareManager::getInstance()->addExcludedDirs(excludedAdd);
		ShareManager::getInstance()->changeExcludedDirs(excludedAdd, excludedRemove);
		excludedAdd.clear();
		excludedRemove.clear();
	}

	if (!isQuit) {
		shareDirs.clear();
		ShareManager::getInstance()->getShares(shareDirs);
		::EnableWindow(GetDlgItem(IDC_APPLY_CHANGES), false);
	}
}

bool SharePage::addExcludeFolder(const string &path) {
	auto excludes = getExcludedDirs();
	auto shares = getViewItems();
	
	// make sure this is a sub folder of a shared folder
	if (boost::find_if(shares, [path](const ShareDirInfo& aDir) { return AirUtil::isSub(path, aDir.path); } ) == shares.end())
		return false;

	// Make sure this not a subfolder of an already excluded folder
	if (boost::find_if(excludes, [path](const string& aDir) { return AirUtil::isParentOrExact(aDir, path); } ) != excludes.end())
		return false;

	// remove all sub folder excludes
	for(auto j = excludes.begin(); j != excludes.end(); ++j) {
		if (AirUtil::isSub(*j, path))
			removeExcludeFolder(*j);
	}

	// add it to the list
	excludedAdd[curProfile].insert(path);
	fixControls();
	return true;
}

bool SharePage::removeExcludeFolder(const string& path) {
	auto add = excludedAdd[curProfile].find(path);
	if (add != excludedAdd[curProfile].end()) {
		auto p = excludedAdd[curProfile];
		p.erase(path);
		if (p.empty())
			excludedAdd.erase(curProfile);
		return true;
	}

	excludedRemove[curProfile].insert(path);
	fixControls();
	return true;
}

bool SharePage::shareFolder(const string& path, ShareDirInfo::list& aShared) {
	auto excludes = getExcludedDirs();

	// check if it's part of the share before checking if it's in the exclusions
	bool result = false;
	ShareDirInfo::list::iterator i;
	for(i = aShared.begin(); i != aShared.end(); ++i) {
		if (i->profile != curProfile)
			continue;

		if((path.size() == i->path.size()) && (stricmp(path, i->path) == 0)) {
			// is it a perfect match
			i->found = true;
			return true;
		} else if (path.size() > i->path.size()) // this might be a subfolder of a shared folder
		{
			// if the left-hand side matches and there is a \ in the remainder then it is a subfolder
			if((stricmp(path.substr(0, i->path.size()), i->path) == 0) && (path.find('\\', i->path.size()) != string::npos)) {
				result = true;
				break;
			}
		}
	}

	if(!result)
		return false;

	// check if it's an excluded folder or a sub folder of an excluded folder
	if (boost::find_if(excludes, [path](const string& aDir) { return AirUtil::isParentOrExact(aDir, path); } ) != excludes.end())
		return false;

	i->found = true;
	return true;
}