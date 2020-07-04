/* 
 * Copyright (C) 2001-2019 Jacek Sieka, arnetheduck on gmail point com
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

#include <airdcpp/Util.h>
#include <airdcpp/ClientManager.h>
#include <airdcpp/FavoriteManager.h>

#include <web-server/ContextMenuManager.h>
#include <web-server/WebServerManager.h>

#include "BrowseDlg.h"
#include "Resource.h"
#include "ShareDirectories.h"
#include "WinUtil.h"
#include "LineDlg.h"
#include "PropertiesDlg.h"
#include "TempShare_dlg.h"
#include "SharePageDlg.h"
#include "MainFrm.h"
#include "ActionUtil.h"

#include <boost/range/adaptor/filtered.hpp>
using boost::adaptors::filtered;

#define curProfile parent->getCurProfile()
#define defaultProfile parent->getDefaultProfile()

using boost::find_if;

PropPage::TextItem ShareDirectories::texts[] = {
	{ IDC_SETTINGS_SHARED_DIRECTORIES, ResourceManager::SETTINGS_SHARED_DIRECTORIES },
	{ IDC_REMOVE_DIR, ResourceManager::REMOVE },
	{ IDC_ADD_DIR, ResourceManager::SETTINGS_ADD_FOLDER },
	{ IDC_RENAME_DIR, ResourceManager::RENAME },
	{ IDC_SHARE_PROFILE_NOTE, ResourceManager::SETTINGS_SHARE_PROFILE_NOTE },
	{ IDC_SHOW_TREE, ResourceManager::SHOW_DIRECTORY_TREE },
	{ 0, ResourceManager::LAST }
};

ProfileDirectoryInfo::ProfileDirectoryInfo(ShareDirectoryInfoPtr& aInfo, State aState) :
	found(false), state(aState), dir(aInfo) {


}

SharePageBase::SharePageBase() {
	profiles = ShareManager::getInstance()->getProfileInfos();
	dcassert(!profiles.empty());
}

ShareProfileInfoPtr SharePageBase::getProfile(ProfileToken aProfile) {
	auto p = find(profiles, aProfile);
	return p != profiles.end() ? *p : nullptr;
}

ShareDirectories::ShareDirectories(SharePageBase* aParent, SettingsManager *s) : parent(aParent), SettingTab(s), ft(nullptr) { }

LRESULT ShareDirectories::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	SettingTab::translate((HWND)(*this), texts);

	ctrlDirectories.Attach(GetDlgItem(IDC_DIRECTORIES));
	ctrlDirectories.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES);


	CheckDlgButton(IDC_SHOW_TREE, SETTING(USE_OLD_SHARING_UI) ? BST_UNCHECKED : BST_CHECKED);

	GetDlgItem(IDC_TREE1).ShowWindow((SETTING(USE_OLD_SHARING_UI)) ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_DIRECTORIES).ShowWindow((SETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_ADD_DIR).ShowWindow((SETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_REMOVE_DIR).ShowWindow((SETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_RENAME_DIR).ShowWindow((SETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);

	if(SETTING(USE_OLD_SHARING_UI)) {
		// Prepare shared dir list
		ctrlDirectories.InsertColumn(0, CTSTRING(VIRTUAL_NAME), LVCFMT_LEFT, 80, 0);
		ctrlDirectories.InsertColumn(1, CTSTRING(DIRECTORY), LVCFMT_LEFT, 197, 1);
		ctrlDirectories.InsertColumn(2, CTSTRING(SIZE), LVCFMT_RIGHT, 90, 2);
	}

	auto dirs = ShareManager::getInstance()->getRootInfos();
	for (auto& d : dirs) {
		shareDirs.emplace_back(make_shared<ProfileDirectoryInfo>(d));
	}

	excludedPaths = ShareManager::getInstance()->getExcludedPaths();

	showProfile();

	return TRUE;
}

LRESULT ShareDirectories::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if(reinterpret_cast<HWND>(wParam) == ctrlDirectories) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		
		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlDirectories, pt);
		}

		int selectedDirs = ctrlDirectories.GetSelectedCount();
		if (selectedDirs > 0) {
			int i = -1;

			OMenu menu;
			menu.CreatePopupMenu();

			{
				vector<TTHValue> tokens;
				while ((i = ctrlDirectories.GetNextItem(i, LVNI_SELECTED)) != -1) {
					auto sdi = (ProfileDirectoryInfo*)ctrlDirectories.GetItemData(i);
					tokens.push_back(AirUtil::getPathId(sdi->dir->path));
				}
				EXT_CONTEXT_MENU(menu, ShareRoot, tokens);
			}

			menu.AppendMenu(MF_STRING, IDC_REMOVE_DIR, CTSTRING(REMOVE));
			if (selectedDirs == 1) {
				auto path = Text::toT(((ProfileDirectoryInfo*)ctrlDirectories.GetItemData(ctrlDirectories.GetNextItem(i, LVNI_SELECTED)))->dir->path);
				menu.appendItem(TSTRING(OPEN_FOLDER), [path] { ActionUtil::openFolder(path); });
			}

			menu.open(m_hWnd, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt);
			return TRUE;
		}
	}
	bHandled = FALSE;
	return FALSE; 
}

static int sort(LPARAM lParam1, LPARAM lParam2, int column) {
	auto left = (ProfileDirectoryInfo*)lParam1;
	auto right = (ProfileDirectoryInfo*)lParam2;
	
	//don't return values over 1 so that ExListViewEx won't do its "tricks"
	if (column == 0) {
		return min(1, Util::DefaultSort(Text::toT(left->dir->virtualName).c_str(), Text::toT(right->dir->virtualName).c_str()));
	} else if (column == 1) {
		return min(1, Util::DefaultSort(Text::toT(left->dir->path).c_str(), Text::toT(right->dir->path).c_str()));
	} else {
		return compare(right->dir->size, left->dir->size);
	}
}

void ShareDirectories::showProfile() {
	if (ft.get()) {
		ft->Clear();
		ft->UnsubclassWindow(true);
		//ft->DestroyWindow();
		ft.reset();
	} else {
		ctrlDirectories.DeleteAllItems();
	}

	GetDlgItem(IDC_TREE1).ShowWindow((SETTING(USE_OLD_SHARING_UI)) ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_DIRECTORIES).ShowWindow((SETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_ADD_DIR).ShowWindow((SETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_REMOVE_DIR).ShowWindow((SETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_RENAME_DIR).ShowWindow((SETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);

	if(SETTING(USE_OLD_SHARING_UI)) {
		// Prepare shared dir list
		for (auto& sdi : shareDirs | filtered(ProfileDirectoryInfo::HasProfile(curProfile))) {
			if (sdi->state == ProfileDirectoryInfo::STATE_REMOVED)
				continue;

			int i = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), Text::toT(sdi->dir->virtualName), 0, (LPARAM)sdi.get());
			ctrlDirectories.SetItemText(i, 1, Text::toT(sdi->dir->path).c_str());
			ctrlDirectories.SetItemText(i, 2, sdi->state == ProfileDirectoryInfo::STATE_ADDED && sdi->dir->size == 0 ? CTSTRING(NEW) : Util::formatBytesW(sdi->dir->size).c_str());
			ctrlDirectories.SetCheckState(i, sdi->dir->incoming);
		}
		ctrlDirectories.setSort(0, ExListViewCtrl::SORT_FUNC, true, sort);
	} else {
		ft.reset(new FolderTree(this));
		ft->SubclassWindow(GetDlgItem(IDC_TREE1));
		ft->PopulateTree();
	}
}

ProfileDirectoryInfo::List ShareDirectories::getCurItems() {
	ProfileDirectoryInfo::List ret;
	copy_if(shareDirs.begin(), shareDirs.end(), back_inserter(ret), ProfileDirectoryInfo::HasProfile(curProfile));
	return ret;
}


LRESULT ShareDirectories::onCustomDrawList(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;
	switch(cd->nmcd.dwDrawStage) {

		case CDDS_PREPAINT:
			return CDRF_NOTIFYITEMDRAW;

		default:
			return CDRF_DODEFAULT;
	}
}

LRESULT ShareDirectories::onClickedShowTree(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
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

LRESULT ShareDirectories::onColumnClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	
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

LRESULT ShareDirectories::onDropFiles(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/){
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

LRESULT ShareDirectories::onItemchangedDirectories(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
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
		auto sdi = (ProfileDirectoryInfo*)ctrlDirectories.GetItemData(itemActivate->iItem);
		bool boxChecked = ctrlDirectories.GetCheckState(itemActivate->iItem) > 0;

		if (sdi->dir->incoming == boxChecked)
			return 0; //we are probably loading an initial listing...

		auto dirItem = getItemByPath(sdi->dir->path, true);
		if (dirItem)
			dirItem->dir->incoming = boxChecked;

		//if (sdi->state != ProfileDirectoryInfo::STATE_ADDED && sdi->isCurItem()) {
		//	sdi->state = ProfileDirectoryInfo::STATE_CHANGED;
		//}
	}

	return 0;		
}

ProfileDirectoryInfoPtr ShareDirectories::getItemByPath(const string& aPath, bool listRemoved) {
	ProfileDirectoryInfo::List dirItems;

	//add shared dirs
	auto p = find_if(shareDirs, ProfileDirectoryInfo::PathCompare(aPath));
	if (p != shareDirs.end() /*&& (*p)->diffState == ProfileDirectoryInfo::DIFF_NORMAL*/) {
		if (!listRemoved && (*p)->state == ProfileDirectoryInfo::STATE_REMOVED)
			return nullptr;

		return *p;
	}

	return nullptr;
}

ShareDirectories::~ShareDirectories() { }

LRESULT ShareDirectories::onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	//use the gui message for cleanup, these wont exist if the page hasnt been opened.
	if (ft.get()) {
		ft->UnsubclassWindow(true);
	}
	ctrlDirectories.DeleteAllItems();
	ctrlDirectories.Detach();
	return 0;
}

LRESULT ShareDirectories::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
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

LRESULT ShareDirectories::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

	if(item->iItem >= 0) {
		PostMessage(WM_COMMAND, IDC_RENAME_DIR, 0);
	} else if(item->iItem == -1) {
		PostMessage(WM_COMMAND, IDC_ADD_DIR, 0);
	}

	return 0;
}

void ShareDirectories::onCopyProfile(ProfileToken aNewProfile) {
	for (const auto& sourceDir : shareDirs | filtered(ProfileDirectoryInfo::HasProfile(curProfile))) {
		sourceDir->dir->profiles.insert(aNewProfile);
	}
}

void ShareDirectories::onRemoveProfile(ProfileToken aProfile) {
	/* Newly added dirs can be just deleted... */

	ProfileDirectoryInfo::List dirs;
	copy_if(shareDirs.begin(), shareDirs.end(), back_inserter(dirs), ProfileDirectoryInfo::HasProfile(curProfile));

	for (const auto& sourceDir : dirs) {
		auto confirm = CONFIRM_LEAVE;
		removeDir(sourceDir->dir->path, aProfile, confirm, false);
	}
}

LRESULT ShareDirectories::onClickedAddDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	//add selected merged items missing from this profile
	//int i = -1;
	bool added = false;
	/*while((i = ctrlDirectories.GetNextItem(i, LVNI_SELECTED)) != -1) {
		auto sdi = (ProfileDirectoryInfo*)ctrlDirectories.GetItemData(i);
		if (sdi->diffState == ProfileDirectoryInfo::DIFF_REMOVED) {
			sdi->diffState = ProfileDirectoryInfo::DIFF_NORMAL;
			sdi->state = ProfileDirectoryInfo::STATE_ADDED;
			added = true;
		}
	}*/

	if (!added) {
		tstring target;

		BrowseDlg dlg(m_hWnd, BrowseDlg::TYPE_GENERIC, BrowseDlg::DIALOG_SELECT_FOLDER);
		if (dlg.show(target)) {
			addDirectory(target);
		}
	} else if (SETTING(USE_OLD_SHARING_UI)) {
		//update the list to fix the colors and sorting
		RedrawWindow();
		ctrlDirectories.resort();
	}
	
	return 0;
}

LRESULT ShareDirectories::onClickedRemoveDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int i = -1;
	bool redraw = false;
	auto confirmOption = CONFIRM_ASK;
	int selectedDirs = ctrlDirectories.GetSelectedCount();
	while((i = ctrlDirectories.GetNextItem(i, LVNI_SELECTED)) != -1) {
		auto sdiPlain = (ProfileDirectoryInfo*)ctrlDirectories.GetItemData(i);
		selectedDirs--;

		//make sure that we increase the reference count or the dir may get deleted...
		auto sdi = *find_if(shareDirs, ProfileDirectoryInfo::PathCompare(sdiPlain->dir->path));

		removeDir(sdi->dir->path, curProfile, confirmOption, true, selectedDirs);
		ctrlDirectories.DeleteItem(i);
		i--;
	}

	if (redraw) {
		//update the list to fix the colors and sorting
		RedrawWindow();
		ctrlDirectories.resort();
	}
	
	return 0;
}

void ShareDirectories::removeDir(const string& rPath, ProfileToken aToken, ConfirmOption& confirmOption, bool checkDupes /*true*/, int remainingItems /*1*/) {
	auto dirItem = getItemByPath(rPath, false);
	if (!dirItem) {
		return;
	}

	dirItem->dir->profiles.erase(aToken);

	if (checkDupes && !dirItem->dir->profiles.empty()) {
		if (confirmOption == CONFIRM_ASK) {
			CTaskDialog taskdlg;

			int sel;
			BOOL remember = FALSE;
			tstring msg = TSTRING_F(X_PROFILE_DIRS_EXISTS, (int)dirItem->dir->profiles.size());
			taskdlg.SetMainInstructionText(msg.c_str());
			TASKDIALOG_BUTTON buttons[] =
			{
				{ IDC_REMOVE_OTHER, CTSTRING(REMOVE_OTHER_PROFILES), },
				{ IDC_LEAVE_OTHER, CTSTRING(LEAVE_OTHER_PROFILES), },
			};
			taskdlg.ModifyFlags(0, TDF_USE_COMMAND_LINKS);

			auto title = Text::toT(rPath);
			taskdlg.SetWindowTitle(title.c_str());
			//taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);

			taskdlg.SetButtons(buttons, _countof(buttons));
			taskdlg.SetMainIcon(TD_INFORMATION_ICON);

			tstring profiles = Text::toT(STRING(OTHER_PROFILES) + ":");
			for (const auto token : dirItem->dir->profiles) {
				profiles += _T("\n") + Text::toT(parent->getProfile(token)->name);
			}

			taskdlg.SetExpandedInformationText(profiles.c_str());
			taskdlg.SetExpandedControlText(TSTRING(SHOW_OTHER_PROFILES).c_str());

			if (remainingItems >= 1) {
				tstring remove_msg = CTSTRING_F(PERFORM_X_REMAINING, remainingItems);
				taskdlg.SetVerificationText(remove_msg.c_str());
				taskdlg.DoModal(NULL, &sel, 0, &remember);
			} else {
				taskdlg.DoModal(NULL, &sel);
			}

			if (remember) {
				confirmOption = sel == IDC_REMOVE_OTHER ? CONFIRM_REMOVE : CONFIRM_LEAVE;
			} 
					
			if (sel == IDC_LEAVE_OTHER)
				return;
		} else if (confirmOption == CONFIRM_LEAVE) {
			//the remember option was used with a previous item
			return;
		}

		auto profiles = dirItem->dir->profiles;
		for (auto i : profiles) {
			removeDir(rPath, i, confirmOption, false);
		}

		return;
	}

	if (!dirItem->dir->profiles.empty()) {
		return;
	}

	// erase/mark as removed
	auto& list = shareDirs;
	auto p = find_if(list, ProfileDirectoryInfo::PathCompare(rPath));
	dcassert(p != list.end());
	if (p != list.end()) {
		if ((*p)->state == ProfileDirectoryInfo::STATE_ADDED) {
			list.erase(p);
		} else if ((*p)->isCurItem()) {
			(*p)->state = ProfileDirectoryInfo::STATE_REMOVED;
		}
	}
}

bool ShareDirectories::showShareDlg(const ShareProfileInfo::List& spList, ProfileToken aProfile, const tstring& curName, ProfileTokenSet& profilesTokens, tstring& newName, bool rename) {
	if (!rename) {
		SharePageDlg virt;
		virt.rename = rename;
		virt.line = curName;
		virt.profiles = spList;
		virt.selectedProfile = aProfile;
		if(virt.DoModal(m_hWnd) == IDOK) {
			newName = virt.line;
			//profilesTokens.insert(virt.selectedProfiles.begin(), virt.selectedProfiles.end());

			profilesTokens = virt.selectedProfiles;
			if (profilesTokens.empty()) {
				return false;
			}

		} else {
			return false;
		}
	} else {
		LineDlg virt;
		virt.title = TSTRING(VIRTUAL_NAME);
		virt.description = TSTRING(VIRTUAL_NAME_LONG);
		virt.line = curName;
		if(virt.DoModal(m_hWnd) != IDOK) {
			return false;
		}

		newName = virt.line;
	}

	if (newName.empty()) {
		WinUtil::showMessageBox(TSTRING(NO_DIRECTORY_SPECIFIED), MB_ICONEXCLAMATION);
		return false;
	}

	newName = Text::toT(ShareManager::getInstance()->validateVirtualName(Text::fromT(newName)));
	return true;
}

LRESULT ShareDirectories::onClickedRenameDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int i = -1;
	while((i = ctrlDirectories.GetNextItem(i, LVNI_SELECTED)) != -1) {
		auto sdi = (ProfileDirectoryInfo*)ctrlDirectories.GetItemData(i);

		auto profileTokens = sdi->dir->profiles;

		tstring vName = Text::toT(sdi->dir->virtualName);
		//auto items = getItemsByPath(sdi->dir->path, false, false);
		tstring newName;

		ShareProfileInfo::List spList;
		for(auto j: sdi->dir->profiles) {
			if (j != curProfile)
				spList.push_back(parent->getProfile(j));
		}

		if (!showShareDlg(spList, curProfile, vName, profileTokens, newName, true))
			return 0;

		if (stricmp(vName, newName) != 0) {
			ctrlDirectories.SetItemText(i, 0, newName.c_str());
			sdi->dir->virtualName = Text::fromT(newName);
		} else {
			WinUtil::showMessageBox(TSTRING(SKIP_RENAME));
		}
	}

	return 0;
}

bool ShareDirectories::addDirectory(const tstring& aPath){
	auto path = Util::validatePath(Text::fromT(aPath), true);

	/* Check if we are trying to share a directory which exists already in this profile */
	for(const auto& sdi: shareDirs) {
		StringSet profileNames;
		for (auto t : sdi->dir->profiles) {
			profileNames.insert(parent->getProfile(t)->name);
		}

		if (AirUtil::isParentOrExactLocal(sdi->dir->path, path)) {
			if (Util::stricmp(sdi->dir->path, path) == 0) {
				if (!sdi->hasProfile(curProfile)) {
					continue;
				}

				WinUtil::showMessageBox(TSTRING(DIRECTORY_SHARED));
			} else {
				WinUtil::showMessageBox(TSTRING_F(DIRECTORY_PARENT_SHARED, Text::toT(Util::listToString(profileNames))));
			}

			return false;
		} else if (AirUtil::isSubLocal(sdi->dir->path, path)) {
			WinUtil::showMessageBox(TSTRING_F(DIRECTORY_SUBDIRS_SHARED, Text::toT(Util::listToString(profileNames))));
			return false;
		}
	}

	// Validate path
	try {
		ShareManager::getInstance()->validateRootPath(path, false);
	} catch (const ShareException& e) {
		WinUtil::showMessageBox(Text::toT(e.getError()), MB_ICONEXCLAMATION);
		return false;
	}

	ShareProfileInfo::List spList;
	ProfileTokenSet profileTokens = { curProfile };
	tstring newName;

	if (!showShareDlg(parent->getProfiles(), curProfile, Text::toT(ShareManager::getInstance()->validateVirtualName(Util::getLastDir(path))), profileTokens, newName, false)) {
		return false;
	}

	string virtualName = Text::fromT(newName);

	auto info = getItemByPath(path, true);
	if (info) {
		//check if the virtual name is different...
		if (info->dir->virtualName != virtualName) {
			info->dir->virtualName = virtualName;
		}

		info->dir->profiles.insert(profileTokens.begin(), profileTokens.end());
	} else {
		//create new directory
		auto dir = make_shared<ShareDirectoryInfo>(path, virtualName, false, profileTokens);
		info = make_shared<ProfileDirectoryInfo>(dir, ProfileDirectoryInfo::STATE_ADDED);

		shareDirs.emplace_back(info);
	}

	if(SETTING(USE_OLD_SHARING_UI)) {
		int i = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), Text::toT(info->dir->virtualName), 0, (LPARAM)info.get());
		ctrlDirectories.SetItemText(i, 1, Text::toT(info->dir->path).c_str());
		ctrlDirectories.SetItemText(i, 2, info->dir->size == 0 ? CTSTRING(NEW) : Util::formatBytesW(info->dir->size).c_str());
	}

	ctrlDirectories.resort();
	return true;
}

void ShareDirectories::write() { }

Dispatcher::F ShareDirectories::getThreadedTask() {
	return Dispatcher::F([=] { 
		applyChanges(true); 
	});
}
void ShareDirectories::applyChanges(bool /*isQuit*/) {
	ShareDirectoryInfoList dirs;
	auto getDirs = [&](ProfileDirectoryInfo::State aState) {
		dirs.clear();
		for (auto& sdi : shareDirs) {
			if (sdi->state == aState)
				dirs.push_back(sdi->dir);
		}
	};

	getDirs(ProfileDirectoryInfo::STATE_ADDED);
	if (!dirs.empty()) {
		ShareManager::getInstance()->addRootDirectories(dirs);
		//will be cleared later...
	}

	getDirs(ProfileDirectoryInfo::STATE_REMOVED);
	if (!dirs.empty()) {
		for (const auto& sdi : dirs) {
			ShareManager::getInstance()->removeRootDirectory(sdi->path);
		}
		//ShareManager::getInstance()->removeDirectories(dirs);
	}

	getDirs(ProfileDirectoryInfo::STATE_NORMAL);
	if (!dirs.empty()) {
		ShareManager::getInstance()->updateRootDirectories(dirs);
	}

	ShareManager::getInstance()->setExcludedPaths(excludedPaths);
}

bool ShareDirectories::addExcludeFolder(const string& path) {
	// make sure this is a sub folder of a shared folder
	if (find_if(shareDirs, [&path](const ProfileDirectoryInfoPtr& aDir) { return aDir->isCurItem() && AirUtil::isSubLocal(path, aDir->dir->path); } ) == shareDirs.end())
		return false;

	// Make sure this not a subfolder of an already excluded folder
	if (find_if(excludedPaths, [&path](const string& aDir) { return AirUtil::isParentOrExactLocal(aDir, path); } ) != excludedPaths.end())
		return false;

	// remove all sub folder excludes
	for(auto& j: excludedPaths) {
		if (AirUtil::isSubLocal(j, path))
			removeExcludeFolder(j);
	}

	// add it to the list
	excludedPaths.emplace(path);
	return true;
}

bool ShareDirectories::removeExcludeFolder(const string& aPath) {
	excludedPaths.erase(aPath);
	return true;
}

bool ShareDirectories::shareFolder(const string& path) {
	// check if it's part of the share before checking if it's in the exclusions
	//bool result = false;

	ProfileDirectoryInfoPtr foundInfo = nullptr;
	for (const auto& sdi : shareDirs | filtered(ProfileDirectoryInfo::HasProfile(curProfile))) {
		if (!sdi->isCurItem())
			continue;

		if ((path.size() == sdi->dir->path.size()) && (stricmp(path, sdi->dir->path) == 0)) {
			// is it a perfect match
			sdi->found = true;
			return true;
		} else if (path.size() > sdi->dir->path.size()) // this might be a subfolder of a shared folder
		{
			// if the left-hand side matches and there is a \ in the remainder then it is a subfolder
			if ((stricmp(path.substr(0, sdi->dir->path.size()), sdi->dir->path) == 0) && (path.find('\\', sdi->dir->path.size()) != string::npos)) {
				foundInfo = sdi;
				break;
			}
		}
	}

	if(!foundInfo)
		return false;

	// check if it's an excluded folder or a sub folder of an excluded folder
	if (find_if(excludedPaths, [&path](const string& aDir) { return AirUtil::isParentOrExactLocal(aDir, path); } ) != excludedPaths.end())
		return false;

	foundInfo->found = true;
	return true;
}