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

#ifndef DCPLUSPLUS_WIZARD_GENERAL
#define DCPLUSPLUS_WIZARD_GENERAL

#include "stdafx.h"
#include "resource.h"
#include "PropPage.h"
#include "ActionUtil.h"

#include "Wizard.h"

#include <atldlgs.h>

namespace wingui {
class SetupWizard;
class WizardGeneral : public PropPage, public CAeroWizardPageImpl<WizardGeneral> { 
public: 
	typedef CAeroWizardPageImpl<WizardGeneral> baseClass;
	BEGIN_MSG_MAP(WizardGeneral) 
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_HANDLER(IDC_NICK, EN_CHANGE, ActionUtil::onUserFieldChar)
		COMMAND_HANDLER(IDC_EMAIL, EN_CHANGE, ActionUtil::onUserFieldChar)
		COMMAND_HANDLER(IDC_USERDESC, EN_CHANGE, ActionUtil::onUserFieldChar)
		CHAIN_MSG_MAP(baseClass) 
	END_MSG_MAP() 
			
	LRESULT OnInitDialog(UINT /*message*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */);

	enum { IDD = IDD_WIZARD_GENERAL }; 

	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }

	WizardGeneral(SettingsManager *s, SetupWizard* aWizard);

	void write();
	int OnWizardNext();
	int OnSetActive();
private: 
	static Item items[];
	static TextItem texts[];

	SetupWizard* wizard;
}; 

}

#endif
