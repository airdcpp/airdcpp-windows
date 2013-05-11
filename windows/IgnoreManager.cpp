///////////////////////////////////////////////////////////////////////////////
//
//	Handles saving and loading of ignorelists
//
///////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "IgnoreManager.h"
#include "WinUtil.h"

#include "../client/stdinc.h"
#include "../client/Util.h"
#include "../client/Wildcards.h"
#include "../client/User.h"
#include "../client/ClientManager.h"


void IgnoreManager::load(SimpleXML& aXml) {
	if(aXml.findChild("IgnoreList")) {
		aXml.stepIn();
		while(aXml.findChild("User")) {	
			ignoredUsers.insert(Text::toT(aXml.getChildAttrib("Nick")));
		}
		aXml.stepOut();
	}
}

void IgnoreManager::save(SimpleXML& aXml) {
	aXml.addTag("IgnoreList");
	aXml.stepIn();

	for(TStringHashIterC i = ignoredUsers.begin(); i != ignoredUsers.end(); ++i) {
		aXml.addTag("User");
		aXml.addChildAttrib("Nick", Text::fromT(*i));
	}
	aXml.stepOut();
}


void IgnoreManager::storeIgnore(const UserPtr& user) {
	ignoredUsers.insert(WinUtil::getNicks(user->getCID()));
}

void IgnoreManager::removeIgnore(const UserPtr& user) {
	ignoredUsers.erase(WinUtil::getNicks(user->getCID()));
}


bool IgnoreManager::isIgnored(const string& aNick) {
	bool ret = false;

	if(ignoredUsers.count(Text::toT(aNick)))
		ret = true;

	if(SETTING(IGNORE_USE_REGEXP_OR_WC) && !ret) {
		Lock l(cs);
		for(TStringHashIterC i = ignoredUsers.begin(); i != ignoredUsers.end(); ++i) {
			const string tmp = Text::fromT(*i);
			if(strnicmp(tmp, "$Re:", 4) == 0) {
				if(tmp.length() > 4) {
					string str1 = tmp.substr(4);
					string str2 = aNick;
					try {
						boost::regex reg(str1);
						if(boost::regex_search(str2.begin(), str2.end(), reg)){
							ret = true;
							break;
						};
					} catch(...) {
					}
				}
			} else {
				ret = Wildcard::patternMatch(Text::toLower(aNick), Text::toLower(tmp), false);
				if(ret)
					break;
			}
		}
	}

	return ret;
}

// SettingsManagerListener
void IgnoreManager::on(SettingsManagerListener::Load, SimpleXML& aXml) {
	load(aXml);
}

void IgnoreManager::on(SettingsManagerListener::Save, SimpleXML& aXml) {
	save(aXml);
}