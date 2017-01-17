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

#include <api/SearchEntity.h>

#include <api/common/Deserializer.h>
#include <api/common/FileSearchParser.h>

#include <airdcpp/BundleInfo.h>
#include <airdcpp/ClientManager.h>
#include <airdcpp/SearchManager.h>
#include <airdcpp/SearchInstance.h>


namespace webserver {
	const StringList SearchEntity::subscriptionList = {
		"search_user_result",
		"search_result_added",
		"search_result_updated",
		"search_hub_searches_sent",
	};

	SearchEntity::SearchEntity(ParentType* aParentModule, const SearchInstancePtr& aSearch, SearchInstanceToken aId, uint64_t aExpirationTick) :
		expirationTick(aExpirationTick), id(aId),
		SubApiModule(aParentModule, aId, subscriptionList), search(aSearch),
		searchView("search_view", this, SearchUtils::propertyHandler, std::bind(&SearchEntity::getResultList, this)) {

		METHOD_HANDLER(Access::SEARCH,		METHOD_POST,	(EXACT_PARAM("hub_search")),									SearchEntity::handlePostHubSearch);
		METHOD_HANDLER(Access::SEARCH,		METHOD_POST,	(EXACT_PARAM("user_search")),									SearchEntity::handlePostUserSearch);

		METHOD_HANDLER(Access::SEARCH,		METHOD_GET,		(EXACT_PARAM("results"), RANGE_START_PARAM, RANGE_MAX_PARAM),	SearchEntity::handleGetResults);
		METHOD_HANDLER(Access::DOWNLOAD,	METHOD_POST,	(EXACT_PARAM("results"), TTH_PARAM, EXACT_PARAM("download")),	SearchEntity::handleDownload);
		METHOD_HANDLER(Access::SEARCH,		METHOD_GET,		(EXACT_PARAM("results"), TTH_PARAM, EXACT_PARAM("children")),	SearchEntity::handleGetChildren);
	}

	SearchEntity::~SearchEntity() {
		search->removeListener(this);
	}

	void SearchEntity::init() noexcept {
		search->addListener(this);
	}

	optional<int64_t> SearchEntity::getTimeToExpiration() const noexcept {
		if (expirationTick == 0) {
			return boost::none;
		}

		return static_cast<int64_t>(expirationTick) - static_cast<int64_t>(GET_TICK());
	}

	GroupedSearchResultList SearchEntity::getResultList() noexcept {
		return search->getResultList();
	}

	api_return SearchEntity::handleGetResults(ApiRequest& aRequest) {
		// Serialize the most relevant results first
		auto j = Serializer::serializeItemList(aRequest.getRangeParam(START_POS), aRequest.getRangeParam(MAX_COUNT), SearchUtils::propertyHandler, search->getResultSet());

		aRequest.setResponseBody(j);
		return websocketpp::http::status_code::ok;
	}

	api_return SearchEntity::handleGetChildren(ApiRequest& aRequest) {
		auto result = search->getResult(aRequest.getTTHParam());
		if (!result) {
			aRequest.setResponseErrorStr("Result not found");
			return websocketpp::http::status_code::not_found;
		}

		aRequest.setResponseBody(Serializer::serializeList(result->getChildren(), serializeSearchResult));
		return websocketpp::http::status_code::ok;
	}

	json SearchEntity::serializeSearchResult(const SearchResultPtr& aSR) noexcept {
		return {
			{ "id", aSR->getId() },
			{ "path", Util::toAdcFile(aSR->getPath()) },
			{ "ip", Serializer::serializeIp(aSR->getIP()) },
			{ "user", Serializer::serializeHintedUser(aSR->getUser()) },
			{ "connection", aSR->getConnectionInt() },
			{ "time", aSR->getDate() },
			{ "slots", Serializer::serializeSlots(aSR->getFreeSlots(), aSR->getTotalSlots()) },
		};
	}

	api_return SearchEntity::handleDownload(ApiRequest& aRequest) {
		auto result = search->getResult(aRequest.getTTHParam());
		if (!result) {
			aRequest.setResponseErrorStr("Result not found");
			return websocketpp::http::status_code::not_found;
		}

		string targetDirectory, targetName = result->getFileName();
		Priority prio;
		Deserializer::deserializeDownloadParams(aRequest.getRequestBody(), aRequest.getSession(), targetDirectory, targetName, prio);

		try {
			if (result->isDirectory()) {
				auto directoryDownloads = result->downloadDirectory(targetDirectory, targetName, prio);
				aRequest.setResponseBody({
					{ "directory_download_ids", Serializer::serializeList(directoryDownloads, Serializer::serializeDirectoryDownload) }
				});
			} else {
				auto bundleAddInfo = result->downloadFile(targetDirectory, targetName, prio);
				aRequest.setResponseBody({
					{ "bundle_info", Serializer::serializeBundleAddInfo(bundleAddInfo) }
				});
			}
		} catch (const Exception& e) {
			aRequest.setResponseErrorStr(e.what());
			return websocketpp::http::status_code::bad_request;
		}

		return websocketpp::http::status_code::ok;
	}

	api_return SearchEntity::handlePostHubSearch(ApiRequest& aRequest) {
		const auto& reqJson = aRequest.getRequestBody();

		// Parse request
		auto s = FileSearchParser::parseSearch(reqJson, false, Util::toString(Util::rand()));
		auto hubs = Deserializer::deserializeHubUrls(reqJson);

		auto queueResult = search->hubSearch(hubs, s);
		if (queueResult.queuedHubUrls.empty() && !queueResult.error.empty()) {
			aRequest.setResponseErrorStr(queueResult.error);
			return websocketpp::http::status_code::bad_request;
		}

		aRequest.setResponseBody({
			{ "queue_time", queueResult.queueTime },
			{ "search_id", search->getCurrentSearchToken() },
			{ "queued_count", queueResult.queuedHubUrls.size() },
		});

		return websocketpp::http::status_code::ok;
	}

	api_return SearchEntity::handlePostUserSearch(ApiRequest& aRequest) {
		const auto& reqJson = aRequest.getRequestBody();

		// Parse user and query
		auto user = Deserializer::deserializeHintedUser(reqJson, false);
		auto s = FileSearchParser::parseSearch(reqJson, true, Util::toString(Util::rand()));

		string error;
		if (!search->userSearch(user, s, error)) {
			aRequest.setResponseErrorStr(error);
			return websocketpp::http::status_code::bad_request;
		}

		return websocketpp::http::status_code::no_content;
	}

	void SearchEntity::on(SearchInstanceListener::GroupedResultAdded, const GroupedSearchResultPtr& aResult) noexcept {
		searchView.onItemAdded(aResult);

		if (subscriptionActive("search_result_added")) {
			send("search_result_added", {
				{ "search_id", search->getCurrentSearchToken() },
				{ "result", Serializer::serializeItem(aResult, SearchUtils::propertyHandler) }
			});
		}
	}

	void SearchEntity::on(SearchInstanceListener::GroupedResultUpdated, const GroupedSearchResultPtr& aResult) noexcept {
		searchView.onItemUpdated(aResult, {
			SearchUtils::PROP_RELEVANCE, SearchUtils::PROP_CONNECTION,
			SearchUtils::PROP_HITS, SearchUtils::PROP_SLOTS,
			SearchUtils::PROP_USERS
		});
		
		if (subscriptionActive("search_result_updated")) {
			send("search_result_updated", {
				{ "search_id", search->getCurrentSearchToken() },
				{ "result", Serializer::serializeItem(aResult, SearchUtils::propertyHandler) }
			});
		}
	}

	void SearchEntity::on(SearchInstanceListener::UserResult, const SearchResultPtr& aResult, const GroupedSearchResultPtr& aParent) noexcept {
		if (subscriptionActive("search_user_result")) {
			send("search_user_result", {
				{ "search_id", search->getCurrentSearchToken() },
				{ "parent_id", aParent->getToken() },
				{ "result", serializeSearchResult(aResult) }
			});
		}
	}

	void SearchEntity::on(SearchInstanceListener::Reset) noexcept {
		searchView.resetItems();
	}

	void SearchEntity::on(SearchInstanceListener::HubSearchSent, const string& aSearchToken, int aSent) noexcept {
		if (subscriptionActive("search_hub_searches_sent")) {
			send("search_hub_searches_sent", {
				{ "search_id", aSearchToken },
				{ "sent", aSent }
			});
		}
	}
}