/*
* Copyright (C) 2011-2015 AirDC++ Project
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

#ifndef DCPLUSPLUS_WEBSERVER_STDINC_H
#define DCPLUSPLUS_WEBSERVER_STDINC_H

#include <airdcpp/stdinc.h>

#include <boost/asio/ssl/context.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

#include "json.h"

namespace webserver {
	using json = nlohmann::json;

	namespace http = boost::beast::http;
	namespace websocket = boost::beast::websocket;

	// Generic connection handle used by adapters (points to underlying session object)
	using ConnectionHdl = std::weak_ptr<void>;

	// API aliases
	using api_return = http::status;

	using HTTPFileCompletionF = std::function<void(api_return aStatus, const std::string& aOutput, const std::vector<std::pair<std::string, std::string>>& aHeaders)>;
	using ApiCompletionF = std::function<void(api_return aStatus, const json& aResponseJsonData, const json& aResponseErrorJson)>;
	using FileDeferredHandler = std::function<HTTPFileCompletionF()>;
	using ApiDeferredHandler = std::function<ApiCompletionF()>;

	using namespace dcpp;
}

#endif // !defined(DCPLUSPLUS_WEBSERVER_STDINC_H)
