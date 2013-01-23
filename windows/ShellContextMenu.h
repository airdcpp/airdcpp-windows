/*
* Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
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

/*
* Based on a class by R. Engels
* http://www.codeproject.com/shell/shellcontextmenu.asp
*/

#ifndef DCPLUSPLUS_WIN32_SHELL_CONTEXT_MENU_H
#define DCPLUSPLUS_WIN32_SHELL_CONTEXT_MENU_H

#include "stdafx.h"
#include "../client/typedefs.h"
#include "OMenu.h"

class CShellContextMenu
{
	static IContextMenu2* g_IContext2;
	static IContextMenu3* g_IContext3;
	static WNDPROC OldWndProc;

public:
	CShellContextMenu();
	~CShellContextMenu();

	void SetPath(const tstring& strPath);
	OMenu* GetMenu();
	UINT ShowContextMenu(HWND hWnd, CPoint pt);

private:
	bool bDelete;
	OMenu* m_Menu;
	IShellFolder* m_psfFolder;
	LPITEMIDLIST* m_pidlArray;

	void FreePIDLArray(LPITEMIDLIST* pidlArray);
	// this functions determines which version of IContextMenu is available for those objects(always the highest one)
	// and returns that interface
	bool GetContextMenu(LPVOID* ppContextMenu, int& iMenuType);

	void InvokeCommand(LPCONTEXTMENU pContextMenu, UINT idCommand);

	static LRESULT CALLBACK HookWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};


#endif // !defined(DCPLUSPLUS_WIN32_SHELL_CONTEXT_MENU_H)
