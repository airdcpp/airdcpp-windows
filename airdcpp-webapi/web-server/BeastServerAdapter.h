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
#include <mutex>

namespace webserver {
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace websocket = beast::websocket;
	using tcp = boost::asio::ip::tcp;

	class BeastServerAdapter : public IServerEndpoint, public std::enable_shared_from_this<BeastServerAdapter> {
	public:
		BeastServerAdapter() = default;
		~BeastServerAdapter() override { stopListening(); }

		// IServerEndpoint
		void initAsio(boost::asio::io_context* ios) override { ios_ = ios; }
		void setOpenHandshakeTimeout(int) override {}
		void setPongTimeout(int) override {}
		void setMaxHttpBodySize(std::size_t) override {}
		void setTlsInitHandler(std::function<contextPtr()>) override {}
		void configureLogging(std::ostream* access, std::ostream* error, bool enable) override {
			std::lock_guard<std::mutex> lk(logMutex_);
			accessLog_ = access;
			errorLog_ = error;
			loggingEnabled_ = enable && (access || error);
		}

		void setReuseAddr(bool enable) override { reuseAddr_ = enable; }
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
		void startAccept() override {
			if (!acceptor_ || !acceptor_->is_open() || listening_) return;
			listening_ = true;
			doAccept();
		}
		bool isListening() const noexcept override { return listening_; }
		void stopListening() override {
			listening_ = false;
			if (acceptor_) {
				boost::system::error_code _;
				acceptor_->close(_);
			}
		}

		void setMessageHandler(std::function<void(ConnectionHdl, const std::string&)> f) override { onMessage_ = std::move(f); }
		void setHttpHandler(std::function<void(ConnectionHdl)> f) override { onHttp_ = std::move(f); }
		void setCloseHandler(std::function<void(ConnectionHdl)> f) override { onClose_ = std::move(f); }
		void setOpenHandler(std::function<void(ConnectionHdl)> f) override { onOpen_ = std::move(f); }
		void setPongTimeoutHandler(std::function<void(ConnectionHdl, const std::string&)> f) override { onPongTimeout_ = std::move(f); }

		std::string getRemoteIp(ConnectionHdl hdl) override {
			if (auto s = lockSession(hdl)) {
				return s->remoteIp;
			}
			return {};
		}
		std::string getHeader(ConnectionHdl hdl, const std::string& name) override {
			if (auto s = lockHttp(hdl)) {
				auto it = s->req.find(name);
				return it == s->req.end() ? std::string() : std::string(it->value());
			}
			return {};
		}
		std::string getBody(ConnectionHdl hdl) override {
			if (auto s = lockHttp(hdl)) return std::string(s->req.body());
			return {};
		}
		std::string getMethod(ConnectionHdl hdl) override {
			if (auto s = lockHttp(hdl)) return std::string(s->req.method_string());
			if (auto ws = lockWs(hdl)) return ws->methodStr; // usually GET
			return {};
		}
		std::string getUri(ConnectionHdl hdl) override {
			if (auto s = lockHttp(hdl)) return std::string(s->req.target());
			if (auto ws = lockWs(hdl)) return ws->target; // captured during upgrade
			return {};
		}
		std::string getResource(ConnectionHdl hdl) override {
			if (auto s = lockHttp(hdl)) return std::string(s->req.target());
			else if (auto ws = lockWs(hdl)) return ws->target;
			return "";
		}

		void httpAppendHeader(ConnectionHdl hdl, const std::string& name, const std::string& value) override {
			if (auto s = lockHttp(hdl)) s->respHeaders.emplace_back(name, value);
		}
		void httpSetBody(ConnectionHdl hdl, const std::string& body) override {
			if (auto s = lockHttp(hdl)) s->respBody = body;
		}
		void httpSetStatus(ConnectionHdl hdl, http::status status) override {
			if (auto s = lockHttp(hdl)) s->respStatus = status;
		}
		void httpDeferResponse(ConnectionHdl hdl) override {
			if (auto s = lockHttp(hdl)) s->markDeferred();
		}
		void httpSendResponse(ConnectionHdl hdl) override {
			if (auto s = lockHttp(hdl)) s->sendAndRelease();
		}

		void wsSendText(ConnectionHdl hdl, const std::string& text) override {
			if (auto s = lockWs(hdl)) {
				s->sendText(text);
			}
		}
		void wsPing(ConnectionHdl hdl) override {
			if (auto s = lockWs(hdl)) {
				s->sendPing();
			}
		}
		void wsClose(ConnectionHdl hdl, uint16_t code, const std::string& reason) override {
			if (auto s = lockWs(hdl)) {
				s->sendClose(code, reason);
			}
		}

	private:
		// Logging helpers
		void logAccess(const std::string& msg) const {
			if (!loggingEnabled_) return;
			std::lock_guard<std::mutex> lk(logMutex_);
			if (accessLog_) (*accessLog_) << msg << std::endl;
		}
		void logError(const std::string& msg) const {
			if (!loggingEnabled_) return;
			std::lock_guard<std::mutex> lk(logMutex_);
			if (errorLog_) (*errorLog_) << msg << std::endl;
		}

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
					static_cast<HttpSession*>(self.get())->onRead(ec);
				});
			}

			void onRead(beast::error_code ec) {
				if (ec) {
					adapter.logError("HTTP read error: " + ec.message());
					return;
				}

				adapter.logAccess("HTTP request " + std::string(req.method_string()) + " " + std::string(req.target()));

				if (websocket::is_upgrade(req)) {
					// Upgrade to websocket
					auto wsSession = std::make_shared<WsSession>(adapter, stream.release_socket(), remoteIp);
					auto h = makeHandle(std::static_pointer_cast<SessionBase>(wsSession));
					wsSession->doAccept(std::move(req), h);
					return;
				}

				hdl = makeHandle(std::static_pointer_cast<SessionBase>(this->shared_from_this()));

				if (adapter.onHttp_) {
					adapter.onHttp_(hdl);
				}

				if (!deferred) {
					sendResponse();
				}
			}

			// keep the session alive while deferred
			void markDeferred() {
				if (!deferred) {
					deferred = true;
					selfKeep = shared_from_this();
					adapter.logAccess("HTTP deferred response");
				}
			}

			void sendAndRelease() {
				sendResponse();
				deferred = false;
				selfKeep.reset();
			}

			void sendResponse() {
				auto beastStatus = static_cast<http::status>(static_cast<unsigned>(respStatus));
				respPtr = std::make_shared<http::response<http::string_body>>(beastStatus, req.version());
				respPtr->set(http::field::server, "airdcpp-beast");
				for (const auto& [k,v] : respHeaders) respPtr->set(k, v);
				respPtr->body() = respBody;
				respPtr->prepare_payload();

				adapter.logAccess("HTTP response " + std::to_string(static_cast<unsigned>(respStatus)) + " bytes=" + std::to_string(respPtr->body().size()));

				auto self = shared_from_this();
				http::async_write(stream, *respPtr, [self](beast::error_code ec, std::size_t /*bytes*/){
					auto* p = static_cast<HttpSession*>(self.get());
					if (ec) {
						p->adapter.logError("HTTP write error: " + ec.message());
					}
					p->respPtr.reset();
					p->doClose();
				});
			}

			void doClose() {
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
			http::status respStatus = http::status::ok;
			std::vector<std::pair<std::string,std::string>> respHeaders;
			std::string respBody;
			bool deferred = false;
			ConnectionHdl hdl;
			std::shared_ptr<http::response<http::string_body>> respPtr;
			std::shared_ptr<HttpSession> selfKeep;
		};

		struct WsSession : public SessionBase, public std::enable_shared_from_this<WsSession> {
			WsSession(BeastServerAdapter& owner, tcp::socket&& socket, const std::string& ip)
				: adapter(owner), ws(std::move(socket)) {
				remoteIp = ip;
			}

			void doAccept(http::request<http::string_body>&& req, ConnectionHdl handle) {
				// Persist the HTTP request for the async_accept lifetime
				upgradeReq_ = std::move(req);
				// Capture essential request info before upgrading
				target = std::string(upgradeReq_.target());
				methodStr = std::string(upgradeReq_.method_string());
				hdl = handle;
				// Accept the websocket handshake
				ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
				ws.async_accept(upgradeReq_, beast::bind_front_handler(&WsSession::onAccept, shared_from_this()));
			}

			void onAccept(beast::error_code ec) {
				// release the stored request
				upgradeReq_ = {};
				if (ec) {
					adapter.logError("WS accept error: " + ec.message());
					return;
				}
				adapter.logAccess("WS open " + target);
				if (adapter.onOpen_) adapter.onOpen_(hdl);
				readLoop();
			}

			void readLoop() {
				ws.async_read(buffer, beast::bind_front_handler(&WsSession::onRead, shared_from_this()));
			}

			void onRead(beast::error_code ec, std::size_t) {
				if (ec) {
					adapter.logAccess("WS close " + target + " reason=" + ec.message());
					if (adapter.onClose_) adapter.onClose_(hdl);
					return;
				}
				if (adapter.onMessage_) {
					auto data = beast::buffers_to_string(buffer.cdata());
					adapter.logAccess("WS message bytes=" + std::to_string(data.size()));
					adapter.onMessage_(hdl, data);
				}
				buffer.consume(buffer.size());
				readLoop();
			}

			// Thread-safe send helpers queued on the ws executor
			void sendText(const std::string& text) {
				auto self = shared_from_this();
				boost::asio::post(ws.get_executor(), [self, text] {
					self->outQueue.push_back(text);
					if (!self->writing) {
						self->startWrite();
					}
				});
			}

			void sendPing() {
				auto self = shared_from_this();
				boost::asio::post(ws.get_executor(), [self] {
					self->ws.async_ping(websocket::ping_data{}, [self](beast::error_code ec){ if(ec) self->adapter.logError("WS ping error: "+ec.message()); });
				});
			}

			void sendClose(uint16_t code, const std::string& /*reason*/) {
				auto self = shared_from_this();
				boost::asio::post(ws.get_executor(), [self, code] {
					websocket::close_reason cr;
					cr.code = static_cast<websocket::close_code>(code);
					self->ws.async_close(cr, [self](beast::error_code ec){ if(ec) self->adapter.logError("WS close error: "+ec.message()); });
				});
			}

			void startWrite() {
				if (outQueue.empty()) return;
				writing = true;
				ws.text(true);
				auto self = shared_from_this();
				ws.async_write(boost::asio::buffer(outQueue.front()), [self](beast::error_code ec, std::size_t bytes) {
					if (ec) {
						self->adapter.logError("WS write error: " + ec.message());
						if (self->adapter.onClose_) self->adapter.onClose_(self->hdl);
						return;
					}
					self->adapter.logAccess("WS sent bytes=" + std::to_string(bytes));
					self->outQueue.pop_front();
					if (!self->outQueue.empty()) {
						self->startWrite();
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
			if (ec) logError("Listen error: " + ec.message());
		}

		void doAccept() {
			acceptor_->async_accept(
				boost::asio::make_strand(*ios_),
				[this](beast::error_code ec, tcp::socket socket) {
					if (!listening_) return;
					if (!ec) {
						auto ip = socket.remote_endpoint(ec).address().to_string();
						logAccess("ACCEPT " + ip);
						auto httpSess = std::make_shared<HttpSession>(*this, std::move(socket), ip);
						httpSess->run();
					} else {
						logError("Accept error: " + ec.message());
					}
					if (listening_) doAccept();
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

		// logging state
		mutable std::mutex logMutex_;
		std::ostream* accessLog_ = nullptr;
		std::ostream* errorLog_ = nullptr;
		bool loggingEnabled_ = false;
	};
}

#endif // DCPLUSPLUS_WEBSERVER_BEAST_SERVER_ADAPTER_H
