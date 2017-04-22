#pragma once
#pragma once

/*
* Copyright (C) 2011-2017 AirDC++ Project
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

#ifndef DYNAMIC_TAB_PAGE_H
#define DYNAMIC_TAB_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <atlcrack.h>

#include <airdcpp/Util.h>
#include "ConfigUtil.h"


class DynamicTabPage : public CDialogImpl<DynamicTabPage> {
public:

	enum { IDD = IDD_TABPAGE };

	DynamicTabPage();
	~DynamicTabPage();

	BEGIN_MSG_MAP_EX(DynamicTabPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		//MESSAGE_HANDLER(WM_CTLCOLORDLG, onCtlColor)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBackground)
		MESSAGE_HANDLER(WM_PARENTNOTIFY, OnClick)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onCtlColor(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);

	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL & /*bHandled*/) {
		return 0;
	}

	LRESULT OnClick(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL & bHandled) {

		if (LOWORD(wParam) == WM_LBUTTONDOWN) {
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			HWND hWndCtrl = ChildWindowFromPoint(pt);
			if (hWndCtrl != NULL && hWndCtrl != m_hWnd && hWndCtrl != GetParent().m_hWnd) {
				for (auto cfg : configs) {
					if (cfg->handleClick(hWndCtrl))
						break;
				}
			}
		}
		bHandled = FALSE;
		return 0;
	}


	LRESULT onEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL & /*bHandled*/);

	void resizePage();

	void addConfigItem(const string& aName, const string& aId, int aType) {
		auto item = ConfigUtil::getConfigItem(aName, aId, aType);
		if(item)
			configs.emplace_back(item);
	}

	bool write() {
		for (auto cfg : configs) {
			auto i = cfg->getValue();
			if(i.type() == typeid(std::string))
				string tmp = boost::get<string>(i);
		}

		return true;
	}

	void updateLayout();

private:

	bool loading;
	vector<shared_ptr<ConfigUtil::ConfigIem>> configs;

	int prevConfigBottomMargin = 0;
	int configSpacing = 20;

	int editHeight = 25;

};


#endif
