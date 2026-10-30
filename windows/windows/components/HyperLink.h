/*
 * Copyright (C) 2011-2024 AirDC++ Project
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

#ifndef HYPERLINK_H
#define HYPERLINK_H

#include <windows/stdafx.h>
#include <windows/util/DarkMode.h>

namespace wingui {

// Hyperlink with colors that are readable with the current window theme
class HyperLink : public CHyperLinkImpl<HyperLink> {
public:
	DECLARE_WND_CLASS(_T("AirDC_HyperLink"))

	// Called when the window is created or subclassed
	void Init() {
		CHyperLinkImpl<HyperLink>::Init();

		// The default colors (also read from the registry by the base class) aren't readable on dark backgrounds
		if (DarkMode::isEnabled()) {
			m_clrLink = RGB(100, 170, 255);
			m_clrVisited = RGB(190, 150, 255);
		}
	}
};

}

#endif // !defined(HYPERLINK_H)
