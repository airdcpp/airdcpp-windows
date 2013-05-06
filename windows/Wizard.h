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

#ifndef DCPLUSPLUS_SETUP_WIZARD
#define DCPLUSPLUS_SETUP_WIZARD

#include "stdafx.h"
#include "resource.h"
#include "PropPage.h"

#include "WTL\atldlgs.h"

#include "WizardAutoConnectivity.h"
#include "WizardGeneral.h"
#include "WizardConnspeed.h"
#include "WizardProfile.h"
#include "WizardSharing.h"

class SetupWizard : public CAeroWizardFrameImpl<SetupWizard> { 
public: 
	enum Pages { 
		PAGE_GENERAL,
		PAGE_PROFILE,
		PAGE_CONNSPEED,
		PAGE_AUTOCONN,
		PAGE_SHARING,
		PAGE_LAST
	};

	BEGIN_MSG_MAP(SampleWizard) 
		CHAIN_MSG_MAP(__super) 
	END_MSG_MAP() 
		
	SetupWizard(bool isInitial = false);
	~SetupWizard();

	template<class T>
	T* getPage(Pages aPage) {
		const HWND page = PropSheet_IndexToHwnd((HWND)*this, aPage);
		if(page != NULL) {
			return (T*)pages[aPage];
		}
		return nullptr;
	}

	//LRESULT onFinish(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	//LRESULT onCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	int OnWizardFinish();
	bool isInitialRun() { return initial; }
private: 
	PropPage *pages[PAGE_LAST];
	bool initial;
};

#endif
