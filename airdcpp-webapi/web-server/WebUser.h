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

#ifndef DCPLUSPLUS_WEBSERVER_WEBUSER_H
#define DCPLUSPLUS_WEBSERVER_WEBUSER_H

#include "forward.h"
#include <web-server/Access.h>

#include <airdcpp/core/header/typedefs.h>
#include <airdcpp/core/types/GetSet.h>

namespace webserver {
	class WebUser {
	public:
		WebUser(const std::string& aUserName, const std::string& aPasswordHashOrPlain, bool aIsAdmin = false);

		const string& getToken() const noexcept {
			return userName;
		}

		GETSET(std::string, userName, UserName);
		IGETSET(time_t, lastLogin, LastLogin, 0);

		WebUser(WebUser&) = delete;
		WebUser& operator=(WebUser&) = delete;

		// Sessions
		int getActiveSessions() const noexcept {
			return activeSessions;
		}

		void addSession() noexcept {
			activeSessions++;
		}

		void removeSession() noexcept {
			activeSessions--;
		}

		// Access
		bool hasPermission(Access aAccess) const noexcept;

		void setPermissions(const string& aStr) noexcept;
		void setPermissions(const StringList& aPermissions) noexcept;

		string getPermissionsStr() const noexcept;
		static StringList permissionsToStringList(const AccessList& aPermissions) noexcept;
		AccessList getPermissions() const noexcept;
		bool isAdmin() const noexcept;

		const static StringList accessStrings;
		static Access stringToAccess(const string& aStr) noexcept;
		static const string& accessToString(Access aAccess) noexcept;

		int countPermissions() const noexcept;

		static bool validateUsername(const string& aUsername) noexcept;

		bool matchPassword(const string& aPasswordPlain) const noexcept;
		void setPassword(const std::string& aPasswordHashOrPlain) noexcept;
		const string& getPasswordHash() const noexcept {
			return passwordHash;
		}
	private:
		void clearPermissions() noexcept;
		int activeSessions = 0;

		AccessMap permissions;

		static string hashPassword(const string& aPasswordPlain) noexcept;

		string passwordHash;
	};

}

#endif