// Boost.Beast based implementation of IServerEndpoint (phase 2 adapter)
#ifndef DCPLUSPLUS_WEBSERVER_BEAST_SERVER_ADAPTER_H
#define DCPLUSPLUS_WEBSERVER_BEAST_SERVER_ADAPTER_H

#include "stdinc.h"
#include "IServerEndpoint.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>

#include <memory>
#include <optional>
#include <unordered_map>
#include <deque>

namespace webserver {
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace websocket = beast::websocket;
	using tcp = boost::asio::ip::tcp;

	class BeastServerAdapter : public IServerEndpoint, public std::enable_shared_from_this<BeastServerAdapter> {
	public:
		BeastServerAdapter() = default;
		~BeastServerAdapter() override { stop_listening(); }

		// IServerEndpoint
		void init_asio(boost::asio::io_context* ios) override { ios_ = ios; }
		void set_open_handshake_timeout(int) override {}
		void set_pong_timeout(int) override {}
		void set_max_http_body_size(std::size_t) override {}
		void set_tls_init_handler(std::function<context_ptr()>) override {}
		void configureLogging(std::ostream*, std::ostream*, bool) override {}

		void set_reuse_addr(bool enable) override { reuseAddr_ = enable; }
		void listen(const std::string& bindAddress, const std::string& port) override {
			ensureAcceptor();
			boost::system::error_code ec;
			auto addr = boost::asio::ip::make_address(bindAddress, ec);
			if (ec) addr = boost::asio::ip::address_v4::any();
			uint16_t prt = static_cast<uint16_t>(dcpp::Util::toInt(port));
			tcp::endpoint ep(addr, prt);
			openAndBind(ep);
		}
		void listen(const boost::asio::ip::tcp& protocol, uint16_t port) override {
			ensureAcceptor();
			tcp::endpoint ep(protocol, port);
			openAndBind(ep);
		}
		void start_accept() override {
			if (!acceptor_ || !acceptor_->is_open() || listening_) return;
			listening_ = true;
			do_accept();
		}
		bool is_listening() const noexcept override { return listening_; }
		void stop_listening() override {
			listening_ = false;
			if (acceptor_) {
				boost::system::error_code _;
				acceptor_->close(_);
			}
		}

		void set_message_handler(std::function<void(ConnectionHdl, const std::string&)> f) override { onMessage_ = std::move(f); }
		void set_http_handler(std::function<void(ConnectionHdl)> f) override { onHttp_ = std::move(f); }
		void set_close_handler(std::function<void(ConnectionHdl)> f) override { onClose_ = std::move(f); }
		void set_open_handler(std::function<void(ConnectionHdl)> f) override { onOpen_ = std::move(f); }
		void set_pong_timeout_handler(std::function<void(ConnectionHdl, const std::string&)> f) override { onPongTimeout_ = std::move(f); }

		std::string get_remote_ip(ConnectionHdl hdl) override {
			if (auto s = lockSession(hdl)) {
				return s->remoteIp;
			}
			return {};
		}
		std::string get_header(ConnectionHdl hdl, const std::string& name) override {
			if (auto s = lockHttp(hdl)) {
				auto it = s->req.find(name);
				return it == s->req.end() ? std::string() : std::string(it->value());
			}
			return {};
		}
		std::string get_body(ConnectionHdl hdl) override {
			if (auto s = lockHttp(hdl)) return std::string(s->req.body());
			return {};
		}
		std::string get_method(ConnectionHdl hdl) override {
			if (auto s = lockHttp(hdl)) return std::string(s->req.method_string());
			if (auto ws = lockWs(hdl)) return ws->methodStr; // usually GET
			return {};
		}
		std::string get_uri(ConnectionHdl hdl) override {
			if (auto s = lockHttp(hdl)) return std::string(s->req.target());
			if (auto ws = lockWs(hdl)) return ws->target; // captured during upgrade
			return {};
		}
		std::string get_resource(ConnectionHdl hdl) override {
			// std::string t;
			if (auto s = lockHttp(hdl)) return std::string(s->req.target());
			else if (auto ws = lockWs(hdl)) return ws->target;
			return "";
		}

		void http_append_header(ConnectionHdl hdl, const std::string& name, const std::string& value) override {
			if (auto s = lockHttp(hdl)) s->respHeaders.emplace_back(name, value);
		}
		void http_set_body(ConnectionHdl hdl, const std::string& body) override {
			if (auto s = lockHttp(hdl)) s->respBody = body;
		}
		void http_set_status(ConnectionHdl hdl, http::status status) override {
			if (auto s = lockHttp(hdl)) s->respStatus = status;
		}
		void http_defer_response(ConnectionHdl hdl) override {
			if (auto s = lockHttp(hdl)) s->mark_deferred();
		}
		void http_send_response(ConnectionHdl hdl) override {
			if (auto s = lockHttp(hdl)) s->send_and_release();
		}

		void ws_send_text(ConnectionHdl hdl, const std::string& text) override {
			if (auto s = lockWs(hdl)) {
				s->send_text(text);
			}
		}
		void ws_ping(ConnectionHdl hdl) override {
			if (auto s = lockWs(hdl)) {
				s->send_ping();
			}
		}
		void ws_close(ConnectionHdl hdl, uint16_t code, const std::string& reason) override {
			if (auto s = lockWs(hdl)) {
				s->send_close(code, reason);
			}
		}

	private:
		struct SessionBase {
			virtual ~SessionBase() = default;
			std::string remoteIp;
		};

		struct HttpSession : public SessionBase, public std::enable_shared_from_this<HttpSession> {
			HttpSession(BeastServerAdapter& owner, tcp::socket&& socket, const std::string& ip)
				: adapter(owner), stream(std::move(socket)) {
				remoteIp = ip;
			}

			void run() {
				auto self = shared_from_this();
				http::async_read(stream, buffer, req,
					[self](beast::error_code ec, std::size_t) {
					static_cast<HttpSession*>(self.get())->on_read(ec);
				});
			}

			void on_read(beast::error_code ec) {
				if (ec) return; // connection closed or error

				if (websocket::is_upgrade(req)) {
					// Upgrade to websocket
					auto wsSession = std::make_shared<WsSession>(adapter, stream.release_socket(), remoteIp);
					// create handle
					auto h = makeHandle(std::static_pointer_cast<SessionBase>(wsSession));
					wsSession->do_accept(std::move(req), h);
					return;
				}

				// Create handle for HTTP
				hdl = makeHandle(std::static_pointer_cast<SessionBase>(this->shared_from_this()));

				if (adapter.onHttp_) {
					adapter.onHttp_(hdl);
				}

				if (!deferred) {
					sendResponse();
				}
			}

			// keep the session alive while deferred
			void mark_deferred() {
				if (!deferred) {
					deferred = true;
					selfKeep = shared_from_this();
				}
			}

			void send_and_release() {
				// capture self during async write
				sendResponse();
				deferred = false;
				// release our manual keep-alive; async write holds its own ref
				selfKeep.reset();
			}

			void sendResponse() {
				// default status if not set
				auto beastStatus = static_cast<http::status>(static_cast<unsigned>(respStatus));
				respPtr = std::make_shared<http::response<http::string_body>>(beastStatus, req.version());
				respPtr->set(http::field::server, "airdcpp-beast");
				for (const auto& [k,v] : respHeaders) respPtr->set(k, v);
				respPtr->body() = respBody;
				respPtr->prepare_payload();

				auto self = shared_from_this();
				http::async_write(stream, *respPtr, [self](beast::error_code /*ec*/, std::size_t /*bytes*/){
					// Close connection after response
					auto* p = static_cast<HttpSession*>(self.get());
					p->respPtr.reset();
					p->do_close();
				});
			}

			void do_close() {
				beast::error_code ec;
				stream.socket().shutdown(tcp::socket::shutdown_send, ec);
			}

			static ConnectionHdl makeHandle(const std::shared_ptr<SessionBase>& s) {
				std::weak_ptr<void> w = std::static_pointer_cast<void>(s);
				return w;
			}

			BeastServerAdapter& adapter;
			beast::tcp_stream stream;
			beast::flat_buffer buffer;
			http::request<http::string_body> req;

			// response state
			http::status respStatus = http::status::ok;
			std::vector<std::pair<std::string,std::string>> respHeaders;
			std::string respBody;
			bool deferred = false;
			ConnectionHdl hdl; // for callbacks
			std::shared_ptr<http::response<http::string_body>> respPtr; // keep response alive during async_write
			std::shared_ptr<HttpSession> selfKeep; // keep session alive while deferred
		};

		struct WsSession : public SessionBase, public std::enable_shared_from_this<WsSession> {
			WsSession(BeastServerAdapter& owner, tcp::socket&& socket, const std::string& ip)
				: adapter(owner), ws(std::move(socket)) {
				remoteIp = ip;
			}

			void do_accept(http::request<http::string_body>&& req, ConnectionHdl handle) {
				// Persist the HTTP request for the async_accept lifetime
				upgradeReq_ = std::move(req);
				// Capture essential request info before upgrading
				target = std::string(upgradeReq_.target());
				methodStr = std::string(upgradeReq_.method_string());
				hdl = handle;
				// Accept the websocket handshake
				ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
				ws.async_accept(upgradeReq_, beast::bind_front_handler(&WsSession::on_accept, shared_from_this()));
			}

			void on_accept(beast::error_code ec) {
				// release the stored request
				upgradeReq_ = {};
				if (ec) return;
				if (adapter.onOpen_) adapter.onOpen_(hdl);
				read_loop();
			}

			void read_loop() {
				ws.async_read(buffer, beast::bind_front_handler(&WsSession::on_read, shared_from_this()));
			}

			void on_read(beast::error_code ec, std::size_t) {
				if (ec) {
					if (adapter.onClose_) adapter.onClose_(hdl);
					return;
				}
				if (adapter.onMessage_) {
					auto data = beast::buffers_to_string(buffer.cdata());
					adapter.onMessage_(hdl, data);
				}
				buffer.consume(buffer.size());
				read_loop();
			}

			// Thread-safe send helpers queued on the ws executor
			void send_text(const std::string& text) {
				auto self = shared_from_this();
				boost::asio::post(ws.get_executor(), [self, text] {
					self->outQueue.push_back(text);
					if (!self->writing) {
						self->start_write();
					}
				});
			}

			void send_ping() {
				auto self = shared_from_this();
				boost::asio::post(ws.get_executor(), [self] {
					self->ws.async_ping(websocket::ping_data{}, [self](beast::error_code /*ec*/){ /* ignore */ });
				});
			}

			void send_close(uint16_t code, const std::string& /*reason*/) {
				auto self = shared_from_this();
				boost::asio::post(ws.get_executor(), [self, code] {
					websocket::close_reason cr;
					cr.code = static_cast<websocket::close_code>(code);
					self->ws.async_close(cr, [self](beast::error_code /*ec*/){ /* ignore */ });
				});
			}

			void start_write() {
				if (outQueue.empty()) return;
				writing = true;
				ws.text(true);
				auto self = shared_from_this();
				ws.async_write(boost::asio::buffer(outQueue.front()), [self](beast::error_code ec, std::size_t) {
					if (ec) {
						if (self->adapter.onClose_) self->adapter.onClose_(self->hdl);
						return;
					}
					self->outQueue.pop_front();
					if (!self->outQueue.empty()) {
						self->start_write();
					} else {
						self->writing = false;
					}
				});
			}

			BeastServerAdapter& adapter;
			websocket::stream<tcp::socket> ws;
			beast::flat_buffer buffer;
			ConnectionHdl hdl;
			std::string target;
			std::string methodStr = "GET";
			std::deque<std::string> outQueue;
			bool writing = false;
			http::request<http::string_body> upgradeReq_;
		};

		// Helpers to lock specific session types
		std::shared_ptr<SessionBase> lockSession(ConnectionHdl hdl) const {
			auto p = hdl.lock();
			return std::static_pointer_cast<SessionBase>(p);
		}
		std::shared_ptr<HttpSession> lockHttp(ConnectionHdl hdl) const {
			auto p = hdl.lock();
			return std::dynamic_pointer_cast<HttpSession>(std::static_pointer_cast<SessionBase>(p));
		}
		std::shared_ptr<WsSession> lockWs(ConnectionHdl hdl) const {
			auto p = hdl.lock();
			return std::dynamic_pointer_cast<WsSession>(std::static_pointer_cast<SessionBase>(p));
		}

		void ensureAcceptor() {
			if (!ios_) return;
			if (!acceptor_) acceptor_ = std::make_unique<tcp::acceptor>(*ios_);
		}

		void openAndBind(const tcp::endpoint& ep) {
			if (!acceptor_) return;
			boost::system::error_code ec;
			acceptor_->open(ep.protocol(), ec);
			if (reuseAddr_) acceptor_->set_option(boost::asio::socket_base::reuse_address(true), ec);
			acceptor_->bind(ep, ec);
			acceptor_->listen(boost::asio::socket_base::max_listen_connections, ec);
		}

		void do_accept() {
			acceptor_->async_accept(
				boost::asio::make_strand(*ios_),
				[this](beast::error_code ec, tcp::socket socket) {
					if (!listening_) return;
					if (!ec) {
						auto ip = socket.remote_endpoint(ec).address().to_string();
						// Start HTTP session
						auto httpSess = std::make_shared<HttpSession>(*this, std::move(socket), ip);
						httpSess->run();
					}
					// accept next
					if (listening_) do_accept();
				}
			);
		}

		// members
		boost::asio::io_context* ios_ = nullptr;
		std::unique_ptr<tcp::acceptor> acceptor_;
		bool listening_ = false;
		bool reuseAddr_ = false;

		std::function<void(ConnectionHdl, const std::string&)> onMessage_;
		std::function<void(ConnectionHdl)> onHttp_;
		std::function<void(ConnectionHdl)> onClose_;
		std::function<void(ConnectionHdl)> onOpen_;
		std::function<void(ConnectionHdl, const std::string&)> onPongTimeout_;
	};
}

#endif // DCPLUSPLUS_WEBSERVER_BEAST_SERVER_ADAPTER_H
