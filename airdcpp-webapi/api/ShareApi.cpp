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

#include <api/ShareApi.h>
#include <api/common/Serializer.h>
#include <api/common/Deserializer.h>
#include <web-server/JsonUtil.h>

#include <airdcpp/ShareManager.h>
#include <airdcpp/HubEntry.h>

namespace webserver {
	ShareApi::ShareApi(Session* aSession) : ApiModule(aSession) {
		ShareManager::getInstance()->addListener(this);

		METHOD_HANDLER("profiles", ApiRequest::METHOD_GET, (), false, ShareApi::handleGetProfiles);
		METHOD_HANDLER("profile", ApiRequest::METHOD_POST, (), true, ShareApi::handleAddProfile);
		METHOD_HANDLER("profile", ApiRequest::METHOD_PATCH, (TOKEN_PARAM), true, ShareApi::handleUpdateProfile);
		METHOD_HANDLER("profile", ApiRequest::METHOD_DELETE, (TOKEN_PARAM), false, ShareApi::handleRemoveProfile);
		METHOD_HANDLER("profile", ApiRequest::METHOD_POST, (TOKEN_PARAM, EXACT_PARAM("default")), false, ShareApi::handleDefaultProfile);

		METHOD_HANDLER("roots", ApiRequest::METHOD_GET, (), false, ShareApi::handleGetRoots);
		METHOD_HANDLER("root", ApiRequest::METHOD_POST, (EXACT_PARAM("add")), true, ShareApi::handleAddRoot);
		METHOD_HANDLER("root", ApiRequest::METHOD_POST, (EXACT_PARAM("update")), true, ShareApi::handleUpdateRoot);
		METHOD_HANDLER("root", ApiRequest::METHOD_POST, (EXACT_PARAM("remove")), true, ShareApi::handleRemoveRoot);

		METHOD_HANDLER("grouped_root_paths", ApiRequest::METHOD_GET, (), false, ShareApi::handleGetGroupedRootPaths);
		METHOD_HANDLER("stats", ApiRequest::METHOD_GET, (), false, ShareApi::handleGetStats);
		METHOD_HANDLER("find_dupe_paths", ApiRequest::METHOD_POST, (), true, ShareApi::handleFindDupePaths);


		createSubscription("share_profile_added");
		createSubscription("share_profile_updated");
		createSubscription("share_profile_removed");
	}

	ShareApi::~ShareApi() {
		ShareManager::getInstance()->removeListener(this);
	}

	api_return ShareApi::handleGetStats(ApiRequest& aRequest) {
		json j;

		auto optionalStats = ShareManager::getInstance()->getShareStats();
		if (!optionalStats) {
			return websocketpp::http::status_code::no_content;
		}

		auto stats = *optionalStats;
		j["total_file_count"] = stats.totalFileCount;
		j["total_directory_count"] = stats.totalDirectoryCount;
		j["files_per_directory"] = stats.filesPerDirectory;
		j["total_size"] = stats.totalSize;
		j["unique_file_percentage"] = stats.uniqueFilePercentage;
		j["unique_files"] = stats.uniqueFileCount;
		j["average_file_age"] = stats.averageFileAge;
		j["profile_count"] = stats.profileCount;
		j["profile_root_count"] = stats.profileDirectoryCount;

		aRequest.setResponseBody(j);
		return websocketpp::http::status_code::ok;
	}

	json ShareApi::serializeShareProfile(const ShareProfilePtr& aProfile) noexcept {
		size_t totalFiles = 0;
		int64_t totalSize = 0;
		ShareManager::getInstance()->getProfileInfo(aProfile->getToken(), totalSize, totalFiles);

		return {
			{ "id", aProfile->getToken() },
			{ "name", aProfile->getDisplayName() },
			{ "plain_name", aProfile->getPlainName() },
			{ "default", aProfile->isDefault() },
			{ "size", totalSize },
			{ "files", totalFiles },
		};
	}

	/*json ShareApi::serializeShareRoot(const ShareDirInfo& aRoot) noexcept {
		return{
			{ "path", aRoot.path },
			{ "virtual_name", aRoot.vname },
			{ "size", aRoot.size },
		};
	}*/

	api_return ShareApi::handleGetRoots(ApiRequest& aRequest) {
		return websocketpp::http::status_code::ok;
	}

	api_return ShareApi::handleAddRoot(ApiRequest& aRequest) {
		return websocketpp::http::status_code::ok;
	}

	api_return ShareApi::handleUpdateRoot(ApiRequest& aRequest) {
		return websocketpp::http::status_code::ok;
	}

	api_return ShareApi::handleRemoveRoot(ApiRequest& aRequest) {
		return websocketpp::http::status_code::ok;
	}

	api_return ShareApi::handleDefaultProfile(ApiRequest& aRequest) {
		auto token = aRequest.getTokenParam(0);
		auto profile = ShareManager::getInstance()->getShareProfile(token);
		if (!profile) {
			aRequest.setResponseErrorStr("Profile not found");
			return websocketpp::http::status_code::not_found;
		}

		ShareManager::getInstance()->setDefaultProfile(token);
		return websocketpp::http::status_code::ok;
	}

	void ShareApi::on(ShareManagerListener::ProfileAdded, ProfileToken aProfile) noexcept {
		if (!subscriptionActive("share_profile_added")) {
			return;
		}

		send("share_profile_added", serializeShareProfile(ShareManager::getInstance()->getShareProfile(aProfile)));
	}

	void ShareApi::on(ShareManagerListener::ProfileUpdated, ProfileToken aProfile) noexcept {
		if (!subscriptionActive("share_profile_updated")) {
			return;
		}

		send("share_profile_updated", serializeShareProfile(ShareManager::getInstance()->getShareProfile(aProfile)));
	}

	void ShareApi::on(ShareManagerListener::ProfileRemoved, ProfileToken aProfile) noexcept {
		if (!subscriptionActive("share_profile_removed")) {
			return;
		}

		send("share_profile_removed", { 
			{ "id", aProfile } 
		});
	}

	void ShareApi::parseProfile(ShareProfilePtr& aProfile, const json& j) {
		auto name = JsonUtil::getField<string>("name", j, false);

		auto token = ShareManager::getInstance()->getProfileByName(name);
		if (token && token != aProfile->getToken()) {
			JsonUtil::throwError("name", JsonUtil::ERROR_EXISTS, "Profile with the same name exists");
		}

		aProfile->setPlainName(name);
	}

	api_return ShareApi::handleAddProfile(ApiRequest& aRequest) {
		const auto& reqJson = aRequest.getRequestBody();

		auto profile = make_shared<ShareProfile>();
		parseProfile(profile, reqJson);

		ShareManager::getInstance()->addProfile(profile);
		return websocketpp::http::status_code::ok;
	}

	api_return ShareApi::handleUpdateProfile(ApiRequest& aRequest) {
		const auto& reqJson = aRequest.getRequestBody();

		auto profile = ShareManager::getInstance()->getShareProfile(aRequest.getTokenParam(0));
		if (!profile) {
			aRequest.setResponseErrorStr("Profile not found");
			return websocketpp::http::status_code::not_found;
		}

		parseProfile(profile, reqJson);
		ShareManager::getInstance()->updateProfile(profile);
		return websocketpp::http::status_code::ok;
	}

	api_return ShareApi::handleRemoveProfile(ApiRequest& aRequest) {
		auto token = aRequest.getTokenParam(0);

		if (!ShareManager::getInstance()->getShareProfile(token)) {
			aRequest.setResponseErrorStr("Profile not found");
			return websocketpp::http::status_code::not_found;
		}

		ShareManager::getInstance()->removeProfiles({ make_shared<ShareProfileInfo>(Util::emptyString, token) });

		return websocketpp::http::status_code::ok;
	}

	api_return ShareApi::handleGetProfiles(ApiRequest& aRequest) {
		json j;

		auto profiles = ShareManager::getInstance()->getProfiles();

		// Profiles can't be empty
		for (const auto& p : profiles) {
			j.push_back(serializeShareProfile(p));
		}

		aRequest.setResponseBody(j);
		return websocketpp::http::status_code::ok;
	}

	api_return ShareApi::handleGetGroupedRootPaths(ApiRequest& aRequest) {
		json ret;

		auto roots = ShareManager::getInstance()->getGroupedDirectories();
		if (!roots.empty()) {
			for (const auto& vPath : roots) {
				json parentJson;
				parentJson["name"] = vPath.first;
				for (const auto& realPath : vPath.second) {
					parentJson["paths"].push_back(realPath);
				}

				ret.push_back(parentJson);
			}
		} else {
			ret = json::array();
		}

		aRequest.setResponseBody(ret);
		return websocketpp::http::status_code::ok;
	}

	api_return ShareApi::handleFindDupePaths(ApiRequest& aRequest) {
		const auto& reqJson = aRequest.getRequestBody();

		json ret;

		StringList paths;
		auto path = JsonUtil::getOptionalField<string>("path", reqJson, false, false);
		if (path) {
			paths = ShareManager::getInstance()->getDirPaths(Util::toNmdcFile(*path));
		} else {
			auto tth = Deserializer::deserializeTTH(reqJson);
			paths = ShareManager::getInstance()->getRealPaths(tth);
		}

		if (!paths.empty()) {
			for (const auto& p : paths) {
				ret.push_back(p);
			}
		} else {
			ret = json::array();
		}

		aRequest.setResponseBody(ret);
		return websocketpp::http::status_code::ok;
	}
}