/* 
 * Copyright (C) 2008 Big Muscle
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
#include "../client/ShareManager.h"

#include "Resource.h"
#include "UploadPage.h"

PropPage::TextItem UploadPage::texts[] = {
	{ IDC_SLOTS_GROUP, ResourceManager::SLOTS },
	{ IDC_SETTINGS_UPLOADS_MIN_SPEED, ResourceManager::SETTINGS_UPLOADS_MIN_SPEED },
	{ IDC_SETTINGS_KBPS, ResourceManager::KBPS }, 
	{ IDC_SETTINGS_UPLOADS_SLOTS, ResourceManager::SETTINGS_UPLOADS_SLOTS },
	{ IDC_CZDC_SMALL_SLOTS, ResourceManager::SETCZDC_SMALL_UP_SLOTS },
	{ IDC_CZDC_SMALL_SIZE, ResourceManager::SETCZDC_SMALL_FILES },
	{ IDC_CZDC_NOTE_SMALL, ResourceManager::SETCZDC_NOTE_SMALL_UP },
	{ IDC_SETTINGS_AUTO_SLOTS, ResourceManager::SETTINGS_AUTO_SLOTS	},	
	{ IDC_SETTINGS_PARTIAL_SLOTS, ResourceManager::SETSTRONGDC_PARTIAL_SLOTS },		
	{ IDC_STATICb, ResourceManager::EXTRA_HUB_SLOTS },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY },
};
PropPage::Item UploadPage::items[] = {
	{ IDC_SLOTS, SettingsManager::SLOTS, PropPage::T_INT }, 
	{ IDC_MIN_UPLOAD_SPEED, SettingsManager::MIN_UPLOAD_SPEED, PropPage::T_INT },
	{ IDC_EXTRA_SLOTS, SettingsManager::EXTRA_SLOTS, PropPage::T_INT },
	{ IDC_SMALL_FILE_SIZE, SettingsManager::SET_MINISLOT_SIZE, PropPage::T_INT },
	{ IDC_EXTRA_SLOTS2, SettingsManager::HUB_SLOTS, PropPage::T_INT },
	{ IDC_AUTO_SLOTS, SettingsManager::AUTO_SLOTS, PropPage::T_INT  },
	{ IDC_PARTIAL_SLOTS, SettingsManager::EXTRA_PARTIAL_SLOTS, PropPage::T_INT  },
	{ 0, 0, PropPage::T_END }
};

LRESULT UploadPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	CUpDownCtrl updown;
	updown.Attach(GetDlgItem(IDC_SLOTSPIN));
	updown.SetRange(1, UD_MAXVAL);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_MIN_UPLOAD_SPIN));
	updown.SetRange32(0, UD_MAXVAL);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_EXTRA_SLOTS_SPIN));
	updown.SetRange(3, 100);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_SMALL_FILE_SIZE_SPIN));
	updown.SetRange32(64, 30000);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_EXTRASPIN));
	updown.SetRange(0, 10);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_AUTO_SLOTS_SPIN));
	updown.SetRange(0, 100);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_PARTIAL_SLOTS_SPIN));
	updown.SetRange(0, 10);
	updown.Detach();
			
	return TRUE;
}

void UploadPage::write() {
	PropPage::write((HWND)(*this), items);
	
	if(SETTING(SLOTS) < 1)
		settings->set(SettingsManager::SLOTS, 1);

	// Do specialized writing here
	if(SETTING(EXTRA_SLOTS) < 3)
		settings->set(SettingsManager::EXTRA_SLOTS, 3);

	if(SETTING(SET_MINISLOT_SIZE) < 64)
		settings->set(SettingsManager::SET_MINISLOT_SIZE, 64);

	if(SETTING(HUB_SLOTS) < 0)
		settings->set(SettingsManager::HUB_SLOTS, 0);
		
	if(SETTING(AUTO_SLOTS) < 0)
		settings->set(SettingsManager::AUTO_SLOTS, 0);		
}

/**
 * @file
 * $Id: UploadPage.cpp 414 2008-08-01 19:16:45Z BigMuscle $
 */