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
#include "SpeedPage.h"
#include "PropertiesDlg.h"

PropPage::TextItem SpeedPage::texts[] = {
	{ IDC_LINE_SPEED, ResourceManager::LINE_SPEED },
	{ IDC_SETTINGS_UPLOAD_LINE_SPEED, ResourceManager::SETCZDC_UPLOAD_SPEED },
	{ IDC_SETTINGS_DOWNLOAD_LINE_SPEED, ResourceManager::SETCZDC_DOWNLOAD_SPEED },
	{ IDC_SETTINGS_MEBIBITS, ResourceManager::MBITSPS },
	{ IDC_SETTINGS_MEBIBITS2, ResourceManager::MBITSPS },
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
	{ IDC_DL_AUTODETECT, ResourceManager::AUTODETECT },
	{ IDC_SETTINGS_DOWNLOAD_LIMITS, ResourceManager::SETTINGS_DOWNLOAD_LIMITS },
	{ IDC_SETTINGS_DOWNLOADS_MAX, ResourceManager::SETTINGS_DOWNLOADS_MAX },
	{ IDC_EXTRA_DOWNLOADS_MAX, ResourceManager::SETTINGS_CZDC_EXTRA_DOWNLOADS },
	{ IDC_SETTINGS_DOWNLOADS_SPEED_PAUSE, ResourceManager::SETTINGS_DOWNLOADS_SPEED_PAUSE },
	{ IDC_MCN_AUTODETECT, ResourceManager::AUTODETECT },
	{ IDC_SETTINGS_MCNSLOTS, ResourceManager::SETTINGS_MCNSLOTS },
	{ IDC_SETTINGS_MAX_MCN_DL, ResourceManager::SETTINGS_MAX_MCN_DL },
	{ IDC_SETTINGS_MAX_MCN_UL, ResourceManager::SETTINGS_MAX_MCN_UL },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY },
};
PropPage::Item SpeedPage::items[] = {
	{ IDC_CONNECTION,	SettingsManager::UPLOAD_SPEED,	PropPage::T_STR },
	{ IDC_DL_SPEED,		SettingsManager::DOWNLOAD_SPEED,	PropPage::T_STR },
	{ IDC_UL_AUTODETECT, SettingsManager::UL_AUTODETECT, PropPage::T_BOOL },
	{ IDC_SLOTS, SettingsManager::SLOTS, PropPage::T_INT }, 
	{ IDC_MIN_UPLOAD_SPEED, SettingsManager::MIN_UPLOAD_SPEED, PropPage::T_INT },
	{ IDC_EXTRA_SLOTS, SettingsManager::EXTRA_SLOTS, PropPage::T_INT },
	{ IDC_SMALL_FILE_SIZE, SettingsManager::SET_MINISLOT_SIZE, PropPage::T_INT },
	{ IDC_EXTRA_SLOTS2, SettingsManager::HUB_SLOTS, PropPage::T_INT },
	{ IDC_AUTO_SLOTS, SettingsManager::AUTO_SLOTS, PropPage::T_INT  },
	{ IDC_PARTIAL_SLOTS, SettingsManager::EXTRA_PARTIAL_SLOTS, PropPage::T_INT  },
	{ IDC_DL_AUTODETECT, SettingsManager::DL_AUTODETECT, PropPage::T_BOOL },
	{ IDC_DOWNLOADS, SettingsManager::DOWNLOAD_SLOTS, PropPage::T_INT },
	{ IDC_MAXSPEED, SettingsManager::MAX_DOWNLOAD_SPEED, PropPage::T_INT },
	{ IDC_EXTRA_DOWN_SLOT, SettingsManager::EXTRA_DOWNLOAD_SLOTS, PropPage::T_INT },
	{ IDC_MCN_AUTODETECT, SettingsManager::MCN_AUTODETECT, PropPage::T_BOOL },
	{ IDC_MCNDLSLOTS, SettingsManager::MAX_MCN_DOWNLOADS, PropPage::T_INT },
	{ IDC_MCNULSLOTS, SettingsManager::MAX_MCN_UPLOADS, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};


LRESULT SpeedPage::onEnable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

void SpeedPage::fixControls() {
	BOOL ul = IsDlgButtonChecked(IDC_UL_AUTODETECT) != BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_SLOTS),					ul);
	::EnableWindow(GetDlgItem(IDC_MIN_UPLOAD_SPEED),		ul);
	::EnableWindow(GetDlgItem(IDC_EXTRA_SLOTS),				ul);
	::EnableWindow(GetDlgItem(IDC_SMALL_FILE_SIZE),			ul);
	::EnableWindow(GetDlgItem(IDC_EXTRA_SLOTS2),			ul);
	::EnableWindow(GetDlgItem(IDC_AUTO_SLOTS),				ul);
	::EnableWindow(GetDlgItem(IDC_PARTIAL_SLOTS),			ul);

	BOOL dl = IsDlgButtonChecked(IDC_DL_AUTODETECT) != BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_DOWNLOADS),				dl);
	::EnableWindow(GetDlgItem(IDC_MAXSPEED),				dl);
	::EnableWindow(GetDlgItem(IDC_EXTRA_DOWN_SLOT),			dl);

	BOOL mcn = IsDlgButtonChecked(IDC_MCN_AUTODETECT) != BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_MCNDLSLOTS),				mcn);
	::EnableWindow(GetDlgItem(IDC_MCNULSLOTS),				mcn);
}

LRESULT SpeedPage::onSpeedChanged(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
	tstring speed;
	speed.resize(1024);
	speed.resize(GetDlgItemText(wID, &speed[0], 1024));
	if (!speed.empty()) {
		boost::wregex reg;
		if(speed[speed.size() -1] == '.')
			reg.assign(_T("(\\d+\\.)"));
		else
			reg.assign(_T("(\\d+(\\.\\d+)?)"));
		if (!regex_match(speed, reg)) {
			CComboBox tmp;
			tmp.Attach(hWndCtl);
			DWORD dwSel;
			if ((dwSel = tmp.GetEditSel()) != CB_ERR) {
				tstring::iterator it = speed.begin() +  HIWORD(dwSel)-1;
				speed.erase(it);
				tmp.SetEditSel(0,-1);
				tmp.SetWindowText(speed.c_str());
				tmp.SetEditSel(HIWORD(dwSel)-1, HIWORD(dwSel)-1);
				tmp.Detach();
			}
		}
	}
	return TRUE;
}

LRESULT SpeedPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);

	ctrlDownload.Attach(GetDlgItem(IDC_DL_SPEED));
	for(StringIter i = SettingsManager::connectionSpeeds.begin(); i != SettingsManager::connectionSpeeds.end(); ++i)
		ctrlDownload.AddString(Text::toT(*i).c_str());

		if(find(SettingsManager::connectionSpeeds.begin(), SettingsManager::connectionSpeeds.end(),
			SETTING(DOWNLOAD_SPEED)) == SettingsManager::connectionSpeeds.end()) {
		ctrlDownload.AddString(Text::toT(SETTING(DOWNLOAD_SPEED)).c_str());
	}

	ctrlUpload.Attach(GetDlgItem(IDC_CONNECTION));
	for(StringIter i = SettingsManager::connectionSpeeds.begin(); i != SettingsManager::connectionSpeeds.end(); ++i)
		ctrlUpload.AddString(Text::toT(*i).c_str());

		if(find(SettingsManager::connectionSpeeds.begin(), SettingsManager::connectionSpeeds.end(),
			SETTING(UPLOAD_SPEED)) == SettingsManager::connectionSpeeds.end()) {
		ctrlUpload.AddString(Text::toT(SETTING(UPLOAD_SPEED)).c_str());
	}

	PropPage::read((HWND)*this, items);


	ctrlUpload.SetCurSel(ctrlUpload.FindString(0, Text::toT(SETTING(UPLOAD_SPEED)).c_str()));
	ctrlDownload.SetCurSel(ctrlDownload.FindString(0, Text::toT(SETTING(DOWNLOAD_SPEED)).c_str()));

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

	updown.Attach(GetDlgItem(IDC_SLOTSSPIN));
	updown.SetRange32(0, 100);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_SPEEDSPIN));
	updown.SetRange32(0, 10000);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_EXTRASLOTSSPIN));
	updown.SetRange32(0, 100);
	updown.Detach();

	updown.Attach(GetDlgItem(IDC_MCN_SPIN));
	updown.SetRange32(0, 100);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_MCN_SPIN2));
	updown.SetRange32(0, 100);
	updown.Detach();

	fixControls();
	return TRUE;
}



void SpeedPage::write() {

	PropPage::write((HWND)(*this), items);
	
	if(SETTING(SLOTS) < 1)
		settings->set(SettingsManager::SLOTS, 1);

	// Do specialized writing here
	if(SETTING(EXTRA_SLOTS) < 1)
		settings->set(SettingsManager::EXTRA_SLOTS, 1);

	if(SETTING(SET_MINISLOT_SIZE) < 64)
		settings->set(SettingsManager::SET_MINISLOT_SIZE, 64);

	if(SETTING(HUB_SLOTS) < 0)
		settings->set(SettingsManager::HUB_SLOTS, 0);
		
	if(SETTING(AUTO_SLOTS) < 0)
		settings->set(SettingsManager::AUTO_SLOTS, 0);		
}