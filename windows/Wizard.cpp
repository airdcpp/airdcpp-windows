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

SetupWizard::SetupWizard() : CAeroWizardFrameImpl<SetupWizard>(_T("Setup Wizard")) { 
	auto s = SettingsManager::getInstance();

	int n = 0;
	pages[n++] = new WizardGeneral(s);
	pages[n++] = new WizardConnspeed(s);
	pages[n++] = new WizardAutoConnectivity(s, this);
	pages[n++] = new WizardSharing(s, this);

	for(int i=0; i < n; i++) {
		AddPage(pages[i]->getPSP());
	}
}

SetupWizard::~SetupWizard() {
	for(int i=0; i < PAGE_LAST; i++) {
		delete pages[i];
	}
}

int SetupWizard::OnWizardFinish() {
	for(int i=0; i < PAGE_LAST; i++)
	{
		// Check HWND of page to see if it has been created
		const HWND page = PropSheet_IndexToHwnd((HWND)*this, i);

		if(page != NULL)
			pages[i]->write();	
	}
	return FALSE;
}