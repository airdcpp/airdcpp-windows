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

#include <windows/stdafx.h>

#include <windows/util/TextZoom.h>
#include <windows/util/WinUtil.h>

#include <airdcpp/settings/SettingsManager.h>

namespace wingui {

const UINT TextZoom::WM_TEXT_ZOOM_CHANGED = ::RegisterWindowMessage(_T("AirDCTextZoomChanged"));

std::set<HWND> TextZoom::richEdits;
std::map<HWND, HFONT> TextZoom::edits;
std::map<HFONT, TextZoom::ScaledFont> TextZoom::fonts;
int TextZoom::wheelDelta = 0;

void TextZoom::addRichEdit(HWND aWnd) noexcept {
	removeClosed();
	richEdits.insert(aWnd);
	apply(aWnd);
}

void TextZoom::addEdit(HWND aWnd, HFONT aBaseFont) noexcept {
	removeClosed();
	edits[aWnd] = aBaseFont;
	apply(aWnd);
}

void TextZoom::remove(HWND aWnd) noexcept {
	richEdits.erase(aWnd);
	edits.erase(aWnd);
}

void TextZoom::removeClosed() noexcept {
	std::erase_if(richEdits, [](HWND w) { return !::IsWindow(w); });
	std::erase_if(edits, [](const auto& e) { return !::IsWindow(e.first); });
}

HFONT TextZoom::getFont(HFONT aBaseFont) noexcept {
	auto zoom = SETTING(TEXT_ZOOM);
	if (zoom == 100) {
		return aBaseFont;
	}

	if (auto i = fonts.find(aBaseFont); i != fonts.end()) {
		return i->second.font;
	}

	LOGFONT lf = { 0 };
	if (!::GetObject(aBaseFont, sizeof(lf), &lf)) {
		return aBaseFont;
	}

	lf.lfHeight = MulDiv(lf.lfHeight, zoom, 100);
	auto font = ::CreateFontIndirect(&lf);
	if (!font) {
		return aBaseFont;
	}

	fonts[aBaseFont] = { font, WinUtil::getTextHeight(WinUtil::mainWnd, font) };
	return font;
}

int TextZoom::getTextHeight(HFONT aBaseFont) noexcept {
	auto font = getFont(aBaseFont);
	if (font == aBaseFont) {
		return aBaseFont == WinUtil::font ? WinUtil::fontHeight : WinUtil::getTextHeight(WinUtil::mainWnd, aBaseFont);
	}

	return fonts[aBaseFont].height;
}

void TextZoom::apply(HWND aWnd) noexcept {
	if (richEdits.contains(aWnd)) {
		// 0/0 turns the zooming off
		auto zoom = SETTING(TEXT_ZOOM);
		::SendMessage(aWnd, EM_SETZOOM, zoom == 100 ? 0 : zoom, zoom == 100 ? 0 : 100);
	} else if (auto i = edits.find(aWnd); i != edits.end()) {
		::SendMessage(aWnd, WM_SETFONT, reinterpret_cast<WPARAM>(getFont(i->second)), TRUE);
	}
}

void TextZoom::setZoom(int aZoom) noexcept {
	if (aZoom == SETTING(TEXT_ZOOM)) {
		return;
	}

	SettingsManager::getInstance()->set(SettingsManager::TEXT_ZOOM, aZoom);

	// The old fonts can be deleted after all edits have got the new ones
	auto oldFonts = std::move(fonts);
	fonts.clear();

	removeClosed();
	for (auto w: richEdits) {
		apply(w);
	}

	std::set<HWND> parents;
	for (const auto& e: edits) {
		apply(e.first);
		parents.insert(::GetParent(e.first));
	}

	for (auto p: parents) {
		::SendMessage(p, WM_TEXT_ZOOM_CHANGED, 0, 0);
	}

	for (const auto& f: oldFonts) {
		::DeleteObject(f.second.font);
	}
}

bool TextZoom::preTranslateMessage(const MSG* aMsg) noexcept {
	if (aMsg->message != WM_MOUSEWHEEL && aMsg->message != WM_MBUTTONDOWN && aMsg->message != WM_MBUTTONUP) {
		return false;
	}

	if (!(GET_KEYSTATE_WPARAM(aMsg->wParam) & MK_CONTROL) || ::GetKeyState(VK_MENU) >= 0) {
		return false;
	}

	if (!richEdits.contains(aMsg->hwnd) && !edits.contains(aMsg->hwnd)) {
		return false;
	}

	if (aMsg->message == WM_MOUSEWHEEL) {
		// High resolution wheels send smaller steps
		wheelDelta += GET_WHEEL_DELTA_WPARAM(aMsg->wParam);
		auto steps = wheelDelta / WHEEL_DELTA;
		if (steps != 0) {
			wheelDelta -= steps * WHEEL_DELTA;
			setZoom(std::clamp(SETTING(TEXT_ZOOM) + steps * 10, 50, 400));
		}
	} else if (aMsg->message == WM_MBUTTONDOWN) {
		setZoom(100);
	}

	return true;
}

}
