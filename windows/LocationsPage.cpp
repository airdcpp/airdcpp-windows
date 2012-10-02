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

#include <boost/range/algorithm.hpp>

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
	{ IDC_FORMAT_REMOTE_TIME, ResourceManager::SETTINGS_FORMAT_REMOTE_TIME },
	{ IDC_AUTOPATH_CAPTION, ResourceManager::AUTOPATH_CAPTION }, 
	{ IDC_SETTINGS_OPTIONS, ResourceManager::SETTINGS_OPTIONS }, 
	{ IDC_CLEAR_DIR_HISTORY, ResourceManager::CLEAR_DIR_HISTORY }, 
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};


PropPage::Item LocationsPage::items[] = {
	{ IDC_TEMP_DOWNLOAD_DIRECTORY, SettingsManager::TEMP_DOWNLOAD_DIRECTORY, PropPage::T_STR },
	{ IDC_DOWNLOADDIR,	SettingsManager::DOWNLOAD_DIRECTORY, PropPage::T_STR }, 
	{ IDC_FAVDIRS_SHOW_SHARED, SettingsManager::SHOW_SHARED_DIRS_FAV, PropPage::T_BOOL },
	{ IDC_FORMAT_REMOTE_TIME, SettingsManager::FORMAT_DIR_REMOTE_TIME, PropPage::T_BOOL },
	{ IDC_CLEAR_DIR_HISTORY, SettingsManager::CLEAR_DIR_HISTORY, PropPage::T_BOOL },
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
		ctrlDirectories.SetItemText(i, 1, Text::toT(Util::toString(j->second)).c_str());
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
	const string& t = SETTING(TEMP_DOWNLOAD_DIRECTORY);
	if(t.length() > 0 && t[t.length() - 1] != '\\') {
		SettingsManager::getInstance()->set(SettingsManager::TEMP_DOWNLOAD_DIRECTORY, t + '\\');
	}

	SettingsManager::getInstance()->set(SettingsManager::DL_AUTOSELECT_METHOD, ctrlAutoSelect.GetCurSel());
	AirUtil::updateCachedSettings();
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
	TCHAR buf[MAX_PATH];

	int i = -1;
	while((i = ctrlDirectories.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ctrlDirectories.GetItemText(i, 0, buf, MAX_PATH); //vname
		
		FavoriteDirDlg dlg;
		dlg.vName = buf;
		auto s = find_if(favoriteDirs.begin(), favoriteDirs.end(), CompareFirst<string, StringList>(Text::fromT(buf)));
		dcassert(s != favoriteDirs.end());
		dlg.paths = s->second;
	
		if(dlg.DoModal() == IDOK) {
			if (removeFavoriteDir(Text::fromT(buf)))
				ctrlDirectories.DeleteItem(i);

			addDirs(Text::fromT(dlg.vName), dlg.paths);
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
	std::sort(paths.begin(), paths.end());

	auto i = find_if(favoriteDirs.begin(), favoriteDirs.end(), CompareFirst<string, StringList>(vName));
	if (i != favoriteDirs.end()) {
		pos = ctrlDirectories.find(Text::toT(vName));
		boost::for_each(aPaths, [&](string p) {
			if (find(i->second.begin(), i->second.end(), p) == i->second.end())
				i->second.push_back(p); 
		});
		paths = i->second;
	} else {
		favoriteDirs.push_back(make_pair(vName, paths));
		pos = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), Text::toT(vName) );
	}
	dcassert(pos != -1);
	ctrlDirectories.SetItemText(pos, 1, Text::toT(Util::toString(paths)).c_str());
}

bool LocationsPage::removeFavoriteDir(const string& vName) {
	auto i = find_if(favoriteDirs.begin(), favoriteDirs.end(), CompareFirst<string, StringList>(vName));
	if (i != favoriteDirs.end()) {
		favoriteDirs.erase(i);
		return true;
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