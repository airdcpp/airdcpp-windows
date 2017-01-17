/*
* Copyright (C) 2011-2015 AirDC++ Project
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
#include <web-server/WebSocket.h>
#include <web-server/WebServerManager.h>

#include <api/ApiModule.h>

namespace webserver {
	ApiModule::ApiModule(Session* aSession) : session(aSession) {

	}

	ApiModule::~ApiModule() {

	}

	optional<ApiRequest::NamedParamMap> ApiModule::RequestHandler::matchParams(const ApiRequest::ParamList& aRequestParams) const noexcept {
		if (method == METHOD_FORWARD) {
			// The request must contain more params than the forwarder has
			// (there must be at least one parameter left for the next handler)
			if (aRequestParams.size() <= params.size()) {
				return boost::none;
			}
		} else if (aRequestParams.size() != params.size()) {
			return boost::none;
		}

		for (auto i = 0; i < static_cast<int>(params.size()); i++) {
			try {
				if (!boost::regex_search(aRequestParams[i], params[i].reg)) {
					return boost::none;
				}
			} catch (const std::runtime_error&) {
				return boost::none;
			}
		}

		ApiRequest::NamedParamMap paramMap;
		for (auto i = 0; i < static_cast<int>(params.size()); i++) {
			paramMap[params[i].id] = aRequestParams[i];
		}

		return paramMap;
	}

	api_return ApiModule::handleRequest(ApiRequest& aRequest) {
		bool hasParamNameMatch = false; // for better error reporting

		// Match parameters
		auto handler = find_if(requestHandlers.begin(), requestHandlers.end(), [&](const RequestHandler& aHandler) {
			// Regular matching
			auto namedParams = aHandler.matchParams(aRequest.getParameters());
			if (!namedParams) {
				return false;
			}

			if (aHandler.method == aRequest.getMethod() || aHandler.method == METHOD_FORWARD) {
				aRequest.setNamedParams(*namedParams);
				return true;
			}

			hasParamNameMatch = true;
			return false;
		});

		if (handler == requestHandlers.end()) {
			if (hasParamNameMatch) {
				aRequest.setResponseErrorStr("Method " + aRequest.getMethodStr() + " is not supported for this handler");
				return websocketpp::http::status_code::method_not_allowed;
			}

			aRequest.setResponseErrorStr("Supplied URL doesn't match any method handler in this API module");
			return websocketpp::http::status_code::bad_request;
		}

		// Check permission
		if (!session->getUser()->hasPermission(handler->access)) {
			aRequest.setResponseErrorStr("Permission denied");
			return websocketpp::http::status_code::forbidden;
		}

		return handler->f(aRequest);
	}

	TimerPtr ApiModule::getTimer(CallBack&& aTask, time_t aIntervalMillis) {
		return session->getServer()->addTimer(move(aTask), aIntervalMillis,
			std::bind(&ApiModule::asyncRunWrapper, std::placeholders::_1, session->getId())
		);
	}

	CallBack ApiModule::getAsyncWrapper(CallBack&& aTask) noexcept {
		auto sessionId = session->getId();
		return [aTask, sessionId] {
			return asyncRunWrapper(aTask, sessionId);
		};
	}

	void ApiModule::asyncRunWrapper(const CallBack& aTask, LocalSessionId aSessionId) {
		// Ensure that the session (and socket) won't be deleted
		auto s = WebServerManager::getInstance()->getUserManager().getSession(aSessionId);
		if (!s) {
			return;
		}

		aTask();
	}

	void ApiModule::addAsyncTask(CallBack&& aTask) {
		session->getServer()->addAsyncTask(getAsyncWrapper(move(aTask)));
	}


	SubscribableApiModule::SubscribableApiModule(Session* aSession, Access aSubscriptionAccess, const StringList* aSubscriptions) : ApiModule(aSession), subscriptionAccess(aSubscriptionAccess) {
		socket = WebServerManager::getInstance()->getSocket(aSession->getId());

		if (aSubscriptions) {
			for (const auto& s : *aSubscriptions) {
				subscriptions.emplace(s, false);
			}
		}

		aSession->addListener(this);

		METHOD_HANDLER(aSubscriptionAccess, METHOD_POST, (EXACT_PARAM("listener"), STR_PARAM(LISTENER_PARAM_ID)), SubscribableApiModule::handleSubscribe);
		METHOD_HANDLER(aSubscriptionAccess, METHOD_DELETE, (EXACT_PARAM("listener"), STR_PARAM(LISTENER_PARAM_ID)), SubscribableApiModule::handleUnsubscribe);
	}

	SubscribableApiModule::~SubscribableApiModule() {
		session->removeListener(this);
		socket = nullptr;
	}

	void SubscribableApiModule::on(SessionListener::SocketConnected, const WebSocketPtr& aSocket) noexcept {
		socket = aSocket;
	}

	void SubscribableApiModule::on(SessionListener::SocketDisconnected) noexcept {
		// Disable all subscriptions
		for (auto& s : subscriptions) {
			s.second = false;
		}

		socket = nullptr;
	}

	api_return SubscribableApiModule::handleSubscribe(ApiRequest& aRequest) {
		if (!socket) {
			aRequest.setResponseErrorStr("Socket required");
			return websocketpp::http::status_code::precondition_required;
		}

		const auto& subscription = aRequest.getStringParam(LISTENER_PARAM_ID);
		if (!subscriptionExists(subscription)) {
			aRequest.setResponseErrorStr("No such subscription: " + subscription);
			return websocketpp::http::status_code::not_found;
		}

		setSubscriptionState(subscription, true);
		return websocketpp::http::status_code::no_content;
	}

	api_return SubscribableApiModule::handleUnsubscribe(ApiRequest& aRequest) {
		auto subscription = aRequest.getStringParam(LISTENER_PARAM_ID);
		if (subscriptionExists(subscription)) {
			setSubscriptionState(subscription, false);
			return websocketpp::http::status_code::no_content;
		}

		return websocketpp::http::status_code::not_found;
	}

	bool SubscribableApiModule::send(const json& aJson) {
		// Ensure that the socket won't be deleted while sending the message...
		auto s = socket;
		if (!s) {
			return false;
		}

		s->sendPlain(aJson);
		return true;
	}

	bool SubscribableApiModule::send(const string& aSubscription, const json& aData) {
		return send({
			{ "event", aSubscription },
			{ "data", aData },
		});
	}

	bool SubscribableApiModule::maybeSend(const string& aSubscription, JsonCallback aCallback) {
		if (!subscriptionActive(aSubscription)) {
			return false;
		}

		return send(aSubscription, aCallback());
	}
}