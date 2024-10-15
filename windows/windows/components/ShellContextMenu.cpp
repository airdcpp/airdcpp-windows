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

#include <windows/stdafx.h>
#include <windows/components/ShellContextMenu.h>

#include <windows/resource.h>
#include <windows/util/ActionUtil.h>

#include <airdcpp/util/PathUtil.h>
#include <airdcpp/util/text/Text.h>

namespace wingui {
ShellMenu* ShellMenu::curMenu = nullptr;
unique_ptr<ShellMenu::Handler> ShellMenu::curHandler;
unsigned ShellMenu::sel_id = 0; 
std::vector<std::pair<OMenu*, string>> ShellMenu::paths;

ShellMenu::ShellMenu()
{
}

void ShellMenu::appendShellMenu(const StringList& aPaths) {
	curMenu = this;

	if (aPaths.empty())
		return;

	appendSeparator();

	if (aPaths.size() == 1) {
		appendItem(TSTRING(OPEN_FOLDER), [=] { ActionUtil::openFolder(Text::toT(PathUtil::getFilePath(aPaths.front()))); }, OMenu::FLAG_THREADED);
		paths.emplace_back(createSubMenu(TSTRING(SHELL_MENU), true, true), aPaths.front());
	} else {
		auto fo = createSubMenu(TSTRING(OPEN_FOLDER));
		for (auto& i : aPaths)
			fo->appendItem(Text::toT(PathUtil::getFilePath(i)), [=] { ActionUtil::openFolder(Text::toT(i)); }, OMenu::FLAG_THREADED);

		auto sh = createSubMenu(TSTRING(SHELL_MENUS));
		for (auto& i : aPaths)
			paths.emplace_back(sh->createSubMenu(Text::toT(i)), i);
	}
}

ShellMenu::~ShellMenu() {
	curMenu = nullptr;
	curHandler.reset();
	sel_id = 0;
	paths.clear();
}

void ShellMenu::open(HWND aHWND, unsigned flags, CPoint pt) {
	BaseType::open(aHWND, flags, pt);

	if(sel_id >= ID_SHELLCONTEXTMENU_MIN && sel_id <= ID_SHELLCONTEXTMENU_MAX && curHandler) {
		CMINVOKECOMMANDINFO cmi = { sizeof(CMINVOKECOMMANDINFO) };
		cmi.lpVerb = (LPSTR)MAKEINTRESOURCE(sel_id - ID_SHELLCONTEXTMENU_MIN);
		cmi.nShow = SW_SHOWNORMAL;
		curHandler->getMenu()->InvokeCommand(&cmi);
	}
}

LRESULT ShellMenu::handleDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if(wParam)
		return false;

	LPDRAWITEMSTRUCT t = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
	if(!t)
		return false;
	if(t->CtlType != ODT_MENU)
		return false;

	const unsigned& id = t->itemID;
	if(id >= ID_SHELLCONTEXTMENU_MIN && id <= ID_SHELLCONTEXTMENU_MAX)
		return dispatch(uMsg, wParam, lParam, bHandled);

	return false;
}

LRESULT ShellMenu::handleMeasureItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if(wParam)
		return false;

	LPMEASUREITEMSTRUCT t = reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);
	if(!t)
		return false;
	if(t->CtlType != ODT_MENU)
		return false;

	const unsigned& id = t->itemID;
	if(id >= ID_SHELLCONTEXTMENU_MIN && id <= ID_SHELLCONTEXTMENU_MAX)
		return dispatch(uMsg, wParam, lParam, bHandled);

	return false;
}

LRESULT ShellMenu::handleInitMenuPopup(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HMENU menu = reinterpret_cast<HMENU>(wParam);
	for(auto& i: paths) {
		if(i.first->m_hMenu == menu) {
			// We'll load the menu here because GetUIObjectOf requires disk access

#define check(x) if(!(x)) { i.first->appendItem(TSTRING_F(SHELL_MENU_FAILED, Text::toT(i.second)), nullptr, OMenu::FLAG_DISABLED); break; }

			// get IShellFolder interface of Desktop (root of Shell namespace)
			IShellFolder* desktop = 0;
			HRESULT hr = ::SHGetDesktopFolder(&desktop);
			check(hr == S_OK && desktop);

			// get interface to IMalloc used to free PIDLs
			LPMALLOC lpMalloc = 0;
			hr = ::SHGetMalloc(&lpMalloc);
			check(hr == S_OK && lpMalloc);

			// ParseDisplayName creates a PIDL from a file system path relative to the IShellFolder interface
			// but since we use the Desktop as our interface and the Desktop is the namespace root
			// that means that it's a fully qualified PIDL, which is what we need
			LPITEMIDLIST pidl = 0;
			hr = desktop->ParseDisplayName(0, 0, const_cast<LPWSTR>(Text::utf8ToWide(i.second).c_str()), 0, &pidl, 0);
			check(hr == S_OK && pidl);

			// get the parent IShellFolder interface of pidl and the relative PIDL
			IShellFolder* folder = 0;
			LPCITEMIDLIST pidlItem = 0;
			hr = ::SHBindToParent(pidl, IID_IShellFolder, reinterpret_cast<LPVOID*>(&folder), &pidlItem);
			check(hr == S_OK && folder && pidlItem);

			// first we retrieve the normal IContextMenu interface (every object should have it)
			LPCONTEXTMENU handler1 = 0;
			hr = folder->GetUIObjectOf(0, 1, &pidlItem, IID_IContextMenu, 0, reinterpret_cast<LPVOID*>(&handler1));
			folder->Release();
			check(hr == S_OK && handler1);

			// then try to get the version 3 interface
			LPCONTEXTMENU3 handler3 = 0;
			hr = handler1->QueryInterface(IID_IContextMenu3, reinterpret_cast<LPVOID*>(&handler3));
			handler1->Release();
			check(hr == S_OK && handler3);

#undef check
			lpMalloc->Free(pidl);
			lpMalloc->Release();

			desktop->Release();

			curHandler = make_unique<Handler>(handler3);
			handler3->QueryContextMenu(i.first->m_hMenu, 0, ID_SHELLCONTEXTMENU_MIN, ID_SHELLCONTEXTMENU_MAX, CMF_NORMAL | CMF_EXPLORE);
			break;
		}
	}

	return dispatch(uMsg, wParam, lParam, bHandled);
}

LRESULT ShellMenu::handleUnInitMenuPopup(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HMENU menu = reinterpret_cast<HMENU>(wParam);
	for (auto& i : paths) {
		if (i.first->m_hMenu == menu) {
			i.first->ClearMenu();
			break;
		}
	}

	return dispatch(uMsg, wParam, lParam, bHandled);
}

LRESULT ShellMenu::handleMenuSelect(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	// make sure this isn't a "menu closed" signal
	if((HIWORD(wParam) == 0xFFFF) && (lParam == 0))
		return false;

	// save the currently selected id in case we need to dispatch it later on
	sel_id = LOWORD(wParam);
	return false;
}

LRESULT ShellMenu::dispatch(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	LRESULT ret;
	if(curHandler) {
		curHandler->getMenu()->HandleMenuMsg2(uMsg, wParam, lParam, &ret);
		return TRUE;
	}

	bHandled = FALSE;
	return FALSE;
}
}