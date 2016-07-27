
#include "stdinc.h"
#include "DCPlusPlus.h"

#include "HttpConnection.h"
#include "RSSManager.h"
#include "LogManager.h"
#include "SearchManager.h"
#include "ScopedFunctor.h"
#include "AirUtil.h"

namespace dcpp {

#define CONFIG_NAME "RSS.xml"
#define CONFIG_DIR Util::PATH_USER_CONFIG

RSSManager::RSSManager() { }

RSSManager::~RSSManager()
{
	TimerManager::getInstance()->removeListener(this);
}

void RSSManager::clearRSSData(const RSSPtr& aFeed) {
	
	{
		Lock l(cs);
		aFeed->getFeedData().clear(); 
	}
	fire(RSSManagerListener::RSSDataCleared(), aFeed);

}

RSSPtr RSSManager::getFeedByCategory(const string& aCategory) {
	Lock l(cs);
	auto r = find_if(rssList.begin(), rssList.end(), [aCategory](const RSSPtr& a) { return aCategory == a->getCategory(); });
	if (r != rssList.end())
		return *r;

	return nullptr;
}

RSSPtr RSSManager::getFeedByUrl(const string& aUrl) {
	Lock l(cs);
	auto r = find_if(rssList.begin(), rssList.end(), [aUrl](const RSSPtr& a) { return aUrl == a->getUrl(); });
	if (r != rssList.end())
		return *r;

	return nullptr;
}

void RSSManager::downloadComplete(const string& aUrl) {
	auto feed = getFeedByUrl(aUrl);
	if (!feed)
		return;

	auto& conn = feed->rssDownload;
	ScopedFunctor([&conn] { conn.reset(); });

	if (conn->buf.empty()) {
		LogManager::getInstance()->message(conn->status, LogMessage::SEV_ERROR);
		return;
	}

	string tmpdata(conn->buf);
	string erh;
	string type;
	unsigned long i = 1;
	while (i) {
		unsigned int res = 0;
		sscanf(tmpdata.substr(i-1,4).c_str(), "%x", &res);
		if (res == 0){
			i=0;
		}else{
			if (tmpdata.substr(i-1,3).find("\x0d") != string::npos)
				erh += tmpdata.substr(i+3,res);
			if (tmpdata.substr(i-1,4).find("\x0d") != string::npos)
				erh += tmpdata.substr(i+4,res);
			else
				erh += tmpdata.substr(i+5,res);
			i += res+8;
		}
	}
	try {
		SimpleXML xml;
		xml.fromXML(tmpdata.c_str());
		if(xml.findChild("rss")) {
			xml.stepIn();
			if(xml.findChild("channel")) {	
				xml.stepIn();
				while(xml.findChild("item")) {
					xml.stepIn();
					bool newdata = false;
					string titletmp;
					string link;
					string date;
					if(xml.findChild("title")){
						titletmp = xml.getChildData();
						Lock l(cs);
						newdata = feed->getFeedData().find(titletmp) == feed->getFeedData().end();
					}
					if (xml.findChild("link")) {
						link = xml.getChildData();
						//temp fix for some urls
						if (strncmp(link.c_str(), "//", 2) == 0)
							link = "https:" + link;
					}
					if(xml.findChild("pubDate"))
						date = xml.getChildData();

					
					if(newdata) {
						RSSDataPtr data = new RSSData(titletmp, link, date, feed);
						matchAutosearch(feed, data);
						{
							Lock l(cs);
							feed->getFeedData().emplace(titletmp, data);
						}
						fire(RSSManagerListener::RSSDataAdded(), data);
					}

					titletmp.clear();
					link.clear();
					xml.stepOut();
				}
			xml.stepOut();
			}
		xml.stepOut();
		}
	} catch(const Exception& e) {
		LogManager::getInstance()->message(e.getError().c_str(), LogMessage::SEV_ERROR);
	}
}

void RSSManager::matchAutosearchFilters(const RSSPtr& aFeed) {
	if (aFeed) {
		Lock l(cs);
		for (auto data : aFeed->getFeedData() | map_values) {
				matchAutosearch(aFeed, data);
		}
	}
}

void RSSManager::matchAutosearch(const RSSPtr& aRss, const RSSDataPtr& aData) {
	
	if (AirUtil::stringRegexMatch(aRss->getAutoSearchFilter(), aData->getTitle())) {

		auto targetType = TargetUtil::TargetType::TARGET_PATH;
		AutoSearchManager::getInstance()->addAutoSearch(aData->getTitle(), 
			aRss->getDownloadTarget(), targetType, true, AutoSearch::RSS_DOWNLOAD, true);

	}
}

void RSSManager::updateFeedItem(const string& aUrl, const string& aCategory, const string& aAutoSearchFilter, const string& aDownloadTarget, int aUpdateInterval) {
	auto feed = getFeedByUrl(aUrl);
	if (feed) {
		{
			Lock l(cs);
			feed->setCategory(aCategory);
			feed->setAutoSearchFilter(aAutoSearchFilter);
			feed->setDownloadTarget(aDownloadTarget);
			feed->setUpdateInterval(aUpdateInterval);
		}
		fire(RSSManagerListener::RSSFeedChanged(), feed);
	} else {
		{
			Lock l(cs);
			feed = std::make_shared<RSS>(aUrl, aCategory, 0, aAutoSearchFilter, aDownloadTarget, aUpdateInterval);
			rssList.push_back(feed);
		}
		fire(RSSManagerListener::RSSFeedAdded(), feed);
	}
}

void RSSManager::removeFeedItem(const string& aUrl) {
	Lock l(cs);
	auto feed = getFeedByUrl(aUrl);
	rssList.erase(remove_if(rssList.begin(), rssList.end(), [&](const RSSPtr& a) { return aUrl == a->getUrl(); }), rssList.end());
	fire(RSSManagerListener::RSSFeedRemoved(), feed);
}

void RSSManager::downloadFeed(const RSSPtr& aRss) {
	if (!aRss)
		return;

	string url = aRss->getUrl();
	aRss->setLastUpdate(GET_TIME());
	aRss->rssDownload.reset(new HttpDownload(aRss->getUrl(),
		[this, url] { downloadComplete(url); }, false));

	fire(RSSManagerListener::RSSFeedUpdated(), aRss);
	LogManager::getInstance()->message("updating the " + aRss->getUrl(), LogMessage::SEV_INFO);
}

RSSPtr RSSManager::getUpdateItem() {
	for (auto i : rssList) {
		if (i->allowUpdate())
			return i;
	}
	return nullptr;
}


void RSSManager::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
	if (rssList.empty())
		return;

	if (nextUpdate < aTick) {
		Lock l(cs);
		downloadFeed(getUpdateItem());
		nextUpdate = GET_TICK() + 1 * 60 * 1000; //Minute between item updates for now, TODO: handle intervals smartly :)
	}
}

void RSSManager::load() {

	SimpleXML xml;
	SettingsManager::loadSettingFile(xml, CONFIG_DIR, CONFIG_NAME);
	if (xml.findChild("RSS")) {
		xml.stepIn();

		while (xml.findChild("Settings")) {
			auto feed = std::make_shared<RSS>(xml.getChildAttrib("Url"),
				xml.getChildAttrib("Categorie"),
				Util::toInt64(xml.getChildAttrib("LastUpdate")),
				xml.getChildAttrib("AutoSearchFilter"),
				xml.getChildAttrib("DownloadTarget"),
				xml.getIntChildAttrib("UpdateInterval"));
			xml.stepIn();
			loaddatabase(feed, xml);
			xml.stepOut();
			rssList.push_back(feed);
		}
		xml.stepOut();
	}
	
	TimerManager::getInstance()->addListener(this);
	nextUpdate = GET_TICK() + 10 * 1000; //start after 10 seconds
}

void RSSManager::loaddatabase(const RSSPtr& aFeed, SimpleXML& aXml) {
	if (aXml.findChild("Data")) {
		aXml.stepIn();
		while (aXml.findChild("item")) {

			auto rd = new RSSData(aXml.getChildAttrib("title"),
				aXml.getChildAttrib("link"),
				aXml.getChildAttrib("pubdate"),
				aFeed,
				Util::toInt64(aXml.getChildAttrib("dateadded")));

			aFeed->getFeedData().emplace(rd->getTitle(), rd);
		}
		aXml.stepOut();
	}
}

void RSSManager::save() {
	SimpleXML xml;
	xml.addTag("RSS");
	xml.stepIn();
	for (auto r : rssList) {
		xml.addTag("Settings");
		xml.addChildAttrib("Url", r->getUrl());
		xml.addChildAttrib("Categorie", r->getCategory());
		xml.addChildAttrib("LastUpdate", Util::toString(r->getLastUpdate()));
		xml.addChildAttrib("AutoSearchFilter", r->getAutoSearchFilter());
		xml.addChildAttrib("DownloadTarget", r->getDownloadTarget());
		xml.addChildAttrib("UpdateInterval", Util::toString(r->getUpdateInterval()));
		xml.stepIn();
		savedatabase(r, xml);
		xml.stepOut();
	}
	xml.stepOut();

	SettingsManager::saveSettingFile(xml, CONFIG_DIR, CONFIG_NAME);
}

void RSSManager::savedatabase(const RSSPtr& aFeed, SimpleXML& aXml) {
	aXml.addTag("Data");
	aXml.stepIn();
	for (auto r : aFeed->getFeedData() | map_values) {
		//Don't save more than 3 days old entries... Todo: setting?
		if ((r->getDateAdded() + 3 * 24 * 60 * 60) > GET_TIME()) {
			aXml.addTag("item");
			aXml.addChildAttrib("title", r->getTitle());
			aXml.addChildAttrib("link", r->getLink());
			aXml.addChildAttrib("pubdate", r->getPubDate());
			aXml.addChildAttrib("dateadded", Util::toString(r->getDateAdded()));
		}
	}
	aXml.stepOut();
}

}
