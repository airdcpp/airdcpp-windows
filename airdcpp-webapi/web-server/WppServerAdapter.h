/* WebSocket++ based implementation of IServerEndpoint (phase 1 adapter) */
#ifndef DCPLUSPLUS_WEBSERVER_WPP_SERVER_ADAPTER_H
#define DCPLUSPLUS_WEBSERVER_WPP_SERVER_ADAPTER_H

#include "stdinc.h"
#include "IServerEndpoint.h"

#include <websocketpp/logger/levels.hpp>

namespace webserver {
	using server_plain = websocketpp::server<websocketpp::config::asio>;
	using server_tls = websocketpp::server<websocketpp::config::asio_tls>;

	template <typename ServerT>
	class WppServerAdapter final : public IServerEndpoint {
	public:
		WppServerAdapter() = default;
		~WppServerAdapter() override = default;

		ServerT& underlying() noexcept { return endpoint; }

		void init_asio(boost::asio::io_context* ios) override {
			endpoint.init_asio(ios);
		}

		void set_open_handshake_timeout(int millis) override {
			endpoint.set_open_handshake_timeout(millis);
		}
		void set_pong_timeout(int millis) override {
			endpoint.set_pong_timeout(millis);
		}
		void set_max_http_body_size(std::size_t bytes) override {
			endpoint.set_max_http_body_size(bytes);
		}

		void set_tls_init_handler(std::function<context_ptr()> handler) override {
			// Only meaningful for TLS server; for plain do nothing
			setTlsInit(handler);
		}

		void configureLogging(std::ostream* access, std::ostream* error, bool enable) override {
			if (enable) {
				endpoint.set_access_channels(websocketpp::log::alevel::all);
				endpoint.clear_access_channels(websocketpp::log::alevel::frame_payload | websocketpp::log::alevel::frame_header | websocketpp::log::alevel::control);
				endpoint.get_alog().set_ostream(access);

				endpoint.set_error_channels(websocketpp::log::elevel::all);
				endpoint.get_elog().set_ostream(error);
			} else {
				endpoint.clear_access_channels(websocketpp::log::alevel::all);
				endpoint.clear_error_channels(websocketpp::log::elevel::all);
			}
		}

		void set_reuse_addr(bool enable) override { endpoint.set_reuse_addr(enable); }
		void listen(const std::string& bindAddress, const std::string& port) override { endpoint.listen(bindAddress, port); }
		void listen(const boost::asio::ip::tcp& protocol, uint16_t port) override { endpoint.listen(protocol, port); }
		void start_accept() override { endpoint.start_accept(); }
		bool is_listening() const noexcept override { return endpoint.is_listening(); }
		void stop_listening() override { endpoint.stop_listening(); }

		void set_message_handler(std::function<void(ConnectionHdl, const std::string&)> onMessage) override {
			endpoint.set_message_handler([onMessage](ConnectionHdl hdl, typename ServerT::message_ptr msg) {
				onMessage(hdl, msg->get_payload());
			});
		}
		void set_http_handler(std::function<void(ConnectionHdl)> onHttp) override {
			endpoint.set_http_handler([onHttp](ConnectionHdl hdl) { onHttp(hdl); });
		}
		void set_close_handler(std::function<void(ConnectionHdl)> onClose) override {
			endpoint.set_close_handler([onClose](ConnectionHdl hdl) { onClose(hdl); });
		}
		void set_open_handler(std::function<void(ConnectionHdl)> onOpen) override {
			endpoint.set_open_handler([onOpen](ConnectionHdl hdl) { onOpen(hdl); });
		}
		void set_pong_timeout_handler(std::function<void(ConnectionHdl, const std::string&)> onPongTimeout) override {
			endpoint.set_pong_timeout_handler([onPongTimeout](ConnectionHdl hdl, const std::string& payload) { onPongTimeout(hdl, payload); });
		}

		std::string get_remote_ip(ConnectionHdl hdl) override {
			try {
				auto con = endpoint.get_con_from_hdl(hdl);
				return con->get_raw_socket().remote_endpoint().address().to_string();
			} catch (...) { return std::string(); }
		}
		// Request helpers
		std::string get_header(ConnectionHdl hdl, const std::string& name) override {
			return endpoint.get_con_from_hdl(hdl)->get_request().get_header(name);
		}
		std::string get_body(ConnectionHdl hdl) override {
			return endpoint.get_con_from_hdl(hdl)->get_request_body();
		}
		std::string get_method(ConnectionHdl hdl) override {
			return endpoint.get_con_from_hdl(hdl)->get_request().get_method();
		}
		std::string get_uri(ConnectionHdl hdl) override {
			return endpoint.get_con_from_hdl(hdl)->get_request().get_uri();
		}
		std::string get_resource(ConnectionHdl hdl) override {
			return endpoint.get_con_from_hdl(hdl)->get_resource();
		}

		void http_append_header(ConnectionHdl hdl, const std::string& name, const std::string& value) override {
			endpoint.get_con_from_hdl(hdl)->append_header(name, value);
		}
		void http_set_body(ConnectionHdl hdl, const std::string& body) override {
			endpoint.get_con_from_hdl(hdl)->set_body(body);
		}
		void http_set_status(ConnectionHdl hdl, http_status status) override {
			endpoint.get_con_from_hdl(hdl)->set_status(status);
		}
		void http_defer_response(ConnectionHdl hdl) override {
			endpoint.get_con_from_hdl(hdl)->defer_http_response();
		}
		void http_send_response(ConnectionHdl hdl) override {
			endpoint.get_con_from_hdl(hdl)->send_http_response();
		}

		void ws_send_text(ConnectionHdl hdl, const std::string& text) override {
			endpoint.send(hdl, text, websocketpp::frame::opcode::text);
		}
		void ws_ping(ConnectionHdl hdl) override {
			endpoint.ping(hdl, "");
		}
		void ws_close(ConnectionHdl hdl, uint16_t code, const std::string& reason) override {
			endpoint.close(hdl, static_cast<websocketpp::close::status::value>(code), reason);
		}

	private:
		// Plain server default: no-op
		void setTlsInit(std::function<context_ptr()>) {}

		ServerT endpoint;
	};

	// TLS specialization installs init handler
	template<>
	inline void WppServerAdapter<server_tls>::setTlsInit(std::function<context_ptr()> handler) {
		if (handler) {
			endpoint.set_tls_init_handler([handler](ConnectionHdl) { return handler(); });
		}
	}

}

#endif // DCPLUSPLUS_WEBSERVER_WPP_SERVER_ADAPTER_H
