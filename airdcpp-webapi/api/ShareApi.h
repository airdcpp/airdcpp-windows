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

#ifndef DCPLUSPLUS_DCPP_SHAREAPI_H
#define DCPLUSPLUS_DCPP_SHAREAPI_H

#include <web-server/stdinc.h>

#include <api/ApiModule.h>
#include <api/common/ListViewController.h>

#include <airdcpp/typedefs.h>
#include <airdcpp/ShareDirectoryInfo.h>
#include <airdcpp/ShareManagerListener.h>

namespace webserver {
	class ShareApi : public ApiModule, private ShareManagerListener {
	public:
		ShareApi(Session* aSession);
		~ShareApi();

		int getVersion() const noexcept {
			return 0;
		}

		const PropertyList properties = {
			{ PROP_PATH, "path", TYPE_TEXT, SERIALIZE_TEXT, SORT_TEXT },
			{ PROP_VIRTUAL_NAME, "virtual_name", TYPE_TEXT, SERIALIZE_TEXT, SORT_TEXT },
			{ PROP_SIZE, "size", TYPE_SIZE, SERIALIZE_NUMERIC, SORT_NUMERIC },
			{ PROP_PROFILES, "profiles", TYPE_LIST_NUMERIC, SERIALIZE_CUSTOM, SORT_CUSTOM },
			{ PROP_INCOMING, "incoming", TYPE_NUMERIC_OTHER, SERIALIZE_BOOL, SORT_NUMERIC },
			{ PROP_LAST_REFRESH_TIME, "last_refresh_time", TYPE_TIME, SERIALIZE_NUMERIC, SORT_NUMERIC },
			{ PROP_REFRESH_STATE, "refresh_state", TYPE_NUMERIC_OTHER, SERIALIZE_TEXT_NUMERIC, SORT_NUMERIC },
		};

		enum Properties {
			PROP_TOKEN = -1,
			PROP_PATH,
			PROP_VIRTUAL_NAME,
			PROP_SIZE,
			PROP_PROFILES,
			PROP_INCOMING,
			PROP_LAST_REFRESH_TIME,
			PROP_REFRESH_STATE,
			PROP_LAST
		};
	private:
		static json serializeShareProfile(const ShareProfilePtr& aProfile) noexcept;

		api_return handleRefreshShare(ApiRequest& aRequest);
		api_return handleRefreshPaths(ApiRequest& aRequest);

		api_return handleGetProfiles(ApiRequest& aRequest);
		api_return handleAddProfile(ApiRequest& aRequest);
		api_return handleUpdateProfile(ApiRequest& aRequest);
		api_return handleRemoveProfile(ApiRequest& aRequest);
		api_return handleDefaultProfile(ApiRequest& aRequest);

		api_return handleGetRoots(ApiRequest& aRequest);
		api_return handleAddRoot(ApiRequest& aRequest);
		api_return handleUpdateRoot(ApiRequest& aRequest);
		api_return handleRemoveRoot(ApiRequest& aRequest);

		void parseProfile(ShareProfilePtr& aProfile, const json& j);
		void parseRoot(ShareDirectoryInfoPtr& aInfo, const json& j, bool aIsNew);

		api_return handleGetStats(ApiRequest& aRequest);

		api_return handleGetGroupedRootPaths(ApiRequest& aRequest);
		api_return handleFindDupePaths(ApiRequest& aRequest);

		void on(ShareManagerListener::ProfileAdded, ProfileToken aProfile) noexcept;
		void on(ShareManagerListener::ProfileUpdated, ProfileToken aProfile) noexcept;
		void on(ShareManagerListener::ProfileRemoved, ProfileToken aProfile) noexcept;

		void on(ShareManagerListener::RootCreated, const string& aPath) noexcept;
		void on(ShareManagerListener::RootRemoved, const string& aPath) noexcept;
		void on(ShareManagerListener::RootUpdated, const string& aPath) noexcept;

		PropertyItemHandler<ShareDirectoryInfoPtr> itemHandler;

		typedef ListViewController<ShareDirectoryInfoPtr, PROP_LAST> RootView;
		RootView rootView;

		ShareDirectoryInfoList getRoots() noexcept;

		// ListViewController compares items by memory address so we need to store the list here 
		ShareDirectoryInfoList roots;
		mutable SharedMutex rootCS;
	};
}

#endif