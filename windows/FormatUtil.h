/*
 * Copyright (C) 2001-2021 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef FORMAT_UTIL_H
#define FORMAT_UTIL_H

#include <airdcpp/typedefs.h>

#include <airdcpp/HintedUser.h>
#include <airdcpp/Util.h>



class FormatUtil {
public:
	static tstring getNicks(const CID& cid);
	static tstring getNicks(const HintedUser& user);

	/** @return Pair of hubnames as a string and a bool representing the user's online status */
	static pair<tstring, bool> getHubNames(const CID& cid);
	static pair<tstring, bool> getHubNames(const UserPtr& u) { return getHubNames(u->getCID()); }
	static tstring getHubNames(const HintedUser& user);

	static tstring formatFolderContent(const DirectoryContentInfo& aContentInfo);
	static tstring formatFileType(const string& aFileName);

	struct CountryFlagInfo {
		tstring text;
		uint8_t flagIndex = 0;
	};

	static CountryFlagInfo toCountryInfo(const string& aIP) noexcept;
};


#endif // !defined(FORMAT_UTIL_H)