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
#include "FulPopupsPage.h"
#include "WinUtil.h"

PropPage::TextItem FulPopupsPage::texts[] = {
	{ IDC_POPUP_APPEARANCE,			ResourceManager::POPUP_APPEARANCE			},
	{ IDC_ST_DISPLAYTIME,			ResourceManager::SETTINGS_ST_DISPLAYTIME	},
	{ IDC_ST_MESSAGE_LENGTH,		ResourceManager::SETTINGS_ST_MESSAGE_LENGTH	},
	{ IDC_BTN_FONT,					ResourceManager::SETTINGS_BTN_FONT			},
	{ IDC_BTN_TEXTCOLOR,			ResourceManager::SETTINGS_BTN_TEXTCOLOR		},
	{ 0,							ResourceManager::SETTINGS_AUTO_AWAY			}
};

PropPage::Item FulPopupsPage::items[] = {
	{ IDC_DISPLAYTIME,				SettingsManager::POPUP_TIMEOUT,				PropPage::T_INT  },
	{ IDC_MESSAGE_LENGTH,			SettingsManager::MAX_MSG_LENGTH,			PropPage::T_INT  },
	{ 0,							0,											PropPage::T_END	 }
};

PropPage::ListItem FulPopupsPage::listItems[] = {
	{ SettingsManager::POPUP_HUB_DISCONNECTED, ResourceManager::POPUP_HUB_DISCONNECTED },
	{ SettingsManager::POPUP_FAVORITE_CONNECTED, ResourceManager::POPUP_FAVORITE_CONNECTED },
	{ SettingsManager::POPUP_CHEATING_USER, ResourceManager::POPUP_CHEATING_USER },
	{ SettingsManager::POPUP_DOWNLOAD_START, ResourceManager::POPUP_DOWNLOAD_START },
	{ SettingsManager::POPUP_DOWNLOAD_FAILED, ResourceManager::POPUP_DOWNLOAD_FAILED },
	{ SettingsManager::POPUP_DOWNLOAD_FINISHED, ResourceManager::POPUP_DOWNLOAD_FINISHED },
	{ SettingsManager::POPUP_UPLOAD_FINISHED, ResourceManager::POPUP_UPLOAD_FINISHED },
	{ SettingsManager::POPUP_AWAY,					ResourceManager::SHOW_POPUP_AWAY			},
	{ SettingsManager::POPUP_MINIMIZED,				ResourceManager::SHOW_POPUP_MINIMIZED		},
	{ SettingsManager::POPUP_PM,					ResourceManager::POPUP_PM					},
	{ SettingsManager::POPUP_NEW_PM,				ResourceManager::POPUP_NEW_PM				},
	{ SettingsManager::REMOVE_POPUPS,				ResourceManager::REMOVE_POPUPS				},
	{ SettingsManager::POPUP_ACTIVATE_ON_CLICK,		ResourceManager::POPUP_ACTIVATE_ON_CLICK	},
	{ SettingsManager::POPUP_DONT_SHOW_ON_ACTIVE,	ResourceManager::POPUP_DONT_SHOW_ON_ACTIVE	},
	{ 0,											ResourceManager::SETTINGS_AUTO_AWAY			}
};


LRESULT FulPopupsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_POPUP_ITEMS));
	PropPage::translate((HWND)(*this), texts);

	return TRUE;
}

LRESULT FulPopupsPage::onTextColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CColorDialog dlg(SETTING(POPUP_TEXTCOLOR), CC_FULLOPEN);
	if(dlg.DoModal() == IDOK)
		SettingsManager::getInstance()->set(SettingsManager::POPUP_TEXTCOLOR, (int)dlg.GetColor());
	return 0;
}

LRESULT FulPopupsPage::onFont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	LOGFONT font;  
	WinUtil::decodeFont(Text::toT(SETTING(POPUP_FONT)), font);
	CFontDialog dlg(&font, CF_EFFECTS | CF_SCREENFONTS);
	if(dlg.DoModal() == IDOK){
		settings->set(SettingsManager::POPUP_FONT, Text::fromT(WinUtil::encodeFont(font)));
	}
	return 0;
}

LRESULT FulPopupsPage::onHelp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	//HtmlHelp(m_hWnd, WinUtil::getHelpFile().c_str(), HH_HELP_CONTEXT, IDD_FULPOPUPPAGE);
	return 0;
}

LRESULT FulPopupsPage::onHelpInfo(LPNMHDR /*pnmh*/) {
	//HtmlHelp(m_hWnd, WinUtil::getHelpFile().c_str(), HH_HELP_CONTEXT, IDD_FULPOPUPPAGE);
	return 0;
}

void FulPopupsPage::write() {
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_POPUP_ITEMS));
}