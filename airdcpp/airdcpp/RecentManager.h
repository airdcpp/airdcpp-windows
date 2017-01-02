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

#ifndef RECENT_MANAGER_H
#define RECENT_MANAGER_H

#include "RecentEntry.h"
#include "Singleton.h"
#include "Speaker.h"

#include "RecentManagerListener.h"


namespace dcpp {
	class RecentManager : public Speaker<RecentManagerListener>, public Singleton<RecentManager>
	{
	public:
		// Recent Hubs
		RecentHubEntryList getRecentHubs() noexcept {  RLock l(cs); return recentHubs; };

		void addRecentHub(const string& aEntry) noexcept;
		void removeRecentHub(const string& aEntry) noexcept;
		void updateRecentHub(const ClientPtr& aClient) noexcept;

		RecentHubEntryPtr getRecentHubEntry(const string& aServer) const noexcept;
		RecentHubEntryList searchRecentHubs(const string& aPattern, size_t aMaxResults) const noexcept;

		void clearRecentHubs() noexcept;
		void saveRecentHubs() const noexcept;

		void load() noexcept;

		mutable SharedMutex cs;
	private:
		RecentHubEntryList recentHubs;
		friend class Singleton<RecentManager>;

		RecentManager();
		~RecentManager();

		void loadRecentHubs(SimpleXML& aXml);
	};

} // namespace dcpp

#endif // !defined(RECENT_MANAGER_H)