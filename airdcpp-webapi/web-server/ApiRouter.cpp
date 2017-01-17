/*
* Copyright (C) 2011-2016 AirDC++ Project
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

#include <web-server/stdinc.h>
#include <web-server/version.h>
#include <web-server/ApiRouter.h>

#include <web-server/ApiRequest.h>
#include <web-server/WebServerManager.h>
#include <web-server/WebUserManager.h>
#include <web-server/WebSocket.h>

#include <api/SessionApi.h>

#include <airdcpp/File.h>
#include <airdcpp/Util.h>
#include <airdcpp/StringTokenizer.h>

#include <sstream>

namespace webserver {
	ApiRouter::ApiRouter() {
	}

	ApiRouter::~ApiRouter() {

	}

	void ApiRouter::handleSocketRequest(const string& aMessage, const websocketpp::http::parser::request& aRequest, WebSocketPtr& aSocket, bool aIsSecure) noexcept {

		dcdebug("Received socket request: %s\n", aMessage.c_str());
		bool authenticated = aSocket->getSession() != nullptr;

		json responseJsonData, errorJson;
		websocketpp::http::status_code::value code;
		int callbackId = -1;

		try {
			const auto requestJson = json::parse(aMessage);

			auto cb = requestJson.find("callback_id");
			if (cb != requestJson.end()) {
				callbackId = cb.value();
			}

			const string path = requestJson.at("path");
			ApiRequest apiRequest(aRequest.get_uri() + path, requestJson.at("method"), responseJsonData, errorJson);
			apiRequest.parseSocketRequestJson(requestJson);
			apiRequest.setSession(aSocket->getSession());

			apiRequest.validate();

			code = handleRequest(apiRequest, aIsSecure, aSocket, aSocket->getIp());
		} catch (const std::exception& e) {
			errorJson = { 
				{ "message", "Parsing failed: " + string(e.what()) }
			};

			code = websocketpp::http::status_code::bad_request;
		}

		// Send an error also if parsing failed (there's no callback id)
		if (callbackId > 0 || !errorJson.is_null()) {
			aSocket->sendApiResponse(responseJsonData, errorJson, code, callbackId);
		}
	}

	websocketpp::http::status_code::value ApiRouter::handleHttpRequest(const string& aRequestPath,
		const websocketpp::http::parser::request& aRequest, json& output_, json& error_,
		bool aIsSecure, const string& aIp, const SessionPtr& aSession) noexcept {

		auto& requestBody = aRequest.get_body();
		dcdebug("Received HTTP request: %s\n", aRequest.get_body().c_str());
		try {
			ApiRequest apiRequest(aRequestPath, aRequest.get_method(), output_, error_);
			apiRequest.validate();

			apiRequest.parseHttpRequestJson(requestBody);
			apiRequest.setSession(aSession);

			return handleRequest(apiRequest, aIsSecure, nullptr, aIp);
		} catch (const std::exception& e) {
			error_ = { 
				{ "message", "Parsing failed: " + string(e.what()) }
			};

			return websocketpp::http::status_code::bad_request;
		}

		return websocketpp::http::status_code::ok;
	}

	api_return ApiRouter::handleRequest(ApiRequest& aRequest, bool aIsSecure, const WebSocketPtr& aSocket, const string& aIp) noexcept {
		if (aRequest.getApiVersion() != API_VERSION) {
			aRequest.setResponseErrorStr("Unsupported API version");
			return websocketpp::http::status_code::precondition_failed;
		}

		int code;
		try {
			// Special case because we may not have the session yet
			if (aRequest.getApiModule() == "sessions" && !aRequest.getSession()) {
				return routeAuthRequest(aRequest, aIsSecure, aSocket, aIp);
			}

			// Require auth for all other modules
			if (!aRequest.getSession()) {
				aRequest.setResponseErrorStr("Not authorized");
				return websocketpp::http::status_code::unauthorized;
			}

			// Require using the same protocol that was used for logging in
			if ((aRequest.getSession()->getSessionType() == Session::TYPE_SECURE) != aIsSecure) {
				aRequest.setResponseErrorStr("Protocol mismatch");
				return websocketpp::http::status_code::not_acceptable;
			}

			aRequest.getSession()->updateActivity();

			code = aRequest.getSession()->handleRequest(aRequest);
		} catch (const ArgumentException& e) {
			aRequest.setResponseErrorJson(e.getErrorJson());
			code = CODE_UNPROCESSABLE_ENTITY;
		} catch (const RequestException& e) {
			aRequest.setResponseErrorStr(e.what());
			code = e.getCode();
		} catch (const std::exception& e) {
			aRequest.setResponseErrorStr(e.what());
			code = websocketpp::http::status_code::bad_request;
		}

		return static_cast<api_return>(code);
	}

	api_return ApiRouter::routeAuthRequest(ApiRequest& aRequest, bool aIsSecure, const WebSocketPtr& aSocket, const string& aIp) {
		if (aRequest.getParamAt(0) == "authorize" && aRequest.getMethod() == METHOD_POST) {
			return SessionApi::handleLogin(aRequest, aIsSecure, aSocket, aIp);
		} else if (aRequest.getParamAt(0) == "socket" && aRequest.getMethod() == METHOD_POST) {
			return SessionApi::handleSocketConnect(aRequest, aIsSecure, aSocket);
		}

		aRequest.setResponseErrorStr("Invalid command/method (not authenticated)");
		return websocketpp::http::status_code::bad_request;
	}
}