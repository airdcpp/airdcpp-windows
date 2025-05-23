/*
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
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
#include <airdcpp/connection/socket/BufferedSocket.h>

#include <algorithm>

#include <boost/scoped_array.hpp>

#include <airdcpp/connectivity/ConnectivityManager.h>
#include <airdcpp/settings/SettingsManager.h>
#include <airdcpp/connection/socket/SSLSocket.h>
#include <airdcpp/core/io/stream/StreamBase.h>
#include <airdcpp/connection/ThrottleManager.h>
#include <airdcpp/core/timer/TimerManager.h>
#include <airdcpp/core/io/compress/ZUtils.h>

namespace dcpp {

using std::min;
using std::max;

// Polling is used for tasks...should be fixed...
constexpr auto POLL_TIMEOUT = 250;

BufferedSocket::BufferedSocket(char aSeparator, bool v4only) :
separator(aSeparator), v4only(v4only) {
	++sockets;
}

atomic<long> BufferedSocket::sockets(0);

BufferedSocket::~BufferedSocket() {
	--sockets;
}

void BufferedSocket::disconnect(bool graceless) noexcept {
	Lock l(cs); 
	if (graceless) 
		disconnecting = true; 
	addTask(DISCONNECT, nullptr); 
}

void BufferedSocket::setMode (Modes aMode, size_t aRollback) {
	if (mode == aMode) {
		dcdebug ("WARNING: Re-entering mode %d\n", mode);
		return;
	}

	if(mode == MODE_ZPIPE && filterIn) {
		// delete the filter when going out of zpipe mode.
		filterIn.reset();
	}

	switch (aMode) {
		case MODE_LINE:
			rollback = aRollback;
			break;
		case MODE_ZPIPE:
			filterIn = make_unique<UnZFilter>();
			break;
		case MODE_DATA:
			break;
	}
	mode = aMode;
}

void BufferedSocket::setSocket(unique_ptr<Socket>&& s) {
	dcassert(!sock.get());
	sock = std::move(s);
	sock->setV4only(v4only);
}

void BufferedSocket::setOptions() {
	if(SETTING(SOCKET_IN_BUFFER) > 0)
		sock->setSocketOpt(SO_RCVBUF, SETTING(SOCKET_IN_BUFFER));
	if(SETTING(SOCKET_OUT_BUFFER) > 0)
		sock->setSocketOpt(SO_SNDBUF, SETTING(SOCKET_OUT_BUFFER));
}

void BufferedSocket::accept(const Socket& srv, bool aSecure, bool aAllowUntrusted, const SocketAcceptFloodF& aFloodCheckF) {
	//dcdebug("BufferedSocket::accept() %p\n", (void*)this);

	unique_ptr<Socket> s(aSecure ? new SSLSocket(CryptoManager::SSL_SERVER, aAllowUntrusted, Util::emptyString) : new Socket(Socket::TYPE_TCP));

	s->accept(srv);
	if (!aFloodCheckF(s->getIp())) {
		s->disconnect();
		throw SocketException("Connect limit exceeded");
	}

	setSocket(std::move(s));
	setOptions();

	start();

	Lock l(cs);
	addTask(ACCEPTED, nullptr);
}

void BufferedSocket::connect(const AddressInfo& aAddress, const SocketConnectOptions& aOptions, bool allowUntrusted, bool proxy, const string& expKP) {
	connect(aAddress, aOptions, Util::emptyString, allowUntrusted, proxy, expKP);
}

CryptoManager::SSLContext toSSLContext(NatRole aNatRole) noexcept {
	return aNatRole == NatRole::SERVER ? CryptoManager::SSL_SERVER : CryptoManager::SSL_CLIENT;
}

void BufferedSocket::connect(const AddressInfo& aAddress, const SocketConnectOptions& aOptions, const string& aLocalPort, bool aAllowUntrusted, bool aProxy, const string& expKP) {
	//dcdebug("BufferedSocket::connect() %p\n", (void*)this);
	auto s(aOptions.secure ?
		make_unique<SSLSocket>(toSSLContext(aOptions.natRole), aAllowUntrusted, expKP) :
		make_unique<Socket>(Socket::TYPE_TCP)
	);

	s->setLocalIp4(CONNSETTING(BIND_ADDRESS));
	s->setLocalIp6(CONNSETTING(BIND_ADDRESS6));

	setSocket(std::move(s));

	start();

	auto proxy = aProxy && (CONNSETTING(OUTGOING_CONNECTIONS) == SettingsManager::OUTGOING_SOCKS5);

	Lock l(cs);
	addTask(CONNECT, make_unique<ConnectInfo>(aAddress, aOptions.port, aLocalPort, aOptions.natRole, proxy));
}

constexpr auto LONG_TIMEOUT = 30000;
constexpr auto SHORT_TIMEOUT = 1000;
void BufferedSocket::threadConnect(const AddressInfo& aAddr, const string& aPort, const string& aLocalPort, NatRole aNatRole, bool aProxy) {
	dcassert(state == STARTING);

	fire(BufferedSocketListener::Connecting());

	const uint64_t endTime = GET_TICK() + LONG_TIMEOUT;
	state = RUNNING;

	while (GET_TICK() < endTime) {
		//dcdebug("threadConnect attempt %s %s:%s\n", localPort.c_str(), aAddr.c_str(), aPort.c_str());
		try {

			if (aProxy) {
				sock->socksConnect(aAddr, aPort, LONG_TIMEOUT);
			} else {
				sock->connect(aAddr, aPort, aLocalPort);
			}

			setOptions();

			bool connSucceeded;
			while(!(connSucceeded = sock->waitConnected(POLL_TIMEOUT)) && endTime >= GET_TICK()) {
				if(disconnecting) return;
			}

			if (connSucceeded) {
				inbuf.resize(sock->getSocketOptInt(SO_RCVBUF));

				fire(BufferedSocketListener::Connected());
				return;
			}
		}
		catch (const SSLSocketException&) {
			throw;
		} catch (const SocketException&) {
			if (aNatRole == NatRole::NONE)
				throw;
			Thread::sleep(SHORT_TIMEOUT);
		}
	}

	throw SocketException(STRING(CONNECTION_TIMEOUT));
}

void BufferedSocket::threadAccept() {
	dcassert(state == STARTING);

	//dcdebug("threadAccept\n");

	state = RUNNING;

	inbuf.resize(sock->getSocketOptInt(SO_RCVBUF));

	uint64_t startTime = GET_TICK();
	while(!sock->waitAccepted(POLL_TIMEOUT)) {
		if(disconnecting)
			return;

		if((startTime + 30000) < GET_TICK()) {
			throw SocketException(STRING(CONNECTION_TIMEOUT));
		}
	}
}

void BufferedSocket::threadRead() {
	if(state != RUNNING)
		return;

	int left = (mode == MODE_DATA && useLimiter) ? ThrottleManager::getInstance()->read(sock.get(), &inbuf[0], inbuf.size()) : sock->read(&inbuf[0], inbuf.size());
	if(left == -1) {
		// EWOULDBLOCK, no data received...
		return;
	} else if(left == 0) {
		// This socket has been closed...
		throw SocketException(STRING(CONNECTION_CLOSED));
	}

	string::size_type pos = 0;
	// always uncompressed data
	string l;
	int bufpos = 0, total = left;

	while (left > 0) {
		switch (mode) {
			case MODE_ZPIPE: {
					const int BUF_SIZE = 1024;
					// Special to autodetect nmdc connections...
					boost::scoped_array<char> buffer(new char[BUF_SIZE]);
					l = line;
					// decompress all input data and store in l.
					while (left) {
						size_t in = BUF_SIZE;
						size_t used = left;
						bool ret = (*filterIn) (&inbuf[0] + total - left, used, &buffer[0], in);
						left -= used;
						l.append (&buffer[0], in);
						// if the stream ends before the data runs out, keep remainder of data in inbuf
						if (!ret) {
							bufpos = total-left;
							setMode (MODE_LINE, rollback);
							break;
						}
					}
					// process all lines
					while ((pos = l.find(separator)) != string::npos) {
                       	if(pos > 0) // check empty (only pipe) command and don't waste cpu with it ;o)
							fire(BufferedSocketListener::Line(), l.substr(0, pos));
						l.erase (0, pos + 1 /* separator char */);
					}
					// store remainder
					line = l;

					break;
				}
			case MODE_LINE:
				// Special to autodetect nmdc connections...
				if(separator == 0) {
					if(inbuf[0] == '$') {
						separator = '|';
					} else {
						separator = '\n';
					}
				}
				l = line + string ((char*)&inbuf[bufpos], left);
				while ((pos = l.find(separator)) != string::npos) {
	                if(pos > 0) // check empty (only pipe) command and don't waste cpu with it ;o)
						fire(BufferedSocketListener::Line(), l.substr(0, pos));
					l.erase (0, pos + 1 /* separator char */);
					if (l.length() < (size_t)left) left = l.length();
					if (mode != MODE_LINE) {
						// we changed mode; remainder of l is invalid.
						l.clear();
						bufpos = total - left;
						break;
					}
				}
				if (pos == string::npos) 
					left = 0;
				line = l;
				break;
			case MODE_DATA:
				while(left > 0) {
					if(dataBytes == -1) {
						fire(BufferedSocketListener::Data(), &inbuf[bufpos], left);
						bufpos += (left - rollback);
						left = static_cast<int>(rollback);
						rollback = 0;
					} else {
						auto high = (int)min(dataBytes, (int64_t)left);
						fire(BufferedSocketListener::Data(), &inbuf[bufpos], high);
						bufpos += high;
						left -= high;

						dataBytes -= high;
						if(dataBytes == 0) {
							mode = MODE_LINE;
							fire(BufferedSocketListener::ModeChange());
							break; // break loop, in case setDataMode is called with less than read buffer size
						}
					}
				}
				break;
		}
	}

	if(mode == MODE_LINE && line.size() > static_cast<size_t>(SETTING(MAX_COMMAND_LENGTH))) {
		throw SocketException(STRING(COMMAND_TOO_LONG));
	}
}

void BufferedSocket::threadSendFile(InputStream* file) {
	if(state != RUNNING)
		return;

	if(disconnecting)
		return;
	dcassert(file);
	auto sockSize = (size_t)sock->getSocketOptInt(SO_SNDBUF);
	size_t bufSize = max(sockSize, (size_t)64*1024);

	ByteVector readBuf(bufSize);
	ByteVector writeBufTmp(bufSize);

	size_t readPos = 0;

	bool readDone = false;
	//dcdebug("Starting threadSend\n");
	while(!disconnecting) {
		if(!readDone && readBuf.size() > readPos) {
			// Fill read buffer
			size_t bytesRead = readBuf.size() - readPos;
			size_t actual = file->read(&readBuf[readPos], bytesRead);

			if(bytesRead > 0) {
				fire(BufferedSocketListener::BytesSent(), bytesRead, 0);
			}

			if(actual == 0) {
				readDone = true;
			} else {
				readPos += actual;
			}
		}

		if(readDone && readPos == 0) {
			fire(BufferedSocketListener::TransmitDone());
			return;
		}

		readBuf.swap(writeBufTmp);
		readBuf.resize(bufSize);
		writeBufTmp.resize(readPos);
		readPos = 0;

		size_t writePos = 0, writeSize = 0;
		int written = 0;

		while(writePos < writeBufTmp.size()) {
			if(disconnecting)
				return;

			// Process possible async calls 
			// File uploads can take a really long time so we don't want to wait for it to finish
			checkEvents();

			if(written == -1) {
				// workaround for OpenSSL (crashes when previous write failed and now retrying with different writeSize)
				written = sock->write(&writeBufTmp[writePos], writeSize);
			} else {
				writeSize = min(sockSize / 2, writeBufTmp.size() - writePos);
				written = useLimiter ? 
					ThrottleManager::getInstance()->write(sock.get(), &writeBufTmp[writePos], writeSize) : 
					sock->read(&inbuf[0], inbuf.size());
			}
			
			if(written > 0) {
				writePos += written;

				fire(BufferedSocketListener::BytesSent(), 0, written);

			} else if(written == -1) {
				if(!readDone && readPos < readBuf.size()) {
					// Read a little since we're blocking anyway...
					size_t bytesRead = min(readBuf.size() - readPos, readBuf.size() / 2);
					size_t actual = file->read(&readBuf[readPos], bytesRead);

					if(bytesRead > 0) {
						fire(BufferedSocketListener::BytesSent(), bytesRead, 0);
					}

					if(actual == 0) {
						readDone = true;
					} else {
						readPos += actual;
					}
				} else {
					while(!disconnecting) {
						auto [read, write] = sock->wait(POLL_TIMEOUT, true, true);
						if (read) {
							threadRead();
						}
						if (write) {
							break;
						}
					}
				}
			}
		}
	}
}

void BufferedSocket::write(const char* aBuf, size_t aLen) noexcept {
	if(!sock.get())
		return;
	Lock l(cs);
	if(writeBuf.empty())
		addTask(SEND_DATA, nullptr);

	writeBuf.insert(writeBuf.end(), aBuf, aBuf+aLen);
}

void BufferedSocket::threadSendData() {
	if(state != RUNNING)
		return;

	{
		Lock l(cs);
		if(writeBuf.empty())
			return;

		writeBuf.swap(sendBuf);
	}

	size_t left = sendBuf.size();
	size_t done = 0;
	while(left > 0) {
		if(disconnecting) {
			return;
		}

		auto [read, write] = sock->wait(POLL_TIMEOUT, true, true);

		if(read) {
			threadRead();
		}

		if(write) {
			int n = sock->write(&sendBuf[done], left);
			if(n > 0) {
				left -= n;
				done += n;
			}
		}
	}
	sendBuf.clear();
}

bool BufferedSocket::checkEvents() {
	while(state == RUNNING ? taskSem.wait(0) : taskSem.wait()) {
		TaskPair p;
		{
			Lock l(cs);
			dcassert(!tasks.empty());
			p = std::move(tasks.front());
			tasks.erase(tasks.begin());
		}

		if(p.first == SHUTDOWN) {
			if (p.second)
				static_cast<CallData*>(p.second.get())->f();
			return false;
		}

		if(state == STARTING) {
			if(p.first == CONNECT) {
				auto ci = static_cast<ConnectInfo*>(p.second.get());
				threadConnect(ci->addr, ci->port, ci->localPort, ci->natRole, ci->proxy);
			} else if(p.first == ACCEPTED) {
				threadAccept();
			} else {
				dcdebug("%d unexpected in STARTING state\n", p.first);
			}
		} else if(state == RUNNING) {
			if(p.first == SEND_DATA) {
				threadSendData();
			} else if(p.first == SEND_FILE) {
				threadSendFile(static_cast<SendFileInfo*>(p.second.get())->stream); break;
			} else if(p.first == DISCONNECT) {
				fail(STRING(DISCONNECTED));
			} else if (p.first == ASYNC_CALL) {
				static_cast<CallData*>(p.second.get())->f();
			} else {
				dcdebug("%d unexpected in RUNNING state\n", p.first);
			}
		}
	}
	return true;
}

void BufferedSocket::checkSocket() {
	auto [read, _] = sock->wait(POLL_TIMEOUT, true, false);
	if (read) {
		threadRead();
	}
}

/**
 * Main task dispatcher for the buffered socket abstraction.
 * @todo Fix the polling...
 */
int BufferedSocket::run() {
	//dcdebug("BufferedSocket::run() start %p\n", (void*)this);
	while(true) {
		try {
			if(!checkEvents()) {
				break;
			}
			if(state == RUNNING) {
				checkSocket();
			}
		} catch(const Exception& e) {
			fail(e.getError());
		}
	}
	//dcdebug("BufferedSocket::run() end %p\n", (void*)this);
	delete this;
	return 0;
}

void BufferedSocket::fail(const string& aError) {
	if(state != FAILED) {
		state = FAILED;
		fire(BufferedSocketListener::Failed(), aError);
	}
	//fire listener before deleting socket to be able to retrieve information from it.. does it cause any problems?? 
	if (sock.get()) {
		sock->disconnect();
	}
}

void BufferedSocket::shutdown(const Callback& f) {
	Lock l(cs);
	disconnecting = true;
	addTask(SHUTDOWN, f ? make_unique<CallData>(f) : nullptr);
}

void BufferedSocket::addTask(Tasks task, unique_ptr<TaskData>&& data) {
	dcassert(task == DISCONNECT || task == SHUTDOWN || task == ASYNC_CALL || sock.get());
	tasks.emplace_back(task, std::move(data)); taskSem.signal();
}

} // namespace dcpp
