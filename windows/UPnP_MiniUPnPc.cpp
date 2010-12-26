/*
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
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

#include "UPnP_MiniUPnPc.h"
#include "../client/DCPlusPlus.h"
#include "../client/Util.h"

#ifndef STATICLIB
#define STATICLIB
#endif
#include "../miniupnpc/miniupnpc.h"
#include "../miniupnpc/upnpcommands.h"

static UPNPUrls urls;
static IGDdatas data;
const string UPnP_MiniUPnPc::name = "MiniUPnP";

bool UPnP_MiniUPnPc::init() {
	UPNPDev* devices = upnpDiscover(2000, 0, 0, 0);
	if(!devices)
		return false;

	bool ret = UPNP_GetValidIGD(devices, &urls, &data, 0, 0);

	freeUPNPDevlist(devices);

	return ret;
}

bool UPnP_MiniUPnPc::add(const unsigned short port, const Protocol protocol, const string& description) {
	const string port_ = Util::toString(port);
	return UPNP_AddPortMapping(urls.controlURL, data.first.servicetype, port_.c_str(), port_.c_str(),
		Util::getLocalIp().c_str(), description.c_str(), protocols[protocol], 0) == UPNPCOMMAND_SUCCESS;
}

bool UPnP_MiniUPnPc::remove(const unsigned short port, const Protocol protocol) {
	return UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, Util::toString(port).c_str(),
		protocols[protocol], 0) == UPNPCOMMAND_SUCCESS;
}

string UPnP_MiniUPnPc::getExternalIP() {
	char buf[16] = { 0 };
	if(UPNP_GetExternalIPAddress(urls.controlURL, data.first.servicetype, buf) == UPNPCOMMAND_SUCCESS)
		return buf;
	return Util::emptyString;
}
