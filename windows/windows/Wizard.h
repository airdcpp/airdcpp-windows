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

#ifndef DCPLUSPLUS_SETUP_WIZARD
#define DCPLUSPLUS_SETUP_WIZARD

#include <windows/stdafx.h>
#include <windows/resource.h>
#include <windows/PropPage.h>
#include <windows/Async.h>

#include <atldlgs.h>

namespace wingui {
class SetupWizard : public CAeroWizardFrameImpl<SetupWizard>, public Async<SetupWizard> { 
public: 
	enum { WM_USER_INITDIALOG = WM_APP + 501 };
	enum Pages { 
		PAGE_LANGUAGE,
		PAGE_GENERAL,
		PAGE_PROFILE,
		PAGE_CONNSPEED,
		PAGE_AUTOCONN,
		PAGE_MANUALCONN,
		PAGE_SHARING,
		PAGE_FINISH,
		PAGE_LAST
	};

	typedef CAeroWizardFrameImpl<SetupWizard> baseClass;
	BEGIN_MSG_MAP(SampleWizard)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		CHAIN_MSG_MAP(baseClass) 
	END_MSG_MAP() 
		
	SetupWizard(bool isInitial = false);
	~SetupWizard();

	template<class T>
	T* getPage(Pages aPage) {
		const HWND page = PropSheet_IndexToHwnd((HWND)*this, aPage);
		if(page != NULL) {
			return (T*)pages[aPage].get();
		}
		return nullptr;
	}

	int OnWizardFinish();
	bool isInitialRun() { return initial; }
	void getTasks(PropPage::TaskList& tasks);
private: 
	unique_ptr<PropPage> pages[PAGE_LAST];
	bool initial;
	bool saved;
	bool pagesDeleted;
};
}

#endif
