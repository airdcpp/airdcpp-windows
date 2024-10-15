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

#ifndef DCPLUSPLUS_WIZARD_FINISH
#define DCPLUSPLUS_WIZARD_FINISH

#include <windows/stdafx.h>
#include <windows/resource.h>
#include <windows/settings/base/PropPage.h>
#include <windows/settings/wizard/Wizard.h>

#include <atldlgs.h>

namespace wingui {
class SetupWizard;
class WizardFinish : public PropPage, public CAeroWizardPageImpl<WizardFinish> { 
public: 
	typedef CAeroWizardPageImpl<WizardFinish> baseClass;
	BEGIN_MSG_MAP(WizardFinish) 
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog) 
		CHAIN_MSG_MAP(baseClass) 
	END_MSG_MAP() 

	LRESULT OnInitDialog(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */);
	
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }

	enum { IDD = IDD_WIZARD_FINISH }; 
	WizardFinish(SettingsManager *s, SetupWizard* wizard);

	void write();
	int OnWizardFinish();
	int OnSetActive();
	int OnWizardBack();
private: 
	SetupWizard* wizard;
	static TextItem texts[];
}; 

}

#endif
