

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

#include <web-server/ApiSettingItem.h>

#include <airdcpp/typedefs.h>

using namespace webserver;

class ConfigUtil {

#define MAX_TEXT_WIDTH 400
#define MARGIN_LEFT 20

public:

	class ConfigItem {
	public:
		enum {
			FLAG_DISABLE_LABEL = 0x01,
			FLAG_DISABLE_HELP = 0x02,
		};

		ConfigItem(ExtensionSettingItem& aSetting, int aFlags = 0);

		string getId() const {
			return setting.name;
		}
		string getLabel() const {
			return setting.getTitle();
		}
		

		ExtensionSettingItem& setting;

		void Create(HWND m_hWnd);
		int updateLayout(HWND m_hWnd, int aPrevConfigBottomMargin, int aConfigSpacing);
		virtual void onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam);

		virtual bool handleClick(HWND m_hWnd) = 0;
		virtual bool write() = 0;
		virtual tstring valueToString() noexcept;

		int getParentRightEdge(HWND m_hWnd);

		static int calculateTextRows(const tstring& aText, HWND m_hWndControl, int aMaxWidth = MAX_TEXT_WIDTH) noexcept;
	protected:
		virtual void Create(HWND m_hWnd, RECT rcDefault) = 0;
		virtual void updateLayout(HWND m_hWnd, CRect& rect_) = 0;

		CStatic ctrlLabel;
		CStatic ctrlHelp;

		void addLabel(HWND m_hWnd, CRect& rect_) noexcept;
		void addHelpText(HWND m_hWnd, CRect& rect_) noexcept;
	private:
		CRect calculateItemPosition(HWND m_hWnd, int aPrevConfigBottomMargin, int aConfigSpacing);

		const int flags;
	};

	static shared_ptr<ConfigItem> getConfigItem(ExtensionSettingItem& aSetting);

	//Combines CStatic as setting label and CEdit as setting field
	struct StringConfigItem : public ConfigItem {

		StringConfigItem(ExtensionSettingItem& aSetting) : ConfigItem(aSetting) {}

		//todo handle errors
		bool write() override;

		void Create(HWND m_hWnd, RECT rcDefault) override;
		void updateLayout(HWND m_hWnd, CRect& rect_) override;

		bool handleClick(HWND m_hWnd) override;

		CEdit ctrlEdit;
	};

	//CheckBox type config
	struct BoolConfigItem : public ConfigItem {

		BoolConfigItem(ExtensionSettingItem& aSetting) : ConfigItem(aSetting, FLAG_DISABLE_LABEL) {}


		//todo handle errors
		bool write() override;

		tstring valueToString() noexcept override;

		void Create(HWND m_hWnd, RECT rcDefault) override;
		void updateLayout(HWND m_hWnd, CRect& rect_) override;

		bool handleClick(HWND m_hWnd) override;

		CButton ctrlCheck;
	};

	//Extends StringConfigItem by adding a browse button after CEdit field
	struct BrowseConfigItem : public StringConfigItem {

		BrowseConfigItem(ExtensionSettingItem& aSetting) : StringConfigItem(aSetting) {}

		void Create(HWND m_hWnd, RECT rcDefault) override;
		void updateLayout(HWND m_hWnd, CRect& rect_) override;

		bool handleClick(HWND m_hWnd) override;
		CButton ctrlButton;
		int buttonWidth = 80;
	};

	//Combines CStatic as setting label and CEdit as setting field with spin control
	struct IntConfigItem : public ConfigItem {

		IntConfigItem(ExtensionSettingItem& aSetting) : ConfigItem(aSetting) {}

		//todo handle errors
		bool write() override;

		void Create(HWND m_hWnd, RECT rcDefault) override;
		void updateLayout(HWND m_hWnd, CRect& rect_) override;

		bool handleClick(HWND m_hWnd) override;

		CEdit ctrlEdit;
		CUpDownCtrl spin;
	};

	struct EnumConfigItem : public ConfigItem {

		EnumConfigItem(ExtensionSettingItem& aSetting) : ConfigItem(aSetting) {}

		void Create(HWND m_hWnd, RECT rcDefault) override;
		void updateLayout(HWND m_hWnd, CRect& rect_) override;
		tstring valueToString() noexcept override;

		//todo handle errors
		bool write() override;
		bool handleClick(HWND m_hWnd) override;

		CComboBox ctrlSelect;
		int buttonWidth = 80;
	};

	struct WebConfigItem : public ConfigItem {

		WebConfigItem(ExtensionSettingItem& aSetting) : ConfigItem(aSetting, FLAG_DISABLE_HELP) {}

		void Create(HWND m_hWnd, RECT rcDefault) override;
		void updateLayout(HWND m_hWnd, CRect& rect_) override;

		//todo handle errors
		bool write() override;
		bool handleClick(HWND m_hWnd) override;

		CStatic ctrlValue;
		CHyperLink url;

		void onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam) override;
	};
};



#endif