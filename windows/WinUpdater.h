/*
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef WINUPDATER_H
#define WINUPDATER_H

#include "stdafx.h"

#include <airdcpp/StartupParams.h>
#include <airdcpp/updater/Updater.h>

namespace wingui {
class WinUpdater {
public:
	WinUpdater();

	// Add a pending update
	void addUpdate(const string& aUpdaterFile) noexcept;

	// Run an update that was added with addUpdate
	bool runPendingUpdate(const StartupParams& aStartupParams) noexcept;

	// Complete an installed update (log message + remove updater params)
	void reportPostInstall(StartupParams& startupParams_) const noexcept;

	// Handle startup arguments
	// Returns false if the application should exit
	bool isUpdaterAction(const StartupParams& aStartupParams) noexcept;
private:
	static void listUpdaterFiles(StringPairList& files_, const string& aUpdateFilePath) noexcept;

	void createUpdater(const StartupParams& aStartupParams);

	static bool installUpdate(const string& aInstallPath);
	static void updateRegistry(const string& aApplicationPath, Updater::FileLogger& logger_);
	void startNewInstance(const string& aInstallPath, const StartupParams& aNewStartupParams) const;

	// Check from disk whether we have an update pending
	bool checkAndCleanUpdaterFiles(const StartupParams& aStartupParams) noexcept;

	struct PendingUpdate {
		string updaterFilePath;
		string updateParams;
	};

	optional<PendingUpdate> pendingUpdate;
};

}

#endif // !defined(WINCLIENT_H)