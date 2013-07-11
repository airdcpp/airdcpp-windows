/* 
 * Copyright (C) 2012-2013 AirDC++ Project
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

#include "../client/AirUtil.h"
#include "../client/SettingsManager.h"

#include "Resource.h"
#include "SpeedPage.h"
#include "PropertiesDlg.h"
#include "WinUtil.h"

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
	{ IDC_DL_AUTODETECT, ResourceManager::AUTODETECT },
	{ IDC_UL_AUTODETECT, ResourceManager::AUTODETECT },
	{ IDC_SETTINGS_DOWNLOAD_LIMITS, ResourceManager::DOWNLOAD_LIMITS },
	{ IDC_SETTINGS_DOWNLOADS_MAX, ResourceManager::SETTINGS_DOWNLOADS_MAX },
	{ IDC_EXTRA_DOWNLOADS_MAX, ResourceManager::SETTINGS_CZDC_EXTRA_DOWNLOADS },
	{ IDC_SETTINGS_DOWNLOADS_SPEED_PAUSE, ResourceManager::SETTINGS_DOWNLOADS_SPEED_PAUSE },
	{ IDC_MCN_AUTODETECT, ResourceManager::AUTODETECT },
	{ IDC_SETTINGS_MCNSLOTS, ResourceManager::SETTINGS_MCNSLOTS },
	{ IDC_SETTINGS_MAX_MCN_DL, ResourceManager::SETTINGS_MAX_MCN_DL },
	{ IDC_SETTINGS_MAX_MCN_UL, ResourceManager::SETTINGS_MAX_MCN_UL },
	{ IDC_SETTINGS_MCN_NOTE, ResourceManager::SETTINGS_MCN_NOTE },

	{ IDC_MAX_RUNNING_BUNDLES_LBL, ResourceManager::MAX_RUNNING_BUNDLES },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY },
};

PropPage::Item SpeedPage::items[] = {
	{ IDC_CONNECTION,	SettingsManager::UPLOAD_SPEED,	PropPage::T_STR },
	{ IDC_DL_SPEED,		SettingsManager::DOWNLOAD_SPEED,	PropPage::T_STR },
	{ IDC_UL_AUTODETECT, SettingsManager::UL_AUTODETECT, PropPage::T_BOOL },
	{ IDC_DL_AUTODETECT, SettingsManager::DL_AUTODETECT, PropPage::T_BOOL },
	{ IDC_MCN_AUTODETECT, SettingsManager::MCN_AUTODETECT, PropPage::T_BOOL },

	{ IDC_MAX_RUNNING_BUNDLES, SettingsManager::MAX_RUNNING_BUNDLES, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

PropPage::Item SpeedPage::uploadItems[] = {
	{ IDC_SLOTS, SettingsManager::SLOTS, PropPage::T_INT }, 
	{ IDC_AUTO_SLOTS, SettingsManager::AUTO_SLOTS, PropPage::T_INT  },
	{ IDC_MIN_UPLOAD_SPEED, SettingsManager::MIN_UPLOAD_SPEED, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

PropPage::Item SpeedPage::downloadItems[] = {
	{ IDC_DOWNLOADS, SettingsManager::DOWNLOAD_SLOTS, PropPage::T_INT },
	{ IDC_MAXSPEED, SettingsManager::MAX_DOWNLOAD_SPEED, PropPage::T_INT },
	{ IDC_EXTRA_DOWN_SLOT, SettingsManager::EXTRA_DOWNLOAD_SLOTS, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

PropPage::Item SpeedPage::mcnItems[] = {
	{ IDC_MCNDLSLOTS, SettingsManager::MAX_MCN_DOWNLOADS, PropPage::T_INT },
	{ IDC_MCNULSLOTS, SettingsManager::MAX_MCN_UPLOADS, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

LRESULT SpeedPage::onEnable(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();

	updateValues(wNotifyCode);

	return 0;
}

void SpeedPage::fixControls() {
	BOOL ul = IsDlgButtonChecked(IDC_UL_AUTODETECT) != BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_SLOTS),					ul);
	::EnableWindow(GetDlgItem(IDC_MIN_UPLOAD_SPEED),		ul);
	::EnableWindow(GetDlgItem(IDC_AUTO_SLOTS),				ul);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_UPLOADS_SLOTS),	ul);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_AUTO_SLOTS),		ul);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_UPLOADS_MIN_SPEED),	ul);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_KBPS),			ul);

	BOOL dl = IsDlgButtonChecked(IDC_DL_AUTODETECT) != BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_DOWNLOADS),				dl);
	::EnableWindow(GetDlgItem(IDC_MAXSPEED),				dl);
	::EnableWindow(GetDlgItem(IDC_EXTRA_DOWN_SLOT),			dl);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_DOWNLOADS_MAX),	dl);
	::EnableWindow(GetDlgItem(IDC_EXTRA_DOWNLOADS_MAX),		dl);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_DOWNLOADS_SPEED_PAUSE),	dl);

	BOOL mcn = IsDlgButtonChecked(IDC_MCN_AUTODETECT) != BST_CHECKED;
	::EnableWindow(GetDlgItem(IDC_MCNDLSLOTS),				mcn);
	::EnableWindow(GetDlgItem(IDC_MCNULSLOTS),				mcn);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_MAX_MCN_DL),		mcn);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_MAX_MCN_UL),		mcn);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_MCN_NOTE),		mcn);
}

LRESULT SpeedPage::onSpeedChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
	if (loading)
		return FALSE;

	if (WinUtil::onConnSpeedChanged(wNotifyCode, wID, hWndCtl)) {
		updateValues(wNotifyCode);
		//validateMCNLimits(wNotifyCode);
	}
	return TRUE;
}

LRESULT SpeedPage::onSlotsChanged(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (loading || (wID == IDC_DOWNLOADS && IsDlgButtonChecked(IDC_DL_AUTODETECT)) || (wID == IDC_SLOTS && IsDlgButtonChecked(IDC_UL_AUTODETECT))) {
		return FALSE;
	}
	updateValues(wNotifyCode);
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

LRESULT SpeedPage::checkMCN(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	//validateMCNLimits(wNotifyCode);
	return TRUE;
}

void SpeedPage::validateMCNLimits(WORD wNotifyCode) {
	if (loading)
		return;

	if (IsDlgButtonChecked(IDC_MCN_AUTODETECT)) {
		return;
	}

	// We don't allow setting per user download slots higher from the default value if upload slots have been lowered from the default

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

	int downloadAutoSlots = AirUtil::getSlotsPerUser(true, downloadvalue);
	int uploadAutoSlots = AirUtil::getSlotsPerUser(false, uploadvalue);

	int mcnExtrasDL = maxMCNExtras(downloadvalue);
	int mcnExtrasUL = maxMCNExtras(uploadvalue);

	ctrlMcnDL.GetWindowText(buf, 40);
	int fieldDlSlots = Util::toInt(Text::fromT(buf));

	ctrlMcnUL.GetWindowText(buf, 40);
	int fieldUlSlots = Util::toInt(Text::fromT(buf));

	/*if (fieldDlSlots == 0 && ctrlMcnDL.GetWindowTextLength() > 0) {
		ctrlMcnDL.SetWindowText(_T(""));
	}

	if (fieldUlSlots == 0 && ctrlMcnUL.GetWindowTextLength() > 0) {
		ctrlMcnUL.SetWindowText(_T(""));
	}*/

	if (fieldDlSlots == 0 || (fieldDlSlots > downloadAutoSlots + mcnExtrasDL)) {
		if ((downloadAutoSlots + mcnExtrasDL) > 0) {
			fieldDlSlots = downloadAutoSlots + mcnExtrasDL;
			ctrlMcnDL.SetWindowText(Util::toStringW(fieldDlSlots).c_str());
		}
	}

	if (fieldUlSlots == 0 || (fieldUlSlots > uploadAutoSlots + mcnExtrasUL)) {
		if ((uploadAutoSlots + mcnExtrasUL) > 0) {
			fieldUlSlots = uploadAutoSlots + mcnExtrasUL;
			ctrlMcnUL.SetWindowText(Util::toStringW(fieldUlSlots).c_str());
		}
	}

	// download slots can't be more than upload slots
	if ((fieldDlSlots > fieldUlSlots) && fieldUlSlots < uploadAutoSlots) {
		ctrlMcnDL.SetWindowText(Util::toStringW(fieldUlSlots).c_str());
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
	int dlSlots=0;
	if (cAutoDL.GetCheck() > 0) {
		dlSlots=AirUtil::getSlots(true, value);
		SetDlgItemText(IDC_DOWNLOADS, Util::toStringW(dlSlots).c_str());
	
		int dlLimit=AirUtil::getSpeedLimit(true, value);
		SetDlgItemText(IDC_MAXSPEED, Util::toStringW(dlLimit).c_str());
	} else {
		tstring slots;
		slots.resize(40);
		slots.resize(GetDlgItemText(IDC_DOWNLOADS, &slots[0], 40));
		dlSlots = Util::toInt(Text::fromT(slots));
	}

	if (IsDlgButtonChecked(IDC_MCN_AUTODETECT)) {
		int mcnDls=AirUtil::getSlotsPerUser(true, value, dlSlots);
		SetDlgItemText(IDC_MCNDLSLOTS, Util::toStringW(mcnDls).c_str());
	}
}

void SpeedPage::setUploadLimits(double value) {
	int ulSlots=0;
	if (cAutoUL.GetCheck() > 0) {

		ulSlots=AirUtil::getSlots(false, value);
		SetDlgItemText(IDC_SLOTS, Util::toStringW(ulSlots).c_str());
	
		int ulLimit=AirUtil::getSpeedLimit(false, value);
		SetDlgItemText(IDC_MIN_UPLOAD_SPEED, Util::toStringW(ulLimit).c_str());

		int autoOpened=AirUtil::getMaxAutoOpened(value);
		SetDlgItemText(IDC_AUTO_SLOTS, Util::toStringW(autoOpened).c_str());
	} else {
		tstring slots;
		slots.resize(40);
		slots.resize(GetDlgItemText(IDC_SLOTS, &slots[0], 40));
		ulSlots = Util::toInt(Text::fromT(slots));
	}

	if (IsDlgButtonChecked(IDC_MCN_AUTODETECT)) {
		int mcnUls=AirUtil::getSlotsPerUser(false, value, ulSlots);
		SetDlgItemText(IDC_MCNULSLOTS, Util::toStringW(mcnUls).c_str());
	}
}

LRESULT SpeedPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);

	ctrlDownload.Attach(GetDlgItem(IDC_DL_SPEED));
	ctrlUpload.Attach(GetDlgItem(IDC_CONNECTION));

	cAutoDL.Attach(GetDlgItem(IDC_DL_AUTODETECT));
	cAutoUL.Attach(GetDlgItem(IDC_UL_AUTODETECT));

	ctrlMcnUL.Attach(GetDlgItem(IDC_MCNULSLOTS));
	ctrlMcnDL.Attach(GetDlgItem(IDC_MCNDLSLOTS));

	WinUtil::appendSpeedCombo(ctrlDownload, SettingsManager::DOWNLOAD_SPEED);
	WinUtil::appendSpeedCombo(ctrlUpload, SettingsManager::UPLOAD_SPEED);


	PropPage::read((HWND)*this, items);
	PropPage::read((HWND)*this, downloadItems);
	PropPage::read((HWND)*this, uploadItems);
	PropPage::read((HWND)*this, mcnItems);

	// set the correct values if using auto detection
	updateValues(0);

	setMinMax(IDC_SLOTSPIN, 1, UD_MAXVAL);
	setMinMax(IDC_MIN_UPLOAD_SPIN, 0, UD_MAXVAL);
	setMinMax(IDC_AUTO_SLOTS_SPIN, 0, 100);
	setMinMax(IDC_SLOTSPIN, 1, UD_MAXVAL);

	setMinMax(IDC_SLOTSSPIN, 0, 100);
	setMinMax(IDC_SPEEDSPIN, 0, 90000);
	setMinMax(IDC_EXTRASLOTSSPIN, 0, 100);

	setMinMax(IDC_MCN_SPIN, 1, 1000);
	setMinMax(IDC_MCN_SPIN2, 1, 1000);

	setMinMax(IDC_MAX_RUNNING_BUNDLES_SPIN, 0, 1000);

	fixControls();

	loading = false;
	return TRUE;
}



void SpeedPage::write() {

	PropPage::write((HWND)(*this), items);

	if (!SETTING(DL_AUTODETECT)) {
		PropPage::write((HWND)(*this), downloadItems);
	}

	if (!SETTING(UL_AUTODETECT)) {
		PropPage::write((HWND)(*this), uploadItems);

		if(SETTING(SLOTS) < 1)
			settings->set(SettingsManager::SLOTS, 1);

		if(SETTING(EXTRA_SLOTS) < 1)
			settings->set(SettingsManager::EXTRA_SLOTS, 1);

		if(SETTING(SET_MINISLOT_SIZE) < 64)
			settings->set(SettingsManager::SET_MINISLOT_SIZE, 64);
	}

	if (!SETTING(MCN_AUTODETECT)) {
		validateMCNLimits(0);
		PropPage::write((HWND)(*this), mcnItems);

		/*if(SETTING(MAX_MCN_DOWNLOADS) < 1)
			settings->set(SettingsManager::MAX_MCN_DOWNLOADS, AirUtil::getSlotsPerUser(true, Util::toDouble(SETTING(DOWNLOAD_SPEED))));

		if(SETTING(MAX_MCN_UPLOADS) < 1)
			settings->set(SettingsManager::MAX_MCN_UPLOADS, AirUtil::getSlotsPerUser(false, Util::toDouble(SETTING(UPLOAD_SPEED))));*/
	}	
}