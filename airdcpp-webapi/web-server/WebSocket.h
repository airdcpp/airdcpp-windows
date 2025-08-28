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

#ifndef DCPLUSPLUS_WEBSERVER_WEBSOCKET_H
#define DCPLUSPLUS_WEBSERVER_WEBSOCKET_H

#include "stdinc.h"

#include "forward.h"
#include "IServerEndpoint.h"

#include <airdcpp/core/types/GetSet.h>

namespace webserver {
	// WebSockets are owned by SocketManager and API modules
	class WebServerManager;

	class WebSocket {
	public:
		WebSocket(bool aIsSecure, ConnectionHdl aHdl, IServerEndpoint& aEndpoint, WebServerManager* aWsm);
		~WebSocket();

		void close(uint16_t aCode, const std::string& aMsg);

		IGETSET(SessionPtr, session, Session, nullptr);

		// Send raw data
		// Throws json::exception on JSON conversion errors
		void sendPlain(const json& aJson);
		void sendApiResponse(const json& aJsonResponse, const json& aErrorJson, http_status aCode, int aCallbackId) noexcept;

		void onData(const string& aPayload, const SessionCallback& aAuthCallback);

		WebSocket(WebSocket&) = delete;
		WebSocket& operator=(WebSocket&) = delete;

		const string& getIp() const noexcept {
			return ip;
		}

		void ping() noexcept;

		void logError(const string& aMessage) const noexcept;
		void debugMessage(const string& aMessage) const noexcept;

		time_t getTimeCreated() const noexcept {
			return timeCreated;
		}

		const string& getConnectUrl() const noexcept {
			return url;
		}

		// Throws json exception (from the json library) in case of invalid JSON, ArgumentException in case of invalid properties
		static void parseRequest(const string& aRequest, int& callbackId_, string& method_, string& path_, json& data_);
	private:
		const ConnectionHdl hdl;
		IServerEndpoint& endpoint;
		WebServerManager* wsm;
		const bool secure;
		const time_t timeCreated;
		string url;
		string ip;
	};
}

#endif