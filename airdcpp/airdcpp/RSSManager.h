
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
	RSS(const string& aUrl, const string& aCategory, uint64_t aLastUpdate) noexcept : 
		url(aUrl), categories(aCategory), lastUpdate(aLastUpdate) {}

	RSS(const string& aUrl, const string& aCategory, uint64_t aLastUpdate, const string& aAutoSearchFilter, const string& aDownloadTarget) noexcept :
		url(aUrl), categories(aCategory), lastUpdate(aLastUpdate), autoSearchFilter(aAutoSearchFilter), downloadTarget(aDownloadTarget) {}

	virtual ~RSS(){};

	GETSET(string, url, Url);
	GETSET(string, categories, Categories);
	GETSET(uint64_t, lastUpdate, LastUpdate);

	GETSET(string, autoSearchFilter, AutoSearchFilter);
	GETSET(string, downloadTarget, DownloadTarget);

};

class RSSdata {
public:
	RSSdata(string aTitle, string aLink) noexcept : title(aTitle), link(aLink), pubDate(Util::emptyString) {
	}
	RSSdata(string aTitle, string aLink, string aPubDate, string aCategorie, time_t aDateAdded = GET_TIME()) noexcept :
		title(aTitle), link(aLink), pubDate(aPubDate), categorie(aCategorie), dateAdded(aDateAdded)  {
	}
	virtual ~RSSdata(){};
	
	GETSET(string, title, Title);
	GETSET(string, link, Link);
	GETSET(string, pubDate, PubDate);
	GETSET(string, categorie, Categorie);
	GETSET(time_t, dateAdded, DateAdded); //For prune old entries in database...


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

	unordered_map<string, RSSdata> getRssData(){
		return rssData;
	}

	deque<RSS> getRss(){
		return rssList;
	}

private:

	void loaddatabase(); 
	void savedatabase();

	atomic<bool> updating;
	uint64_t nextUpdate;
	
	void matchAutosearch(const RSSdata& aData);

	deque<RSS> rssList;
	unordered_map<string, RSSdata> rssData;
	
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