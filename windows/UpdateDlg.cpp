/* 
 * Copyright (C) 2011-2013 AirDC++ Project
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
#include "../client/version.h"
#include "Resource.h"

#include "UpdateDlg.h"

#include "../client/Util.h"
#include "../client/UpdateManager.h"
#include "WinUtil.h"
#include "MainFrm.h"

UpdateDlg::UpdateDlg(const string& aTitle, const string& aMessage, const string& aVersionString, const string& infoUrl, bool bAutoUpdate, int aBuildID, const string& bAutoUpdateUrl)
	: title(aTitle), message(aMessage), versionString(aVersionString), infoLink(infoUrl), autoUpdate(bAutoUpdate), m_hIcon(NULL), 
	autoUpdateUrl(bAutoUpdateUrl), buildID(aBuildID), versionAvailable(false) { };


UpdateDlg::~UpdateDlg() {
	if (m_hIcon)
		DeleteObject((HGDIOBJ)m_hIcon);
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
	
	versionAvailable = (Util::toInt(SVNVERSION) < buildID);
	bool versionDownloaded = UpdateManager::getInstance()->getInstalledUpdate() == buildID;
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

	ctrlCurrentVersion.SetWindowText(Text::toT(SHORTVERSIONSTRING).c_str());

	m_Changelog.SetFont(WinUtil::font);
	m_Changelog.SetBackgroundColor(WinUtil::bgColor); 
	m_Changelog.SetDefaultCharFormat(WinUtil::m_ChatTextGeneral);
	m_Changelog.setAutoScrollToEnd(false);
	m_Changelog.AppendHTML(message);
	m_Changelog.SetSel(0, 0); //set scroll position to top

	//m_Changelog.AppendText(Text::toT(message), WinUtil::m_ChatTextGeneral);
	ctrlLatestVersion.SetWindowText(Text::toT(versionString).c_str());

	url.SubclassWindow(GetDlgItem(IDC_LINK));
	url.SetHyperLinkExtendedStyle(HLINK_UNDERLINEHOVER);

	url.SetHyperLink(Text::toT(infoLink).c_str());
	url.SetLabel(CTSTRING(MORE_INFORMATION));

	SetWindowText(CTSTRING(UPDATE_CHECK));

	m_hIcon = ::LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDR_UPDATE));
	SetIcon(m_hIcon, FALSE);
	SetIcon(m_hIcon, TRUE);
	SetWindowPos(HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	CenterWindow(GetParent());
	ShowWindow(SW_HIDE);   
	return FALSE;
}

LRESULT UpdateDlg::OnDownload(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(autoUpdate && canAutoUpdate(autoUpdateUrl)) {
		EndDialog(wID);
	} else {
		WinUtil::openLink(Text::toT(infoLink));
	}
	return S_OK;
}

bool UpdateDlg::canAutoUpdate(const string& url) {
	if(Util::getFileExt(url) == ".zip") {
		tstring buf(256, _T('\0'));
		tstring sDrive = Text::toT(Util::getAppName().substr(0, 3));
		GetVolumeInformation(sDrive.c_str(), NULL, 0, NULL, NULL, NULL, &buf[0], 256);
		return (buf.find(_T("FAT")) == string::npos);
	}
	return false;
}
