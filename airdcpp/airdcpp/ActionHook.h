/*
* Copyright (C) 2001-2017 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_ACTION_HOOK_H
#define DCPLUSPLUS_DCPP_ACTION_HOOK_H

#include "stdinc.h"
#include "forward.h"
#include "typedefs.h"

#include "CriticalSection.h"

#include <vector>


namespace dcpp {
	struct ActionHookRejection {
		ActionHookRejection(const string& aHookId, const string& aHookName, const string& aRejectId, const string& aMessage) :
			hookId(aHookId), hookName(aHookName), rejectId(aRejectId), message(aMessage) {}

		const string hookId;
		const string hookName;
		const string rejectId;
		const string message;

		static string formatError(const ActionHookRejectionPtr& aRejection) noexcept {
			if (!aRejection) return "";
			return aRejection->hookName + ": " + aRejection->message;
		}

		static bool matches(const ActionHookRejectionPtr& aRejection, const string& aHookId, const string& aRejectId) noexcept {
			if (!aRejection) return false;

			return aRejection->hookId == aHookId && aRejection->rejectId == aRejectId;
		}
	};

	template<typename ItemT>
	class ActionHook {
	public:
#define HOOK_HANDLER(func) std::bind(&func, this, placeholders::_1, placeholders::_2)

		typedef std::function<ActionHookRejectionPtr(ItemT& aItem, const HookRejectionGetter& aRejectionGetter)> HookCallback;
		struct Subscriber {
			string id;
			string name;

			HookCallback callback;

			ActionHookRejectionPtr getRejection(const string& aRejectId, const string& aMessage) noexcept {
				return make_shared<ActionHookRejection>(id, name, aRejectId, aMessage);
			}
		};

		bool addSubscriber(const string& aId, const string& aName, HookCallback aCallback) noexcept {
			Lock l(cs);
			if (findById(aId) != subscribers.end()) {
				return false;
			}

			subscribers.push_back({ aId, aName, std::move(aCallback) });
			return true;
		}

		bool removeSubscriber(const string& aId) noexcept {
			Lock l(cs);
			auto i = findById(aId);
			if (i == subscribers.end()) {
				return false;
			}

			subscribers.erase(i);
			return true;
		}

		// Run all validation hooks, returns a rejection object in case of errors
		ActionHookRejectionPtr runHooksError(ItemT& aItem) const noexcept {
			SubscriberList subscribersCopy;

			{
				Lock l(cs);
				subscribersCopy = subscribers;
			}

			for (const auto& handler : subscribersCopy) {
				auto error = handler.callback(aItem, std::bind(&Subscriber::getRejection, handler, std::placeholders::_1, std::placeholders::_2));
				if (error) {
					dcdebug("Hook rejected by handler %s: %s\n", error->hookId.c_str(), error->rejectId.c_str());
					return error;
				}
			}

			return nullptr;
		}

		// Run all validation hooks, returns false in case of rejections
		bool runHooksBasic(ItemT& aItem) const noexcept {
			auto rejection = runHooksError(aItem);
			return rejection ? false : true;
		}

		bool hasSubscribers() const noexcept {
			Lock l(cs);
			return !subscribers.empty();
		}
	private:
		typedef std::vector<Subscriber> SubscriberList;

		typename SubscriberList::iterator findById(const string& aId) noexcept {
			return find_if(subscribers.begin(), subscribers.end(), [&aId](const Subscriber& aSubscriber) {
				return aSubscriber.id == aId;
			});
		}

		SubscriberList subscribers;
		mutable CriticalSection cs;
	};
}

#endif