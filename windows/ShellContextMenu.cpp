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

#include "stdafx.h"

#include "ShellContextMenu.h"
#include "resource.h"

IContextMenu2* CShellContextMenu::g_IContext2 = NULL;
IContextMenu3* CShellContextMenu::g_IContext3 = NULL;
WNDPROC CShellContextMenu::OldWndProc = NULL;

CShellContextMenu::CShellContextMenu() :
	bDelete(false),
	m_psfFolder(NULL),
	m_pidlArray(NULL),
	m_Menu(NULL)
{
}

CShellContextMenu::~CShellContextMenu() {
	// free all allocated datas
	if(m_psfFolder && bDelete)
		m_psfFolder->Release();
	m_psfFolder = NULL;
	FreePIDLArray(m_pidlArray);
	m_pidlArray = NULL;

	if(m_Menu)
		delete m_Menu;
}

void CShellContextMenu::SetPath(const tstring& strPath)
{
	// free all allocated datas
	if(m_psfFolder && bDelete)
		m_psfFolder->Release();
	m_psfFolder = NULL;
	FreePIDLArray(m_pidlArray);
	m_pidlArray = NULL;

	// get IShellFolder interface of Desktop(root of shell namespace)
	IShellFolder* psfDesktop = NULL;
	SHGetDesktopFolder(&psfDesktop);

	// ParseDisplayName creates a PIDL from a file system path relative to the IShellFolder interface
	// but since we use the Desktop as our interface and the Desktop is the namespace root
	// that means that it's a fully qualified PIDL, which is what we need
	LPITEMIDLIST pidl = NULL;
	psfDesktop->ParseDisplayName(NULL, 0, (LPOLESTR)const_cast<TCHAR*>(strPath.c_str()), NULL, &pidl, NULL);

	// now we need the parent IShellFolder interface of pidl, and the relative PIDL to that interface
	typedef HRESULT (CALLBACK* LPFUNC)(LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlLast);
	LPFUNC MySHBindToParent = (LPFUNC)GetProcAddress(LoadLibrary(_T("shell32")), "SHBindToParent");
	if(MySHBindToParent == NULL) return;

	MySHBindToParent(pidl, IID_IShellFolder, (LPVOID*)&m_psfFolder, NULL);

	// get interface to IMalloc (need to free the PIDLs allocated by the shell functions)
	LPMALLOC lpMalloc = NULL;
	SHGetMalloc(&lpMalloc);
	lpMalloc->Free(pidl);

	// now we need the relative pidl
	IShellFolder* psfFolder = NULL;
	HRESULT res = psfDesktop->ParseDisplayName (NULL, 0, (LPOLESTR)const_cast<TCHAR*>(strPath.c_str()), NULL, &pidl, NULL);
	if(res != S_OK) return;

	LPITEMIDLIST pidlItem = NULL;
	MySHBindToParent(pidl, IID_IShellFolder, (LPVOID*)&psfFolder, (LPCITEMIDLIST*)&pidlItem);

	// copy pidlItem to m_pidlArray
	m_pidlArray = (LPITEMIDLIST *) realloc(m_pidlArray, sizeof (LPITEMIDLIST));
	int nSize = 0;
	LPITEMIDLIST pidlTemp = pidlItem;
	while(pidlTemp->mkid.cb)
	{
		nSize += pidlTemp->mkid.cb;
		pidlTemp = (LPITEMIDLIST) (((LPBYTE) pidlTemp) + pidlTemp->mkid.cb);
	}
	LPITEMIDLIST pidlRet = (LPITEMIDLIST) calloc(nSize + sizeof (USHORT), sizeof (BYTE));
	CopyMemory(pidlRet, pidlItem, nSize);
	m_pidlArray[0] = pidlRet;

	//free(pidlItem);
	lpMalloc->Free(pidl);

	lpMalloc->Release();
	psfFolder->Release();
	psfDesktop->Release();

	bDelete = true;	// indicates that m_psfFolder should be deleted by CShellContextMenu
}

OMenu* CShellContextMenu::GetMenu()
{
	if(!m_Menu)
	{
		m_Menu = new OMenu;
		m_Menu->CreatePopupMenu();	// create the popupmenu(its empty)
	}
	return m_Menu;
}

UINT CShellContextMenu::ShowContextMenu(HWND hWnd, CPoint pt)
{
	int iMenuType = 0;	// to know which version of IContextMenu is supported
	LPCONTEXTMENU pContextMenu;	// common pointer to IContextMenu and higher version interface

	if(!GetContextMenu((LPVOID*)&pContextMenu, iMenuType))	
		return 0;	// something went wrong

	if(!m_Menu)
	{
		delete m_Menu;
		m_Menu = NULL;
		m_Menu = new OMenu;
		m_Menu->CreatePopupMenu();
	}

	// lets fill the popupmenu 
	pContextMenu->QueryContextMenu(m_Menu->m_hMenu, m_Menu->GetMenuItemCount(), ID_SHELLCONTEXTMENU_MIN, ID_SHELLCONTEXTMENU_MAX, CMF_NORMAL | CMF_EXPLORE);

	// subclass window to handle menurelated messages in CShellContextMenu 
	if(iMenuType > 1)	// only subclass if its version 2 or 3
	{
		OldWndProc = (WNDPROC) SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookWndProc));
		if(iMenuType == 2)
			g_IContext2 = (LPCONTEXTMENU2) pContextMenu;
		else	// version 3
			g_IContext3 = (LPCONTEXTMENU3) pContextMenu;
	}
	else
		OldWndProc = NULL;

	UINT idCommand = m_Menu->TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN, pt.x, pt.y, hWnd);

	if(OldWndProc) // unsubclass
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(OldWndProc));

	if(idCommand >= ID_SHELLCONTEXTMENU_MIN && idCommand <= ID_SHELLCONTEXTMENU_MAX)
	{
		InvokeCommand(pContextMenu, idCommand - ID_SHELLCONTEXTMENU_MIN);
		idCommand = 0;
	}

	pContextMenu->Release();
	g_IContext2 = NULL;
	g_IContext3 = NULL;

	return idCommand;
}

void CShellContextMenu::FreePIDLArray(LPITEMIDLIST* pidlArray)
{
	if(!pidlArray)
		return;

	int iSize = _msize (pidlArray) / sizeof (LPITEMIDLIST);

	for(int i = 0; i < iSize; i++)
		free(pidlArray[i]);
	free(pidlArray);
}

// this functions determines which version of IContextMenu is avaibale for those objects(always the highest one)
// and returns that interface
bool CShellContextMenu::GetContextMenu(LPVOID* ppContextMenu, int& iMenuType)
{
	if(m_pidlArray == NULL) return false;

	*ppContextMenu = NULL;
	LPCONTEXTMENU icm1 = NULL;

	// first we retrieve the normal IContextMenu interface(every object should have it)
	m_psfFolder->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST *) m_pidlArray, IID_IContextMenu, NULL, (LPVOID*) &icm1);

	if(icm1)
	{	// since we got an IContextMenu interface we can now obtain the higher version interfaces via that
		if(icm1->QueryInterface(IID_IContextMenu3, ppContextMenu) == NOERROR)
			iMenuType = 3;
		else if(icm1->QueryInterface(IID_IContextMenu2, ppContextMenu) == NOERROR)
			iMenuType = 2;

		if(*ppContextMenu) 
			icm1->Release(); // we can now release version 1 interface, cause we got a higher one
		else 
		{	
			iMenuType = 1;
			*ppContextMenu = icm1;	// since no higher versions were found
		}							// redirect ppContextMenu to version 1 interface

		return true; // success
	}

	return false;	// something went wrong
}

void CShellContextMenu::InvokeCommand(LPCONTEXTMENU pContextMenu, UINT idCommand)
{
	CMINVOKECOMMANDINFO cmi = {0};
	cmi.cbSize = sizeof(CMINVOKECOMMANDINFO);
	cmi.lpVerb = (LPSTR) MAKEINTRESOURCE(idCommand);
	cmi.nShow = SW_SHOWNORMAL;

	pContextMenu->InvokeCommand(&cmi);
}

LRESULT CALLBACK CShellContextMenu::HookWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{ 
	case WM_MENUCHAR:	// only supported by IContextMenu3
		if(g_IContext3)
		{
			LRESULT lResult = 0;
			g_IContext3->HandleMenuMsg2(message, wParam, lParam, &lResult);
			return lResult;
		}
		break;

	case WM_DRAWITEM:
	case WM_MEASUREITEM:
		if(wParam) 
			break; // if wParam != 0 then the message is not menu-related

	case WM_INITMENUPOPUP:
		if(g_IContext2)
			g_IContext2->HandleMenuMsg(message, wParam, lParam);
		else	// version 3
			g_IContext3->HandleMenuMsg(message, wParam, lParam);
		return (message == WM_INITMENUPOPUP) ? 0 : TRUE; // inform caller that we handled WM_INITPOPUPMENU by ourself
		break;

	default:
		break;
	}

	// call original WndProc of window to prevent undefined bevhaviour of window
	return ::CallWindowProc(OldWndProc, hWnd, message, wParam, lParam);
}

