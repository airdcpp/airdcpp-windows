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

#include <web-server/stdinc.h>

#include <web-server/Extension.h>

#include <web-server/WebUserManager.h>
#include <web-server/WebServerManager.h>
#include <web-server/WebServerSettings.h>

#include <airdcpp/File.h>


namespace webserver {
	Extension::Extension(const string& aPath, StopF&& aStopF) : stopF(std::move(aStopF)) {
		json packageJson = json::parse(File(aPath + "package" + PATH_SEPARATOR_STR + "package.json", File::READ, File::OPEN).read());

		try {
			const string packageName = packageJson.at("name");
			const string packageEntry = packageJson.at("main");
			const string packageVersion = packageJson.at("version");

			name = packageName;
			entry = packageEntry;
			version = packageVersion;
		} catch (const std::exception& e) {
			throw Exception("Failed to parse package.json: " + string(e.what()));
		}
	}

	void Extension::start(WebServerManager* wsm) {
		auto session = wsm->getUserManager().createExtensionSession(name);
		
		createProcess(wsm, session);
		running = true;
	}

	string Extension::getLaunchCommand(WebServerManager* wsm, const SessionPtr& aSession) const noexcept {
		// Script to launch
		string ret = getPackageDirectory() + entry + " ";

		// Connect URL
		{
			const auto& serverConfig = wsm->isListeningPlain() ? wsm->getPlainServerConfig() : wsm->getTlsServerConfig();

			auto bindAddress = serverConfig.bindAddress.str();
			if (bindAddress.empty()) {
				bindAddress = "localhost";
			}

			ret += wsm->isListeningPlain() ? "ws://" : "wss://";
			ret += bindAddress + ":" + Util::toString(serverConfig.port.num()) + "/api/v1/ ";
		}

		// Session token
		ret += aSession->getAuthToken() + " ";

		// Package directory
		ret += getRootPath();

		return ret;
	}

#ifdef _WIN32
	void Extension::initLog(HANDLE& aHandle, const string& aPath) {
		SECURITY_ATTRIBUTES saAttr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;


		if (Util::fileExists(aPath) && !File::deleteFile(aPath)) {
			dcdebug("Failed to delete the old extension output log %s: %s\n", aPath.c_str(), Util::translateError(::GetLastError()).c_str());
			throw Exception("Failed to delete the old extension output log");
		}

		aHandle = CreateFile(Text::toT(aPath).c_str(),
			FILE_APPEND_DATA,
			FILE_SHARE_WRITE | FILE_SHARE_READ,
			&saAttr,
			CREATE_NEW,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (aHandle == INVALID_HANDLE_VALUE) {
			dcdebug("Failed to create extension output log %s: %s\n", aPath.c_str(), Util::translateError(::GetLastError()).c_str());
			throw Exception("Failed to create extension output log");
		}
	}

	void Extension::closeLog(HANDLE& aHandle) {
		if (aHandle != INVALID_HANDLE_VALUE) {
			CloseHandle(aHandle);
			aHandle = INVALID_HANDLE_VALUE;
		}
	}

	void Extension::createProcess(WebServerManager* wsm, const SessionPtr& aSession) {
		// Setup log file for console output
		initLog(messageLogHandle, getMessageLogPath());
		initLog(errorLogHandle, getErrorLogPath());

		// Set streams
		STARTUPINFO siStartInfo;
		ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
		siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		siStartInfo.hStdOutput = messageLogHandle;
		siStartInfo.hStdError = errorLogHandle;

		ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

		// Start the process
		tstring command = _T("node ") + Text::toT(getLaunchCommand(wsm, aSession));
		dcdebug("Starting extension %s, command %s\n", name.c_str(), Text::fromT(command).c_str());

#ifdef _DEBUG
		// Show the console window in debug mode
		// The connection may stay alive indefinitely when the process is killed 
		// and the extension will not quit until the ping fails
		DWORD flags = 0;
		//siStartInfo.wShowWindow = SW_MINIMIZE;
#else
		DWORD flags = CREATE_NO_WINDOW;
#endif

		auto res = CreateProcess(
			NULL,
			(LPWSTR)command.c_str(),
			0,
			0,
			TRUE,
			flags,
			0,
			NULL,
			&siStartInfo,
			&piProcInfo
		);

		if (res == 0) {
			dcdebug("Failed to start the extension process: %s (code %d)\n", Util::translateError(::GetLastError()).c_str(), res);
			throw Exception("Failed to create process for the extension");
		}

		CloseHandle(piProcInfo.hThread);

		// Monitor the running state of the script
		timer = wsm->addTimer([this, wsm] { checkRunningState(wsm); }, 2500);
		timer->start(false);
	}

	bool Extension::stop() noexcept {
		if (!isRunning()) {
			return true;
		}

		timer->stop(false);

		if (TerminateProcess(piProcInfo.hProcess, 0) == 0) {
			dcdebug("Failed to terminate the extension %s: %s\n", name.c_str(), Util::translateError(::GetLastError()).c_str());
			dcassert(0);
			return false;
		}

		WaitForSingleObject(piProcInfo.hProcess, 5000);

		onStopped(false);
		return true;
	}

	void Extension::onStopped(bool aFailed) noexcept {
		closeLog(messageLogHandle);
		closeLog(errorLogHandle);

		if (piProcInfo.hProcess != INVALID_HANDLE_VALUE) {
			CloseHandle(piProcInfo.hProcess);
			piProcInfo.hProcess = INVALID_HANDLE_VALUE;
		}

		running = false;
		if (stopF) {
			stopF(this, aFailed);
		}
	}

	void Extension::checkRunningState(WebServerManager* wsm) noexcept {
		DWORD exitCode = 0;
		if (GetExitCodeProcess(piProcInfo.hProcess, &exitCode) != 0) {
			if (exitCode != STILL_ACTIVE) {
				dcdebug("Extension %s exited with code %d\n", name.c_str(), exitCode);
				timer->stop(false);
				onStopped(true);
			}
		} else {
			dcdebug("Failed to check running state of extension %s (%s)\n", name.c_str(), Util::translateError(::GetLastError()).c_str());
			dcassert(0);
		}
	}
#else
	void Extension::createProcess(WebServerManager* wsm, const SessionPtr& aSession) {
		t.reset(new std::thread([this]() { run(); }));
	}

	void Extension::run() {
		char buff[512];

		string command = "node " + getLaunchCommand(wsm, aSession);
		if (!(procHandle = popen(command.c_str(), "r"))) {
			return;
		}

		while (fgets(buff, 512, handle)) {
			printf(buff);
		}

		if (stopF) {
			stopF(this, aFailed);
		}
	}

	void Extension::stop() noexcept {
		if (pclose(procHandle) != 0) {
			dcdebug("Failed to terminate the extension %s: %s\n", name.c_str(), Util::translateError(::GetLastError()).c_str());
			return;
		}

		t->join();
		t.reset(nullptr);
	}

#endif
}