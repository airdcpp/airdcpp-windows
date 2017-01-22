/* 
 * Copyright (C) 2001-2016 Jacek Sieka, arnetheduck on gmail point com
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

#include "stdinc.h"
#include "SearchManager.h"

#include "AdcHub.h"
#include "AirUtil.h"
#include "ClientManager.h"
#include "QueueManager.h"
#include "ResourceManager.h"
#include "ScopedFunctor.h"
#include "SearchResult.h"
#include "ShareManager.h"
#include "SimpleXML.h"
#include "StringTokenizer.h"
#include "TimerManager.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

namespace dcpp {

const char* SearchManager::types[Search::TYPE_LAST] = {
	CSTRING(ANY),
	CSTRING(AUDIO),
	CSTRING(COMPRESSED),
	CSTRING(DOCUMENT),
	CSTRING(EXECUTABLE),
	CSTRING(PICTURE),
	CSTRING(VIDEO),
	CSTRING(DIRECTORY),
	"TTH",
	CSTRING(FILE)
};
const char* SearchManager::getTypeStr(int type) {
	return types[type];
}

bool SearchManager::isDefaultTypeStr(const string& type) {
	 return type.size() == 1 && type[0] >= '0' && type[0] <= '9';
}

SearchManager::SearchManager() {
	setSearchTypeDefaults();
	TimerManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);
}

SearchManager::~SearchManager() {
	TimerManager::getInstance()->removeListener(this);
	SettingsManager::getInstance()->removeListener(this);

	for(auto& p: searchKeys) 
		delete p.first;
}

string SearchManager::normalizeWhitespace(const string& aString){
	string::size_type found = 0;
	string normalized = aString;
	while((found = normalized.find_first_of("\t\n\r", found)) != string::npos) {
		normalized[found] = ' ';
		found++;
	}
	return normalized;
}

SearchQueueInfo SearchManager::search(const SearchPtr& aSearch) noexcept {
	StringList who;
	ClientManager::getInstance()->getOnlineClients(who);

	return search(who, aSearch);
}

SearchQueueInfo SearchManager::search(StringList& aHubUrls, const SearchPtr& aSearch, void* aOwner /* NULL */) noexcept {

	string keyStr;
	if (SETTING(ENABLE_SUDP)) {
		//generate a random key and store it so we can check the results
		uint8_t* key = new uint8_t[16];
		RAND_bytes(key, 16);
		{
			WLock l (cs);
			searchKeys.emplace_back(key, GET_TICK());
		}
		keyStr = Encoder::toBase32(key, 16);
	}

	aSearch->owner = aOwner;
	aSearch->key = keyStr;

	StringSet queued;
	uint64_t estimateSearchSpan = 0;
	string lastError;
	for(auto& hubUrl: aHubUrls) {
		auto queueTime = ClientManager::getInstance()->search(hubUrl, aSearch, lastError);
		if (queueTime) {
			estimateSearchSpan = max(estimateSearchSpan, *queueTime);
			queued.insert(hubUrl);
		}
	}

	return { queued, estimateSearchSpan, lastError };
}

bool SearchManager::decryptPacket(string& x, size_t aLen, const ByteVector& aBuf) {
	RLock l (cs);
	for(auto& i: searchKeys | reversed) {
		boost::scoped_array<uint8_t> out(new uint8_t[aBuf.size()]);

		uint8_t ivd[16] = { };

		EVP_CIPHER_CTX ctx;
		EVP_CIPHER_CTX_init(&ctx);

		int len = 0, tmpLen=0;
		EVP_DecryptInit_ex(&ctx, EVP_aes_128_cbc(), NULL, i.first, ivd);
		EVP_DecryptUpdate(&ctx, &out[0], &len, aBuf.data(), aLen);
		EVP_DecryptFinal_ex(&ctx, &out[0] + aLen, &tmpLen);
		EVP_CIPHER_CTX_cleanup(&ctx);

		// Validate padding and replace with 0-bytes.
		int padlen = out[aLen-1];
		if(padlen < 1 || padlen > 16) {
			continue;
		}

		bool valid = true;
		for(auto r=0; r<padlen; r++) {
			if(out[aLen-padlen+r] != padlen) {
				valid = false;
				break;
			} else
				out[aLen-padlen+r] = 0;
		}

		if (valid) {
			x = (char*)&out[0]+16;
			break;
		}
	}
	return true;
}

const string& SearchManager::getPort() const { 
	return udpServer.getPort(); 
}

void SearchManager::listen() {
	udpServer.listen();
}

void SearchManager::disconnect() noexcept {
	udpServer.disconnect();
}

void SearchManager::onSR(const string& x, const string& aRemoteIP /*Util::emptyString*/) {
	string::size_type i, j;
	// Directories: $SR <nick><0x20><directory><0x20><free slots>/<total slots><0x05><Hubname><0x20>(<Hubip:port>)
	// Files:		$SR <nick><0x20><filename><0x05><filesize><0x20><free slots>/<total slots><0x05><Hubname><0x20>(<Hubip:port>)
	i = 4;
	if( (j = x.find(' ', i)) == string::npos) {
		return;
	}
	string nick = x.substr(i, j-i);
	i = j + 1;

	// A file has 2 0x05, a directory only one
	size_t cnt = count(x.begin() + j, x.end(), 0x05);

	SearchResult::Types type = SearchResult::TYPE_FILE;
	string file;
	int64_t size = 0;

	if(cnt == 1) {
		// We have a directory...find the first space beyond the first 0x05 from the back
		// (dirs might contain spaces as well...clever protocol, eh?)
		type = SearchResult::TYPE_DIRECTORY;
		// Get past the hubname that might contain spaces
		if((j = x.rfind(0x05)) == string::npos) {
			return;
		}
		// Find the end of the directory info
		if((j = x.rfind(' ', j-1)) == string::npos) {
			return;
		}
		if(j < i + 1) {
			return;
		}
		file = x.substr(i, j-i) + '\\';
	} else if(cnt == 2) {
		if( (j = x.find((char)5, i)) == string::npos) {
			return;
		}
		file = x.substr(i, j-i);
		i = j + 1;
		if( (j = x.find(' ', i)) == string::npos) {
			return;
		}
		size = Util::toInt64(x.substr(i, j-i));
	}
	i = j + 1;

	if( (j = x.find('/', i)) == string::npos) {
		return;
	}
	uint8_t freeSlots = (uint8_t)Util::toInt(x.substr(i, j-i));
	i = j + 1;
	if( (j = x.find((char)5, i)) == string::npos) {
		return;
	}
	uint8_t slots = (uint8_t)Util::toInt(x.substr(i, j-i));
	i = j + 1;
	if( (j = x.rfind(" (")) == string::npos) {
		return;
	}
	string hubName = x.substr(i, j-i);
	i = j + 2;
	if( (j = x.rfind(')')) == string::npos) {
		return;
	}

	string connection;
	HintedUser user;
	if (!ClientManager::getInstance()->connectNMDCSearchResult(aRemoteIP, x.substr(i, j - i), user, nick, connection, file, hubName))
		return;


	string tth;
	if(hubName.compare(0, 4, "TTH:") == 0) {
		tth = hubName.substr(4);
	}

	if(tth.empty() && type == SearchResult::TYPE_FILE && !SettingsManager::lanMode) {
		return;
	}


	auto sr = make_shared<SearchResult>(user, type, slots, freeSlots, size,
		file, aRemoteIP, SettingsManager::lanMode ? TTHValue() : TTHValue(tth), Util::emptyString, 0, connection, DirectoryContentInfo());
	fire(SearchManagerListener::SR(), sr);
}

void SearchManager::onRES(const AdcCommand& cmd, const UserPtr& from, const string& remoteIp) {
	int freeSlots = -1;
	int64_t size = -1;
	string file;
	string tth;
	string token;
	time_t date = 0;
	int files = -1, folders = -1;

	for(auto& str: cmd.getParameters()) {
		if(str.compare(0, 2, "FN") == 0) {
			file = Util::toNmdcFile(str.substr(2));
		} else if(str.compare(0, 2, "SL") == 0) {
			freeSlots = Util::toInt(str.substr(2));
		} else if(str.compare(0, 2, "SI") == 0) {
			size = Util::toInt64(str.substr(2));
		} else if(str.compare(0, 2, "TR") == 0) {
			tth = str.substr(2);
		} else if(str.compare(0, 2, "TO") == 0) {
			token = str.substr(2);
		} else if(str.compare(0, 2, "DM") == 0) {
			date = Util::toUInt32(str.substr(2));
		} else if(str.compare(0, 2, "FI") == 0) {
			files = Util::toInt(str.substr(2));
		} else if(str.compare(0, 2, "FO") == 0) {
			folders = Util::toInt(str.substr(2));
		}
	}

	if(freeSlots != -1 && size != -1) {
		//connect to a correct hub
		string hubUrl, connection;
		uint8_t slots = 0;
		if (!ClientManager::getInstance()->connectADCSearchResult(from->getCID(), token, hubUrl, connection, slots))
			return;

		auto type = (file.empty() || file.back() == NMDC_SEPARATOR ? SearchResult::TYPE_DIRECTORY : SearchResult::TYPE_FILE);
		if(type == SearchResult::TYPE_FILE && tth.empty())
			return;

		TTHValue th;
		if (type == SearchResult::TYPE_DIRECTORY || SettingsManager::lanMode) {
			//calculate a TTH from the directory name and size
			th = AirUtil::getTTH(type == SearchResult::TYPE_FILE ? Util::getNmdcFileName(file) : Util::getNmdcLastDir(file), size);
		} else {
			th = TTHValue(tth);
		}
		
		auto sr = make_shared<SearchResult>(HintedUser(from, hubUrl), type, slots, (uint8_t)freeSlots, size,
			file, remoteIp, th, token, date, connection, DirectoryContentInfo(folders, files));
		fire(SearchManagerListener::SR(), sr);
	}
}

void SearchManager::on(TimerManagerListener::Minute, uint64_t aTick) noexcept {
	WLock l (cs);
	for (auto i = searchKeys.begin(); i != searchKeys.end();) {
		if (i->second + 1000*60*15 < aTick) {
			delete i->first;
			searchKeys.erase(i);
			i = searchKeys.begin();
		} else {
			++i;
		}
	}
}

void SearchManager::onPBD(const AdcCommand& aCmd, const UserPtr& from) {
	string remoteBundle;
	string hubIpPort;
	string tth;
	bool add=false, update=false, reply=false, notify = false, remove = false;

	for(auto& str: aCmd.getParameters()) {
		if(str.compare(0, 2, "HI") == 0) {
			hubIpPort = str.substr(2);
		} else if(str.compare(0, 2, "BU") == 0) {
			remoteBundle = str.substr(2);
		} else if(str.compare(0, 2, "TH") == 0) {
			tth = str.substr(2);
		} else if(str.compare(0, 2, "UP") == 0) { //new notification of a finished TTH
			update=true;
		} else if (str.compare(0, 2, "AD") == 0) { //add TTHList
			add=true;
		} else if (str.compare(0, 2, "RE") == 0) { //require reply
			reply=true;
		} else if (str.compare(0, 2, "NO") == 0) { //notify only, don't download TTHList
			notify=true;
		} else if (str.compare(0, 2, "RM") == 0) { //remove notifications for a selected user and bundle
			remove=true;
		} else {
			//LogManager::getInstance()->message("ONPBD UNKNOWN PARAM: " + str);
		}
	}

	if (remove && !remoteBundle.empty()) {
		//LogManager::getInstance()->message("ONPBD REMOVE");
		// Local bundle really...
		QueueManager::getInstance()->removeBundleNotify(from, Util::toUInt32(remoteBundle));
	}

	if (tth.empty()) {
		//LogManager::getInstance()->message("ONPBD EMPTY TTH");
		return;
	}

	string url = ClientManager::getInstance()->findHub(hubIpPort, !from);

	if (update) {
		//LogManager::getInstance()->message("PBD UPDATE TTH");
		QueueManager::getInstance()->updatePBD(HintedUser(from, url), TTHValue(tth));
		return;
	} else if (remoteBundle.empty()) {
		//LogManager::getInstance()->message("ONPBD EMPTY BUNDLE");
		return;
	}

	HintedUser u = HintedUser(from, url);
	if (notify) {
		//LogManager::getInstance()->message("PBD NOTIFY");
		QueueManager::getInstance()->addFinishedNotify(u, TTHValue(tth), remoteBundle);
	} else if (reply) {
		//LogManager::getInstance()->message("PBD REQUIRE REPLY");

		string localBundle;
		bool sendNotify = false, sendAdd = false;
		if (QueueManager::getInstance()->checkPBDReply(u, TTHValue(tth), localBundle, sendNotify, sendAdd, remoteBundle)) {
			//LogManager::getInstance()->message("PBD REPLY: ACCEPTED");
			AdcCommand cmd = toPBD(hubIpPort, localBundle, tth, false, sendAdd, sendNotify);
			ClientManager::getInstance()->sendUDP(cmd, from->getCID(), false, true);
		} else {
			//LogManager::getInstance()->message("PBD REPLY: QUEUEMANAGER FAIL");
		}
	}

	if (add) {
		try {
			QueueManager::getInstance()->addBundleTTHList(u, remoteBundle, TTHValue(tth));
		}catch(const Exception&) { }
	}
}

void SearchManager::onPSR(const AdcCommand& aCmd, UserPtr from, const string& remoteIp) {
	if (!SETTING(USE_PARTIAL_SHARING)) {
		return;
	}

	string udpPort;
	uint32_t partialCount = 0;
	string tth;
	string hubIpPort;
	string nick;
	PartsInfo partialInfo;

	for(auto& str: aCmd.getParameters()) {
		if(str.compare(0, 2, "U4") == 0) {
			udpPort = str.substr(2);
		} else if(str.compare(0, 2, "NI") == 0) {
			nick = str.substr(2);
		} else if(str.compare(0, 2, "HI") == 0) {
			hubIpPort = str.substr(2);
		} else if(str.compare(0, 2, "TR") == 0) {
			tth = str.substr(2);
		} else if(str.compare(0, 2, "PC") == 0) {
			partialCount = Util::toUInt32(str.substr(2))*2;
		} else if(str.compare(0, 2, "PI") == 0) {
			StringTokenizer<string> tok(str.substr(2), ',');
			for(auto& i: tok.getTokens()) {
				partialInfo.push_back((uint16_t)Util::toInt(i));
			}
		}
	}

	string url = ClientManager::getInstance()->findHub(hubIpPort, !from);
	if(!from || from == ClientManager::getInstance()->getMe()) {
		// for NMDC support
		
		if(nick.empty() || hubIpPort.empty()) {
			return;
		}
		
		from = ClientManager::getInstance()->findUser(nick, url);
		if(!from) {
			// Could happen if hub has multiple URLs / IPs
			from = ClientManager::getInstance()->findLegacyUser(nick);
			if(!from) {
				dcdebug("Search result from unknown user");
				return;
			}
		}

		ClientManager::getInstance()->setIPUser(from, remoteIp, udpPort);
	}

	if(partialInfo.size() != partialCount) {
		// what to do now ? just ignore partial search result :-/
		return;
	}

	PartsInfo outPartialInfo;
	QueueItem::PartialSource ps(from->isNMDC() ? ClientManager::getInstance()->getMyNick(url) : Util::emptyString, hubIpPort, remoteIp, udpPort);
	ps.setPartialInfo(partialInfo);

	QueueManager::getInstance()->handlePartialResult(HintedUser(from, url), TTHValue(tth), ps, outPartialInfo);
	
	if(Util::toInt(udpPort) > 0 && !outPartialInfo.empty()) {
		try {
			AdcCommand cmd = SearchManager::getInstance()->toPSR(false, ps.getMyNick(), hubIpPort, tth, outPartialInfo);
			ClientManager::getInstance()->sendUDP(cmd, from->getCID(), false, true, Util::emptyString, url);
		} catch(...) {
			dcdebug("Partial search caught error\n");
		}
	}

}

void SearchManager::respond(const AdcCommand& adc, OnlineUser& aUser, bool isUdpActive, const string& hubIpPort, ProfileToken aProfile) {
	auto isDirect = adc.getType() == 'D';
	string path = ADC_ROOT_STR, key;
	int maxResults = isUdpActive ? 10 : 5;

	bool replyDirect = false;
	if (isDirect) {
		adc.getParam("PA", 0, path);
		replyDirect = adc.hasFlag("RE", 0);

		string tmp;
		if (adc.getParam("MR", 0, tmp)) 
			maxResults = min(isUdpActive ? 20 : 10, Util::toInt(tmp));
	}

	SearchResultList results;
	SearchQuery srch(adc.getParameters(), maxResults);

	string token;
	adc.getParam("TO", 0, token);

	try {
		ShareManager::getInstance()->adcSearch(results, srch, aProfile, aUser.getUser()->getCID(), path, token.find("/as") != string::npos);
	} catch(const ShareException& e) {
		if (replyDirect) {
			//path not found (direct search)
			AdcCommand c(AdcCommand::SEV_FATAL, AdcCommand::ERROR_FILE_NOT_AVAILABLE, e.getError(), AdcCommand::TYPE_DIRECT);
			c.setTo(aUser.getIdentity().getSID());
			c.addParam("TO", token);

			aUser.getClient()->send(c);
		}
		return;
	}

	// TODO: don't send replies to passive users
	if(results.empty() && SETTING(USE_PARTIAL_SHARING) && aProfile != SP_HIDDEN) {
		string tth;
		if(!adc.getParam("TR", 0, tth))
			goto end;
			
		PartsInfo partialInfo;
		string bundle;
		bool reply = false, add = false;
		QueueManager::getInstance()->handlePartialSearch(aUser.getUser(), TTHValue(tth), partialInfo, bundle, reply, add);

		if (!partialInfo.empty()) {
			//LogManager::getInstance()->message("SEARCH RESPOND: PARTIALINFO NOT EMPTY");
			AdcCommand cmd = toPSR(isUdpActive, Util::emptyString, hubIpPort, tth, partialInfo);
			ClientManager::getInstance()->sendUDP(cmd, aUser.getUser()->getCID(), false, true, Util::emptyString, aUser.getHubUrl());
		}
		
		if (!bundle.empty()) {
			//LogManager::getInstance()->message("SEARCH RESPOND: BUNDLE NOT EMPTY");
			AdcCommand cmd = toPBD(hubIpPort, bundle, tth, reply, add);
			ClientManager::getInstance()->sendUDP(cmd, aUser.getUser()->getCID(), false, true, Util::emptyString, aUser.getHubUrl());
		}

		goto end;
	}


	adc.getParam("KY", 0, key);
	for(const auto& sr: results) {
		AdcCommand cmd = sr->toRES(AdcCommand::TYPE_UDP);
		if(!token.empty())
			cmd.addParam("TO", token);
		ClientManager::getInstance()->sendUDP(cmd, aUser.getUser()->getCID(), false, false, key, aUser.getHubUrl());
	}

end:
	if (replyDirect) {
		AdcCommand c(AdcCommand::SEV_SUCCESS, AdcCommand::SUCCESS, "Succeed", AdcCommand::TYPE_DIRECT);
		c.setTo(aUser.getIdentity().getSID());
		c.addParam("FC", adc.getFourCC());
		c.addParam("TO", token);
		c.addParam("RC", Util::toString(results.size()));

		aUser.getClient()->send(c);
	}
}

string SearchManager::getPartsString(const PartsInfo& partsInfo) const {
	string ret;

	for(auto i = partsInfo.begin(); i < partsInfo.end(); i+=2){
		ret += Util::toString(*i) + "," + Util::toString(*(i+1)) + ",";
	}

	return ret.substr(0, ret.size()-1);
}


AdcCommand SearchManager::toPSR(bool wantResponse, const string& myNick, const string& hubIpPort, const string& tth, const vector<uint16_t>& partialInfo) const {
	AdcCommand cmd(AdcCommand::CMD_PSR, AdcCommand::TYPE_UDP);
		
	if(!myNick.empty())
		cmd.addParam("NI", Text::utf8ToAcp(myNick));
		
	cmd.addParam("HI", hubIpPort);
	cmd.addParam("U4", wantResponse ? getPort() : "0");
	cmd.addParam("TR", tth);
	cmd.addParam("PC", Util::toString(partialInfo.size() / 2));
	cmd.addParam("PI", getPartsString(partialInfo));
	
	return cmd;
}

AdcCommand SearchManager::toPBD(const string& hubIpPort, const string& bundle, const string& aTTH, bool reply, bool add, bool notify) const {
	AdcCommand cmd(AdcCommand::CMD_PBD, AdcCommand::TYPE_UDP);

	cmd.addParam("HI", hubIpPort);
	cmd.addParam("BU", bundle);
	cmd.addParam("TH", aTTH);
	if (notify) {
		cmd.addParam("NO1");
	} else if (reply) {
		cmd.addParam("RE1");
	}

	if (add) {
		cmd.addParam("AD1");
	}
	return cmd;
}

void SearchManager::validateSearchTypeName(const string& name) const {
	if(name.empty() || (name.size() == 1 && name[0] >= '0' && name[0] <= '8')) {
		throw SearchTypeException("Invalid search type name"); // TODO: localize
	}

	for(int type = Search::TYPE_ANY; type != Search::TYPE_LAST; ++type) {
		if(getTypeStr(type) == name) {
			throw SearchTypeException("This search type already exists"); // TODO: localize
		}
	}
}

void SearchManager::setSearchTypeDefaults() {
	WLock l(cs);
	{
		searchTypes.clear();

		// for conveniency, the default search exts will be the same as the ones defined by SEGA.
		const auto& searchExts = AdcHub::getSearchExts();
		for(size_t i = 0, n = searchExts.size(); i < n; ++i)
			searchTypes[string(1, '1' + i)] = searchExts[i];
	}

	fire(SearchManagerListener::SearchTypesChanged());
}

void SearchManager::addSearchType(const string& name, const StringList& extensions, bool validated) {
	if(!validated) {
		validateSearchTypeName(name);
	}

	{
		WLock l(cs);
		if(searchTypes.find(name) != searchTypes.end()) {
			throw SearchTypeException("This search type already exists"); // TODO: localize
		}

		searchTypes[name] = extensions;
	}
	fire(SearchManagerListener::SearchTypesChanged());
}

void SearchManager::delSearchType(const string& name) {
	validateSearchTypeName(name);
	{
		WLock l(cs);
		searchTypes.erase(name);
	}
	fire(SearchManagerListener::SearchTypesChanged());
}

void SearchManager::renameSearchType(const string& oldName, const string& newName) {
	validateSearchTypeName(newName);
	StringList exts = getSearchType(oldName)->second;
	addSearchType(newName, exts, true);
	{
		WLock l(cs);
		searchTypes.erase(oldName);
	}
	fire(SearchManagerListener::SearchTypeRenamed(), oldName, newName);
}

void SearchManager::modSearchType(const string& name, const StringList& extensions) {
	{
		WLock l(cs);
		getSearchType(name)->second = extensions;
	}
	fire(SearchManagerListener::SearchTypesChanged());
}

const StringList& SearchManager::getExtensions(const string& name) {
	return getSearchType(name)->second;
}

SearchManager::SearchTypesIter SearchManager::getSearchType(const string& aName) {
	SearchTypesIter ret = searchTypes.find(aName);
	if(ret == searchTypes.end()) {
		throw SearchTypeException("No such search type"); // TODO: localize
	}
	return ret;
}

void SearchManager::getSearchType(int pos, Search::TypeModes& type_, StringList& extList_, string& name_) {
	// Any, directory or TTH
	if (pos < 4) {
		if (pos == 0) {
			name_ = SEARCH_TYPE_ANY;
			type_ = Search::TYPE_ANY;
		} else if (pos == 1) {
			name_ = SEARCH_TYPE_DIRECTORY;
			type_ = Search::TYPE_DIRECTORY;
		} else if (pos == 2) {
			name_ = SEARCH_TYPE_TTH;
			type_ = Search::TYPE_TTH;
		} else if (pos == 3) {
			name_ = SEARCH_TYPE_FILE;
			type_ = Search::TYPE_FILE;
		}
		return;
	}
	pos = pos-4;

	int counter = 0;
	for(auto& i: searchTypes) {
		if (counter++ == pos) {
			if(i.first.size() > 1 || i.first[0] < '1' || i.first[0] > '6') {
				// custom search type
				type_ = Search::TYPE_ANY;
			} else {
				type_ = static_cast<Search::TypeModes>(i.first[0] - '0');
			}

			name_ = i.first;
			extList_ = i.second;
			return;
		}
	}

	throw SearchTypeException("No such search type"); 
}

void SearchManager::getSearchType(const string& aName, Search::TypeModes& type_, StringList& extList_, bool aLock) {
	if (aName.empty())
		throw SearchTypeException("No such search type"); 

	// Any, directory or TTH
	if (aName[0] == SEARCH_TYPE_ANY[0] || aName[0] == SEARCH_TYPE_DIRECTORY[0] || aName[0] == SEARCH_TYPE_TTH[0]  || aName[0] == SEARCH_TYPE_FILE[0]) {
		type_ = static_cast<Search::TypeModes>(aName[0] - '0');
		return;
	}

	ConditionalRLock(cs, aLock);
	auto p = searchTypes.find(aName);
	if (p != searchTypes.end()) {
		extList_ = p->second;
		if(aName[0] < '1' || aName[0] > '6') {
			// custom search type
			type_ = Search::TYPE_ANY;
		} else {
			type_ = static_cast<Search::TypeModes>(aName[0] - '0');
		}
		return;
	}

	throw SearchTypeException("No such search type"); 
}

string SearchManager::getNameByExtension(const string& aExtension, bool aDefaultsOnly) const noexcept {
	auto extensionLower = Text::toLower(aExtension);

	RLock l(cs);
	for (const auto& type : searchTypes) {
		if (aDefaultsOnly && (type.first.size() > 1 || type.first[0] < '1' || type.first[0] > '6')) {
			continue;
		}

		auto i = boost::find(type.second, extensionLower);
		if (i != type.second.end()) {
			return type.first;
		}
	}

	return Util::emptyString;
}


void SearchManager::on(SettingsManagerListener::Save, SimpleXML& xml) noexcept {
	xml.addTag("SearchTypes");
	xml.stepIn();
	{
		for(auto& i: searchTypes) {
			xml.addTag("SearchType", Util::toString(";", i.second));
			xml.addChildAttrib("Id", i.first);
		}
	}
	xml.stepOut();
}

void SearchManager::on(SettingsManagerListener::Load, SimpleXML& xml) noexcept {
	xml.resetCurrentChild();
	if(xml.findChild("SearchTypes")) {
		searchTypes.clear();
		xml.stepIn();
		while(xml.findChild("SearchType")) {
			const string& extensions = xml.getChildData();
			if(extensions.empty()) {
				continue;
			}
			const string& name = xml.getChildAttrib("Id");
			if(name.empty()) {
				continue;
			}
			searchTypes[name] = StringTokenizer<string>(extensions, ';').getTokens();
		}
		xml.stepOut();
	}
}

} // namespace dcpp
