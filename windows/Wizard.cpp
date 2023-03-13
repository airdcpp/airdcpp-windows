/*
 * Copyright (C) 2012-2023 AirDC++ Project
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

#include "Wizard.h"

#include "WizardAutoConnectivity.h"
#include "WizardManualConnectivity.h"
#include "WizardGeneral.h"
#include "WizardConnspeed.h"
#include "WizardProfile.h"
#include "WizardSharing.h"
#include "WizardFinish.h"
#include "WizardLanguage.h"


SetupWizard::SetupWizard(bool isInitial /*false*/) : CAeroWizardFrameImpl<SetupWizard>(CTSTRING(WIZARD)), initial(isInitial), saved(false), pagesDeleted(false) {
	//m_psh.pfnCallback = &PropSheetProc;

	auto s = SettingsManager::getInstance();

	int n = 0;
	pages[n++] = make_unique<WizardLanguage>(s, this);
	pages[n++] = make_unique<WizardGeneral>(s, this);
	pages[n++] = make_unique<WizardProfile>(s, this);
	pages[n++] = make_unique<WizardConnspeed>(s, this);
	pages[n++] = make_unique<WizardAutoConnectivity>(s, this);
	pages[n++] = make_unique<WizardManualConnectivity>(s, this);
	pages[n++] = make_unique<WizardSharing>(s, this);
	pages[n++] = make_unique<WizardFinish>(s, this);
	for(int i=0; i < n; i++) {
		AddPage(pages[i]->getPSP());
	}

	WinUtil::SetIcon(m_hWnd, IDR_MAINFRAME, false);
}

SetupWizard::~SetupWizard() { }

void SetupWizard::getTasks(PropPage::TaskList& tasks) {
	pagesDeleted = true;
	for(int i=0; i < PAGE_LAST; i++) {
		if (saved) {
			auto t = pages[i]->getThreadedTask();
			if (t) {
				tasks.emplace_back(pages[i]->getThreadedTask());
			}
		}
	}
}

int SetupWizard::OnWizardFinish() {
	saved = true;
	for(int i=0; i < PAGE_LAST; i++) {
		// Check HWND of page to see if it has been created
		const HWND page = PropSheet_IndexToHwnd((HWND)*this, i);
		if (page)
			pages[i]->write();	
	}
	return FALSE;
}