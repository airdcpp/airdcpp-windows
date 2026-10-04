/*
* Abstraction for HTTP/WebSocket server endpoint to decouple from concrete libraries
*/
#ifndef DCPLUSPLUS_WEBSERVER_ISERVERENDPOINT_H
#define DCPLUSPLUS_WEBSERVER_ISERVERENDPOINT_H

#include "stdinc.h"

#include <functional>
#include <memory>
#include <string>

namespace webserver {
	using contextPtr = std::shared_ptr<boost::asio::ssl::context>;

	class IServerEndpoint {
	public:
		virtual ~IServerEndpoint() = default;

		// Initialization
		virtual void initAsio(boost::asio::io_context* ios) = 0;

		// Timeouts & limits
		virtual void setOpenHandshakeTimeout(int millis) = 0;
		virtual void setPongTimeout(int millis) = 0;
		virtual void setMaxHttpBodySize(std::size_t bytes) = 0;

		// TLS init (no handle is required by the current user code)
		virtual void setTlsInitHandler(std::function<contextPtr()> handler) = 0;

		// Logging
		virtual void configureLogging(std::ostream* access, std::ostream* error, bool enable) = 0;

		// Listening / lifecycle
		virtual void setReuseAddr(bool enable) = 0;
		virtual void listen(const std::string& bindAddress, const std::string& port) = 0;
		virtual void listen(const boost::asio::ip::tcp& protocol, uint16_t port) = 0;
		virtual void startAccept() = 0;
		virtual bool isListening() const noexcept = 0;
		virtual void stopListening() = 0;

		// Event handlers
		virtual void setMessageHandler(std::function<void(ConnectionHdl, const std::string&)> onMessage) = 0;
		virtual void setHttpHandler(std::function<void(ConnectionHdl)> onHttp) = 0;
		virtual void setCloseHandler(std::function<void(ConnectionHdl)> onClose) = 0;
		virtual void setOpenHandler(std::function<void(ConnectionHdl)> onOpen) = 0;
		virtual void setPongTimeoutHandler(std::function<void(ConnectionHdl, const std::string&)> onPongTimeout) = 0;

		// Connection helpers used by higher layers
		virtual std::string getRemoteIp(ConnectionHdl hdl) = 0;
		// Generic HTTP request accessors (adapter-agnostic)
		virtual std::string getHeader(ConnectionHdl hdl, const std::string& name) = 0;
		virtual std::string getBody(ConnectionHdl hdl) = 0;
		virtual std::string getMethod(ConnectionHdl hdl) = 0;
		virtual std::string getUri(ConnectionHdl hdl) = 0;
		virtual std::string getResource(ConnectionHdl hdl) = 0;

		// HTTP response helpers
		virtual void httpAppendHeader(ConnectionHdl hdl, const std::string& name, const std::string& value) = 0;
		virtual void httpSetBody(ConnectionHdl hdl, const std::string& body) = 0;
		virtual void httpSetStatus(ConnectionHdl hdl, http::status status) = 0;
		virtual void httpDeferResponse(ConnectionHdl hdl) = 0;
		virtual void httpSendResponse(ConnectionHdl hdl) = 0;

		// WebSocket helpers
		virtual void wsSendText(ConnectionHdl hdl, const std::string& text) = 0;
		virtual void wsPing(ConnectionHdl hdl) = 0;
		virtual void wsClose(ConnectionHdl hdl, uint16_t code, const std::string& reason) = 0;
	};
}

#endif // DCPLUSPLUS_WEBSERVER_ISERVERENDPOINT_H
