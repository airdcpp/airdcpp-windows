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

#ifndef WINCLIENT_H
#define WINCLIENT_H

#include "stdafx.h"

#include <airdcpp/DCPlusPlus.h>
#include <airdcpp/StartupParams.h>

#include "WinUpdater.h"

class SetupWizard;
class MainFrame;
class WinClient {
public:
	int run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT);

	// Handle startup arguments
	// Returns false if the application should exit
	bool checkStartupParams() noexcept;
	const StartupParams& getStartupParams() noexcept {
		return startupParams;
	}
private:
	void installExtensions();

	static void initModules();
	static void destroyModules();
	static void unloadModules(StepFunction& aStepF, ProgressFunction&);

	static void webErrorF(const string& aError);
	static bool questionF(const string& aStr, bool aIsQuestion, bool aIsError);
	static void splashStrF(const string& str);
	static void splashProgressF(float progress);

	Callback wizardFGetter(unique_ptr<SetupWizard>& wizard);
	StartupLoadCallback moduleLoadFGetter(unique_ptr<MainFrame>& wndMain);

	StartupParams startupParams;
	WinUpdater updater;
};


#endif // !defined(WINCLIENT_H)