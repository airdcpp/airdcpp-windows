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

#include <windows/util/UserUtil.h>

#include <windows/ResourceLoader.h>
#include <windows/util/FormatUtil.h>

#include <airdcpp/user/OnlineUser.h>
#include <airdcpp/settings/SettingsManager.h>


namespace wingui {
uint8_t UserUtil::getUserImageIndex(const OnlineUserPtr& aUser) noexcept {
	const auto& identity = aUser->getIdentity();
	return getIdentityImage(identity, identity.hasActiveTcpConnectivity(aUser->getClient()));
}

uint8_t UserUtil::getIdentityImage(const Identity& identity, bool aIsClientTcpActive) {

	bool bot = identity.isBot() && !identity.getUser()->isSet(User::NMDC);
	uint8_t image = bot ? ResourceLoader::USER_ICON_BOT : identity.isAway() ? ResourceLoader::USER_ICON_AWAY : ResourceLoader::USER_ICON;
	image *= (ResourceLoader::USER_ICON_LAST - ResourceLoader::USER_ICON_MOD_START) * (ResourceLoader::USER_ICON_LAST - ResourceLoader::USER_ICON_MOD_START);

	if (identity.getUser()->isNMDC() || identity.isMe()) {
		// NMDC / me
		if (!bot && !aIsClientTcpActive) {
			image += 1 << (ResourceLoader::USER_ICON_PASSIVE - ResourceLoader::USER_ICON_MOD_START);
		}
	} else {
		// ADC
		const auto cm = identity.getTcpConnectMode();
		if (!bot && (cm == Identity::MODE_PASSIVE_V6 || cm == Identity::MODE_PASSIVE_V4)) {
			image += 1 << (ResourceLoader::USER_ICON_PASSIVE - ResourceLoader::USER_ICON_MOD_START);
		}

		if (!bot && (cm == Identity::MODE_NOCONNECT_PASSIVE || cm == Identity::MODE_NOCONNECT_IP || cm == Identity::MODE_UNDEFINED)) {
			image += 1 << (ResourceLoader::USER_ICON_NOCONNECT - ResourceLoader::USER_ICON_MOD_START);
		}

		//TODO: add icon for unknown (passive) connectivity
		if (!bot && (cm == Identity::MODE_PASSIVE_V4_UNKNOWN || cm == Identity::MODE_PASSIVE_V6_UNKNOWN)) {
			image += 1 << (ResourceLoader::USER_ICON_PASSIVE - ResourceLoader::USER_ICON_MOD_START);
		}
	}

	if (identity.isOp()) {
		image += 1 << (ResourceLoader::USER_ICON_OP - ResourceLoader::USER_ICON_MOD_START);
	}
	return image;
}

int UserUtil::compareUsers(const OnlineUserPtr& a, const OnlineUserPtr& b, uint8_t col) noexcept {
	if (col == COLUMN_NICK) {
		bool a_isOp = a->getIdentity().isOp(),
			b_isOp = b->getIdentity().isOp();
		if (a_isOp && !b_isOp)
			return -1;
		if (!a_isOp && b_isOp)
			return 1;
		if (SETTING(SORT_FAVUSERS_FIRST)) {
			bool a_isFav = a->getUser()->isFavorite(),
				b_isFav = b->getUser()->isFavorite();

			if (a_isFav && !b_isFav)
				return -1;
			if (!a_isFav && b_isFav)
				return 1;
		}
		// workaround for faster hub loading
		// lstrcmpiA(a->identity.getNick().c_str(), b->identity.getNick().c_str());
	}
	else if (!a->getUser()->isNMDC()) {
		if (col == COLUMN_ULSPEED) {
			return compare(a->getIdentity().getAdcConnectionSpeed(false), b->getIdentity().getAdcConnectionSpeed(false));
		}
		else if (col == COLUMN_DLSPEED) {
			return compare(a->getIdentity().getAdcConnectionSpeed(true), b->getIdentity().getAdcConnectionSpeed(true));
		}
	}

	switch (col) {
	case COLUMN_SHARED:
	case COLUMN_EXACT_SHARED: return compare(a->getIdentity().getBytesShared(), b->getIdentity().getBytesShared());
	case COLUMN_SLOTS: return compare(Util::toInt(a->getIdentity().get("SL")), Util::toInt(b->getIdentity().get("SL")));
	case COLUMN_HUBS: return compare(a->getIdentity().getTotalHubCount(), b->getIdentity().getTotalHubCount());
	case COLUMN_FILES: return compare(Util::toInt64(a->getIdentity().get("SF")), Util::toInt64(b->getIdentity().get("SF")));
	}
	return Util::DefaultSort(getUserText(a, col).c_str(), getUserText(b, col).c_str());
}

tstring UserUtil::getUserText(const OnlineUserPtr& aUser, uint8_t col, bool copy /*false*/) noexcept {
	const auto& identity = aUser->getIdentity();

	switch (col) {
	case COLUMN_NICK: return Text::toT(identity.getNick());
	case COLUMN_SHARED: return Util::formatBytesW(identity.getBytesShared());
	case COLUMN_EXACT_SHARED: return FormatUtil::formatExactSizeW(identity.getBytesShared());
	case COLUMN_DESCRIPTION: return Text::toT(identity.getDescription());
	case COLUMN_TAG: return Text::toT(identity.getTag());
	case COLUMN_ULSPEED: return identity.get("US").empty() ? Text::toT(identity.getNmdcConnection()) : (FormatUtil::formatConnectionSpeedW(identity.getAdcConnectionSpeed(false)));
	case COLUMN_DLSPEED: return identity.get("DS").empty() ? Util::emptyStringT : (FormatUtil::formatConnectionSpeedW(identity.getAdcConnectionSpeed(true)));
	case COLUMN_IP4: {
		string ip = identity.getIp4();
		if (!copy) {
			string country = ip.empty() ? Util::emptyString : identity.getCountry();
			if (!country.empty())
				ip = country + " (" + ip + ")";
		}
		return Text::toT(ip);
	}
	case COLUMN_IP6: {
		string ip = identity.getIp6();
		if (!copy) {
			string country = ip.empty() ? Util::emptyString : identity.getCountry();
			if (!country.empty())
				ip = country + " (" + ip + ")";
		}
		return Text::toT(ip);
	}
	case COLUMN_EMAIL: return Text::toT(identity.getEmail());
	case COLUMN_VERSION: return Text::toT(identity.get("CL").empty() ? identity.get("VE") : identity.get("CL"));
	case COLUMN_MODE4: return Text::toT(identity.getV4ModeString());
	case COLUMN_MODE6: return Text::toT(identity.getV6ModeString());
	case COLUMN_FILES: return Text::toT(identity.get("SF"));
	case COLUMN_HUBS: {
		const tstring hn = Text::toT(identity.get("HN"));
		const tstring hr = Text::toT(identity.get("HR"));
		const tstring ho = Text::toT(identity.get("HO"));
		return (hn.empty() || hr.empty() || ho.empty()) ? Util::emptyStringT : (hn + _T("/") + hr + _T("/") + ho);
	}
	case COLUMN_SLOTS: return Text::toT(identity.get("SL"));
	case COLUMN_CID: return Text::toT(identity.getUser()->getCID().toBase32());
	default: return Util::emptyStringT;
	}
}
}