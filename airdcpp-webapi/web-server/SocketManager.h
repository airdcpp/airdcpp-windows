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

#ifndef DCPLUSPLUS_WEBSERVER_SOCKET_MANAGER_H
#define DCPLUSPLUS_WEBSERVER_SOCKET_MANAGER_H

#include "forward.h"
#include "stdinc.h"

#include "SocketManagerListener.h"
#include "WebSocket.h"
#include "WebUserManagerListener.h"
#include "IServerEndpoint.h"

#include <airdcpp/core/Speaker.h>


namespace webserver {
	class WebServerManager;
	class SocketManager : public Speaker<SocketManagerListener>, private WebUserManagerListener {
	public:
		explicit SocketManager(WebServerManager* aWsm) noexcept : wsm(aWsm) {}

		void start();
		void stop() noexcept;

		// Disconnect all sockets
		void disconnectSockets(const std::string& aMessage) noexcept;

		// Reset sessions for associated sockets
		WebSocketPtr getSocket(LocalSessionId aSessionToken) noexcept;

		SocketManager(SocketManager&) = delete;
		SocketManager& operator=(SocketManager&) = delete;

		void setEndpointHandlers(IServerEndpoint& aEndpoint, bool aIsSecure) {
			aEndpoint.set_message_handler(std::bind_front(&SocketManager::handleSocketMessage, this));
			aEndpoint.set_close_handler(std::bind_front(&SocketManager::handleSocketDisconnected, this));
			aEndpoint.set_open_handler(std::bind_front(&SocketManager::handleSocketConnected, this, std::ref(aEndpoint), aIsSecure));
			aEndpoint.set_pong_timeout_handler(std::bind_front(&SocketManager::handlePongTimeout, this));
		}
	private:
		void onAuthenticated(const SessionPtr& aSession, const WebSocketPtr& aSocket) noexcept;

		void handleSocketConnected(IServerEndpoint& ep, bool aIsSecure, ConnectionHdl hdl);
		void handleSocketDisconnected(ConnectionHdl hdl);

		void handlePongReceived(ConnectionHdl hdl, const string& aPayload);
		void handlePongTimeout(ConnectionHdl hdl, const string& aPayload);

		void handleSocketMessage(ConnectionHdl hdl, const std::string& payload);

		void addSocket(ConnectionHdl hdl, const WebSocketPtr& aSocket) noexcept;
		WebSocketPtr getSocket(ConnectionHdl hdl) const noexcept;

		void pingTimer() noexcept;

		mutable SharedMutex cs;

		using WebSocketList = vector<WebSocketPtr>;
		std::map<ConnectionHdl, WebSocketPtr, std::owner_less<ConnectionHdl>> sockets;

		TimerPtr socketPingTimer;
		WebServerManager* wsm;

		void resetSocketSession(const WebSocketPtr& aSocket) noexcept;

		void on(WebUserManagerListener::SessionRemoved, const SessionPtr& aSession, int aReason) noexcept override;
	};
}

#endif // DCPLUSPLUS_DCPP_WEBSERVER_H
