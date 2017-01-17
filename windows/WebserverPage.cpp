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


#include "stdafx.h"
#include "Resource.h"

#include "LineDlg.h"
#include "WebServerPage.h"
#include "WebUserDlg.h"

#include <web-server/WebServerSettings.h>

#include <airdcpp/LogManager.h>


PropPage::TextItem WebServerPage::texts[] = {
	{ IDC_WEBSERVER_ADD_USER,					ResourceManager::ADD },
	{ IDC_WEBSERVER_REMOVE_USER,				ResourceManager::REMOVE },
	{ IDC_WEBSERVER_CHANGE,						ResourceManager::SETTINGS_CHANGE },

	{ IDC_ADMIN_ACCOUNTS,						ResourceManager::ADMIN_ACCOUNTS },
	{ IDC_WEBSERVER_LABEL,						ResourceManager::SERVER_SETTINGS },
	{ IDC_WEB_SERVER_USERS_NOTE,				ResourceManager::WEB_ACCOUNTS_NOTE },

	{ IDC_WEB_SERVER_HELP,						ResourceManager::WEB_SERVER_HELP },
	{ IDC_LINK,									ResourceManager::MORE_INFORMATION },
	{ IDC_SERVER_STATE,							ResourceManager::SERVER_STATE },
	{ 0, ResourceManager::LAST }
};

WebServerPage::WebServerPage(SettingsManager *s) : PropPage(s), webMgr(webserver::WebServerManager::getInstance()) {
	SetTitle(CTSTRING(WEB_SERVER));
};

LRESULT WebServerPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);

	::SetWindowText(GetDlgItem(IDC_WEBSERVER_PORT_LABEL), CTSTRING_F(WEB_SERVER_PORT, "HTTP"));
	::SetWindowText(GetDlgItem(IDC_WEBSERVER_TLSPORT_LABEL), CTSTRING_F(WEB_SERVER_PORT, "HTTPS"));

	::SetWindowText(GetDlgItem(IDC_WEBSERVER_PORT), Util::toStringW(WEBCFG(PLAIN_PORT).num()).c_str());
	::SetWindowText(GetDlgItem(IDC_WEBSERVER_TLSPORT), Util::toStringW(WEBCFG(TLS_PORT).num()).c_str());

	ctrlTlsPort.Attach(GetDlgItem(IDC_WEBSERVER_TLSPORT));
	ctrlPort.Attach(GetDlgItem(IDC_WEBSERVER_PORT));

	url.SubclassWindow(GetDlgItem(IDC_LINK));
	url.SetHyperLinkExtendedStyle(HLINK_UNDERLINEHOVER);

	// TODO: add better help link
	url.SetHyperLink(_T("http://www.airdcpp.net/component/k2/24-web-server"));
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

void WebServerPage::applySettings() noexcept {
	auto plainserverPort = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlPort)));
	auto tlsServerPort = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlTlsPort)));

	WEBCFG(PLAIN_PORT).setCurValue(plainserverPort);
	WEBCFG(TLS_PORT).setCurValue(tlsServerPort);

	webMgr->getUserManager().replaceWebUsers(webUserList);
}

LRESULT WebServerPage::onServerState(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (!webMgr->isRunning()) {
		ctrlStart.EnableWindow(FALSE);
		lastError.clear();

		applySettings();

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
	} else {
		lastError.clear();
		webserver::WebServerManager::getInstance()->stop();
	}

	return 0;
}

void WebServerPage::on(webserver::WebServerManagerListener::Started) noexcept {
	callAsync([=] {
		updateState(STATE_STARTED);
	});
}
void WebServerPage::on(webserver::WebServerManagerListener::Stopped) noexcept {
	callAsync([=] {
		updateState(STATE_STOPPED);
	});
}

void WebServerPage::on(webserver::WebServerManagerListener::Stopping) noexcept {
	callAsync([=] {
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
	lastError = Text::toT(aError);
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
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*)pnmh;
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
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

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

	ctrlStatus.SetWindowText(statusText.c_str());


	ctrlStart.EnableWindow(aNewState == STATE_STOPPING ? FALSE : TRUE);
	currentState = aNewState;
}

void WebServerPage::write() {
	auto plainserverPort = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlPort)));
	auto tlsServerPort = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlTlsPort)));

	auto needServerRestart = WEBCFG(PLAIN_PORT).num() != plainserverPort || WEBCFG(TLS_PORT).num() != tlsServerPort;

	const auto errorF = [](const string& aError) {
		LogManager::getInstance()->message(aError, LogMessage::SEV_ERROR);
	};

	applySettings();
	webMgr->save(errorF);

	if (needServerRestart) {
		if (webMgr->isRunning()) {
			webMgr->getInstance()->stop();
		}

		webMgr->getInstance()->start(errorF);
	}
}

void WebServerPage::addListItem(const webserver::WebUserPtr& aUser) noexcept {
	int p = ctrlWebUsers.insert(ctrlWebUsers.GetItemCount(), Text::toT(aUser->getUserName()), 0, reinterpret_cast<LPARAM>(aUser.get()));

	auto login = aUser->getLastLogin() == 0 ? STRING(NEVER) : Util::formatTime("%c", aUser->getLastLogin());
	ctrlWebUsers.SetItemText(p, 1, Text::toT(login).c_str());
}

webserver::WebUserList::iterator WebServerPage::getListUser(int aPos) noexcept {
	auto data = reinterpret_cast<webserver::WebUser*>(ctrlWebUsers.GetItemData(aPos));

	return find_if(webUserList.begin(), webUserList.end(), [&data](const webserver::WebUserPtr& aUser) { 
		return aUser.get() == data; 
	});
}

WebServerPage::~WebServerPage() {
	webMgr->removeListener(this);
	webUserList.clear();
	ctrlWebUsers.Detach();
};
