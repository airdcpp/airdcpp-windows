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

ShareDirInfo::list SharePage::getViewItems(const string& aProfile, bool getDiffItems /*false*/) {
	ShareDirInfo::list ret;

	auto& dirs = shareDirs[aProfile];
	for(auto i = dirs.begin(); i != dirs.end(); i++) {
		if (find(removeDirs.begin(), removeDirs.end(), *i) == removeDirs.end()) {
			ret.push_back(*i);
		}
	}

	for(auto j = newDirs.begin(); j != newDirs.end(); j++) {
		if (compare((*j)->profile, aProfile) == 0) {
			ret.push_back(*j);
		}
	}

	if (getDiffItems && aProfile != SP_DEFAULT) {
		// show the diff
		auto defaultItems = getViewItems(SP_DEFAULT);
		for(auto j = defaultItems.begin(); j != defaultItems.end(); j++) {
			if (boost::find_if(ret, [j](const ShareDirInfo* aDir) { return aDir->path == (*j)->path; }) == ret.end()) {
				auto sdi = new ShareDirInfo((*j)->vname, aProfile, (*j)->path, (*j)->incoming);
				sdi->size = (*j)->size;
				sdi->state = ShareDirInfo::REMOVED;
				tempViewItems.push_back(sdi);
				ret.push_back(sdi);
			}
		}

		for(auto j = ret.begin(); j != ret.end(); j++) {
			if (boost::find_if(defaultItems, [j](const ShareDirInfo* aDir) { return aDir->path == (*j)->path; }) == defaultItems.end()) {
				(*j)->state = ShareDirInfo::ADDED;
			}
		}
	}

	//sort(ret.begin(), ret.end(), ShareDirInfo::Sort());
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

/*static int vNameSort(LPARAM lParam1, LPARAM lParam2) {
	auto left = (ShareDirInfo*)lParam1;
	auto right = (ShareDirInfo*)lParam2;
	if (left->state == ShareDirInfo::REMOVED && right->state != ShareDirInfo::REMOVED) return 1;
	if (left->state != ShareDirInfo::REMOVED && right->state == ShareDirInfo::REMOVED) return -1;

	return compare(left->vname, right->vname);
}

static int sizeSort(LPARAM lParam1, LPARAM lParam2) {
	auto left = (ShareDirInfo*)lParam1;
	auto right = (ShareDirInfo*)lParam2;
	if (left->state == ShareDirInfo::REMOVED && right->state != ShareDirInfo::REMOVED) return 1;
	if (left->state != ShareDirInfo::REMOVED && right->state == ShareDirInfo::REMOVED) return -1;

	return compare(right->size, left->size);
}

static int pathSort(LPARAM lParam1, LPARAM lParam2) {
	auto left = (ShareDirInfo*)lParam1;
	auto right = (ShareDirInfo*)lParam2;
	if (left->state == ShareDirInfo::REMOVED && right->state != ShareDirInfo::REMOVED) return 1;
	if (left->state != ShareDirInfo::REMOVED && right->state == ShareDirInfo::REMOVED) return -1;

	return compare(left->path, right->path);
}*/

static int sort(LPARAM lParam1, LPARAM lParam2, int column) {
	auto left = (ShareDirInfo*)lParam1;
	auto right = (ShareDirInfo*)lParam2;

	//removed always go last
	if (left->state == ShareDirInfo::REMOVED && right->state != ShareDirInfo::REMOVED) return 1;
	if (left->state != ShareDirInfo::REMOVED && right->state == ShareDirInfo::REMOVED) return -1;

	//added are first
	if (left->state != ShareDirInfo::ADDED && right->state == ShareDirInfo::ADDED) return 1;
	if (left->state == ShareDirInfo::ADDED && right->state != ShareDirInfo::ADDED) return -1;
	
	if (column == 0)
		return compare(left->vname, right->vname);
	else if (column == 1)
		return compare(left->path, right->path);
	else
		return compare(right->size, left->size);
}

void SharePage::showProfile() {
	deleteTempViewItems();
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
		auto shares = getViewItems(curProfile, true);
		for(auto j = shares.begin(); j != shares.end(); j++) {
			int i = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), Text::toT((*j)->vname), 0, (LPARAM)*j);
			ctrlDirectories.SetItemText(i, 1, Text::toT((*j)->path).c_str());
			ctrlDirectories.SetItemText(i, 2, Util::formatBytesW((*j)->size).c_str());
			ctrlDirectories.SetCheckState(i, (*j)->incoming);
			/*if ((*j)->state == ShareDirInfo::REMOVED) {
				ctrlDirectories.SetItemState(i, INDEXTOSTATEIMAGEMASK(3), LVIS_STATEIMAGEMASK);
			}*/
		}
		ctrlDirectories.setSort(0, ExListViewCtrl::SORT_FUNC, true, sort);
	} else {
		ft = new FolderTree(this);
		ft->SubclassWindow(GetDlgItem(IDC_TREE1));
		ft->PopulateTree();
	}
}

LRESULT SharePage::onCustomDrawList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;
	switch(cd->nmcd.dwDrawStage) {

	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT: {
		ShareDirInfo* ii = reinterpret_cast<ShareDirInfo*>(cd->nmcd.lItemlParam);
		if (ii->state == ShareDirInfo::REMOVED) {
			cd->clrText = RGB(225, 225, 225);
		} else if (ii->state == ShareDirInfo::ADDED) {
			cd->clrText = RGB(0, 180, 13);
		}
		return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
	}

	default:
		return CDRF_DODEFAULT;
	}

	return CDRF_DODEFAULT;
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
		ctrlDirectories.setSort(l->iSubItem, ExListViewCtrl::SORT_FUNC, true, sort);
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
		auto sdi = (ShareDirInfo*)ctrlDirectories.GetItemData(itemActivate->iItem);
		bool boxChecked = ctrlDirectories.GetCheckState(itemActivate->iItem) > 0;

		if (sdi->incoming == boxChecked)
			return 0; //we are probably loading an initial listing...

		auto dirItems = getItemsByPath(sdi->path);
		boost::for_each(dirItems, [boxChecked](ShareDirInfo* aDir) { aDir->incoming = boxChecked; });

		auto s = find(newDirs.begin(), newDirs.end(), sdi);
		if (s == newDirs.end()) {
			auto s = find(changedDirs.begin(), changedDirs.end(), sdi);
			if (s == changedDirs.end()) {
				changedDirs.push_back(sdi);
			}
		}
	}

	return 0;		
}

ShareDirInfo::list SharePage::getItemsByPath(const string& aPath) {
	ShareDirInfo::list dirItems;
	for(auto i = shareDirs.begin(); i != shareDirs.end(); i++) {
		auto& dirs = i->second;
		for(auto i = dirs.begin(); i != dirs.end(); i++) {
			if ((*i)->path == aPath && find(removeDirs.begin(), removeDirs.end(), *i) == removeDirs.end()) {
				dirItems.push_back(*i);
			}
		}
	}

	for(auto j = newDirs.begin(); j != newDirs.end(); j++) {
		if ((*j)->path == aPath)
			dirItems.push_back(*j);
	}
	return dirItems;
}

SharePage::~SharePage() {
	if (ft) {
		ft->UnsubclassWindow(true);
		delete ft;
	}
	ctrlDirectories.DeleteAllItems();
	ctrlDirectories.Detach();
	deleteDirectoryInfoItems();
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
	auto dirs = getViewItems(curProfile);
	for(auto j = dirs.begin(); j != dirs.end(); j++) {
		auto sdi = new ShareDirInfo((*j)->vname, newProfile->getToken(), (*j)->path);
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
	auto hasProfile = [p](const ShareDirInfo* sdi) -> bool {
		return sdi->profile == p->getToken();
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
	virt.title = TSTRING(PROFILE_NAME);
	virt.description = TSTRING(PROFILE_NAME_DESC);
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
	int i = -1;
	bool added = false;
	while((i = ctrlDirectories.GetNextItem(i, LVNI_SELECTED)) != -1) {
		auto sdi = (ShareDirInfo*)ctrlDirectories.GetItemData(i);
		if (sdi->state == ShareDirInfo::REMOVED) {
			sdi->state = ShareDirInfo::NORMAL;
			newDirs.push_back(sdi);
			tempViewItems.erase(remove(tempViewItems.begin(), tempViewItems.end(), sdi), tempViewItems.end());
			added = true;
		}
	}

	if (!added) {
		tstring target;
		if(WinUtil::browseDirectory(target, m_hWnd)) {
			addDirectory(target);
		}
	} else if (BOOLSETTING(USE_OLD_SHARING_UI)) {
		//update the list to fix the colors and sorting
		RedrawWindow();
		ctrlDirectories.resort();
	}
	
	return 0;
}

LRESULT SharePage::onClickedRemoveDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int i = -1;
	bool redraw = false;
	while((i = ctrlDirectories.GetNextItem(i, LVNI_SELECTED)) != -1) {
		auto sdi = (ShareDirInfo*)ctrlDirectories.GetItemData(i);
		if (sdi->state != ShareDirInfo::REMOVED) {
			removeDir(sdi->path, curProfile);
			if (sdi->state == ShareDirInfo::NORMAL && curProfile != SP_DEFAULT) {
				sdi->state = ShareDirInfo::REMOVED;
				redraw = true;
			} else {
				ctrlDirectories.DeleteItem(i);
			}
		}
	}

	if (redraw) {
		//update the list to fix the colors and sorting
		RedrawWindow();
		ctrlDirectories.resort();
	}
	
	return 0;
}

void SharePage::removeDir(const string& rPath, const string& aProfile, bool checkDupes /*true*/) {
	//update the diff info in case this path exists in other profiles
	if (curProfile == SP_DEFAULT) {
		auto items = getItemsByPath(rPath);
		boost::for_each(items, [](ShareDirInfo* sdi) { sdi->state = ShareDirInfo::REMOVED; });
	}

	auto p = boost::find_if(newDirs, [rPath, aProfile](const ShareDirInfo* sdi) { return sdi->path == rPath && sdi->profile == aProfile; });
	if (p != newDirs.end()) {
		newDirs.erase(p);
		return;
	}

	auto& dirs = shareDirs[curProfile];
	auto k = boost::find_if(dirs, [rPath, aProfile](const ShareDirInfo* sdi) { return sdi->path == rPath && sdi->profile == aProfile; });
	if (k != dirs.end())
		removeDirs.push_back(*k);

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
	int i = -1;
	while((i = ctrlDirectories.GetNextItem(i, LVNI_SELECTED)) != -1) {
		auto sdi = (ShareDirInfo*)ctrlDirectories.GetItemData(i);
		if (sdi->state == ShareDirInfo::REMOVED) {
			continue;
		}

		tstring vName = Text::toT(sdi->vname);
		LineDlg virt;
		virt.title = TSTRING(VIRTUAL_NAME);
		virt.description = TSTRING(VIRTUAL_NAME_LONG);
		virt.line = vName;
		if(virt.DoModal(m_hWnd) == IDOK) {
			if (stricmp(vName, virt.line) != 0) {
				ctrlDirectories.SetItemText(i, 0, virt.line.c_str());
				sdi->vname = Text::fromT(virt.line);

				/* Is this a newly added dir? */
				auto p = find(newDirs.begin(), newDirs.end(), sdi);
				if (p != newDirs.end() && find(changedDirs.begin(), changedDirs.end(), sdi) == changedDirs.end()) {
					changedDirs.push_back(sdi);
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
	auto dirs = getViewItems(curProfile);
	for(auto j = dirs.begin(); j != dirs.end(); j++) {
		if (AirUtil::isParentOrExact((*j)->path, Text::fromT(aPath))) {
			MessageBox(CTSTRING(DIRECTORY_SHARED), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONINFORMATION | MB_OK);
			return false;
		} else if (AirUtil::isSub((*j)->path, Text::fromT(aPath))) {
			if (ft) {
				//Remove the subdir but only from this profile
				removeDir((*j)->path, (*j)->profile, false);
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
		auto dir = new ShareDirInfo(vPath, curProfile, rPath);
		if (find(newDirs.begin(), newDirs.end(), dir) != newDirs.end()) {
			delete dir;
			return false;
		}

		auto items = getItemsByPath(rPath);
		if (curProfile == SP_DEFAULT) {
			//update the diff info in case this path exists in other profiles
			boost::for_each(items, [](ShareDirInfo* sdi) { sdi->state = ShareDirInfo::NORMAL; });
		} else {
			//check if this exists in the default profile
			auto pos = boost::find_if(items, [](const ShareDirInfo* sdi) { return sdi->profile == SP_DEFAULT; });
			if (pos == items.end())
				dir->state = ShareDirInfo::ADDED;
		}

		newDirs.push_back(dir);
		if(BOOLSETTING(USE_OLD_SHARING_UI)) {
			int i = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), virt.line, 0, (LPARAM)dir);
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
		//will be cleared later...
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
		deleteDirectoryInfoItems();
		ShareManager::getInstance()->getShares(shareDirs);
		showProfile();
		::EnableWindow(GetDlgItem(IDC_APPLY_CHANGES), false);
	}
}

void SharePage::deleteDirectoryInfoItems() {
	deleteTempViewItems();
	for(auto j = shareDirs.begin(); j != shareDirs.end(); j++) {
		boost::for_each(j->second, DeleteFunction());
		j->second.clear();
	}
	shareDirs.clear();

	boost::for_each(newDirs, DeleteFunction());
	newDirs.clear();
}

void SharePage::deleteTempViewItems() {
	boost::for_each(tempViewItems, DeleteFunction());
	tempViewItems.clear();
}

bool SharePage::addExcludeFolder(const string &path) {
	auto excludes = getExcludedDirs();
	auto shares = getViewItems(curProfile);
	
	// make sure this is a sub folder of a shared folder
	if (boost::find_if(shares, [path](const ShareDirInfo* aDir) { return AirUtil::isSub(path, aDir->path); } ) == shares.end())
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
		if ((*i)->profile != curProfile)
			continue;

		if((path.size() == (*i)->path.size()) && (stricmp(path, (*i)->path) == 0)) {
			// is it a perfect match
			(*i)->found = true;
			return true;
		} else if (path.size() > (*i)->path.size()) // this might be a subfolder of a shared folder
		{
			// if the left-hand side matches and there is a \ in the remainder then it is a subfolder
			if((stricmp(path.substr(0, (*i)->path.size()), (*i)->path) == 0) && (path.find('\\', (*i)->path.size()) != string::npos)) {
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

	(*i)->found = true;
	return true;
}