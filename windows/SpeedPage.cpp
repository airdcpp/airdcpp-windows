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
	{ IDC_SLOTS_GROUP, ResourceManager::UPLOAD_LIMITS },
	{ IDC_SETTINGS_UPLOADS_SLOTS, ResourceManager::SETTINGS_UPLOADS_SLOTS },
	{ IDC_SETTINGS_UPLOADS_MIN_SPEED, ResourceManager::SETTINGS_UPLOADS_MIN_SPEED },
	{ IDC_SETTINGS_KBPS, ResourceManager::KBPS },
	{ IDC_SETTINGS_AUTO_SLOTS, ResourceManager::SETTINGS_AUTO_SLOTS	},
	{ IDC_SLOTS_GROUP2, ResourceManager::UPLOAD_LIMITS_ADD },
	{ IDC_CZDC_SMALL_SLOTS, ResourceManager::SETCZDC_SMALL_UP_SLOTS },
	{ IDC_CZDC_SMALL_SIZE, ResourceManager::SETCZDC_SMALL_FILES },
	{ IDC_CZDC_NOTE_SMALL, ResourceManager::SETCZDC_NOTE_SMALL_UP },
	{ IDC_SETTINGS_PARTIAL_SLOTS, ResourceManager::SETSTRONGDC_PARTIAL_SLOTS },		
	{ IDC_STATICb, ResourceManager::EXTRA_HUB_SLOTS },
	{ IDC_DL_AUTODETECT, ResourceManager::AUTODETECT },
	{ IDC_SETTINGS_DOWNLOAD_LIMITS, ResourceManager::DOWNLOAD_LIMITS },
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


LRESULT SpeedPage::onEnable(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();

	updateValues(wNotifyCode);

	return 0;
}

void SpeedPage::fixControls() {
	BOOL ul = IsDlgButtonChecked(IDC_UL_AUTODETECT) != BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_SLOTS),					ul);
	::EnableWindow(GetDlgItem(IDC_MIN_UPLOAD_SPEED),		ul);
	::EnableWindow(GetDlgItem(IDC_AUTO_SLOTS),				ul);

	BOOL dl = IsDlgButtonChecked(IDC_DL_AUTODETECT) != BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_DOWNLOADS),				dl);
	::EnableWindow(GetDlgItem(IDC_MAXSPEED),				dl);
	::EnableWindow(GetDlgItem(IDC_EXTRA_DOWN_SLOT),			dl);

	BOOL mcn = IsDlgButtonChecked(IDC_MCN_AUTODETECT) != BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_MCNDLSLOTS),				mcn);
	::EnableWindow(GetDlgItem(IDC_MCNULSLOTS),				mcn);
}

LRESULT SpeedPage::onSpeedChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
	tstring speed;
	speed.resize(1024);
	speed.resize(GetDlgItemText(wID, &speed[0], 1024));
	if (!speed.empty() && wNotifyCode != CBN_SELENDOK) {
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

	updateValues(wNotifyCode);
	validateMCNLimits(wNotifyCode);
	return TRUE;
}

void SpeedPage::updateValues(WORD wNotifyCode) {

		//upload
		TCHAR buf[64];
		if (wNotifyCode == CBN_SELENDOK) {
			ctrlUpload.GetLBText(ctrlUpload.GetCurSel(), buf);
		} else {
			GetDlgItemText(IDC_CONNECTION, buf, sizeof(buf) +1);
		}

		double uploadvalue = Util::toDouble(Text::fromT(buf));
		setUploadLimits(uploadvalue);

		//download
		TCHAR buf2[64];
		if (wNotifyCode == CBN_SELENDOK) {
			ctrlDownload.GetLBText(ctrlDownload.GetCurSel(), buf2);
		} else {
			GetDlgItemText(IDC_DL_SPEED, buf2, sizeof(buf2) +1);
		}

		double downloadvalue = Util::toDouble(Text::fromT(buf2));
		setDownloadLimits(downloadvalue);
}

LRESULT SpeedPage::checkMCN(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	validateMCNLimits(wNotifyCode);
	return TRUE;
}

void SpeedPage::validateMCNLimits(WORD wNotifyCode) {
	if (IsDlgButtonChecked(IDC_MCN_AUTODETECT)) {
		return;
	}

	TCHAR buf[64];
	if (wNotifyCode == CBN_SELENDOK) {
		ctrlUpload.GetLBText(ctrlUpload.GetCurSel(), buf);
	} else {
		GetDlgItemText(IDC_CONNECTION, buf, sizeof(buf) +1);
	}
	double uploadvalue = Util::toDouble(Text::fromT(buf));

	TCHAR buf2[64];
	if (wNotifyCode == CBN_SELENDOK) {
		ctrlDownload.GetLBText(ctrlDownload.GetCurSel(), buf2);
	} else {
		GetDlgItemText(IDC_DL_SPEED, buf2, sizeof(buf2) +1);
	}
	double downloadvalue = Util::toDouble(Text::fromT(buf2));

	int downloadAutoSlots=Util::getSlotsPerUser(true, downloadvalue);
	int uploadAutoSlots=Util::getSlotsPerUser(false, uploadvalue);

	int mcnExtrasDL=maxMCNExtras(downloadvalue);
	int mcnExtrasUL=maxMCNExtras(uploadvalue);

	tstring aSlotsDL;
	aSlotsDL.resize(40);
	aSlotsDL.resize(GetDlgItemText(IDC_MCNDLSLOTS, &aSlotsDL[0], 40));

	tstring aSlotsUL;
	aSlotsUL.resize(40);
	aSlotsUL.resize(GetDlgItemText(IDC_MCNULSLOTS, &aSlotsUL[0], 40));

	//LogManager::getInstance()->message("DLslots: " + Util::toString(downloadAutoSlots) + " ULslots: " + Util::toString(uploadAutoSlots) + " MaxDLExtra: " + Util::toString(mcnExtrasDL) + " MaxULExtra: " + Util::toString(mcnExtrasUL) + " DL: " + Util::toString(downloadvalue) + " UL " + Util::toString(uploadvalue));
	//LogManager::getInstance()->message("aSlotsDL: " + Text::fromT(aSlotsDL) + " aSlotsUL: " + Text::fromT(aSlotsUL));

	if ((Util::toInt(Text::fromT(aSlotsDL)) > downloadAutoSlots + mcnExtrasDL)) {
		//LogManager::getInstance()->message("Change1");
		SetDlgItemText(IDC_MCNDLSLOTS, Util::toStringW(downloadAutoSlots + mcnExtrasDL).c_str());
	}

	if ((Util::toInt(Text::fromT(aSlotsUL)) > uploadAutoSlots + mcnExtrasUL)) {
		//LogManager::getInstance()->message("Change2");
		SetDlgItemText(IDC_MCNULSLOTS, Util::toStringW(uploadAutoSlots + mcnExtrasUL).c_str());
	}

	if ((Util::toInt(Text::fromT(aSlotsDL)) > uploadAutoSlots) && (Util::toInt(Text::fromT(aSlotsUL)) < uploadAutoSlots)) {
		//LogManager::getInstance()->message("Change3");
		SetDlgItemText(IDC_MCNDLSLOTS, Util::toStringW(uploadAutoSlots).c_str());
	}

	return;
}

int SpeedPage::maxMCNExtras(double speed) {

	int extras=0;

	if (speed < 1) {
		extras=0;
	} else if (speed >= 1 && speed <= 10) {
		extras=1;
	}  else if (speed > 10 && speed <= 40) {
		extras=2;
	} else if (speed > 40 && speed < 100) {
		extras=3;
	} else if (speed >= 100) {
		extras=4;
	}
	return extras;
}

void SpeedPage::setDownloadLimits(double value) {

	if (IsDlgButtonChecked(IDC_DL_AUTODETECT)) {
		int dlSlots=Util::getSlots(true, value, SETTING(SETTINGS_PROFILE) == SettingsManager::PROFILE_RAR);
		SetDlgItemText(IDC_DOWNLOADS, Util::toStringW(dlSlots).c_str());
	
		int dlLimit=Util::getSpeedLimit(true, value);
		SetDlgItemText(IDC_MAXSPEED, Util::toStringW(dlLimit).c_str());
	}

	if (IsDlgButtonChecked(IDC_MCN_AUTODETECT)) {
		int mcnDls=Util::getSlotsPerUser(true, value);
		SetDlgItemText(IDC_MCNDLSLOTS, Util::toStringW(mcnDls).c_str());
	}
}

void SpeedPage::setUploadLimits(double value) {

	if (IsDlgButtonChecked(IDC_UL_AUTODETECT)) {

		int ulSlots=Util::getSlots(false, value, SETTING(SETTINGS_PROFILE) == SettingsManager::PROFILE_RAR);
		SetDlgItemText(IDC_SLOTS, Util::toStringW(ulSlots).c_str());
	
		int ulLimit=Util::getSpeedLimit(false, value);
		SetDlgItemText(IDC_MIN_UPLOAD_SPEED, Util::toStringW(ulLimit).c_str());

		int autoOpened=Util::getMaxAutoOpened(value);
		SetDlgItemText(IDC_AUTO_SLOTS, Util::toStringW(autoOpened).c_str());
	}

	if (IsDlgButtonChecked(IDC_MCN_AUTODETECT)) {
		int mcnUls=Util::getSlotsPerUser(false, value);
		SetDlgItemText(IDC_MCNULSLOTS, Util::toStringW(mcnUls).c_str());
	}
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
	updown.SetRange32(1, 100);
	updown.Detach();
	updown.Attach(GetDlgItem(IDC_MCN_SPIN2));
	updown.SetRange32(1, 100);
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