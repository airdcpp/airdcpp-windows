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

#ifndef DCPLUSPLUS_PRIORITY_SEARCH_QUEUE_H
#define DCPLUSPLUS_PRIORITY_SEARCH_QUEUE_H

#include "stdinc.h"

#include "SettingsManager.h"

// TODO: replace with STD when MSVC can initialize the distribution correctly
#include <boost/random/discrete_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>

namespace dcpp {

template<class ItemT>
class PrioritySearchQueue {
	typedef vector<double> ProbabilityList;
public:
	void addSearchPrio(ItemT& aItem) noexcept{
		if (aItem->getPriority() < Priority::LOW) {
			return;
		}

		if (aItem->isRecent()) {
			dcassert(find(recentSearchQueue.begin(), recentSearchQueue.end(), aItem) == recentSearchQueue.end());
			recentSearchQueue.push_back(aItem);
			return;
		} else {
			auto& queue = prioSearchQueue[static_cast<int>(aItem->getPriority())];
			dcassert(find(queue.begin(), queue.end(), aItem) == queue.end());
			queue.push_back(aItem);
		}
	}

	void removeSearchPrio(ItemT& aItem) noexcept{
		if (aItem->getPriority() < Priority::LOW) {
			return;
		}

		if (aItem->isRecent()) {
			auto i = find(recentSearchQueue.begin(), recentSearchQueue.end(), aItem);
			if (i != recentSearchQueue.end()) {
				recentSearchQueue.erase(i);
			}
		} else {
			auto& queue = prioSearchQueue[static_cast<int>(aItem->getPriority())];
			auto i = find(queue.begin(), queue.end(), aItem);
			if (i != queue.end()) {
				queue.erase(i);
			}
		}
	}

	ItemT findSearchItem(uint64_t aTick, bool aForce = false) noexcept{
		ItemT ret = nullptr;
		if (aTick >= nextSearch || aForce) {
			ret = findNormal();
		}

		if (!ret && (aTick >= nextRecentSearch || aForce)) {
			ret = findRecent();
		}
		return ret;
	}

	int64_t recalculateSearchTimes(bool aRecent, bool isPrioChange, uint64_t aTick, int aItemCount = 0, int minInterval = SETTING(SEARCH_TIME)) noexcept{
		if (!aRecent) {
			if(aItemCount == 0)
				aItemCount = getPrioSum();

			//start with the min interval
			if(isPrioChange && nextSearch == 0)
				nextSearch = aTick + (minInterval * 60 * 1000);

			if (aItemCount > 0) {
				minInterval = max(60 / aItemCount, minInterval);
			}

			if (nextSearch > 0 && isPrioChange) {
				nextSearch = min(nextSearch, aTick + (minInterval * 60 * 1000));
			} else {
				nextSearch = aTick + (minInterval * 60 * 1000);
			}
			return nextSearch;
		} else {
			//start with the min interval
			if (isPrioChange && nextRecentSearch == 0)
				nextRecentSearch = aTick + (minInterval * 60 * 1000);

			int IntervalMS = getRecentIntervalMs();
			if (nextRecentSearch > 0 && isPrioChange) {
				nextRecentSearch = min(nextRecentSearch, aTick + IntervalMS);
			} else {
				nextRecentSearch = aTick + IntervalMS;
			}
			return nextRecentSearch;
		}
	}



	int getRecentIntervalMs(int aItemCount = 0) const noexcept{

		auto recentItems = aItemCount == 0 ? count_if(recentSearchQueue.begin(), recentSearchQueue.end(), [](const ItemT& aItem) { 
			return aItem->allowAutoSearch(); 
		}) : aItemCount;

		if (recentItems == 1) {
			return 15 * 60 * 1000;
		} else if (recentItems == 2) {
			return 8 * 60 * 1000;
		} else {
			return 5 * 60 * 1000;
		}
	}

private:

	ItemT findRecent() noexcept{
		if (recentSearchQueue.size() == 0) {
			return nullptr;
		}

		uint32_t count = 0;
		for (;;) {
			auto item = recentSearchQueue.front();
			recentSearchQueue.pop_front();

			//check if the item still belongs to here
			if (item->checkRecent()) {
				recentSearchQueue.push_back(item);
			} else {
				addSearchPrio(item);
			}

			if (item->allowAutoSearch()) {
				return item;
			} else if (count >= recentSearchQueue.size()) {
				break;
			}

			count++;
		}

		return nullptr;
	}

	ItemT findNormal() noexcept{
		ProbabilityList probabilities;
		int itemCount = getPrioSum(&probabilities);

		//do we have anything where to search from?
		if (itemCount == 0) {
			return nullptr;
		}

		auto dist = boost::random::discrete_distribution<>(probabilities);

		//choose the search queue, can't be paused or lowest
		auto& sbq = prioSearchQueue[dist(gen) + static_cast<int>(Priority::LOW)];
		dcassert(!sbq.empty());

		//find the first item from the search queue that can be searched for
		auto s = find_if(sbq.begin(), sbq.end(), [](const ItemT& item) { return item->allowAutoSearch(); });
		if (s != sbq.end()) {
			auto item = *s;
			//move to the back
			sbq.erase(s);
			sbq.push_back(item);
			return item;
		}

		return nullptr;
	}

	boost::mt19937 gen;

	int getPrioSum(ProbabilityList* probabilities_ = nullptr) const noexcept{
		int itemCount = 0;
		int p = static_cast<int>(Priority::LOW);
		do {
			int dequeItems = static_cast<int>(count_if(prioSearchQueue[p].begin(), prioSearchQueue[p].end(), [](const ItemT& aItem) { 
				return aItem->allowAutoSearch(); 
			}));

			if (probabilities_)
				(*probabilities_).push_back((p - 1)*dequeItems); //multiply with a priority factor to get bigger probability for items with higher priority
			itemCount += dequeItems;
			p++;
		} while (p < static_cast<int>(Priority::LAST));

		return itemCount;
	}

	/** Search items by priority (low-highest) */
	vector<ItemT> prioSearchQueue[static_cast<int>(Priority::LAST)];
	deque<ItemT> recentSearchQueue;

	/** Next normal search */
	uint64_t nextSearch = 0;
	/** Next recent search */
	uint64_t nextRecentSearch = 0;

	//SettingsManager::IntSetting minSearchInterval;
};

}

#endif