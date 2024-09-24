/*
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
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

#if !defined(PROP_PAGE_H)
#define PROP_PAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows/DimEdit.h>

#define SETTINGS_BUF_LEN 1024
#define DIM_EDIT_EXPERIMENT 0

#include <airdcpp/SettingsManager.h>
#include <airdcpp/ResourceManager.h>

#include <windows/Dispatchers.h>

namespace wingui {
#define setMinMax(x, y, z) \
	updown.Attach(GetDlgItem(x)); \
	updown.SetRange32(y, z); \
	updown.Detach();


class SettingTab {

public:
	SettingTab(SettingsManager *src) : settings(src) { }
	virtual void write() = 0;
	virtual Dispatcher::F getThreadedTask() { return nullptr; }

	enum Type { T_STR, T_INT, T_BOOL, T_CUSTOM, T_INT64, T_END };

	struct Item
	{
		WORD itemID;
		int setting;
		Type type;
	};
	struct ListItem {
		int setting;
		ResourceManager::Strings desc;
	};
	struct TextItem {
		WORD itemID;
		ResourceManager::Strings translatedString;
	};
	CUpDownCtrl updown;
protected:

#if DIM_EDIT_EXPERIMENT
	std::map<WORD, CDimEdit *> ctrlMap;
#endif
	SettingsManager *settings;
	void read(HWND page, Item const* items, ListItem* listItems = NULL, HWND list = NULL);
	void write(HWND page, Item const* items, ListItem* listItems = NULL, HWND list = NULL);
	void translate(HWND page, TextItem* textItems);
};

class PropPage : public SettingTab
{
public:
	typedef vector<Dispatcher::F> TaskList;

	PropPage(SettingsManager *src) : SettingTab(src) { }
	virtual ~PropPage() { }

	virtual PROPSHEETPAGE *getPSP() = 0;
};
}

#endif // !defined(PROP_PAGE_H)
