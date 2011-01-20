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
#include "../client/ShareManager.h"
#include "../client/SettingsManager.h"
#include "../client/version.h"

#include "Resource.h"
#include "SharePage.h"
#include "WinUtil.h"
#include "HashProgressDlg.h"
#include "LineDlg.h"
#include "PropertiesDlg.h"

PropPage::TextItem SharePage::texts[] = {
	{ IDC_SETTINGS_SHARED_DIRECTORIES, ResourceManager::SETTINGS_SHARED_DIRECTORIES },
	{ IDC_SETTINGS_SHARE_SIZE, ResourceManager::SETTINGS_SHARE_SIZE }, 
	{ IDC_SHAREHIDDEN, ResourceManager::SETTINGS_SHARE_HIDDEN },
	{ IDC_REMOVE, ResourceManager::REMOVE },
	{ IDC_ADD, ResourceManager::SETTINGS_ADD_FOLDER },
	{ IDC_RENAME, ResourceManager::SETTINGS_RENAME_FOLDER },
	{ IDC_SETTINGS_ONLY_HASHED, ResourceManager::SETTINGS_ONLY_HASHED },
	{ IDC_SETTINGS_AUTO_REFRESH_TIME, ResourceManager::SETTINGS_AUTO_REFRESH_TIME },
	{ IDC_SETTINGS_INCOMING_REFRESH_TIME, ResourceManager::SETTINGS_INCOMING_REFRESH_TIME },
	{ IDC_SETTINGS_MAX_HASH_SPEED, ResourceManager::SETTINGS_MAX_HASH_SPEED },
	{ IDC_SETTINGS_MBS, ResourceManager::MBPS },
	{ IDC_REFRESH_ON_SHAREPAGE, ResourceManager::DISABLE_REFRESH_ON_SHAREPAGE},
	{ IDC_REFRESH_VNAME_ON_SHAREPAGE, ResourceManager::REFRESH_VNAME_ON_SHAREPAGE},
	{ IDC_SHARE_SFV, ResourceManager::SETTINGS_SHARE_SFV },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item SharePage::items[] = {
	{ IDC_SHAREHIDDEN, SettingsManager::SHARE_HIDDEN, PropPage::T_BOOL },
	{ IDC_AUTO_REFRESH_TIME, SettingsManager::AUTO_REFRESH_TIME, PropPage::T_INT },
	{ IDC_SHARE_SFV, SettingsManager::SHARE_SFV, PropPage::T_BOOL },
	{ IDC_INCOMING_REFRESH_TIME, SettingsManager::INCOMING_REFRESH_TIME, PropPage::T_INT },
	{ IDC_MAX_HASH_SPEED, SettingsManager::MAX_HASH_SPEED, PropPage::T_INT },
	{ IDC_REFRESH_ON_SHAREPAGE, SettingsManager::DISABLE_REFRESH_ON_SHAREPAGE, PropPage::T_BOOL },	
	{ IDC_REFRESH_VNAME_ON_SHAREPAGE, SettingsManager::REFRESH_VNAME_ON_SHAREPAGE, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

LRESULT SharePage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);

	GetDlgItem(IDC_TREE1).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_HIDE : SW_SHOW);
	GetDlgItem(IDC_DIRECTORIES).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_ADD).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_REMOVE).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);
	GetDlgItem(IDC_RENAME).ShowWindow((BOOLSETTING(USE_OLD_SHARING_UI)) ? SW_SHOW : SW_HIDE);


	ctrlDirectories.Attach(GetDlgItem(IDC_DIRECTORIES));
		ctrlDirectories.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES);
		
	ctrlTotal.Attach(GetDlgItem(IDC_TOTAL));

	PropPage::read((HWND)*this, items);

	if(BOOLSETTING(USE_OLD_SHARING_UI)) {
		// Prepare shared dir list
		ctrlDirectories.InsertColumn(0, CTSTRING(VIRTUAL_NAME), LVCFMT_LEFT, 80, 0);
		ctrlDirectories.InsertColumn(1, CTSTRING(DIRECTORY), LVCFMT_LEFT, 197, 1);
		ctrlDirectories.InsertColumn(2, CTSTRING(SIZE), LVCFMT_RIGHT, 90, 2);
		StringPairList directories = ShareManager::getInstance()->getDirectories(ShareManager::REFRESH_ALL);
		for(StringPairIter j = directories.begin(); j != directories.end(); j++)
		{
			int i = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), Text::toT(j->first));
			ctrlDirectories.SetItemText(i, 1, Text::toT(j->second).c_str() );
			ctrlDirectories.SetItemText(i, 2, Util::formatBytesW(ShareManager::getInstance()->getShareSize(j->second)).c_str());
			StringList incoming =  ShareManager::getInstance()->getIncoming();
			for(StringIter k = incoming.begin(); k != incoming.end(); ++k) {
				if(stricmp(j->second, *k) == 0) {
					ctrlDirectories.SetCheckState(i, true);
				}
			}

		}

	}

	ctrlTotal.SetWindowText(Util::formatBytesW(ShareManager::getInstance()->getShareSize()).c_str());

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
	
	ft.SubclassWindow(GetDlgItem(IDC_TREE1));
	ft.SetStaticCtrl(&ctrlTotal);
	if(!BOOLSETTING(USE_OLD_SHARING_UI))
		ft.PopulateTree();

	fixControls();

	return TRUE;
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

void SharePage::write()
{

	PropPage::write((HWND)*this, items);
	
	if(BOOLSETTING(REFRESH_VNAME_ON_SHAREPAGE) && !BOOLSETTING(DISABLE_REFRESH_ON_SHAREPAGE) && !RefreshDirs.empty()){
	ShareManager::getInstance()->refreshDirs(RefreshDirs);
	}

	if(!BOOLSETTING(DISABLE_REFRESH_ON_SHAREPAGE) && !BOOLSETTING(REFRESH_VNAME_ON_SHAREPAGE)) {
	if(!BOOLSETTING(USE_OLD_SHARING_UI) && ft.IsDirty()) {
		ShareManager::getInstance()->setDirty();
		ShareManager::getInstance()->refresh(ShareManager::REFRESH_ALL | ShareManager::REFRESH_UPDATE);
	} else {
		ShareManager::getInstance()->refresh(ShareManager::REFRESH_ALL | ShareManager::REFRESH_UPDATE);
	}
	
	}
	ShareManager::getInstance()->DelIncoming();
	int size = ctrlDirectories.GetItemCount();
	TCHAR buf[MAX_PATH];
	for(int i = 0; i < size; ++i){
			ctrlDirectories.GetItemText(i, 1, buf, MAX_PATH);
			string tmp = Text::fromT(buf);
		ShareManager::getInstance()->setIncoming(tmp, ctrlDirectories.GetCheckState(i) ? true : false);
		}

	RefreshDirs.clear();

}

LRESULT SharePage::onItemchangedDirectories(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
	NM_LISTVIEW* lv = (NM_LISTVIEW*) pnmh;
	::EnableWindow(GetDlgItem(IDC_REMOVE), (lv->uNewState & LVIS_FOCUSED));
	::EnableWindow(GetDlgItem(IDC_RENAME), (lv->uNewState & LVIS_FOCUSED));
	return 0;		
}

LRESULT SharePage::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
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

LRESULT SharePage::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

	if(item->iItem >= 0) {
		PostMessage(WM_COMMAND, IDC_RENAME, 0);
	} else if(item->iItem == -1) {
		PostMessage(WM_COMMAND, IDC_ADD, 0);
	}

	return 0;
}

LRESULT SharePage::onClickedAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	tstring target;
	if(WinUtil::browseDirectory(target, m_hWnd)) {
		addDirectory(target);
		HashProgressDlg(true).DoModal();
	}
	
	return 0;
}

LRESULT SharePage::onClickedRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR buf[MAX_PATH];
	LVITEM item;
	memzero(&item, sizeof(item));
	item.mask = LVIF_TEXT;
	item.cchTextMax = sizeof(buf);
	item.pszText = buf;

	int i = -1;
	while((i = ctrlDirectories.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		item.iItem = i;
		item.iSubItem = 1;
		ctrlDirectories.GetItem(&item);
		ShareManager::getInstance()->removeDirectory(Text::fromT(buf));
		ctrlTotal.SetWindowText(Util::formatBytesW(ShareManager::getInstance()->getShareSize()).c_str());
		ctrlDirectories.DeleteItem(i);
		
	}
	
	return 0;
}

LRESULT SharePage::onClickedRename(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR buf[MAX_PATH];
	LVITEM item;
	memzero(&item, sizeof(item));
	item.mask = LVIF_TEXT;
	item.cchTextMax = sizeof(buf);
	item.pszText = buf;

	bool setDirty = false;

	int i = -1;
	while((i = ctrlDirectories.GetNextItem(i, LVNI_SELECTED)) != -1) {
		item.iItem = i;
		item.iSubItem = 0;
		ctrlDirectories.GetItem(&item);
		tstring vName = buf;
		item.iSubItem = 1;
		ctrlDirectories.GetItem(&item);
		tstring rPath = buf;
		try {
			LineDlg virt;
			virt.title = TSTRING(VIRTUAL_NAME);
			virt.description = TSTRING(VIRTUAL_NAME_LONG);
			virt.line = vName;
			if(virt.DoModal(m_hWnd) == IDOK) {
				if (stricmp(buf, virt.line) != 0) {
					ShareManager::getInstance()->renameDirectory(Text::fromT(rPath), Text::fromT(virt.line));
					ctrlDirectories.SetItemText(i, 0, virt.line.c_str());

					setDirty = true;
					RefreshDirs.push_back(Text::fromT(virt.line));
				} else {
					MessageBox(CTSTRING(SKIP_RENAME), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONINFORMATION | MB_OK);
				}
			}
		} catch(const ShareException& e) {
			MessageBox(Text::toT(e.getError()).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
		}
	}

	if( setDirty )
		ShareManager::getInstance()->setDirty();

	return 0;
}

LRESULT SharePage::onClickedShareHidden(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// Save the checkbox state so that ShareManager knows to include/exclude hidden files
	Item i = items[1]; // The checkbox. Explicit index used - bad!
	if(::IsDlgButtonChecked((HWND)* this, i.itemID) == BST_CHECKED){
		settings->set((SettingsManager::IntSetting)i.setting, true);
	} else {
		settings->set((SettingsManager::IntSetting)i.setting, false);
	}

	// Refresh the share. 
	ShareManager::getInstance()->setDirty();
	ShareManager::getInstance()->refresh(ShareManager::REFRESH_ALL);

	if(BOOLSETTING(USE_OLD_SHARING_UI))	{
		// Clear the GUI list, for insertion of updated shares
		ctrlDirectories.DeleteAllItems();
		StringPairList directories = ShareManager::getInstance()->getDirectories(ShareManager::REFRESH_ALL);
		for(StringPairIter j = directories.begin(); j != directories.end(); j++)
		{
			int i = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), Text::toT(j->first));
			ctrlDirectories.SetItemText(i, 1, Text::toT(j->second).c_str() );
			ctrlDirectories.SetItemText(i, 2, Util::formatBytesW(ShareManager::getInstance()->getShareSize(j->second)).c_str());
		}
	}

	// Display the new total share size
	ctrlTotal.SetWindowText(Util::formatBytesW(ShareManager::getInstance()->getShareSize()).c_str());
	return 0;
}

LRESULT SharePage::onClickedShareSFV(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// Save the checkbox state so that ShareManager knows to include/exclude hidden files
	Item i = items[1]; // The checkbox. Explicit index used - bad!
	if(::IsDlgButtonChecked((HWND)* this, i.itemID) == BST_CHECKED){
		settings->set((SettingsManager::IntSetting)i.setting, true);
	} else {
		settings->set((SettingsManager::IntSetting)i.setting, false);
	}
	return 0;
}


void SharePage::addDirectory(const tstring& aPath){
	tstring path = aPath;
	if( path[ path.length() -1 ] != _T('\\') )
		path += _T('\\');

	try {
		LineDlg virt;
		virt.title = TSTRING(VIRTUAL_NAME);
		virt.description = TSTRING(VIRTUAL_NAME_LONG);
		virt.line = Text::toT(ShareManager::getInstance()->validateVirtual(
			Util::getLastDir(Text::fromT(path))));
		if(virt.DoModal(m_hWnd) == IDOK) {
			ShareManager::getInstance()->addDirectory(Text::fromT(path), Text::fromT(virt.line));
			int i = ctrlDirectories.insert(ctrlDirectories.GetItemCount(), virt.line );
			ctrlDirectories.SetItemText(i, 1, path.c_str());
			ctrlDirectories.SetItemText(i, 2, Util::formatBytesW(ShareManager::getInstance()->getShareSize(Text::fromT(path))).c_str());
			ctrlTotal.SetWindowText(Util::formatBytesW(ShareManager::getInstance()->getShareSize()).c_str());
			RefreshDirs.push_back(Text::fromT(virt.line));
		}
	} catch(const ShareException& e) {
		MessageBox(Text::toT(e.getError()).c_str(), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONSTOP | MB_OK);
	}
}
void SharePage::fixControls(){
	
	BOOL use = IsDlgButtonChecked(IDC_REFRESH_ON_SHAREPAGE) == BST_UNCHECKED;
	
	::EnableWindow(GetDlgItem(IDC_REFRESH_VNAME_ON_SHAREPAGE),					use);
	}


LRESULT SharePage::onClickedRefreshDisable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	
	fixControls();

return 0;
}



/**
 * @file
 * $Id: SharePage.cpp 389 2008-06-08 10:51:15Z BigMuscle $
 */
