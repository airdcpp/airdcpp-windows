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

#include "stdinc.h"

#include <web-server/HttpManager.h>
#include <web-server/ApiRouter.h>
#include <web-server/HttpUtil.h>

#include <web-server/ApiRequest.h>
#include <web-server/HttpRequest.h>
#include <web-server/Session.h>


namespace webserver {

	void HttpManager::start(const string& aWebResourcePath) noexcept {
		if (!aWebResourcePath.empty()) {
			fileServer.setResourcePath(aWebResourcePath);
		} else {
			fileServer.setResourcePath(AppUtil::getPath(AppUtil::PATH_RESOURCES) + "web-resources" + PATH_SEPARATOR);
		}
	}

	void HttpManager::stop() noexcept {
		fileServer.stop();
	}

	http::status HttpManager::handleApiRequest(const HttpRequest& aRequest, json& output_, json& error_, const ApiDeferredHandler& aDeferredHandler) noexcept
	{
		dcdebug("Received HTTP request: %s\n", aRequest.body.c_str());

		json bodyJson;
		if (!aRequest.body.empty()) {
			try {
				bodyJson = json::parse(aRequest.body);
			} catch (const std::exception& e) {
				error_ = ApiRequest::toResponseErrorStr("Failed to parse JSON: " + string(e.what()));
				return http::status::bad_request;
			}
		}

		try {
			ApiRequest apiRequest(aRequest.path, aRequest.method, std::move(bodyJson), aRequest.session, aDeferredHandler, output_, error_);
			RouterRequest routerRequest{ apiRequest, aRequest.secure, nullptr, aRequest.ip };
			const auto status = ApiRouter::handleRequest(routerRequest);
			return status;
		} catch (const std::invalid_argument& e) {
			error_ = ApiRequest::toResponseErrorStr(string(e.what()));
		}

		return http::status::bad_request;
	}

	bool HttpManager::getOptionalHttpSession(IServerEndpoint& ep, ConnectionHdl hdl, const string& aIp, SessionPtr& session_) {
		auto authToken = HttpUtil::parseAuthToken(ep.getHeader(hdl, "Authorization"), ep.getHeader(hdl, "X-Authorization"));
		if (!authToken.empty()) {
			try {
				session_ = wsm->getUserManager().parseHttpSession(authToken, aIp);
			} catch (const std::exception& e) {
				ep.httpSetBody(hdl, e.what());
				ep.httpSetStatus(hdl, http::status::unauthorized);
				return false;
			}
		}

		return true;
	}

	bool HttpManager::setHttpResponse(IServerEndpoint& ep, ConnectionHdl hdl, http::status aStatus, const string& aOutput) {
		/*if (aOutput.length() > MAX_HTTP_BODY_SIZE) {
			ep.http_set_status(hdl, http::status::internal_server_error);
			ep.http_set_body(hdl, "The response size is larger than " + Util::toString(MAX_HTTP_BODY_SIZE) + " bytes");
			return false;
		}*/

		ep.httpSetStatus(hdl, aStatus);
		try {
			ep.httpSetBody(hdl, aOutput);
		} catch (const std::exception&) {
			// Shouldn't really happen
			ep.httpSetStatus(hdl, http::status::internal_server_error);
			ep.httpSetBody(hdl, "Failed to set response body");
			return false;
		}

		return true;
	}

	void HttpManager::handleHttpApiRequest(const HttpRequest& aRequest, IServerEndpoint& ep, ConnectionHdl hdl) {
		wsm->onData(aRequest.path + ": " + aRequest.body, TransportType::TYPE_HTTP_API, Direction::INCOMING, aRequest.ip);

		// Don't capture aRequest in here (it can't be used for async actions)
		auto responseF = [this, &ep, hdl, ip = aRequest.ip](http::status aStatus, const json& aResponseJsonData, const json& aResponseErrorJson) {
			string data;
			const auto& responseJson = !aResponseErrorJson.is_null() ? aResponseErrorJson : aResponseJsonData;
			if (!responseJson.is_null()) {
				try {
					data = responseJson.dump();
				} catch (const std::exception& e) {
					// keep legacy logging semantics via WebServerManager helper for now
					ep.httpSetBody(hdl, "Failed to convert data to JSON: " + string(e.what()));
					ep.httpSetStatus(hdl, http::status::internal_server_error);
					return;
				}
			}

			wsm->onData(ep.getResource(hdl) + " (" + std::string(http::obsolete_reason(aStatus)) + "): " + data, TransportType::TYPE_HTTP_API, Direction::OUTGOING, ip);

			if (setHttpResponse(ep, hdl, aStatus, data)) {
				ep.httpAppendHeader(hdl, "Content-Type", "application/json");
			}
		};

		bool isDeferred = false;
		const auto deferredF = [&isDeferred, &responseF, &ep, hdl]() {
			ep.httpDeferResponse(hdl);
			isDeferred = true;

			return [&ep, hdl, cb = std::move(responseF)](http::status aStatus, const json& aResponseJsonData, const json& aResponseErrorJson) {
				cb(aStatus, aResponseJsonData, aResponseErrorJson);
				ep.httpSendResponse(hdl);
			};
		};

		json output, apiError;
		auto status = handleApiRequest(
			aRequest,
			output,
			apiError,
			deferredF
		);

		if (!isDeferred) {
			responseF(status, output, apiError);
		}
	}

	void HttpManager::handleHttpFileRequest(const HttpRequest& aRequest, IServerEndpoint& ep, ConnectionHdl hdl) {
		wsm->onData(aRequest.method + " " + aRequest.path, TransportType::TYPE_HTTP_FILE, Direction::INCOMING, aRequest.ip);

		StringPairList headers;
		std::string output;

		// Don't capture aRequest in here (it can't be used for async actions)
		auto responseF = [this, &ep, hdl, ip = aRequest.ip](http::status aStatus, const string& aOutput, const StringPairList& aHeaders = StringPairList()) {
			wsm->onData(
				"GET " + ep.getResource(hdl) + ": " + std::string(http::obsolete_reason(aStatus)) + " (" + Util::formatBytes(aOutput.length()) + ")",
				TransportType::TYPE_HTTP_FILE,
				Direction::OUTGOING,
				ip
			);

			auto responseOk = setHttpResponse(ep, hdl, aStatus, aOutput);
			if (responseOk && HttpUtil::isStatusOk(aStatus)) {
				// Don't set any incomplete/invalid headers in case of errors...
				for (const auto& [name, value] : aHeaders) {
					ep.httpAppendHeader(hdl, name, value);
				}
			}

		};

		bool isDeferred = false;
		const auto deferredF = [&isDeferred, &responseF, &ep, hdl]() {
			ep.httpDeferResponse(hdl);
			isDeferred = true;

			return [cb = std::move(responseF), &ep, hdl](http::status aStatus, const string& aOutput, const StringPairList& aHeaders) {
				cb(aStatus, aOutput, aHeaders);
				ep.httpSendResponse(hdl);
			};
		};

		auto status = fileServer.handleRequest(aRequest, output, headers, deferredF);
		if (!isDeferred) {
			responseF(status, output, headers);
		}
	}

	void HttpManager::handleHttpRequest(IServerEndpoint& ep, bool aIsSecure, ConnectionHdl hdl) {
		// Blocking HTTP Handler
		auto ip = ep.getRemoteIp(hdl);

		// Beast handles connection lifetimes itself; don't add legacy Connection: close header
		// We also have public resources (such as UI resources and auth endpoints) 
		// so session isn't required at this point
		SessionPtr session = nullptr;
		if (!getOptionalHttpSession(ep, hdl, ip, session)) {
			return;
		}

		HttpRequest request{ session, ip, ep.getResource(hdl), ep.getMethod(hdl), ep.getBody(hdl),
			[&ep, hdl](const std::string& name) { return ep.getHeader(hdl, name); }, aIsSecure };
		if (request.path.length() >= 4 && request.path.compare(0, 4, "/api") == 0) {
			handleHttpApiRequest(request, ep, hdl);
		} else {
			handleHttpFileRequest(request, ep, hdl);
		}
	}
}