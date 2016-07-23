
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Util.h"
#include "Singleton.h"
#include "Speaker.h"
#include "SimpleXML.h"
#include "HttpDownload.h"
#include "TimerManager.h"
#include "ShareManager.h"
#include "AutoSearchManager.h"
#include "TargetUtil.h"

namespace dcpp {

class RSS : private boost::noncopyable {
public:
	RSS()
	{
		rssDownload.reset();
	}
	RSS(const string& aUrl, const string& aCategory, time_t aLastUpdate) noexcept : 
		url(aUrl), categories(aCategory), lastUpdate(aLastUpdate)
	{
		rssDownload.reset();
	}

	RSS(const string& aUrl, const string& aCategory, time_t aLastUpdate, const string& aAutoSearchFilter, const string& aDownloadTarget) noexcept :
		url(aUrl), categories(aCategory), lastUpdate(aLastUpdate), autoSearchFilter(aAutoSearchFilter), downloadTarget(aDownloadTarget) 
	{
		rssDownload.reset();
	}

	~RSS(){};

	GETSET(string, url, Url);
	GETSET(string, categories, Categories);
	GETSET(time_t, lastUpdate, LastUpdate);

	GETSET(string, autoSearchFilter, AutoSearchFilter);
	GETSET(string, downloadTarget, DownloadTarget);

	bool operator==(const RSSPtr& rhs) const { return url == rhs->getUrl(); }

	unique_ptr<HttpDownload> rssDownload;

	bool allowUpdate() {
		return (getLastUpdate() + 15 * 60) < GET_TIME();
	}

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


class RSSManager : public Speaker<RSSManagerListener>, public Singleton<RSSManager>, private TimerManagerListener
{
public:
	friend class Singleton<RSSManager>;	
	RSSManager(void);
	~RSSManager(void);

	void load();
	void save();

	unordered_map<string, RSSdata> getRssData(){
		Lock l(cs);
		return rssData;
	}

	deque<RSSPtr> getRss(){
		Lock l(cs);
		return rssList;
	}

	void updateItem(const string& aUrl, const string& aCategory, const string& aAutoSearchFilter, const string& aDownloadTarget) {
		Lock l(cs);
		auto r = find_if(rssList.begin(), rssList.end(), [aUrl](const RSSPtr& a) { return aUrl == a->getUrl(); });
		if (r != rssList.end()) {
			auto rss = *r;
			rss->setCategories(aCategory);
			rss->setAutoSearchFilter(aAutoSearchFilter);
			rss->setDownloadTarget(aDownloadTarget);
		} else {
			rssList.push_back(
				std::make_shared<RSS>(aUrl, aCategory, 0, aAutoSearchFilter, aDownloadTarget));
		}
	}

	void removeItem(const string& aUrl) {
		Lock l(cs);
		rssList.erase(remove_if(rssList.begin(), rssList.end(), [&](const RSSPtr& a) { return aUrl == a->getUrl(); }), rssList.end());
	}

private:

	void loaddatabase(); 
	void savedatabase();

	uint64_t nextUpdate;

	RSSPtr getUpdateItem();
	void downloadFeed(const RSSPtr& aRss);
	
	void matchAutosearch(const RSSPtr& aRss, const RSSdata& aData);

	deque<RSSPtr> rssList;
	unordered_map<string, RSSdata> rssData;
	
	mutable CriticalSection cs;

	void downloadComplete(const string& aUrl);
	string getConfigFile() { return Util::getPath(Util::PATH_USER_CONFIG) + "RSS.xml"; }
	// TimerManagerListener
	void on(TimerManagerListener::Second, uint64_t tick) noexcept;

};

}