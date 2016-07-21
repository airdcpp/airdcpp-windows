
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
	RSS(){}
	RSS(const string& aUrl) throw() : url(aUrl) {}
	RSS(const string& aUrl, const string& aCategory, uint64_t aLastUpdate) noexcept : 
		url(aUrl), categories(aCategory), lastUpdate(aLastUpdate) {}
	virtual ~RSS(){};

	GETSET(string, url, Url);
	GETSET(string, categories, Categories);
	GETSET(uint64_t, lastUpdate, LastUpdate);

};

class RSSdata {
public:
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

	virtual void on(RSSAdded, const RSSdata&) noexcept { }
	virtual void on(RSSRemoved, string) noexcept { }
};


class RSSManager : public Speaker<RSSManagerListener>, public Singleton<RSSManager>
	, private HttpConnectionListener, private TimerManagerListener
{
public:
	friend class Singleton<RSSManager>;	
	RSSManager(void);
	~RSSManager(void);

	void load();
	void save();

	vector<RSSdata> getRssData(){
		return rssdata;
	}

	deque<RSS> getRss(){
		return rsslist;
	}

private:

	void loaddatabase(); 
	void savedatabase();

	atomic<bool> updating;
	uint64_t nextUpdate;
	
	//void matchAutosearch(const RSSdata& aData);

	deque<RSS> rsslist;
	vector<RSSdata> rssdata;
	
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