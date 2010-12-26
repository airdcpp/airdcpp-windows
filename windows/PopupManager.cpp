/* 
* Copyright (C) 2003-2005 Pär Björklund, per.bjorklund@gmail.com
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

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"
#include "../client/Util.h"

#include "WinUtil.h"
#include "PopupManager.h"
#include "MainFrm.h"


PopupManager* Singleton< PopupManager >::instance = NULL;

void PopupManager::Show(const tstring &aMsg, const tstring &aTitle, int Icon, bool preview /*= false*/) {
	if(!activated)
		return;

	if (!Util::getAway() && BOOLSETTING(POPUP_AWAY) && !preview)
		return;
	
	if(!MainFrame::getMainFrame()->getAppMinimized() && BOOLSETTING(POPUP_MINIMIZED) && !preview) {
		return;
	}

	tstring msg = aMsg;
	if(int(aMsg.length()) > SETTING(MAX_MSG_LENGTH)) {
		msg = aMsg.substr(0, (SETTING(MAX_MSG_LENGTH) - 3));
		msg += _T("...");
	}

	if(SETTING(POPUP_TYPE) == BALLOON) {
		NOTIFYICONDATA m_nid;
		m_nid.cbSize = sizeof(NOTIFYICONDATA);
		m_nid.hWnd = MainFrame::getMainFrame()->m_hWnd;
		m_nid.uID = 0;
		m_nid.uFlags = NIF_INFO;
		m_nid.uTimeout = (SETTING(POPUP_TIME) * 1000);
		m_nid.dwInfoFlags = Icon;
		_tcscpy(m_nid.szInfo, msg.c_str());
		_tcscpy(m_nid.szInfoTitle, aTitle.c_str());
		Shell_NotifyIcon(NIM_MODIFY, &m_nid);
		return;
	}

	if(PopupImage != SETTING(POPUPFILE) || popuptype != SETTING(POPUP_TYPE)) {
		PopupImage = SETTING(POPUPFILE);
		popuptype = SETTING(POPUP_TYPE);
		hBitmap = (HBITMAP)::LoadImage(NULL, (Text::toT(PopupImage).c_str()), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
		if(hBitmap != NULL && SETTING(POPUP_TYPE) == CUSTOM) {
			BITMAP bm;
			GetObject(hBitmap,sizeof(bm),&bm);
			height = (uint16_t)bm.bmHeight;
			width = (uint16_t)bm.bmWidth;
		} else if(SETTING(POPUP_TYPE) != CUSTOM) {
			height = 90;
			width = 200;
		}
	}
	
	CRect rcDesktop;
	
	//get desktop rect so we know where to place the popup
	::SystemParametersInfo(SPI_GETWORKAREA,0,&rcDesktop,0);
	
	int screenHeight = rcDesktop.bottom;
	int screenWidth = rcDesktop.right;

	//if we have popups all the way up to the top of the screen do not create a new one
	if( (offset + height) > screenHeight)
		return;
	
	//get the handle of the window that has focus
	HWND gotFocus = ::SetFocus(WinUtil::mainWnd);
	
	//compute the window position
	CRect rc(screenWidth - width , screenHeight - height - offset, screenWidth, screenHeight - offset);
	
	//Create a new popup
	PopupWnd *p = new PopupWnd(msg, aTitle, rc, id++, hBitmap);
	p->height = height;	// save the height, for removal
	
	if(SETTING(POPUP_TYPE) != CUSTOM) {
		if(LOBYTE(LOWORD(GetVersion())) >= 5) {
			p->SetWindowLong(GWL_EXSTYLE, p->GetWindowLong(GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT);
			typedef bool (CALLBACK* LPFUNC)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
			LPFUNC _d_SetLayeredWindowAttributes = (LPFUNC)GetProcAddress(LoadLibrary(_T("user32")), "SetLayeredWindowAttributes");
			_d_SetLayeredWindowAttributes(p->m_hWnd, 0, 200, LWA_ALPHA);
		}
	}

	//move the window to the top of the z-order and display it
	p->SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	p->ShowWindow(SW_SHOWNA);
		
	//restore focus to window
	::SetFocus(gotFocus);
	
	//increase offset so we know where to place the next popup
	offset = offset + height;

	popups.push_back(p);
}

void PopupManager::on(TimerManagerListener::Second /*type*/, uint64_t /*tick*/) {
	//post a message and let the main window thread take care of the window
	::PostMessage(WinUtil::mainWnd, WM_SPEAKER, MainFrame::REMOVE_POPUP, 0);
}


void PopupManager::AutoRemove(){
	//we got nothing to do here
	if(popups.empty()) {
		return;
	}

	//check all popups and see if we need to remove anyone
	PopupList::const_iterator i = popups.begin();
	for(; i != popups.end(); ++i) {

		if((*i)->visible + SETTING(POPUP_TIME) * 1000 < GET_TICK()) {
			//okay remove the first popup
			Remove((*i)->id);

			//if list is empty there is nothing more to do
			if(popups.empty())
				return;
				
			//start over from the beginning
			i = popups.begin();
		}
	}
}

void PopupManager::Remove(uint32_t pos) {
	//find the correct window
	PopupIter i = popups.begin();

	for(; i != popups.end(); ++i) {
		if((*i)->id == pos)
			break;
	}

	dcassert(i != popups.end());	

	//remove the window from the list
	PopupWnd *p = (*i);
	i = popups.erase(i);
	
	if(p == NULL){
		return;
	}
	
	//close the window and delete it, ensure that correct height is used from here
	height = p->height;
	p->SendMessage(WM_CLOSE, 0, 0);
	delete p;
	p = NULL;
	 
	//set offset one window position lower
	dcassert(offset > 0);
	offset = offset - height;

	//nothing to do
	if(popups.empty())
		return;

	CRect rc;

	//move down all windows
	for(; i != popups.end(); ++i) {
		(*i)->GetWindowRect(rc);
		rc.top += height;
		rc.bottom += height;
		(*i)->MoveWindow(rc);
	}
}
