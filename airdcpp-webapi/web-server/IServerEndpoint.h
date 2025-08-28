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
	// Keep using websocketpp::connection_hdl in phase 1 to minimize churn
	using ConnectionHdl = websocketpp::connection_hdl;
	using context_ptr = std::shared_ptr<boost::asio::ssl::context>;

	class IServerEndpoint {
	public:
		virtual ~IServerEndpoint() = default;

		// Initialization
		virtual void init_asio(boost::asio::io_context* ios) = 0;

		// Timeouts & limits
		virtual void set_open_handshake_timeout(int millis) = 0;
		virtual void set_pong_timeout(int millis) = 0;
		virtual void set_max_http_body_size(std::size_t bytes) = 0;

		// TLS init (no handle is required by the current user code)
		virtual void set_tls_init_handler(std::function<context_ptr()> handler) = 0;

		// Logging
		virtual void configureLogging(std::ostream* access, std::ostream* error, bool enable) = 0;

		// Listening / lifecycle
		virtual void set_reuse_addr(bool enable) = 0;
		virtual void listen(const std::string& bindAddress, const std::string& port) = 0;
		virtual void listen(const boost::asio::ip::tcp& protocol, uint16_t port) = 0;
		virtual void start_accept() = 0;
		virtual bool is_listening() const noexcept = 0;
		virtual void stop_listening() = 0;

		// Event handlers
		virtual void set_message_handler(std::function<void(ConnectionHdl, const std::string&)> onMessage) = 0;
		virtual void set_http_handler(std::function<void(ConnectionHdl)> onHttp) = 0;
		virtual void set_close_handler(std::function<void(ConnectionHdl)> onClose) = 0;
		virtual void set_open_handler(std::function<void(ConnectionHdl)> onOpen) = 0;
		virtual void set_pong_timeout_handler(std::function<void(ConnectionHdl, const std::string&)> onPongTimeout) = 0;

		// Connection helpers used by higher layers
		virtual std::string get_remote_ip(ConnectionHdl hdl) = 0;
		// Generic HTTP request accessors (adapter-agnostic)
		virtual std::string get_header(ConnectionHdl hdl, const std::string& name) = 0;
		virtual std::string get_body(ConnectionHdl hdl) = 0;
		virtual std::string get_method(ConnectionHdl hdl) = 0;
		virtual std::string get_uri(ConnectionHdl hdl) = 0;
		virtual std::string get_resource(ConnectionHdl hdl) = 0;

		// HTTP response helpers
		virtual void http_append_header(ConnectionHdl hdl, const std::string& name, const std::string& value) = 0;
		virtual void http_set_body(ConnectionHdl hdl, const std::string& body) = 0;
		virtual void http_set_status(ConnectionHdl hdl, http_status status) = 0;
		virtual void http_defer_response(ConnectionHdl hdl) = 0;
		virtual void http_send_response(ConnectionHdl hdl) = 0;

		// WebSocket helpers
		virtual void ws_send_text(ConnectionHdl hdl, const std::string& text) = 0;
		virtual void ws_ping(ConnectionHdl hdl) = 0;
		virtual void ws_close(ConnectionHdl hdl, uint16_t code, const std::string& reason) = 0;
	};
}

#endif // DCPLUSPLUS_WEBSERVER_ISERVERENDPOINT_H
