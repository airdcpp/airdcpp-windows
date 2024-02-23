/*
 * Copyright (C) 2011-2015 AirDC++ Project
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

#ifndef WIN_USERUTIL_H
#define WIN_USERUTIL_H


#include <airdcpp/typedefs.h>

class UserUtil
{
public:
	enum {
		COLUMN_FIRST,
		COLUMN_NICK = COLUMN_FIRST,
		COLUMN_SHARED,
		COLUMN_EXACT_SHARED,
		COLUMN_DESCRIPTION,
		COLUMN_TAG,
		COLUMN_ULSPEED,
		COLUMN_DLSPEED,
		COLUMN_IP4,
		COLUMN_IP6,
		COLUMN_EMAIL,
		COLUMN_VERSION,
		COLUMN_MODE4,
		COLUMN_MODE6,
		COLUMN_FILES,
		COLUMN_HUBS,
		COLUMN_SLOTS,
		COLUMN_CID,
		COLUMN_LAST
	};

	static uint8_t getUserImageIndex(const OnlineUserPtr& aUser) noexcept;
	static int compareUsers(const OnlineUserPtr& a, const OnlineUserPtr& b, uint8_t col) noexcept;
	static tstring getUserText(const OnlineUserPtr& aUser, uint8_t col, bool copy = false) noexcept;
	static uint8_t getIdentityImage(const Identity& identity, bool aIsClientTcpActive);
};

#endif // WIN_USERUTIL_H