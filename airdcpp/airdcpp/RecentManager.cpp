/*
* Copyright (C) 2011-2016 AirDC++ Project
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "stdinc.h"
#include "RecentManager.h"

#include "AirUtil.h"
#include "ClientManager.h"
#include "LogManager.h"
#include "RelevanceSearch.h"
#include "ResourceManager.h"
#include "ShareManager.h"
#include "SimpleXML.h"
#include "UserCommand.h"

namespace dcpp {

#define CONFIG_RECENTS_NAME "Recents.xml"
#define CONFIG_DIR Util::PATH_USER_CONFIG

using boost::range::find_if;

RecentManager::RecentManager() {

}

RecentManager::~RecentManager() {

}

void RecentManager::clearRecentHubs() noexcept {
	{
		WLock l(cs);
		recentHubs.clear();
	}

	saveRecentHubs();
}


void RecentManager::addRecentHub(const string& aUrl) noexcept {
	
	RecentHubEntryPtr r = getRecentHubEntry(aUrl);
	if (r)
		return;

	{
		WLock l(cs);
		r = new RecentHubEntry(aUrl);
		recentHubs.push_back(r);
	}

	fire(RecentManagerListener::RecentHubAdded(), r);
	saveRecentHubs();
}

void RecentManager::removeRecentHub(const string& aUrl) noexcept {
	RecentHubEntryPtr r = nullptr;
	{
		WLock l(cs);
		auto i = find_if(recentHubs, [&aUrl](const RecentHubEntryPtr& rhe) { return Util::stricmp(rhe->getUrl(), aUrl) == 0; });
		if (i == recentHubs.end()) {
			return;
		}
		r = *i;
		recentHubs.erase(i);
	}
	fire(RecentManagerListener::RecentHubRemoved(), r);
	saveRecentHubs();
}

void RecentManager::updateRecentHub(const ClientPtr& aClient) noexcept {
	RecentHubEntryPtr r = getRecentHubEntry(aClient->getHubUrl());
	if (!r)
		return;


	if (r) {
		r->setName(aClient->getHubName());
		r->setDescription(aClient->getHubDescription());
	}

	fire(RecentManagerListener::RecentHubUpdated(), r);
	saveRecentHubs();
}

void RecentManager::saveRecentHubs() const noexcept {
	SimpleXML xml;

	xml.addTag("Recents");
	xml.stepIn();

	xml.addTag("Hubs");
	xml.stepIn();

	{
		RLock l(cs);
		for (const auto& rhe : recentHubs) {
			xml.addTag("Hub");
			xml.addChildAttrib("Name", rhe->getName());
			xml.addChildAttrib("Description", rhe->getDescription());
			xml.addChildAttrib("Server", rhe->getUrl());
		}
	}

	xml.stepOut();
	xml.stepOut();

	SettingsManager::saveSettingFile(xml, CONFIG_DIR, CONFIG_RECENTS_NAME);
}

void RecentManager::load() noexcept {
	try {
		SimpleXML xml;
		SettingsManager::loadSettingFile(xml, CONFIG_DIR, CONFIG_RECENTS_NAME);
		if (xml.findChild("Recents")) {
			xml.stepIn();
			loadRecentHubs(xml);
			xml.stepOut();
		}
	} catch (const Exception& e) {
		LogManager::getInstance()->message(STRING_F(LOAD_FAILED_X, CONFIG_RECENTS_NAME % e.getError()), LogMessage::SEV_ERROR);
	}
}

void RecentManager::loadRecentHubs(SimpleXML& aXml) {
	aXml.resetCurrentChild();
	if (aXml.findChild("Hubs")) {
		aXml.stepIn();
		while (aXml.findChild("Hub")) {
			RecentHubEntryPtr e = new RecentHubEntry(aXml.getChildAttrib("Server"));
			e->setName(aXml.getChildAttrib("Name"));
			e->setDescription(aXml.getChildAttrib("Description"));
			recentHubs.push_back(e);
		}
		aXml.stepOut();
	}
}

RecentHubEntryPtr RecentManager::getRecentHubEntry(const string& aServer) const noexcept {
	RLock l(cs);
	auto i = find_if(recentHubs, [&aServer](const RecentHubEntryPtr& rhe) { return Util::stricmp(rhe->getUrl(), aServer) == 0; });
	if (i != recentHubs.end())
		return *i;

	return nullptr;
}

RecentHubEntryList RecentManager::searchRecentHubs(const string& aPattern, size_t aMaxResults) const noexcept {
	auto search = RelevanceSearch<RecentHubEntryPtr>(aPattern, [](const RecentHubEntryPtr& aHub) {
		return aHub->getName();
	});

	{
		RLock l(cs);
		for (const auto& hub : recentHubs) {
			search.match(hub);
		}
	}

	return search.getResults(aMaxResults);
}

} // namespace dcpp
