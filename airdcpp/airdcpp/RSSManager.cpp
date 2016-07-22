
#include "stdinc.h"
#include "DCPlusPlus.h"

#include "HttpConnection.h"
#include "RSSManager.h"
#include "LogManager.h"
#include "SearchManager.h"

namespace dcpp {

RSSManager::RSSManager() { }

RSSManager::~RSSManager()
{
	c.removeListener(this);
	TimerManager::getInstance()->removeListener(this);
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
					RSS(xml.getChildAttrib("Url"),
						xml.getChildAttrib("Categorie"),
						Util::toInt64(xml.getChildAttrib("LastUpdate")),
						xml.getChildAttrib("AutoSearchFilter"),
						xml.getChildAttrib("DownloadTarget")
					)
				);
			}
			xml.stepOut();
		}
	} catch(const Exception& e) {
		dcdebug("RSSManager::load: %s\n", e.getError().c_str());
	}
	
	TimerManager::getInstance()->addListener(this);
	updating = false;
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
			xml.addChildAttrib("Url", r.getUrl());
			xml.addChildAttrib("Categorie", r.getCategories());
			xml.addChildAttrib("LastUpdate", Util::toString(r.getLastUpdate()));
			xml.addChildAttrib("AutoSearchFilter", r.getAutoSearchFilter());
			xml.addChildAttrib("DownloadTarget", r.getDownloadTarget());
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
		dcdebug("RSSManager::savedatabase: %s\n", e.getError().c_str());
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
				xml.addChildAttrib("categorie", r.getCategorie());
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

void RSSManager::on(HttpConnectionListener::Complete, HttpConnection* conn, const string&, bool) noexcept{
	Lock l(cs);
	string url = conn->getCurrentUrl();
	conn->removeListener(this);
	string tmpdata(downBuf.c_str());
	downBuf.clear();
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
						if(rssData.find(titletmp) == rssData.end())
							newdata = true;
					}
					if(xml.findChild("link"))
						link = "http:" + xml.getChildData();

					if(xml.findChild("pubDate"))
						date = xml.getChildData();

					if(newdata) {
						auto x = find_if(rssList.begin(), rssList.end(), [url](const RSS& r) {return url == r.getUrl(); });
						if(x != rssList.end()) {
							auto rss = *x;
							auto data = RSSdata(titletmp, link, date, rss.getCategories());
							matchAutosearch(rss, data);
							rssData.emplace(titletmp, data);

							fire(RSSManagerListener::RSSAdded(), data);
						}
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
	updating = false;
}

void RSSManager::on(HttpConnectionListener::Failed, HttpConnection* conn, const string& status_) noexcept{
	updating = false;
	LogManager::getInstance()->message(status_, LogMessage::SEV_ERROR);
	conn->removeListener(this);
}

void RSSManager::matchAutosearch(const RSS& aRss, const RSSdata& aData) {
	
	try {

		boost::regex reg(aRss.getAutoSearchFilter());

		if (regex_match(aData.getTitle().c_str(), reg)) {
			AutoSearchPtr as = new AutoSearch;
			as->setSearchString(aData.getTitle());
			as->setCheckAlreadyQueued(true);
			as->setCheckAlreadyShared(true);
			as->setRemove(true);
			as->setAction(AutoSearch::ActionType::ACTION_DOWNLOAD);
			as->setTargetType(TargetUtil::TargetType::TARGET_PATH);
			as->setMethod(StringMatch::Method::EXACT);
			as->setFileType(SEARCH_TYPE_DIRECTORY);
			as->setTarget(aRss.getDownloadTarget());
			AutoSearchManager::getInstance()->addAutoSearch(as, true);
		}

	} catch (const Exception& e) {
		LogManager::getInstance()->message(e.getError().c_str(), LogMessage::SEV_ERROR);
	}

}


void RSSManager::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
	if (rssList.empty() || updating)
		return;

	if (nextUpdate < aTick) {
		try {
			Lock l(cs);
			//Move item to the back of the list
			auto item = rssList.front();
			item.setLastUpdate(aTick);
			rssList.pop_front();
			rssList.push_back(item);

			c.addListener(this);
			c.downloadFile(item.getUrl());
			updating = true;
			LogManager::getInstance()->message("updating the " + item.getUrl(), LogMessage::SEV_INFO);
	
		} catch (const Exception& e) {
			LogManager::getInstance()->message(e.getError().c_str(), LogMessage::SEV_ERROR);
			updating = false;
		}

		nextUpdate = GET_TICK() + 1 * 60 * 1000; //Minute between item updates for now, TODO: handle intervals smartly :)
	}
}

}
