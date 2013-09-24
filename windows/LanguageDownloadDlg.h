/*
 * Copyright (C) 2012-2013 AirDC++ Project
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

#ifndef DCPLUSPLUS_LANGUAGE_DOWNLOAD
#define DCPLUSPLUS_LANGUAGE_DOWNLOAD

#include "Async.h"
#include "stdafx.h"
#include "resource.h"
#include "WinUtil.h"

#include <atldlgs.h>

#include "../client/UpdateManagerListener.h"
#include "../client/UpdateManager.h"
#include "../client/TimerManager.h"

class LanguageDownloadDlg : private UpdateManagerListener, private TimerManagerListener { 
public: 
	LanguageDownloadDlg(HWND hwnd, function<void()> aCompletionF, bool aIsInitial) : completionF(aCompletionF), m_hwnd(hwnd), isValid(false), isInitial(aIsInitial) {

	}

	bool show() {
		if (isInitial)
			TimerManager::getInstance()->start();

		TimerManager::getInstance()->addListener(this);
		UpdateManager::getInstance()->addListener(this);

		pProgressDlg = NULL;
		pOleWnd = NULL;
		if (CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, IID_IProgressDialog, (void**)&pProgressDlg) == S_OK) {
			if (pProgressDlg->QueryInterface(__uuidof(IOleWindow),(void**)&pOleWnd) == S_OK) {
				//pProgressDlg->SetTitle(L"Title");
				pProgressDlg->SetLine(1, _T("Checking for language updates..."), FALSE, NULL);
				pProgressDlg->SetCancelMsg(_T("Cancelling..."), NULL);

				if (pProgressDlg->StartProgressDialog(m_hwnd, NULL, PROGDLG_MARQUEEPROGRESS, NULL) == S_OK) {
					isValid = true;
					UpdateManager::getInstance()->checkLanguage();
					return true;
				}
			}
		}

		return false;
	}

	~LanguageDownloadDlg() {
		if (isValid) {
			pOleWnd->Release();
			pProgressDlg->Release();
		}
	}

	void close() {
		TimerManager::getInstance()->removeListener(this);
		UpdateManager::getInstance()->removeListener(this);

		if (isInitial)
			TimerManager::getInstance()->shutdown();

		pProgressDlg->StopProgressDialog();

		completionF();
		delete this;
	}

	void on(TimerManagerListener::Second, uint64_t /*aTick*/) {
		callAsync(m_hwnd, [this] { 
			if (pProgressDlg->HasUserCancelled() > 0) { 
				close();
			} 
		});
	}
private:
	bool isInitial;
	bool isValid;
	function<void()> completionF;
	HWND m_hwnd;
	IProgressDialog* pProgressDlg;
	IOleWindow* pOleWnd;

	void on(UpdateManagerListener::LanguageDownloading) noexcept {
		callAsync(m_hwnd, [this] { 
			pProgressDlg->SetLine(1, _T("Downloading language..."), FALSE, NULL);
		});
	}

	void on(UpdateManagerListener::LanguageFinished) noexcept {
		callAsync(m_hwnd, [this] { 
			close();
		});
	}

	void on(UpdateManagerListener::LanguageFailed, const string& aError) noexcept {
		callAsync(m_hwnd, [&] { 
			MessageBox(m_hwnd, Text::toT(aError).c_str(), _T("Language download failed"), MB_OK | MB_ICONERROR);
			close();
		});
	}
}; 

#endif
