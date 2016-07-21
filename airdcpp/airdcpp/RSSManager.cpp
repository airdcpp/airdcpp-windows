
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
				rsslist.push_back(
					RSS(xml.getChildAttrib("Url"),
						xml.getChildAttrib("Categorie"),
						Util::toInt64(xml.getChildAttrib("LastUpdate"))
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
				rssdata.push_back(
					RSSdata(xml.getChildAttrib("title"),
							xml.getChildAttrib("link"),
							xml.getChildAttrib("date"),
							!ShareManager::getInstance()->getNmdcDirPaths(xml.getChildAttrib("title")).empty(),
							xml.getChildAttrib("categorie")
							)
					);
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
		for (auto r : rsslist) {
			xml.addTag("Settings");
			xml.addChildAttrib("Url", r.getUrl());
			xml.addChildAttrib("Categorie", r.getCategories());
			xml.addChildAttrib("LastUpdate", Util::toString(r.getLastUpdate()));
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
		for(auto r : rssdata) {
			xml.addTag("item");
			xml.addChildAttrib("title", r.getTitle());
			xml.addChildAttrib("link", r.getLink());
			xml.addChildAttrib("date", r.getDate());
			xml.addChildAttrib("shared", r.getShared());
			xml.addChildAttrib("categorie", r.getCategorie());
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
						for(auto r : rssdata) {
							if ( r.getTitle().find(titletmp) != string::npos ){
								newdata = true;
							}
						}
					}
					if(xml.findChild("link"))
						link = xml.getChildData();

					if(xml.findChild("pubDate"))
						date = xml.getChildData();

					if(!newdata) {

						for (auto rss : rsslist) {
							if (url == rss.getUrl()){
								url = rss.getCategories();
							}
						}
							
						auto data = RSSdata(titletmp, link, date, !ShareManager::getInstance()->getNmdcDirPaths(titletmp).empty(), url);
						//matchAutosearch(data);
						rssdata.push_back(data);
							
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
	updating = false;
}

void RSSManager::on(HttpConnectionListener::Failed, HttpConnection* conn, const string& status_) noexcept{
	updating = false;
	LogManager::getInstance()->message(status_, LogMessage::SEV_ERROR);
	conn->removeListener(this);
}
/*
void RSSManager::matchAutosearch(const RSSdata& aData) {
	
	try {

		for (auto m : rsslist) {

			boost::regex reg(m.getFilter());

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
				as->setTarget(m.getDownloadFolder());
				AutoSearchManager::getInstance()->addAutoSearch(as, true);
				break;
			}
		}

	} catch (const Exception& e) {
		LogManager::getInstance()->message(e.getError().c_str(), LogMessage::SEV_ERROR);
	}

}
*/

void RSSManager::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
	if (rsslist.empty() || updating)
		return;

	if (nextUpdate < aTick) {
		try {
			Lock l(cs);
			//Move item to the back of the list
			auto item = rsslist.front();
			item.setLastUpdate(aTick);
			rsslist.pop_front();
			rsslist.push_back(item);

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
