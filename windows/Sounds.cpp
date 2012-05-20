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

#include "Sounds.h"
#include "WinUtil.h"

PropPage::TextItem Sounds::texts[] = {
	{ IDC_PRIVATE_MESSAGE_BEEP, ResourceManager::SETTINGS_PM_BEEP },
	{ IDC_PRIVATE_MESSAGE_BEEP_OPEN, ResourceManager::SETTINGS_PM_BEEP_OPEN },
	{ IDC_CZDC_PM_SOUND, ResourceManager::SETCZDC_PRIVATE_SOUND },
	{ IDC_BROWSE, ResourceManager::BROWSE },	
	{ IDC_PLAY, ResourceManager::PLAY },
	{ IDC_NONE, ResourceManager::NONE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item Sounds::items[] = {
	{ IDC_PRIVATE_MESSAGE_BEEP, SettingsManager::PRIVATE_MESSAGE_BEEP, PropPage::T_BOOL },
	{ IDC_PRIVATE_MESSAGE_BEEP_OPEN, SettingsManager::PRIVATE_MESSAGE_BEEP_OPEN, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

Sounds::snds Sounds::sounds[] = {
	{ ResourceManager::SOUND_DOWNLOAD_BEGINS,	SettingsManager::BEGINFILE, ""},
	{ ResourceManager::SOUND_DOWNLOAD_FINISHED,	SettingsManager::FINISHFILE, ""},
	{ ResourceManager::SOUND_SOURCE_ADDED,	SettingsManager::SOURCEFILE, ""},
	{ ResourceManager::SOUND_UPLOAD_FINISHED,	SettingsManager::UPLOADFILE, ""},
	{ ResourceManager::SETCZDC_PRIVATE_SOUND,	SettingsManager::BEEPFILE, ""},
	{ ResourceManager::MYNICK_IN_CHAT,	SettingsManager::CHATNAMEFILE, ""},
	{ ResourceManager::SOUND_TTH_INVALID,	SettingsManager::SOUND_TTH, ""},
	{ ResourceManager::SOUND_EXCEPTION,	SettingsManager::SOUND_EXC, ""},
	{ ResourceManager::HUB_CONNECTED,	SettingsManager::SOUND_HUBCON, ""},
	{ ResourceManager::HUB_DISCONNECTED,	SettingsManager::SOUND_HUBDISCON, ""},
	{ ResourceManager::FAVUSER_ONLINE,	SettingsManager::SOUND_FAVUSER, ""},
	{ ResourceManager::SOUND_TYPING_NOTIFY,	SettingsManager::SOUND_TYPING_NOTIFY, ""}
};

LRESULT Sounds::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	ctrlSounds.Attach(GetDlgItem(IDC_SOUNDLIST));

	ctrlSounds.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	ctrlSounds.InsertColumn(0, CTSTRING(SETTINGS_SOUNDS), LVCFMT_LEFT, 172, 0);
	ctrlSounds.InsertColumn(1, CTSTRING(FILENAME), LVCFMT_LEFT, 210, 1);

	// Do specialized reading here

	int j;
	for(int i=0; i < sizeof(sounds) / sizeof(snds); i++){				
		j = ctrlSounds.insert(i, Text::toT(ResourceManager::getInstance()->getString(sounds[i].name)).c_str());
		sounds[i].value = SettingsManager::getInstance()->get((SettingsManager::StrSetting)sounds[i].setting, true);
		ctrlSounds.SetItemText(j, 1, Text::toT(sounds[i].value).c_str());
	}

	return TRUE;
}


void Sounds::write()
{
	PropPage::write((HWND)*this, items);

	TCHAR buf[256];

	for(int i=0; i < sizeof(sounds) / sizeof(snds); i++){
		ctrlSounds.GetItemText(i, 1, buf, 255);
		settings->set((SettingsManager::StrSetting)sounds[i].setting, Text::fromT(buf));
	}

	// Do specialized writing here
	// settings->set(XX, YY);
}

LRESULT Sounds::onBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR buf[MAX_PATH];
	LVITEM item = { 0 };
	item.mask = LVIF_TEXT;
	item.cchTextMax = 255;
	item.pszText = buf;
	if(ctrlSounds.GetSelectedItem(&item)) {
	tstring x = _T("");	
		if(WinUtil::browseFile(x, m_hWnd, false) == IDOK) {
			ctrlSounds.SetItemText(item.iItem, 1, x.c_str());
		}
	}
	return 0;
}

LRESULT Sounds::onClickedNone(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR buf[MAX_PATH];
	LVITEM item =  { 0 };
	item.mask = LVIF_TEXT;
	item.cchTextMax = 255;
	item.pszText = buf;
	if(ctrlSounds.GetSelectedItem(&item)) {
		tstring x = _T("");	
		ctrlSounds.SetItemText(item.iItem, 1, x.c_str());
	}
	return 0;
}

LRESULT Sounds::onPlay(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR buf[MAX_PATH];
	LVITEM item = { 0 };
	item.mask = LVIF_TEXT;
	item.cchTextMax = 255;
	item.pszText = buf;
	if(ctrlSounds.GetSelectedItem(&item)) {
		ctrlSounds.GetItemText(item.iItem, 1, buf, MAX_PATH);
		PlaySound(buf, NULL, SND_FILENAME | SND_ASYNC);

	}
	return 0;
}
/**
 * @file
 * $Id: Sounds.cpp 477 2010-01-29 08:59:43Z bigmuscle $
 */

