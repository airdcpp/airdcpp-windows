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

#include "Wizard.h"


SetupWizard::SetupWizard(bool isInitial /*false*/) : CAeroWizardFrameImpl<SetupWizard>(_T("Setup Wizard")), initial(isInitial), saved(false), pagesDeleted(false) {
	//m_psh.pfnCallback = &PropSheetProc;

	auto s = SettingsManager::getInstance();

	int n = 0;
	pages[n++] = new WizardGeneral(s, this);
	pages[n++] = new WizardProfile(s, this);
	pages[n++] = new WizardConnspeed(s, this);
	pages[n++] = new WizardAutoConnectivity(s, this);
	pages[n++] = new WizardManualConnectivity(s, this);
	pages[n++] = new WizardSharing(s, this);
	pages[n++] = new WizardFinish(s, this);

	for(int i=0; i < n; i++) {
		AddPage(pages[i]->getPSP());
	}
}

SetupWizard::~SetupWizard() { 
	if (!pagesDeleted) {
		for(int i=0; i < PAGE_LAST; i++) {
			delete pages[i];
		}
	}
}

void SetupWizard::deletePages(PropPage::TaskList& tasks) {
	pagesDeleted = true;
	for(int i=0; i < PAGE_LAST; i++) {
		if (saved) {
			auto t = pages[i]->getThreadedTask();
			if (t) {
				tasks.emplace_back(pages[i]->getThreadedTask(), pages[i]);
				continue;
			}
		}
		
		delete pages[i];
	}
}

int SetupWizard::OnWizardFinish() {
	saved = true;
	for(int i=0; i < PAGE_LAST; i++)
	{
		// Check HWND of page to see if it has been created
		const HWND page = PropSheet_IndexToHwnd((HWND)*this, i);

		if(page != NULL)
			pages[i]->write();	
	}
	return FALSE;
}