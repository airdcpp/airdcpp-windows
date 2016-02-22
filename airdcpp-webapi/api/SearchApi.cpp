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

#include <api/SearchApi.h>
#include <api/SearchUtils.h>

#include <api/common/Deserializer.h>

#include <airdcpp/DirectSearch.h>
#include <airdcpp/ScopedFunctor.h>

const unsigned int MIN_SEARCH = 2;

namespace webserver {
	SearchApi::SearchApi(Session* aSession) : ApiModule(aSession, Access::SEARCH), itemHandler(properties,
		SearchUtils::getStringInfo, SearchUtils::getNumericInfo, SearchUtils::compareResults, SearchUtils::serializeResult), 
		searchView("search_view", this, itemHandler, std::bind(&SearchApi::getResultList, this)) {

		SearchManager::getInstance()->addListener(this);

		METHOD_HANDLER("query", Access::SEARCH, ApiRequest::METHOD_POST, (), true, SearchApi::handlePostHubSearch);
		METHOD_HANDLER("query", Access::SEARCH, ApiRequest::METHOD_POST, (EXACT_PARAM("user")), true, SearchApi::handlePostUserSearch);

		METHOD_HANDLER("types", Access::ANY, ApiRequest::METHOD_GET, (), false, SearchApi::handleGetTypes);

		METHOD_HANDLER("results", Access::SEARCH, ApiRequest::METHOD_GET, (NUM_PARAM, NUM_PARAM), false, SearchApi::handleGetResults);
		METHOD_HANDLER("result", Access::DOWNLOAD, ApiRequest::METHOD_POST, (TOKEN_PARAM, EXACT_PARAM("download")), false, SearchApi::handleDownload);
	}

	SearchApi::~SearchApi() {
		SearchManager::getInstance()->removeListener(this);
	}

	api_return SearchApi::handleGetResults(ApiRequest& aRequest) {
		// Serialize the most relevant results first
		SearchResultInfo::Set resultSet;

		{
			RLock l(cs);
			boost::range::copy(results | map_values, inserter(resultSet, resultSet.begin()));
		}

		auto j = Serializer::serializeItemList(aRequest.getRangeParam(0), aRequest.getRangeParam(1), itemHandler, resultSet);

		aRequest.setResponseBody(j);
		return websocketpp::http::status_code::ok;
	}

	SearchResultInfo::List SearchApi::getResultList() {
		SearchResultInfo::List ret;

		RLock l(cs);
		boost::range::copy(results | map_values, back_inserter(ret));
		return ret;
	}

	api_return SearchApi::handleDownload(ApiRequest& aRequest) {
		SearchResultInfoPtr result = nullptr;

		{
			RLock l(cs);
			auto i = find_if(results | map_values, [&](const SearchResultInfoPtr& aSI) { return aSI->getToken() == aRequest.getTokenParam(0); });
			if (i.base() == results.end()) {
				aRequest.setResponseErrorStr("Result not found");
				return websocketpp::http::status_code::not_found;
			}

			result = *i;
		}

		string targetDirectory, targetName = result->sr->getFileName();
		TargetUtil::TargetType targetType;
		QueueItemBase::Priority prio;
		Deserializer::deserializeDownloadParams(aRequest.getRequestBody(), targetDirectory, targetName, targetType, prio);

		return result->download(targetDirectory, targetName, targetType, prio);
	}

	api_return SearchApi::handleGetTypes(ApiRequest& aRequest) {
		auto getName = [](const string& aId) -> string {
			if (aId.size() == 1 && aId[0] >= '1' && aId[0] <= '6') {
				return string(SearchManager::getTypeStr(aId[0] - '0'));
			}

			return aId;
		};

		auto types = SearchManager::getInstance()->getSearchTypes();

		json retJ;
		for (const auto& s : types) {
			retJ.push_back({
				{ "id", s.first },
				{ "str", getName(s.first) }
			});
		}

		aRequest.setResponseBody(retJ);
		return websocketpp::http::status_code::ok;
	}

	map<string, string> typeMappings = {
		{ "any", "0" },
		{ "audio", "1" },
		{ "compressed", "2" },
		{ "document", "3" },
		{ "executable", "4" },
		{ "picture", "5" },
		{ "video", "6" },
		{ "directory", "7" },
		{ "tth", "8" },
		{ "file", "9" },
	};

	const string& SearchApi::parseFileType(const string& aType) noexcept {
		auto i = typeMappings.find(aType);
		return i != typeMappings.end() ? i->second : aType;
	}

	SearchPtr SearchApi::parseQuery(const json& aJson, const string& aToken) {
		auto queryJson = JsonUtil::getRawValue("query", aJson);

		auto pattern = JsonUtil::getOptionalFieldDefault<string>("pattern", queryJson, Util::emptyString, false);

		auto s = make_shared<Search>(Search::MANUAL, pattern, aToken);

		// Filetype
		s->fileType = pattern.size() == 39 && Encoder::isBase32(pattern.c_str()) ? Search::TYPE_TTH : Search::TYPE_ANY;

		auto fileTypeStr = JsonUtil::getOptionalField<string>("file_type", queryJson, false);
		if (fileTypeStr) {
			try {
				SearchManager::getInstance()->getSearchType(parseFileType(*fileTypeStr), s->fileType, s->exts, true);
			} catch (...) {
				throw std::domain_error("Invalid file type");
			}
		}

		// Extensions
		auto optionalExtensions = JsonUtil::getOptionalField<StringList>("extensions", queryJson);
		if (optionalExtensions) {
			s->exts = *optionalExtensions;
		}

		// Anything to search for?
		if (s->exts.empty() && pattern.empty()) {
			throw std::domain_error("A valid pattern or file extensions must be provided");
		}

		// Date
		s->maxDate = JsonUtil::getOptionalField<time_t>("max_age", queryJson);
		s->minDate = JsonUtil::getOptionalField<time_t>("min_age", queryJson);

		// Size
		auto minSize = JsonUtil::getOptionalField<int64_t>("min_size", queryJson);
		if (minSize) {
			s->size = *minSize;
			s->sizeType = Search::SIZE_ATLEAST;
		}

		auto maxSize = JsonUtil::getOptionalField<int64_t>("max_size", queryJson);
		if (maxSize) {
			s->size = *maxSize;
			s->sizeType = Search::SIZE_ATMOST;
		}

		// Excluded
		s->excluded = JsonUtil::getOptionalFieldDefault<StringList>("excluded", queryJson, StringList());

		return s;
	}

	api_return SearchApi::handlePostHubSearch(ApiRequest& aRequest) {
		const auto& reqJson = aRequest.getRequestBody();

		currentSearchToken = Util::toString(Util::rand());

		auto s = parseQuery(reqJson, currentSearchToken);
		auto hubs = Deserializer::deserializeHubUrls(reqJson);

		// new search
		auto matcher = SearchQuery::getSearch(s);

		{
			WLock l(cs);
			results.clear();
		}

		searchView.resetItems();

		curSearch = shared_ptr<SearchQuery>(matcher);

		//SettingsManager::getInstance()->addToHistory(s->query, SettingsManager::HISTORY_SEARCH);

		auto result = SearchManager::getInstance()->search(hubs, s);

		aRequest.setResponseBody({
			{ "queue_time", result.queueTime },
			{ "search_token", currentSearchToken },
			{ "sent", result.succeed },
		});
		return websocketpp::http::status_code::ok;
	}

	api_return SearchApi::handlePostUserSearch(ApiRequest& aRequest) {
		const auto& reqJson = aRequest.getRequestBody();

		// User
		auto user = Deserializer::deserializeHintedUser(reqJson, false);

		// Common query properties
		auto s = parseQuery(reqJson, Util::toString(Util::rand()));

		// Direct search properties
		s->maxResults = JsonUtil::getOptionalFieldDefault<int>("max_results", reqJson, 5);
		s->returnParents = JsonUtil::getOptionalFieldDefault<bool>("return_parents", reqJson, false);
		s->namesOnly = JsonUtil::getOptionalFieldDefault<bool>("match_name", reqJson, false);
		s->requireReply = true;

		// Search and matcher
		auto ds = DirectSearch(user, s, JsonUtil::getOptionalFieldDefault<string>("path", reqJson, Util::emptyString));
		unique_ptr<SearchQuery> matcher(SearchQuery::getSearch(s));

		// Wait for the search to finish
		while (true) {
			Thread::sleep(50);
			if (ds.finished()) {
				break;
			}
		}

		// Construct SearchResultInfos
		SearchResultInfo::Set resultSet;
		for (const auto& sr : ds.getResults()) {
			SearchResult::RelevancyInfo relevancyInfo;
			if (sr->getRelevancy(*matcher.get(), relevancyInfo)) {
				resultSet.emplace(std::make_shared<SearchResultInfo>(sr, move(relevancyInfo)));
			}
		}

		// Serialize results
		auto j = Serializer::serializeItemList(itemHandler, resultSet);
		aRequest.setResponseBody(j);

		return websocketpp::http::status_code::ok;
	}

	void SearchApi::on(SearchManagerListener::SR, const SearchResultPtr& aResult) noexcept {
		auto search = curSearch; // Increase the refs
		if (!search) {
			return;
		}

		SearchResult::RelevancyInfo relevancyInfo;
		{
			WLock l(cs);
			if (!aResult->getRelevancy(*search.get(), relevancyInfo, currentSearchToken)) {
				return;
			}
		}

		SearchResultInfoPtr parent = nullptr;
		auto result = std::make_shared<SearchResultInfo>(aResult, move(relevancyInfo));

		{
			WLock l(cs);
			auto i = results.emplace(aResult->getTTH(), result);
			if (!i.second) {
				parent = i.first->second;
			}
		}

		if (!parent) {
			searchView.onItemAdded(result);
			return;
		}

		// No duplicate results for the same user that are received via different hubs
		if (parent->hasUser(aResult->getUser())) {
			return;
		}

		// Add as child
		parent->addChildResult(result);
		searchView.onItemUpdated(parent, { PROP_RELEVANCY, PROP_CONNECTION, PROP_HITS, PROP_SLOTS, PROP_USERS });
	}
}