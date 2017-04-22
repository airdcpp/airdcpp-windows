

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

#ifndef CONFIG_UTIL_H
#define CONFIG_UTIL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WinUtil.h"
#include <api/common/SettingUtils.h>

class ConfigUtil {
public:

	struct ConfigIem {
		ConfigIem(const string& aName, const string& aId, int aType) : label(aName), id(aId), type(aType) {}
		GETSET(string, id, Id);
		GETSET(string, label, Label);
		int type;

		virtual void Create(HWND m_hWnd) = 0;
		virtual int updateLayout(HWND m_hWnd, int prevConfigBottomMargin, int configSpacing) = 0;
		virtual bool handleClick(HWND m_hWnd) = 0;
		virtual boost::variant<string, bool, int> getValue() = 0;

		int getParentRightEdge(HWND m_hWnd);
		CRect calculateItemPosition(HWND m_hWnd, int prevConfigBottomMargin, int configSpacing);

	};

	static shared_ptr<ConfigIem> getConfigItem(const string& aName, const string& aId, int aType);

	//Combines CStatic as setting label and CEdit as setting field
	struct StringConfigItem : public ConfigIem {

		StringConfigItem(const string& aName, const string& aId, int aType) : ConfigIem(aName, aId, aType) {}

		boost::variant<string, bool, int> getValue() {
			return Text::fromT(WinUtil::getEditText(ctrlEdit));
		}

		void setLabel();

		void Create(HWND m_hWnd);
		int updateLayout(HWND m_hWnd, int prevConfigBottomMargin, int configSpacing);

		bool handleClick(HWND m_hWnd);

		CStatic ctrlLabel;
		CEdit ctrlEdit;
	};

	//CheckBox type config
	struct BoolConfigItem : public ConfigIem {

		BoolConfigItem(const string& aName, const string& aId, int aType) : ConfigIem(aName, aId, aType) {}

		boost::variant<string, bool, int> getValue() {
			return ctrlCheck.GetCheck() == 1;
		}

		void setLabel();

		void Create(HWND m_hWnd);
		int updateLayout(HWND m_hWnd, int prevConfigBottomMargin, int configSpacing);

		bool handleClick(HWND m_hWnd);

		CButton ctrlCheck;
	};

	//Extends StringConfigItem by adding a browse button after CEdit field
	struct BrowseConfigItem : public StringConfigItem {

		BrowseConfigItem(const string& aName, const string& aId, int aType) : StringConfigItem(aName, aId, aType) {}

		void Create(HWND m_hWnd);
		int updateLayout(HWND m_hWnd, int prevConfigBottomMargin, int configSpacing);

		bool handleClick(HWND m_hWnd);
		CButton ctrlButton;
		int buttonWidth = 80;

		int clickCounter = 0; //for testing...
	};
};



#endif