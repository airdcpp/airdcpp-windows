/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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
#include "HashProgressDlg.h"

#include "../client/HashManager.h"
#include "../client/Util.h"
#include "../client/Text.h"
#include "../client/SettingsManager.h"
#include "../client/Thread.h"
#include "../client/TimerManager.h"


HashProgressDlg::HashProgressDlg(bool aAutoClose /*false*/) : autoClose(aAutoClose), startBytes(0), startFiles(0), init(false), hashers(0), startTime(0), stopped(false) { }

LRESULT HashProgressDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// Translate static strings
	init = false;
	SetWindowText(CTSTRING(HASH_PROGRESS));
	SetDlgItemText(IDOK, CTSTRING(HASH_PROGRESS_BACKGROUND));
	SetDlgItemText(IDC_STATISTICS, CTSTRING(HASH_PROGRESS_STATS));
	SetDlgItemText(IDC_HASH_INDEXING, CTSTRING(HASH_PROGRESS_TEXT));
	// KUL - hash progress dialog patch (begin)
	SetDlgItemText(IDC_SETTINGS_MAX_HASH_SPEED, CTSTRING(SETTINGS_MAX_HASHER_SPEED));
	SetDlgItemText(IDC_MAX_HASH_SPEED, Text::toT(Util::toString(SETTING(MAX_HASH_SPEED))).c_str());
	SetDlgItemText(IDC_PAUSE, HashManager::getInstance()->isHashingPaused() ? CTSTRING(RESUME) : CTSTRING(PAUSE));
	SetDlgItemText(IDC_STOP, CTSTRING(STOP));

	// KUL - hash progress dialog patch (end)

	string tmp;
	int64_t speed = 0;

	startBytes = 0;
	startFiles = 0;
	hashers = 0;
	HashManager::getInstance()->getStats(tmp, startBytes, startFiles, speed, hashers);

	// KUL - hash progress dialog patch
	CUpDownCtrl hashspin; 
	hashspin.Attach(GetDlgItem(IDC_HASH_SPIN));
	hashspin.SetRange(0, 999);
	hashspin.Detach();

	progress.Attach(GetDlgItem(IDC_HASH_PROGRESS));
	progress.SetRange(0, 10000);
	updateStats();

	HashManager::getInstance()->setPriority(Thread::NORMAL);
		
	SetTimer(1, 1000);
	init = true;
	return TRUE;
}
void HashProgressDlg::setAutoClose(bool val) {
	autoClose = val;
}

LRESULT HashProgressDlg::onMaxHashSpeed(WORD /*wNotifyCode*/, WORD, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!init)
		return 0;

		TCHAR buf[256];
		GetDlgItemText(IDC_MAX_HASH_SPEED, buf, 256);
		SettingsManager::getInstance()->set(SettingsManager::MAX_HASH_SPEED, Util::toInt(Text::fromT(buf)));
		
	return 0;
}

LRESULT HashProgressDlg::onPause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(HashManager::getInstance()->isHashingPaused()) {
		HashManager::getInstance()->resumeHashing(true);
		SetDlgItemText(IDC_PAUSE, CTSTRING(PAUSE));
	} else {
		HashManager::getInstance()->pauseHashing();
		SetDlgItemText(IDC_PAUSE, CTSTRING(RESUME));
	}
	return 0;
}
		
LRESULT HashProgressDlg::onClear(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	HashManager::getInstance()->stop();
	::EnableWindow(GetDlgItem(IDC_STOP), false);	
	stopped = true;
	return 0;
}
LRESULT HashProgressDlg::onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	updateStats();
	return 0;
}
LRESULT HashProgressDlg::onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	HashManager::getInstance()->setPriority(Thread::IDLE);
	init = false;
	progress.Detach();
	KillTimer(1);
	return 0;
}

void HashProgressDlg::updateStats() {
	string file;
	int64_t bytes = 0;
	size_t files = 0;
	int64_t speed = 0;

	HashManager::getInstance()->getStats(file, bytes, files, speed, hashers);
	bool paused = HashManager::getInstance()->isHashingPaused();

	if(files > 0 && stopped) {
		::EnableWindow(GetDlgItem(IDC_STOP), true);
		stopped = false;
	}

	if(startTime == 0 && files > 0 && !paused)
		startTime = GET_TICK();

	if(bytes > startBytes)
		startBytes = bytes;

	if(files > startFiles)
		startFiles = files;

	if(autoClose && files == 0) {
		PostMessage(WM_CLOSE);
		return;
	}

	if(files == 0 || bytes == 0 || paused) { // KUL - hash progress dialog patch
		SetDlgItemText(IDC_FILES_PER_HOUR, Text::toT("-.-- " + STRING(FILES_PER_HOUR) + ", " + Util::toString((uint32_t)files) + " " + STRING(FILES_LEFT)).c_str());
		SetDlgItemText(IDC_HASH_SPEED, (_T("-.-- B/s, ") + Util::formatBytesW(bytes) + _T(" ") + TSTRING(LEFT)).c_str());
		// KUL - hash progress dialog patch
		if(paused) {
			SetDlgItemText(IDC_TIME_LEFT, Text::toT("( " + STRING(PAUSED) + " )").c_str());
		} else {
			SetDlgItemText(IDC_TIME_LEFT, Text::toT("-:--:-- " + STRING(LEFT)).c_str());
			progress.SetPos(0);
		}
	} else {
		uint64_t tick = GET_TICK();
		double diff = static_cast<double>(tick - startTime);
		double filestat = diff > 0 ? (((double)(startFiles - files)) * 60 * 60 * 1000) / diff : 0;
			
		SetDlgItemText(IDC_FILES_PER_HOUR, Text::toT(Util::toString(filestat) + " " + STRING(FILES_PER_HOUR) + ", " + Util::toString((uint32_t)files) + " " + STRING(FILES_LEFT)).c_str());
		SetDlgItemText(IDC_HASH_SPEED, (Util::formatBytesW(speed) + _T("/s, ") + Util::formatBytesW(bytes) + _T(" ") + TSTRING(LEFT)).c_str());

		if(filestat == 0 || speed == 0) {
			SetDlgItemText(IDC_TIME_LEFT, Text::toT("-:--:-- " + STRING(LEFT)).c_str());
		} else {
			int64_t timeleft = (startBytes - (startBytes - bytes)) / speed;
			SetDlgItemText(IDC_TIME_LEFT, (Util::formatSecondsW(timeleft) + _T(" ") + TSTRING(LEFT)).c_str());
		}
	}

	if(files == 0 && bytes == 0) {
		startFiles = 0;
		startBytes = 0;
		startTime = 0;
	}

	if(files == 0) {
		SetDlgItemText(IDC_CURRENT_FILE, CTSTRING(DONE));
	} else if (hashers > 1) {
		SetDlgItemText(IDC_CURRENT_FILE, Text::toT(Util::toString(hashers) + " threads").c_str());
	} else {
		SetDlgItemText(IDC_CURRENT_FILE, Text::toT(file).c_str());
	}

	if(startFiles == 0 || startBytes == 0) {
		progress.SetPos(0);
	} else {
		progress.SetPos((int)(10000 * ((0.5 * (startFiles - files)/startFiles) + 0.5 * (startBytes - bytes) / startBytes)));
	}
		
	SetDlgItemText(IDC_PAUSE, paused ? CTSTRING(RESUME) : CTSTRING(PAUSE)); // KUL - hash progress dialog patch
}

LRESULT HashProgressDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD/* wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	//EndDialog(wID);
	init = false;
	if( IsWindow() )
		DestroyWindow();
	return 0;
}