
#include "stdinc.h"
#include "DCPlusPlus.h"

#include "HttpConnection.h"
#include "RSSManager.h"
#include "LogManager.h"
#include "SearchManager.h"
#include "ScopedFunctor.h"
#include "AirUtil.h"

namespace dcpp {

RSSManager::RSSManager() { }

RSSManager::~RSSManager()
{
	TimerManager::getInstance()->removeListener(this);
}

void RSSManager::clearRSSData(const string& aCategory) {
	Lock l(cs);
	rssData.erase(boost::remove_if(rssData | map_values, [&](RSSdata& d) {
		if (compare(d.getCategory(), aCategory) == 0) {
			fire(RSSManagerListener::RSSRemoved(), d.getTitle());
			return true;
		}
		return false;
	}).base(), rssData.end());

}

void RSSManager::removeRSSData(const string& aTitle) {
	Lock l(cs);
	auto ret = rssData.erase(aTitle);
	if (ret > 0)
		fire(RSSManagerListener::RSSRemoved(), aTitle);
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


void RSSManager::load() {
	loaddatabase();
	try {
		SimpleXML xml;
		string tmpf = getConfigFile();
		xml.fromXML(File(tmpf, File::READ, File::OPEN).read());

		if(xml.findChild("RSS")) {
			xml.stepIn();
			
			while (xml.findChild("Settings")) {
				rssList.push_back(
					std::make_shared<RSS>(xml.getChildAttrib("Url"),
						xml.getChildAttrib("Categorie"),
						Util::toInt64(xml.getChildAttrib("LastUpdate")),
						xml.getChildAttrib("AutoSearchFilter"),
						xml.getChildAttrib("DownloadTarget"),
						xml.getIntChildAttrib("UpdateInterval")
					)
				);
			}
			xml.stepOut();
		}
	} catch(const Exception& e) {
		dcdebug("RSSManager::load: %s\n", e.getError().c_str());
	}
	
	TimerManager::getInstance()->addListener(this);
	nextUpdate = GET_TICK() + 10 * 1000; //start after 10 seconds
}

void RSSManager::loaddatabase() {
	try {
		SimpleXML xml;
		xml.fromXML(File(Util::getPath(Util::PATH_USER_CONFIG) + "RSSDatabase.xml", File::READ, File::OPEN).read());

		if(xml.findChild("RSSDatabase")) {
			xml.stepIn();
			while(xml.findChild("item")) {
				
				auto rd = RSSdata(xml.getChildAttrib("title"),
					xml.getChildAttrib("link"),
					xml.getChildAttrib("pubdate"),
					xml.getChildAttrib("categorie"),
					Util::toInt64(xml.getChildAttrib("dateadded")));

				rssData.emplace(rd.getTitle(), rd);
			}
			xml.stepOut();
		}
	} catch(const Exception& e) {
		dcdebug("RSSManager::loaddatabase: %s\n", e.getError().c_str());
	}

	//checkshared();
}

void RSSManager::save() {
	try {
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
		}
		xml.stepOut();

		string fname = getConfigFile();
		File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		f.write(SimpleXML::utf8Header);
		f.write(xml.toXML());
		f.close();
		File::deleteFile(fname);
		File::renameFile(fname + ".tmp", fname);
	}
	catch (const Exception& e) {
		dcdebug("RSSManager::save: %s\n", e.getError().c_str());
	}

	savedatabase();
}

void RSSManager::savedatabase() {
	try {
		SimpleXML xml;
		xml.addTag("RSSDatabase"); 
		xml.stepIn();
		for(auto r : rssData | map_values) {
			//Don't save more than 3 days old entries... Todo: setting?
			if ((r.getDateAdded() + 3 * 24 * 60 * 60) > GET_TIME()) {
				xml.addTag("item");
				xml.addChildAttrib("title", r.getTitle());
				xml.addChildAttrib("link", r.getLink());
				xml.addChildAttrib("pubdate", r.getPubDate());
				xml.addChildAttrib("categorie", r.getCategory());
				xml.addChildAttrib("dateadded", Util::toString(r.getDateAdded()));
			}
		}
		xml.stepOut();
		
		string fname = Util::getPath(Util::PATH_USER_CONFIG) + "RSSDatabase.xml";

		File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		f.write(SimpleXML::utf8Header);
		f.write(xml.toXML());
		f.close();
		File::deleteFile(fname);
		File::renameFile(fname + ".tmp", fname);
	} catch(const Exception& e) {
		dcdebug("RSSManager::savedatabase: %s\n", e.getError().c_str());
	}
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
						if(rssData.find(titletmp) == rssData.end())
							newdata = true;
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
							Lock l(cs);
							auto data = RSSdata(titletmp, link, date, feed->getCategory());
							matchAutosearch(feed, data);
							rssData.emplace(titletmp, data);

							fire(RSSManagerListener::RSSAdded(), data);
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

void RSSManager::matchAutosearchFilters(const string& aCategory) {
	auto feed = getFeedByCategory(aCategory);
	if (feed) {
		Lock l(cs);
		for (auto data : rssData | map_values) {
			if (data.getCategory() == aCategory)
				matchAutosearch(feed, data);
		}
	}
}

void RSSManager::matchAutosearch(const RSSPtr& aRss, const RSSdata& aData) {
	
	if (AirUtil::stringRegexMatch(aRss->getAutoSearchFilter(), aData.getTitle())) {
		AutoSearchPtr as = new AutoSearch;
		as->setSearchString(aData.getTitle());
		as->setCheckAlreadyQueued(true);
		as->setCheckAlreadyShared(true);
		as->setRemove(true);
		as->setAction(AutoSearch::ActionType::ACTION_DOWNLOAD);
		as->setTargetType(TargetUtil::TargetType::TARGET_PATH);
		as->setMethod(StringMatch::Method::EXACT);
		as->setFileType(SEARCH_TYPE_DIRECTORY);
		as->setTarget(aRss->getDownloadTarget());
		AutoSearchManager::getInstance()->addAutoSearch(as, true);
	}
}

void RSSManager::downloadFeed(const string& aCategory) {
	auto feed = getFeedByCategory(aCategory);
	if (feed)
		downloadFeed(feed);
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
	rssList.erase(remove_if(rssList.begin(), rssList.end(), [&](const RSSPtr& a) { return aUrl == a->getUrl(); }), rssList.end());
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

}
