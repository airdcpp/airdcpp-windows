/* 
 * Copyright (C) 2003 Twink,  spm7@waikato.ac.nz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include <windows/stdafx.h>
#include <airdcpp/settings/SettingsManager.h>
#include <airdcpp/util/text/StringTokenizer.h>
#include <airdcpp/util/Util.h>

#include <windows/BrowseDlg.h>
#include <windows/Resource.h>
#include <windows/settings/ToolbarPage.h>
#include <windows/WinUtil.h>
#include <windows/MainFrm.h>

namespace wingui {
PropPage::TextItem ToolbarPage::texts[] = {
	{ IDC_MOUSE_OVER, ResourceManager::SETTINGS_MOUSE_OVER },
	{ IDC_IMAGEBROWSE, ResourceManager::BROWSE },
	{ IDC_HOTBROWSE, ResourceManager::BROWSE },
	{ IDC_NORMAL, ResourceManager::SETTINGS_NORMAL },
	{ IDC_TOOLBAR_IMAGE_BOX, ResourceManager::SETTINGS_TOOLBAR_IMAGE },
	{ IDC_TOOLBAR_ADD, ResourceManager::SETTINGS_TOOLBAR_ADD },
	{ IDC_TOOLBAR_REMOVE, ResourceManager::SETTINGS_TOOLBAR_REMOVE },
	{ IDC_TB_SIZE, ResourceManager::SETTINGS_TOOLBAR_SIZE },
	{ IDC_WTB_SIZE, ResourceManager::SETTINGS_WTOOLBAR_SIZE },
	{ IDC_TB_NOTE, ResourceManager::SETTINGS_TOOLBAR_NOTE },
	{ 0, ResourceManager::LAST }
};

PropPage::Item ToolbarPage::items[] = {
	{ IDC_ICON_SIZE, SettingsManager::TB_IMAGE_SIZE, PropPage::T_INT },
	{ IDC_WTB_ICON_SIZE, SettingsManager::WTB_IMAGE_SIZE, PropPage::T_INT },
	//{ IDC_ICON_SIZE_HOVER, SettingsManager::TB_IMAGE_SIZE_HOT, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};


string ToolbarPage::filter(string s){
//	s = Util::replace(s, "&","");
	s = s.substr(0,s.find("\t"));
	return s;
}

LRESULT ToolbarPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);
	
	ctrlCommands.Attach(GetDlgItem(IDC_TOOLBAR_POSSIBLE));
	CRect rc;
	ctrlCommands.GetClientRect(rc);
	ctrlCommands.InsertColumn(0, _T("Dummy"), LVCFMT_LEFT, rc.Width(), 0);

	ctrlCommands.SetImageList(MainFrame::getMainFrame()->ToolbarImages, LVSIL_SMALL);
		

	LVITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE;
	lvi.iSubItem = 0;

	for(int i = -1; i < static_cast<int>(sizeof(ToolbarButtons) / sizeof(ToolbarButtons[0])); i++) {
		makeItem(&lvi, i);
		lvi.iItem = i+1;
		ctrlCommands.InsertItem(&lvi);
		ctrlCommands.SetItemData(lvi.iItem, i);
	}
	ctrlCommands.SetColumnWidth(0, LVSCW_AUTOSIZE);

	ctrlToolbar.Attach(GetDlgItem(IDC_TOOLBAR_ACTUAL));
	ctrlToolbar.GetClientRect(rc);
	ctrlToolbar.InsertColumn(0, _T("Dummy"), LVCFMT_LEFT, rc.Width(), 0);
	ctrlToolbar.SetImageList(MainFrame::getMainFrame()->ToolbarImages, LVSIL_SMALL);
	
	StringTokenizer<string> t(SETTING(TOOLBAR_ORDER), ',');
	StringList& l = t.getTokens();

	int n = 0;
	for(StringList::const_iterator k = l.begin(); k != l.end(); ++k) {
		int i = Util::toInt(*k);
		makeItem(&lvi, i);
		lvi.iItem = n++;
		ctrlToolbar.InsertItem(&lvi);
		ctrlToolbar.SetItemData(lvi.iItem, i);
	}

	ctrlToolbar.SetColumnWidth(0, LVSCW_AUTOSIZE);

	return TRUE;
}

void ToolbarPage::write()
{
	PropPage::write((HWND)*this, items);
	string toolbar;
	for(int i = 0; i < ctrlToolbar.GetItemCount(); i++) {
		if(i!=0)toolbar+=",";
		int j = ctrlToolbar.GetItemData(i);
		toolbar += Util::toString(j);
	}
	if(toolbar != settings->get(SettingsManager::TOOLBAR_ORDER)) {
	settings->set(SettingsManager::TOOLBAR_ORDER, toolbar);
		::SendMessage(WinUtil::mainWnd, IDC_REBUILD_TOOLBAR, 0, 0);
	}
}

void ToolbarPage::BrowseForPic(int DLGITEM) {
	TCHAR buf[MAX_PATH];

	GetDlgItemText(DLGITEM, buf, MAX_PATH);
	tstring x = buf;

	BrowseDlg dlg(m_hWnd, BrowseDlg::TYPE_SETTINGS_RESOURCES, BrowseDlg::DIALOG_SELECT_FILE);
	dlg.setPath(x, true);
	if (dlg.show(x)) {
		SetDlgItemText(DLGITEM, x.c_str());
	}
}

void ToolbarPage::makeItem(LPLVITEM lvi, int item){
	if((item >= 0) && (item < sizeof(ToolbarButtons) / sizeof(ToolbarButtons[0]))) {
		lvi->iImage = ToolbarButtons[item].image;
		name = Text::toT(filter(ResourceManager::getInstance()->getString(ToolbarButtons[item].tooltip)));			
	} else {
		name = TSTRING(SEPARATOR);
		lvi->iImage = -1;
	}	
	lvi->pszText = const_cast<TCHAR*>(name.c_str());	
}

LRESULT ToolbarPage::onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlCommands.GetSelectedCount() == 1) {				
		LVITEM lvi;
		lvi.mask = LVIF_TEXT | LVIF_IMAGE;
		lvi.iSubItem = 0;
		int i = ctrlCommands.GetItemData(ctrlCommands.GetSelectedIndex());
		makeItem(&lvi, i);
		lvi.iItem = ctrlToolbar.GetSelectedIndex() + 1;//ctrlToolbar.GetSelectedIndex()>0?ctrlToolbar.GetSelectedIndex():ctrlToolbar.GetItemCount();
		ctrlToolbar.InsertItem(&lvi);
		ctrlToolbar.SetItemData(lvi.iItem, i);
	}
	return 0;
}

LRESULT ToolbarPage::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlToolbar.GetSelectedCount() == 1) {				
		int sel = ctrlToolbar.GetSelectedIndex();
		ctrlToolbar.DeleteItem(sel);
		ctrlToolbar.SelectItem(max(sel-1,0));
	}
	return 0;
}
}