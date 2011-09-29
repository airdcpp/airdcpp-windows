/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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

#include "Popups.h"
#include "WinUtil.h"
#include "MainFrm.h"

PropPage::TextItem Popups::texts[] = {
	{ IDC_POPUPGROUP, ResourceManager::BALLOON_POPUPS },
	{ IDC_PREVIEW, ResourceManager::SETCZDC_PREVIEW },
	{ IDC_POPUPTYPE, ResourceManager::POPUP_TYPE },
	{ IDC_POPUP_TIME_STR, ResourceManager::POPUP_TIME },
	{ IDC_POPUP_BACKCOLOR, ResourceManager::POPUP_BACK_COLOR },
	{ IDC_POPUP_FONT, ResourceManager::POPUP_FONT },
	{ IDC_POPUP_TITLE_FONT, ResourceManager::POPUP_TITLE_FONT },
	{ IDC_MAX_MSG_LENGTH_STR, ResourceManager::MAX_MSG_LENGTH },
	{ IDC_POPUP_IMAGE_GP, ResourceManager::POPUP_IMAGE },
	{ IDC_POPUPBROWSE, ResourceManager::BROWSE },
	{ IDC_PREVIEW, ResourceManager::PREVIEW_MENU },
	{ IDC_POPUP_COLORS, ResourceManager::POPUP_COLORS },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item Popups::items[] = {
	{ IDC_POPUP_TIME, SettingsManager::POPUP_TIME, PropPage::T_INT },
	{ IDC_POPUPFILE, SettingsManager::POPUPFILE, PropPage::T_STR },
	{ IDC_MAX_MSG_LENGTH, SettingsManager::MAX_MSG_LENGTH, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

Popups::ListItem Popups::listItems[] = {
	{ SettingsManager::POPUP_HUB_CONNECTED, ResourceManager::POPUP_HUB_CONNECTED },
	{ SettingsManager::POPUP_HUB_DISCONNECTED, ResourceManager::POPUP_HUB_DISCONNECTED },
	{ SettingsManager::POPUP_FAVORITE_CONNECTED, ResourceManager::POPUP_FAVORITE_CONNECTED },
	{ SettingsManager::POPUP_DOWNLOAD_START, ResourceManager::POPUP_DOWNLOAD_START },
	{ SettingsManager::POPUP_DOWNLOAD_FAILED, ResourceManager::POPUP_DOWNLOAD_FAILED },
	{ SettingsManager::POPUP_DOWNLOAD_FINISHED, ResourceManager::POPUP_DOWNLOAD_FINISHED },
	{ SettingsManager::POPUP_UPLOAD_FINISHED, ResourceManager::POPUP_UPLOAD_FINISHED },
	{ SettingsManager::POPUP_PM, ResourceManager::POPUP_PM },
	{ SettingsManager::POPUP_NEW_PM, ResourceManager::POPUP_NEW_PM },
	{ SettingsManager::POPUP_AWAY, ResourceManager::SHOW_POPUP_AWAY },
	{ SettingsManager::POPUP_MINIMIZED, ResourceManager::SHOW_POPUP_MINIMIZED },
	{ SettingsManager::PM_PREVIEW, ResourceManager::PM_PREVIEW },
	{ SettingsManager::POPUP_BUNDLE_DLS, ResourceManager::SETTINGS_BUNDLE_DL_POPUP },
	{ SettingsManager::POPUP_BUNDLE_ULS, ResourceManager::SETTINGS_BUNDLE_UL_POPUP },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT Popups::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_POPUPLIST));

	ctrlPopupType.Attach(GetDlgItem(IDC_POPUP_TYPE));
	ctrlPopupType.AddString(CTSTRING(POPUP_BALOON));
	ctrlPopupType.AddString(CTSTRING(POPUP_CUSTOM));
	ctrlPopupType.AddString(CTSTRING(POPUP_SPLASH));
	ctrlPopupType.AddString(CTSTRING(POPUP_WINDOW));
	ctrlPopupType.SetCurSel(SETTING(POPUP_TYPE));

	CUpDownCtrl spin;
	spin.Attach(GetDlgItem(IDC_POPUP_TIME_SPIN));
	spin.SetBuddy(GetDlgItem(IDC_POPUP_TIME));
	spin.SetRange32(1, 15);
	spin.Detach();

	spin.Attach(GetDlgItem(IDC_MAX_MSG_LENGTH_SPIN));
	spin.SetBuddy(GetDlgItem(IDC_MAX_MSG_LENGTH));
	spin.SetRange32(1, 255);
	spin.Detach();

	SetDlgItemText(IDC_POPUPFILE, Text::toT(SETTING(POPUPFILE)).c_str());

	if(SETTING(POPUP_TYPE) == BALLOON) {
		::EnableWindow(GetDlgItem(IDC_POPUP_BACKCOLOR), false);
		::EnableWindow(GetDlgItem(IDC_POPUP_FONT), false);
		::EnableWindow(GetDlgItem(IDC_POPUP_TITLE_FONT), false);
		::EnableWindow(GetDlgItem(IDC_POPUPFILE), false);
		::EnableWindow(GetDlgItem(IDC_POPUPBROWSE), false);
	} else if(SETTING(POPUP_TYPE) == CUSTOM) {
		::EnableWindow(GetDlgItem(IDC_POPUP_BACKCOLOR), false);
	} else {
		::EnableWindow(GetDlgItem(IDC_POPUPFILE), false);
		::EnableWindow(GetDlgItem(IDC_POPUPBROWSE), false);
	}

	return TRUE;
}

LRESULT Popups::onBackColor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CColorDialog dlg(SETTING(POPUP_BACKCOLOR), CC_FULLOPEN);
	if(dlg.DoModal() == IDOK)
		SettingsManager::getInstance()->set(SettingsManager::POPUP_BACKCOLOR, (int)dlg.GetColor());
	return 0;
}

LRESULT Popups::onFont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	LOGFONT font;  
	WinUtil::decodeFont(Text::toT(SETTING(POPUP_FONT)), font);
	CFontDialog dlg(&font, CF_EFFECTS | CF_SCREENFONTS);
	dlg.m_cf.rgbColors = SETTING(POPUP_TEXTCOLOR);
	if(dlg.DoModal() == IDOK){
		SettingsManager::getInstance()->set(SettingsManager::POPUP_TEXTCOLOR, (int)dlg.GetColor());
		SettingsManager::getInstance()->set(SettingsManager::POPUP_FONT, Text::fromT(WinUtil::encodeFont(font)));
	}
	return 0;
}

LRESULT Popups::onTitleFont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	LOGFONT tmp = myFont;
	WinUtil::decodeFont(Text::toT(SETTING(POPUP_TITLE_FONT)), tmp);
	CFontDialog dlg(&tmp, CF_EFFECTS | CF_SCREENFONTS);
	dlg.m_cf.rgbColors = SETTING(POPUP_TITLE_TEXTCOLOR);
	if(dlg.DoModal() == IDOK){
		myFont = tmp;
		SettingsManager::getInstance()->set(SettingsManager::POPUP_TITLE_TEXTCOLOR, (int)dlg.GetColor());
		SettingsManager::getInstance()->set(SettingsManager::POPUP_TITLE_FONT, Text::fromT(WinUtil::encodeFont(myFont)));
	}
	return 0;
}

LRESULT Popups::onPopupBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TCHAR buf[MAX_PATH];

	GetDlgItemText(IDC_POPUPFILE, buf, MAX_PATH);
	tstring x = buf;

	if(WinUtil::browseFile(x, m_hWnd, false) == IDOK) {
		SetDlgItemText(IDC_POPUPFILE, x.c_str());
		SettingsManager::getInstance()->set(SettingsManager::POPUPFILE, Text::fromT(x));
	}
	return 0;
}

LRESULT Popups::onTypeChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	SettingsManager::getInstance()->set(SettingsManager::POPUP_TYPE, ctrlPopupType.GetCurSel());
	if(ctrlPopupType.GetCurSel() == BALLOON) {
		::EnableWindow(GetDlgItem(IDC_POPUP_BACKCOLOR), false);
		::EnableWindow(GetDlgItem(IDC_POPUP_FONT), false);
		::EnableWindow(GetDlgItem(IDC_POPUP_TITLE_FONT), false);
		::EnableWindow(GetDlgItem(IDC_POPUPFILE), false);
		::EnableWindow(GetDlgItem(IDC_POPUPBROWSE), false);
	} else if(ctrlPopupType.GetCurSel() == CUSTOM) {
		::EnableWindow(GetDlgItem(IDC_POPUP_BACKCOLOR), false);
		::EnableWindow(GetDlgItem(IDC_POPUP_FONT), true);
		::EnableWindow(GetDlgItem(IDC_POPUP_TITLE_FONT), true);
		::EnableWindow(GetDlgItem(IDC_POPUPFILE), true);
		::EnableWindow(GetDlgItem(IDC_POPUPBROWSE), true);
	} else {
		::EnableWindow(GetDlgItem(IDC_POPUP_BACKCOLOR), true);
		::EnableWindow(GetDlgItem(IDC_POPUP_FONT), true);
		::EnableWindow(GetDlgItem(IDC_POPUP_TITLE_FONT), true);
		::EnableWindow(GetDlgItem(IDC_POPUPFILE), false);
		::EnableWindow(GetDlgItem(IDC_POPUPBROWSE), false);
	}
	return 0;
}

void Popups::write()
{
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_POPUPLIST));

	SettingsManager::getInstance()->set(SettingsManager::POPUP_TYPE, ctrlPopupType.GetCurSel());
	
	/*if(SETTING(POPUP_TIME) < 1) {
		settings->set(SettingsManager::POPUP_TIME, 1);
	} else if(SETTING(POPUP_TIME) > 15) {
		settings->set(SettingsManager::POPUP_TIME, 15);
	}

	if(SETTING(MAX_MSG_LENGTH) < 1) {
		settings->set(SettingsManager::MAX_MSG_LENGTH, 1);
	} else if(SETTING(MAX_MSG_LENGTH) > 255) {
		settings->set(SettingsManager::MAX_MSG_LENGTH, 255);
	}*/
	
	// Do specialized writing here
	// settings->set(XX, YY);
}


LRESULT Popups::onPreview(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	PopupManager::getInstance()->Show(TSTRING(FILE) + _T(": air100.rar\n") +
		TSTRING(USER) + _T(": ") + Text::toT(SETTING(NICK)), TSTRING(DOWNLOAD_FINISHED_IDLE), NIIF_INFO, true);

	return 0;
}
/**
 * @file
 * $Id: Popups.cpp 274 2007-01-17 18:03:56Z bigmuscle $
 */

