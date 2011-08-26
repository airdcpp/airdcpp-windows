/*
 * Copyright (C) 2001-2011 Jacek Sieka, arnetheduck on gmail point com
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

#include "Mapper_MiniUPnPc.h"

#include "../client/SettingsManager.h"
#include "../client/Socket.h"
#include "../client/Util.h"

extern "C" {
#ifndef STATICLIB
#define STATICLIB
#endif
#include "../miniupnpc/miniupnpc.h"
#include "../miniupnpc/upnpcommands.h"
}

const string Mapper_MiniUPnPc::name = "MiniUPnP";

bool Mapper_MiniUPnPc::init() {
	if(initialized)
		return true;

	UPNPDev* devices = upnpDiscover(2000,
		SettingsManager::getInstance()->isDefault(SettingsManager::BIND_INTERFACE) ? nullptr : Socket::getBindAddress().c_str(),
		0, 0, 0, 0);
	if(!devices)
		return false;

	UPNPUrls urls;
	IGDdatas data;

	initialized = UPNP_GetValidIGD(devices, &urls, &data, 0, 0) == 1;
	if(initialized) {
		url = urls.controlURL;
		service = data.first.servicetype;
		device = data.CIF.friendlyName;
	}

	FreeUPNPUrls(&urls);
	freeUPNPDevlist(devices);

	return initialized;
}

void Mapper_MiniUPnPc::uninit() {
}

bool Mapper_MiniUPnPc::add(const unsigned short port, const Protocol protocol, const string& description) {
	const string port_ = Util::toString(port);
	return UPNP_AddPortMapping(url.c_str(), service.c_str(), port_.c_str(), port_.c_str(),
		Util::getLocalIp().c_str(), description.c_str(), protocols[protocol], 0, 0) == UPNPCOMMAND_SUCCESS;
}

bool Mapper_MiniUPnPc::remove(const unsigned short port, const Protocol protocol) {
	return UPNP_DeletePortMapping(url.c_str(), service.c_str(), Util::toString(port).c_str(),
		protocols[protocol], 0) == UPNPCOMMAND_SUCCESS;
}

string Mapper_MiniUPnPc::getDeviceName() {
	return device;
}

string Mapper_MiniUPnPc::getExternalIP() {
	char buf[16] = { 0 };
	if(UPNP_GetExternalIPAddress(url.c_str(), service.c_str(), buf) == UPNPCOMMAND_SUCCESS)
		return buf;
	return Util::emptyString;
}
