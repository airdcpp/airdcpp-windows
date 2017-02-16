/*
* Copyright (C) 2011-2017 AirDC++ Project
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

#include "IgnoreManager.h"

#include "ClientManager.h"
#include "FavoriteManager.h"
#include "LogManager.h"
#include "PrivateChatManager.h"

#include "Message.h"
#include "Util.h"

#define CONFIG_DIR Util::PATH_USER_CONFIG
#define CONFIG_NAME "IgnoredUsers.xml"

namespace dcpp {

IgnoreManager::IgnoreManager() noexcept {
	SettingsManager::getInstance()->addListener(this);
}

IgnoreManager::~IgnoreManager() noexcept {
	SettingsManager::getInstance()->removeListener(this);
}

// SettingsManagerListener
void IgnoreManager::on(SettingsManagerListener::Load, SimpleXML& aXml) noexcept {
	load(aXml);
}

void IgnoreManager::on(SettingsManagerListener::Save, SimpleXML& aXml) noexcept {
	save(aXml);
}

IgnoreManager::IgnoreMap IgnoreManager::getIgnoredUsers() const noexcept {
	RLock l(cs);
	return ignoredUsers;
}

bool IgnoreManager::storeIgnore(const UserPtr& aUser) noexcept {
	if (aUser->isIgnored()) {
		return false;
	}

	{
		WLock l(cs);
		ignoredUsers.emplace(aUser, 0);
	}

	aUser->setFlag(User::IGNORED);
	dirty = true;

	fire(IgnoreManagerListener::IgnoreAdded(), aUser);

	{
		auto chat = PrivateChatManager::getInstance()->getChat(aUser);
		if (chat) {
			chat->checkIgnored();
		}
	}

	ClientManager::getInstance()->userUpdated(aUser);
	return true;
}

bool IgnoreManager::removeIgnore(const UserPtr& aUser) noexcept {
	{
		WLock l(cs);
		auto i = ignoredUsers.find(aUser);
		if (i == ignoredUsers.end()) {
			return false;
		}

		ignoredUsers.erase(i);
	}

	aUser->unsetFlag(User::IGNORED);
	dirty = true;

	fire(IgnoreManagerListener::IgnoreRemoved(), aUser);
	ClientManager::getInstance()->userUpdated(aUser);
	return true;
}

bool IgnoreManager::checkIgnored(const OnlineUserPtr& aUser) noexcept {
	if (!aUser) {
		return false;
	}

	RLock l(cs);
	auto i = ignoredUsers.find(aUser->getUser());
	if (i == ignoredUsers.end()) {
		return false;
	}

	// Increase the ignored message count
	auto ignored = (aUser->getClient() && aUser->getClient()->isOp()) || !aUser->getIdentity().isOp() || aUser->getIdentity().isBot();
	if (ignored) {
		i->second++;
	}

	return ignored;
}

bool IgnoreManager::isIgnoredOrFiltered(const ChatMessagePtr& msg, Client* aClient, bool PM) {
	const auto& fromIdentity = msg->getFrom()->getIdentity();

	auto logIgnored = [&](bool filter) -> void {
		if (SETTING(LOG_IGNORED)) {
			string tmp;
			if (PM) {
				tmp = filter ? STRING(PM_MESSAGE_FILTERED) : STRING(PM_MESSAGE_IGNORED);
			}
			else {
				string hub = "[" + ((aClient && !aClient->getHubName().empty()) ?
					(aClient->getHubName().size() > 50 ? (aClient->getHubName().substr(0, 50) + "...") : aClient->getHubName()) : aClient->getHubUrl()) + "] ";
				tmp = (filter ? STRING(MC_MESSAGE_FILTERED) : STRING(MC_MESSAGE_IGNORED)) + hub;
			}
			tmp += "<" + fromIdentity.getNick() + "> " + msg->getText();
			LogManager::getInstance()->message(tmp, LogMessage::SEV_INFO);
		}
	};

	// replyTo can be different if the message is received via a chat room (it should be possible to ignore those as well)
	if (checkIgnored(msg->getFrom()) || checkIgnored(msg->getReplyTo())) {
		return true;
	}

	if (isChatFiltered(fromIdentity.getNick(), msg->getText(), PM ? ChatFilterItem::PM : ChatFilterItem::MC)) {
		logIgnored(true);
		return true;
	}

	return false;
}

bool IgnoreManager::isChatFiltered(const string& aNick, const string& aText, ChatFilterItem::Context aContext) {
	RLock l(cs);
	for (auto& i : ChatFilterItems) {
		if (i.match(aNick, aText, aContext))
			return true;
	}
	return false;
}
void IgnoreManager::load(SimpleXML& aXml) {
	aXml.resetCurrentChild();
	if (aXml.findChild("ChatFilterItems")) {
		aXml.stepIn();
		while (aXml.findChild("ChatFilterItem")) {
			WLock l(cs);
			ChatFilterItems.push_back(ChatFilterItem(aXml.getChildAttrib("Nick"), aXml.getChildAttrib("Text"),
				(StringMatch::Method)aXml.getIntChildAttrib("NickMethod"), (StringMatch::Method)aXml.getIntChildAttrib("TextMethod"),
				aXml.getBoolChildAttrib("MC"), aXml.getBoolChildAttrib("PM"), aXml.getBoolChildAttrib("Enabled")));
		}
		aXml.stepOut();
	}
	loadUsers();
}

void IgnoreManager::save(SimpleXML& aXml) {
	aXml.addTag("ChatFilterItems");
	aXml.stepIn();
	{
		RLock l(cs);
		for (const auto& i : ChatFilterItems) {
			aXml.addTag("ChatFilterItem");
			aXml.addChildAttrib("Nick", i.getNickPattern());
			aXml.addChildAttrib("NickMethod", i.getNickMethod());
			aXml.addChildAttrib("Text", i.getTextPattern());
			aXml.addChildAttrib("TextMethod", i.getTextMethod());
			aXml.addChildAttrib("MC", i.matchMainchat);
			aXml.addChildAttrib("PM", i.matchPM);
			aXml.addChildAttrib("Enabled", i.getEnabled());
		}
	}
	aXml.stepOut();

	if (dirty)
		saveUsers();
}

void IgnoreManager::saveUsers() {
	SimpleXML xml;

	xml.addTag("Ignored");
	xml.stepIn();

	xml.addTag("Users");
	xml.stepIn();

	{
		RLock l(cs);
		for (const auto& u : ignoredUsers) {
			xml.addTag("User");
			xml.addChildAttrib("CID", u.first->getCID().toBase32());
			xml.addChildAttrib("IgnoredMessages", u.second);

			FavoriteManager::getInstance()->addSavedUser(u.first);
		}
	}

	xml.stepOut();
	xml.stepOut();

	SettingsManager::saveSettingFile(xml, CONFIG_DIR, CONFIG_NAME);
}

void IgnoreManager::loadUsers() {
	try {
		SimpleXML xml;
		SettingsManager::loadSettingFile(xml, CONFIG_DIR, CONFIG_NAME);
		if (xml.findChild("Ignored")) {
			xml.stepIn();
			xml.resetCurrentChild();
			if (xml.findChild("Users")) {
				xml.stepIn();
				while (xml.findChild("User")) {
					auto user = ClientManager::getInstance()->getUser(CID(xml.getChildAttrib("CID")));
					if (!user)
						continue;

					WLock l(cs);
					ignoredUsers.emplace(user, xml.getIntChildAttrib("IgnoredMessages"));
					user->setFlag(User::IGNORED);
				}
				xml.stepOut();
			}
			xml.stepOut();
		}
	} catch (const Exception& e) {
		LogManager::getInstance()->message(STRING_F(LOAD_FAILED_X, CONFIG_NAME % e.getError()), LogMessage::SEV_ERROR);
	}
}

}