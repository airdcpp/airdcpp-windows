/*
* Copyright (C) 2011-2017 AirDC++ Project
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

#include <api/ExtensionInfo.h>
#include <api/common/Serializer.h>

#include <web-server/JsonUtil.h>
#include <web-server/Extension.h>
#include <web-server/ExtensionManager.h>
#include <web-server/Session.h>
#include <web-server/WebServerManager.h>

#include <airdcpp/File.h>


namespace webserver {
	const StringList ExtensionInfo::subscriptionList = {
		"extension_started",
		"extension_stopped"
	};

	ExtensionInfo::ExtensionInfo(ParentType* aParentModule, const ExtensionPtr& aExtension) : 
		SubApiModule(aParentModule, aExtension->getName(), subscriptionList),
		extension(aExtension) 
	{
		extension->addListener(this);

		METHOD_HANDLER(Access::ADMIN, METHOD_POST, (EXACT_PARAM("start")), ExtensionInfo::handleStartExtension);
		METHOD_HANDLER(Access::ADMIN, METHOD_POST, (EXACT_PARAM("stop")), ExtensionInfo::handleStopExtension);
	}

	void ExtensionInfo::init() noexcept {

	}

	string ExtensionInfo::getId() const noexcept {
		return extension->getName();
	}

	ExtensionInfo::~ExtensionInfo() {
		extension->removeListener(this);
	}

	api_return ExtensionInfo::handleStartExtension(ApiRequest& aRequest) {
		try {
			auto server = aRequest.getSession()->getServer();
			extension->start(server->getExtensionManager().getStartCommand(extension), server);
		} catch (const Exception& e) {
			aRequest.setResponseErrorStr(e.what());
			return websocketpp::http::status_code::internal_server_error;
		}

		return websocketpp::http::status_code::no_content;
	}

	api_return ExtensionInfo::handleStopExtension(ApiRequest& aRequest) {
		extension->stop();
		return websocketpp::http::status_code::no_content;
	}

	void ExtensionInfo::on(ExtensionListener::ExtensionStarted) noexcept {
		//maybeSend("extension_started", [&] {
		//	return serializeExtension(aExtension);
		//});
	}

	void ExtensionInfo::on(ExtensionListener::ExtensionStopped) noexcept {
		//maybeSend("extension_stopped", [&] {
		//	return serializeExtension(aExtension);
		//});
	}
}