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
#include "../client/AirUtil.h"

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
	{ IDC_BROWSEDIR, ResourceManager::BROWSE_ACCEL },
	{ IDC_SETTINGS_UNFINISHED_DOWNLOAD_DIRECTORY, ResourceManager::SETTINGS_UNFINISHED_DOWNLOAD_DIRECTORY }, 
	{ IDC_BROWSETEMPDIR, ResourceManager::BROWSE },
	{ IDC_SETTINGS_BTN_TARGETDRIVE, ResourceManager::SETTINGS_USE_TARGETDRIVE }, 
	{ IDC_SETTINGS_TARGETDRIVE_NOTE, ResourceManager::SETTINGS_TARGETDRIVE_NOTE }, 
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};


PropPage::Item LocationsPage::items[] = {
	{ IDC_TEMP_DOWNLOAD_DIRECTORY, SettingsManager::TEMP_DOWNLOAD_DIRECTORY, PropPage::T_STR },
	{ IDC_DOWNLOADDIR,	SettingsManager::DOWNLOAD_DIRECTORY, PropPage::T_STR }, 
	{ IDC_FAVDIRS_SHOW_SHARED, SettingsManager::SHOW_SHARED_DIRS_FAV, PropPage::T_BOOL },
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
	ctrlDirectories.InsertColumn(1, CTSTRING(DIRECTORY), LVCFMT_LEFT, rc.Width()*2 /2, 1);
	favoriteDirs = FavoriteManager::getInstance()->getFavoriteDirs();
	for(auto j = favoriteDirs.begin(); j != favoriteDirs.end(); j++) {
		tstring vname = Text::toT(j->first);
		int i = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), vname);
		string paths = Util::emptyString;
		for (auto s = j->second.begin(); s != j->second.end(); ++s) {
			if(!paths.empty())
				paths += "|";

			paths += *s;
		}
		ctrlDirectories.SetItemText(i, 1, Text::toT(paths).c_str());
	}
	
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
	const string& t = SETTING(TEMP_DOWNLOAD_DIRECTORY);
	if(t.length() > 0 && t[t.length() - 1] != '\\') {
		SettingsManager::getInstance()->set(SettingsManager::TEMP_DOWNLOAD_DIRECTORY, t + '\\');
		
	}
	AirUtil::updateCachedSettings();
}

LRESULT LocationsPage::onDropFiles(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/){
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

LRESULT LocationsPage::onItemchangedDirectories(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	NM_LISTVIEW* lv = (NM_LISTVIEW*) pnmh;
	::EnableWindow(GetDlgItem(IDC_REMOVE), (lv->uNewState & LVIS_FOCUSED));
	::EnableWindow(GetDlgItem(IDC_RENAME), (lv->uNewState & LVIS_FOCUSED));
	::EnableWindow(GetDlgItem(IDC_MOVEUP), (lv->uNewState & LVIS_FOCUSED));
	::EnableWindow(GetDlgItem(IDC_MOVEDOWN), (lv->uNewState & LVIS_FOCUSED));
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
	addDirectory(Util::emptyStringT);
	return 0;
}
	

LRESULT LocationsPage::onClickedRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR buf[MAX_PATH];

	int i = -1;
	while((i = ctrlDirectories.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		ctrlDirectories.GetItemText(i, 0, buf, MAX_PATH); //vname
		if(removeFavoriteDir(Text::fromT(buf)))
			ctrlDirectories.DeleteItem(i);
	}
	
	return 0;
}

LRESULT LocationsPage::onClickedRename(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR buf[MAX_PATH];
	TCHAR buf2[2048];

	int i = -1;
	while((i = ctrlDirectories.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ctrlDirectories.GetItemText(i, 0, buf, MAX_PATH); //vname
		ctrlDirectories.GetItemText(i, 1, buf2, 2048); //path
		
		FavoriteDirDlg dlg;
		dlg.name = buf;
		dlg.path = buf2;
	
		if(dlg.DoModal() == IDOK) {
			removeFavoriteDir(Text::fromT(buf));
			ctrlDirectories.DeleteItem(i);
			
			string path = Text::fromT(dlg.path + _T("|"));
			string::size_type j = 0;
			string::size_type i = 0;
	
			while((i = path.find("|", j)) != string::npos) {
				string str = path.substr(j, i-j);
				j = i +1;
				if(str.size() >= 3){ //path must be atleast 3 characters
					if (addFavoriteDir(str, Text::fromT(dlg.name))) {
						int pos;
						pos = ctrlDirectories.find(dlg.name);
						if(pos == -1) {
							//new virtual name
							pos = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), dlg.name );
							ctrlDirectories.SetItemText(pos, 1, dlg.path.c_str());
						} else {
							//existing virtual name, reload the paths in it.
							tstring newstr = Util::emptyStringT;
							auto paths = favoriteDirs[pos];
							for(auto p = paths.second.begin(); p != paths.second.end(); ++p) {
								if(!newstr.empty())
									newstr += _T("|");
								newstr += Text::toT(*p);
							}
							ctrlDirectories.SetItemText(pos, 1, newstr.c_str());
						}
					}
				}
			}
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

void LocationsPage::addDirectory(const tstring& aPath){
	tstring _path = aPath;
	if(!_path.empty()) {
		if( _path[ _path.length() -1 ] != PATH_SEPARATOR )
			_path += PATH_SEPARATOR;
	}
	FavoriteDirDlg dlg;
	dlg.path = _path;
	if(dlg.DoModal() == IDOK) {
		string path = Text::fromT(dlg.path) + "|";
		string::size_type j = 0;
		string::size_type i = 0;
	
		while((i = path.find("|", j)) != string::npos) {
			string str = path.substr(j, i-j);
			j = i +1;
			if(str.size() >= 3) { //path must be atleast 3 characters
				if(addFavoriteDir(str, Text::fromT(dlg.name))) {
					int pos;
					pos = ctrlDirectories.find(dlg.name);
					if(pos == -1) {
						//new virtual name
						pos = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), dlg.name );
						ctrlDirectories.SetItemText(pos, 1, dlg.path.c_str());
					} else {
						//existing virtual name, reload the paths in it.
						tstring newstr = Util::emptyStringT;
						auto paths = favoriteDirs[pos];
						for(auto p = paths.second.begin(); p != paths.second.end(); ++p) {
							if(!newstr.empty())
								newstr += _T("|");
							newstr += Text::toT(*p);
						}
						ctrlDirectories.SetItemText(pos, 1, newstr.c_str());
					}
				}
			}
		}
	}
}

bool LocationsPage::addFavoriteDir(const string& aDirectory, const string & aName){
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


bool LocationsPage::removeFavoriteDir(const string& aName) {

	for(auto i = favoriteDirs.begin(); i != favoriteDirs.end(); ++i) {
		if(stricmp(aName, i->first) == 0) {
				favoriteDirs.erase(i);
					return true;
		}
	}
	return false;
}


LRESULT LocationsPage::onClickedBrowseDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	tstring dir = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
	if(WinUtil::browseDirectory(dir, m_hWnd))
	{
		// Adjust path string
		if(dir.size() > 0 && dir[dir.size() - 1] != '\\')
			dir += '\\';
	
		SetDlgItemText(IDC_DOWNLOADDIR, dir.c_str());
	}
	return 0;
}

LRESULT LocationsPage::onClickedBrowseTempDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	tstring dir = Text::toT(SETTING(TEMP_DOWNLOAD_DIRECTORY));
	if(WinUtil::browseDirectory(dir, m_hWnd))
	{
		// Adjust path string
		if(dir.size() > 0 && dir[dir.size() - 1] != '\\')
			dir += '\\';

		SetDlgItemText(IDC_TEMP_DOWNLOAD_DIRECTORY, dir.c_str());
	}
	return 0;
}

LRESULT LocationsPage::onClickedTargetdrive(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	SetDlgItemText(IDC_TEMP_DOWNLOAD_DIRECTORY, _T("%[targetdrive]DCUnfinished"));
	return 0;
}


/**
 * @file
 * $Id: FavoriteDirsPage.cpp 477 2010-01-29 08:59:43Z bigmuscle $
 */
