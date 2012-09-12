/*
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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
#include "WinUtil.h"
#include "AboutDlg.h"

#include "../client/format.h"
#include "../client/HttpDownload.h"
#include "../client/SimpleXML.h"
#include "../client/version.h"

LRESULT AboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	SetDlgItemText(IDC_VERSION, _T("AirDC++ v") _T(VERSIONSTRING) _T(" By Night and maksalaatikko \n http://www.airdcpp.net") _T("\n Based on: StrongDC++ \n Copyright 2004-2011 Big Muscle"));
	CEdit ctrlThanks(GetDlgItem(IDC_THANKS));
	ctrlThanks.FmtLines(TRUE);
	ctrlThanks.AppendText(thanks, TRUE);
	ctrlThanks.Detach();
		
	ctrlThanks.Attach(GetDlgItem(IDC_AIRTHANKS));
	ctrlThanks.FmtLines(TRUE);
	ctrlThanks.AppendText(Airthanks, TRUE);
	ctrlThanks.Detach();
	SetDlgItemText(IDC_TTH, WinUtil::tth.c_str());
	SetDlgItemText(IDC_LATEST, CTSTRING(DOWNLOADING));
	SetDlgItemText(IDC_TOTALS, (_T("Upload: ") + Util::formatBytesW(SETTING(TOTAL_UPLOAD)) + _T(", Download: ") + 
		Util::formatBytesW(SETTING(TOTAL_DOWNLOAD))).c_str());

	if(SETTING(TOTAL_DOWNLOAD) > 0) {
		TCHAR buf[64];
		snwprintf(buf, sizeof(buf), _T("Ratio (up/down): %.2f"), ((double)SETTING(TOTAL_UPLOAD)) / ((double)SETTING(TOTAL_DOWNLOAD)));

		SetDlgItemText(IDC_RATIO, buf);
	/*	sprintf(buf, "Uptime: %s", Util::formatTime(Util::getUptime()));
		SetDlgItemText(IDC_UPTIME, Text::toT(buf).c_str());*/
	}
	CenterWindow(GetParent());
	dl.reset(new HttpDownload(VERSION_URL,
		[this] { completeDownload(); }, true));

	return TRUE;
}

void AboutDlg::completeDownload() {
	string msg;
	if(!dl->buf.empty()) {
		try {
			SimpleXML xml;
			xml.fromXML(dl->buf);
			if(xml.findChild("DCUpdate")) {
				xml.stepIn();
				if(xml.findChild("Version")) {
					msg = xml.getChildData();
				}
			}
		} catch(const SimpleXMLException&) { }
	} else {
		msg = dl->status;
	}

	PostMessage(WM_VERSIONDATA, (WPARAM) new tstring(Text::toT(msg)));
}

LRESULT AboutDlg::onVersionData(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	tstring* x = (tstring*) wParam;
	SetDlgItemText(IDC_LATEST, x->c_str());
	delete x;
	return 0;
}
		
LRESULT AboutDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}