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
#include "Resource.h"


#include "FulTabsPage.h"
#include "WinUtil.h"


PropPage::TextItem FulTabsPage::texts[] = {
	{ IDC_CH_STATUS_HUB_BOLD,		ResourceManager::HUB_BOLD_TABS				},
	{ IDC_CH_ICONS,					ResourceManager::TAB_SHOW_ICONS				},
	{ IDC_CH_BLEND,					ResourceManager::BLEND_TABS					},
	{ IDC_SB_TAB_COLORS,			ResourceManager::SETTINGS_SB_TAB_COLORS		},
	{ IDC_SB_TAB_SIZE,				ResourceManager::SETTINGS_SB_TAB_SIZE		},
	{ IDC_BTN_COLOR,				ResourceManager::SETTINGS_BTN_COLOR			},
	{ IDC_SB_TAB_DIRTY_BLEND,		ResourceManager::SETTINGS_SB_DIRTY_BLEND	},
	{ IDC_SETTINGS_BOLD_CONTENTS,	ResourceManager::SETTINGS_BOLD_OPTIONS		},
	{ IDC_SETTINGS_MAX_TAB_ROWS,	ResourceManager::SETTINGS_MAX_TAB_ROWS		},
	{ IDC_BOLD_HUB_TABS_ON_KICK,	ResourceManager::BOLD_HUB_TABS_ON_KICK		},
	{ 0,							ResourceManager::SETTINGS_AUTO_AWAY			}
};

PropPage::Item FulTabsPage::items[] = {
	{ IDC_CH_STATUS_HUB_BOLD,		SettingsManager::HUB_BOLD_TABS,				PropPage::T_BOOL },
	{ IDC_CH_ICONS,					SettingsManager::TAB_SHOW_ICONS,			PropPage::T_BOOL },
	{ IDC_CH_BLEND,					SettingsManager::BLEND_TABS,				PropPage::T_BOOL },
	{ IDC_TAB_SIZE,					SettingsManager::TAB_SIZE,					PropPage::T_INT }, 
	{ IDC_TAB_DIRTY_BLEND,			SettingsManager::TAB_DIRTY_BLEND,			PropPage::T_INT },
	{ IDC_MAX_TAB_ROWS,				SettingsManager::MAX_TAB_ROWS,				PropPage::T_INT },
	{ IDC_BOLD_HUB_TABS_ON_KICK,	SettingsManager::BOLD_HUB_TABS_ON_KICK,		PropPage::T_BOOL },
	{ 0,							0,											PropPage::T_END }
};

PropPage::ListItem FulTabsPage::listItems[] = {
	{ SettingsManager::BOLD_FINISHED_DOWNLOADS, ResourceManager::FINISHED_DOWNLOADS },
	{ SettingsManager::BOLD_FINISHED_UPLOADS, ResourceManager::FINISHED_UPLOADS },
	{ SettingsManager::BOLD_QUEUE, ResourceManager::DOWNLOAD_QUEUE },
	{ SettingsManager::BOLD_HUB, ResourceManager::HUB },
	{ SettingsManager::BOLD_PM, ResourceManager::PRIVATE_MESSAGE },
	{ SettingsManager::BOLD_SEARCH, ResourceManager::SEARCH },
	{ SettingsManager::BOLD_WAITING_USERS, ResourceManager::WAITING_USERS },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT FulTabsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::read((HWND)*this, items, listItems,GetDlgItem(IDC_BOLD_BOOLEANS));
	PropPage::translate((HWND)(*this), texts);

	colorList.Attach(GetDlgItem(IDC_COLOR_COMBO));

	colorList.AddString(CTSTRING(TAB_ACTIVE_BG));
	colorList.AddString(CTSTRING(TAB_ACTIVE_TEXT));
	colorList.AddString(CTSTRING(TAB_ACTIVE_BORDER));
	colorList.AddString(CTSTRING(TAB_INACTIVE_BG));
	colorList.AddString(CTSTRING(TAB_INACTIVE_TEXT));
	colorList.AddString(CTSTRING(TAB_INACTIVE_BORDER));
	colorList.AddString(CTSTRING(TAB_INACTIVE_BG_NOTIFY));
	colorList.AddString(CTSTRING(TAB_INACTIVE_BG_DISCONNECTED));
	colorList.SetCurSel(0);

	colorList.Detach();

	BOOL b;

	onClickedBox(0, IDC_CH_BLEND, 0, b );

	return TRUE;
}

void FulTabsPage::write() {
	PropPage::write((HWND)*this, items, listItems,GetDlgItem(IDC_BOLD_BOOLEANS));
}

LRESULT FulTabsPage::onColorButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	colorList.Attach(GetDlgItem(IDC_COLOR_COMBO));
	int sel = colorList.GetCurSel();
	COLORREF col;
	switch(sel){
		case 0: col = SETTING(TAB_ACTIVE_BG); break;
		case 1: col = SETTING(TAB_ACTIVE_TEXT); break;
		case 2: col = SETTING(TAB_ACTIVE_BORDER); break;
		case 3: col = SETTING(TAB_INACTIVE_BG); break;
		case 4: col = SETTING(TAB_INACTIVE_TEXT); break;
		case 5: col = SETTING(TAB_INACTIVE_BORDER); break;
		case 6: col = SETTING(TAB_INACTIVE_BG_NOTIFY); break;
		case 7: col = SETTING(TAB_INACTIVE_BG_DISCONNECTED); break;
		default: col = RGB(0, 0, 0); break;
	}
	
	CColorDialog d(col, CC_FULLOPEN);
	if(d.DoModal() == IDOK){
		switch(sel){
		case 0: settings->set(SettingsManager::TAB_ACTIVE_BG, (int)d.GetColor()); break;
		case 1: settings->set(SettingsManager::TAB_ACTIVE_TEXT, (int)d.GetColor()); break;
		case 2: settings->set(SettingsManager::TAB_ACTIVE_BORDER, (int)d.GetColor()); break;
		case 3: settings->set(SettingsManager::TAB_INACTIVE_BG, (int)d.GetColor()); break;
		case 4: settings->set(SettingsManager::TAB_INACTIVE_TEXT, (int)d.GetColor()); break;
		case 5: settings->set(SettingsManager::TAB_INACTIVE_BORDER, (int)d.GetColor()); break;
		case 6: settings->set(SettingsManager::TAB_INACTIVE_BG_NOTIFY, (int)d.GetColor()); break;
		case 7: settings->set(SettingsManager::TAB_INACTIVE_BG_DISCONNECTED, (int)d.GetColor()); break;
		}
	}

	colorList.Detach();
	return 0;
}

LRESULT FulTabsPage::onClickedBox(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
	int button = 0;
	
	//this seems utterly stupid now, but it's easier if i want/need to
	//add more items in the future =)
	switch(wID) {
	case IDC_CH_BLEND:		button = IDC_TAB_DIRTY_BLEND; break;
	}

	::EnableWindow( GetDlgItem(button), IsDlgButtonChecked(wID) == BST_CHECKED );

	return TRUE;
}

LRESULT FulTabsPage::onHelp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
//	HtmlHelp(m_hWnd, WinUtil::getHelpFile().c_str(), HH_HELP_CONTEXT, IDD_FULTABSPAGE);
	return 0;
}

LRESULT FulTabsPage::onHelpInfo(LPNMHDR /*pnmh*/) {
//	HtmlHelp(m_hWnd, WinUtil::getHelpFile().c_str(), HH_HELP_CONTEXT, IDD_FULTABSPAGE);
	return 0;
}