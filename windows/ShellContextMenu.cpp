/*
* Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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

// Based on <http://www.codeproject.com/shell/shellcontextmenu.asp> by R. Engels.

#include "stdafx.h"
#include "ShellContextMenu.h"

#include "resource.h"
#include "WinUtil.h"

ShellMenu* ShellMenu::curMenu = nullptr;
LPCONTEXTMENU3 ShellMenu::handler = nullptr;
unsigned ShellMenu::sel_id = 0; 
std::vector<std::pair<OMenu*, LPCONTEXTMENU3>> ShellMenu::handlers;

ShellMenu::ShellMenu()
//handler(0),
//sel_id(0)
{
}

void ShellMenu::appendShellMenu(const StringList& paths) {
	curMenu = this;
#define check(x) if(!(x)) { return; }

	// get IShellFolder interface of Desktop (root of Shell namespace)
	IShellFolder* desktop = 0;
	HRESULT hr = ::SHGetDesktopFolder(&desktop);
	check(hr == S_OK && desktop);

	// get interface to IMalloc used to free PIDLs
	LPMALLOC lpMalloc = 0;
	hr = ::SHGetMalloc(&lpMalloc);
	check(hr == S_OK && lpMalloc);

#undef check
#define check(x) if(!(x)) { continue; }

	// stores allocated PIDLs to free them afterwards.
	typedef std::vector<LPITEMIDLIST> pidls_type;
	pidls_type pidls;

	// stores paths for which we have managed to get a valid IContextMenu3 interface.
	typedef std::pair<string, LPCONTEXTMENU3> valid_pair;
	typedef std::vector<valid_pair> valid_type;
	valid_type valid;

	for(auto& i: paths) {
		// ParseDisplayName creates a PIDL from a file system path relative to the IShellFolder interface
		// but since we use the Desktop as our interface and the Desktop is the namespace root
		// that means that it's a fully qualified PIDL, which is what we need
		LPITEMIDLIST pidl = 0;
		hr = desktop->ParseDisplayName(0, 0, const_cast<LPWSTR>(Text::utf8ToWide(i).c_str()), 0, &pidl, 0);
		check(hr == S_OK && pidl);
		pidls.push_back(pidl);

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

		valid.emplace_back(i, handler3);
	}

#undef check

	for(auto& i: pidls)
		lpMalloc->Free(i);
	lpMalloc->Release();

	desktop->Release();

	if(valid.empty())
		return;

	appendSeparator();

	if (valid.size() == 1) {
		handlers.emplace_back(createSubMenu(TSTRING(SHELL_MENU), true, true), valid[0].second);
		appendItem(TSTRING(OPEN_FOLDER), [=] { WinUtil::openFolder(Text::toT(Util::getFilePath(valid[0].first))); }, OMenu::FLAG_THREADED);
	} else {
		auto sh = createSubMenu(TSTRING(SHELL_MENUS));
		for(auto& i: valid)
			handlers.emplace_back(sh->createSubMenu(Text::toT(i.first)), i.second);

		auto fo = createSubMenu(TSTRING(OPEN_FOLDER));
		for (auto& i : valid)
			fo->appendItem(Text::toT(Util::getFilePath(i.first)), [=] { WinUtil::openFolder(Text::toT(i.first)); }, OMenu::FLAG_THREADED);
	}
}

ShellMenu::~ShellMenu() {
	curMenu = nullptr;
	handler = nullptr;
	sel_id = 0;
	for(auto& i: handlers)
		i.second->Release();
	handlers.clear();
}

void ShellMenu::open(HWND aHWND, unsigned flags, CPoint pt) {
	BaseType::open(aHWND, flags, pt);

	if(sel_id >= ID_SHELLCONTEXTMENU_MIN && sel_id <= ID_SHELLCONTEXTMENU_MAX && handler) {
		CMINVOKECOMMANDINFO cmi = { sizeof(CMINVOKECOMMANDINFO) };
		cmi.lpVerb = (LPSTR)MAKEINTRESOURCE(sel_id - ID_SHELLCONTEXTMENU_MIN);
		cmi.nShow = SW_SHOWNORMAL;
		handler->InvokeCommand(&cmi);
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
	for(auto& i: handlers) {
		if(i.first->m_hMenu == menu) {
			handler = i.second;
			handler->QueryContextMenu(i.first->m_hMenu, 0, ID_SHELLCONTEXTMENU_MIN, ID_SHELLCONTEXTMENU_MAX, CMF_NORMAL | CMF_EXPLORE);
			break;
		}
	}

	return dispatch(uMsg, wParam, lParam, bHandled);
}

LRESULT ShellMenu::handleUnInitMenuPopup(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	HMENU menu = reinterpret_cast<HMENU>(wParam);
	for (auto& i : handlers) {
		if (i.first->m_hMenu == menu)
			i.first->ClearMenu();
	}

	return dispatch(uMsg, wParam, lParam, bHandled);
}

LRESULT ShellMenu::handleMenuSelect(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	// make sure this isn't a "menu closed" signal
	if((HIWORD(wParam) == 0xFFFF) && (lParam == 0))
		return false;

	// save the currently selected id in case we need to dispatch it later on
	sel_id = LOWORD(wParam);
	return false;
}

LRESULT ShellMenu::dispatch(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	LRESULT ret;
	if(handler) {
		handler->HandleMenuMsg2(uMsg, wParam, lParam, &ret);
		return TRUE;
	}

	bHandled = FALSE;
	return FALSE;
}