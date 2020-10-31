/*
 * Copyright (C) 2001-2021 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef STATICFRAME_H
#define STATICFRAME_H

#include "MainFrm.h"
#include "WinUtil.h"


template<class T, int title, int ID = -1>
class StaticFrame {
public:
	virtual ~StaticFrame() {
		frame = nullptr;
	}

	static T* frame;
	static void openWindow() {
		if (!frame) {
			frame = new T();
			frame->CreateEx(WinUtil::mdiClient, frame->rcDefault, CTSTRING_I(ResourceManager::Strings(title)));
			WinUtil::setButtonPressed(ID, true);
		}
		else {
			// match the behavior of MainFrame::onSelected()
			HWND hWnd = frame->m_hWnd;
			if (isMDIChildActive(hWnd)) {
				::PostMessage(hWnd, WM_CLOSE, NULL, NULL);
			}
			else if (frame->MDIGetActive() != hWnd) {
				MainFrame::getMainFrame()->MDIActivate(hWnd);
				WinUtil::setButtonPressed(ID, true);
			}
			else if (SETTING(TOGGLE_ACTIVE_WINDOW)) {
				::SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
				frame->MDINext(hWnd);
				hWnd = frame->MDIGetActive();
				WinUtil::setButtonPressed(ID, true);
			}
			if (::IsIconic(hWnd))
				::ShowWindow(hWnd, SW_RESTORE);
		}
	}
	static bool isMDIChildActive(HWND hWnd) {
		HWND wnd = MainFrame::getMainFrame()->MDIGetActive();
		dcassert(!!wnd);
		return (hWnd == wnd);
	}

	static bool parseWindowParams(StringMap& params) {
		if (params["id"] == T::id) {
			MainFrame::getMainFrame()->callAsync([=] { T::openWindow(); });
			return true;
		}
		return false;
	}

	static bool getWindowParams(HWND hWnd, StringMap& params) {
		if (!!frame && hWnd == frame->m_hWnd) {
			params["id"] = T::id;
			return true;
		}
		return false;
	}
};

template<class T, int title, int ID> T* StaticFrame<T, title, ID>::frame = nullptr;

#endif // !defined(STATICFRAME_H)