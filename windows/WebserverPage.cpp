/*
* Copyright (C) 2011-2015 AirDC++ Project
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

#include "WebServerPage.h"
#include "WebUserDlg.h"
#include <airdcpp/LogManager.h>

PropPage::TextItem WebServerPage::texts[] = {
	{ IDC_WEBSERVER_ADD_USER,					ResourceManager::ADD },
	{ IDC_WEBSERVER_REMOVE_USER,				ResourceManager::REMOVE },
	{ IDC_WEBSERVER_CHANGE,						ResourceManager::SETTINGS_CHANGE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT WebServerPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);

	::SetWindowText(GetDlgItem(IDC_WEBSERVER_PORT), Util::toStringW(webMgr->getPlainServerConfig().getPort()).c_str());
	::SetWindowText(GetDlgItem(IDC_WEBSERVER_TLSPORT), Util::toStringW(webMgr->getTlsServerConfig().getPort()).c_str());

	ctrlTlsPort.Attach(GetDlgItem(IDC_WEBSERVER_TLSPORT));
	ctrlPort.Attach(GetDlgItem(IDC_WEBSERVER_PORT));

	ctrlWebUsers.Attach(GetDlgItem(IDC_WEBSERVER_USERS));
	ctrlWebUsers.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
	CRect rc;
	ctrlWebUsers.GetClientRect(rc);
	ctrlWebUsers.InsertColumn(0, CTSTRING(NAME), LVCFMT_LEFT, rc.Width() / 2, 0);
	ctrlWebUsers.InsertColumn(1, CTSTRING(PASSWORD), LVCFMT_LEFT, rc.Width() / 2, 1);

	ctrlRemove.Attach(GetDlgItem(IDC_WEBSERVER_REMOVE_USER));
	ctrlAdd.Attach(GetDlgItem(IDC_WEBSERVER_ADD_USER));
	ctrlChange.Attach(GetDlgItem(IDC_WEBSERVER_CHANGE));
	ctrlStart.Attach(GetDlgItem(IDC_WEBSERVER_START));
	ctrlStatus.Attach(GetDlgItem(IDC_WEBSERVER_STATUS));

	currentState = webMgr->isRunning() ? STATE_STARTED : STATE_STOPPED;
	updateStatus();

	webUserList = webMgr->getUserManager().getUsers();
	for (auto u : webUserList) {
		addListItem(u->getUserName(), u->getPassword());
	}
	webMgr->addListener(this);
	return TRUE;
}
LRESULT WebServerPage::onButton(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	if (wID == IDC_WEBSERVER_ADD_USER) {
		WebUserDlg dlg;
		if (dlg.DoModal() == IDOK) {
			webUserList.emplace_back(make_shared<webserver::WebUser>(dlg.getUserName(), dlg.getPassWord()));
			addListItem(dlg.getUserName(), dlg.getPassWord());
		}
	}
	else if (wID == IDC_WEBSERVER_CHANGE) {
		if (ctrlWebUsers.GetSelectedCount() == 1) {
			int sel = ctrlWebUsers.GetSelectedIndex();
			auto webUser = webUserList[sel];
			WebUserDlg dlg(Text::toT(webUser->getUserName()), Text::toT(webUser->getPassword()));
			if (dlg.DoModal() == IDOK) {
				webUser->setUserName(dlg.getUserName());
				webUser->setPassword(dlg.getPassWord());
				ctrlWebUsers.SetItemText(sel, 0, Text::toT(dlg.getUserName()).c_str());
				ctrlWebUsers.SetItemText(sel, 1, Text::toT(dlg.getPassWord()).c_str());
			}
		}
	}
	else if (wID == IDC_WEBSERVER_REMOVE_USER) {
		if (ctrlWebUsers.GetSelectedCount() == 1) {
			int sel = ctrlWebUsers.GetSelectedIndex();
			webUserList.erase(find(webUserList.begin(), webUserList.end(), webUserList[sel]));
			ctrlWebUsers.DeleteItem(sel);
		}
	}
	else if (wID == IDC_WEBSERVER_START) {
		if (!webMgr->isRunning()) {
			auto plainserverPort = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlPort)));
			auto tlsServerPort = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlTlsPort)));

			if (webMgr->getPlainServerConfig().getPort() != plainserverPort) {
				webMgr->getPlainServerConfig().setPort(plainserverPort);
			}
			if (webMgr->getTlsServerConfig().getPort() != tlsServerPort) {
				webMgr->getTlsServerConfig().setPort(tlsServerPort);
			}

			ctrlStart.EnableWindow(FALSE);
			lastError.clear();
			webMgr->start([&](const string& aError) { lastError += aError + "\n"; });
		} else {
			lastError.clear();
			webserver::WebServerManager::getInstance()->stop();
		}
	}

	return 0;
}
LRESULT WebServerPage::onSelChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
	if (ctrlWebUsers.GetSelectedCount() == 1) {
		ctrlChange.EnableWindow(TRUE);
		ctrlRemove.EnableWindow(TRUE);
	}
	else {
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

void WebServerPage::updateStatus() {

	if (currentState == STATE_STARTED) {
		ctrlStart.SetWindowText(CTSTRING(STOP));
		ctrlStatus.SetWindowText((TSTRING(WEBSERVER_RUNNING) + (lastError.empty() ? _T("") : _T(" : ") + Text::toT(lastError))).c_str());
	} else if(currentState == STATE_STOPPING) {
		ctrlStatus.SetWindowText(_T("Stopping..."));
	} else if (currentState == STATE_STOPPED) {
	 ctrlStart.SetWindowText(CTSTRING(START));
	 ctrlStatus.SetWindowText(CTSTRING(WEBSERVER_STOPPED));
	}
}

void WebServerPage::write() {

	bool needServerRestart = false;

	auto plainserverPort = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlPort)));
	auto tlsServerPort = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlTlsPort)));

	if(webMgr->getPlainServerConfig().getPort() != plainserverPort) {
		webMgr->getPlainServerConfig().setPort(plainserverPort);
		needServerRestart = true;
	}
	if (webMgr->getTlsServerConfig().getPort() != tlsServerPort) {
		webMgr->getPlainServerConfig().setPort(tlsServerPort);
		needServerRestart = true;
	}

	webMgr->getUserManager().replaceWebUsers(webUserList);
	webMgr->save();

	if (needServerRestart) {
		if (webMgr->isRunning())
			webMgr->getInstance()->stop();
		webMgr->getInstance()->start([](const string& aError) {
			LogManager::getInstance()->message(aError, LogMessage::SEV_ERROR);
		});
	}
}

void WebServerPage::addListItem(const string& aUserName, const string& aPassword) {
	int p = ctrlWebUsers.insert(ctrlWebUsers.GetItemCount(), Text::toT(aUserName));
	ctrlWebUsers.SetItemText(p, 1, Text::toT(aPassword).c_str());
}

WebServerPage::~WebServerPage() {
	webMgr->removeListener(this);
	webUserList.clear();
	ctrlWebUsers.Detach();
	free(title);
};
