
#include "stdinc.h"
#include "DCPlusPlus.h"

#include "HttpConnection.h"
#include "RSSManager.h"
#include "LogManager.h"

namespace dcpp {

RSSManager::RSSManager(void) : startcheck(false)
{
}

RSSManager::~RSSManager(void)
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
			load(xml);
			xml.stepOut();
		}
	} catch(const Exception& e) {
		dcdebug("RSSManager::load: %s\n", e.getError().c_str());
	}
	
	TimerManager::getInstance()->addListener(this);
	updating = false;
}

void RSSManager::load(SimpleXML& aXml) {
	int i = 0;
	while(aXml.findChild("Settings")) {
		i = i + 1000;
		rsslist.push_back(
			RSS(aXml.getChildAttrib("Url"),
				GET_TICK() + (aXml.getLongLongChildAttrib("Update") * 1000) + 4000,
				aXml.getChildAttrib("Categorie"),
				aXml.getLongLongChildAttrib("Update"),
				aXml.getChildAttrib("AutoSearch")
				)
			);
	}
	aXml.stepOut();
	aXml.stepIn();
	while (aXml.findChild("AutoSearch")) {
		rssautosearch.push_back(
			AutoSearchRSS(aXml.getChildAttrib("Type"),
			aXml.getLongLongChildAttrib("Settings"),
			aXml.getChildAttrib("Folder"),
			aXml.getChildAttrib("Filter")
				)
			);
	}
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
							ShareManager::getInstance()->isDirShared(xml.getChildAttrib("title")),
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

void RSSManager::savedatabase() {
	try {
		SimpleXML xml;
		xml.addTag("RSSDatabase"); 
		xml.stepIn();
		for(RSSdata::Iter r = rssdata.begin(); r != rssdata.end(); ++r) {
			xml.addTag("item");
			xml.addChildAttrib("title", r->getTitle());
			xml.addChildAttrib("link", r->getLink());
			xml.addChildAttrib("date", r->getDate());
			xml.addChildAttrib("shared", r->getShared());
			xml.addChildAttrib("categorie", r->getCategorie());
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

void RSSManager::on(HttpConnectionListener::Complete, HttpConnection* conn, const string& status_, bool) noexcept{
		Lock l(cs);
	    string url = conn->getCurrentUrl();
	    conn->removeListener(this);
		string tmpdata(downBuf.c_str());
		downBuf.clear();
		string erh;
		string type;
		//if (c.){
		{
		unsigned long i = 1;
		while (i){
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
						string pubDate;
						if(xml.findChild("title")){
							titletmp = xml.getChildData();
							for(RSSdata::Iter r = rssdata.begin(); r != rssdata.end(); ++r) {
								if ( r->getTitle().find(titletmp) != string::npos ){
									newdata = true;
								}
							}
						}
						if(xml.findChild("link"))
							link = xml.getChildData();

						if(xml.findChild("pubDate"))
							pubDate = xml.getChildData();

						if(!newdata) {
							for (auto r = rsslist.begin(); r != rsslist.end(); ++r) {
								if (url == r->getRSSUrl()){
									url = r->getCategories();
									type = r->getType();
								}
							}
							
							rssdata.push_back(nextup(RSSdata(titletmp, link, pubDate, ShareManager::getInstance()->isDirShared(titletmp), url), type));
							
							

							fire(RSSManagerListener::RSSAdded(), titletmp, link, pubDate, url);
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
			
			LogManager::getInstance()->message(e.getError().c_str(), LogManager::LOG_ERROR);
		}
	//}else{
	//	LogManager::getInstance()->message(downBuf, LogManager::LOG_INFO);
	}
	
		updating = false;
}

void RSSManager::on(HttpConnectionListener::Failed, HttpConnection* conn, const string& status_) noexcept{
	updating = false;
	LogManager::getInstance()->message(status_, LogManager::LOG_ERROR);
	conn->removeListener(this);
}

RSSdata RSSManager::nextup(RSSdata url, string ftype) {
	try {

	using namespace boost;
	for (AutoSearchRSS::Iter m = rssautosearch.begin(); m != rssautosearch.end(); ++m) {
		if (m->getType() == ftype){			
			
			cmatch what;
    		regex expression(m->getFilter());

			
			if (regex_match(url.getTitle().c_str(), what, expression)) {
			
				AutoSearchPtr as = new AutoSearch;
				as->setSearchString(url.getTitle());
				
				(m->getSettings() & m->SETTINGS_MatchFullPath) ? as->setMatchFullPath(true) : as->setMatchFullPath(false);
				(m->getSettings() & m->SETTINGS_CheckAlreadyQueued) ? as->setCheckAlreadyQueued(true) : as->setCheckAlreadyQueued(false);
				(m->getSettings() & m->SETTINGS_CheckAlreadyShared) ? as->setCheckAlreadyShared(true) : as->setCheckAlreadyShared(false);
				(m->getSettings() & m->SETTINGS_Remove) ? as->setRemove(true) : as->setRemove(false);


				as->setExpireTime(0);
			//	as->setDisable(true);

				as->setAction(AutoSearch::ActionType::ACTION_DOWNLOAD);
				as->setTargetType(TargetUtil::TargetType::TARGET_PATH);
				as->setMethod(StringMatch::Method::EXACT);
				as->setFileType(SEARCH_TYPE_DIRECTORY);
				as->setTarget(Util::formatParams(m->getDownloadFolder(), ucParams).c_str());

				/*

				ParamMap ucParams;
				ucParams["file"] = "\"" + tempTarget + "\"";
				ucParams["dir"] = "\"" + Util::getFilePath(tempTarget) + "\"";

				
				as->setTarget(m->getDownloadFolder());
				as->setExcludedString(dlg.excludedWords);			
				as->setTarget(dlg.target);
				as->setMatcherString(dlg.matcherString);
				as->setUserMatcher(dlg.userMatch);

				if (as->getCurNumber() != dlg.curNumber)
				as->setLastIncFinish(0);

				as->startTime = dlg.startTime;
				as->endTime = dlg.endTime;
				as->searchDays = dlg.searchDays;

				as->setCurNumber(dlg.curNumber);
				as->setMaxNumber(dlg.maxNumber);
				as->setNumberLen(dlg.numberLen);
				as->setUseParams(dlg.useParams);
				*/

				AutoSearchManager::getInstance()->addAutoSearch(as, true);
			}
			break;
		}
	}	

	}
	catch (const Exception& e) {
		LogManager::getInstance()->message(e.getError().c_str(), LogManager::LOG_ERROR);
	}

	return url;
}


void RSSManager::on(TimerManagerListener::Second, uint64_t aTick) noexcept{
try {	
	Lock l(cs);
	int64_t to = 0;
	int ext = 0;

	for(RSS::Iter i = rsslist.begin(); i != rsslist.end(); ++i) {
		if ((i->getNextUpdate() < aTick) ){
			to = i->getUpdate() * 1000;
			i->setNextUpdate(aTick + to + (rsslist.size() * 4000));
			c.addListener(this);
			c.downloadFile(i->getRSSUrl());
			updating = true;
			LogManager::getInstance()->message("updating the " + i->getRSSUrl(), LogManager::LOG_INFO);

			for (RSS::Iter m = rsslist.begin(); m != rsslist.end(); ++m) {
				if (m != i && (m->getNextUpdate() < aTick)){
					ext++;
					to = m->getNextUpdate() + (4000 * ext);
					m->setNextUpdate(to);
				}
			}
			break;
		}
	}

} catch(const Exception& e) {
			
	LogManager::getInstance()->message(e.getError().c_str(), LogManager::LOG_ERROR);
	updating = false;
}
}

void RSSManager::check(){
	try {
/*	SimpleXML xml;
		xml.fromXML(File(Util::getPath(Util::PATH_USER_CONFIG) + "HashIndex.xml", File::READ, File::OPEN).read());
		this->startcheck = false;
		if(xml.findChild("HashStore")) {
				xml.stepIn();
			if(xml.findChild("Files")) {
				xml.stepIn();
				while (xml.findChild("File")){
					for(RSSdata::Iter r = rssdata.begin(); r != rssdata.end(); ++r) {
						if (!r->getShared() && (xml.getChildAttrib("Name").find(Text::toLower(r->getTitle()).c_str())) != string::npos ){
							fire(RSSManagerListener::RSSRemoved(),r->getTitle());
							r->setShared(true);
							r->setFolder(xml.getChildAttrib("Name"));
						}else if (r->getFolder().empty() && (xml.getChildAttrib("Name").find(Text::toLower(r->getTitle()).c_str())) != string::npos ){
							r->setFolder(xml.getChildAttrib("Name"));			
						}
					}
				}
				xml.stepOut();
			}
			xml.stepOut();
		}
*/
} catch(const FileException&) {
		// ...
	}
}

}
