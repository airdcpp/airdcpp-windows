

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


#include <api/ExtensionInfo.h>
#include <api/common/Serializer.h>
#include <api/common/SettingUtils.h>
#include <web-server/JsonUtil.h>

#include "WinUtil.h"

using namespace webserver;

class ConfigUtil {
public:

	struct ConfigIem {
		ConfigIem(ExtensionSettingItem& aSetting) : setting(aSetting) {}

		string getId() const {
			return setting.name;
		}
		string getLabel() const {
			return setting.getTitle();
		}
		

		ExtensionSettingItem& setting;

		virtual void Create(HWND m_hWnd) = 0;
		virtual int updateLayout(HWND m_hWnd, int aPrevConfigBottomMargin, int aConfigSpacing) = 0;
		virtual bool handleClick(HWND m_hWnd) = 0;
		virtual bool write() = 0;

		int getParentRightEdge(HWND m_hWnd);
		CRect calculateItemPosition(HWND m_hWnd, int aPrevConfigBottomMargin, int aConfigSpacing);

	};

	static shared_ptr<ConfigIem> getConfigItem(ExtensionSettingItem& aSetting);

	//Combines CStatic as setting label and CEdit as setting field
	struct StringConfigItem : public ConfigIem {

		StringConfigItem(ExtensionSettingItem& aSetting) : ConfigIem(aSetting) {}

		//todo handle errors
		bool write() override {
			auto val = SettingUtils::validateValue(Text::fromT(WinUtil::getEditText(ctrlEdit)), setting, nullptr);
			setting.setValue(val);
			return true;
		}

		void setLabel();

		void Create(HWND m_hWnd) override;
		int updateLayout(HWND m_hWnd, int aPrevConfigBottomMargin, int aConfigSpacing) override;

		bool handleClick(HWND m_hWnd) override;

		CStatic ctrlLabel;
		CEdit ctrlEdit;
	};

	//CheckBox type config
	struct BoolConfigItem : public ConfigIem {

		BoolConfigItem(ExtensionSettingItem& aSetting) : ConfigIem(aSetting) {}


		//todo handle errors
		bool write() override {
			auto val = SettingUtils::validateValue((ctrlCheck.GetCheck() == 1), setting, nullptr);
			setting.setValue(val);
			return true;
		}

		void setLabel();

		void Create(HWND m_hWnd) override;
		int updateLayout(HWND m_hWnd, int aPrevConfigBottomMargin, int aConfigSpacing) override;

		bool handleClick(HWND m_hWnd) override;

		CButton ctrlCheck;
	};

	//Extends StringConfigItem by adding a browse button after CEdit field
	struct BrowseConfigItem : public StringConfigItem {

		BrowseConfigItem(ExtensionSettingItem& aSetting) : StringConfigItem(aSetting) {}

		void Create(HWND m_hWnd) override;
		int updateLayout(HWND m_hWnd, int aPrevConfigBottomMargin, int aConfigSpacing) override;

		bool handleClick(HWND m_hWnd) override;
		CButton ctrlButton;
		int buttonWidth = 80;

		int clickCounter = 0; //for testing...
	};

	//Combines CStatic as setting label and CEdit as setting field with spin control
	struct IntConfigItem : public ConfigIem {

		IntConfigItem(ExtensionSettingItem& aSetting) : ConfigIem(aSetting) {}

		//todo handle errors
		bool write() override {
			auto val = SettingUtils::validateValue(Util::toInt(Text::fromT(WinUtil::getEditText(ctrlEdit))), setting, nullptr);
			setting.setValue(val);
			return true;
		}

		void setLabel();

		void Create(HWND m_hWnd) override;
		int updateLayout(HWND m_hWnd, int aPrevConfigBottomMargin, int aConfigSpacing) override;

		bool handleClick(HWND m_hWnd) override;

		CStatic ctrlLabel;
		CEdit ctrlEdit;
		CUpDownCtrl spin;

	};

};



#endif