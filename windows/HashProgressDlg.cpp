/*
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
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

#include <airdcpp/HashManager.h>
#include <airdcpp/Util.h>
#include <airdcpp/Text.h>
#include <airdcpp/SettingsManager.h>
#include <airdcpp/Thread.h>
#include <airdcpp/TimerManager.h>


HashProgressDlg::HashProgressDlg(bool aAutoClose /*false*/) : autoClose(aAutoClose) { }

LRESULT HashProgressDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// Translate static strings
	init = false;
	SetWindowText(CTSTRING(HASH_PROGRESS));
	SetDlgItemText(IDOK, CTSTRING(HASH_PROGRESS_BACKGROUND));
	SetDlgItemText(IDC_STATISTICS, CTSTRING(HASH_PROGRESS_STATS));
	SetDlgItemText(IDC_HASH_INDEXING, CTSTRING(HASH_PROGRESS_TEXT));

	SetDlgItemText(IDC_SETTINGS_MAX_HASH_SPEED, CTSTRING(SETTINGS_MAX_HASHER_SPEED));
	SetDlgItemText(IDC_MAX_HASH_SPEED, Text::toT(Util::toString(SETTING(MAX_HASH_SPEED))).c_str());
	SetDlgItemText(IDC_PAUSE, HashManager::getInstance()->isHashingPaused() ? CTSTRING(RESUME) : CTSTRING(PAUSE));
	SetDlgItemText(IDC_STOP, CTSTRING(STOP));

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
	if (!init) {
		return 0;
	}

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
	auto stats = HashManager::getInstance()->getStats();

	hashersRunning = stats.hashersRunning;

	if (stats.filesLeft > 0 && stopped) {
		::EnableWindow(GetDlgItem(IDC_STOP), true);
		stopped = false;
	}

	if (startTime == 0 && stats.filesLeft > 0 && !stats.isPaused)
		startTime = GET_TICK();

	if (stats.bytesLeft > startBytes)
		startBytes = stats.bytesLeft;

	if (stats.filesLeft > startFiles)
		startFiles = stats.filesLeft;

	if(autoClose && stats.filesLeft == 0) {
		PostMessage(WM_CLOSE);
		return;
	}

	if (stats.filesLeft == 0 || stats.bytesLeft == 0 || stats.isPaused) { // KUL - hash progress dialog patch
		SetDlgItemText(IDC_FILES_PER_HOUR, Text::toT("-.-- " + STRING(FILES_PER_HOUR) + ", " + Util::toString((uint32_t)stats.filesLeft) + " " + STRING(FILES_LEFT)).c_str());
		SetDlgItemText(IDC_HASH_SPEED, (_T("-.-- B/s, ") + Util::formatBytesW(stats.bytesLeft) + _T(" ") + TSTRING(LEFT)).c_str());
		// KUL - hash progress dialog patch
		if (stats.isPaused) {
			SetDlgItemText(IDC_TIME_LEFT, Text::toT("( " + STRING(PAUSED) + " )").c_str());
		} else {
			SetDlgItemText(IDC_TIME_LEFT, Text::toT("-:--:-- " + STRING(LEFT)).c_str());
			progress.SetPos(0);
		}
	} else {
		uint64_t tick = GET_TICK();
		double diff = static_cast<double>(tick - startTime);
		double filestat = diff > 0 ? (((double)(startFiles - stats.filesLeft)) * 60 * 60 * 1000) / diff : 0;
			
		SetDlgItemText(IDC_FILES_PER_HOUR, Text::toT(Util::toString(filestat) + " " + STRING(FILES_PER_HOUR) + ", " + Util::toString((uint32_t)stats.filesLeft) + " " + STRING(FILES_LEFT)).c_str());
		SetDlgItemText(IDC_HASH_SPEED, (Util::formatBytesW(stats.speed) + _T("/s, ") + Util::formatBytesW(stats.bytesLeft) + _T(" ") + TSTRING(LEFT)).c_str());

		if (filestat == 0 || stats.speed == 0) {
			SetDlgItemText(IDC_TIME_LEFT, Text::toT("-:--:-- " + STRING(LEFT)).c_str());
		} else {
			int64_t timeleft = (startBytes - (startBytes - stats.bytesLeft)) / stats.speed;
			SetDlgItemText(IDC_TIME_LEFT, (Util::formatSecondsW(timeleft) + _T(" ") + TSTRING(LEFT)).c_str());
		}
	}

	if (stats.filesLeft == 0 && stats.bytesLeft == 0) {
		startFiles = 0;
		startBytes = 0;
		startTime = 0;
	}

	if (stats.filesLeft == 0) {
		SetDlgItemText(IDC_CURRENT_FILE, CTSTRING(DONE));
	} else if (hashersRunning > 1) {
		SetDlgItemText(IDC_CURRENT_FILE, CTSTRING_F(X_THREADS, hashersRunning));
	} else {
		SetDlgItemText(IDC_CURRENT_FILE, Text::toT(stats.curFile).c_str());
	}

	if (startFiles == 0 || startBytes == 0) {
		progress.SetPos(0);
	} else {
		progress.SetPos((int)(10000 * ((0.5 * (startFiles - stats.filesLeft) / startFiles) + 0.5 * (startBytes - stats.bytesLeft) / startBytes)));
	}
		
	SetDlgItemText(IDC_PAUSE, stats.isPaused ? CTSTRING(RESUME) : CTSTRING(PAUSE)); // KUL - hash progress dialog patch
}

LRESULT HashProgressDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD/* wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	//EndDialog(wID);
	init = false;
	if( IsWindow() )
		DestroyWindow();
	return 0;
}