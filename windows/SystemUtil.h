/*
 * Copyright (C) 2001-2023 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef SYSTEM_UTIL_H
#define SYSTEM_UTIL_H

#include <airdcpp/typedefs.h>


class SystemUtil {
public:
	static double getFontFactor();

	static bool isElevated();
	static void playSound(const tstring& sound);
	static bool getVersionInfo(OSVERSIONINFOEX& ver);

	static bool shutdown(int action);
	static string getCompileDate();

	static time_t fromSystemTime(const SYSTEMTIME* pTime);
	static void toSystemTime(const time_t aTime, SYSTEMTIME* sysTime);
};


#endif // !defined(SYSTEM_UTIL_H)