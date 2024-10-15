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

#ifndef DCPLUSPLUS_WIZARD_LANGUAGE
#define DCPLUSPLUS_WIZARD_LANGUAGE

#include <windows/stdafx.h>
#include <windows/resource.h>
#include <windows/settings/base/PropPage.h>

#include <windows/settings/wizard/Wizard.h>
#include <windows/dialog/LanguageDownloadDlg.h>

#include <atldlgs.h>

namespace wingui {

class SetupWizard;
class WizardLanguage : public PropPage, public CAeroWizardPageImpl<WizardLanguage> { 
public: 
	typedef CAeroWizardPageImpl<WizardLanguage> baseClass;
	BEGIN_MSG_MAP(WizardLanguage) 
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		CHAIN_MSG_MAP(baseClass) 
	END_MSG_MAP() 
			
	LRESULT OnInitDialog(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */);

	enum { IDD = IDD_WIZARD_LANGUAGE }; 

	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }

	WizardLanguage(SettingsManager *s, SetupWizard* aWizard);
	~WizardLanguage();

	void write();
	int OnWizardNext();
	int OnSetActive();

private:
	bool checkLanguage();
	void completeLanguageCheck();
	CComboBoxEx ctrlLanguage;
	SetupWizard* wizard;
	LanguageDownloadDlg* dl;
}; 

}

#endif
