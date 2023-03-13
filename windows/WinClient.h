/*
 * Copyright (C) 2001-2023 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef WINCLIENT_H
#define WINCLIENT_H

#include "stdafx.h"

#include <airdcpp/DCPlusPlus.h>

class SetupWizard;
class MainFrame;
class WinClient {
public:
	static int run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT);

	// Handle startup arguments
	// Returns false if the application should exit
	static bool checkStartupParams() noexcept;
private:
	static void installExtensions();

	static void listUpdaterFiles(StringPairList& files_, const string& aUpdateFilePath) noexcept;

	static void webErrorF(const string& aError);

	static void initModules();
	static void destroyModules();
	static void unloadModules(StepFunction& aStepF, ProgressFunction&);
	static bool questionF(const string& aStr, bool aIsQuestion, bool aIsError);
	static void splashStrF(const string& str);
	static void splashProgressF(float progress);
	static Callback wizardFGetter(unique_ptr<SetupWizard>& wizard);
	static StartupLoadCallback moduleLoadFGetter(unique_ptr<MainFrame>& wndMain);
};


#endif // !defined(WINCLIENT_H)