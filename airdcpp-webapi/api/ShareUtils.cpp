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

#include "stdinc.h"

#include <api/ShareUtils.h>

#include <api/common/Format.h>
#include <api/common/Serializer.h>

#include <airdcpp/share/ShareManager.h>


namespace webserver {
	const PropertyList ShareUtils::properties = {
		{ PROP_PATH, "path", TYPE_TEXT, SERIALIZE_TEXT, SORT_TEXT },
		{ PROP_VIRTUAL_NAME, "virtual_name", TYPE_TEXT, SERIALIZE_TEXT, SORT_CUSTOM },
		{ PROP_SIZE, "size", TYPE_SIZE, SERIALIZE_NUMERIC, SORT_NUMERIC },
		{ PROP_PROFILES, "profiles", TYPE_LIST_NUMERIC, SERIALIZE_CUSTOM, SORT_CUSTOM },
		{ PROP_INCOMING, "incoming", TYPE_NUMERIC_OTHER, SERIALIZE_BOOL, SORT_NUMERIC },
		{ PROP_LAST_REFRESH_TIME, "last_refresh_time", TYPE_TIME, SERIALIZE_NUMERIC, SORT_NUMERIC },
		{ PROP_STATUS, "status", TYPE_TEXT, SERIALIZE_CUSTOM, SORT_NUMERIC },
		{ PROP_TYPE, "type", TYPE_TEXT, SERIALIZE_CUSTOM, SORT_CUSTOM },
	};

	const PropertyItemHandler<ShareDirectoryInfoPtr> ShareUtils::propertyHandler = {
		properties,
		ShareUtils::getStringInfo, ShareUtils::getNumericInfo, ShareUtils::compareItems, ShareUtils::serializeItem, ShareUtils::filterItem
	};

	json ShareUtils::serializeItem(const ShareDirectoryInfoPtr& aItem, int aPropertyName) noexcept {
		switch (aPropertyName) {
		case PROP_PROFILES:
		{
			return Serializer::serializeList(aItem->profiles, Serializer::serializeShareProfileSimple);
		}
		case PROP_TYPE: {
			return Serializer::serializeFolderType(aItem->contentInfo);
		}
		case PROP_STATUS: {
			return {
				{ "id", formatStatusId(aItem) },
				{ "str", formatDisplayStatus(aItem) },
				{ "refresh_task_id", aItem->refreshTaskToken ? json(*aItem->refreshTaskToken) : json() },
			};
		}
		}

		dcassert(0);
		return nullptr;
	}

	string ShareUtils::formatStatusId(const ShareDirectoryInfoPtr& aItem) noexcept {
		switch (static_cast<ShareRootRefreshState>(aItem->refreshState)) {
			case ShareRootRefreshState::STATE_NORMAL: return "normal";
			case ShareRootRefreshState::STATE_PENDING: return "refresh_pending";
			case ShareRootRefreshState::STATE_RUNNING: return "refresh_running";
		}

		return Util::emptyString;
	}

	string ShareUtils::formatDisplayStatus(const ShareDirectoryInfoPtr& aItem) noexcept {
		switch (static_cast<ShareRootRefreshState>(aItem->refreshState)) {
			case ShareRootRefreshState::STATE_NORMAL: return STRING(NORMAL);
			case ShareRootRefreshState::STATE_PENDING: return STRING(API_SHARE_REFRESH_PENDING);
			case ShareRootRefreshState::STATE_RUNNING: return STRING(API_SHARE_REFRESHING);
		}

		return Util::emptyString;
	}

	bool ShareUtils::filterItem(const ShareDirectoryInfoPtr& aItem, int aPropertyName, const StringMatch&, double aNumericMatcher) noexcept {
		switch (aPropertyName) {
		case PROP_PROFILES:
		{
			return aItem->profiles.contains(static_cast<int>(aNumericMatcher));
		}
		}

		return false;
	}

	int ShareUtils::compareItems(const ShareDirectoryInfoPtr& a, const ShareDirectoryInfoPtr& b, int aPropertyName) noexcept {
		switch (aPropertyName) {
		case PROP_TYPE: {
			return DirectoryContentInfo::Sort(a->contentInfo, b->contentInfo);
		}
		case PROP_PROFILES: {
			return compare(a->profiles.size(), b->profiles.size());
		}
		case PROP_VIRTUAL_NAME: {
			if (a->virtualName != b->virtualName) {
				return compare(a->virtualName, b->virtualName);
			}

			return compare(a->path, b->path);
		}
		default:
			dcassert(0);
		}

		return 0;
	}

	std::string ShareUtils::getStringInfo(const ShareDirectoryInfoPtr& aItem, int aPropertyName) noexcept {
		switch (aPropertyName) {
		case PROP_VIRTUAL_NAME: return aItem->virtualName;
		case PROP_PATH: return aItem->path;
		case PROP_STATUS: return formatDisplayStatus(aItem);
		case PROP_TYPE: return Util::formatDirectoryContent(aItem->contentInfo);
		default: dcassert(0); return Util::emptyString;
		}
	}
	double ShareUtils::getNumericInfo(const ShareDirectoryInfoPtr& aItem, int aPropertyName) noexcept {
		switch (aPropertyName) {
		case PROP_SIZE: return (double)aItem->size;
		case PROP_INCOMING: return (double)aItem->incoming;
		case PROP_LAST_REFRESH_TIME: return (double)aItem->lastRefreshTime;
		case PROP_STATUS: return (double)aItem->refreshState;
		default: dcassert(0); return 0;
		}
	}
}