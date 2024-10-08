/*
* Copyright (C) 2011-2024 AirDC++ Project
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
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

#ifndef DCPLUSPLUS_DCPP_SERIALIZER_H
#define DCPLUSPLUS_DCPP_SERIALIZER_H

#include <api/common/Property.h>

#include <web-server/Access.h>

#include <airdcpp/core/header/typedefs.h>
#include <airdcpp/core/classes/tribool.h>

#include <airdcpp/core/types/DirectoryContentInfo.h>
#include <airdcpp/filelist/DirectoryDownload.h>
#include <airdcpp/core/types/DupeType.h>
#include <airdcpp/queue/QueueItemBase.h>
#include <airdcpp/transfer/download/TrackableDownloadItem.h>


namespace dcpp {
	struct DirectoryContentInfo;
}

namespace webserver {
	class Serializer {
	public:
		static StringSet getUserFlags(const UserPtr& aUser) noexcept;
		static StringSet getOnlineUserFlags(const OnlineUserPtr& aUser) noexcept;

		static json serializeUser(const UserPtr& aUser) noexcept;
		static json serializeHintedUser(const HintedUser& aUser) noexcept;
		static json serializeOnlineUser(const OnlineUserPtr& aUser) noexcept;
		static json serializeClient(const Client* aClient) noexcept;

		static string toFileContentType(const string& aExt) noexcept;
		static string getFileTypeId(const string& aName) noexcept;
		static json serializeFileType(const string& aName) noexcept;
		static json serializeFolderType(const DirectoryContentInfo& aContentInfo) noexcept;

		static json serializeIp(const string& aIP) noexcept;
		static json serializeIp(const string& aIP, const string& aCountryCode) noexcept;

		static json serializeShareProfileSimple(ProfileToken aProfile) noexcept;
		static json serializeEncryption(const string& aInfo, bool aIsTrusted) noexcept;

		static string getDownloadStateId(TrackableDownloadItem::State aState) noexcept;
		static json serializeDownloadState(const TrackableDownloadItem& aItem) noexcept;

		static string getDupeId(DupeType aDupeType) noexcept;
		static json serializeDupe(DupeType aDupeType, StringList&& aPaths) noexcept;
		static json serializeFileDupe(DupeType aDupeType, const TTHValue& aTTH) noexcept;
		static json serializeDirectoryDupe(DupeType aDupeType, const string& aAdcPath) noexcept;
		static json serializeSlots(int aFree, int aTotal) noexcept;
		
		static string getDirectoryDownloadStateId(DirectoryDownload::State aState) noexcept;
		static json serializeDirectoryDownload(const DirectoryDownloadPtr& aDownload) noexcept;
		static json serializeDirectoryBundleAddResult(const DirectoryBundleAddResult& aInfo, const string& aError) noexcept;
		static json serializeBundleAddInfo(const BundleAddInfo& aInfo) noexcept;

		static json serializePriorityId(Priority aPriority) noexcept;
		static json serializePriority(const QueueItemBase& aItem) noexcept;
		static json serializeSourceCount(const QueueItemBase::SourceCount& aCount) noexcept;

		static json serializeGroupedPaths(const pair<string, OrderedStringSet>& aGroupedPair) noexcept;
		static json serializeActionHookError(const ActionHookRejectionPtr& aError) noexcept;

		static json serializeFilesystemItem(const FilesystemItem& aInfo) noexcept;

		static StringList serializePermissions(const AccessList& aPermissions) noexcept;

		// Serialize n items from end by keeping the list order
		// Throws for invalid parameters
		template <class ContainerT, class FuncT>
		static json serializeFromEnd(int aCount, const ContainerT& aList, const FuncT& aF) {
			if (aList.empty()) {
				return json::array();
			}

			if (aCount < 0) {
				throw std::domain_error("Invalid range");
			}

			auto listSize = static_cast<int>(std::distance(std::begin(aList), std::end(aList)));
			auto beginIter = std::begin(aList);
			if (aCount > 0 && listSize > aCount) {
				std::advance(beginIter, listSize - aCount);
			}

			return serializeRange(beginIter, aList.end(), aF);
		}

		// Serialize n items from beginning by keeping the list order
		// Throws for invalid parameters
		template <class ContainerT, class FuncT>
		static json serializeFromBegin(int aCount, const ContainerT& aList, const FuncT& aF) {
			if (aList.empty()) {
				return json::array();
			}

			if (aCount < 0) {
				throw std::domain_error("Invalid range");
			}

			auto listSize = static_cast<int>(std::distance(std::begin(aList), std::end(aList)));
			auto endIter = std::end(aList);
			if (aCount > 0 && listSize > aCount) {
				endIter = std::begin(aList);
				std::advance(endIter, aCount);
			}

			return serializeRange(std::begin(aList), endIter, aF);
		}

		template <class ContainerT, class FuncT>
		static json serializeList(const ContainerT& aList, const FuncT& aF) noexcept {
			return serializeRange(std::begin(aList), std::end(aList), aF);
		}

		// Serialize n messages from position
		// Throws for invalid parameters
		template <class ContainerT, class FuncT>
		static json serializeFromPosition(int aBeginPos, int aCount, const ContainerT& aList, const FuncT& aF) {
			auto listSize = static_cast<int>(std::distance(std::begin(aList), std::end(aList)));
			if (listSize == 0) {
				return json::array();
			}

			if (aBeginPos >= listSize || aCount <= 0) {
				throw std::domain_error("Invalid range");
			}

			auto beginIter = std::begin(aList);
			std::advance(beginIter, aBeginPos);

			auto endIter = beginIter;
			std::advance(endIter, min(listSize - aBeginPos, aCount));

			return serializeRange(beginIter, endIter, aF);
		}

		// Serialize a list of items provider by the handler with a custom range
		// Throws for invalid range parameters
		template <class T, class ContainerT>
		static json serializeItemList(int aStart, int aCount, const PropertyItemHandler<T>& aHandler, const ContainerT& aItems) {
			return Serializer::serializeFromPosition(aStart, aCount, aItems, [&aHandler](const T& aItem) {
				return Serializer::serializeItem(aItem, aHandler);
			});
		}

		// Serialize a list of items provider by the handler
		template <class T, class ContainerT>
		static json serializeItemList(const PropertyItemHandler<T>& aHandler, const ContainerT& aItems) {
			return Serializer::serializeRange(std::begin(aItems), std::end(aItems), [&aHandler](const T& aItem) {
				return Serializer::serializeItem(aItem, aHandler);
			});
		}

		// Serialize item with ID and all properties
		template <class T>
		static json serializeItem(const T& aItem, const PropertyItemHandler<T>& aHandler) noexcept {
			return serializePartialItem(aItem, aHandler, toPropertyIdSet(aHandler.properties));
		}

		// Serialize item with ID and specified properties
		template <class T>
		static json serializePartialItem(const T& aItem, const PropertyItemHandler<T>& aHandler, const PropertyIdSet& aPropertyIds) noexcept {
			auto j = serializeProperties(aItem, aHandler, aPropertyIds);
			j["id"] = aItem->getToken();
			return j;
		}

		// Serialize specified item properties (without the ID)
		template <class T>
		static json serializeProperties(const T& aItem, const PropertyItemHandler<T>& aHandler, const PropertyIdSet& aPropertyIds) noexcept {
			json j;
			for (auto id : aPropertyIds) {
				const auto& prop = aHandler.properties[id];
				switch (prop.serializationMethod) {
				case SERIALIZE_NUMERIC: {
					j[prop.name] = aHandler.numberF(aItem, id);
					break;
				}
				case SERIALIZE_TEXT: {
					j[prop.name] = aHandler.stringF(aItem, id);
					break;
				}
				case SERIALIZE_BOOL: {
					j[prop.name] = aHandler.numberF(aItem, id) == 0 ? false : true;
					break;
				}
				case SERIALIZE_CUSTOM: {
					j[prop.name] = aHandler.jsonF(aItem, id);
					break;
				}
				}
			}

			return j;
		}

		static json serializeChangedProperties(const json& aNewProperties, const json& aOldProperties) noexcept;

		template<typename IdT>
		static json defaultArrayValueSerializer(const IdT& aJson) {
			return aJson;
		}

		static json serializeHubSetting(const tribool& aSetting) noexcept;
		static json serializeHubSetting(int aSetting) noexcept;
		static string serializeHubSetting(const string& aSetting) noexcept;
	private:
		static void appendOnlineUserFlags(const OnlineUserPtr& aUser, StringSet& flags_) noexcept;

		template <class IterT, class FuncT>
		static json serializeRange(const IterT& aBegin, const IterT& aEnd, const FuncT& aF) noexcept {
			auto ret = json::array();
			std::for_each(aBegin, aEnd, [&](const auto& elem) {
				ret.push_back(aF(elem));
			});
			return ret;
		}
	};
}

#endif
