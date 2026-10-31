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

#ifndef DARK_MODE_H
#define DARK_MODE_H

namespace wingui {

// Dark window theme (Windows 10 1809 or newer)
//
// The theme follows the Windows app mode unless the WINDOW_THEME setting forces it.
// While dark mode is enabled, color settings that haven't been customized use dark defaults
// and the standard controls created by the application are themed (or painted) dark.
class DarkMode {
public:
	// Call after the settings have been loaded, before creating the main window
	static void init() noexcept;
	static void uninit() noexcept;

	static bool isEnabled() noexcept { return enabled; }

	// The settings have been loaded and the window theme has been applied
	static bool isInitialized() noexcept { return initialized; }

	// Re-evaluates the theme after the setting or the Windows app mode has changed
	// Returns true if the theme was switched (the colors must be reloaded by the caller)
	static bool update() noexcept;

	// Windows app mode (usable before the settings have been loaded)
	static bool isWindowsAppModeDark() noexcept;

	// Is the dark theme selected (WINDOW_THEME setting, the Windows app mode when automatic)?
	// Usable before init() (the default setting is used if the settings haven't been loaded yet)
	static bool isDarkThemeSelected() noexcept;

	// Is this WM_SETTINGCHANGE about a change in the Windows app mode?
	static bool isThemeChangeMessage(UINT aMsg, LPARAM lParam) noexcept;

	// Theme-aware versions of GetSysColor and GetSysColorBrush (the brushes must not be deleted)
	static COLORREF getSysColor(int aIndex) noexcept;
	static HBRUSH getSysColorBrush(int aIndex) noexcept;

	// Applies the current theme to a window and its children (windows created after init() are handled automatically)
	static void apply(HWND hWnd) noexcept;
private:
	static bool enabled;
	static bool initialized;
};

}

#endif // !defined(DARK_MODE_H)
