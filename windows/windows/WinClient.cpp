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

#include <windows/WinClient.h>

#include <windows/MainFrm.h>
#include <windows/EmoticonsManager.h>
#include <windows/PopupManager.h>
#include <windows/Resource.h>
#include <windows/ResourceLoader.h>
#include <windows/SplashWindow.h>
#include <windows/util/WinUtil.h>
#include <windows/settings/wizard/Wizard.h>

#include <airdcpp/events/LogManager.h>
#include <airdcpp/util/PathUtil.h>
#include <airdcpp/core/version.h>

#include <airdcpp/modules/ADLSearch.h>
#include <airdcpp/modules/AutoSearchManager.h>
#include <airdcpp/modules/FinishedManager.h>
#include <airdcpp/modules/HighlightManager.h>
#include <airdcpp/modules/HublistManager.h>
#include <airdcpp/modules/PreviewAppManager.h>
#include <airdcpp/modules/RSSManager.h>

#include <web-server/ExtensionManager.h>
#include <web-server/NpmRepository.h>
#include <web-server/WebServerManager.h>
#include <web-server/WebServerSettings.h>

#include <future>


namespace wingui {
class CFindDialogMessageFilter : public CMessageFilter
{
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg) {
		if (WinUtil::findDialog) {
			return IsDialogMessage(WinUtil::findDialog, pMsg);
		}
		return FALSE;
	}
};

void WinClient::webErrorF(const string& aError) {
	LogManager::getInstance()->message(aError, LogMessage::SEV_ERROR, STRING(WEB_SERVER));
};

void WinClient::initModules() {
	ADLSearchManager::newInstance();
	HighlightManager::newInstance();
	FinishedManager::newInstance();
	AutoSearchManager::newInstance();
	RSSManager::newInstance();
	PreviewAppManager::newInstance();
	HublistManager::newInstance();

	webserver::WebServerManager::newInstance();
}

void WinClient::destroyModules() {
	webserver::WebServerManager::deleteInstance();

	ADLSearchManager::deleteInstance();
	HublistManager::deleteInstance();
	PreviewAppManager::deleteInstance();
	HighlightManager::deleteInstance();
	AutoSearchManager::deleteInstance();
	RSSManager::deleteInstance();
	FinishedManager::deleteInstance();
}

void WinClient::unloadModules(StepFunction& aStepF, ProgressFunction&) {
	AutoSearchManager::getInstance()->save();
	RSSManager::getInstance()->save(true);
	ADLSearchManager::getInstance()->save(true);

	aStepF(STRING(WEB_SERVER));
	webserver::WebServerManager::getInstance()->stop();
	webserver::WebServerManager::getInstance()->save(webErrorF);
}

bool WinClient::questionF(const string& aStr, bool aIsQuestion, bool aIsError) {
	auto ret = ::MessageBox(WinUtil::splash->m_hWnd, Text::toT(aStr).c_str(), Text::toT(shortVersionString).c_str(), MB_SETFOREGROUND | (aIsQuestion ? MB_YESNO : MB_OK) | (aIsError ? MB_ICONEXCLAMATION : MB_ICONQUESTION));
	return aIsQuestion ? ret == IDYES : true;
}

void WinClient::splashStrF(const string& str) {
	WinUtil::splash->update(str);
}

void WinClient::splashProgressF(float progress) {
	WinUtil::splash->update(progress);
}

Callback WinClient::wizardFGetter(unique_ptr<SetupWizard>& wizard) {
	return [&]() {
		Semaphore s;
		WinUtil::splash->callAsync([&] {
			wizard = make_unique<SetupWizard>(true);
			if (wizard->DoModal(/*m_hWnd*/) != IDOK) {
				// The wizard was cancelled
				wizard.reset(nullptr);
			}
			s.signal();
		});

		// Wait for the wizard to finish
		s.wait();
	};
}

StartupLoadCallback WinClient::moduleLoadFGetter(unique_ptr<MainFrame>& wndMain) {
	return [&](StartupLoader& aLoader) {
		// Core modules
		AutoSearchManager::getInstance()->load();
		RSSManager::getInstance()->load();
		ADLSearchManager::getInstance()->load();

		// Web server
		auto wsm = webserver::WebServerManager::getInstance();
		WEBCFG(PLAIN_BIND).setDefaultValue("127.0.0.1");
		WEBCFG(TLS_BIND).setDefaultValue("127.0.0.1");
		if (wsm->load(webErrorF)) {
			aLoader.stepF(STRING(WEB_SERVER));

			// Determine config
			auto webResourcePath = startupParams.getValue("--web-resources");
#ifdef _DEBUG
			wsm->setEnableSocketLogging(true);
#endif

			// Run
			auto started = wsm->startup(
				webErrorF,
				webResourcePath ? *webResourcePath : Util::emptyString,
				[&wndMain]() { wndMain->shutdown(); }
			);

			wsm->waitExtensionsLoaded();
			if (started) {
				LogManager::getInstance()->message(STRING(WEB_SERVER_STARTED), LogMessage::SEV_INFO, STRING(WEB_SERVER));
			}
		}
	};
}

#define SHARE_MONITORING_EXT "airdcpp-share-monitor"
#define RELEASE_VALIDATOR_EXT "airdcpp-release-validator"

void WinClient::installExtensions() {
	decltype(auto) extMgr = webserver::WebServerManager::getInstance()->getExtensionManager();

	// Share monitoring
	{
		auto incomingSet = SettingsManager::getInstance()->isKeySet(SettingsManager::INCOMING_REFRESH_TIME);
		auto normalSet = SettingsManager::getInstance()->isKeySet(SettingsManager::AUTO_REFRESH_TIME);

		if (!incomingSet || !normalSet) {
			if (!extMgr.getExtension(SHARE_MONITORING_EXT)) {
				extMgr.getNpmRepository().install(SHARE_MONITORING_EXT);
			}

			if (!incomingSet) {
				SettingsManager::getInstance()->set(SettingsManager::INCOMING_REFRESH_TIME, 0);
			}

			if (!normalSet) {
				SettingsManager::getInstance()->set(SettingsManager::AUTO_REFRESH_TIME, 0);
			}
		}
	}

	// Share scanner
	if (SETTING(SETTINGS_PROFILE) == SettingsManager::PROFILE_RAR && !extMgr.getExtension(RELEASE_VALIDATOR_EXT)) {
		extMgr.getNpmRepository().install("airdcpp-release-validator");
	}
}

bool WinClient::checkStartupParams() noexcept {
	LPTSTR* argv = __targv;
	int argc = --__argc;

	// Ignore the app path (first param)
	argv++;

	while (argc > 0) {
		startupParams.addParam(Text::fromT(*argv));
		argc--;
		argv++;
	}

	if (updater.isUpdaterAction(startupParams)) {
		return false;
	}

	return true;
}

int WinClient::run(LPTSTR /*lpstrCmdLine*/, int nCmdShow) {
	static unique_ptr<MainFrame> wndMain;

	CMessageLoop theLoop;

	CFindDialogMessageFilter findDialogFilter;
	theLoop.AddMessageFilter(&findDialogFilter);
	_Module.AddMessageLoop(&theLoop);

	WinUtil::preInit();

	SplashWindow::create();
	WinUtil::splash->update("Starting up");

	auto loader = std::async(std::launch::async, [&] {
		unique_ptr<SetupWizard> wizard = nullptr;
		try {
			startup(
				splashStrF,
				questionF,
				wizardFGetter(wizard),
				splashProgressF,
				initModules,
				moduleLoadFGetter(wndMain)
			);

			WinUtil::splash->update(STRING(LOADING_GUI));

			updater.reportPostInstall(startupParams);
		} catch (const AbortException&) {
			// Critical error, exit
			ExitProcess(1);
		} catch (const std::exception& e) {
			// Crashed
			dcassert(0);
			::MessageBox(WinUtil::mainWnd, Text::toT("Error happened while initializing the application (" + string(e.what()) + "). AirDC++ will now exit.").c_str(), _T("AirDC++ has crashed"), MB_OK);
			ExitProcess(1);
		}

		// Apply share changes from wizard (if available)
		if (wizard) {
			PropPage::TaskList tasks;
			wizard->getTasks(tasks);
			for (auto& t : tasks) {
				t();
			}
			wizard.reset(nullptr);
			SettingsManager::getInstance()->save();
		}

		PopupManager::newInstance();

		WinUtil::splash->callAsync([=] {
			if (SETTING(PASSWD_PROTECT) && !WinUtil::checkClientPassword()) {
				ExitProcess(1);
			}

			if (ResourceManager::getInstance()->isRTL()) {
				SetProcessDefaultLayout(LAYOUT_RTL);
			}

			wndMain = make_unique<MainFrame>(
				startupParams, 
				[this](auto file) { updater.addUpdate(file); }
			);

			CRect rc = wndMain->rcDefault;
			if ((SETTING(MAIN_WINDOW_POS_X) != CW_USEDEFAULT) &&
				(SETTING(MAIN_WINDOW_POS_Y) != CW_USEDEFAULT) &&
				(SETTING(MAIN_WINDOW_SIZE_X) != CW_USEDEFAULT) &&
				(SETTING(MAIN_WINDOW_SIZE_Y) != CW_USEDEFAULT)) {

				rc.left = SETTING(MAIN_WINDOW_POS_X);
				rc.top = SETTING(MAIN_WINDOW_POS_Y);
				rc.right = rc.left + SETTING(MAIN_WINDOW_SIZE_X);
				rc.bottom = rc.top + SETTING(MAIN_WINDOW_SIZE_Y);
				// Now, let's ensure we have sane values here...
				if ((rc.left < 0) || (rc.top < 0) || (rc.right - rc.left < 10) || ((rc.bottom - rc.top) < 10)) {
					rc = wndMain->rcDefault;
				}
			}

			if (wndMain->CreateEx(NULL, rc, 0, ResourceManager::getInstance()->isRTL() ? WS_EX_RTLREADING : 0 | WS_EX_APPWINDOW | WS_EX_WINDOWEDGE) == NULL) {
				ATLTRACE(_T("Main window creation failed!\n"));
				//throw("Main window creation failed");
				return;
			}

			// Must be initialized after the main frame
			EmoticonsManager::newInstance();

			if (SETTING(MINIMIZE_ON_STARTUP)) {
				wndMain->ShowWindow(SW_SHOWMINIMIZED);
			} else {
				WinUtil::isAppActive = true;
				wndMain->ShowWindow(((nCmdShow == SW_SHOWDEFAULT) || (nCmdShow == SW_SHOWNORMAL)) ? SETTING(MAIN_WINDOW_STATE) : nCmdShow);
			}

			// Install replacement extensions for previously inbuilt functionality when migrating from a previous version
			auto configVersion = Util::toDouble(SETTING(CONFIG_VERSION));
			if (configVersion == 3.70 || configVersion == 3.71) {
				wndMain->addThreadedTask([this] {
					Thread::sleep(3000);
					installExtensions();
				});
			}
		});
	});

	int nRet = theLoop.Run();

	dcassert(WinUtil::splash);
	loader = std::async(std::launch::async, [=] {
		PopupManager::deleteInstance();
		EmoticonsManager::deleteInstance();

		shutdown(
			splashStrF,
			splashProgressF,
			unloadModules,
			destroyModules
		);

		WinUtil::splash->callAsync([=] { PostQuitMessage(0); });
	});

	nRet = theLoop.Run();
	_Module.RemoveMessageLoop();

	wndMain.reset(nullptr);
	WinUtil::splash->destroy();

	updater.runPendingUpdate(startupParams);
	return nRet;
}
}