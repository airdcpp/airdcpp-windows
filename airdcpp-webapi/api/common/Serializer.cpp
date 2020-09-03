/*
* Copyright (C) 2011-2019 AirDC++ Project
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

#include <api/common/Serializer.h>
#include <api/common/Format.h>

#include <api/OnlineUserUtils.h>

#include <web-server/WebUser.h>

#include <airdcpp/AirUtil.h>
#include <airdcpp/Bundle.h>
#include <airdcpp/Client.h>
#include <airdcpp/ClientManager.h>
#include <airdcpp/Message.h>
#include <airdcpp/DirectoryListing.h>
#include <airdcpp/DirectoryListingManager.h>
#include <airdcpp/GeoManager.h>
#include <airdcpp/OnlineUser.h>
#include <airdcpp/QueueItem.h>
#include <airdcpp/QueueManager.h>
#include <airdcpp/SearchManager.h>
#include <airdcpp/ShareManager.h>

namespace webserver {
	StringSet Serializer::getUserFlags(const UserPtr& aUser) noexcept {
		StringSet ret;
		if (aUser->isSet(User::BOT)) {
			ret.insert("bot");
		}

		if (aUser->isSet(User::FAVORITE)) {
			ret.insert("favorite");
		}

		if (aUser->isSet(User::IGNORED)) {
			ret.insert("ignored");
		}

		if (aUser == ClientManager::getInstance()->getMe()) {
			ret.insert("self");
		}

		if (aUser->isSet(User::NMDC)) {
			ret.insert("nmdc");
		}

		if (aUser->isSet(User::ASCH)) {
			ret.insert("asch");
		}

		if (!aUser->isOnline()) {
			ret.insert("offline");
		}

		if (aUser->isSet(User::CCPM)) {
			ret.insert("ccpm");
		}

		return ret;
	}

	StringSet Serializer::getOnlineUserFlags(const OnlineUserPtr& aUser) noexcept {
		auto flags = getUserFlags(aUser->getUser());
		appendOnlineUserFlags(aUser, flags);
		return flags;
	}

	void Serializer::appendOnlineUserFlags(const OnlineUserPtr& aUser, StringSet& flags_) noexcept {
		if (aUser->getIdentity().isAway()) {
			flags_.insert("away");
		}

		if (aUser->getIdentity().isOp()) {
			flags_.insert("op");
		}

		if (aUser->isHidden()) {
			flags_.insert("hidden");
		}

		auto cm = aUser->getIdentity().getConnectMode();
		if (!aUser->getUser()->isNMDC() && (cm == Identity::MODE_NOCONNECT_PASSIVE || cm == Identity::MODE_NOCONNECT_IP || cm == Identity::MODE_UNDEFINED)) {
			flags_.insert("noconnect");
		} else if (!aUser->getIdentity().isTcpActive(aUser->getClient())) {
			flags_.insert("passive");
		}
	}

	json Serializer::serializeUser(const UserPtr& aUser) noexcept {
		return {
			{ "id", aUser->getCID().toBase32() },
			{ "cid", aUser->getCID().toBase32() },
			{ "nicks", Util::listToString(ClientManager::getInstance()->getNicks(aUser->getCID())) },
			{ "hub_names", Util::listToString(ClientManager::getInstance()->getHubNames(aUser->getCID())) },
			{ "hub_urls", ClientManager::getInstance()->getHubUrls(aUser->getCID()) },
			{ "flags", getUserFlags(aUser) }
		};
	}

	json Serializer::serializeHintedUser(const HintedUser& aUser) noexcept {
		auto flags = getUserFlags(aUser);
		if (aUser.user->isOnline()) {
			auto user = ClientManager::getInstance()->findOnlineUser(aUser);
			if (user) {
				appendOnlineUserFlags(user, flags);
			}
		}

		return {
			{ "cid", aUser.user->getCID().toBase32() },
			{ "nicks", ClientManager::getInstance()->getFormatedNicks(aUser) },
			{ "hub_url", aUser.hint },
			{ "hub_names", ClientManager::getInstance()->getFormatedHubNames(aUser) },
			{ "hub_urls", ClientManager::getInstance()->getHubUrls(aUser.user->getCID()) },
			{ "flags", flags }
		};
	}

	json Serializer::serializeMessage(const Message& aMessage) noexcept {
		if (aMessage.type == Message::TYPE_CHAT) {
			return{
				{ "chat_message", serializeChatMessage(aMessage.chatMessage) }
			};
		}

		return{
			{ "log_message", serializeLogMessage(aMessage.logMessage) }
		};
	}

	json Serializer::serializeShareProfileSimple(ProfileToken aProfile) noexcept {
		auto sp = ShareManager::getInstance()->getShareProfile(aProfile);
		if (!sp) {
			// Shouldn't happen
			return nullptr;
		}

		return {
			{ "id", sp->getToken() },
			{ "str", sp->getPlainName() },
		};
	}

	json Serializer::serializeEncryption(const string& aInfo, bool aIsTrusted) noexcept {
		if (aInfo.empty()) {
			return nullptr;
		}

		return {
			{ "str", aInfo },
			{ "trusted", aIsTrusted },
		};
	}

	json Serializer::serializeChatMessage(const ChatMessagePtr& aMessage) noexcept {
		json ret = {
			{ "id", aMessage->getId()},
			{ "text", aMessage->getText() },
			{ "from", serializeOnlineUser(aMessage->getFrom()) },
			{ "time", aMessage->getTime() },
			{ "is_read", aMessage->getRead() },
			{ "third_person", aMessage->getThirdPerson() },
		};

		if (aMessage->getTo()) {
			ret["to"] = serializeOnlineUser(aMessage->getTo());
		}

		if (aMessage->getReplyTo()) {
			ret["reply_to"] = serializeOnlineUser(aMessage->getReplyTo());
		}

		return ret;
	}

	string Serializer::getSeverity(LogMessage::Severity aSeverity) noexcept {
		switch (aSeverity) {
			case LogMessage::SEV_NOTIFY: return "notify";
			case LogMessage::SEV_INFO: return "info";
			case LogMessage::SEV_WARNING: return "warning";
			case LogMessage::SEV_ERROR: return "error";
			default: return Util::emptyString;
		}
	}

	json Serializer::serializeLogMessage(const LogMessagePtr& aMessageData) noexcept {
		return{
			{ "id", aMessageData->getId() },
			{ "text", aMessageData->getText() },
			{ "time", aMessageData->getTime() },
			{ "severity", getSeverity(aMessageData->getSeverity()) },
			{ "is_read", aMessageData->getRead() }
		};
	}

	json Serializer::serializeCacheInfo(const MessageCache& aCache, const UnreadSerializerF& unreadF) noexcept {
		return {
			{ "total", aCache.size() },
			{ "unread", unreadF(aCache) },
		};
	}

	json Serializer::serializeUnreadLog(const MessageCache& aCache) noexcept {
		return{
			{ "info", aCache.countUnreadLogMessages(LogMessage::SEV_INFO) },
			{ "warning", aCache.countUnreadLogMessages(LogMessage::SEV_WARNING) },
			{ "error", aCache.countUnreadLogMessages(LogMessage::SEV_ERROR) },
		};
	}

	json Serializer::serializeUnreadChat(const MessageCache& aCache) noexcept {
		MessageCache::ChatMessageFilterF isBot = [](const ChatMessagePtr& aMessage) {
			return aMessage->getFrom()->getIdentity().isBot() || aMessage->getFrom()->getIdentity().isHub();
		};

		return {
			{ "user", aCache.countUnreadChatMessages(std::not1(isBot)) },
			{ "bot", aCache.countUnreadChatMessages(isBot) },
			{ "status", aCache.countUnreadLogMessages(LogMessage::SEV_LAST) },
		};
	}

	json Serializer::serializeOnlineUser(const OnlineUserPtr& aUser) noexcept {
		return serializeItem(aUser, OnlineUserUtils::propertyHandler);
	}

	std::string Serializer::getFileTypeId(const string& aId) noexcept {
		if (aId.length() != 1) {
			return aId;
		}

		switch (aId[0]) {
			case '1': return "audio";
			case '2': return "compressed";
			case '3': return "document";
			case '4': return "executable";
			case '5': return "picture";
			case '6': return "video";
			default: return aId;
		}
	}

	json Serializer::serializeFileType(const string& aPath) noexcept {
		auto ext = Util::formatFileType(aPath);
		auto typeName = getFileTypeId(SearchManager::getInstance()->getTypeIdByExtension(ext, true));

		return{
			{ "id", "file" },
			{ "content_type", typeName },
			{ "str", ext }
		};
	}

	json Serializer::serializeFolderType(const DirectoryContentInfo& aContentInfo) noexcept {
		json retJson = {
			{ "id", "directory" },
			{ "str", Util::formatDirectoryContent(aContentInfo) }
		};

		if (Util::hasContentInfo(aContentInfo)) {
			retJson["files"] = aContentInfo.files;
			retJson["directories"] = aContentInfo.directories;
		}

		return retJson;
	}

	json Serializer::serializeIp(const string& aIP) noexcept {
		return serializeIp(aIP, GeoManager::getInstance()->getCountry(aIP));
	}

	json Serializer::serializeIp(const string& aIP, const string& aCountryCode) noexcept {
		return {
			{ "str", Format::formatIp(aIP, aCountryCode) },
			{ "country", aCountryCode },
			{ "ip", aIP }
		};
	}

	string Serializer::getDownloadStateId(TrackableDownloadItem::State aState) noexcept {
		switch (aState) {
			case TrackableDownloadItem::STATE_DOWNLOAD_FAILED: return "download_failed";
			case TrackableDownloadItem::STATE_DOWNLOAD_PENDING: return "download_pending";
			case TrackableDownloadItem::STATE_DOWNLOADING: return "downloading";
			case TrackableDownloadItem::STATE_DOWNLOADED: return "downloaded";
		}

		dcassert(0);
		return Util::emptyString;
	}

	string Serializer::getDirectoryDownloadStateId(DirectoryDownload::State aState) noexcept {
		switch (aState) {
			case DirectoryDownload::State::PENDING: return "pending";
			case DirectoryDownload::State::QUEUED: return "queued";
			case DirectoryDownload::State::FAILED: return "failed";
		}

		dcassert(0);
		return Util::emptyString;
	}

	json Serializer::serializeDownloadState(const TrackableDownloadItem& aItem) noexcept {
		auto info = aItem.getStatusInfo();
		return {
			{ "id", getDownloadStateId(info.state) },
			{ "str", info.str },
			{ "time_finished", aItem.isDownloaded() ? aItem.getLastTimeFinished() : 0 },
		};
	}

	string Serializer::getDupeId(DupeType aDupeType) noexcept {
		switch (aDupeType) {
			case DUPE_SHARE_PARTIAL: return "share_partial";
			case DUPE_SHARE_FULL: return "share_full";
			case DUPE_QUEUE_PARTIAL: return "queue_partial";
			case DUPE_QUEUE_FULL: return "queue_full";
			case DUPE_FINISHED_PARTIAL: return "finished_partial";
			case DUPE_FINISHED_FULL: return "finished_full";
			case DUPE_SHARE_QUEUE: return "share_queue";
			default: dcassert(0); return Util::emptyString;
		}
	}

	json Serializer::serializeFileDupe(DupeType aDupeType, const TTHValue& aTTH) noexcept {
		if (aDupeType == DUPE_NONE) {
			return nullptr;
		}

		return serializeDupe(aDupeType, AirUtil::getFileDupePaths(aDupeType, aTTH));
	}

	json Serializer::serializeDirectoryDupe(DupeType aDupeType, const string& aAdcPath) noexcept {
		if (aDupeType == DUPE_NONE) {
			return nullptr;
		}

		return serializeDupe(aDupeType, AirUtil::getAdcDirectoryDupePaths(aDupeType, aAdcPath));
	}

	json Serializer::serializeDupe(DupeType aDupeType, StringList&& aPaths) noexcept {
		return {
			{ "id", getDupeId(aDupeType) },
			{ "paths", aPaths },
		};
	}

	json Serializer::serializeSlots(int aFree, int aTotal) noexcept {
		return {
			{ "str", SearchResult::formatSlots(aFree, aTotal) },
			{ "free", aFree },
			{ "total", aTotal }
		};
	}

	json Serializer::serializePriorityId(Priority aPriority) noexcept {
		if (aPriority == Priority::DEFAULT) {
			return nullptr;
		}

		return static_cast<int>(aPriority);
	}

	json Serializer::serializePriority(const QueueItemBase& aItem) noexcept {
		return {
			{ "id", serializePriorityId(aItem.getPriority()) },
			{ "str", AirUtil::getPrioText(aItem.getPriority()) },
			{ "auto", aItem.getAutoPriority() }
		};
	}

	json Serializer::serializeDirectoryDownload(const DirectoryDownloadPtr& aDownload) noexcept {
		return {
			{ "id", aDownload->getId() },
			{ "user", Serializer::serializeHintedUser(aDownload->getUser()) },
			{ "target_name", aDownload->getBundleName() },
			{ "target_directory", aDownload->getTarget() },
			{ "priority", Serializer::serializePriorityId(aDownload->getPriority()) },
			{ "list_path", aDownload->getListPath() },
			{ "state", getDirectoryDownloadStateId(aDownload->getState()) },
			{ "queue_info", aDownload->getQueueInfo() ? serializeDirectoryBundleAddInfo(*aDownload->getQueueInfo(), aDownload->getError()) : json() },
			{ "error", aDownload->getError() },
		};
	}

	json Serializer::serializeDirectoryBundleAddInfo(const DirectoryBundleAddInfo& aInfo, const string& aError) noexcept {
		return {
			{ "files_queued", aInfo.filesAdded },
			{ "files_updated", aInfo.filesUpdated },
			{ "files_failed", aInfo.filesFailed },
			{ "error", aError },
			{ "bundle", serializeBundleAddInfo(aInfo.bundleInfo) }
		};
	}

	json Serializer::serializeBundleAddInfo(const BundleAddInfo& aInfo) noexcept {
		return {
			{ "id", aInfo.bundle->getToken() },
			{ "merged", aInfo.merged },
		};
	}

	json Serializer::serializeSourceCount(const QueueItemBase::SourceCount& aCount) noexcept {
		return {
			{ "online", aCount.online },
			{ "total", aCount.total },
			{ "str", aCount.format() },
		};
	}

	json Serializer::serializeGroupedPaths(const pair<string, OrderedStringSet>& aGroupedPair) noexcept {
		return {
			{ "name", aGroupedPair.first },
			{ "paths", aGroupedPair.second }
		};
	}

	json Serializer::serializeActionHookError(const ActionHookRejectionPtr& aError) noexcept {
		if (!aError) {
			return nullptr;
		}

		return{
			{ "hook_id", aError->hookId },
			{ "hook_name", aError->hookName },
			{ "error_id", aError->rejectId },
			{ "str", aError->message },
		};
	}

	json Serializer::serializeFilesystemItem(const FilesystemItem& aInfo) noexcept {
		json ret = {
			{ "name", aInfo.name }
		};

		if (aInfo.isDirectory) {
			ret["type"] = Serializer::serializeFolderType(DirectoryContentInfo());
		} else {
			ret["type"] = Serializer::serializeFileType(aInfo.name);
			ret["size"] = aInfo.size;
		}

		return ret;
	}

	StringList Serializer::serializePermissions(const AccessList& aPermissions) noexcept {
		return WebUser::permissionsToStringList(aPermissions);
	}

	json Serializer::serializeHubSetting(tribool aSetting) noexcept {
		if (!HubSettings::defined(aSetting)) {
			return nullptr;
		}

		return aSetting.value ? true : false;
	}

	json Serializer::serializeHubSetting(int aSetting) noexcept {
		if (!HubSettings::defined(aSetting)) {
			return nullptr;
		}

		return aSetting;
	}

	string Serializer::serializeHubSetting(const string& aSetting) noexcept {
		if (!HubSettings::defined(aSetting)) {
			return Util::emptyString;
		}

		return aSetting;
	}
}