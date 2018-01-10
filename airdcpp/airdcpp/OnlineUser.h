/*
 * Copyright (C) 2001-2017 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_ONLINEUSER_H_
#define DCPLUSPLUS_DCPP_ONLINEUSER_H_

#include <map>

#include <boost/noncopyable.hpp>

#include "forward.h"

#include "CriticalSection.h"
#include "Flags.h"
#include "FastAlloc.h"
#include "GetSet.h"
#include "Pointer.h"
#include "Util.h"
#include "User.h"
#include "UserInfoBase.h"

namespace dcpp {

/** One of possibly many identities of a user, mainly for UI purposes */
class Identity : public Flags {
public:
	enum ClientType {
		CT_BOT = 1,
		CT_REGGED = 2,
		CT_OP = 4,
		CT_SU = 8,
		CT_OWNER = 16,
		CT_HUB = 32,
		CT_HIDDEN = 64
	};
	
	enum StatusFlags {
		NORMAL		= 0x01,
		AWAY		= 0x02,
		TLS			= 0x10,
		NAT			= 0x20,
		AIRDC		= 0x40
	};

	enum Mode {
		MODE_UNDEFINED,
		MODE_ME,
		MODE_NOCONNECT_IP,
		MODE_NOCONNECT_PASSIVE,
		MODE_ACTIVE_DUAL,
		MODE_ACTIVE_V4,
		MODE_ACTIVE_V6,
		MODE_PASSIVE_V4,
		MODE_PASSIVE_V6,
		MODE_PASSIVE_V4_UNKNOWN,
		MODE_PASSIVE_V6_UNKNOWN,
	};

	Identity();
	Identity(const UserPtr& ptr, uint32_t aSID);
	Identity(const Identity& rhs);
	Identity& operator=(const Identity& rhs);


#define GETSET_FIELD(n, x) string get##n() const { return get(x); } void set##n(const string& v) { set(x, v); }
	GETSET_FIELD(Nick, "NI")
	GETSET_FIELD(Description, "DE")
	GETSET_FIELD(Ip4, "I4")
	GETSET_FIELD(Ip6, "I6")
	GETSET_FIELD(Udp4Port, "U4")
	GETSET_FIELD(Udp6Port, "U6")
	GETSET_FIELD(Email, "EM")
	GETSET_FIELD(NmdcConnection, "CO")
	GETSET_FIELD(DownloadSpeed, "DS")
	//GETSET_FIELD(UploadSpeed, "US")
	GETSET_FIELD(SharedFiles, "SF")
	GETSET_FIELD(ShareSize, "SS")
#undef GETSET_FIELD
	uint8_t getSlots() const noexcept;
	void setBytesShared(const string& bs) noexcept { set("SS", bs); }
	int64_t getBytesShared() const noexcept { return Util::toInt64(get("SS")); }
	
	void setStatus(const string& st) noexcept { set("ST", st); }
	StatusFlags getStatus() const noexcept { return static_cast<StatusFlags>(Util::toInt(get("ST"))); }

	void setOp(bool op) noexcept { set("OP", op ? "1" : Util::emptyString); }
	void setHub(bool hub) noexcept { set("HU", hub ? "1" : Util::emptyString); }
	void setBot(bool bot) noexcept { set("BO", bot ? "1" : Util::emptyString); }
	void setHidden(bool hidden) noexcept { set("HI", hidden ? "1" : Util::emptyString); }
	string getTag() const noexcept;
	string getApplication() const noexcept;
	int getTotalHubCount() const noexcept;
	const string getCountry() const noexcept;
	StringList getSupports() const noexcept;
	bool supports(const string& name) const noexcept;
	bool isHub() const noexcept { return isClientType(CT_HUB) || isSet("HU"); }
	bool isOp() const noexcept { return isClientType(CT_OP) || isClientType(CT_SU) || isClientType(CT_OWNER) || isSet("OP"); }
	bool isRegistered() const noexcept { return isClientType(CT_REGGED) || isSet("RG"); }
	bool isHidden() const noexcept { return isClientType(CT_HIDDEN) || isClientType(CT_HUB) || isSet("HI"); }
	bool isBot() const noexcept { return isClientType(CT_BOT) || isSet("BO"); }
	bool isAway() const noexcept { return (getStatus() & AWAY) || isSet("AW"); }
	bool isTcpActive(const ClientPtr& = nullptr) const noexcept;
	bool isTcp4Active(const ClientPtr& = nullptr) const noexcept;
	bool isTcp6Active() const noexcept;
	bool isUdpActive() const noexcept;
	bool isUdp4Active() const noexcept;
	bool isUdp6Active() const noexcept;
	string getIp() const noexcept;
	string getUdpPort() const noexcept;
	string getV4ModeString() const noexcept;
	string getV6ModeString() const noexcept;

	string getConnectionString() const noexcept;
	int64_t getAdcConnectionSpeed(bool download) const noexcept;

	std::map<string, string> getInfo() const noexcept;
	string get(const char* name) const noexcept;
	void set(const char* name, const string& val) noexcept;
	bool isSet(const char* name) const noexcept;
	string getSIDString() const noexcept { return string((const char*)&sid, 4); }
	
	bool isClientType(ClientType ct) const noexcept;
	
	void getParams(ParamMap& map, const string& prefix, bool compatibility) const noexcept;
	const UserPtr& getUser() const noexcept { return user; }
	UserPtr& getUser() noexcept { return user; }
	uint32_t getSID() const noexcept { return sid; }

	//this caches the mode, taking into account what we support too
	GETSET(Mode, connectMode, ConnectMode);
	bool updateConnectMode(const Identity& me, const Client* aClient) noexcept;

	bool allowV4Connections() const noexcept;
	bool allowV6Connections() const noexcept;
private:
	UserPtr user;
	uint32_t sid;

	typedef map<short, string> InfMap;
	InfMap info;

	static SharedMutex cs;
};

class OnlineUser :  public FastAlloc<OnlineUser>, public intrusive_ptr_base<OnlineUser>, public UserInfoBase, private boost::noncopyable {
public:
	enum {
		COLUMN_FIRST,
		COLUMN_NICK = COLUMN_FIRST, 
		COLUMN_SHARED, 
		COLUMN_EXACT_SHARED, 
		COLUMN_DESCRIPTION, 
		COLUMN_TAG,
		COLUMN_ULSPEED,
		COLUMN_DLSPEED,
		COLUMN_IP4,
		COLUMN_IP6,
		COLUMN_EMAIL, 
		COLUMN_VERSION, 
		COLUMN_MODE4,
		COLUMN_MODE6,
		COLUMN_FILES, 
		COLUMN_HUBS, 
		COLUMN_SLOTS,
		COLUMN_CID,
		COLUMN_LAST
	};

	struct Hash {
		size_t operator()(const OnlineUserPtr& x) const { return ((size_t)(&(*x)))/sizeof(OnlineUser); }
	};

	struct NickSort {
		bool operator()(const OnlineUserPtr& left, const OnlineUserPtr& right) const;
	};

	struct Nick {
		string operator()(const OnlineUserPtr& u) { return u->getIdentity().getNick(); }
	};

	struct HubName {
		string operator()(const OnlineUserPtr& u);
	};

	class UrlCompare {
	public:
		UrlCompare(const string& aUrl) : url(aUrl) { }
		bool operator()(const OnlineUserPtr& ou) { return ou->getHubUrl() == url; }

		UrlCompare& operator=(const UrlCompare&) = delete;
	private:
		const string& url;
	};

	OnlineUser(const UserPtr& ptr, const ClientPtr& client_, uint32_t sid_);
	~OnlineUser() noexcept;

	uint32_t getToken() const noexcept {
		return identity.getSID();
	}

	operator UserPtr&() noexcept { return getUser(); }
	operator const UserPtr&() const noexcept { return getUser(); }

	UserPtr& getUser() noexcept { return getIdentity().getUser(); }
	const UserPtr& getUser() const noexcept { return getIdentity().getUser(); }
	const string& getHubUrl() const noexcept;
	Identity& getIdentity() noexcept { return identity; }

	/* UserInfo */
	uint8_t getImageIndex() const noexcept;
	bool isHidden() const noexcept { return identity.isHidden(); }

	const ClientPtr& getClient() const noexcept { return client; }
#ifdef _WIN32
	static int compareItems(const OnlineUser* a, const OnlineUser* b, uint8_t col) noexcept;
	bool update(int sortCol, const tstring& oldText = Util::emptyStringT);
	tstring getText(uint8_t col, bool copy = false) const;
#endif

	string getLogPath() const noexcept;
	bool supportsCCPM() const noexcept;

	bool isInList;
	GETSET(Identity, identity, Identity);
private:
	ClientPtr client;
};

}

#endif /* ONLINEUSER_H_ */
