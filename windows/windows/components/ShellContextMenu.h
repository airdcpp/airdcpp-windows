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

// Based on <http://www.codeproject.com/shell/shellcontextmenu.asp> by R. Engels.

#ifndef DCPLUSPLUS_WIN32_SHELL_MENU_H
#define DCPLUSPLUS_WIN32_SHELL_MENU_H

#include <windows/stdafx.h>

#include <windows/components/OMenu.h>

namespace wingui {
class ShellMenu : public OMenu {
	typedef OMenu BaseType;
	static ShellMenu* curMenu;
public:
	BEGIN_MSG_MAP(ShellMenu)
		MESSAGE_HANDLER(WM_DRAWITEM, handleDrawItem)
		MESSAGE_HANDLER(WM_MEASUREITEM, handleMeasureItem)
		MESSAGE_HANDLER(WM_MENUCHAR, dispatch)
		MESSAGE_HANDLER(WM_INITMENUPOPUP, handleInitMenuPopup)
		MESSAGE_HANDLER(WM_UNINITMENUPOPUP, handleUnInitMenuPopup)
		MESSAGE_HANDLER(WM_MENUSELECT, handleMenuSelect)
	END_MSG_MAP()

	typedef ShellMenu ThisType;
	//typedef ShellMenuPtr ObjectType;

	ShellMenu();
	~ShellMenu();

	void appendShellMenu(const StringList& paths);
	void open(HWND aHWND, unsigned flags = TPM_LEFTALIGN | TPM_RIGHTBUTTON, CPoint pt = GetMessagePos());


	static LRESULT handleDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	static LRESULT handleMeasureItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	static LRESULT handleInitMenuPopup(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	static LRESULT handleUnInitMenuPopup(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	static LRESULT handleMenuSelect(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);

	static LRESULT dispatch(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
private:
	class Handler {
	public:
		Handler(LPCONTEXTMENU3 aMenu) : menu(aMenu) { }
		~Handler() { menu->Release(); }
		LPCONTEXTMENU3 getMenu() { return menu; }
	private:
		LPCONTEXTMENU3 menu;
	};

	static std::vector<std::pair<OMenu*, string>> paths;

	static unique_ptr<Handler> curHandler;
	static unsigned sel_id;
};
}

#endif // !defined(DCPLUSPLUS_WIN32_SHELL_MENU_H)
