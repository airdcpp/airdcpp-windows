/*
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
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

#include "SystemUtil.h"

#include <mmsystem.h>
#include <powrprof.h>

#include <airdcpp/Text.h>

#define byte BYTE // 'byte': ambiguous symbol (C++17)
#include <atlcomtime.h>
#undef byte


string SystemUtil::getCompileDate() {
	COleDateTime tCompileDate;
	tCompileDate.ParseDateTime(_T(__DATE__), LOCALE_NOUSEROVERRIDE, 1033);
	return Text::fromT(tCompileDate.Format(_T("%d.%m.%Y")).GetString());
}

double SystemUtil::getFontFactor() {
	return static_cast<float>(::GetDeviceCaps(::GetDC(reinterpret_cast<HWND>(0)), LOGPIXELSX)) / 96.0;
}

void SystemUtil::playSound(const tstring& sound) {
	if (sound == _T("beep")) {
		::MessageBeep(MB_OK);
	}
	else {
		::PlaySound(sound.c_str(), 0, SND_FILENAME | SND_ASYNC);
	}
}

bool SystemUtil::getVersionInfo(OSVERSIONINFOEX& ver) {
	memzero(&ver, sizeof(OSVERSIONINFOEX));
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if (!GetVersionEx((OSVERSIONINFO*)&ver)) {
		ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if (!GetVersionEx((OSVERSIONINFO*)&ver)) {
			return false;
		}
	}
	return true;
}

bool SystemUtil::shutdown(int action) {
	// Prepare for shutdown
	UINT iForceIfHung = 0;
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	if (GetVersionEx(&osvi) != 0 && osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
		iForceIfHung = 0x00000010;
		HANDLE hToken;
		OpenProcessToken(GetCurrentProcess(), (TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY), &hToken);

		LUID luid;
		LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &luid);

		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		tp.Privileges[0].Luid = luid;
		AdjustTokenPrivileges(hToken, FALSE, &tp, 0, (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL);
		CloseHandle(hToken);
	}

	// Shutdown
	switch (action) {
	case 0: { action = EWX_POWEROFF; break; }
	case 1: { action = EWX_LOGOFF; break; }
	case 2: { action = EWX_REBOOT; break; }
	case 3: { SetSuspendState(false, false, false); return true; }
	case 4: { SetSuspendState(true, false, false); return true; }
	case 5: {
		typedef bool (CALLBACK* LPLockWorkStation)(void);
		LPLockWorkStation _d_LockWorkStation = (LPLockWorkStation)GetProcAddress(LoadLibrary(_T("user32")), "LockWorkStation");
		_d_LockWorkStation();
		return true;
	}
	}

	if (ExitWindowsEx(action | iForceIfHung, 0) == 0) {
		return false;
	}
	else {
		return true;
	}
}

time_t SystemUtil::fromSystemTime(const SYSTEMTIME* pTime) {
	struct tm tm;
	memset(&tm, 0, sizeof(tm));

	tm.tm_year = pTime->wYear - 1900;
	tm.tm_mon = pTime->wMonth - 1;
	tm.tm_mday = pTime->wDay;

	tm.tm_hour = pTime->wHour;
	tm.tm_min = pTime->wMinute;
	tm.tm_sec = pTime->wSecond;
	tm.tm_wday = pTime->wDayOfWeek;

	return mktime(&tm);
}

void SystemUtil::toSystemTime(const time_t aTime, SYSTEMTIME* sysTime) {
	tm _tm;
	localtime_s(&_tm, &aTime);

	sysTime->wYear = _tm.tm_year + 1900;
	sysTime->wMonth = _tm.tm_mon + 1;
	sysTime->wDay = _tm.tm_mday;

	sysTime->wHour = _tm.tm_hour;

	sysTime->wMinute = _tm.tm_min;
	sysTime->wSecond = _tm.tm_sec;
	sysTime->wDayOfWeek = _tm.tm_wday;
	sysTime->wMilliseconds = 0;
}

/*void SystemUtil::addUpdate(const string& aUpdater, bool aTesting) noexcept {
	updated = true;
	auto appPath = Util::getAppFilePath();

	auto updateCmd = Text::toT("/update \"" + appPath + "\\\""); // The extra end slash is required!
	if (isElevated()) {
		updateCmd += _T(" /elevation");
	}

	if (aTesting) {
		updateCmd += _T(" /test");
	}

	updateCommand = make_pair(Text::toT(aUpdater), updateCmd);
}

bool SystemUtil::runPendingUpdate() noexcept {
	if (updated && !updateCommand.first.empty()) {
		auto cmd = updateCommand.second + Text::toT(Util::getStartupParams(false));
		ShellExecute(NULL, _T("runas"), updateCommand.first.c_str(), cmd.c_str(), NULL, SW_SHOWNORMAL);
		return true;
	}

	return false;
}*/

bool SystemUtil::isElevated() {
	BOOL fRet = FALSE;
	HANDLE hToken = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
		TOKEN_ELEVATION Elevation;
		DWORD cbSize = sizeof(TOKEN_ELEVATION);
		if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
			fRet = Elevation.TokenIsElevated;
		}
	}
	if (hToken) {
		CloseHandle(hToken);
	}
	return fRet ? true : false;
}

