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
#include <airdcpp/core/version.h>

#include <windows/Resource.h>
#include <windows/ResourceLoader.h>
#include <windows/UpdateDlg.h>

#include <airdcpp/util/PathUtil.h>
#include <airdcpp/util/Util.h>
#include <airdcpp/core/update/UpdateManager.h>
#include <airdcpp/core/update/UpdateDownloader.h>

#include <windows/WinUtil.h>
#include <windows/ActionUtil.h>
#include <windows/MainFrm.h>

namespace wingui {
UpdateDlg::UpdateDlg(const string& aTitle, const string& aMessage, const UpdateVersion& aVersionInfo)
	: message(aMessage), title(aTitle), updateVersion(aVersionInfo) { };


UpdateDlg::~UpdateDlg() {
	//if (m_hIcon)
	//	DeleteObject((HGDIOBJ)m_hIcon);
};

LRESULT UpdateDlg::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if((HWND) lParam == GetDlgItem(IDC_UPDATE_STATUS)) {
		HDC hDC = (HDC)wParam;
		if (versionAvailable)
			::SetTextColor(hDC, RGB(251,69,69));
		else
			::SetTextColor(hDC, RGB(0,148,10));
		::SetBkMode(hDC, TRANSPARENT);
		return (LRESULT)GetStockObject(HOLLOW_BRUSH);
	}
	return FALSE;
}


LRESULT UpdateDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlCurrentVersion.Attach(GetDlgItem(IDC_UPDATE_VERSION_CURRENT));
	ctrlLatestVersion.Attach(GetDlgItem(IDC_UPDATE_VERSION_LATEST));
	ctrlDownload.Attach(GetDlgItem(IDC_UPDATE_DOWNLOAD));
	ctrlClose.Attach(GetDlgItem(IDCLOSE));
	ctrlUpdateStatus.Attach(GetDlgItem(IDC_UPDATE_STATUS));

	m_Changelog.Attach(GetDlgItem(IDC_UPDATE_HISTORY_TEXT));
	m_Changelog.Subclass();

	m_Changelog.SetAutoURLDetect(false);
	m_Changelog.SetEventMask( m_Changelog.GetEventMask() | ENM_LINK );

	::SetWindowText(GetDlgItem(IDC_UPDATE_VERSION_CURRENT_LBL), (TSTRING(CURRENT_VERSION) + _T(":")).c_str());
	::SetWindowText(GetDlgItem(IDC_UPDATE_VERSION_LATEST_LBL), (TSTRING(LATEST_VERSION) + _T(":")).c_str());

	ctrlDownload.SetWindowText(CTSTRING(DOWNLOAD));
	
#ifndef FORCE_UPDATE
	versionAvailable = BUILD_NUMBER < updateVersion.build;
#else
	versionAvailable = true;
#endif

	bool versionDownloaded = UpdateManager::getInstance()->getUpdater().getInstalledUpdate() == updateVersion.build;
	ctrlDownload.EnableWindow(!versionDownloaded && versionAvailable);
	//ctrlDownload.EnableWindow(!versionDownloaded);

	if (versionDownloaded) {
		ctrlUpdateStatus.SetWindowText(CTSTRING(UPDATE_ALREADY_DOWNLOADED));
	} else if (versionAvailable) {
		ctrlUpdateStatus.SetWindowText(CTSTRING(NEW_VERSION_AVAILABLE));
	} else {
		ctrlUpdateStatus.SetWindowText(CTSTRING(USING_LATEST));
	}

	//::ShowWindow(GetDlgItem(IDC_UPDATE_STATUS), versionDownloaded);

	ctrlClose.SetWindowText(CTSTRING(CLOSE));

	::SetWindowText(GetDlgItem(IDC_UPDATE_VERSION), CTSTRING(CLIENT_VERSION));
	::SetWindowText(GetDlgItem(IDC_UPDATE_HISTORY), CTSTRING(HISTORY));

	ctrlCurrentVersion.SetWindowText(Text::toT(VERSIONSTRING).c_str());

	m_Changelog.SetFont(WinUtil::font);
	m_Changelog.SetBackgroundColor(WinUtil::bgColor); 
	m_Changelog.SetDefaultCharFormat(WinUtil::m_ChatTextGeneral);
	m_Changelog.setAutoScrollToEnd(false);
	m_Changelog.AppendHTML(message);
	m_Changelog.SetSel(0, 0); //set scroll position to top

	//m_Changelog.AppendText(Text::toT(message), WinUtil::m_ChatTextGeneral);
	ctrlLatestVersion.SetWindowText(Text::toT(updateVersion.versionStr).c_str());

	url.SubclassWindow(GetDlgItem(IDC_LINK));
	url.SetHyperLinkExtendedStyle(HLINK_UNDERLINEHOVER);

	url.SetHyperLink(Text::toT(updateVersion.infoUrl).c_str());
	url.SetLabel(CTSTRING(MORE_INFORMATION));

	SetWindowText(CTSTRING(UPDATE_CHECK));

	m_hIcon16 = ResourceLoader::loadIcon(IDR_UPDATE, 16);
	m_hIcon32 = ResourceLoader::loadIcon(IDR_UPDATE, 32);

	CStatic tmp;
	tmp.Attach(GetDlgItem(IDC_UPDATE_ICON));
	tmp.SetIcon(m_hIcon32);

	SetIcon(m_hIcon16, FALSE);
	SetIcon(m_hIcon16, TRUE);
	SetWindowPos(HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	CenterWindow(GetParent());
	ShowWindow(SW_HIDE);   
	return FALSE;
}

LRESULT UpdateDlg::OnDownload(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(updateVersion.autoUpdate && canAutoUpdate(updateVersion.updateUrl)) {
		EndDialog(wID);
	} else {
		ActionUtil::openLink(Text::toT(updateVersion.infoUrl));
	}
	return S_OK;
}

bool UpdateDlg::canAutoUpdate(const string& url) {
	if(PathUtil::getFileExt(url) == ".zip") {
		tstring buf(256, _T('\0'));
		tstring sDrive = Text::toT(AppUtil::getAppPath().substr(0, 3));
		GetVolumeInformation(sDrive.c_str(), NULL, 0, NULL, NULL, NULL, &buf[0], 256);
		return (buf.find(_T("FAT")) == string::npos);
	}
	return false;
}
}