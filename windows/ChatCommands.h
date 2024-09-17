/*
 * Copyright (C) 2012-2024 AirDC++ Project
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

#ifndef CHATCOMMANDS_H
#define CHATCOMMANDS_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <airdcpp/typedefs.h>

namespace wingui {
class ChatCommands {
public:
	static tstring UselessInfo();
	static tstring Speedinfo();
	static tstring DiskSpaceInfo(bool onlyTotal = false);
	static string CPUInfo();
	
	static string getSysUptime();
	static tstring diskInfo();
	static string generateStats();
	static string uptimeInfo();

	static string ClientVersionInfo();

	static TStringList FindVolumes();
	static string shareStats() noexcept;
	static string hubStats() noexcept;
};
}

#endif // !defined(CHATCOMMANDS_H)