/*
 * Copyright (C) 2012-2024 AirDC++ Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#ifndef DCPLUSPLUS_WIZARD_CONNSPEED
#define DCPLUSPLUS_WIZARD_CONNSPEED

#include "stdafx.h"
#include "resource.h"
#include "PropPage.h"
#include "Wizard.h"

#include <atldlgs.h>

namespace wingui {
class SetupWizard;
class WizardConnspeed : public PropPage, public CAeroWizardPageImpl<WizardConnspeed> { 
public: 
	typedef CAeroWizardPageImpl<WizardConnspeed> baseClass;
	BEGIN_MSG_MAP(WizardConnspeed) 
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_HANDLER(IDC_DOWN_SPEED, CBN_SELENDOK, OnDownSpeed)
		COMMAND_HANDLER(IDC_CONNECTION, CBN_SELENDOK, OnUploadSpeed)
		COMMAND_HANDLER(IDC_DOWN_SPEED, CBN_EDITCHANGE, OnDownSpeed)
		COMMAND_HANDLER(IDC_CONNECTION, CBN_EDITCHANGE, OnUploadSpeed)
		COMMAND_ID_HANDLER(IDC_DL_AUTODETECT, OnDetect)
		COMMAND_ID_HANDLER(IDC_UL_AUTODETECT, OnDetect)
		COMMAND_ID_HANDLER(IDC_SPEEDTEST, onSpeedtest)
		CHAIN_MSG_MAP(baseClass) 
	END_MSG_MAP() 

	LRESULT OnInitDialog(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */);
	LRESULT OnDetect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDownSpeed(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnUploadSpeed(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSpeedtest(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }

	enum { IDD = IDD_WIZARD_CONNSPEED }; 
	WizardConnspeed(SettingsManager *s, SetupWizard* aWizard);

	void write();

	int OnSetActive();
private: 
	static Item items[];
	static Item uploadItems[];
	static Item downloadItems[];
	static TextItem texts[];

	void setDownloadLimits(double value);
	void setUploadLimits(double value);
	void fixcontrols();
	void updateAutoValues();

	CComboBox ctrlUpload;	
	CComboBox ctrlDownload;

	CButton cAutoUL;
	CButton cAutoDL;

	SetupWizard* wizard;
}; 

}

#endif
