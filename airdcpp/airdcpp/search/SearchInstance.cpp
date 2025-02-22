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
#include <airdcpp/search/SearchInstance.h>

#include <airdcpp/hub/ClientManager.h>
#include <airdcpp/search/SearchManager.h>
#include <airdcpp/search/SearchQuery.h>
#include <airdcpp/core/timer/TimerManager.h>


namespace dcpp {
	atomic<SearchInstanceToken> searchInstanceIdCounter { 1 };
	SearchInstance::SearchInstance(const string& aOwnerId, uint64_t aExpirationTick) : token(searchInstanceIdCounter++), expirationTick(aExpirationTick), ownerId(aOwnerId) {
		SearchManager::getInstance()->addListener(this);
		ClientManager::getInstance()->addListener(this);
	}

	SearchInstance::~SearchInstance() {
		ClientManager::getInstance()->cancelSearch(this);

		ClientManager::getInstance()->removeListener(this);
		SearchManager::getInstance()->removeListener(this);
	}

	optional<int64_t> SearchInstance::getTimeToExpiration() const noexcept {
		if (expirationTick == 0) {
			return nullopt;
		}

		return std::max(static_cast<int64_t>(expirationTick) - static_cast<int64_t>(GET_TICK()), static_cast<int64_t>(0));
	}

	GroupedSearchResult::Ptr SearchInstance::getResult(GroupedResultToken aToken) const noexcept {
		RLock l(cs);
		auto i = ranges::find_if(results | views::values, [&](const GroupedSearchResultPtr& aSI) { return aSI->getTTH() == aToken; });
		if (i.base() == results.end()) {
			return nullptr;
		}

		return *i;
	}

	GroupedSearchResultList SearchInstance::getResultList() const noexcept {
		GroupedSearchResultList ret;

		RLock l(cs);
		ranges::copy(results | views::values, back_inserter(ret));
		return ret;
	}

	GroupedSearchResult::Set SearchInstance::getResultSet() const noexcept {
		GroupedSearchResult::Set resultSet;

		{
			RLock l(cs);
			ranges::copy(results | views::values, inserter(resultSet, resultSet.begin()));
		}

		return resultSet;
	}

	void SearchInstance::reset(const SearchPtr& aSearch) noexcept {
		ClientManager::getInstance()->cancelSearch(this);

		{
			WLock l(cs);
			currentSearchToken = aSearch->token;
			curMatcher = shared_ptr<SearchQuery>(SearchQuery::fromSearch(aSearch));
			curParams = aSearch;

			results.clear();
			queuedHubUrls.clear();
			searchesSent = 0;
			filteredResultCount = 0;
		}

		fire(SearchInstanceListener::Reset());
	}

	SearchQueueInfo SearchInstance::hubSearch(StringList& aHubUrls, const SearchPtr& aSearch) noexcept {
		reset(aSearch);

		auto queueInfo = SearchManager::getInstance()->search(aHubUrls, aSearch, this);
		if (!queueInfo.queuedHubUrls.empty()) {
			lastSearchTime = GET_TIME();

			fire(SearchInstanceListener::HubSearchQueued(), currentSearchToken, queueInfo.queueTime, queuedHubUrls.size());
			if (queueInfo.queueTime < 1000) {
				fire(SearchInstanceListener::HubSearchSent(), currentSearchToken, queueInfo.queuedHubUrls.size());
			} else {
				WLock l(cs);
				queuedHubUrls = queueInfo.queuedHubUrls;
			}
		}

		return queueInfo;
	}

	bool SearchInstance::userSearchHooked(const HintedUser& aUser, const SearchPtr& aSearch, string& error_) noexcept {
		reset(aSearch);
		if (!ClientManager::getInstance()->directSearchHooked(aUser, aSearch, error_)) {
			return false;
		}

		lastSearchTime = GET_TIME();
		return true;
	}

	uint64_t SearchInstance::getTimeFromLastSearch() const noexcept {
		if (lastSearchTime == 0) {
			return 0;
		}

		return GET_TIME() - lastSearchTime;
	}

	uint64_t SearchInstance::getQueueTime() const noexcept {
		if (auto time = ClientManager::getInstance()->getMaxSearchQueueTime(this); time) {
			return *time;
		}

		return 0;
	}

	int SearchInstance::getQueueCount() const noexcept {
		RLock l(cs);
		return static_cast<int>(queuedHubUrls.size());
	}

	int SearchInstance::getResultCount() const noexcept {
		RLock l(cs);
		return static_cast<int>(results.size());
	}

	void SearchInstance::on(ClientManagerListener::OutgoingSearch, const string& aHubUrl, const SearchPtr& aSearch) noexcept {
		if (aSearch->owner != this) {
			return;
		}

		searchesSent++;
		removeQueuedUrl(aHubUrl);
	}

	void SearchInstance::on(ClientManagerListener::ClientDisconnected, const string& aHubUrl) noexcept {
		removeQueuedUrl(aHubUrl);
	}

	void SearchInstance::removeQueuedUrl(const string& aHubUrl) noexcept {
		auto queueEmpty = false;

		{
			WLock l(cs);
			if (auto removed = queuedHubUrls.erase(aHubUrl); removed == 0) {
				return;
			}

			queueEmpty = queuedHubUrls.empty();
		}

		if (queueEmpty) {
			fire(SearchInstanceListener::HubSearchSent(), currentSearchToken, searchesSent);
		}
	}

	optional<SearchResult::RelevanceInfo> SearchInstance::matchResult(const SearchResultPtr& aResult) noexcept {
		auto matcher = curMatcher; // Increase the refs
		if (!matcher) {
			return nullopt;
		}

		// NMDC doesn't support both min and max size
		if (aResult->isNMDC() && aResult->getType() == SearchResult::Type::FILE) {
			if (!matcher->matchesSize(aResult->getSize())) {
				filteredResultCount++;
				fire(SearchInstanceListener::ResultFiltered());
				return nullopt;
			}
		}

		SearchResult::RelevanceInfo relevanceInfo;
		{
			WLock l(cs);
			if (!aResult->getRelevance(*matcher.get(), relevanceInfo, currentSearchToken)) {
				filteredResultCount++;
				fire(SearchInstanceListener::ResultFiltered());
				return nullopt;
			}
		}

		if (freeSlotsOnly && aResult->getFreeSlots() < 1) {
			filteredResultCount++;
			fire(SearchInstanceListener::ResultFiltered());
			return nullopt;
		}

		return relevanceInfo;
	}

	void SearchInstance::on(SearchManagerListener::SR, const SearchResultPtr& aResult) noexcept {
		auto relevanceInfo = matchResult(aResult);
		if (!relevanceInfo) {
			return;
		}

		GroupedSearchResultPtr parent = nullptr;
		bool created = false;

		{
			WLock l(cs);
			auto i = results.find(aResult->getTTH());
			if (i == results.end()) {
				parent = std::make_shared<GroupedSearchResult>(aResult, std::move(*relevanceInfo));
				results.try_emplace(aResult->getTTH(), parent);
				created = true;
			} else {
				parent = i->second;
			}
		}

		if (created) {
			// New parent
			fire(SearchInstanceListener::GroupedResultAdded(), parent);
		} else {
			// Existing parent from now on
			if (!parent->addChildResult(aResult)) {
				return;
			}

			fire(SearchInstanceListener::ChildResultAdded(), parent, aResult);
		}

		fire(SearchInstanceListener::UserResult(), aResult, parent);
	}
}