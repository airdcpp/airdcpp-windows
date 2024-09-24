/* 
* Copyright (C) 2003-2005 P�r Bj�rklund, per.bjorklund@gmail.com
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

#ifndef POPUPMANAGER_H
#define POPUPMANAGER_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// we want to get rid of those damn warnings, hosting is not needed here anyways
#define _ATL_NO_HOSTING
#define _ATL_NO_DOCHOSTUIHANDLER

#include <windows/stdafx.h>

#include <airdcpp/GetSet.h>
#include <airdcpp/Singleton.h>
#include <airdcpp/TimerManagerListener.h>

#include <windows/PopupDlg.h>

namespace wingui {
class PopupManager : public Singleton< PopupManager >, private TimerManagerListener
{
public:
	PopupManager();
	~PopupManager();

	enum { BALLOON, CUSTOM, SPLASH, WINDOW };
	
	//call this with a preformatted message
	void Show(const tstring &aMsg, const tstring &aTitle, int Icon, HICON hIcon, bool force);

	//remove first popup in list and move everyone else
	void Remove(uint32_t pos = 0);

	//remove the popups that are scheduled to be removed
	void AutoRemove();
	
	void Mute(bool mute) { activated = !mute; }

	IGETSET(bool, creating, Creating, false);
private:
	typedef list< PopupWnd* > PopupList;
	typedef PopupList::iterator PopupIter;
	PopupList popups;
	
	typedef bool (CALLBACK* LPFUNC)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
	LPFUNC _d_SetLayeredWindowAttributes;
	HMODULE user32lib;
	
	
	//size of the popup window
	uint16_t height;
	uint16_t width;

	//if we have multiple windows displayed, 
	//keep track of where the new one will be displayed
	uint16_t offset;
	
	//turn on/off popups completely
	bool activated;

	//for custom popups
	HBITMAP hBitmap;
	string PopupImage;
	int popuptype;

	//id of the popup to keep track of them
	uint32_t id;
 	
	// TimerManagerListener
	void on(TimerManagerListener::Second, uint64_t tick) noexcept;

};
}

#endif