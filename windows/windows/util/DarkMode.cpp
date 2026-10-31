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

#include <windows/util/DarkMode.h>

#include <airdcpp/settings/SettingsManager.h>

#include <dwmapi.h>
#include <vssym32.h>

#pragma comment(lib, "dwmapi.lib")

namespace wingui {

bool DarkMode::enabled = false;

namespace {

// Undocumented uxtheme.dll exports (by ordinal), available since Windows 10 1809
enum class PreferredAppMode { Default, AllowDark, ForceDark, ForceLight };

using SetPreferredAppModeF = PreferredAppMode(WINAPI*)(PreferredAppMode);
using AllowDarkModeForWindowF = bool(WINAPI*)(HWND, bool);
using ShouldAppsUseDarkModeF = bool(WINAPI*)();
using FlushMenuThemesF = void(WINAPI*)();
using RefreshImmersiveColorPolicyStateF = void(WINAPI*)();

SetPreferredAppModeF setPreferredAppMode = nullptr;
AllowDarkModeForWindowF allowDarkModeForWindow = nullptr;
ShouldAppsUseDarkModeF shouldAppsUseDarkMode = nullptr;
FlushMenuThemesF flushMenuThemes = nullptr;
RefreshImmersiveColorPolicyStateF refreshImmersiveColorPolicyState = nullptr;

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

const UINT_PTR SUBCLASS_ID = 0x44524B4D; // 'DRKM'
const UINT WM_APPLY_THEME = RegisterWindowMessage(_T("AirDCApplyWindowTheme"));

HHOOK hook = nullptr;
HINSTANCE user32 = nullptr;
HMODULE comctl32 = nullptr;

// Colors of the dark theme
const COLORREF DARK_FACE = RGB(32, 32, 32);
const COLORREF DARK_CONTROL = RGB(45, 45, 45);
const COLORREF DARK_WORKSPACE = RGB(24, 24, 24);
const COLORREF DARK_TEXT = RGB(230, 230, 230);
const COLORREF DARK_DISABLED_TEXT = RGB(130, 130, 130);
const COLORREF DARK_EDGE = RGB(75, 75, 75);
const COLORREF DARK_SHADOW = RGB(60, 60, 60);
const COLORREF DARK_DARK_SHADOW = RGB(20, 20, 20);
const COLORREF DARK_LIGHT = RGB(55, 55, 55);
const COLORREF DARK_MENU = RGB(43, 43, 43);

// Dark defaults for the color settings (customized colors are kept)
struct ColorDefault {
	SettingsManager::IntSetting setting;
	COLORREF dark;
	COLORREF light;
};

ColorDefault colorDefaults[] = {
	{ SettingsManager::BACKGROUND_COLOR, RGB(28, 28, 28) },
	{ SettingsManager::TEXT_COLOR, RGB(220, 220, 220) },

	{ SettingsManager::TEXT_GENERAL_BACK_COLOR, RGB(28, 28, 28) },
	{ SettingsManager::TEXT_GENERAL_FORE_COLOR, RGB(220, 220, 220) },
	{ SettingsManager::TEXT_MYOWN_BACK_COLOR, RGB(28, 28, 28) },
	{ SettingsManager::TEXT_MYOWN_FORE_COLOR, RGB(220, 220, 220) },
	{ SettingsManager::TEXT_PRIVATE_BACK_COLOR, RGB(28, 28, 28) },
	{ SettingsManager::TEXT_PRIVATE_FORE_COLOR, RGB(220, 220, 220) },
	{ SettingsManager::TEXT_SYSTEM_BACK_COLOR, RGB(28, 28, 28) },
	{ SettingsManager::TEXT_SYSTEM_FORE_COLOR, RGB(255, 140, 60) },
	{ SettingsManager::TEXT_SERVER_BACK_COLOR, RGB(28, 28, 28) },
	{ SettingsManager::TEXT_SERVER_FORE_COLOR, RGB(240, 150, 200) },
	{ SettingsManager::TEXT_TIMESTAMP_BACK_COLOR, RGB(28, 28, 28) },
	{ SettingsManager::TEXT_TIMESTAMP_FORE_COLOR, RGB(255, 110, 110) },
	{ SettingsManager::TEXT_MYNICK_BACK_COLOR, RGB(28, 28, 28) },
	{ SettingsManager::TEXT_MYNICK_FORE_COLOR, RGB(90, 200, 90) },
	{ SettingsManager::TEXT_FAV_BACK_COLOR, RGB(28, 28, 28) },
	{ SettingsManager::TEXT_FAV_FORE_COLOR, RGB(220, 220, 220) },
	{ SettingsManager::TEXT_OP_BACK_COLOR, RGB(28, 28, 28) },
	{ SettingsManager::TEXT_OP_FORE_COLOR, RGB(220, 220, 220) },
	{ SettingsManager::TEXT_NORM_BACK_COLOR, RGB(28, 28, 28) },
	{ SettingsManager::TEXT_NORM_FORE_COLOR, RGB(220, 220, 220) },
	{ SettingsManager::TEXT_URL_BACK_COLOR, RGB(28, 28, 28) },
	{ SettingsManager::TEXT_URL_FORE_COLOR, RGB(100, 170, 255) },
	{ SettingsManager::TEXT_SHARE_DUPE_BACK_COLOR, RGB(28, 28, 28) },
	{ SettingsManager::TEXT_QUEUE_DUPE_BACK_COLOR, RGB(28, 28, 28) },
	{ SettingsManager::LIST_HL_BG_COLOR, RGB(28, 28, 28) },

	{ SettingsManager::NORMAL_COLOUR, RGB(220, 220, 220) },
	{ SettingsManager::RESERVED_SLOT_COLOR, RGB(110, 190, 110) },
	{ SettingsManager::IGNORED_COLOR, RGB(110, 110, 110) },
	{ SettingsManager::FAVORITE_COLOR, RGB(120, 140, 255) },
	{ SettingsManager::PASSIVE_COLOR, RGB(150, 150, 150) },
	{ SettingsManager::OP_COLOR, RGB(100, 150, 255) },

	{ SettingsManager::TAB_ACTIVE_BG, RGB(55, 95, 130) },
	{ SettingsManager::TAB_ACTIVE_TEXT, RGB(240, 240, 240) },
	{ SettingsManager::TAB_ACTIVE_BORDER, RGB(20, 20, 20) },
	{ SettingsManager::TAB_INACTIVE_BG, RGB(45, 45, 45) },
	{ SettingsManager::TAB_INACTIVE_BG_DISCONNECTED, RGB(70, 80, 110) },
	{ SettingsManager::TAB_INACTIVE_TEXT, RGB(190, 190, 190) },
	{ SettingsManager::TAB_INACTIVE_BORDER, RGB(70, 70, 70) },
	{ SettingsManager::TAB_INACTIVE_BG_NOTIFY, RGB(95, 80, 115) },
};

// Window types that need special handling
enum WindowType : DWORD_PTR {
	TYPE_OTHER,
	TYPE_DIALOG,
	TYPE_BUTTON,
	TYPE_EDIT,
	TYPE_COMBO,
	TYPE_LISTBOX,
	TYPE_LISTVIEW,
	TYPE_TREEVIEW,
	TYPE_HEADER,
	TYPE_STATUSBAR,
	TYPE_REBAR,
	TYPE_TOOLTIP,
	TYPE_SCROLLBAR,
	TYPE_RICHEDIT,
	TYPE_TAB,
	TYPE_PROGRESS,
	TYPE_MESSAGEBOX,
	TYPE_TOOLBAR,
	TYPE_UPDOWN,
};

WindowType getWindowType(HWND hWnd) noexcept {
	// Real class of superclassed standard controls
	TCHAR buf[64];
	if (!RealGetWindowClass(hWnd, buf, 64)) {
		return TYPE_OTHER;
	}

	struct ClassType {
		const TCHAR* name;
		WindowType type;
	};

	static const ClassType types[] = {
		{ _T("#32770"), TYPE_DIALOG },
		{ WC_BUTTON, TYPE_BUTTON },
		{ WC_EDIT, TYPE_EDIT },
		{ WC_COMBOBOX, TYPE_COMBO },
		{ WC_COMBOBOXEX, TYPE_COMBO },
		{ WC_LISTBOX, TYPE_LISTBOX },
		{ _T("ComboLBox"), TYPE_LISTBOX },
		{ WC_LISTVIEW, TYPE_LISTVIEW },
		{ WC_TREEVIEW, TYPE_TREEVIEW },
		{ WC_HEADER, TYPE_HEADER },
		{ STATUSCLASSNAME, TYPE_STATUSBAR },
		{ REBARCLASSNAME, TYPE_REBAR },
		{ TOOLTIPS_CLASS, TYPE_TOOLTIP },
		{ WC_SCROLLBAR, TYPE_SCROLLBAR },
		{ MSFTEDIT_CLASS, TYPE_RICHEDIT },
		{ _T("RichEdit20W"), TYPE_RICHEDIT },
		{ WC_TABCONTROL, TYPE_TAB },
		{ PROGRESS_CLASS, TYPE_PROGRESS },
		{ TOOLBARCLASSNAME, TYPE_TOOLBAR },
		{ UPDOWN_CLASS, TYPE_UPDOWN },
	};

	for (const auto& t : types) {
		if (_tcsicmp(buf, t.name) == 0) {
			return t.type;
		}
	}

	// Common controls superclassed by ATL (the messages are only handled by the matching control type)
	if (_tcsncmp(buf, _T("ATL:"), 4) == 0) {
		if (SendMessage(hWnd, LVM_GETHEADER, 0, 0) || SendMessage(hWnd, LVM_GETBKCOLOR, 0, 0)) {
			return TYPE_LISTVIEW;
		}

		if (SendMessage(hWnd, TVM_GETINDENT, 0, 0)) {
			return TYPE_TREEVIEW;
		}
	}

	return TYPE_OTHER;
}

// Type of a window handled by us (avoids class name lookups for frequent messages)
WindowType getSubclassedType(HWND hWnd) noexcept;
void subclassWindow(HWND hWnd, WindowType aType) noexcept;

bool isSysColorBrush(HBRUSH aBrush) noexcept {
	if (!aBrush) {
		return true;
	}

	// The light tab texture of property pages
	LOGBRUSH lb;
	if (GetObject(aBrush, sizeof(lb), &lb) == sizeof(lb) && (lb.lbStyle == BS_PATTERN || lb.lbStyle == BS_DIBPATTERN || lb.lbStyle == BS_DIBPATTERNPT)) {
		return true;
	}

	for (auto i : { COLOR_3DFACE, COLOR_WINDOW, COLOR_APPWORKSPACE, COLOR_MENU, COLOR_INFOBK }) {
		if (aBrush == GetSysColorBrush(i) || aBrush == DarkMode::getSysColorBrush(i)) {
			return true;
		}
	}

	return false;
}

bool isUserCheckbox(DWORD aStyle) noexcept {
	switch (aStyle & BS_TYPEMASK) {
		case BS_CHECKBOX:
		case BS_AUTOCHECKBOX:
		case BS_3STATE:
		case BS_AUTO3STATE:
		case BS_RADIOBUTTON:
		case BS_AUTORADIOBUTTON:
			return (aStyle & BS_PUSHLIKE) == 0;
		default:
			return false;
	}
}

bool isGroupBox(DWORD aStyle) noexcept {
	return (aStyle & BS_TYPEMASK) == BS_GROUPBOX;
}

void setTitleBarTheme(HWND hWnd) noexcept {
	if ((GetWindowLongPtr(hWnd, GWL_STYLE) & WS_CAPTION) != WS_CAPTION || GetAncestor(hWnd, GA_ROOT) != hWnd) {
		return;
	}

	BOOL dark = DarkMode::isEnabled();
	DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
}

// Light colors of standard controls that the application hasn't changed
void setListColors(HWND hWnd, WindowType aType) noexcept {
	auto dark = DarkMode::isEnabled();
	auto bg = dark ? DARK_CONTROL : GetSysColor(COLOR_WINDOW);
	auto text = dark ? DARK_TEXT : GetSysColor(COLOR_WINDOWTEXT);
	auto oldBg = dark ? GetSysColor(COLOR_WINDOW) : DARK_CONTROL;
	auto oldText = dark ? GetSysColor(COLOR_WINDOWTEXT) : DARK_TEXT;

	if (aType == TYPE_LISTVIEW) {
		auto currentBg = ListView_GetBkColor(hWnd);
		if (currentBg == oldBg || currentBg == CLR_NONE) {
			ListView_SetBkColor(hWnd, bg);
			ListView_SetTextBkColor(hWnd, bg);
		}

		auto currentText = ListView_GetTextColor(hWnd);
		if (currentText == oldText || currentText == CLR_DEFAULT) {
			ListView_SetTextColor(hWnd, dark ? text : CLR_DEFAULT);
		}
	} else if (aType == TYPE_TREEVIEW) {
		// The default colors are reported as CLR_NONE
		auto currentBg = TreeView_GetBkColor(hWnd);
		if (currentBg == oldBg || currentBg == static_cast<COLORREF>(-1)) {
			TreeView_SetBkColor(hWnd, dark ? bg : static_cast<COLORREF>(-1));
		}

		auto currentText = TreeView_GetTextColor(hWnd);
		if (currentText == oldText || currentText == static_cast<COLORREF>(-1)) {
			TreeView_SetTextColor(hWnd, dark ? text : static_cast<COLORREF>(-1));
		}
	}
}

bool isMultilineEdit(HWND hWnd, WindowType aType) noexcept {
	return aType == TYPE_EDIT && (GetWindowLongPtr(hWnd, GWL_STYLE) & ES_MULTILINE);
}

const TCHAR* PROP_DARK = _T("AirDCDarkMode");
const TCHAR* PROP_HOT_IMAGES = _T("AirDCDarkModeHotImages");
const TCHAR* PROP_PAINT_DISABLED = _T("AirDCDarkModePaintDisabled");

void applyWindowTheme(HWND hWnd, WindowType aType) noexcept {
	auto dark = DarkMode::isEnabled();
	if (aType == TYPE_TOOLBAR) {
		// The hot images (also used for checked buttons) are grayscale versions of the icons
		// (the image lists are set after the toolbar has been created)
		auto normal = reinterpret_cast<HIMAGELIST>(SendMessage(hWnd, TB_GETIMAGELIST, 0, 0));
		auto hot = reinterpret_cast<HIMAGELIST>(SendMessage(hWnd, TB_GETHOTIMAGELIST, 0, 0));
		if (dark && normal && hot && hot != normal && !GetProp(hWnd, PROP_HOT_IMAGES)) {
			SetProp(hWnd, PROP_HOT_IMAGES, hot);
			SendMessage(hWnd, TB_SETHOTIMAGELIST, 0, reinterpret_cast<LPARAM>(normal));
		} else if (!dark) {
			if (auto original = RemoveProp(hWnd, PROP_HOT_IMAGES); original) {
				SendMessage(hWnd, TB_SETHOTIMAGELIST, 0, reinterpret_cast<LPARAM>(original));
			}
		}
	}

	if (dark == (GetProp(hWnd, PROP_DARK) != nullptr)) {
		// Nothing to do (light windows that have never been dark are left untouched)
		return;
	}

	if (dark) {
		SetProp(hWnd, PROP_DARK, reinterpret_cast<HANDLE>(1));
	} else {
		RemoveProp(hWnd, PROP_DARK);
	}

	// Dark list boxes would draw a white selection
	if (allowDarkModeForWindow && aType != TYPE_LISTBOX) {
		allowDarkModeForWindow(hWnd, dark);
	}

	setTitleBarTheme(hWnd);

	switch (aType) {
		case TYPE_LISTVIEW:
		case TYPE_TREEVIEW:
			SetWindowTheme(hWnd, dark ? L"DarkMode_Explorer" : (SETTING(USE_EXPLORER_THEME) ? L"explorer" : nullptr), nullptr);
			setListColors(hWnd, aType);
			if (aType == TYPE_LISTVIEW) {
				// Created by the list view (not necessarily handled by our hook)
				if (auto header = ListView_GetHeader(hWnd); header) {
					SetWindowTheme(header, dark ? L"DarkMode_ItemsView" : nullptr, nullptr);
				}
			}

			if (auto tooltip = reinterpret_cast<HWND>(SendMessage(hWnd, aType == TYPE_LISTVIEW ? LVM_GETTOOLTIPS : TVM_GETTOOLTIPS, 0, 0)); tooltip) {
				SetWindowTheme(tooltip, dark ? L"DarkMode_Explorer" : nullptr, nullptr);
			}
			break;
		case TYPE_HEADER:
			SetWindowTheme(hWnd, dark ? L"DarkMode_ItemsView" : nullptr, nullptr);
			break;
		case TYPE_EDIT:
		case TYPE_COMBO:
			// The CFD style is for single line fields (multiline edits need the dark scroll bars)
			SetWindowTheme(hWnd, dark ? (isMultilineEdit(hWnd, aType) ? L"DarkMode_Explorer" : L"DarkMode_CFD") : nullptr, nullptr);
			if (aType == TYPE_COMBO) {
				COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };
				if (GetComboBoxInfo(hWnd, &info) && info.hwndList) {
					// The CFD style has dark scroll bars and the system selection colors
					SetWindowTheme(info.hwndList, dark ? L"DarkMode_CFD" : nullptr, nullptr);
				}
			}
			break;
		case TYPE_BUTTON:
		case TYPE_LISTBOX:
		case TYPE_SCROLLBAR:
		case TYPE_UPDOWN:
		case TYPE_TOOLTIP:
		case TYPE_RICHEDIT:
			SetWindowTheme(hWnd, dark ? L"DarkMode_Explorer" : nullptr, nullptr);
			break;
		case TYPE_PROGRESS:
			// The visual style ignores the background color
			SetWindowTheme(hWnd, dark ? L"" : nullptr, dark ? L"" : nullptr);
			SendMessage(hWnd, PBM_SETBKCOLOR, 0, dark ? DARK_CONTROL : CLR_DEFAULT);
			break;
		default:
			break;
	}

	if (aType == TYPE_TOOLTIP) {
		// The window theme isn't enough for balloon tips
		SendMessage(hWnd, TTM_SETTIPBKCOLOR, dark ? DARK_CONTROL : GetSysColor(COLOR_INFOBK), 0);
		SendMessage(hWnd, TTM_SETTIPTEXTCOLOR, dark ? DARK_TEXT : GetSysColor(COLOR_INFOTEXT), 0);
	}
}

void drawText(HWND hWnd, HDC hdc, RECT& rc, UINT aFormat) noexcept {
	auto length = GetWindowTextLength(hWnd);
	if (length == 0) {
		return;
	}

	tstring text;
	text.resize(length + 1);
	text.resize(GetWindowText(hWnd, &text[0], length + 1));

	auto uiState = static_cast<DWORD>(SendMessage(hWnd, WM_QUERYUISTATE, 0, 0));
	if (uiState & UISF_HIDEACCEL) {
		aFormat |= DT_HIDEPREFIX;
	}

	SetTextColor(hdc, IsWindowEnabled(hWnd) ? DARK_TEXT : DARK_DISABLED_TEXT);
	SetBkMode(hdc, TRANSPARENT);
	DrawText(hdc, text.c_str(), static_cast<int>(text.size()), &rc, aFormat);
}

void excludeChildren(HWND hWnd, HDC hdc, bool aIncludeDialogs = true) noexcept {
	for (auto child = GetWindow(hWnd, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
		auto type = getSubclassedType(child);
		if (type == TYPE_BUTTON && isGroupBox(static_cast<DWORD>(GetWindowLongPtr(child, GWL_STYLE)))) {
			// Transparent
			continue;
		}

		if (IsWindowVisible(child) && (aIncludeDialogs || type != TYPE_DIALOG)) {
			RECT rc;
			GetWindowRect(child, &rc);
			MapWindowPoints(nullptr, hWnd, reinterpret_cast<LPPOINT>(&rc), 2);
			ExcludeClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
		}
	}
}

HBRUSH getParentBrush(HWND hWnd, HDC hdc) noexcept {
	auto brush = reinterpret_cast<HBRUSH>(SendMessage(GetParent(hWnd), WM_CTLCOLORSTATIC, reinterpret_cast<WPARAM>(hdc), reinterpret_cast<LPARAM>(hWnd)));
	return brush ? brush : DarkMode::getSysColorBrush(COLOR_3DFACE);
}

// Themed check boxes, radio buttons and group boxes ignore the text color
void paintButton(HWND hWnd, HDC hdc, const RECT& rcClient) noexcept {
	auto style = static_cast<DWORD>(GetWindowLongPtr(hWnd, GWL_STYLE));
	auto font = reinterpret_cast<HFONT>(SendMessage(hWnd, WM_GETFONT, 0, 0));
	auto oldFont = SelectObject(hdc, font ? font : GetStockObject(DEFAULT_GUI_FONT));

	auto theme = OpenThemeData(hWnd, VSCLASS_BUTTON);
	if (isGroupBox(style)) {
		TEXTMETRIC tm;
		GetTextMetrics(hdc, &tm);

		RECT rcText = rcClient;
		rcText.left += tm.tmAveCharWidth;
		drawText(hWnd, hdc, rcText, DT_SINGLELINE | DT_CALCRECT);
		auto hasText = rcText.right > rcText.left + tm.tmAveCharWidth;

		// Frame
		RECT rcFrame = rcClient;
		rcFrame.top += tm.tmHeight / 2;
		auto pen = CreatePen(PS_SOLID, 1, DARK_EDGE);
		auto oldPen = SelectObject(hdc, pen);
		auto oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
		auto saved = SaveDC(hdc);
		if (hasText) {
			ExcludeClipRect(hdc, rcText.left - 2, rcText.top, rcText.right + 2, rcText.bottom);
		}
		RoundRect(hdc, rcFrame.left, rcFrame.top, rcFrame.right, rcFrame.bottom, 4, 4);
		RestoreDC(hdc, saved);
		SelectObject(hdc, oldBrush);
		DeleteObject(SelectObject(hdc, oldPen));

		if (hasText) {
			RECT rcFill = rcText;
			InflateRect(&rcFill, 2, 0);
			FillRect(hdc, &rcFill, getParentBrush(hWnd, hdc));
			drawText(hWnd, hdc, rcText, DT_SINGLELINE);
		}
	} else {
		FillRect(hdc, &rcClient, getParentBrush(hWnd, hdc));

		auto type = style & BS_TYPEMASK;
		auto isRadio = type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON;
		auto check = static_cast<UINT>(SendMessage(hWnd, BM_GETCHECK, 0, 0));
		auto buttonState = static_cast<UINT>(SendMessage(hWnd, BM_GETSTATE, 0, 0));

		int stateBase = check == BST_CHECKED ? CBS_CHECKEDNORMAL : check == BST_INDETERMINATE ? CBS_MIXEDNORMAL : CBS_UNCHECKEDNORMAL;
		int offset = !IsWindowEnabled(hWnd) ? 3 : (buttonState & BST_PUSHED) ? 2 : (buttonState & BST_HOT) ? 1 : 0;
		if (isRadio) {
			// Radio buttons have no mixed state
			stateBase = check == BST_CHECKED ? RBS_CHECKEDNORMAL : RBS_UNCHECKEDNORMAL;
		}

		auto part = isRadio ? BP_RADIOBUTTON : BP_CHECKBOX;
		SIZE glyph = { 13, 13 };
		if (theme) {
			GetThemePartSize(theme, hdc, part, stateBase + offset, nullptr, TS_DRAW, &glyph);
		}

		RECT rcGlyph = rcClient;
		rcGlyph.top += (rcClient.bottom - rcClient.top - glyph.cy) / 2;
		rcGlyph.bottom = rcGlyph.top + glyph.cy;
		rcGlyph.right = rcGlyph.left + glyph.cx;

		if (theme) {
			DrawThemeBackground(theme, hdc, part, stateBase + offset, &rcGlyph, nullptr);
		} else {
			UINT dfcs = (isRadio ? DFCS_BUTTONRADIO : DFCS_BUTTONCHECK) | (check == BST_CHECKED ? DFCS_CHECKED : 0);
			DrawFrameControl(hdc, &rcGlyph, DFC_BUTTON, dfcs);
		}

		RECT rcText = rcClient;
		rcText.left = rcGlyph.right + glyph.cx / 3;

		UINT format = DT_LEFT;
		if (style & BS_MULTILINE) {
			// Center the text block vertically
			RECT rcCalc = rcText;
			drawText(hWnd, hdc, rcCalc, DT_WORDBREAK | DT_CALCRECT);
			auto height = rcCalc.bottom - rcCalc.top;
			rcText.top += std::max<LONG>(0, (rcText.bottom - rcText.top - height) / 2);
			format |= DT_WORDBREAK;
		} else {
			format |= DT_SINGLELINE | DT_VCENTER;
		}

		drawText(hWnd, hdc, rcText, format);

		auto uiState = static_cast<DWORD>(SendMessage(hWnd, WM_QUERYUISTATE, 0, 0));
		if (GetFocus() == hWnd && !(uiState & UISF_HIDEFOCUS)) {
			RECT rcFocus = rcText;
			drawText(hWnd, hdc, rcFocus, (format & ~DT_VCENTER) | DT_CALCRECT);
			auto height = rcFocus.bottom - rcFocus.top;
			if (!(style & BS_MULTILINE)) {
				rcFocus.top = rcText.top + (rcText.bottom - rcText.top - height) / 2;
				rcFocus.bottom = rcFocus.top + height;
			}
			InflateRect(&rcFocus, 1, 1);
			SetTextColor(hdc, DARK_TEXT);
			DrawFocusRect(hdc, &rcFocus);
		}
	}

	if (theme) {
		CloseThemeData(theme);
	}

	SelectObject(hdc, oldFont);
}

void paintStatusBar(HWND hWnd, HDC hdc, const RECT& rcClient) noexcept {
	FillRect(hdc, &rcClient, DarkMode::getSysColorBrush(COLOR_3DFACE));

	auto font = reinterpret_cast<HFONT>(SendMessage(hWnd, WM_GETFONT, 0, 0));
	auto oldFont = SelectObject(hdc, font ? font : GetStockObject(DEFAULT_GUI_FONT));
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, DARK_TEXT);

	auto pen = CreatePen(PS_SOLID, 1, DARK_EDGE);
	auto oldPen = SelectObject(hdc, pen);

	auto parts = static_cast<int>(SendMessage(hWnd, SB_GETPARTS, 0, 0));
	if (SendMessage(hWnd, SB_ISSIMPLE, 0, 0)) {
		parts = 1;
	}

	const auto iconSize = GetSystemMetrics(SM_CXSMICON);
	for (int i = 0; i < parts; ++i) {
		RECT rc;
		if (!SendMessage(hWnd, SB_GETRECT, i, reinterpret_cast<LPARAM>(&rc))) {
			continue;
		}

		// Separator
		if (i < parts - 1) {
			MoveToEx(hdc, rc.right - 1, rc.top + 2, nullptr);
			LineTo(hdc, rc.right - 1, rc.bottom - 2);
		}

		rc.left += 2;
		rc.right -= 2;

		if (auto icon = reinterpret_cast<HICON>(SendMessage(hWnd, SB_GETICON, i, 0)); icon) {
			DrawIconEx(hdc, rc.left, rc.top + (rc.bottom - rc.top - iconSize) / 2, icon, iconSize, iconSize, 0, nullptr, DI_NORMAL);
			rc.left += iconSize + 2;
		}

		auto info = static_cast<DWORD>(SendMessage(hWnd, SB_GETTEXTLENGTH, i, 0));
		auto length = LOWORD(info);
		auto type = HIWORD(info);
		if (type & SBT_OWNERDRAW) {
			DRAWITEMSTRUCT dis = { 0 };
			dis.CtlID = GetDlgCtrlID(hWnd);
			dis.itemID = i;
			dis.hwndItem = hWnd;
			dis.hDC = hdc;
			dis.rcItem = rc;
			dis.itemData = SendMessage(hWnd, SB_GETTEXT, i, 0);
			SendMessage(GetParent(hWnd), WM_DRAWITEM, dis.CtlID, reinterpret_cast<LPARAM>(&dis));
			continue;
		}

		if (length == 0) {
			continue;
		}

		tstring text;
		text.resize(length + 1);
		SendMessage(hWnd, SB_GETTEXT, i, reinterpret_cast<LPARAM>(&text[0]));
		text.resize(length);

		// Leading tabs center or right align the text
		UINT format = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX;
		if (!(type & SBT_NOTABPARSING)) {
			size_t tabs = 0;
			while (tabs < 2 && tabs < text.size() && text[tabs] == _T('\t')) {
				tabs++;
			}
			text.erase(0, tabs);
			format |= tabs == 2 ? DT_RIGHT : tabs == 1 ? DT_CENTER : DT_LEFT;
		}

		DrawText(hdc, text.c_str(), static_cast<int>(text.size()), &rc, format);
	}

	DeleteObject(SelectObject(hdc, oldPen));
	SelectObject(hdc, oldFont);

	// Size grip
	if ((GetWindowLongPtr(hWnd, GWL_STYLE) & SBARS_SIZEGRIP) && !IsZoomed(GetAncestor(hWnd, GA_ROOT))) {
		const int dot = std::max(2, iconSize / 8);
		for (int row = 0; row < 3; ++row) {
			for (int col = 0; col < 3 - row; ++col) {
				RECT rcDot;
				rcDot.left = rcClient.right - (col + 1) * dot * 2;
				rcDot.top = rcClient.bottom - (row + 1) * dot * 2;
				rcDot.right = rcDot.left + dot;
				rcDot.bottom = rcDot.top + dot;
				FillRect(hdc, &rcDot, DarkMode::getSysColorBrush(COLOR_3DSHADOW));
			}
		}
	}
}

// There is no dark visual style for tab controls
void paintTab(HWND hWnd, HDC hdc, const RECT& rcClient) noexcept {
	FillRect(hdc, &rcClient, DarkMode::getSysColorBrush(COLOR_3DFACE));

	auto font = reinterpret_cast<HFONT>(SendMessage(hWnd, WM_GETFONT, 0, 0));
	auto oldFont = SelectObject(hdc, font ? font : GetStockObject(DEFAULT_GUI_FONT));
	SetBkMode(hdc, TRANSPARENT);

	auto edgeBrush = CreateSolidBrush(DARK_EDGE);

	// Page area
	auto count = TabCtrl_GetItemCount(hWnd);
	RECT rcPage = rcClient;
	TabCtrl_AdjustRect(hWnd, FALSE, &rcPage);
	InflateRect(&rcPage, 2, 2);
	FrameRect(hdc, &rcPage, edgeBrush);

	auto selected = TabCtrl_GetCurSel(hWnd);
	auto images = TabCtrl_GetImageList(hWnd);
	for (int i = 0; i < count; ++i) {
		RECT rc;
		if (!TabCtrl_GetItemRect(hWnd, i, &rc)) {
			continue;
		}

		if (i == selected) {
			InflateRect(&rc, 2, 2);
			rc.bottom = rcPage.top + 1;
		}

		FillRect(hdc, &rc, DarkMode::getSysColorBrush(i == selected ? COLOR_WINDOW : COLOR_3DFACE));
		FrameRect(hdc, &rc, edgeBrush);
		if (i == selected) {
			// Merge with the page area
			RECT rcJoin = { rc.left + 1, rcPage.top, rc.right - 1, rcPage.top + 1 };
			FillRect(hdc, &rcJoin, DarkMode::getSysColorBrush(COLOR_WINDOW));
		}

		TCHAR text[256] = { 0 };
		TCITEM item = { 0 };
		item.mask = TCIF_TEXT | TCIF_IMAGE;
		item.pszText = text;
		item.cchTextMax = 255;
		TabCtrl_GetItem(hWnd, i, &item);

		RECT rcText = rc;
		if (images && item.iImage >= 0) {
			int cx = 0, cy = 0;
			ImageList_GetIconSize(images, &cx, &cy);
			ImageList_Draw(images, item.iImage, hdc, rc.left + 6, rc.top + (rc.bottom - rc.top - cy) / 2, ILD_TRANSPARENT);
			rcText.left += cx + 6;
		}

		SetTextColor(hdc, i == selected ? DARK_TEXT : DarkMode::getSysColor(COLOR_GRAYTEXT));
		DrawText(hdc, text, -1, &rcText, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);
	}

	DeleteObject(edgeBrush);
	SelectObject(hdc, oldFont);
}

// The sunken client edge is drawn with the light system colors
// Edits draw their themed (light) border directly when they get the focus or the mouse moves over them
bool hasThemedClientEdge(HWND hWnd, WindowType aType) noexcept {
	return (aType == TYPE_EDIT || aType == TYPE_RICHEDIT) && (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_CLIENTEDGE);
}

void paintClientEdge(HWND hWnd) noexcept {
	RECT rc;
	GetWindowRect(hWnd, &rc);
	OffsetRect(&rc, -rc.left, -rc.top);

	auto hdc = GetWindowDC(hWnd);
	auto outer = CreateSolidBrush(DARK_SHADOW);
	FrameRect(hdc, &rc, outer);
	InflateRect(&rc, -1, -1);
	FrameRect(hdc, &rc, DarkMode::getSysColorBrush(COLOR_3DFACE));
	DeleteObject(outer);
	ReleaseDC(hWnd, hdc);
}

// The column separators of the visual style and the grid lines are light
void paintGridLines(HWND hWnd) noexcept {
	if ((GetWindowLongPtr(hWnd, GWL_STYLE) & LVS_TYPEMASK) != LVS_REPORT) {
		return;
	}

	auto header = ListView_GetHeader(hWnd);
	if (!header) {
		return;
	}

	RECT rcClient;
	GetClientRect(hWnd, &rcClient);

	RECT rcHeader = { 0 };
	if (IsWindowVisible(header)) {
		GetWindowRect(header, &rcHeader);
		MapWindowPoints(nullptr, hWnd, reinterpret_cast<LPPOINT>(&rcHeader), 2);
		rcClient.top = rcHeader.bottom;
	}

	auto hdc = GetDC(hWnd);
	auto brush = CreateSolidBrush(DARK_CONTROL);
	auto bgBrush = CreateSolidBrush(ListView_GetBkColor(hWnd));
	int right = rcClient.left;

	// Columns
	auto columns = Header_GetItemCount(header);
	for (int i = 0; i < columns; ++i) {
		RECT rc;
		if (!Header_GetItemRect(header, i, &rc)) {
			continue;
		}

		MapWindowPoints(header, hWnd, reinterpret_cast<LPPOINT>(&rc), 2);
		RECT rcLine = { rc.right - 1, rcClient.top, rc.right, rcClient.bottom };
		FillRect(hdc, &rcLine, brush);

		// Second pixel of the separator
		RECT rcShade = { rc.right, rcClient.top, rc.right + 1, rcClient.bottom };
		FillRect(hdc, &rcShade, bgBrush);
		right = std::max(right, static_cast<int>(rc.right));
	}

	if (!(ListView_GetExtendedListViewStyle(hWnd) & LVS_EX_GRIDLINES)) {
		right = rcClient.left;
	}

	// Rows
	auto top = ListView_GetTopIndex(hWnd);
	auto count = ListView_GetItemCount(hWnd);
	for (int i = top; i < count && right > rcClient.left; ++i) {
		RECT rc;
		if (!ListView_GetItemRect(hWnd, i, &rc, LVIR_BOUNDS) || rc.top > rcClient.bottom) {
			break;
		}

		RECT rcLine = { rcClient.left, rc.bottom - 1, right, rc.bottom };
		FillRect(hdc, &rcLine, brush);
	}

	DeleteObject(bgBrush);
	DeleteObject(brush);
	ReleaseDC(hWnd, hdc);
}

// While the mouse isn't over them, the dark scroll bars of Windows 11 show a light thin thumb
const TCHAR* PROP_SCROLL_HOT = _T("AirDCDarkModeScrollHot");
const COLORREF SCROLL_TRACK = RGB(23, 23, 23);
const UINT_PTR SCROLL_TIMER = 0x44524B53;

// The size box between the scroll bars is lighter than the tracks
void paintScrollCorner(HWND hWnd) noexcept {
	auto style = GetWindowLongPtr(hWnd, GWL_STYLE);
	if ((style & (WS_HSCROLL | WS_VSCROLL)) != (WS_HSCROLL | WS_VSCROLL)) {
		return;
	}

	SCROLLBARINFO h = { sizeof(SCROLLBARINFO) }, v = { sizeof(SCROLLBARINFO) };
	if (!GetScrollBarInfo(hWnd, OBJID_HSCROLL, &h) || !GetScrollBarInfo(hWnd, OBJID_VSCROLL, &v) ||
		((h.rgstate[0] | v.rgstate[0]) & (STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_OFFSCREEN))) {
		return;
	}

	RECT rcWindow;
	GetWindowRect(hWnd, &rcWindow);

	RECT rc = { v.rcScrollBar.left, h.rcScrollBar.top, v.rcScrollBar.right, h.rcScrollBar.bottom };
	OffsetRect(&rc, -rcWindow.left, -rcWindow.top);

	auto hdc = GetWindowDC(hWnd);
	auto brush = CreateSolidBrush(SCROLL_TRACK);
	FillRect(hdc, &rc, brush);
	DeleteObject(brush);
	ReleaseDC(hWnd, hdc);
}

void paintThinScrollThumbs(HWND hWnd, WindowType aType) noexcept {
	if (aType != TYPE_LISTVIEW && aType != TYPE_TREEVIEW && aType != TYPE_RICHEDIT) {
		return;
	}

	auto style = GetWindowLongPtr(hWnd, GWL_STYLE);
	if (!(style & (WS_HSCROLL | WS_VSCROLL)) || !IsWindowVisible(hWnd) || GetCapture() == hWnd || GetProp(hWnd, PROP_SCROLL_HOT)) {
		return;
	}

	RECT rcWindow;
	GetWindowRect(hWnd, &rcWindow);

	HDC hdc = nullptr;
	int thickness = 2;
	for (auto vertical : { false, true }) {
		if (!(style & (vertical ? WS_VSCROLL : WS_HSCROLL))) {
			continue;
		}

		SCROLLBARINFO sbi = { sizeof(SCROLLBARINFO) };
		if (!GetScrollBarInfo(hWnd, vertical ? OBJID_VSCROLL : OBJID_HSCROLL, &sbi) || (sbi.rgstate[0] & (STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_OFFSCREEN))) {
			continue;
		}

		auto rc = sbi.rcScrollBar;
		OffsetRect(&rc, -rcWindow.left, -rcWindow.top);
		if (!hdc) {
			hdc = GetWindowDC(hWnd);
			thickness = MulDiv(2, GetDeviceCaps(hdc, LOGPIXELSX), 96);
		}

		auto track = CreateSolidBrush(SCROLL_TRACK);
		FillRect(hdc, &rc, track);
		DeleteObject(track);

		if ((sbi.rgstate[0] & STATE_SYSTEM_UNAVAILABLE) || sbi.xyThumbBottom <= sbi.xyThumbTop) {
			continue;
		}

		RECT rcThumb;
		if (vertical) {
			auto x = rc.left + (rc.right - rc.left) / 2;
			SetRect(&rcThumb, x, rc.top + sbi.xyThumbTop, x + thickness, rc.top + sbi.xyThumbBottom);
		} else {
			auto y = rc.top + (rc.bottom - rc.top) / 2;
			SetRect(&rcThumb, rc.left + sbi.xyThumbTop, y, rc.left + sbi.xyThumbBottom, y + thickness);
		}

		auto thumb = CreateSolidBrush(RGB(78, 78, 78));
		FillRect(hdc, &rcThumb, thumb);
		DeleteObject(thumb);
	}

	if (hdc) {
		ReleaseDC(hWnd, hdc);
	}
}

// The combo box styles have light borders
void paintComboBox(HWND hWnd, HDC hdc, const RECT& rcClient) noexcept {
	COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };
	GetComboBoxInfo(hWnd, &info);

	auto style = static_cast<DWORD>(GetWindowLongPtr(hWnd, GWL_STYLE));
	const bool enabled = IsWindowEnabled(hWnd);
	const bool hot = info.stateButton & STATE_SYSTEM_HOTTRACKED;
	const bool focused = GetFocus() == hWnd || GetFocus() == info.hwndItem;

	FillRect(hdc, &rcClient, DarkMode::getSysColorBrush(COLOR_WINDOW));

	auto border = CreateSolidBrush(focused ? GetSysColor(COLOR_HIGHLIGHT) : hot ? DARK_EDGE : DARK_SHADOW);
	FrameRect(hdc, &rcClient, border);
	DeleteObject(border);

	// Arrow
	const auto& rcButton = info.rcButton;
	auto pen = CreatePen(PS_SOLID, 1, enabled ? DarkMode::getSysColor(COLOR_BTNTEXT) : DARK_DISABLED_TEXT);
	auto oldPen = SelectObject(hdc, pen);
	const int size = std::max(3L, (rcButton.right - rcButton.left) / 5);
	const int x = (rcButton.left + rcButton.right) / 2;
	const int y = (rcButton.top + rcButton.bottom) / 2 - size / 2;
	MoveToEx(hdc, x - size, y, nullptr);
	LineTo(hdc, x, y + size);
	LineTo(hdc, x + size + 1, y - 1);
	DeleteObject(SelectObject(hdc, oldPen));

	if (info.hwndItem && info.hwndItem != hWnd) {
		// The edit draws itself (drop down lists report the combo box itself)
		return;
	}

	// Selected item
	auto selection = static_cast<int>(SendMessage(hWnd, CB_GETCURSEL, 0, 0));
	RECT rcItem = info.rcItem;
	if (style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) {
		DRAWITEMSTRUCT dis = { 0 };
		dis.CtlType = ODT_COMBOBOX;
		dis.CtlID = GetDlgCtrlID(hWnd);
		dis.itemID = selection;
		dis.itemAction = ODA_DRAWENTIRE;
		dis.itemState = ODS_COMBOBOXEDIT | (enabled ? 0 : ODS_DISABLED);
		dis.hwndItem = hWnd;
		dis.hDC = hdc;
		dis.rcItem = rcItem;
		dis.itemData = selection >= 0 ? SendMessage(hWnd, CB_GETITEMDATA, selection, 0) : 0;
		SendMessage(GetParent(hWnd), WM_DRAWITEM, dis.CtlID, reinterpret_cast<LPARAM>(&dis));
		return;
	}

	if (selection < 0) {
		return;
	}

	auto length = static_cast<int>(SendMessage(hWnd, CB_GETLBTEXTLEN, selection, 0));
	if (length <= 0) {
		return;
	}

	tstring text;
	text.resize(length + 1);
	SendMessage(hWnd, CB_GETLBTEXT, selection, reinterpret_cast<LPARAM>(&text[0]));
	text.resize(length);

	auto font = reinterpret_cast<HFONT>(SendMessage(hWnd, WM_GETFONT, 0, 0));
	auto oldFont = SelectObject(hdc, font ? font : GetStockObject(DEFAULT_GUI_FONT));
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, enabled ? DARK_TEXT : DARK_DISABLED_TEXT);
	rcItem.left += 2;
	DrawText(hdc, text.c_str(), length, &rcItem, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
	SelectObject(hdc, oldFont);
}

bool isTextPushButton(DWORD aStyle) noexcept {
	auto type = aStyle & BS_TYPEMASK;
	return (type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON) && !(aStyle & (BS_ICON | BS_BITMAP));
}

// The dark push button style has a light border
void paintPushButton(HWND hWnd, HDC hdc, const RECT& rcClient) noexcept {
	FillRect(hdc, &rcClient, getParentBrush(hWnd, hdc));

	auto style = static_cast<DWORD>(GetWindowLongPtr(hWnd, GWL_STYLE));
	auto state = static_cast<UINT>(SendMessage(hWnd, BM_GETSTATE, 0, 0));
	const bool enabled = IsWindowEnabled(hWnd);
	const bool isDefault = (style & BS_TYPEMASK) == BS_DEFPUSHBUTTON;

	auto fill = !enabled ? DARK_FACE : (state & BST_PUSHED) ? DARK_EDGE : (state & BST_HOT) ? RGB(62, 62, 62) : RGB(51, 51, 51);
	auto border = enabled && (isDefault || GetFocus() == hWnd) ? DarkMode::getSysColor(COLOR_MENUHILIGHT) : !enabled ? DARK_SHADOW : fill;

	auto brush = CreateSolidBrush(fill);
	auto pen = CreatePen(PS_SOLID, 1, border);
	auto oldBrush = SelectObject(hdc, brush);
	auto oldPen = SelectObject(hdc, pen);
	RoundRect(hdc, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom, 8, 8);
	DeleteObject(SelectObject(hdc, oldPen));
	DeleteObject(SelectObject(hdc, oldBrush));

	auto font = reinterpret_cast<HFONT>(SendMessage(hWnd, WM_GETFONT, 0, 0));
	auto oldFont = SelectObject(hdc, font ? font : GetStockObject(DEFAULT_GUI_FONT));
	RECT rcText = rcClient;
	InflateRect(&rcText, -4, 0);
	drawText(hWnd, hdc, rcText, DT_CENTER | ((style & BS_MULTILINE) ? DT_WORDBREAK : (DT_SINGLELINE | DT_VCENTER)));
	SelectObject(hdc, oldFont);
}

bool isIconButton(DWORD aStyle) noexcept {
	auto type = aStyle & BS_TYPEMASK;
	return (type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON) && (aStyle & (BS_ICON | BS_BITMAP));
}

// Icon buttons without a border, flush with the background
void paintIconButton(HWND hWnd, HDC hdc, const RECT& rcClient) noexcept {
	FillRect(hdc, &rcClient, getParentBrush(hWnd, hdc));

	auto state = static_cast<UINT>(SendMessage(hWnd, BM_GETSTATE, 0, 0));
	const bool enabled = IsWindowEnabled(hWnd);
	if (enabled && (state & (BST_HOT | BST_PUSHED))) {
		auto brush = CreateSolidBrush((state & BST_PUSHED) ? DARK_EDGE : RGB(62, 62, 62));
		auto oldBrush = SelectObject(hdc, brush);
		auto oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));
		RoundRect(hdc, rcClient.left, rcClient.top, rcClient.right + 1, rcClient.bottom + 1, 6, 6);
		SelectObject(hdc, oldPen);
		DeleteObject(SelectObject(hdc, oldBrush));
	}

	// Icons are also used with the bitmap style
	auto icon = reinterpret_cast<HICON>(SendMessage(hWnd, BM_GETIMAGE, IMAGE_ICON, 0));
	if (icon) {
		ICONINFO ii = { 0 };
		if (GetIconInfo(icon, &ii)) {
			BITMAP bm = { 0 };
			GetObject(ii.hbmColor ? ii.hbmColor : ii.hbmMask, sizeof(bm), &bm);
			int cx = bm.bmWidth, cy = ii.hbmColor ? bm.bmHeight : bm.bmHeight / 2;
			if (ii.hbmColor) DeleteObject(ii.hbmColor);
			if (ii.hbmMask) DeleteObject(ii.hbmMask);

			int x = rcClient.left + (rcClient.right - rcClient.left - cx) / 2;
			int y = rcClient.top + (rcClient.bottom - rcClient.top - cy) / 2;
			if (enabled) {
				DrawIconEx(hdc, x, y, icon, cx, cy, 0, nullptr, DI_NORMAL);
			} else {
				DrawState(hdc, nullptr, nullptr, reinterpret_cast<LPARAM>(icon), 0, x, y, cx, cy, DST_ICON | DSS_DISABLED);
			}
		}
	} else {
		auto bitmap = reinterpret_cast<HBITMAP>(SendMessage(hWnd, BM_GETIMAGE, IMAGE_BITMAP, 0));
		BITMAP bm = { 0 };
		if (bitmap && GetObject(bitmap, sizeof(bm), &bm)) {
			int x = rcClient.left + (rcClient.right - rcClient.left - bm.bmWidth) / 2;
			int y = rcClient.top + (rcClient.bottom - rcClient.top - bm.bmHeight) / 2;
			DrawState(hdc, nullptr, nullptr, reinterpret_cast<LPARAM>(bitmap), 0, x, y, bm.bmWidth, bm.bmHeight, DST_BITMAP | (enabled ? DSS_NORMAL : DSS_DISABLED));
		}
	}
}

// The dark spin style has light borders
void paintUpDown(HWND hWnd, HDC hdc, const RECT& rcClient) noexcept {
	FillRect(hdc, &rcClient, DarkMode::getSysColorBrush(COLOR_WINDOW));

	const bool horizontal = (GetWindowLongPtr(hWnd, GWL_STYLE) & UDS_HORZ) != 0;
	const bool enabled = IsWindowEnabled(hWnd);

	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(hWnd, &pt);
	const bool pressed = GetCapture() == hWnd;

	for (int i = 0; i < 2; ++i) {
		// First part: up (or left)
		RECT rc = rcClient;
		if (horizontal) {
			(i == 0 ? rc.right : rc.left) = (rcClient.left + rcClient.right) / 2;
		} else {
			(i == 0 ? rc.bottom : rc.top) = (rcClient.top + rcClient.bottom) / 2;
		}

		InflateRect(&rc, -1, -1);
		if (enabled && PtInRect(&rc, pt)) {
			auto brush = CreateSolidBrush(pressed ? DARK_EDGE : RGB(62, 62, 62));
			FillRect(hdc, &rc, brush);
			DeleteObject(brush);
		}

		// Arrow
		const int size = std::max(2, static_cast<int>(std::min(rc.right - rc.left, rc.bottom - rc.top) / 4));
		const int cx = (rc.left + rc.right) / 2;
		const int cy = (rc.top + rc.bottom) / 2;
		POINT points[3];
		if (horizontal) {
			const int dir = i == 0 ? -1 : 1;
			points[0] = { cx + dir * size / 2 - dir * size, cy - size };
			points[1] = { cx + dir * size / 2, cy };
			points[2] = { cx + dir * size / 2 - dir * size, cy + size };
		} else {
			const int dir = i == 0 ? -1 : 1;
			points[0] = { cx - size, cy - dir * size / 2 };
			points[1] = { cx, cy + dir * size / 2 };
			points[2] = { cx + size, cy - dir * size / 2 };
		}

		auto color = enabled ? DARK_TEXT : DARK_DISABLED_TEXT;
		auto brush = CreateSolidBrush(color);
		auto pen = CreatePen(PS_SOLID, 1, color);
		auto oldBrush = SelectObject(hdc, brush);
		auto oldPen = SelectObject(hdc, pen);
		Polygon(hdc, points, 3);
		DeleteObject(SelectObject(hdc, oldPen));
		DeleteObject(SelectObject(hdc, oldBrush));
	}
}

using PaintF = void (*)(HWND, HDC, const RECT&) noexcept;

LRESULT bufferedPaint(HWND hWnd, PaintF aPaintF) noexcept {
	PAINTSTRUCT ps;
	auto hdc = BeginPaint(hWnd, &ps);
	excludeChildren(hWnd, hdc);

	RECT rc;
	GetClientRect(hWnd, &rc);

	HDC memDC = nullptr;
	auto buffer = BeginBufferedPaint(hdc, &rc, BPBF_TOPDOWNDIB, nullptr, &memDC);
	if (buffer) {
		aPaintF(hWnd, memDC, rc);
		EndBufferedPaint(buffer, TRUE);
	} else {
		aPaintF(hWnd, hdc, rc);
	}

	EndPaint(hWnd, &ps);
	return 0;
}

LRESULT handleCtlColor(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept {
	auto result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
	if (!isSysColorBrush(reinterpret_cast<HBRUSH>(result))) {
		// Custom colors
		return result;
	}

	auto hdc = reinterpret_cast<HDC>(wParam);
	auto control = reinterpret_cast<HWND>(lParam);

	auto bg = COLOR_3DFACE;
	if (uMsg == WM_CTLCOLOREDIT || uMsg == WM_CTLCOLORLISTBOX) {
		bg = COLOR_WINDOW;
	} else if (uMsg == WM_CTLCOLORSTATIC && getSubclassedType(control) == TYPE_EDIT && IsWindowEnabled(control)) {
		// Read-only edits
		bg = COLOR_WINDOW;
	}

	SetTextColor(hdc, control && !IsWindowEnabled(control) ? DARK_DISABLED_TEXT : DARK_TEXT);
	SetBkColor(hdc, DarkMode::getSysColor(bg));
	return reinterpret_cast<LRESULT>(DarkMode::getSysColorBrush(bg));
}

// The light button backgrounds of toolbars (only the button states are drawn here)
LRESULT handleToolbarCustomDraw(HWND hWnd, WPARAM wParam, LPARAM lParam) noexcept {
	auto tbcd = reinterpret_cast<LPNMTBCUSTOMDRAW>(lParam);
	if (tbcd->nmcd.dwDrawStage == CDDS_PREPAINT) {
		return DefSubclassProc(hWnd, WM_NOTIFY, wParam, lParam) | CDRF_NOTIFYITEMDRAW;
	}

	if (tbcd->nmcd.dwDrawStage != CDDS_ITEMPREPAINT) {
		return DefSubclassProc(hWnd, WM_NOTIFY, wParam, lParam);
	}

	auto result = DefSubclassProc(hWnd, WM_NOTIFY, wParam, lParam);
	if (result & CDRF_SKIPDEFAULT) {
		return result;
	}

	// Checked buttons are flush with the background
	auto state = tbcd->nmcd.uItemState;
	if (state & (CDIS_HOT | CDIS_SELECTED)) {
		auto color = (state & CDIS_SELECTED) ? DARK_EDGE : RGB(62, 62, 62);
		auto brush = CreateSolidBrush(color);
		auto oldBrush = SelectObject(tbcd->nmcd.hdc, brush);
		auto oldPen = SelectObject(tbcd->nmcd.hdc, GetStockObject(NULL_PEN));
		const auto& rc = tbcd->nmcd.rc;
		RoundRect(tbcd->nmcd.hdc, rc.left, rc.top, rc.right + 1, rc.bottom + 1, 6, 6);
		SelectObject(tbcd->nmcd.hdc, oldPen);
		DeleteObject(SelectObject(tbcd->nmcd.hdc, oldBrush));
	}

	tbcd->clrText = (state & CDIS_DISABLED) ? DARK_DISABLED_TEXT : DARK_TEXT;
	return result | TBCDRF_NOBACKGROUND | TBCDRF_NOEDGES | TBCDRF_NOOFFSET | TBCDRF_NOETCHEDEFFECT;
}

// Header controls report their custom draw notifications to the list view
LRESULT handleHeaderCustomDraw(HWND hWnd, WPARAM wParam, LPARAM lParam) noexcept {
	auto nmcd = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);
	if (nmcd->dwDrawStage == CDDS_PREPAINT) {
		return CDRF_NOTIFYITEMDRAW;
	}

	if (nmcd->dwDrawStage == CDDS_ITEMPREPAINT) {
		SetTextColor(nmcd->hdc, DARK_TEXT);
		return CDRF_DODEFAULT;
	}

	return DefSubclassProc(hWnd, WM_NOTIFY, wParam, lParam);
}

// Disabled list views keep the enabled colors (WM_ENABLE isn't passed to them), the parent dims the text
LRESULT paintDisabledListView(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept {
	SetProp(hWnd, PROP_PAINT_DISABLED, reinterpret_cast<HANDLE>(1));
	auto result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
	RemoveProp(hWnd, PROP_PAINT_DISABLED);
	return result;
}

LRESULT CALLBACK subclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData) noexcept {
	auto type = static_cast<WindowType>(dwRefData);

	if (uMsg == WM_APPLY_THEME) {
		applyWindowTheme(hWnd, type);
		if (type == TYPE_MESSAGEBOX) {
			// The buttons and texts are created by the system
			for (auto child = GetWindow(hWnd, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
				subclassWindow(child, getWindowType(child));
				applyWindowTheme(child, getSubclassedType(child));
			}
		}
		return 0;
	}

	switch (uMsg) {
		case WM_NCDESTROY:
			RemoveProp(hWnd, PROP_SCROLL_HOT);
			RemoveProp(hWnd, PROP_DARK);
			RemoveProp(hWnd, PROP_HOT_IMAGES);
			RemoveProp(hWnd, PROP_PAINT_DISABLED);
			RemoveWindowSubclass(hWnd, subclassProc, SUBCLASS_ID);
			break;
		case WM_INITDIALOG:
		{
			auto result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
			applyWindowTheme(hWnd, type);
			return result;
		}
		default:
			break;
	}

	if (!DarkMode::isEnabled()) {
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg) {
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLORBTN:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORSCROLLBAR:
		case WM_CTLCOLORMSGBOX:
			return handleCtlColor(hWnd, uMsg, wParam, lParam);
		case WM_ERASEBKGND:
		{
			if (type == TYPE_BUTTON && isGroupBox(static_cast<DWORD>(GetWindowLongPtr(hWnd, GWL_STYLE)))) {
				// Transparent, the parent paints the background
				return TRUE;
			}

			if (type == TYPE_DIALOG || type == TYPE_MESSAGEBOX || type == TYPE_STATUSBAR || type == TYPE_REBAR || type == TYPE_TAB || type == TYPE_UPDOWN) {
				auto hdc = reinterpret_cast<HDC>(wParam);
				auto brush = type == TYPE_DIALOG || type == TYPE_MESSAGEBOX ?
					reinterpret_cast<HBRUSH>(SendMessage(hWnd, WM_CTLCOLORDLG, wParam, reinterpret_cast<LPARAM>(hWnd))) :
					DarkMode::getSysColorBrush(COLOR_3DFACE);

				// Dialogs don't clip their controls (property pages rely on the sheet erasing the old page)
				// Transparent toolbars rely on the rebar painting their background
				auto saved = SaveDC(hdc);
				if (type == TYPE_DIALOG || type == TYPE_MESSAGEBOX) {
					excludeChildren(hWnd, hdc, false);
				}

				RECT rc;
				GetClientRect(hWnd, &rc);
				FillRect(hdc, &rc, brush);
				RestoreDC(hdc, saved);
				return TRUE;
			}

			// Windows using a system color as the class background (controls paint their own background)
			auto classBrush = GetClassLongPtr(hWnd, GCLP_HBRBACKGROUND);
			if (type == TYPE_OTHER && classBrush > 0 && classBrush <= COLOR_MENUBAR + 1) {
				// Windows that don't erase their background (such as the double buffered tab bar) must be left alone,
				// the erased area would be shown until the window has been painted
				auto hdc = reinterpret_cast<HDC>(wParam);
				auto saved = SaveDC(hdc);
				IntersectClipRect(hdc, 0, 0, 0, 0);
				auto erased = DefSubclassProc(hWnd, uMsg, wParam, lParam);
				RestoreDC(hdc, saved);
				if (!erased) {
					return FALSE;
				}

				RECT rc;
				GetClientRect(hWnd, &rc);
				FillRect(hdc, &rc, DarkMode::getSysColorBrush(static_cast<int>(classBrush) - 1));
				return TRUE;
			}
			break;
		}
		case WM_MDIACTIVATE:
		{
			// Switching between maximized windows shows the new window restored with the light caption for a moment
			BOOL maximized = FALSE;
			if (!SendMessage(hWnd, WM_MDIGETACTIVE, 0, reinterpret_cast<LPARAM>(&maximized)) || !maximized || !IsWindowVisible(hWnd)) {
				// Not maximized, or the redrawing has been disabled by the caller already
				break;
			}

			SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);
			auto result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
			SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);

			// The MDI client doesn't clip its children, so erasing it would blank the whole area until the new window
			// has been painted (the maximized window covers the client area)
			RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_NOCHILDREN | RDW_UPDATENOW);

			auto active = reinterpret_cast<HWND>(SendMessage(hWnd, WM_MDIGETACTIVE, 0, 0));
			if (active) {
				RedrawWindow(active, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
			}
			return result;
		}
		case WM_NCPAINT:
		{
			auto result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

			// The borders of the visual styles are light (combo boxes are painted separately)
			if ((GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_CLIENTEDGE) && type != TYPE_COMBO) {
				paintClientEdge(hWnd);
			} else if (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_STATICEDGE) {
				// Light and white lines
				RECT rc;
				GetWindowRect(hWnd, &rc);
				OffsetRect(&rc, -rc.left, -rc.top);
				auto hdc = GetWindowDC(hWnd);
				auto brush = CreateSolidBrush(DARK_SHADOW);
				FrameRect(hdc, &rc, brush);
				DeleteObject(brush);
				ReleaseDC(hWnd, hdc);
			} else if (type == TYPE_LISTBOX && (GetWindowLongPtr(hWnd, GWL_STYLE) & WS_BORDER)) {
				// Drop down lists of combo boxes
				RECT rc;
				GetWindowRect(hWnd, &rc);
				OffsetRect(&rc, -rc.left, -rc.top);
				auto hdc = GetWindowDC(hWnd);
				auto brush = CreateSolidBrush(DARK_SHADOW);
				FrameRect(hdc, &rc, brush);
				DeleteObject(brush);
				ReleaseDC(hWnd, hdc);
			}

			paintScrollCorner(hWnd);
			paintThinScrollThumbs(hWnd, type);
			if (type == TYPE_LISTVIEW || type == TYPE_TREEVIEW || type == TYPE_RICHEDIT) {
				// Again after the fade in animation of the scroll bars
				SetTimer(hWnd, SCROLL_TIMER, 700, nullptr);
			}
			return result;
		}
		case WM_HSCROLL:
		case WM_VSCROLL:
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
		case WM_SIZE:
		case WM_KEYDOWN:
		{
			auto result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
			paintThinScrollThumbs(hWnd, type);
			return result;
		}
		case WM_NCMOUSEMOVE:
		{
			// Windows shows the wide thumb while the mouse is over the scroll bar
			if ((wParam == HTHSCROLL || wParam == HTVSCROLL) && !GetProp(hWnd, PROP_SCROLL_HOT)) {
				SetProp(hWnd, PROP_SCROLL_HOT, reinterpret_cast<HANDLE>(1));
			}
			break;
		}
		case WM_MOUSELEAVE:
		{
			if (type == TYPE_UPDOWN) {
				InvalidateRect(hWnd, nullptr, FALSE);
			} else if (hasThemedClientEdge(hWnd, type)) {
				// The hot border is painted without WM_NCPAINT
				auto result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
				paintClientEdge(hWnd);
				return result;
			}
			break;
		}
		case WM_NCMOUSELEAVE:
		case WM_MOUSEMOVE:
		{
			if (type == TYPE_UPDOWN) {
				// Hot part
				TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hWnd, 0 };
				TrackMouseEvent(&tme);
				InvalidateRect(hWnd, nullptr, FALSE);
			}

			if (RemoveProp(hWnd, PROP_SCROLL_HOT)) {
				// After the collapse animation
				SetTimer(hWnd, SCROLL_TIMER, 700, nullptr);
			}

			if (uMsg == WM_MOUSEMOVE && hasThemedClientEdge(hWnd, type)) {
				// The hot border is painted without WM_NCPAINT
				auto result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
				paintClientEdge(hWnd);
				return result;
			}
			break;
		}
		case WM_TIMER:
		{
			if (wParam == SCROLL_TIMER) {
				KillTimer(hWnd, SCROLL_TIMER);
				paintThinScrollThumbs(hWnd, type);
				return 0;
			}
			break;
		}
		case WM_PAINT:
		{
			if (type == TYPE_LISTVIEW && !IsWindowEnabled(hWnd)) {
				auto result = paintDisabledListView(hWnd, uMsg, wParam, lParam);
				paintGridLines(hWnd);
				paintThinScrollThumbs(hWnd, type);
				return result;
			}

			if (type == TYPE_LISTVIEW || type == TYPE_TREEVIEW) {
				auto result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
				if (type == TYPE_LISTVIEW) {
					paintGridLines(hWnd);
				}

				paintThinScrollThumbs(hWnd, type);
				return result;
			}

			if (type == TYPE_STATUSBAR) {
				return bufferedPaint(hWnd, paintStatusBar);
			}

			if (type == TYPE_TAB) {
				return bufferedPaint(hWnd, paintTab);
			}

			if (type == TYPE_UPDOWN) {
				return bufferedPaint(hWnd, paintUpDown);
			}

			if (type == TYPE_COMBO && (GetWindowLongPtr(hWnd, GWL_STYLE) & 0x3) != CBS_SIMPLE) {
				// Not for the ComboBoxEx wrapper
				COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };
				if (GetComboBoxInfo(hWnd, &info)) {
					return bufferedPaint(hWnd, paintComboBox);
				}
			}

			if (type == TYPE_MESSAGEBOX) {
				// Message boxes paint the light button area in WM_PAINT (the controls are child windows)
				PAINTSTRUCT ps;
				auto hdc = BeginPaint(hWnd, &ps);
				excludeChildren(hWnd, hdc);

				SendMessage(hWnd, WM_ERASEBKGND, reinterpret_cast<WPARAM>(hdc), 0);
				EndPaint(hWnd, &ps);
				return 0;
			}

			if (type == TYPE_BUTTON) {
				auto style = static_cast<DWORD>(GetWindowLongPtr(hWnd, GWL_STYLE));
				if (isUserCheckbox(style)) {
					return bufferedPaint(hWnd, paintButton);
				}

				if (isIconButton(style)) {
					return bufferedPaint(hWnd, paintIconButton);
				}

				if (isTextPushButton(style)) {
					return bufferedPaint(hWnd, paintPushButton);
				}

				if (isGroupBox(style)) {
					// Don't paint over the controls inside the group
					PAINTSTRUCT ps;
					auto hdc = BeginPaint(hWnd, &ps);
					RECT rc;
					GetClientRect(hWnd, &rc);
					paintButton(hWnd, hdc, rc);
					EndPaint(hWnd, &ps);
					return 0;
				}
			}
			break;
		}
		case WM_NOTIFY:
		{
			auto nmhdr = reinterpret_cast<LPNMHDR>(lParam);
			if (nmhdr->code == NM_CUSTOMDRAW && GetProp(nmhdr->hwndFrom, PROP_PAINT_DISABLED)) {
				auto nmcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);
				auto result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
				if (nmcd->nmcd.dwDrawStage == CDDS_PREPAINT) {
					return result | CDRF_NOTIFYITEMDRAW;
				}

				if (nmcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT || nmcd->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM)) {
					nmcd->clrText = DARK_DISABLED_TEXT;
				}
				return result;
			}

			if (nmhdr->code == NM_CUSTOMDRAW) {
				if (type == TYPE_LISTVIEW && nmhdr->hwndFrom == ListView_GetHeader(hWnd)) {
					return handleHeaderCustomDraw(hWnd, wParam, lParam);
				}

				if (getSubclassedType(nmhdr->hwndFrom) == TYPE_TOOLBAR) {
					return handleToolbarCustomDraw(hWnd, wParam, lParam);
				}

				// Rebar bands would be painted with the light theme
				if (getSubclassedType(nmhdr->hwndFrom) == TYPE_REBAR) {
					auto nmcd = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);
					if (nmcd->dwDrawStage == CDDS_PREPAINT) {
						FillRect(nmcd->hdc, &nmcd->rc, DarkMode::getSysColorBrush(COLOR_3DFACE));
						return CDRF_SKIPDEFAULT;
					}
				}
			}
			break;
		}
		case BM_SETCHECK:
		case BM_SETSTATE:
		case BM_SETSTYLE:
		case WM_ENABLE:
		case WM_SETTEXT:
		case WM_UPDATEUISTATE:
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
		{
			if (uMsg == WM_ENABLE && type == TYPE_LISTVIEW) {
				// The disabled list view would use the light system colors
				InvalidateRect(hWnd, nullptr, TRUE);
				return 0;
			}

			// The focused border is painted without WM_NCPAINT
			if ((uMsg == WM_SETFOCUS || uMsg == WM_KILLFOCUS) && hasThemedClientEdge(hWnd, type)) {
				auto result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
				paintClientEdge(hWnd);
				return result;
			}

			// The button may paint itself without WM_PAINT
			if (type == TYPE_BUTTON) {
				auto result = DefSubclassProc(hWnd, uMsg, wParam, lParam);
				auto style = static_cast<DWORD>(GetWindowLongPtr(hWnd, GWL_STYLE));
				if (isUserCheckbox(style) || isGroupBox(style) || isIconButton(style) || isTextPushButton(style)) {
					InvalidateRect(hWnd, nullptr, FALSE);
				}
				return result;
			}
			break;
		}
		default:
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

WindowType getSubclassedType(HWND hWnd) noexcept {
	DWORD_PTR type = TYPE_OTHER;
	if (!hWnd || !GetWindowSubclass(hWnd, subclassProc, SUBCLASS_ID, &type)) {
		return TYPE_OTHER;
	}

	return static_cast<WindowType>(type);
}

// Lists, trees and rich edits redraw their scroll bars directly when their contents change (without WM_NCPAINT).
// Windows draws the wide thumb at that point when the application isn't active, which makes the scroll bars flicker
// on every update unless the thin thumb is painted again right away (before the frame is shown).
void paintThinScrollThumbsAfterUpdate(HWND hWnd, int aBar, BOOL aRedraw) noexcept {
	if (!aRedraw || aBar == SB_CTL || !DarkMode::isEnabled() || !hWnd || GetWindowThreadProcessId(hWnd, nullptr) != GetCurrentThreadId()) {
		return;
	}

	DWORD_PTR type = 0;
	if (GetWindowSubclass(hWnd, subclassProc, SUBCLASS_ID, &type)) {
		paintThinScrollThumbs(hWnd, static_cast<WindowType>(type));
	}
}

decltype(&SetScrollInfo) origSetScrollInfo = nullptr;
decltype(&SetScrollPos) origSetScrollPos = nullptr;
decltype(&SetScrollRange) origSetScrollRange = nullptr;
decltype(&ShowScrollBar) origShowScrollBar = nullptr;
decltype(&EnableScrollBar) origEnableScrollBar = nullptr;

int WINAPI hookSetScrollInfo(HWND hWnd, int nBar, LPCSCROLLINFO lpsi, BOOL redraw) noexcept {
	auto result = origSetScrollInfo(hWnd, nBar, lpsi, redraw);
	paintThinScrollThumbsAfterUpdate(hWnd, nBar, redraw);
	return result;
}

int WINAPI hookSetScrollPos(HWND hWnd, int nBar, int nPos, BOOL bRedraw) noexcept {
	auto result = origSetScrollPos(hWnd, nBar, nPos, bRedraw);
	paintThinScrollThumbsAfterUpdate(hWnd, nBar, bRedraw);
	return result;
}

BOOL WINAPI hookSetScrollRange(HWND hWnd, int nBar, int nMinPos, int nMaxPos, BOOL bRedraw) noexcept {
	auto result = origSetScrollRange(hWnd, nBar, nMinPos, nMaxPos, bRedraw);
	paintThinScrollThumbsAfterUpdate(hWnd, nBar, bRedraw);
	return result;
}

BOOL WINAPI hookShowScrollBar(HWND hWnd, int wBar, BOOL bShow) noexcept {
	auto result = origShowScrollBar(hWnd, wBar, bShow);
	paintThinScrollThumbsAfterUpdate(hWnd, wBar, TRUE);
	return result;
}

BOOL WINAPI hookEnableScrollBar(HWND hWnd, UINT wSBflags, UINT wArrows) noexcept {
	auto result = origEnableScrollBar(hWnd, wSBflags, wArrows);
	paintThinScrollThumbsAfterUpdate(hWnd, static_cast<int>(wSBflags), TRUE);
	return result;
}

struct ImportHook {
	const char* name;
	void* hook;
};

const ImportHook scrollBarHooks[] = {
	{ "SetScrollInfo", reinterpret_cast<void*>(&hookSetScrollInfo) },
	{ "SetScrollPos", reinterpret_cast<void*>(&hookSetScrollPos) },
	{ "SetScrollRange", reinterpret_cast<void*>(&hookSetScrollRange) },
	{ "ShowScrollBar", reinterpret_cast<void*>(&hookShowScrollBar) },
	{ "EnableScrollBar", reinterpret_cast<void*>(&hookEnableScrollBar) },
};

bool isUser32Import(const char* aDll) noexcept {
	// Delay loaded imports go through the API sets
	return _stricmp(aDll, "user32.dll") == 0 || _strnicmp(aDll, "ext-ms-win-ntuser-", 18) == 0 || _strnicmp(aDll, "api-ms-win-ntuser-", 18) == 0;
}

void patchImportThunks(BYTE* aBase, const IMAGE_THUNK_DATA* aNames, IMAGE_THUNK_DATA* aAddresses) noexcept {
	for (; aNames->u1.AddressOfData; ++aNames, ++aAddresses) {
		if (IMAGE_SNAP_BY_ORDINAL(aNames->u1.Ordinal)) {
			continue;
		}

		auto name = reinterpret_cast<const char*>(reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(aBase + aNames->u1.AddressOfData)->Name);
		for (const auto& h : scrollBarHooks) {
			if (strcmp(name, h.name) != 0) {
				continue;
			}

			// Unresolved delay loaded imports point to the loader stub, which is never needed after this
			DWORD oldProtect;
			if (VirtualProtect(&aAddresses->u1.Function, sizeof(aAddresses->u1.Function), PAGE_READWRITE, &oldProtect)) {
				aAddresses->u1.Function = reinterpret_cast<ULONG_PTR>(h.hook);
				VirtualProtect(&aAddresses->u1.Function, sizeof(aAddresses->u1.Function), oldProtect, &oldProtect);
			}
			break;
		}
	}
}

void patchScrollBarImports(HMODULE aModule) noexcept {
	if (!origSetScrollInfo) {
		auto user32Module = GetModuleHandle(_T("user32.dll"));
		origSetScrollInfo = reinterpret_cast<decltype(origSetScrollInfo)>(GetProcAddress(user32Module, "SetScrollInfo"));
		origSetScrollPos = reinterpret_cast<decltype(origSetScrollPos)>(GetProcAddress(user32Module, "SetScrollPos"));
		origSetScrollRange = reinterpret_cast<decltype(origSetScrollRange)>(GetProcAddress(user32Module, "SetScrollRange"));
		origShowScrollBar = reinterpret_cast<decltype(origShowScrollBar)>(GetProcAddress(user32Module, "ShowScrollBar"));
		origEnableScrollBar = reinterpret_cast<decltype(origEnableScrollBar)>(GetProcAddress(user32Module, "EnableScrollBar"));
		if (!origSetScrollInfo || !origSetScrollPos || !origSetScrollRange || !origShowScrollBar || !origEnableScrollBar) {
			origSetScrollInfo = nullptr;
			return;
		}
	}

	auto base = reinterpret_cast<BYTE*>(aModule);
	auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
	if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
		return;
	}

	auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE) {
		return;
	}

	const auto& imports = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if (imports.VirtualAddress) {
		for (auto desc = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(base + imports.VirtualAddress); desc->Name; ++desc) {
			if (desc->OriginalFirstThunk && isUser32Import(reinterpret_cast<const char*>(base + desc->Name))) {
				patchImportThunks(base, reinterpret_cast<const IMAGE_THUNK_DATA*>(base + desc->OriginalFirstThunk), reinterpret_cast<IMAGE_THUNK_DATA*>(base + desc->FirstThunk));
			}
		}
	}

	const auto& delayImports = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT];
	if (delayImports.VirtualAddress) {
		for (auto desc = reinterpret_cast<const IMAGE_DELAYLOAD_DESCRIPTOR*>(base + delayImports.VirtualAddress); desc->DllNameRVA; ++desc) {
			if (desc->Attributes.RvaBased && isUser32Import(reinterpret_cast<const char*>(base + desc->DllNameRVA))) {
				patchImportThunks(base, reinterpret_cast<const IMAGE_THUNK_DATA*>(base + desc->ImportNameTableRVA), reinterpret_cast<IMAGE_THUNK_DATA*>(base + desc->ImportAddressTableRVA));
			}
		}
	}
}

void subclassWindow(HWND hWnd, WindowType aType) noexcept {
	DWORD_PTR existing = 0;
	if (GetWindowSubclass(hWnd, subclassProc, SUBCLASS_ID, &existing)) {
		return;
	}

	SetWindowSubclass(hWnd, subclassProc, SUBCLASS_ID, static_cast<DWORD_PTR>(aType));

	if (aType == TYPE_RICHEDIT) {
		static bool richEditPatched = false;
		if (!richEditPatched) {
			// Loaded on demand
			richEditPatched = true;
			if (auto module = GetModuleHandle(_T("msftedit.dll"))) {
				patchScrollBarImports(module);
			}
		}
	}
}

// Other user32 windows (such as menus) must not be touched
bool isMessageBox(HWND hWnd) noexcept {
	TCHAR buf[8];
	return GetClassName(hWnd, buf, 8) && _tcscmp(buf, _T("#32770")) == 0;
}

// Menu windows are run by the system and can't be subclassed: the light non-client frame of owner drawn menus
// is painted over while the menu loop is idle
BOOL CALLBACK paintMenuFrameProc(HWND hWnd, LPARAM /*lParam*/) noexcept {
	TCHAR buf[8];
	if (!IsWindowVisible(hWnd) || !GetClassName(hWnd, buf, 8) || _tcscmp(buf, _T("#32768")) != 0) {
		return TRUE;
	}

	RECT rcWindow, rcClient;
	GetWindowRect(hWnd, &rcWindow);
	GetClientRect(hWnd, &rcClient);
	MapWindowPoints(hWnd, nullptr, reinterpret_cast<LPPOINT>(&rcClient), 2);
	OffsetRect(&rcClient, -rcWindow.left, -rcWindow.top);
	OffsetRect(&rcWindow, -rcWindow.left, -rcWindow.top);

	auto hdc = GetWindowDC(hWnd);
	ExcludeClipRect(hdc, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);
	FillRect(hdc, &rcWindow, DarkMode::getSysColorBrush(COLOR_MENU));
	ReleaseDC(hWnd, hdc);
	return TRUE;
}

// Windows are subclassed after WM_CREATE because ATL replaces the window procedure of its windows on creation
LRESULT CALLBACK callWndProcRetHook(int nCode, WPARAM wParam, LPARAM lParam) noexcept {
	auto cwp = reinterpret_cast<const CWPRETSTRUCT*>(lParam);
	if (nCode == HC_ACTION && cwp->message == WM_ENTERIDLE && cwp->wParam == MSGF_MENU && DarkMode::isEnabled()) {
		EnumThreadWindows(GetCurrentThreadId(), paintMenuFrameProc, 0);
	}

	if (nCode == HC_ACTION && cwp->message == WM_CREATE && cwp->lResult != -1) {
		// Only theme our own windows, windows created by the common controls and message boxes
		// (common dialogs have their own dark mode support)
		auto instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(cwp->hwnd, GWLP_HINSTANCE));
		if (instance && (instance == _Module.GetModuleInstance() || instance == _Module.GetResourceInstance() || instance == comctl32 || (instance == user32 && isMessageBox(cwp->hwnd)))) {
			subclassWindow(cwp->hwnd, instance == user32 ? TYPE_MESSAGEBOX : getWindowType(cwp->hwnd));

			auto type = getSubclassedType(cwp->hwnd);
			applyWindowTheme(cwp->hwnd, type);

			// Application code may set the light Explorer theme after the window has been created
			if (type == TYPE_LISTVIEW || type == TYPE_TREEVIEW || type == TYPE_COMBO || type == TYPE_DIALOG || type == TYPE_MESSAGEBOX || type == TYPE_TOOLBAR) {
				PostMessage(cwp->hwnd, WM_APPLY_THEME, 0, 0);
			}
		}
	}

	return CallNextHookEx(hook, nCode, wParam, lParam);
}

BOOL CALLBACK applyChildProc(HWND hWnd, LPARAM /*lParam*/) noexcept {
	DWORD_PTR type = 0;
	if (GetWindowSubclass(hWnd, subclassProc, SUBCLASS_ID, &type)) {
		applyWindowTheme(hWnd, static_cast<WindowType>(type));
	}

	RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);
	return TRUE;
}

BOOL CALLBACK applyThreadWindowProc(HWND hWnd, LPARAM /*lParam*/) noexcept {
	DarkMode::apply(hWnd);
	return TRUE;
}

DWORD getWindowsBuild() noexcept {
	using RtlGetVersionF = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
	auto rtlGetVersion = reinterpret_cast<RtlGetVersionF>(GetProcAddress(GetModuleHandle(_T("ntdll.dll")), "RtlGetVersion"));

	RTL_OSVERSIONINFOW info = { sizeof(RTL_OSVERSIONINFOW) };
	if (!rtlGetVersion || rtlGetVersion(&info) != 0 || info.dwMajorVersion < 10) {
		return 0;
	}

	return info.dwBuildNumber;
}

bool getDarkModeEnabled() noexcept {
	switch (SETTING(WINDOW_THEME)) {
		case SettingsManager::WINDOW_THEME_DARK: return true;
		case SettingsManager::WINDOW_THEME_LIGHT: return false;
		default: return shouldAppsUseDarkMode && shouldAppsUseDarkMode();
	}
}

void setColorDefaults(bool aDark) noexcept {
	auto sm = SettingsManager::getInstance();
	for (auto& c : colorDefaults) {
		// Colors saved with the default value of the other theme haven't been customized
		auto previousDefault = static_cast<int>(aDark ? c.light : c.dark);
		if (sm->isKeySet(c.setting) && sm->get(c.setting, false) == previousDefault) {
			sm->unsetKey(c.setting);
		}

		sm->setDefault(c.setting, static_cast<int>(aDark ? c.dark : c.light));
	}
}

}

void DarkMode::init() noexcept {
	// The ordinals are valid since Windows 10 1903 (build 18362)
	if (getWindowsBuild() < 18362) {
		return;
	}

	auto uxtheme = GetModuleHandle(_T("uxtheme.dll"));
	if (uxtheme) {
		refreshImmersiveColorPolicyState = reinterpret_cast<RefreshImmersiveColorPolicyStateF>(GetProcAddress(uxtheme, MAKEINTRESOURCEA(104)));
		shouldAppsUseDarkMode = reinterpret_cast<ShouldAppsUseDarkModeF>(GetProcAddress(uxtheme, MAKEINTRESOURCEA(132)));
		allowDarkModeForWindow = reinterpret_cast<AllowDarkModeForWindowF>(GetProcAddress(uxtheme, MAKEINTRESOURCEA(133)));
		setPreferredAppMode = reinterpret_cast<SetPreferredAppModeF>(GetProcAddress(uxtheme, MAKEINTRESOURCEA(135)));
		flushMenuThemes = reinterpret_cast<FlushMenuThemesF>(GetProcAddress(uxtheme, MAKEINTRESOURCEA(136)));
	}

	if (!shouldAppsUseDarkMode || !allowDarkModeForWindow || !setPreferredAppMode) {
		// Not supported
		shouldAppsUseDarkMode = nullptr;
		allowDarkModeForWindow = nullptr;
		setPreferredAppMode = nullptr;
		return;
	}

	auto sm = SettingsManager::getInstance();
	for (auto& c : colorDefaults) {
		c.light = static_cast<COLORREF>(sm->getDefault(c.setting));
	}

	user32 = GetModuleHandle(_T("user32.dll"));
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCTSTR>(&InitCommonControlsEx), &comctl32);
	if (comctl32) {
		patchScrollBarImports(comctl32);
	}

	hook = SetWindowsHookEx(WH_CALLWNDPROCRET, callWndProcRetHook, nullptr, GetCurrentThreadId());

	update();
}

void DarkMode::uninit() noexcept {
	if (hook) {
		UnhookWindowsHookEx(hook);
		hook = nullptr;
	}
}

bool DarkMode::isWindowsAppModeDark() noexcept {
	DWORD lightTheme = 1;
	DWORD size = sizeof(lightTheme);
	RegGetValue(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"), _T("AppsUseLightTheme"), RRF_RT_REG_DWORD, nullptr, &lightTheme, &size);
	return lightTheme == 0;
}

bool DarkMode::isDarkThemeSelected() noexcept {
	switch (SETTING(WINDOW_THEME)) {
		case SettingsManager::WINDOW_THEME_DARK: return true;
		case SettingsManager::WINDOW_THEME_LIGHT: return false;
		default: return isWindowsAppModeDark();
	}
}

bool DarkMode::isThemeChangeMessage(UINT aMsg, LPARAM lParam) noexcept {
	return aMsg == WM_SETTINGCHANGE && lParam && CompareStringOrdinal(reinterpret_cast<LPCWCH>(lParam), -1, L"ImmersiveColorSet", -1, TRUE) == CSTR_EQUAL;
}

bool DarkMode::update() noexcept {
	if (!setPreferredAppMode) {
		return false;
	}

	if (refreshImmersiveColorPolicyState) {
		refreshImmersiveColorPolicyState();
	}

	switch (SETTING(WINDOW_THEME)) {
		case SettingsManager::WINDOW_THEME_DARK: setPreferredAppMode(PreferredAppMode::ForceDark); break;
		case SettingsManager::WINDOW_THEME_LIGHT: setPreferredAppMode(PreferredAppMode::ForceLight); break;
		default: setPreferredAppMode(PreferredAppMode::AllowDark); break;
	}

	if (flushMenuThemes) {
		flushMenuThemes();
	}

	auto dark = getDarkModeEnabled();
	if (dark == enabled) {
		return false;
	}

	enabled = dark;
	setColorDefaults(dark);

	EnumThreadWindows(GetCurrentThreadId(), applyThreadWindowProc, 0);
	return true;
}

COLORREF DarkMode::getSysColor(int aIndex) noexcept {
	if (!enabled) {
		return GetSysColor(aIndex);
	}

	switch (aIndex) {
		case COLOR_3DFACE:
		case COLOR_MENUBAR:
			return DARK_FACE;
		case COLOR_MENU:
			return DARK_MENU;
		case COLOR_MENUHILIGHT:
		{
			// Accent color blended with the background
			auto accent = GetSysColor(COLOR_HIGHLIGHT);
			return RGB(
				(GetRValue(accent) * 55 + GetRValue(DARK_FACE) * 45) / 100,
				(GetGValue(accent) * 55 + GetGValue(DARK_FACE) * 45) / 100,
				(GetBValue(accent) * 55 + GetBValue(DARK_FACE) * 45) / 100
			);
		}
		case COLOR_WINDOW:
		case COLOR_INFOBK:
			return DARK_CONTROL;
		case COLOR_APPWORKSPACE:
			return DARK_WORKSPACE;
		case COLOR_WINDOWTEXT:
		case COLOR_BTNTEXT:
		case COLOR_MENUTEXT:
		case COLOR_INFOTEXT:
		case COLOR_CAPTIONTEXT:
			return DARK_TEXT;
		case COLOR_GRAYTEXT:
			return DARK_DISABLED_TEXT;
		case COLOR_3DSHADOW:
			return DARK_SHADOW;
		case COLOR_3DDKSHADOW:
			return DARK_DARK_SHADOW;
		case COLOR_3DLIGHT:
			return DARK_LIGHT;
		case COLOR_3DHILIGHT:
		case COLOR_WINDOWFRAME:
			return DARK_EDGE;
		default:
			return GetSysColor(aIndex);
	}
}

HBRUSH DarkMode::getSysColorBrush(int aIndex) noexcept {
	if (!enabled) {
		return GetSysColorBrush(aIndex);
	}

	static HBRUSH brushes[COLOR_MENUBAR + 1] = { nullptr };
	if (aIndex < 0 || aIndex > COLOR_MENUBAR) {
		return GetSysColorBrush(aIndex);
	}

	auto color = getSysColor(aIndex);
	if (color == GetSysColor(aIndex)) {
		return GetSysColorBrush(aIndex);
	}

	if (!brushes[aIndex]) {
		brushes[aIndex] = CreateSolidBrush(color);
	}

	return brushes[aIndex];
}

void DarkMode::apply(HWND hWnd) noexcept {
	if (!setPreferredAppMode) {
		return;
	}

	applyChildProc(hWnd, 0);
	EnumChildWindows(hWnd, applyChildProc, 0);
}

}
