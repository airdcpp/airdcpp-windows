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


PropPage::TextItem LocationsPage::texts[] = {
	{ IDC_SETTINGS_FAVORITE_DIRECTORIES, ResourceManager::SETTINGS_FAVORITE_DIRS },
	{ IDC_REMOVE, ResourceManager::REMOVE },
	{ IDC_ADD, ResourceManager::SETTINGS_ADD_FOLDER },
	{ IDC_RENAME, ResourceManager::RENAME },
	{ IDC_FAVDIRS_SHOW_SHARED, ResourceManager::FAVDIRS_SHOW_SHARED },

	{ IDC_SETTINGS_DIRECTORIES, ResourceManager::SETTINGS_DIRECTORIES }, 
	{ IDC_SETTINGS_DOWNLOAD_DIRECTORY, ResourceManager::SETTINGS_DOWNLOAD_DIRECTORY },
	{ IDC_BROWSEDIR, ResourceManager::BROWSE },
	{ IDC_BROWSETEMPDIR, ResourceManager::BROWSE },

	{ IDC_FORMAT_REMOTE_TIME, ResourceManager::SETTINGS_FORMAT_REMOTE_TIME },
	{ IDC_SETTINGS_OPTIONS, ResourceManager::SETTINGS_OPTIONS },
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
	ctrlDirectories.InsertColumn(1, CTSTRING(PATH), LVCFMT_LEFT, rc.Width()*2 /2, 1);

	favoriteDirs = FavoriteManager::getInstance()->getFavoriteDirs();
	fillList();

	return TRUE;
}

void LocationsPage::write()
{	
	FavoriteManager::getInstance()->setFavoriteDirs(favoriteDirs);
	PropPage::write((HWND)*this, items);

	const string& s = SETTING(DOWNLOAD_DIRECTORY);
	if(s.length() > 0 && s[s.length() - 1] != '\\') {
		SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_DIRECTORY, s + '\\');
	}

	AirUtil::updateCachedSettings();
}

LRESULT LocationsPage::onItemChangedDirectories(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
	bool hasSel = ctrlDirectories.GetSelectedCount() > 0;
	::EnableWindow(GetDlgItem(IDC_REMOVE), hasSel);
	::EnableWindow(GetDlgItem(IDC_RENAME), hasSel);
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
	BrowseDlg browseDlg(m_hWnd, BrowseDlg::TYPE_SETTINGS_RESOURCES, BrowseDlg::DIALOG_SELECT_FOLDER);

	tstring path;
	if (!browseDlg.show(path)) {
		return 0;
	}

	LineDlg dlg;
	dlg.allowEmpty = false;
	dlg.title = TSTRING(VIRTUAL_NAME);
	dlg.description = path;
	dlg.line = Util::getLastDir(path);
	if (dlg.DoModal() == IDOK) {
		addDirectory(Text::fromT(path), Text::fromT(dlg.line));
	}

	return 0;
}
	

LRESULT LocationsPage::onClickedRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int i = -1;
	while((i = ctrlDirectories.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		auto itemIter = getByPosition(i);
		if(removeDirectory(itemIter->first)) {
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
		auto itemIter = getByPosition(i);

		LineDlg dlg;
		dlg.allowEmpty = false;
		dlg.title = TSTRING(RENAME);
		dlg.description = Text::toT(itemIter->first);
		dlg.line = Text::toT(itemIter->second);
		if (dlg.DoModal() == IDOK) {
			itemIter->second = Text::fromT(dlg.line);
			ctrlDirectories.SetItemText(i, 0, dlg.line.c_str());
		}
	}
	return 0;
}

FavoriteManager::FavoriteDirectoryMap::iterator LocationsPage::getByPosition(int aPos) noexcept {
	auto i = favoriteDirs.begin();
	advance(i, aPos);
	return i;
}

void LocationsPage::fillList() {
	ctrlDirectories.DeleteAllItems();

	for (auto j = favoriteDirs.begin(); j != favoriteDirs.end(); j++) {
		int i = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), Text::toT(j->second).c_str());
		ctrlDirectories.SetItemText(i, 1, Text::toT(j->first).c_str());
	}
}

void LocationsPage::addDirectory(const string& aPath, const string& aName) noexcept {
	favoriteDirs[aPath] = aName;
	fillList();
}

bool LocationsPage::removeDirectory(const string& aPath) noexcept {
	favoriteDirs.erase(aPath);
	return true;
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