
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Util.h"
#include "Singleton.h"
#include "Speaker.h"
#include "SimpleXML.h"
#include "HttpConnection.h"
#include "TimerManager.h"
#include "ShareManager.h"
#include "AutoSearchManager.h"
#include "TargetUtil.h"

namespace dcpp {

class RSS {
public:
	typedef vector<RSS> List;
	typedef List::iterator Iter;
	RSS(){}
	RSS(string url) throw() : rssurl(url), nextupdate(GET_TICK() + 10000), update(9000) {}
	RSS(string url, uint32_t time, string cat, uint32_t fupdate, string ftype ) throw() : rssurl(url), nextupdate(time), categories(cat), update(fupdate), type(ftype) {}
	virtual ~RSS(){};

	GETSET(string, rssurl, RSSUrl);
	GETSET(uint32_t, nextupdate, NextUpdate);
	GETSET(string, categories, Categories);
	GETSET(uint32_t, update, Update);
	GETSET(string, type, Type);
};

class AutoSearchRSS {
public:
	typedef vector<AutoSearchRSS> List;
	typedef List::iterator Iter;
	AutoSearchRSS(){}
	AutoSearchRSS(string ftype, uint32_t fsettings, string ffolder, string ffilter, uint32_t faction, uint32_t ftargettype) throw() : type(ftype), settings(fsettings), downloadfolder(ffolder), filter(ffilter), action(faction), targettype(ftargettype){}
	virtual ~AutoSearchRSS(){};

	GETSET(string, type, Type);
	GETSET(uint32_t, settings, Settings);
	GETSET(string, downloadfolder, DownloadFolder);
	GETSET(string, filter, Filter);
	GETSET(uint32_t, action, Action);
	GETSET(uint32_t, targettype, TargetType);
	GETSET(uint32_t, method, Method);

	friend class AutoSearchRSSManager;
	enum SettingsFlags {
		SETTINGS_MatchFullPath = 0x01,
		SETTINGS_CheckAlreadyQueued = 0x02,
		SETTINGS_CheckAlreadyShared = 0x04,
		SETTINGS_Remove = 0x08
	};
};

class RSSdata {
public:
	typedef vector<RSSdata> List;
	typedef List::iterator Iter;
	RSSdata() : shared(false) {}
	RSSdata(string aTitle, string aLink) throw() : title(aTitle), link(aLink), date(Util::emptyString), shared(false), folder(Util::emptyString) {
	}
	RSSdata(string aTitle, string aLink, string aDate, bool aShared, string aCategorie) throw() : title(aTitle), link(aLink), date(aDate), shared(aShared), folder(Util::emptyString), categorie(aCategorie) {
	}
	virtual ~RSSdata(){};
	
	GETSET(string, title, Title);
	GETSET(string, link, Link);
	GETSET(string, date, Date);
	GETSET(string, folder, Folder);
	GETSET(bool, shared, Shared);
	GETSET(string, categorie, Categorie);


};

class RSSManagerListener {
public:
	virtual ~RSSManagerListener() { }
	template<int I>	struct X { enum { TYPE = I }; };

	typedef X<0> RSSAdded;
	typedef X<1> RSSRemoved;

	virtual void on(RSSAdded, string, string, string, string) throw() { }
	virtual void on(RSSRemoved, string) throw() { }
};


class RSSManager : public Speaker<RSSManagerListener>, public Singleton<RSSManager>
	, private HttpConnectionListener, private TimerManagerListener
{
public:
	friend class Singleton<RSSManager>;	
	RSSManager(void);
	~RSSManager(void);

	void load(); 
	void savedatabase(); 

	RSSdata::List getRssData(){
		return rssdata;
	}

	RSS::List getRss(){
		return rsslist;
	}

	void checkshared(){
		startcheck = true;
	}

private:

	void loaddatabase(); 
	void load(SimpleXML& aXml); 
	bool startcheck;
	bool updating;
	void check();
	RSSdata nextup(RSSdata url,string type);
	
	ParamMap ucParams;
	RSS::List rsslist;
	RSSdata::List rssdata;
	AutoSearchRSS::List rssautosearch;
	mutable CriticalSection cs;
	string getConfigFile() { return Util::getPath(Util::PATH_USER_CONFIG) + "RSS.xml"; }

	HttpConnection c;

	// HttpConnectionListener
	void on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const uint8_t* buf, size_t len) noexcept{
		downBuf.append((char*)buf, len);
	}

	void on(HttpConnectionListener::Complete, HttpConnection* conn, const string&, bool /*fromCoral*/) noexcept;
	void on(HttpConnectionListener::Failed, HttpConnection* conn, const string& aLine) noexcept;
	// TimerManagerListener
	void on(TimerManagerListener::Second, uint64_t tick) noexcept;

	string downBuf;
	string tmpdataq;
};

}