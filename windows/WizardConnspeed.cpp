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

#include "WizardConnspeed.h"
#include "WinUtil.h"

#include "../client/AirUtil.h"


PropPage::TextItem WizardConnspeed::texts[] = {
	{ IDC_CONNSPEED_INTRO, ResourceManager::WIZARD_CONNECTION_SPEED_INTRO },
	{ IDC_SPEEDTEST, ResourceManager::TEST_SPEED_ONLINE },


	{ IDC_LINE_SPEED, ResourceManager::LINE_SPEED },
	{ IDC_SETTINGS_UPLOAD_LINE_SPEED, ResourceManager::SETCZDC_UPLOAD_SPEED },
	{ IDC_SETTINGS_DOWNLOAD_LINE_SPEED, ResourceManager::SETCZDC_DOWNLOAD_SPEED },
	{ IDC_SETTINGS_MEBIBITS, ResourceManager::MBITSPS },
	{ IDC_SETTINGS_MEBIBITS2, ResourceManager::MBITSPS },
	{ IDC_SLOTS_GROUP, ResourceManager::UPLOAD_LIMITS },
	{ IDC_SETTINGS_UPLOADS_SLOTS, ResourceManager::SETTINGS_UPLOADS_SLOTS },
	{ IDC_SETTINGS_UPLOADS_MIN_SPEED, ResourceManager::SETTINGS_UPLOADS_MIN_SPEED },
	{ IDC_SETTINGS_KBPS, ResourceManager::KBPS },
	{ IDC_SETTINGS_KBPS2, ResourceManager::KBPS },
	{ IDC_SETTINGS_AUTO_SLOTS, ResourceManager::SETTINGS_AUTO_SLOTS	},
	{ IDC_DL_AUTODETECT, ResourceManager::AUTODETECT },
	{ IDC_UL_AUTODETECT, ResourceManager::AUTODETECT },
	{ IDC_SETTINGS_DOWNLOAD_LIMITS, ResourceManager::DOWNLOAD_LIMITS },
	{ IDC_SETTINGS_DOWNLOADS_MAX, ResourceManager::SETTINGS_DOWNLOADS_MAX },
	{ IDC_SETTINGS_DOWNLOADS_SPEED_PAUSE, ResourceManager::SETTINGS_DOWNLOADS_SPEED_PAUSE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item WizardConnspeed::items[] = {
	{ IDC_CONNECTION, SettingsManager::UPLOAD_SPEED, PropPage::T_STR },
	{ IDC_DOWN_SPEED, SettingsManager::DOWNLOAD_SPEED, PropPage::T_STR },

	{ IDC_DL_AUTODETECT, SettingsManager::DL_AUTODETECT, PropPage::T_BOOL },
	{ IDC_UL_AUTODETECT, SettingsManager::UL_AUTODETECT, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

PropPage::Item WizardConnspeed::uploadItems[] = {
	{ IDC_UPLOAD_SLOTS, SettingsManager::DOWNLOAD_SLOTS, PropPage::T_INT },
	{ IDC_MAX_UPLOAD_SP, SettingsManager::MIN_UPLOAD_SPEED, PropPage::T_INT },
	{ IDC_MAX_AUTO_OPENED, SettingsManager::AUTO_SLOTS, PropPage::T_INT },

	{ 0, 0, PropPage::T_END }
};

PropPage::Item WizardConnspeed::downloadItems[] = {
	{ IDC_UPLOAD_SLOTS, SettingsManager::SLOTS, PropPage::T_INT },
	{ IDC_MAX_DOWNLOAD_SP, SettingsManager::MAX_DOWNLOAD_SPEED, PropPage::T_INT },

	{ 0, 0, PropPage::T_END }
};

LRESULT WizardConnspeed::OnInitDialog(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) {
	PropPage::translate((HWND)(*this), texts);

	ctrlDownload.Attach(GetDlgItem(IDC_DOWN_SPEED));
	ctrlUpload.Attach(GetDlgItem(IDC_CONNECTION));

	setMinMax(IDC_DOWN_SPEED_SPIN, 0, 100000);
	setMinMax(IDC_DOWNLOAD_S_SPIN, 0, 999);

	setMinMax(IDC_UPLOAD_S_SPIN, 1, 100);
	setMinMax(IDC_UP_SPEED_SPIN, 1, 100);

	setMinMax(IDC_AUTO_SLOTS_SPIN, 1, 100);


	//fill the speed combos
	WinUtil::appendSpeedCombo(ctrlDownload, SettingsManager::DOWNLOAD_SPEED);
	WinUtil::appendSpeedCombo(ctrlUpload, SettingsManager::UPLOAD_SPEED);


	//Set current values
	SetDlgItemText(IDC_MAX_DOWNLOAD_SP, Text::toT(Util::toString(AirUtil::getSpeedLimit(true))).c_str());
	SetDlgItemText(IDC_DOWNLOAD_SLOTS, Text::toT(Util::toString(AirUtil::getSlots(true))).c_str());
	SetDlgItemText(IDC_UPLOAD_SLOTS, Text::toT(Util::toString(AirUtil::getSlots(false))).c_str());
	SetDlgItemText(IDC_MAX_UPLOAD_SP, Text::toT(Util::toString(AirUtil::getSpeedLimit(false))).c_str());
	SetDlgItemText(IDC_MAX_AUTO_OPENED, Text::toT(Util::toString(AirUtil::getMaxAutoOpened())).c_str());

	cAutoDL.Attach(GetDlgItem(IDC_DL_AUTODETECT));
	cAutoUL.Attach(GetDlgItem(IDC_UL_AUTODETECT));

	cAutoDL.SetCheck(SETTING(DL_AUTODETECT) ? BST_CHECKED : BST_UNCHECKED);
	cAutoUL.SetCheck(SETTING(UL_AUTODETECT) ? BST_CHECKED : BST_UNCHECKED);

	fixcontrols();
	return TRUE; 
}

WizardConnspeed::WizardConnspeed(SettingsManager *s, SetupWizard* aWizard) : PropPage(s), wizard(aWizard) { 
	SetHeaderTitle(CTSTRING(CONNECTION_SPEED)); 
} 

void WizardConnspeed::write() {
	// common items
	PropPage::write((HWND)(*this), items);

	// only write those when not using auto detection
	if(cAutoUL.GetCheck() == 0) {
		PropPage::write((HWND)(*this), uploadItems);
	}

	if(cAutoDL.GetCheck() == 0) {
		PropPage::write((HWND)(*this), downloadItems);
	}
}
	
LRESULT WizardConnspeed::onSpeedtest(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	WinUtil::openLink(_T("http://www.speedtest.net"));
	return 0;
}

LRESULT WizardConnspeed::OnDetect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	updateAutoValues();
	return 0;
}

void WizardConnspeed::updateAutoValues() {
	fixcontrols();

	TCHAR buf1[64];
	GetDlgItemText(IDC_DOWN_SPEED, buf1, sizeof(buf1) +1);
	double valueDL = Util::toDouble(Text::fromT(buf1));
	setDownloadLimits(valueDL);

	TCHAR buf2[64];
	GetDlgItemText(IDC_CONNECTION, buf2, sizeof(buf2) +1);
	double valueUL = Util::toDouble(Text::fromT(buf2));
	setUploadLimits(valueUL);
}

LRESULT WizardConnspeed::OnDownSpeed(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
	WinUtil::onConnSpeedChanged(wNotifyCode, wID, hWndCtl);
	TCHAR buf2[64];
	switch(wNotifyCode) {
		case CBN_EDITCHANGE:
			GetDlgItemText(IDC_DOWN_SPEED, buf2, sizeof(buf2) +1);
			break;

		case CBN_SELENDOK:
			ctrlDownload.GetLBText(ctrlDownload.GetCurSel(), buf2);
			break;
	}

	double value = Util::toDouble(Text::fromT(buf2));

	setDownloadLimits(value);

	return 0;
}

LRESULT WizardConnspeed::OnUploadSpeed(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	WinUtil::onConnSpeedChanged(wNotifyCode, wID, hWndCtl);
	TCHAR buf2[64];
	switch(wNotifyCode) {
		case CBN_EDITCHANGE:
			GetDlgItemText(IDC_CONNECTION, buf2, sizeof(buf2) +1);
			break;
		case CBN_SELENDOK:
			ctrlUpload.GetLBText(ctrlUpload.GetCurSel(), buf2);
			break;

	}

	double value = Util::toDouble(Text::fromT(buf2));
	setUploadLimits(value);

	return 0;
}

void WizardConnspeed::fixcontrols() {
	BOOL dl = cAutoDL.GetCheck() == 0;
	::EnableWindow(GetDlgItem(IDC_DOWNLOAD_SLOTS),					dl);
	::EnableWindow(GetDlgItem(IDC_MAX_DOWNLOAD_SP),					dl);

	::EnableWindow(GetDlgItem(IDC_SETTINGS_DOWNLOADS_MAX),			dl);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_DOWNLOADS_SPEED_PAUSE),	dl);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_KBPS2),					dl);


	BOOL ul = cAutoUL.GetCheck() == 0;
	::EnableWindow(GetDlgItem(IDC_UPLOAD_SLOTS),				ul);
	::EnableWindow(GetDlgItem(IDC_MAX_UPLOAD_SP),				ul);
	::EnableWindow(GetDlgItem(IDC_MAX_AUTO_OPENED),				ul);

	::EnableWindow(GetDlgItem(IDC_SETTINGS_UPLOADS_MIN_SPEED),	ul);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_AUTO_SLOTS),			ul);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_UPLOADS_SLOTS),		ul);
	::EnableWindow(GetDlgItem(IDC_SETTINGS_KBPS),				ul);


	TCHAR buf[64];
	GetDlgItemText(IDC_CONNECTION, buf, sizeof(buf) +1);
	double uploadvalue = Util::toDouble(Text::fromT(buf));
	setUploadLimits(uploadvalue);

	TCHAR buf2[64];
	GetDlgItemText(IDC_DOWN_SPEED, buf2, sizeof(buf2) +1);
	double downloadvalue = Util::toDouble(Text::fromT(buf2));
	setDownloadLimits(downloadvalue);
}

void WizardConnspeed::setDownloadLimits(double value) {
	if (cAutoDL.GetCheck() > 0) {
		auto page = wizard->getPage<WizardProfile>(SetupWizard::PAGE_PROFILE);
		int dlSlots=AirUtil::getSlots(true, value, static_cast<SettingsManager::SettingProfile>(page ? page->getCurProfile() : SETTING(SETTINGS_PROFILE)));
		SetDlgItemText(IDC_DOWNLOAD_SLOTS, Util::toStringW(dlSlots).c_str());
	
		int dlLimit=AirUtil::getSpeedLimit(true, value);
		SetDlgItemText(IDC_MAX_DOWNLOAD_SP, Util::toStringW(dlLimit).c_str());
	}
}

void WizardConnspeed::setUploadLimits(double value) {
	if (cAutoUL.GetCheck() > 0) {
		auto page = wizard->getPage<WizardProfile>(SetupWizard::PAGE_PROFILE);
		int ulSlots=AirUtil::getSlots(false, value, static_cast<SettingsManager::SettingProfile>(page ? page->getCurProfile() : SETTING(SETTINGS_PROFILE)));
		SetDlgItemText(IDC_UPLOAD_SLOTS, Util::toStringW(ulSlots).c_str());
	
		int ulLimit=AirUtil::getSpeedLimit(false, value);
		SetDlgItemText(IDC_MAX_UPLOAD_SP, Util::toStringW(ulLimit).c_str());

		int autoOpened=AirUtil::getMaxAutoOpened(value);
		SetDlgItemText(IDC_MAX_AUTO_OPENED, Util::toStringW(autoOpened).c_str());
	}
}

int WizardConnspeed::OnSetActive() {
	ShowWizardButtons( PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_FINISH | PSWIZB_CANCEL, PSWIZB_BACK | PSWIZB_NEXT | PSWIZB_CANCEL); 
	EnableWizardButtons(PSWIZB_BACK, PSWIZB_BACK);
	EnableWizardButtons(PSWIZB_NEXT, PSWIZB_NEXT);

	//update in case the profile has been changed
	updateAutoValues();
	return 0;
}