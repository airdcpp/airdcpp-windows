/* 
 * Copyright (C) 2011-2013 AirDC++ Project
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

#ifndef DCPLUSPLUS_ASYNC_H_
#define DCPLUSPLUS_ASYNC_H_

#include "Dispatchers.h"

#include <functional>

#include "stdafx.h"

namespace dcpp {

static void callAsync(HWND m_hWnd, std::function<void ()> f) {
	PostMessage(m_hWnd, WM_SPEAKER, NULL, (LPARAM)new Dispatcher::F(f));
}

template<class Parent>
class Async {

public:
	Async() {}
	void callAsync(std::function<void ()> f) {
		PostMessage(((Parent*)this)->m_hWnd, WM_SPEAKER, NULL, (LPARAM)new Dispatcher::F(f));
	}

	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		auto f = reinterpret_cast<Dispatcher::F*>(lParam);
		(*f)();
		delete f;

		return 0;
	}
};

}

#endif // DCPLUSPLUS_ASYNC_H_