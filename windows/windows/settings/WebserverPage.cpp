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


#include <windows/stdafx.h>
#include <windows/resource.h>

#include <windows/dialog/LineDlg.h>
#include <windows/HttpLinks.h>
#include <windows/MainFrm.h>
#include <windows/settings/WebserverPage.h>
#include <windows/settings/WebUserDlg.h>
#include <windows/util/FormatUtil.h>

#include <web-server/FileServer.h>
#include <web-server/HttpManager.h>
#include <web-server/WebServerSettings.h>
#include <web-server/WebUserManager.h>

#include <airdcpp/events/LogManager.h>
#include <airdcpp/util/NetworkUtil.h>

namespace wingui {

PropPage::TextItem WebServerPage::texts[] = {
	{ IDC_WEBSERVER_ADD_USER,					ResourceManager::ADD },
	{ IDC_WEBSERVER_REMOVE_USER,				ResourceManager::REMOVE },
	{ IDC_WEBSERVER_CHANGE,						ResourceManager::SETTINGS_CHANGE },

	{ IDC_ADMIN_ACCOUNTS,						ResourceManager::ADMIN_ACCOUNTS },
	{ IDC_WEBSERVER_PORT,						ResourceManager::WEB_SERVER_PORT },
	{ IDC_WEBSERVER_TLSPORT,					ResourceManager::WEB_SERVER_PORT },
	{ IDC_WEB_SERVER_USERS_NOTE,				ResourceManager::WEB_ACCOUNTS_NOTE },

	{ IDC_WEB_SERVER_HELP,						ResourceManager::WEB_SERVER_HELP },
	{ IDC_LINK,									ResourceManager::MORE_INFORMATION },
	{ IDC_SERVER_STATE,							ResourceManager::SERVER_STATE },

	{ IDC_SETTINGS_BIND_ADDRESS_HTTP_LABEL,		ResourceManager::WEB_CFG_BIND_ADDRESS },
	{ IDC_SETTINGS_BIND_ADDRESS_HTTPS_LABEL,	ResourceManager::SETTINGS_BIND_ADDRESS },
	{ 0, ResourceManager::LAST }
};

WebServerPage::WebServerPage(SettingsManager *s) : PropPage(s), webMgr(webserver::WebServerManager::getInstance()) {
	SetTitle(CTSTRING(WEB_SERVER));
};

void WebServerPage::initBindAddresses() noexcept {
	// Network interfaces
	const auto insertProtocolAdapters = [this](bool v6) {
		auto protocolAdapters = NetworkUtil::getNetworkAdapters(v6);

		copy(protocolAdapters.begin(), protocolAdapters.end(), back_inserter(bindAdapters));
	};

	insertProtocolAdapters(false);
	insertProtocolAdapters(true);
	sort(bindAdapters.begin(), bindAdapters.end(), NetworkUtil::adapterSort);

	// Any/localhost (reverse order)
	bindAdapters.emplace(bindAdapters.begin(), "Localhost", "::1", static_cast<uint8_t>(0));
	bindAdapters.emplace(bindAdapters.begin(), "Localhost", "127.0.0.1", static_cast<uint8_t>(0));
	bindAdapters.emplace(bindAdapters.begin(), STRING(ANY), Util::emptyString, static_cast<uint8_t>(0));

	// Ensure that we have the current values
	NetworkUtil::ensureBindAddress(bindAdapters, WEBCFG(PLAIN_BIND).str());
	NetworkUtil::ensureBindAddress(bindAdapters, WEBCFG(TLS_BIND).str());

	// Combo
	WinUtil::insertBindAddresses(bindAdapters, ctrlBindHttp, WEBCFG(PLAIN_BIND).str());
	WinUtil::insertBindAddresses(bindAdapters, ctrlBindHttps, WEBCFG(TLS_BIND).str());
}

LRESULT WebServerPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);

	::SetWindowText(GetDlgItem(IDC_WEBSERVER_PORT), WinUtil::toStringW(WEBCFG(PLAIN_PORT).num()).c_str());
	::SetWindowText(GetDlgItem(IDC_WEBSERVER_TLSPORT), WinUtil::toStringW(WEBCFG(TLS_PORT).num()).c_str());

	ctrlTlsPort.Attach(GetDlgItem(IDC_WEBSERVER_TLSPORT));
	ctrlPort.Attach(GetDlgItem(IDC_WEBSERVER_PORT));

	// Bind address
	ctrlBindHttp.Attach(GetDlgItem(IDC_BIND_ADDRESS_HTTP));
	ctrlBindHttps.Attach(GetDlgItem(IDC_BIND_ADDRESS_HTTPS));
	initBindAddresses();

	url.SubclassWindow(GetDlgItem(IDC_LINK));
	url.SetHyperLinkExtendedStyle(HLINK_UNDERLINEHOVER);

	// TODO: add better help link
	url.SetHyperLink(HttpLinks::webServerHelp.c_str());
	url.SetLabel(CTSTRING(MORE_INFORMATION));

	ctrlWebUsers.Attach(GetDlgItem(IDC_WEBSERVER_USERS));
	ctrlWebUsers.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);

	CRect rc;
	ctrlWebUsers.GetClientRect(rc);
	ctrlWebUsers.InsertColumn(0, CTSTRING(USERNAME), LVCFMT_LEFT, rc.Width() / 2, 0);
	ctrlWebUsers.InsertColumn(1, CTSTRING(LAST_SEEN), LVCFMT_LEFT, rc.Width() / 2, 1);

	ctrlRemove.Attach(GetDlgItem(IDC_WEBSERVER_REMOVE_USER));
	ctrlAdd.Attach(GetDlgItem(IDC_WEBSERVER_ADD_USER));
	ctrlChange.Attach(GetDlgItem(IDC_WEBSERVER_CHANGE));
	ctrlStart.Attach(GetDlgItem(IDC_WEBSERVER_START));

	ctrlStatus.Attach(GetDlgItem(IDC_WEBSERVER_STATUS));
	ctrlStatus.Subclass();	
	ctrlStatus.SetFont(WinUtil::font);
	ctrlStatus.SetBackgroundColor(WinUtil::bgColor);
	ctrlStatus.SetDefaultCharFormat(WinUtil::m_ChatTextGeneral);

	ctrlStatus.SetAutoURLDetect(true);
	ctrlStatus.SetEventMask(ctrlStatus.GetEventMask() | ENM_LINK);

	updateState(webMgr->isRunning() ? STATE_STARTED : STATE_STOPPED);

	webUserList = webMgr->getUserManager().getUsers();
	for (const auto& u : webUserList) {
		if (!u->isAdmin()) {
			continue;
		}

		addListItem(u);
	}

	webMgr->addListener(this);
	return TRUE;
}

bool WebServerPage::applySettings() noexcept {
	bool needsRestart = false;
	decltype(auto) webSettings = webMgr->getSettingsManager();

	{
		auto plainserverPort = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlPort)));
		if (plainserverPort != WEBCFG(PLAIN_PORT).num()) {
			webSettings.setValue(WEBCFG(PLAIN_PORT), plainserverPort);
			needsRestart = true;
		}
	}

	{
		auto tlsServerPort = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlTlsPort)));
		if (tlsServerPort != WEBCFG(TLS_PORT).num()) {
			webSettings.setValue(WEBCFG(TLS_PORT), tlsServerPort);
			needsRestart = true;
		}
	}

	{
		const auto sel = ctrlBindHttp.GetCurSel();
		if (sel != -1 && bindAdapters[sel].ip != WEBCFG(PLAIN_BIND).str()) {
			webSettings.setValue(WEBCFG(PLAIN_BIND), bindAdapters[sel].ip);
			needsRestart = true;
		}
	}

	{
		const auto sel = ctrlBindHttps.GetCurSel();
		if (sel != -1 && bindAdapters[sel].ip != WEBCFG(TLS_BIND).str()) {
			webSettings.setValue(WEBCFG(TLS_BIND), bindAdapters[sel].ip);
			needsRestart = true;
		}
	}

	webMgr->getUserManager().replaceWebUsers(webUserList);
	return needsRestart;
}

LRESULT WebServerPage::onServerState(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (!webMgr->isRunning()) {
		ctrlStart.EnableWindow(FALSE);
		lastError.clear();

		applySettings();

		MainFrame::getMainFrame()->addThreadedTask([this] {
			bool started = webMgr->start([this](const string& aError) {
				setLastError(aError);
			});

			if (!started) {
				// There can be endpoint-specific listening errors already
				if (lastError.empty()) {
					setLastError(STRING(WEB_SERVER_INVALID_CONFIG));
				}

				updateState(STATE_STOPPED);
			}
		});

	} else {
		lastError.clear();

		MainFrame::getMainFrame()->addThreadedTask([] {
			webserver::WebServerManager::getInstance()->stop();
		});
	}

	return 0;
}

void WebServerPage::on(webserver::WebServerManagerListener::Started) noexcept {
	callAsync([this] {
		updateState(STATE_STARTED);
	});
}
void WebServerPage::on(webserver::WebServerManagerListener::Stopped) noexcept {
	callAsync([this] {
		updateState(STATE_STOPPED);
	});
}

void WebServerPage::on(webserver::WebServerManagerListener::Stopping) noexcept {
	callAsync([this] {
		updateState(STATE_STOPPING);
	});
}

LRESULT WebServerPage::onChangeUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (ctrlWebUsers.GetSelectedCount() == 1) {
		auto sel = ctrlWebUsers.GetSelectedIndex();

		auto webUser = *getListUser(sel);

		LineDlg dlg;
		dlg.allowEmpty = false;
		dlg.title = TSTRING(CHANGE_PASSWORD);
		dlg.description = TSTRING(PASSWORD);
		if (dlg.DoModal() == IDOK) {
			webUser->setPassword(Text::fromT(dlg.line));
		}
	}

	return 0;
}

LRESULT WebServerPage::onAddUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	WebUserDlg dlg;
	if (dlg.DoModal() == IDOK) {
		if (dlg.getUserName().empty() || dlg.getPassWord().empty()) {
			WinUtil::showMessageBox(TSTRING(WEB_ACCOUNT_INCOMPLETE), MB_ICONEXCLAMATION);
			return 0;
		}

		if (!webserver::WebUser::validateUsername(dlg.getUserName())) {
			WinUtil::showMessageBox(TSTRING(WEB_USERNAME_APLHANUM), MB_ICONEXCLAMATION);
			return 0;
		}

		auto user = make_shared<webserver::WebUser>(dlg.getUserName(), dlg.getPassWord(), true);

		webUserList.push_back(user);
		addListItem(user);
	}

	return 0;
}

LRESULT WebServerPage::onRemoveUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (ctrlWebUsers.GetSelectedCount() == 1) {
		int sel = ctrlWebUsers.GetSelectedIndex();

		webUserList.erase(getListUser(sel));
		ctrlWebUsers.DeleteItem(sel);
	}

	return 0;
}

void WebServerPage::setLastError(const string& aError) noexcept {
	callAsync([aError, this] {
		lastError = Text::toT(aError);
	});
}

LRESULT WebServerPage::onSelChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
	if (ctrlWebUsers.GetSelectedCount() == 1) {
		ctrlChange.EnableWindow(TRUE);
		ctrlRemove.EnableWindow(TRUE);
	} else {
		ctrlChange.EnableWindow(FALSE);
		ctrlRemove.EnableWindow(FALSE);
	}
	return 0;
}
LRESULT WebServerPage::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	auto kd = (NMLVKEYDOWN*)pnmh;
	switch (kd->wVKey) {
	case VK_INSERT:
		PostMessage(WM_COMMAND, IDC_WEBSERVER_ADD_USER, 0);
		break;
	case VK_DELETE:
		PostMessage(WM_COMMAND, IDC_WEBSERVER_REMOVE_USER, 0);
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}

LRESULT WebServerPage::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	auto item = (NMITEMACTIVATE*)pnmh;

	if (item->iItem >= 0) {
		PostMessage(WM_COMMAND, IDC_WEBSERVER_CHANGE, 0);
	}
	else if (item->iItem == -1) {
		PostMessage(WM_COMMAND, IDC_WEBSERVER_ADD_USER, 0);
	}

	return 0;
}

void WebServerPage::updateState(ServerState aNewState) noexcept {
	tstring statusText;
	if (aNewState == STATE_STARTED) {
		ctrlStart.SetWindowText(CTSTRING(STOP));

		if (!lastError.empty()) {
			statusText += lastError + _T("\n");
		}

		statusText += TSTRING(WEB_SERVER_RUNNING);

		if (File::isDirectory(webMgr->getHttpManager().getFileServer().getResourcePath())) {
			statusText += _T(". ") + TSTRING_F(WEB_UI_ACCESS_URL, Text::toT(webMgr->getLocalServerHttpUrl()));
		}
	} else if(aNewState == STATE_STOPPING) {
		statusText += TSTRING(STOPPING);
		//ctrlStatus.SetWindowText(_T("Stopping..."));
	} else if (aNewState == STATE_STOPPED) {
		ctrlStart.SetWindowText(CTSTRING(START));

		if (lastError.empty()) {
			statusText += TSTRING(WEB_SERVER_STOPPED);
		} else {
			statusText += TSTRING_F(WEB_SERVER_START_FAILED, lastError.c_str());
		}
	}

	ctrlStatus.handleEditClearAll();
	ctrlStatus.AppendMessage(Message::fromText(Text::fromT(statusText), LogMessage::InitFlags::INIT_DISABLE_TIMESTAMP), WinUtil::m_ChatTextGeneral, false);
	ctrlStatus.RedrawWindow();


	ctrlStart.EnableWindow(aNewState == STATE_STOPPING ? FALSE : TRUE);
	currentState = aNewState;
}

void WebServerPage::write() {
	needsRestartTask = applySettings();
}

void WebServerPage::addListItem(const webserver::WebUserPtr& aUser) noexcept {
	int p = ctrlWebUsers.insert(ctrlWebUsers.GetItemCount(), Text::toT(aUser->getUserName()), 0, reinterpret_cast<LPARAM>(aUser.get()));

	auto login = aUser->getLastLogin() == 0 ? STRING(NEVER) : FormatUtil::formatDateTime(aUser->getLastLogin());
	ctrlWebUsers.SetItemText(p, 1, Text::toT(login).c_str());
}

webserver::WebUserList::iterator WebServerPage::getListUser(int aPos) noexcept {
	auto data = reinterpret_cast<webserver::WebUser*>(ctrlWebUsers.GetItemData(aPos));

	return ranges::find_if(webUserList, [&data](const webserver::WebUserPtr& aUser) { 
		return aUser.get() == data; 
	});
}

Dispatcher::F WebServerPage::getThreadedTask() {
	return Dispatcher::F([this] {
		const auto errorF = [](const string& aError) {
			LogManager::getInstance()->message(aError, LogMessage::SEV_ERROR, STRING(WEB_SERVER));
		};

		webMgr->save(errorF);

		if (needsRestartTask) {
			if (webMgr->isRunning()) {
				webMgr->stop();
			}

			webMgr->start(errorF);
		}
	});
}

WebServerPage::~WebServerPage() {
	webMgr->removeListener(this);
	webUserList.clear();
	ctrlWebUsers.Detach();
};
}