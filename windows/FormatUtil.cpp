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

#include "stdafx.h"

#include "FormatUtil.h"

#include <airdcpp/ClientManager.h>
#include <airdcpp/GeoManager.h>
#include <airdcpp/Localization.h>
#include <airdcpp/Util.h>


tstring FormatUtil::getNicks(const CID& cid) {
	return Text::toT(Util::listToString(ClientManager::getInstance()->getNicks(cid)));
}

tstring FormatUtil::getNicks(const HintedUser& user) {
	return Text::toT(ClientManager::getInstance()->getFormatedNicks(user));
}


tstring FormatUtil::formatFolderContent(const DirectoryContentInfo& aContentInfo) {
	return Text::toT(Util::formatDirectoryContent(aContentInfo));
}

tstring FormatUtil::formatFileType(const string& aFileName) {
	auto type = Util::getFileExt(aFileName);
	if (type.size() > 0 && type[0] == '.')
		type.erase(0, 1);

	return Text::toT(type);
}

static pair<tstring, bool> formatHubNames(const StringList& hubs) {
	if (hubs.empty()) {
		return make_pair(CTSTRING(OFFLINE), false);
	} else {
		return make_pair(Text::toT(Util::listToString(hubs)), true);
	}
}

pair<tstring, bool> FormatUtil::getHubNames(const CID& cid) {
	return formatHubNames(ClientManager::getInstance()->getHubNames(cid));
}

tstring FormatUtil::getHubNames(const HintedUser& aUser) {
	return Text::toT(ClientManager::getInstance()->getFormatedHubNames(aUser));
}

FormatUtil::CountryFlagInfo FormatUtil::toCountryInfo(const string& aIP) noexcept {
	auto ip = aIP;
	uint8_t flagIndex = 0;
	if (!ip.empty()) {
		// Only attempt to grab a country mapping if we actually have an IP address
		auto tmpCountry = GeoManager::getInstance()->getCountry(aIP);
		if (!tmpCountry.empty()) {
			ip = tmpCountry + " (" + ip + ")";
			flagIndex = Localization::getFlagIndexByCode(tmpCountry.c_str());
		}
	}

	return {
		Text::toT(ip),
		flagIndex
	};
}
