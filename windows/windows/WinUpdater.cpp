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

#include <windows/stdafx.h>

#include <windows/WinUpdater.h>

#include <windows/OSUtil.h>
#include <windows/ShellExecAsUser.h>
#include <windows/SplashWindow.h>
#include <windows/WinUtil.h>

#include <airdcpp/LogManager.h>
#include <airdcpp/PathUtil.h>
#include <airdcpp/SystemUtil.h>
#include <airdcpp/Text.h>
#include <airdcpp/UpdateDownloader.h>
#include <airdcpp/ValueGenerator.h>
#include <airdcpp/ZipFile.h>

#include <airdcpp/updater/Updater.h>
#include <airdcpp/updater/UpdaterCreator.h>

#include <web-server/WebServerSettings.h>


namespace wingui {

WinUpdater::WinUpdater() {

}

void updateCreatorErrorF(const string& aMessage) {
	::MessageBox(NULL, Text::toT(aMessage).c_str(), _T(""), MB_OK | MB_ICONERROR);
};


void WinUpdater::listUpdaterFiles(StringPairList& files_, const string& aUpdateFilePath) noexcept {
	auto assertFiles = [](const string& title, int minExpected, int added) {
		if (added < minExpected) {
			::MessageBox(
				WinUtil::splash->m_hWnd,
				Text::toT(title + ": added " + Util::toString(added) + " files while at least " + Util::toString(minExpected) + " were expected").c_str(),
				_T("Failed to create updater"),
				MB_SETFOREGROUND | MB_OK | MB_ICONEXCLAMATION
			);
			ExitProcess(1);
		}
	};

	// Node
	// Note: secondary executables must be added before the application because of an extraction issue in version before 4.00
	assertFiles("Node.js", 1, ZipFile::CreateZipFileList(files_, PathUtil::getFilePath(AppUtil::getAppFilePath()), Util::emptyString, "^(" + webserver::WebServerSettings::localNodeDirectoryName + ")$"));

	// Application
	assertFiles("Exe", 2, ZipFile::CreateZipFileList(files_, AppUtil::getAppFilePath(), Util::emptyString, "^(AirDC.exe|AirDC.pdb)$"));

	// Additional resources
	auto installerDirectory = PathUtil::getParentDir(aUpdateFilePath) + "installer" + PATH_SEPARATOR;
	assertFiles("Themes", 1, ZipFile::CreateZipFileList(files_, installerDirectory, Util::emptyString, "^(Themes)$"));
	assertFiles("Web resources", 10, ZipFile::CreateZipFileList(files_, installerDirectory, Util::emptyString, "^(Web-resources)$"));
	assertFiles("Emopacks", 10, ZipFile::CreateZipFileList(files_, installerDirectory, Util::emptyString, "^(EmoPacks)$"));
}

void WinUpdater::createUpdater(const StartupParams& aStartupParams) {
	SplashWindow::create();
	WinUtil::splash->update("Creating updater");

	auto updaterFilePath = UpdaterCreator::createUpdate([this](auto&&... args) { listUpdaterFiles(args...); }, updateCreatorErrorF);

	if (aStartupParams.hasParam("/test")) {
		WinUtil::splash->update("Extracting updater");
		auto updaterExeFile = UpdateDownloader::extractUpdater(updaterFilePath, BUILD_NUMBER + 1, Util::toString(ValueGenerator::rand()));

		addUpdate(updaterExeFile);

		{
			auto newStartupParams = aStartupParams;
			newStartupParams.pop_front(); // remove /createupdate
			runPendingUpdate(newStartupParams);
		}

		if (aStartupParams.hasParam("/fail")) {
			WinUtil::splash->update("Testing failure");
			Sleep(15000); // Prevent overwriting the exe
		}
	}

	WinUtil::splash->destroy();
}

bool WinUpdater::installUpdate(const string& aInstallPath) {
	SplashWindow::create();
	WinUtil::splash->update("Updating");
	string sourcePath = AppUtil::getAppFilePath();

	SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);

	bool success = false;

	{
		auto logger = Updater::createInstallLogger(sourcePath);
		for (;;) {
			string error;
			success = Updater::applyUpdate(sourcePath, aInstallPath, error, 10, logger);
			if (!success) {
				auto message = Text::toT("Updating failed:\n\n" + error + "\n\nDo you want to retry installing the update?");
				if (::MessageBox(NULL, message.c_str(), Text::toT(shortVersionString).c_str(), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_TOPMOST) == IDYES) {
					continue;
				}
			} else {
				updateRegistry(aInstallPath, logger);
			}
			break;
		}
	}

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	WinUtil::splash->destroy();

	return success;
}

void WinUpdater::updateRegistry(const string& aApplicationPath, Updater::FileLogger& logger_) {
	// Update the version in the registry
	HKEY hk;
	TCHAR Buf[512];
	Buf[0] = 0;

#ifdef _WIN64
	string regkey = "SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\AirDC++\\";
	int flags = KEY_WRITE | KEY_QUERY_VALUE | KEY_WOW64_64KEY;
#else
	string regkey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\AirDC++\\";
	int flags = KEY_WRITE | KEY_QUERY_VALUE;
#endif

	auto err = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, Text::toT(regkey).c_str(), 0, flags, &hk);
	if(err == ERROR_SUCCESS) {
		DWORD bufLen = sizeof(Buf);
		DWORD type;
		::RegQueryValueEx(hk, _T("InstallLocation"), 0, &type, (LPBYTE)Buf, &bufLen);

		if(Util::stricmp(Text::toT(aApplicationPath).c_str(), Buf) == 0) {
			::RegSetValueEx(hk, _T("DisplayVersion"), 0, REG_SZ, (LPBYTE) Text::toT(shortVersionString).c_str(), sizeof(TCHAR) * (shortVersionString.length() + 1));
			logger_.log("Registry key was updated successfully");
		} else {
			logger_.log("Skipping updating of registry key (existing key is for a different installation)");
		}

		::RegCloseKey(hk);
	} else {
		logger_.log("Failed to update registry key: " + SystemUtil::translateError(err));
	}
}

bool WinUpdater::isUpdaterAction(const StartupParams& aStartupParams) noexcept {
	// Thread::sleep(10000);

	if (aStartupParams.hasParam("/createupdate", 0)) {
		createUpdater(aStartupParams);
		return true;
	} else if (aStartupParams.hasParam("/sign", 0)) {
		// AirDC++.exe /sign version_file_path private_key_file_path [-pubout]

		if (aStartupParams.getParams().size() >= 3) {
			auto xmlPath = aStartupParams.getParams()[1];
			auto keyPath = aStartupParams.getParams()[2];
			bool genHeader = aStartupParams.hasParam("-pubout");
			if (PathUtil::fileExists(xmlPath) && PathUtil::fileExists(keyPath)) {
				UpdaterCreator::signVersionFile(xmlPath, keyPath, updateCreatorErrorF, genHeader);
			} else {
				updateCreatorErrorF("Version file/private key file is missing.");
			}
		} else {
			updateCreatorErrorF("Invalid arguments");
		}

		return true;
	} else if (aStartupParams.hasParam("/update", 0)) {
		// AirDC++.exe /update version_file_path private_key_file_path [-pubout]
		if (aStartupParams.size() >= 2) {
			// Install new
			auto installPath = aStartupParams.getParams()[1];
			auto success = installUpdate(installPath);

			{
				auto newStartupParams = aStartupParams;

				// Remove the /update command and path
				newStartupParams.pop_front();
				newStartupParams.pop_front();

				// Add new startup params
				newStartupParams.addParam(success ? "/updated" : "/updatefailed");
				newStartupParams.addParam("/silent");

				// Start it
				startNewInstance(installPath, newStartupParams);
			}
		}

		return true;
	}

	return checkAndCleanUpdaterFiles(aStartupParams);
}

void WinUpdater::startNewInstance(const string& aInstallPath, const StartupParams& aNewStartupParams) const {
	auto path = Text::toT(aInstallPath + AppUtil::getAppFileName());
	auto newStartupParams = Text::toT(aNewStartupParams.formatParams(true));

	if (!aNewStartupParams.hasParam("/elevation")) {
		ShellExecAsUser(NULL, path.c_str(), newStartupParams.c_str(), NULL);
	} else {
		ShellExecute(NULL, NULL, path.c_str(), newStartupParams.c_str(), NULL, SW_SHOWNORMAL);
	}
}

bool WinUpdater::checkAndCleanUpdaterFiles(const StartupParams& aStartupParams) noexcept {
	auto updated = aStartupParams.hasParam("/updated") || aStartupParams.hasParam("/updatefailed");

	if (string updaterFile; Updater::checkAndCleanUpdaterFiles(AppUtil::getAppFilePath(), updaterFile, updated)) {
		addUpdate(updaterFile);
		runPendingUpdate(aStartupParams);
		return true;
	}

	return false;
}

void WinUpdater::reportPostInstall(StartupParams& startupParams_) const noexcept {
	// Report
	if (startupParams_.hasParam("/updated")) {
		UpdateDownloader::log(STRING(UPDATE_SUCCEEDED), LogMessage::SEV_INFO);
		startupParams_.removeParam("/updated");
	} else if (startupParams_.hasParam("/updatefailed")) {
		UpdateDownloader::log(STRING_F(UPDATE_FAILED, Updater::getFinalLogFilePath()), LogMessage::SEV_ERROR);
		startupParams_.removeParam("/updatefailed");
	}
}

void WinUpdater::addUpdate(const string& aUpdater) noexcept {
	auto appPath = AppUtil::getAppFilePath();

	string updateCmd = "/update \"" + appPath + "\\\""; // The extra end slash is required!
	if (OSUtil::isElevated()) {
		updateCmd += " /elevation";
	}

	pendingUpdate = { aUpdater, updateCmd };
}

bool WinUpdater::runPendingUpdate(const StartupParams& aStartupParams) noexcept {
	if (pendingUpdate) {
		dcassert(!pendingUpdate->updaterFilePath.empty() && !pendingUpdate->updateParams.empty());

		auto params = pendingUpdate->updateParams + aStartupParams.formatParams(false);
		ShellExecute(NULL, _T("runas"), Text::toT(pendingUpdate->updaterFilePath).c_str(), Text::toT(params).c_str(), NULL, SW_SHOWNORMAL);
		return true;
	}

	return false;
}

}