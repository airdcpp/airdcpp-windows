/* 
 * Copyright (C) 2002-2004 "Opera", <opera at home dot se>
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
#include "../client/DCPlusPlus.h"
#include "../client/version.h"
#include "Resource.h"

#include "UpdateDlg.h"

#include "../client/Util.h"
#include "../client/simplexml.h"
#include "WinUtil.h"
#include "MainFrm.h"



UpdateDlg::~UpdateDlg() {
	if (hc) {
		hc->removeListeners();
		delete hc;
	}
	if (m_hIcon)
		DeleteObject((HGDIOBJ)m_hIcon);

	if(file != NULL) {
		try {
			file->flush();
			delete file;
			file = NULL;
		} catch(const FileException&) { }
	}
	//issue the update here after the file is flushed.
	if(updating) {
		::ShellExecute(NULL, NULL, Text::toT(Util::getPath(Util::PATH_RESOURCES) + INSTALLER).c_str(), NULL, NULL, SW_SHOWNORMAL);
		MainFrame::getMainFrame()->Terminate();
	}
};


LRESULT UpdateDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlCurrentVersion.Attach(GetDlgItem(IDC_UPDATE_VERSION_CURRENT));
	ctrlLatestVersion.Attach(GetDlgItem(IDC_UPDATE_VERSION_LATEST));
	ctrlCurrentLanguageLang.Attach(GetDlgItem(IDC_UPDATE_LANGUAGELANG_CURRENT));
	ctrlCurrentLanguage.Attach(GetDlgItem(IDC_UPDATE_LANGUAGE_CURRENT));
	ctrlLatestLanguage.Attach(GetDlgItem(IDC_UPDATE_LANGUAGE_LATEST));
	ctrlChangeLog.Attach(GetDlgItem(IDC_UPDATE_HISTORY_TEXT));
	ctrlStatus.Attach(GetDlgItem(IDC_HISTORY_STATUS));
	ctrlDownload.Attach(GetDlgItem(IDC_UPDATE_DOWNLOAD));
	ctrlLangDownlod.Attach(GetDlgItem(IDC_UPDATE_LANGUAGE_BUTTON));
	ctrlClose.Attach(GetDlgItem(IDCLOSE));

	::SetWindowText(GetDlgItem(IDC_UPDATE_VERSION_CURRENT_LBL), (TSTRING(CURRENT_VERSION) + _T(":")).c_str());
	::SetWindowText(GetDlgItem(IDC_UPDATE_VERSION_LATEST_LBL), (TSTRING(LATEST_VERSION) + _T(":")).c_str());
	PostMessage(WM_SPEAKER, UPDATE_CURRENT_VERSION, (LPARAM)new tstring(_T(VERSIONSTRING)));
	PostMessage(WM_SPEAKER, UPDATE_LATEST_VERSION, (LPARAM)new tstring(_T("")));

	::SetWindowText(GetDlgItem(IDC_UPDATE_LANGUAGELANG_CURRENT_LBL), (TSTRING(CURRENT_LANGUAGE) + _T(":")).c_str());
	::SetWindowText(GetDlgItem(IDC_UPDATE_LANGUAGE_CURRENT_LBL), (TSTRING(CURRENT_VERSION) + _T(":")).c_str());
	::SetWindowText(GetDlgItem(IDC_UPDATE_LANGUAGE_LATEST_LBL), (TSTRING(LATEST_VERSION) + _T(":")).c_str());
	if(!SETTING(LANGUAGE_FILE).empty()) {
		::SetWindowText(GetDlgItem(IDC_UPDATE_LANGUAGELANG_CURRENT), TSTRING(AAIRDCPP_LANGUAGE_FILE).c_str());
		::SetWindowText(GetDlgItem(IDC_UPDATE_LANGUAGE_CURRENT), TSTRING(AAIRDCPP_LANGUAGE_VERSION).c_str());
	} else {
		::SetWindowText(GetDlgItem(IDC_UPDATE_LANGUAGELANG_CURRENT), _T("Default English"));
		::SetWindowText(GetDlgItem(IDC_UPDATE_LANGUAGE_CURRENT), _T(VERSIONSTRING));
	}
	PostMessage(WM_SPEAKER, UPDATE_LATEST_LANGUAGE, (LPARAM)new tstring(_T("")));

	PostMessage(WM_SPEAKER, UPDATE_CONTENT, (LPARAM)new tstring(_T("")));
	ctrlDownload.SetWindowText(CTSTRING(DOWNLOAD));
	ctrlDownload.EnableWindow(FALSE);
	ctrlLangDownlod.SetWindowText(CTSTRING(LANGDOWNLOAD));
	ctrlLangDownlod.EnableWindow(FALSE);
	ctrlClose.SetWindowText(CTSTRING(CLOSE));
	ctrlStatus.SetWindowText((TSTRING(CONNECTING_TO_SERVER) + _T("...")).c_str());

	::SetWindowText(GetDlgItem(IDC_UPDATE_VERSION), CTSTRING(VERSION));
	::SetWindowText(GetDlgItem(IDC_UPDATE_HISTORY), CTSTRING(HISTORY));

	hc = new HttpConnection;
	hc->addListener(this);
	hc->setCoralizeState(HttpConnection::CST_NOCORALIZE);
	hc->downloadFile((VERSION_URL));

	SetWindowText(CTSTRING(UPDATE_CHECK));

		progress.Attach(GetDlgItem(IDC_UPDATE_PROGRESS));
		progress.SetRange(0, 100);

	m_hIcon = ::LoadIcon(_Module.get_m_hInst(), MAKEINTRESOURCE(IDR_UPDATE));
	SetIcon(m_hIcon, FALSE);
	SetIcon(m_hIcon, TRUE);
	LangDL = false;
	update = false;
	updating = false;
	SetWindowPos(HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	CenterWindow(GetParent());
	ShowWindow(SW_HIDE);   
	return FALSE;
}

LRESULT UpdateDlg::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if (wParam == UPDATE_CURRENT_VERSION) {
		tstring* text = (tstring*)lParam;
		ctrlCurrentVersion.SetWindowText(text->c_str());
		delete text;
	} else if (wParam == UPDATE_LATEST_VERSION) {
		tstring* text = (tstring*)lParam;
		ctrlLatestVersion.SetWindowText(text->c_str());
		delete text;
	} else if (wParam == UPDATE_LATEST_LANGUAGE) {
		tstring* text = (tstring*)lParam;
		ctrlLatestLanguage.SetWindowText(text->c_str());
		delete text;
	} else if (wParam == UPDATE_STATUS) {
		tstring* text = (tstring*)lParam;
		ctrlStatus.SetWindowText(text->c_str());
		delete text;
	} else if (wParam == UPDATE_CONTENT) {
		tstring* text = (tstring*)lParam;
		ctrlChangeLog.SetWindowText(text->c_str());
		delete text;
	}
	return S_OK;
}

LRESULT UpdateDlg::OnDownload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	//WinUtil::openLink(Text::toT(downloadURL).c_str());
	
	progress.SetPos(0);

	if (Util::fileExists(Util::getPath(Util::PATH_RESOURCES) + INSTALLER)) {
		//string filename = (Util::getPath(Util::PATH_RESOURCES) + "AirDC_2.07_client.rar");
		File::deleteFile((Util::getPath(Util::PATH_RESOURCES) + INSTALLER));
		if(file) { delete file; file = NULL; }
	}
	update = true;
	hc = new HttpConnection();
	hc->addListener(this);
	hc->setCoralizeState(HttpConnection::CST_NOCORALIZE);
	hc->downloadFile((downloadURL));
	PostMessage(WM_SPEAKER, UPDATE_CONTENT, (LPARAM)new tstring(_T("Update Downloading...")));
	ctrlDownload.EnableWindow(FALSE);
	return S_OK;
}

LRESULT UpdateDlg::OnLangDownload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	progress.SetPos(0);
	xmldata = Util::emptyString;
	LangDL = true;
	hc->downloadFile(LangdownloadURL + Text::fromT(TSTRING(AAIRDCPP_LANGUAGE_FILE)).c_str());
	return S_OK;
}


void UpdateDlg::on(HttpConnectionListener::Failed, HttpConnection* /*conn*/, const string& aLine) throw() {
	PostMessage(WM_SPEAKER, UPDATE_STATUS, (LPARAM)new tstring(TSTRING(CONNECTION_ERROR) + _T(": ") + Text::toT(aLine) + _T("!")));
	PostMessage(WM_SPEAKER, UPDATE_CONTENT, (LPARAM)new tstring(_T("Update Download Failed...")));
	if(update)
		ctrlDownload.EnableWindow(TRUE);
}

void UpdateDlg::on(HttpConnectionListener::Complete, HttpConnection* /*conn*/, string const& /*aLine*/, bool /*fromCoral*/) throw() {
	//hc->removeListener(this);
	PostMessage(WM_SPEAKER, UPDATE_STATUS, (LPARAM)new tstring(TSTRING(DATA_RETRIEVED) + _T("!")));
			string sText;
			try {
				
				if (update) {
					if (Util::fileExists(Util::getPath(Util::PATH_RESOURCES) + INSTALLER)) {
						if(MessageBox(CTSTRING(UPDATE_CLIENT), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
						update = false;
						updating = true;
						PostMessage(WM_CLOSE);
					}else{ 
						MainFrame::getMainFrame()->Update();
					}
					}
				}

			if(LangDL)
				{
			if (!xmldata.empty())	{	
			string fname = Util::getPath(Util::PATH_USER_LANGUAGE) + Text::fromT(TSTRING(AAIRDCPP_LANGUAGE_FILE)).c_str();
			File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
			f.write(xmldata);
			f.close();
			File::deleteFile(fname);
			File::renameFile(fname + ".tmp", fname);
			PostMessage(WM_SPEAKER, UPDATE_CONTENT, (LPARAM)new tstring(_T("Language File Downloaded")));
			MessageBox(_T("Ok!\r\nRestart the AirDC++ client!"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_ICONWARNING | MB_OK);
	} 
				} else {

				double latestVersion;
				double LangVersion;

				SimpleXML xml;
				xml.fromXML(xmldata);
				xml.stepIn();

				if (xml.findChild("Version")) {
					string ver = xml.getChildData();
					
					PostMessage(WM_SPEAKER, UPDATE_LATEST_VERSION, (LPARAM)new tstring(Text::toT(ver)));

					latestVersion = Util::toDouble(ver);
					xml.resetCurrentChild();
				} else
					throw Exception();
#ifdef _WIN64 
				if (xml.findChild("URL64")) {   //remember to add this in the versionfile
					downloadURL = xml.getChildData();
					xml.resetCurrentChild();
					if (latestVersion > VERSIONFLOAT)
						ctrlDownload.EnableWindow(TRUE);
				} else
					throw Exception();
			
#else
				if (xml.findChild("URL")) {
					downloadURL = xml.getChildData();
					xml.resetCurrentChild();
					if (latestVersion > VERSIONFLOAT)
						ctrlDownload.EnableWindow(TRUE);
				} else
					throw Exception();
#endif	

				if((!SETTING(LANGUAGE_FILE).empty()) &&  (latestVersion == VERSIONFLOAT)) {
						if (xml.findChild(Text::fromT(TSTRING(AAIRDCPP_LANGUAGE_FILE)))) {
						string version = xml.getChildData();
						
						PostMessage(WM_SPEAKER, UPDATE_LATEST_LANGUAGE, (LPARAM)new tstring(Text::toT(version)));
					
						LangVersion = Util::toDouble(version);
						xml.resetCurrentChild();
						if (xml.findChild("LANGURL")) {
							LangdownloadURL = xml.getChildData();
							xml.resetCurrentChild();
							if (LangVersion > Util::toDouble(Text::fromT(TSTRING(AAIRDCPP_LANGUAGE_VERSION))))
								ctrlLangDownlod.EnableWindow(TRUE);
						}
					} else {
						// todo. move to StringDefs ERROR_LANGGUAGE
						PostMessage(WM_SPEAKER, UPDATE_LATEST_LANGUAGE, (LPARAM)new tstring(_T("language not found")));
						xml.resetCurrentChild();
					}
				}

				xml.resetCurrentChild();

				while(xml.findChild("Message")) {
					const string& sData = Text::toDOS(xml.getChildData());
					sText += sData + "\r\n";
				}
				PostMessage(WM_SPEAKER, UPDATE_CONTENT, (LPARAM)new tstring(Text::toT(sText)));
				}
			} catch (const Exception&) {
				PostMessage(WM_SPEAKER, UPDATE_CONTENT, (LPARAM)new tstring(_T("Couldn't parse xml-data")));
			}



			
}

void UpdateDlg::on(HttpConnectionListener::Data, HttpConnection* conn, const uint8_t* buf, size_t len) throw() {
		//Todo Fix Exceptions
		
	

	if (update) {
		if(file == NULL) {
		File* f = NULL;
		try {
			f = new File((Util::getPath(Util::PATH_RESOURCES) + INSTALLER), File::WRITE, File::OPEN | File::CREATE | File::SHARED);
			f->setSize(conn->getSize());
		} catch(const Exception&) {
			PostMessage(WM_SPEAKER, UPDATE_CONTENT, (LPARAM)new tstring(_T("Failed to write update file.")));
		}
			try {
			if(SETTING(BUFFER_SIZE) > 0) {
				file = new BufferedOutputStream<true>(f);
			}
		} catch(const Exception) {
			PostMessage(WM_SPEAKER, UPDATE_CONTENT, (LPARAM)new tstring(_T("Update Download failed.")));
			return;
		} catch(...) {
			if(file) { delete file; file = NULL; }
			return;			
		}

		// Check that we don't get too many bytes
		file = new LimitedOutputStream<true>(((file != NULL) ? file : f), f->getSize());
		}
		
	try {
		pos += file->write(buf, len);
	} catch(const Exception&) {
		PostMessage(WM_SPEAKER, UPDATE_CONTENT, (LPARAM)new tstring(_T("Update Download failed.")));
	}

	} else {
				if (xmldata.empty())
				PostMessage(WM_SPEAKER, UPDATE_STATUS, (LPARAM)new tstring(TSTRING(RETRIEVING_DATA) + _T("...")));
			xmldata += string((const char*)buf, len);
	}

	//progressbar
	if(conn->getSize() > 0) {
		progress.SetPos((int)(100.0 * (double)pos) / (double)conn->getSize());
		} else {
		progress.SetPos(0);
		}

}


		
