/* 
 * Copyright (C) 2001-2016 Jacek Sieka, arnetheduck on gmail point com
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

#include <boost/range/algorithm.hpp>

#include <airdcpp/Util.h>
#include <airdcpp/SettingsManager.h>
#include <airdcpp/FavoriteManager.h>
#include <airdcpp/AirUtil.h>

#include "BrowseDlg.h"
#include "Resource.h"
#include "LocationsPage.h"
#include "WinUtil.h"
#include "LineDlg.h"
#include "FavoriteDirDlg.h"

PropPage::TextItem LocationsPage::texts[] = {
	{ IDC_SETTINGS_FAVORITE_DIRECTORIES, ResourceManager::SETTINGS_FAVORITE_DIRS },
	{ IDC_REMOVE, ResourceManager::REMOVE },
	{ IDC_ADD, ResourceManager::SETTINGS_ADD_FOLDER },
	{ IDC_RENAME, ResourceManager::SETTINGS_CHANGE },
	{ IDC_FAVDIRS_SHOW_SHARED, ResourceManager::FAVDIRS_SHOW_SHARED },

	{ IDC_SETTINGS_DIRECTORIES, ResourceManager::SETTINGS_DIRECTORIES }, 
	{ IDC_SETTINGS_DOWNLOAD_DIRECTORY, ResourceManager::SETTINGS_DOWNLOAD_DIRECTORY },
	{ IDC_BROWSEDIR, ResourceManager::BROWSE },
	{ IDC_BROWSETEMPDIR, ResourceManager::BROWSE },

	{ IDC_FORMAT_REMOTE_TIME, ResourceManager::SETTINGS_FORMAT_REMOTE_TIME },
	{ IDC_AUTOPATH_CAPTION, ResourceManager::AUTOPATH_CAPTION }, 
	{ IDC_SETTINGS_OPTIONS, ResourceManager::SETTINGS_OPTIONS }, 
	{ IDC_MOVEDOWN, ResourceManager::MOVE_DOWN }, 
	{ IDC_MOVEUP, ResourceManager::MOVE_UP }, 
	{ 0, ResourceManager::LAST }
};


PropPage::Item LocationsPage::items[] = {
	{ IDC_DOWNLOADDIR,	SettingsManager::DOWNLOAD_DIRECTORY, PropPage::T_STR }, 
	{ IDC_FAVDIRS_SHOW_SHARED, SettingsManager::SHOW_SHARED_DIRS_FAV, PropPage::T_BOOL },
	{ IDC_FORMAT_REMOTE_TIME, SettingsManager::FORMAT_DIR_REMOTE_TIME, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

LRESULT LocationsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);
	ctrlDirectories.Attach(GetDlgItem(IDC_FAVORITE_DIRECTORIES));
	ctrlDirectories.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
		
	// Prepare shared dir list
	CRect rc; 
	ctrlDirectories.GetClientRect(rc); 
	ctrlDirectories.InsertColumn(0, CTSTRING(FAVORITE_DIR_NAME), LVCFMT_LEFT, rc.Width()/4, 0); 
	ctrlDirectories.InsertColumn(1, CTSTRING(PATHS), LVCFMT_LEFT, rc.Width()*2 /2, 1);
	favoriteDirs = FavoriteManager::getInstance()->getFavoriteDirs();
	for(auto j = favoriteDirs.begin(); j != favoriteDirs.end(); j++) {
		int i = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), Text::toT(j->first).c_str());
		ctrlDirectories.SetItemText(i, 1, Text::toT(Util::listToString(j->second)).c_str());
	}

	ctrlAutoSelect.Attach(GetDlgItem(IDC_AUTOPATH_METHOD));
	ctrlAutoSelect.AddString(CTSTRING(AUTOSELECT_MOST_SPACE));
	ctrlAutoSelect.AddString(CTSTRING(AUTOSELECT_LEAST_SPACE));
	ctrlAutoSelect.SetCurSel(SETTING(DL_AUTOSELECT_METHOD));
	return TRUE;
}

void LocationsPage::write()
{	
	FavoriteManager::getInstance()->saveFavoriteDirs(favoriteDirs);
	PropPage::write((HWND)*this, items);

	const string& s = SETTING(DOWNLOAD_DIRECTORY);
	if(s.length() > 0 && s[s.length() - 1] != '\\') {
		SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_DIRECTORY, s + '\\');
	}

	SettingsManager::getInstance()->set(SettingsManager::DL_AUTOSELECT_METHOD, ctrlAutoSelect.GetCurSel());
	AirUtil::updateCachedSettings();
}

LRESULT LocationsPage::onItemChangedDirectories(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
	bool hasSel = ctrlDirectories.GetSelectedCount() > 0;
	::EnableWindow(GetDlgItem(IDC_REMOVE), hasSel);
	::EnableWindow(GetDlgItem(IDC_RENAME), hasSel);
	::EnableWindow(GetDlgItem(IDC_MOVEUP), hasSel);
	::EnableWindow(GetDlgItem(IDC_MOVEDOWN), hasSel);
	return 0;		
}

LRESULT LocationsPage::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	switch(kd->wVKey) {
	case VK_INSERT:
		PostMessage(WM_COMMAND, IDC_ADD, 0);
		break;
	case VK_DELETE:
		PostMessage(WM_COMMAND, IDC_REMOVE, 0);
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}

LRESULT LocationsPage::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

	if(item->iItem >= 0) {
		PostMessage(WM_COMMAND, IDC_RENAME, 0);
	} else if(item->iItem == -1) {
		PostMessage(WM_COMMAND, IDC_ADD, 0);
	}

	return 0;
}

LRESULT LocationsPage::onClickedAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	FavoriteDirDlg dlg;
	if(dlg.DoModal() == IDOK) {
		addDirs(Text::fromT(dlg.vName), dlg.paths);
	}
	return 0;
}
	

LRESULT LocationsPage::onClickedRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR buf[MAX_PATH];

	int i = -1;
	while((i = ctrlDirectories.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		ctrlDirectories.GetItemText(i, 0, buf, MAX_PATH); //vname
		if(removeFavoriteDir(Text::fromT(buf))) {
			ctrlDirectories.DeleteItem(i);
			i--;
		}
	}
	
	return 0;
}

LRESULT LocationsPage::onClickedRename(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int i = -1;
	while((i = ctrlDirectories.GetNextItem(i, LVNI_SELECTED)) != -1) {
		
		FavoriteDirDlg dlg;
		auto oldItem = favoriteDirs[i];
		dlg.vName = Text::toT(oldItem.first);
		dlg.paths = oldItem.second;
	
		if(dlg.DoModal() == IDOK) {
			sort(dlg.paths.begin(), dlg.paths.end());
			pair<string, StringList> newItem = { Text::fromT(dlg.vName), dlg.paths };

			if (stricmp(newItem.first, oldItem.first) != 0) { //vName changed check if one with the same name already exits.
				auto k = boost::find_if(favoriteDirs, CompareFirst<string, StringList>(newItem.first));
				if (k != favoriteDirs.end()) {
					MessageBox(CTSTRING(ITEM_NAME_EXISTS), CTSTRING(NAME_IN_USE), MB_ICONEXCLAMATION | MB_OK);
					//reset name and do a step back so the user can fix his mistake
					newItem.first = oldItem.first;
					//keep possible changes to paths
					favoriteDirs[i] = newItem;
					ctrlDirectories.SetItemText(i, 1, Text::toT(Util::listToString(dlg.paths)).c_str());
					i = i - 1;
					continue;
				}
			}
			favoriteDirs[i] = newItem;
			ctrlDirectories.SetItemText(i, 0, Text::toT(newItem.first).c_str());
			ctrlDirectories.SetItemText(i, 1, Text::toT(Util::listToString(dlg.paths)).c_str());
		}
	}
	return 0;
}

LRESULT LocationsPage::onMove(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int id = ctrlDirectories.GetSelectedIndex();
	if(id != -1) {
		switch(wID) {
			case IDC_MOVEUP:
				if(id != 0) {
					if((int)favoriteDirs.size() > id) {
						swap(favoriteDirs[id], favoriteDirs[id-1]);
						ctrlDirectories.moveItem(id, id-1);
						ctrlDirectories.SelectItem(id-1);
					}	
				}
				break;
			case IDC_MOVEDOWN:
				if(id != ctrlDirectories.GetItemCount()-1) {
					if((int)favoriteDirs.size() > id) {
						swap(favoriteDirs[id], favoriteDirs[id+1]);
						ctrlDirectories.moveItem(id, id+1);
						ctrlDirectories.SelectItem(id+1);
					}
				}
				break;
			}
		}
	return 0;
}

void LocationsPage::addDirs(const string& vName, const StringList& aPaths){
	int pos = 0;
	StringList paths = aPaths;
	sort(paths.begin(), paths.end());

	auto i = boost::find_if(favoriteDirs, CompareFirst<string, StringList>(vName));
	if (i != favoriteDirs.end()) {
		pos = ctrlDirectories.find(Text::toT(vName));
		//merge(paths.begin(), paths.end(), i->second.begin(), i->second.end(), back_inserter(i->second));

		for(auto& p: aPaths) {
			if (find(i->second, p) == i->second.end())
				i->second.push_back(p); 
		}
		paths = i->second;
	} else {
		favoriteDirs.emplace_back(vName, paths);
		pos = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), Text::toT(vName) );
	}
	dcassert(pos != -1);
	ctrlDirectories.SetItemText(pos, 1, Text::toT(Util::listToString(paths)).c_str());
}

bool LocationsPage::removeFavoriteDir(const string& vName) {
	auto i = boost::find_if(favoriteDirs, CompareFirst<string, StringList>(vName));
	if (i != favoriteDirs.end()) {
		favoriteDirs.erase(i);
		return true;
	}
	return false;
}


LRESULT LocationsPage::onClickedBrowseDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));

	BrowseDlg dlg(m_hWnd, BrowseDlg::TYPE_SETTINGS_RESOURCES, BrowseDlg::DIALOG_SELECT_FOLDER);
	dlg.setPath(target);
	if (dlg.show(target)) {
		SetDlgItemText(IDC_DOWNLOADDIR, target.c_str());
	}
	return 0;
}