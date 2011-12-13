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
#include "../client/FavoriteManager.h"

#include "Resource.h"
#include "FavoriteDirsPage.h"
#include "WinUtil.h"
#include "LineDlg.h"

PropPage::TextItem FavoriteDirsPage::texts[] = {
	{ IDC_SETTINGS_FAVORITE_DIRECTORIES, ResourceManager::SETTINGS_FAVORITE_DIRS },
	{ IDC_REMOVE, ResourceManager::REMOVE },
	{ IDC_ADD, ResourceManager::SETTINGS_ADD_FOLDER },
	{ IDC_RENAME, ResourceManager::SETTINGS_RENAME_FOLDER },
	{ IDC_FAVDIRS_SHOW_SHARED, ResourceManager::FAVDIRS_SHOW_SHARED },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};


PropPage::Item FavoriteDirsPage::items[] = {
	{ IDC_FAVDIRS_SHOW_SHARED, SettingsManager::SHOW_SHARED_DIRS_FAV, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

LRESULT FavoriteDirsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);
	ctrlDirectories.Attach(GetDlgItem(IDC_FAVORITE_DIRECTORIES));
	ctrlDirectories.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
		
	// Prepare shared dir list
	CRect rc; 
	ctrlDirectories.GetClientRect(rc); 
	ctrlDirectories.InsertColumn(0, CTSTRING(FAVORITE_DIR_NAME), LVCFMT_LEFT, rc.Width()/4, 0); 
	ctrlDirectories.InsertColumn(1, CTSTRING(DIRECTORY), LVCFMT_LEFT, rc.Width()*3 /4, 1);
	favoriteDirs = FavoriteManager::getInstance()->getFavoriteDirs();
	for(auto j = favoriteDirs.begin(); j != favoriteDirs.end(); j++) {
		tstring vname = Text::toT(j->first);
		for (auto s = j->second.begin(); s != j->second.end(); ++s) {
			int i = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), vname);
			ctrlDirectories.SetItemText(i, 1, Text::toT(*s).c_str());
		}
	}
	
	return TRUE;
}


void FavoriteDirsPage::write()
{
	PropPage::write((HWND)*this, items);
	FavoriteManager::getInstance()->saveFavoriteDirs(favoriteDirs);
}

LRESULT FavoriteDirsPage::onDropFiles(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/){
	HDROP drop = (HDROP)wParam;
	tstring buf;
	buf.resize(MAX_PATH);

	UINT nrFiles;
	
	nrFiles = DragQueryFile(drop, (UINT)-1, NULL, 0);
	
	for(UINT i = 0; i < nrFiles; ++i){
		if(DragQueryFile(drop, i, &buf[0], MAX_PATH)){
			if(PathIsDirectory(&buf[0]))
				addDirectory(buf);
		}
	}

	DragFinish(drop);

	return 0;
}

LRESULT FavoriteDirsPage::onItemchangedDirectories(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	NM_LISTVIEW* lv = (NM_LISTVIEW*) pnmh;
	::EnableWindow(GetDlgItem(IDC_REMOVE), (lv->uNewState & LVIS_FOCUSED));
	::EnableWindow(GetDlgItem(IDC_RENAME), (lv->uNewState & LVIS_FOCUSED));
	::EnableWindow(GetDlgItem(IDC_MOVEUP), (lv->uNewState & LVIS_FOCUSED));
	::EnableWindow(GetDlgItem(IDC_MOVEDOWN), (lv->uNewState & LVIS_FOCUSED));
	return 0;		
}

LRESULT FavoriteDirsPage::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
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

LRESULT FavoriteDirsPage::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

	if(item->iItem >= 0) {
		PostMessage(WM_COMMAND, IDC_RENAME, 0);
	} else if(item->iItem == -1) {
		PostMessage(WM_COMMAND, IDC_ADD, 0);
	}

	return 0;
}

LRESULT FavoriteDirsPage::onClickedAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	tstring target;
	if(WinUtil::browseDirectory(target, m_hWnd)) {
		addDirectory(target);
	}
	
	return 0;
}

LRESULT FavoriteDirsPage::onClickedRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR buf[MAX_PATH];
	TCHAR buf2[MAX_PATH];

	int i = -1;
	while((i = ctrlDirectories.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		ctrlDirectories.GetItemText(i, 0, buf, MAX_PATH); //vname
		ctrlDirectories.GetItemText(i, 1, buf2, MAX_PATH); //path
		if(removeFavoriteDir(Text::fromT(buf), Text::fromT(buf2)))
			ctrlDirectories.DeleteItem(i);
	}
	
	return 0;
}

LRESULT FavoriteDirsPage::onClickedRename(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR buf[MAX_PATH];
	TCHAR buf2[MAX_PATH];

	int i = -1;
	while((i = ctrlDirectories.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ctrlDirectories.GetItemText(i, 0, buf, MAX_PATH); //vname
		ctrlDirectories.GetItemText(i, 1, buf2, MAX_PATH); //path

		LineDlg virt;
		virt.title = TSTRING(FAVORITE_DIR_NAME);
		virt.description = TSTRING(FAVORITE_DIR_NAME_LONG);
		virt.line = tstring(buf);
		if(virt.DoModal(m_hWnd) == IDOK) {
			if (renameFavoriteDir(Text::fromT(buf), Text::fromT(virt.line), Text::fromT(buf2))) {
				ctrlDirectories.SetItemText(i, 0, virt.line.c_str());
			} else {
				MessageBox(CTSTRING(DIRECTORY_ADD_ERROR));
			}
		}
	}
	return 0;
}

LRESULT FavoriteDirsPage::onMove(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	/*int i = ctrlDirectories.GetSelectedIndex();
	
	if(wID == IDC_MOVEUP && i > 0){
		StringPair j  = favoriteDirs[i];

		favoriteDirs[i] = favoriteDirs[i-1];
		favoriteDirs[i-1] = j;

		ctrlDirectories.DeleteItem(i);
		ctrlDirectories.insert(i-1, Text::toT(j.second));
		ctrlDirectories.SetItemText(i-1, 1, Text::toT(j.first).c_str());
		ctrlDirectories.SelectItem(i-1);
	
	} else if( wID == IDC_MOVEDOWN && i < ctrlDirectories.GetItemCount()-1 ) {
		StringPair j = favoriteDirs[i];

		favoriteDirs[i] = favoriteDirs[i+1];
		favoriteDirs[i+1] = j;

		ctrlDirectories.DeleteItem(i);
		ctrlDirectories.insert(i+1, Text::toT(j.second));
		ctrlDirectories.SetItemText(i+1, 1, Text::toT(j.first).c_str());
		ctrlDirectories.SelectItem(i+1);
	} */

	return 0;
}


bool FavoriteDirsPage::renameFavoriteDir(const string& aName, const string& anotherName, const string& aPath) {
	if (!removeFavoriteDir(aName, aPath)) {
		return false;
	}
	return addFavoriteDir(aPath, anotherName);
}

void FavoriteDirsPage::addDirectory(const tstring& aPath){
	tstring path = aPath;
	if( path[ path.length() -1 ] != PATH_SEPARATOR )
		path += PATH_SEPARATOR;

	LineDlg virt;
	virt.title = TSTRING(FAVORITE_DIR_NAME);
	virt.description = TSTRING(FAVORITE_DIR_NAME_LONG);
	virt.line = Util::getLastDir(path);
	if(virt.DoModal(m_hWnd) == IDOK) {
		if (addFavoriteDir(Text::fromT(path), Text::fromT(virt.line))) {
			int j = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), virt.line );
			ctrlDirectories.SetItemText(j, 1, path.c_str());
		} else {
			MessageBox(CTSTRING(DIRECTORY_ADD_ERROR));
		}
	}
}

bool FavoriteDirsPage::addFavoriteDir(const string& aDirectory, const string & aName){
	string path = aDirectory;

	if( path[ path.length() -1 ] != PATH_SEPARATOR )
		path += PATH_SEPARATOR;

	for(auto i = favoriteDirs.begin(); i != favoriteDirs.end(); ++i) {
		if(stricmp(aName, i->first) == 0) {
			//virtual name exists
			for (auto s = i->second.begin(); s != i->second.end(); ++s) {
				if((strnicmp(path, *s, (*s).length()) == 0) && (strnicmp(path, (*s), path.length()) == 0)) {
					return false;
				}
			}
			i->second.push_back(path);
			return true;
		}
	}

	//new virtual name
	StringList tmp;
	tmp.push_back(path);
	favoriteDirs.push_back(make_pair(aName, tmp));
	return true;
}


bool FavoriteDirsPage::removeFavoriteDir(const string& aName, const string& aTarget) {
	string d(aTarget);

	if(d[d.length() - 1] != PATH_SEPARATOR)
		d += PATH_SEPARATOR;

	for(auto i = favoriteDirs.begin(); i != favoriteDirs.end(); ++i) {
		if(stricmp(aName, i->first) == 0) {
			for (auto s = i->second.begin(); s != i->second.end(); ++s) {
				if(stricmp((*s).c_str(), d.c_str()) == 0) {
					i->second.erase(s);
					if (i->second.empty()) {
						favoriteDirs.erase(i);
					}
					return true;
				}
			}
		}
	}
	return false;
}

/**
 * @file
 * $Id: FavoriteDirsPage.cpp 477 2010-01-29 08:59:43Z bigmuscle $
 */
