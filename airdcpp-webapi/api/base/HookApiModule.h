/*
* Copyright (C) 2011-2017 AirDC++ Project
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

#ifndef DCPLUSPLUS_DCPP_HOOK_APIMODULE_H
#define DCPLUSPLUS_DCPP_HOOK_APIMODULE_H

#include <web-server/stdinc.h>

#include <web-server/Access.h>
#include <web-server/SessionListener.h>

#include <api/ApiModule.h>

#include <airdcpp/CriticalSection.h>

namespace webserver {
	class HookApiModule : public SubscribableApiModule {
	public:
		struct HookCompletionData {
			HookCompletionData(const json& aJson);

			json data;

			string errorId;
			string errorMessage;
			bool hasError() const noexcept {
				return !errorId.empty();
			}
		};
		typedef std::shared_ptr<HookCompletionData> HookCompletionDataPtr;

		typedef std::function<bool(const string& aSubscriberId, const string& aSubscriberName)> HookAddF;
		typedef std::function<void(const string& aSubscriberId)> HookRemoveF;

		class HookSubscriber {
		public:
			HookSubscriber(HookAddF&& aAddHandler, HookRemoveF&& aRemoveF) : addHandler(std::move(aAddHandler)), removeHandler(aRemoveF) {}

			bool enable(const json& aJson);
			void disable();

			bool isActive() const noexcept {
				return active;
			}

			const string& getSubscriberId() const noexcept {
				return subscriberId;
			}
		private:
			bool active = false;

			const HookAddF addHandler;
			const HookRemoveF removeHandler;
			string subscriberId;
		};

		HookApiModule(Session* aSession, Access aSubscriptionAccess, const StringList* aSubscriptions = nullptr);

		virtual void createHook(const string& aSubscription, HookAddF&& aAddHandler, HookRemoveF&& aRemoveF) noexcept;
		virtual bool hookActive(const string& aSubscription) const noexcept;

		virtual HookCompletionDataPtr fireHook(const string& aSubscription, const json& aJson);
	protected:
		HookSubscriber& getHookSubscriber(ApiRequest& aRequest);

		virtual void on(SessionListener::SocketDisconnected) noexcept override;

		virtual api_return handleAddHook(ApiRequest& aRequest);
		virtual api_return handleRemoveHook(ApiRequest& aRequest);
		virtual api_return handleCompleteHook(ApiRequest& aRequest);
	private:
		typedef map<int, HookCompletionDataPtr> PendingHookActionMap;
		PendingHookActionMap pendingHookActions;

		map<string, HookSubscriber> hooks;

		int pendingHookIdCounter = 1;
		mutable SharedMutex cs;
	};

	typedef std::unique_ptr<ApiModule> HandlerPtr;
}

#endif