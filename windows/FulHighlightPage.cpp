/* 
* Copyright (C) 2003-2005 Pär Björklund, per.bjorklund@gmail.com
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

#include "../client/SettingsManager.h"
#include "../client/ColorSettings.h"
#include "../client/HighlightManager.h"
#include "../client/StringTokenizer.h"
#include "../client/version.h"
#include "Resource.h"
#include "FulHighlightPage.h"
#include "FulHighlightDialog.h"
#include "WinUtil.h"
#include "PropertiesDlg.h"

PropPage::TextItem FulHighlightPage::texts[] = {
	{ IDC_ADD,			 ResourceManager::HIGHLIGHT_ADD				},
	{ IDC_DELETE,		 ResourceManager::REMOVE					},
	{ IDC_UPDATE,		 ResourceManager::HIGHLIGHT_CHANGE			},
	{ IDC_MOVEUP,		 ResourceManager::SETTINGS_BTN_MOVEUP		},
	{ IDC_MOVEDOWN,		 ResourceManager::SETTINGS_BTN_MOVEDOWN		},
	{ IDC_USE_HIGHLIGHT, ResourceManager::USE_HIGHLIGHT				},
	{ IDC_PRESET,		 ResourceManager::PRESET					},
	{ 0,				 ResourceManager::SETTINGS_AUTO_AWAY		}
};

PropPage::Item FulHighlightPage::items[] = {
	{ IDC_USE_HIGHLIGHT, SettingsManager::USE_HIGHLIGHT, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};




LRESULT FulHighlightPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);
	CRect rc;
	
	//Initalize listview
	ctrlStrings.Attach(GetDlgItem(IDC_ITEMS));
	ctrlStrings.GetClientRect(rc);
	ctrlStrings.InsertColumn(0, CTSTRING(HIGHLIGHTLIST_HEADER), LVCFMT_LEFT, rc.Width(), 0);
	ctrlStrings.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	ColorList* cList = HighlightManager::getInstance()->getList();
		
	//populate listview with current strings
	highlights.reserve(cList->size());
	for(ColorIter i = cList->begin();i != cList->end(); ++i) {
		highlights.push_back((*i));
		ctrlStrings.insert( ctrlStrings.GetItemCount(), (*i).getMatch());
	}
	
	presets.CreatePopupMenu();
	presets.AppendMenu(MF_STRING, IDC_PRESETMENU, CTSTRING(PRESET_ADD_ALL));
	presets.AppendMenu(MF_SEPARATOR);
	presets.AppendMenu(MF_STRING, IDC_PRESETMENU, CTSTRING(PRESET_JOINS));
	presets.AppendMenu(MF_STRING, IDC_PRESETMENU, CTSTRING(PRESET_PARTS));
	presets.AppendMenu(MF_STRING, IDC_PRESETMENU, CTSTRING(PRESET_RELEASES));
	presets.AppendMenu(MF_STRING, IDC_PRESETMENU, CTSTRING(PRESET_USERS));

	MENUINFO inf;
	inf.cbSize = sizeof(MENUINFO);
	inf.fMask = MIM_STYLE;
	inf.dwStyle = MNS_NOTIFYBYPOS;
	presets.SetMenuInfo(&inf);

	fixControls();
	return TRUE;
}

void FulHighlightPage::write(){
	
	PropPage::write((HWND)*this, items);
	HighlightManager::getInstance()->replaceList(highlights);

}

LRESULT FulHighlightPage::onAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	FulHighlightDialog dlg;
	if(dlg.DoModal(WinUtil::mainWnd) == IDOK) {
		//add the string to the listview
		highlights.push_back(dlg.getColorSetting());
		ctrlStrings.insert( ctrlStrings.GetItemCount(), highlights.back().getMatch());
		ctrlStrings.SelectItem(ctrlStrings.GetItemCount()-1);
	}

	return TRUE;
}

LRESULT FulHighlightPage::onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int sel = ctrlStrings.GetSelectedIndex();
	if(sel == -1)
		return TRUE;
	
	FulHighlightDialog dlg(highlights[sel]);
	if(dlg.DoModal(WinUtil::mainWnd) == IDOK) {
		ColorSettings old, cur;
		old = highlights[sel];
		cur = dlg.getColorSetting();

		if(old.getMatch().compare(cur.getMatch()) != 0){
				ctrlStrings.DeleteItem(sel);
			ctrlStrings.insert(sel, cur.getMatch());
			ctrlStrings.SelectItem(sel);
		} 
		highlights[sel] = cur;
	}
	return TRUE;
}

LRESULT FulHighlightPage::onMove(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int sel = ctrlStrings.GetSelectedIndex();
	if(wID == IDC_MOVEUP && sel > 0){
		ColorSettings cs = highlights[sel];
		highlights[sel] = highlights[sel-1];
		highlights[sel-1] = cs;
		ctrlStrings.DeleteItem(sel);
		ctrlStrings.insert(sel-1, cs.getMatch());
		ctrlStrings.SelectItem(sel-1);
	} else if(wID == IDC_MOVEDOWN && sel < ctrlStrings.GetItemCount()-1){
		//hmm odd, moveItem handles the move but the list doesn't get updated
		//so well this works instead =)
		ColorSettings cs = highlights[sel];
		highlights[sel] = highlights[sel+1];
		highlights[sel+1] = cs;
		ctrlStrings.DeleteItem(sel);
		ctrlStrings.insert(sel+1, cs.getMatch());
		ctrlStrings.SelectItem(sel+1);
	}

	return 0;
}

LRESULT FulHighlightPage::onDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	if(ctrlStrings.GetSelectedCount() == 1) {
		int sel = ctrlStrings.GetSelectedIndex();
		ctrlStrings.DeleteItem(sel);
		
		if(sel > 0 && ctrlStrings.GetItemCount() > 0) {
			ctrlStrings.SelectItem(sel-1);
		} else if(ctrlStrings.GetItemCount() > 0) {
			ctrlStrings.SelectItem(0);
		}

		int j = 0;
		ColorIter i = highlights.begin();
		for(; j < sel; ++i, ++j);

		if(i != highlights.end())
			highlights.erase(i);
	}
	
	return TRUE;
}

FulHighlightPage::~FulHighlightPage() {
	ctrlStrings.Detach();
}

LRESULT FulHighlightPage::onEnable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

void FulHighlightPage::fixControls() {
	BOOL use = IsDlgButtonChecked(IDC_USE_HIGHLIGHT) == BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_ADD),					use);
	::EnableWindow(GetDlgItem(IDC_DELETE),				use);
	::EnableWindow(GetDlgItem(IDC_UPDATE),				use);
	::EnableWindow(GetDlgItem(IDC_MOVEUP),				use);
	::EnableWindow(GetDlgItem(IDC_MOVEDOWN),			use);
	::EnableWindow(GetDlgItem(IDC_ITEMS),				use);
	::EnableWindow(GetDlgItem(IDC_PRESET),				use);
}

LRESULT FulHighlightPage::onPreset(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	POINT pt;
	if(GetCursorPos(&pt) != 0) {
		presets.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_HORPOSANIMATION | TPM_VERPOSANIMATION,
			pt.x, pt.y, m_hWnd);
	}
	return 0;
}

LRESULT FulHighlightPage::onMenuCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if(wParam == 0) {
		for(int i = 2; i < 6; ++i) {
			addPreset(i);
		}
	} else {
		addPreset(wParam);
	}
	return 0;
}

void FulHighlightPage::addPreset(int preset) {
	ColorSettings cs;
	switch(preset){
			case 2:
			cs.setMatch(_T("$Re:^\\[.*?\\] (\\*{3} Joins: .*)"));
			cs.setHasFgColor(true);
			cs.setBold(true);
			cs.setFgColor(RGB(153,153,51));
			break;
		case 3:
			cs.setMatch(_T("$Re:^\\[.*?\\] (\\*{3} Parts: .*)"));
			cs.setHasFgColor(true);
			cs.setBold(true);
			cs.setFgColor(RGB(51, 102, 154));
			break;
		case 4:
			cs.setMatch(_T("$Re:(((?<=\s)[A-Za-z0-9-]+(\.|_)\S+[-]\w+(\.[A-Za-z0-9]{2,4})?)|((?<=\s)\S+\.nfo)|(\S+[-]\S+\.(rar|r\d{2}|\d{3}))(?=(\W)?\s))"));
			cs.setHasFgColor(true);
			cs.setFgColor(RGB(153, 51, 153));
			break;
		case 5:
			cs.setMatch(_T("$users$"));
			cs.setIncludeNick(true);
			cs.setBold(true);
			cs.setHasFgColor(true);
			cs.setFgColor(RGB(115, 115, 118));
			break;
		default:
			break;
	}

	highlights.push_back(cs);
	ctrlStrings.insert( ctrlStrings.GetItemCount(), highlights.back().getMatch());
	ctrlStrings.SelectItem(ctrlStrings.GetItemCount()-1);
}

LRESULT FulHighlightPage::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
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

LRESULT FulHighlightPage::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

	if(item->iItem >= 0) {
		PostMessage(WM_COMMAND, IDC_UPDATE, 0);
	} else if(item->iItem == -1) {
		PostMessage(WM_COMMAND, IDC_ADD, 0);
	}

	return 0;
}