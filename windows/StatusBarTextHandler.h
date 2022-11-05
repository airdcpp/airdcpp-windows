#pragma once
#pragma once
/*
* Copyright (C) 2011-2017 AirDC++ Project
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

#ifndef STATUSBAR_HANDLER_H
#define STATUSBAR_HANDLER_H

#include "stdafx.h"

#include "ResourceLoader.h"


#define POPUP_UID 19000

class StatusBarTextHandler {
public:
	StatusBarTextHandler(CStatusBarCtrl& aCtrlStatus, int aStatusTextSectionIndex, int aMaxHistoryLines) :
		ctrlStatus(aCtrlStatus), maxHistoryLines(aMaxHistoryLines), statusTextSectionIndex(aStatusTextSectionIndex) {}

	~StatusBarTextHandler() {

	}

	void init(const CWindow& aWindows) {
		ctrlTooltips.Create(aWindows.m_hWnd, aWindows.rcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_BALLOON, WS_EX_TOPMOST);
		CToolInfo ti_1(TTF_SUBCLASS, ctrlStatus.m_hWnd, statusTextSectionIndex + POPUP_UID, NULL, LPSTR_TEXTCALLBACK);
		ctrlTooltips.AddTool(&ti_1);
		ctrlTooltips.SetDelayTime(TTDT_AUTOPOP, 15000);
		ctrlTooltips.Activate(TRUE);
	}

	void onUpdateLayout(int aMaxWidth) {
		CRect r;
		ctrlStatus.GetRect(statusTextSectionIndex, r);
		ctrlTooltips.SetMaxTipWidth(aMaxWidth);
		ctrlTooltips.SetToolRect(ctrlStatus.m_hWnd, statusTextSectionIndex + POPUP_UID, r);
	}

	LRESULT onGetToolTip(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMTTDISPINFO* nm = (NMTTDISPINFO*)pnmh;
		LPNMTTDISPINFO pDispInfo = (LPNMTTDISPINFO)pnmh;
		pDispInfo->szText[0] = 0;

		if (idCtrl == statusTextSectionIndex + POPUP_UID) {
			lastLines.clear();
			for (auto& i : lastLinesList) {
				lastLines += i;
				lastLines += _T("\r\n");
			}

			if (lastLines.size() > 2) {
				lastLines.erase(lastLines.size() - 2);
			}

			nm->lpszText = const_cast<TCHAR*>(lastLines.c_str());
		}
		return 0;
	}

	void addStatusText(const tstring& aText, uint8_t aSeverity) {
		tstring line = WinUtil::formatMessageWithTimestamp(aText);
		TCHAR* sLine = (TCHAR*)line.c_str();

		if (line.size() > 512) {
			sLine[512] = NULL;
		}

		ctrlStatus.SetText(statusTextSectionIndex, sLine, SBT_NOTABPARSING);
		ctrlStatus.SetIcon(statusTextSectionIndex, ResourceLoader::getSeverityIcon(aSeverity));
		while (lastLinesList.size() + 1 > maxHistoryLines) {
			lastLinesList.pop_front();
		}

		lastLinesList.push_back(sLine);

	}

	CToolTipCtrl ctrlTooltips;
private:
	CStatusBarCtrl& ctrlStatus;
	const int maxHistoryLines;
	const int statusTextSectionIndex;

	deque<tstring> lastLinesList;
	tstring lastLines;
};
#endif