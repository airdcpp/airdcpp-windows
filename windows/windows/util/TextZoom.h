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
#ifndef DCPLUSPLUS_WIN32_TEXT_ZOOM_H
#define DCPLUSPLUS_WIN32_TEXT_ZOOM_H

namespace wingui {

// Common text size (TEXT_ZOOM setting) of the chat and the other text windows
//
// Ctrl + Alt + mouse wheel over a registered control changes the size of all registered controls
// and Ctrl + Alt + middle click resets it. Ctrl + mouse wheel remains the zooming of a single
// rich edit control.
class TextZoom {
public:
	// Sent to the parent of a registered edit after its font has changed (for updating the layout)
	static const UINT WM_TEXT_ZOOM_CHANGED;

	// Rich edit controls are zoomed with EM_SETZOOM (the zoom is applied again if the control has been registered already)
	static void addRichEdit(HWND aWnd) noexcept;

	// Edit controls get a scaled version of their font
	static void addEdit(HWND aWnd, HFONT aBaseFont) noexcept;

	static void remove(HWND aWnd) noexcept;

	// Scaled version of the font and its text height
	static HFONT getFont(HFONT aBaseFont) noexcept;
	static int getTextHeight(HFONT aBaseFont) noexcept;

	// Call from the message loop, returns true if the message was handled
	static bool preTranslateMessage(const MSG* aMsg) noexcept;
private:
	static void setZoom(int aZoom) noexcept;
	static void apply(HWND aWnd) noexcept;
	static void removeClosed() noexcept;

	struct ScaledFont {
		HFONT font;
		int height;
	};

	static std::set<HWND> richEdits;
	static std::map<HWND, HFONT> edits;
	static std::map<HFONT, ScaledFont> fonts;
	static int wheelDelta;
};

}

#endif
