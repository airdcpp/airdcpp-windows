/*
* Copyright (C) 2011-2015 AirDC++ Project
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

#include <api/ShareUtils.h>
#include <api/ShareApi.h>

#include <api/common/Format.h>

#include <airdcpp/ShareManager.h>

namespace webserver {
	ShareDirectoryInfoList ShareUtils::getItemList() noexcept {
		return ShareManager::getInstance()->getRootInfos();
	}

	json ShareUtils::serializeItem(const ShareDirectoryInfoPtr& aItem, int aPropertyName) noexcept {
		json j;

		switch (aPropertyName) {
		case ShareApi::PROP_PROFILES:
		{
			return aItem->profiles;
		}
		}


		return j;
	}

	int ShareUtils::compareItems(const ShareDirectoryInfoPtr& a, const ShareDirectoryInfoPtr& b, int aPropertyName) noexcept {
		switch (aPropertyName) {
		//case ShareApi::PROP_VIRTUAL_NAME: {
			//if (a->getType() == b->getType())
			//	return Util::stricmp(a->getName(), b->getName());
			//else
			//	return (a->getType() == FilelistItemInfo::DIRECTORY) ? -1 : 1;
		//}
		case ShareApi::PROP_PROFILES: {
			return compare(a->profiles.size(), b->profiles.size());
		}
		default:
			dcassert(0);
		}

		return 0;
	}

	std::string ShareUtils::getStringInfo(const ShareDirectoryInfoPtr& aItem, int aPropertyName) noexcept {
		switch (aPropertyName) {
		case ShareApi::PROP_VIRTUAL_NAME: return aItem->virtualName;
		case ShareApi::PROP_PATH: return aItem->path;
		default: dcassert(0); return 0;
		}
	}
	double ShareUtils::getNumericInfo(const ShareDirectoryInfoPtr& aItem, int aPropertyName) noexcept {
		switch (aPropertyName) {
		case ShareApi::PROP_SIZE: return (double)aItem->size;
		case ShareApi::PROP_INCOMING: return (double)aItem->incoming;
		default: dcassert(0); return 0;
		}
	}
}