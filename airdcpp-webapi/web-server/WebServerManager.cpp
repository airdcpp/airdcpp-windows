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
#include <api/ApiSettingItem.h>
#include <web-server/WebServerSettings.h>
#include <web-server/WebServerManager.h>

#include <airdcpp/typedefs.h>

#include <airdcpp/AirUtil.h>
#include <airdcpp/CryptoManager.h>
#include <airdcpp/SettingsManager.h>
#include <airdcpp/SimpleXML.h>
#include <airdcpp/TimerManager.h>

#define CONFIG_NAME "WebServer.xml"
#define CONFIG_DIR Util::PATH_USER_CONFIG

#define AUTHENTICATION_TIMEOUT 60 // seconds

#define HANDSHAKE_TIMEOUT 0 // disabled, affects HTTP downloads

namespace webserver {
	vector<ServerSettingItem> WebServerSettings::settings = {
		{ "web_plain_port", "Port", 5600 },
		{ "web_plain_bind_address", "Bind address", "" },

		{ "web_tls_port", "Port", 5601 },
		{ "web_tls_bind_address", "Bind address", "" },

		{ "web_tls_certificate_path", "Certificate path", "", ApiSettingItem::TYPE_FILE_PATH },
		{ "web_tls_certificate_key_path", "Certificate key path", "", ApiSettingItem::TYPE_FILE_PATH },

		{ "web_server_threads", "Server threads", 4 },

		{ "default_idle_timeout", "Default session inactivity timeout", 20, ApiSettingItem::TYPE_GENERAL, { ResourceManager::Strings::MINUTES_LOWER, false } },
		{ "ping_interval", "Socket ping interval", 30, ApiSettingItem::TYPE_GENERAL, { ResourceManager::Strings::SECONDS_LOWER, false } },
		{ "ping_timeout", "Socket ping timeout", 10, ApiSettingItem::TYPE_GENERAL, { ResourceManager::Strings::SECONDS_LOWER, false } },
	};

	using namespace dcpp;
	WebServerManager::WebServerManager() : has_io_service(false), 
		ios(WEBCFG(SERVER_THREADS).num()), 
		plainServerConfig(WEBCFG(PLAIN_PORT), WEBCFG(PLAIN_BIND)),
		tlsServerConfig(WEBCFG(TLS_PORT), WEBCFG(TLS_BIND)) {

		fileServer.setResourcePath(Util::getPath(Util::PATH_RESOURCES) + "web-resources" + PATH_SEPARATOR);

		userManager = unique_ptr<WebUserManager>(new WebUserManager(this));
		ios.stop(); //Prevent io service from running until we load
	}

	WebServerManager::~WebServerManager() {
		// Let it remove the listener
		userManager.reset();
	}

	string WebServerManager::getConfigPath() const noexcept {
		return Util::getPath(CONFIG_DIR) + CONFIG_NAME;
	}

	bool WebServerManager::isRunning() const noexcept {
		return !ios.stopped();
	}

#if defined _MSC_VER && defined _DEBUG
	class DebugOutputStream : public std::ostream {
	public:
		DebugOutputStream() : std::ostream(&buf) {}
	private:
		class dbgview_buffer : public std::stringbuf {
		public:
			~dbgview_buffer() {
				sync(); // can be avoided
			}

			int sync() {
				OutputDebugString(Text::toT(str()).c_str());
				str("");
				return 0;
			}
		};

		dbgview_buffer buf;
	};

	DebugOutputStream debugStreamPlain;
	DebugOutputStream debugStreamTls;
#else
#define debugStreamPlain std::cout
#define debugStreamTls std::cout
#endif

	template<class T>
	void setEndpointLogSettings(T& aEndpoint, std::ostream& aStream) {
		// Access
		aEndpoint.set_access_channels(websocketpp::log::alevel::all);
		aEndpoint.clear_access_channels(websocketpp::log::alevel::frame_payload | websocketpp::log::alevel::frame_header | websocketpp::log::alevel::control);
		aEndpoint.get_alog().set_ostream(&aStream);

		// Errors
		aEndpoint.set_error_channels(websocketpp::log::elevel::all);
		aEndpoint.get_elog().set_ostream(&aStream);
	}

	template<class T>
	void setEndpointHandlers(T& aEndpoint, bool aIsSecure, WebServerManager* aServer) {
		aEndpoint.set_http_handler(
			std::bind(&WebServerManager::on_http<T>, aServer, &aEndpoint, _1, aIsSecure));
		aEndpoint.set_message_handler(
			std::bind(&WebServerManager::on_message<T>, aServer, &aEndpoint, _1, _2, aIsSecure));

		aEndpoint.set_close_handler(std::bind(&WebServerManager::on_close_socket, aServer, _1));
		aEndpoint.set_open_handler(std::bind(&WebServerManager::on_open_socket<T>, aServer, &aEndpoint, _1, aIsSecure));

		aEndpoint.set_open_handshake_timeout(HANDSHAKE_TIMEOUT);

		aEndpoint.set_pong_timeout(WEBCFG(PING_TIMEOUT).num() * 1000);
		aEndpoint.set_pong_timeout_handler(std::bind(&WebServerManager::onPongTimeout, aServer, _1, _2));

		// Workaround for https://github.com/zaphoyd/websocketpp/issues/549
		aEndpoint.set_listen_backlog(boost::asio::socket_base::max_connections);
	}

	bool WebServerManager::startup(const ErrorF& errorF, const string& aWebResourcePath, const CallBack& aShutdownF) {
		if (!aWebResourcePath.empty()) {
			fileServer.setResourcePath(aWebResourcePath);
		}

		shutdownF = aShutdownF;
		return start(errorF);
	}

	bool WebServerManager::start(const ErrorF& errorF) {
		if (!hasValidConfig()) {
			return false;
		}

		ios.reset();
		if (!has_io_service) {
			has_io_service = initialize(errorF);
		}

		return listen(errorF);
	}

	bool WebServerManager::initialize(const ErrorF& errorF) {
		SettingsManager::getInstance()->setDefault(SettingsManager::PM_MESSAGE_CACHE, 100);
		SettingsManager::getInstance()->setDefault(SettingsManager::HUB_MESSAGE_CACHE, 100);

		try {
			// initialize asio with our external io_service rather than an internal one
			endpoint_plain.init_asio(&ios);
			endpoint_tls.init_asio(&ios);

			//endpoint_plain.set_pong_handler(std::bind(&WebServerManager::onPongReceived, this, _1, _2));
		} catch (const std::exception& e) {
			if (errorF) {
				errorF(e.what());
			}

			return false;
		}

		// Handlers
		setEndpointHandlers(endpoint_plain, false, this);
		setEndpointHandlers(endpoint_tls, true, this);

		// TLS endpoint has an extra handler for the tls init
		endpoint_tls.set_tls_init_handler(std::bind(&WebServerManager::on_tls_init, this, _1));

		// Logging
		setEndpointLogSettings(endpoint_plain, debugStreamPlain);
		setEndpointLogSettings(endpoint_tls, debugStreamTls);

		return true;
	}

	template <typename EndpointType>
	bool listenEndpoint(EndpointType& aEndpoint, const ServerConfig& aConfig, const string& aProtocol, const WebServerManager::ErrorF& errorF) noexcept {
		if (!aConfig.hasValidConfig()) {
			return false;
		}

		aEndpoint.set_reuse_addr(true);
		try {
			const auto bindAddress = aConfig.bindAddress.str();
			if (!bindAddress.empty()) {
				aEndpoint.listen(bindAddress, Util::toString(aConfig.port.num()));
			} else {
				// IPv6 and IPv4-mapped IPv6 addresses are used by default (given that IPv6 is supported by the OS)
				auto v6Supported = !AirUtil::getLocalIp(true).empty();
				aEndpoint.listen(v6Supported ? boost::asio::ip::tcp::v6() : boost::asio::ip::tcp::v4(), aConfig.port.num());
			}

			aEndpoint.start_accept();
			return true;
		} catch (const std::exception& e) {
			auto message = boost::format("Failed to set up %1% server on port %2%: %3% (is the port in use by another application?)") % aProtocol % aConfig.port.num() % string(e.what());
			if (errorF) {
				errorF(message.str());
			}
		}

		return false;
	}

	bool WebServerManager::listen(const ErrorF& errorF) {
		bool hasServer = false;

		if (listenEndpoint(endpoint_plain, plainServerConfig, "HTTP", errorF)) {
			hasServer = true;
		}

		if (listenEndpoint(endpoint_tls, tlsServerConfig, "HTTPS", errorF)) {
			hasServer = true;
		}

		if (!hasServer) {
			return false;
		}

		// Start the ASIO io_service run loop running both endpoints
		for (int x = 0; x < WEBCFG(SERVER_THREADS).num(); ++x) {
			worker_threads.create_thread(boost::bind(&boost::asio::io_service::run, &ios));
		}

		socketTimer = addTimer([this] { pingTimer(); }, WEBCFG(PING_INTERVAL).num() * 1000);
		socketTimer->start(false);

		fire(WebServerManagerListener::Started());
		return true;
	}

	WebSocketPtr WebServerManager::getSocket(websocketpp::connection_hdl hdl) const noexcept {
		RLock l(cs);
		auto s = sockets.find(hdl);
		if (s != sockets.end()) {
			return s->second;
		}

		return nullptr;
	}

	// For debugging only
	void WebServerManager::onPongReceived(websocketpp::connection_hdl hdl, const string& aPayload) {
		auto socket = getSocket(hdl);
		if (!socket) {
			return;
		}

		socket->debugMessage("PONG succeed");
	}

	void WebServerManager::onData(const string& aData, TransportType aType, Direction aDirection, const string& aIP) noexcept {
		fire(WebServerManagerListener::Data(), aData, aType, aDirection, aIP);
	}

	void WebServerManager::onPongTimeout(websocketpp::connection_hdl hdl, const string& aPayload) {
		auto socket = getSocket(hdl);
		if (!socket) {
			return;
		}

		socket->debugMessage("PONG timed out");

		socket->close(websocketpp::close::status::internal_endpoint_error, "PONG timed out");
	}

	void WebServerManager::pingTimer() noexcept {
		vector<WebSocketPtr> inactiveSockets;
		auto tick = GET_TICK();

		{
			RLock l(cs);
			for (const auto& socket : sockets | map_values) {
				//socket->debugMessage("PING");
				socket->ping();

				// Disconnect sockets without a session after one minute
				if (!socket->getSession() && socket->getTimeCreated() + AUTHENTICATION_TIMEOUT * 1000ULL < tick) {
					inactiveSockets.push_back(socket);
				}
			}
		}

		for (const auto& s : inactiveSockets) {
			s->close(websocketpp::close::status::policy_violation, "Authentication timeout");
		}
	}

	context_ptr WebServerManager::on_tls_init(websocketpp::connection_hdl hdl) {
		//std::cout << "on_tls_init called with hdl: " << hdl.lock().get() << std::endl;
		context_ptr ctx(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv12));

		try {
			ctx->set_options(boost::asio::ssl::context::default_workarounds |
				boost::asio::ssl::context::no_sslv2 |
				boost::asio::ssl::context::no_sslv3 |
				boost::asio::ssl::context::single_dh_use);

			const auto customCert = WEBCFG(TLS_CERT_PATH).str();
			const auto customKey = WEBCFG(TLS_CERT_KEY_PATH).str();

			bool useCustom = !customCert.empty() && !customKey.empty();

			ctx->use_certificate_file(useCustom ? customCert : SETTING(TLS_CERTIFICATE_FILE), boost::asio::ssl::context::pem);
			ctx->use_private_key_file(useCustom ? customKey : SETTING(TLS_PRIVATE_KEY_FILE), boost::asio::ssl::context::pem);

			CryptoManager::setContextOptions(ctx->native_handle(), true);
		} catch (std::exception& e) {
			dcdebug("TLS init failed: %s", e.what());
		}

		return ctx;
	}

	void WebServerManager::disconnectSockets(const string& aMessage) noexcept {
		RLock l(cs);
		for (const auto& socket : sockets | map_values) {
			socket->close(websocketpp::close::status::going_away, aMessage);
		}
	}

	void WebServerManager::stop() {
		if(socketTimer)
			socketTimer->stop(true);
		fire(WebServerManagerListener::Stopping());

		if(endpoint_plain.is_listening())
			endpoint_plain.stop_listening();
		if(endpoint_tls.is_listening())
			endpoint_tls.stop_listening();

		disconnectSockets("Shutting down");

		bool hasSockets = false;

		for (;;) {
			{
				RLock l(cs);
				hasSockets = !sockets.empty();
			}

			if (hasSockets) {
				Thread::sleep(50);
			} else {
				break;
			}
		}

		ios.stop();

		worker_threads.join_all();

		fire(WebServerManagerListener::Stopped());
	}

	void WebServerManager::logout(LocalSessionId aSessionId) noexcept {
		vector<WebSocketPtr> sessionSockets;

		{
			RLock l(cs);
			boost::algorithm::copy_if(sockets | map_values, back_inserter(sessionSockets),
				[&](const WebSocketPtr& aSocket) {
					return aSocket->getSession() && aSocket->getSession()->getId() == aSessionId;
				}
			);
		}

		for (const auto& socket : sessionSockets) {
			userManager->logout(socket->getSession());
			socket->setSession(nullptr);
		}
	}

	WebSocketPtr WebServerManager::getSocket(LocalSessionId aSessionToken) noexcept {
		RLock l(cs);
		auto i = find_if(sockets | map_values, [&](const WebSocketPtr& s) {
			return s->getSession() && s->getSession()->getId() == aSessionToken;
		});

		return i.base() == sockets.end() ? nullptr : *i;
	}

	TimerPtr WebServerManager::addTimer(CallBack&& aCallBack, time_t aIntervalMillis, const Timer::CallbackWrapper& aCallbackWrapper) noexcept {
		return make_shared<Timer>(move(aCallBack), ios, aIntervalMillis, aCallbackWrapper);
	}

	void WebServerManager::addAsyncTask(CallBack&& aCallBack) noexcept {
		ios.post(aCallBack);
	}

	void WebServerManager::on_close_socket(websocketpp::connection_hdl hdl) {
		WebSocketPtr socket = nullptr;

		{
			WLock l(cs);
			auto s = sockets.find(hdl);
			dcassert(s != sockets.end());
			if (s == sockets.end()) {
				return;
			}

			socket = s->second;
			sockets.erase(s);
		}

		dcdebug("Close socket: %s\n", socket->getSession() ? socket->getSession()->getAuthToken().c_str() : "(no session)");
		if (socket->getSession()) {
			socket->getSession()->onSocketDisconnected();
		}
	}

	bool WebServerManager::hasValidConfig() const noexcept {
		return (plainServerConfig.hasValidConfig() || tlsServerConfig.hasValidConfig()) && userManager->hasUsers();
	}

	bool WebServerManager::load(const ErrorF& aErrorF) noexcept {
		try {
			SimpleXML xml;
			SettingsManager::loadSettingFile(xml, CONFIG_DIR, CONFIG_NAME, true);
			if (xml.findChild("WebServer")) {
				xml.stepIn();

				if (xml.findChild("Config")) {
					xml.stepIn();
					loadServer(xml, "Server", plainServerConfig, false);
					loadServer(xml, "TLSServer", tlsServerConfig, true);

					if (xml.findChild("Threads")) {
						xml.stepIn();
						WEBCFG(SERVER_THREADS).setCurValue(max(Util::toInt(xml.getData()), 1));
						xml.stepOut();
					}
					xml.resetCurrentChild();

					xml.stepOut();
				}

				fire(WebServerManagerListener::LoadSettings(), xml);

				xml.stepOut();
			}
		} catch (const Exception& e) {
			if (aErrorF) {
				aErrorF(STRING_F(LOAD_FAILED_X, CONFIG_NAME % e.getError()));
			}
		}

		return hasValidConfig();
	}

	void WebServerManager::loadServer(SimpleXML& aXml, const string& aTagName, ServerConfig& config_, bool aTls) noexcept {
		if (aXml.findChild(aTagName)) {
			config_.port.setCurValue(aXml.getIntChildAttrib("Port"));
			config_.bindAddress.setCurValue(aXml.getChildAttrib("BindAddress"));

			if (aTls) {
				WEBCFG(TLS_CERT_PATH).setCurValue(aXml.getChildAttrib("Certificate"));
				WEBCFG(TLS_CERT_KEY_PATH).setCurValue(aXml.getChildAttrib("CertificateKey"));
			}
		}

		aXml.resetCurrentChild();
	}

	bool WebServerManager::save(const ErrorF& aCustomErrorF) noexcept {
		SimpleXML xml;

		xml.addTag("WebServer");
		xml.stepIn();

		{
			xml.addTag("Config");
			xml.stepIn();

			plainServerConfig.save(xml, "Server");

			tlsServerConfig.save(xml, "TLSServer");
			xml.addChildAttrib("Certificate", WEBCFG(TLS_CERT_PATH).str());
			xml.addChildAttrib("CertificateKey", WEBCFG(TLS_CERT_KEY_PATH).str());

			if (!WEBCFG(SERVER_THREADS).isDefault()) {
				xml.addTag("Threads");
				xml.stepIn();

				xml.setData(Util::toString(WEBCFG(SERVER_THREADS).num()));

				xml.stepOut();
			}

			xml.stepOut();
		}

		fire(WebServerManagerListener::SaveSettings(), xml);

		xml.stepOut();

		auto errorF = aCustomErrorF;
		if (!errorF) {
			// Avoid crashes if the file is saved when core is not loaded
			errorF = [](const string&) {};
		}

		return SettingsManager::saveSettingFile(xml, CONFIG_DIR, CONFIG_NAME, errorF);
	}

	bool ServerConfig::hasValidConfig() const noexcept {
		return port.num() > 0;
	}

	void ServerConfig::save(SimpleXML& xml_, const string& aTagName) noexcept {
		xml_.addTag(aTagName);
		xml_.addChildAttrib("Port", port.num());

		if (!bindAddress.str().empty()) {
			xml_.addChildAttrib("BindAddress", bindAddress.str());
		}
	}
}