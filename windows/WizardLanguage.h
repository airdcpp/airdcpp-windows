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

#ifndef DCPLUSPLUS_WIZARD_LANGUAGE
#define DCPLUSPLUS_WIZARD_LANGUAGE

#include "stdafx.h"
#include "resource.h"
#include "PropPage.h"

#include "Wizard.h"

#include <atldlgs.h>

//#include "../client/UpdateManagerListener.h"

class SetupWizard;
class WizardLanguage : public PropPage, public CAeroWizardPageImpl<WizardLanguage>/*, private UpdateManagerListener*/ { 
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

	void write();
	int OnWizardNext();
	int OnSetActive();

private:
	CComboBoxEx ctrlLanguage;
	SetupWizard* wizard;

	/*void on(UpdateManagerListener::LanguageDownloading) noexcept;
	void on(UpdateManagerListener::LanguageFinished) noexcept;
	void on(UpdateManagerListener::LanguageFailed, const string& aError) noexcept;*/
}; 

#endif
