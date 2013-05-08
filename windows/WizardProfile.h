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

#ifndef DCPLUSPLUS_WIZARD_PROFILE
#define DCPLUSPLUS_WIZARD_PROFILE

#include "stdafx.h"

#include <atldlgs.h>

#include "resource.h"
#include "PropPage.h"
#include "Wizard.h"

#include "../client/SettingItem.h"

class SetupWizard;
class WizardProfile : public PropPage, public CAeroWizardPageImpl<WizardProfile> { 
public: 
	typedef CAeroWizardPageImpl<WizardProfile> baseClass;
	BEGIN_MSG_MAP(WizardProfile) 
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_NORMAL, OnSelProfile)
		COMMAND_ID_HANDLER(IDC_RAR, OnSelProfile)
		COMMAND_ID_HANDLER(IDC_LAN, OnSelProfile)
		CHAIN_MSG_MAP(baseClass) 
	END_MSG_MAP() 
			
	LRESULT OnInitDialog(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */);
	LRESULT OnSelProfile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	enum { IDD = IDD_WIZARD_PROFILE }; 

	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }

	WizardProfile(SettingsManager *s, SetupWizard* aWizard);

	void write();

	int getCurProfile();

	int OnWizardNext();
	int OnSetActive();
private:
	static TextItem texts[];

	void fixcontrols();

	SetupWizard* wizard;
	int lastProfile;
	vector<SettingItem> conflicts;
}; 

#endif
