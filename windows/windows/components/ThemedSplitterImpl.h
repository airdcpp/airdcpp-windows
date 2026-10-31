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

#ifndef THEMED_SPLITTER_IMPL_H
#define THEMED_SPLITTER_IMPL_H

#include <windows/stdafx.h>
#include <windows/util/DarkMode.h>

namespace wingui {

// Splitter that uses the colors of the current window theme
template <class T>
class ThemedSplitterImpl : public CSplitterImpl<T> {
public:
	ThemedSplitterImpl(bool bVertical = true) : CSplitterImpl<T>(bVertical) { }

	void DrawSplitterBar(CDCHandle dc) {
		if (!DarkMode::isEnabled()) {
			CSplitterImpl<T>::DrawSplitterBar(dc);
			return;
		}

		RECT rect = { 0 };
		if (this->GetSplitterBarRect(&rect)) {
			dc.FillRect(&rect, DarkMode::getSysColorBrush(COLOR_3DFACE));
		}
	}
};

}

#endif // !defined(THEMED_SPLITTER_IMPL_H)
