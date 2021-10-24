/* 
 * 
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
#include "LineDlg.h"

#include <airdcpp/SettingsManager.h>

#include "Resource.h"
#include "WinUtil.h"
#include "atlstr.h"


tstring KickDlg::m_sLastMsg = _T("");

LRESULT KickDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	Msgs[0] = Text::toT(SETTING(KICK_MSG_RECENT_01));
	Msgs[1] = Text::toT(SETTING(KICK_MSG_RECENT_02));
	Msgs[2] = Text::toT(SETTING(KICK_MSG_RECENT_03));
	Msgs[3] = Text::toT(SETTING(KICK_MSG_RECENT_04));
	Msgs[4] = Text::toT(SETTING(KICK_MSG_RECENT_05));
	Msgs[5] = Text::toT(SETTING(KICK_MSG_RECENT_06));
	Msgs[6] = Text::toT(SETTING(KICK_MSG_RECENT_07));
	Msgs[7] = Text::toT(SETTING(KICK_MSG_RECENT_08));
	Msgs[8] = Text::toT(SETTING(KICK_MSG_RECENT_09));
	Msgs[9] = Text::toT(SETTING(KICK_MSG_RECENT_10));
	Msgs[10] = Text::toT(SETTING(KICK_MSG_RECENT_11));
	Msgs[11] = Text::toT(SETTING(KICK_MSG_RECENT_12));
	Msgs[12] = Text::toT(SETTING(KICK_MSG_RECENT_13));
	Msgs[13] = Text::toT(SETTING(KICK_MSG_RECENT_14));
	Msgs[14] = Text::toT(SETTING(KICK_MSG_RECENT_15));
	Msgs[15] = Text::toT(SETTING(KICK_MSG_RECENT_16));
	Msgs[16] = Text::toT(SETTING(KICK_MSG_RECENT_17));
	Msgs[17] = Text::toT(SETTING(KICK_MSG_RECENT_18));
	Msgs[18] = Text::toT(SETTING(KICK_MSG_RECENT_19));
	Msgs[19] = Text::toT(SETTING(KICK_MSG_RECENT_20));

	ctrlLine.Attach(GetDlgItem(IDC_LINE));
	ctrlLine.SetFocus();

	line = _T("");
	for(int i = 0; i < 20; i++) {
		if(Msgs[i] != _T(""))
			ctrlLine.AddString(Msgs[i].c_str());
	}
	ctrlLine.SetWindowText(m_sLastMsg.c_str());

	ctrlDescription.Attach(GetDlgItem(IDC_DESCRIPTION));
	ctrlDescription.SetWindowText(description.c_str());
	
	SetWindowText(title.c_str());
	
	CenterWindow(GetParent());
	return FALSE;
}
	
LRESULT KickDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(wID == IDOK) {
		int iLen = ctrlLine.GetWindowTextLength();
		CAtlString sText( ' ', iLen+1 );
		GetDlgItemText(IDC_LINE, sText.GetBuffer(), iLen+1);
		sText.ReleaseBuffer();
		m_sLastMsg = sText;
		line = sText;
		int i, j;

		for ( i = 0; i < 20; i++ ) {
			if ( line == Msgs[i] ) {
				for ( j = i; j > 0; j-- ) {
					Msgs[j] = Msgs[j-1];
				}
				Msgs[0] = line;
				break;
			}
		}

		if ( i >= 20 ) {
			for ( j = 19; j > 0; j-- ) {
				Msgs[j] = Msgs[j-1];
			}
			Msgs[0] = line;
		}

		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_01, Text::fromT(Msgs[0]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_02, Text::fromT(Msgs[1]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_03, Text::fromT(Msgs[2]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_04, Text::fromT(Msgs[3]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_05, Text::fromT(Msgs[4]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_06, Text::fromT(Msgs[5]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_07, Text::fromT(Msgs[6]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_08, Text::fromT(Msgs[7]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_09, Text::fromT(Msgs[8]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_10, Text::fromT(Msgs[9]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_11, Text::fromT(Msgs[10]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_12, Text::fromT(Msgs[11]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_13, Text::fromT(Msgs[12]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_14, Text::fromT(Msgs[13]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_15, Text::fromT(Msgs[14]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_16, Text::fromT(Msgs[15]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_17, Text::fromT(Msgs[16]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_18, Text::fromT(Msgs[17]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_19, Text::fromT(Msgs[18]));
		SettingsManager::getInstance()->set(SettingsManager::KICK_MSG_RECENT_20, Text::fromT(Msgs[19]));
	}
  
	EndDialog(wID);
	return 0;
}


LRESULT LineDlg::onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlLine.SetFocus();
	return FALSE;
}

LRESULT LineDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlLine.Attach(GetDlgItem(IDC_LINE));
	ctrlLine.SetFocus();
	ctrlLine.SetWindowText(line.c_str());
	ctrlLine.SetSelAll(TRUE);
	if (password) {
		ctrlLine.SetWindowLongPtr(GWL_STYLE, ctrlLine.GetWindowLongPtr(GWL_STYLE) | ES_PASSWORD);
		ctrlLine.SetPasswordChar('*');
	}

	ctrlDescription.Attach(GetDlgItem(IDC_DESCRIPTION));
	ctrlDescription.SetWindowText(description.c_str());

	SetWindowText(title.c_str());


	CenterWindow(GetParent());
	return FALSE;
}

LRESULT LineDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (wID == IDOK) {
		if (!allowEmpty && ctrlLine.GetWindowTextLength() == 0) {
			WinUtil::showMessageBox(TSTRING(LINE_EMPTY), MB_ICONINFORMATION);
			return 0;
		}

		line.resize(ctrlLine.GetWindowTextLength() + 1);
		line.resize(GetDlgItemText(IDC_LINE, &line[0], line.size()));
	}

	EndDialog(wID);
	return 0;
}



ComboDlg::ComboDlg(const StringList& aStrings) {
	for (auto j = aStrings.begin(); j != aStrings.end(); j++) {
		ctrlCombo.AddString(Text::toT(*j).c_str());
	}
}

LRESULT ComboDlg::onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlCombo.SetFocus();
	return FALSE;
}

LRESULT ComboDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlCombo.Attach(GetDlgItem(IDC_COMBO));
	ctrlCombo.SetFocus();
	int n = 0;
	for (auto j = strings.begin(); j != strings.end(); j++) {
		ctrlCombo.InsertString(n, Text::toT(*j).c_str());
		n++;
	}

	ctrlCombo.SetCurSel(curSel);

	ctrlDescription.Attach(GetDlgItem(IDC_DESCRIPTION));
	ctrlDescription.SetWindowText(description.c_str());

	SetWindowText(title.c_str());

	CenterWindow(GetParent());
	return FALSE;
}


ChngPassDlg::ChngPassDlg() : hideold(false), okexit(true), ok(_T("OK")), cancel(TSTRING(CANCEL)) { };

LRESULT ComboDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (wID == IDOK) {
		curSel = ctrlCombo.GetCurSel();
	}

	EndDialog(wID);
	return 0;
}

LRESULT ChngPassDlg::onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if (hideold) {
		ctrlNewLine.SetFocus();
	} else {
		ctrlOldLine.SetFocus();
	}

	return FALSE;
}

LRESULT ChngPassDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlOldLine.Attach(GetDlgItem(IDC_LINE));
	ctrlOldLine.SetWindowText(_T(""));
	if (hideold) {
		ctrlOldLine.EnableWindow(FALSE);
	} else {
		ctrlOldLine.EnableWindow(TRUE);
		ctrlOldLine.SetWindowLong(GWL_STYLE, ctrlOldLine.GetWindowLong(GWL_STYLE) | ES_PASSWORD);
		ctrlOldLine.SetPasswordChar('*');
		ctrlOldLine.SetFocus();
		ctrlOldLine.SetSelAll(TRUE);
	}

	ctrlNewLine.Attach(GetDlgItem(IDC_LINE2));
	ctrlNewLine.SetWindowLong(GWL_STYLE, ctrlNewLine.GetWindowLong(GWL_STYLE) | ES_PASSWORD);
	ctrlNewLine.SetPasswordChar('*');
	ctrlNewLine.SetWindowText(_T(""));
	if (hideold) ctrlNewLine.SetFocus();

	ctrlConfirmLine.Attach(GetDlgItem(IDC_LINE3));
	ctrlConfirmLine.SetWindowLong(GWL_STYLE, ctrlConfirmLine.GetWindowLong(GWL_STYLE) | ES_PASSWORD);
	ctrlConfirmLine.SetPasswordChar('*');
	ctrlConfirmLine.SetWindowText(_T(""));

	ctrlOldDescription.Attach(GetDlgItem(IDC_PSWD_CHNG_OLD));
	ctrlOldDescription.SetWindowText(CTSTRING(OLD));

	ctrlNewDescription.Attach(GetDlgItem(IDC_PSWD_CHNG_NEW));
	ctrlNewDescription.SetWindowText(CTSTRING(NEW));

	ctrlConfirmDescription.Attach(GetDlgItem(IDC_PSWD_CHNG_CONFIRM_NEW));
	ctrlConfirmDescription.SetWindowText(CTSTRING(CONFIRM_NEW));

	ctrlOK.Attach(GetDlgItem(IDOK));
	ctrlOK.SetWindowText(ok.c_str());

	ctrlCancel.Attach(GetDlgItem(IDCANCEL));
	ctrlCancel.SetWindowText(cancel.c_str());

	SetWindowText(title.c_str());

	CenterWindow(GetParent());
	return FALSE;
}

LRESULT ChngPassDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (wID == IDOK && okexit) {
		int len = ctrlOldLine.GetWindowTextLength() + 1;
		TCHAR buf1[128];
		GetDlgItemText(IDC_LINE, buf1, len);
		Oldline = buf1;

		len = ctrlNewLine.GetWindowTextLength() + 1;
		TCHAR buf2[128];
		GetDlgItemText(IDC_LINE2, buf2, len);
		Newline = buf2;

		len = ctrlConfirmLine.GetWindowTextLength() + 1;
		TCHAR buf3[128];
		GetDlgItemText(IDC_LINE3, buf3, len);
		Confirmline = buf3;

		if (Newline.empty()) {
			WinUtil::showMessageBox(TSTRING(LINE_EMPTY), MB_ICONINFORMATION);
			return 0;
		}

		if (Confirmline != Newline) {
			WinUtil::showMessageBox(TSTRING(PASS_NO_MATCH), MB_ICONINFORMATION);
			return 0;
		}
	}

	EndDialog(wID);
	return 0;
}

LRESULT PassDlg::onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlLine.SetFocus();
	return FALSE;
}

LRESULT PassDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	ctrlLine.Attach(GetDlgItem(IDC_LINE));
	ctrlLine.SetFocus();
	ctrlLine.SetWindowText(line.c_str());
	ctrlLine.SetSelAll(TRUE);
	if (password) {
		ctrlLine.SetWindowLong(GWL_STYLE, ctrlLine.GetWindowLong(GWL_STYLE) | ES_PASSWORD);
		ctrlLine.SetPasswordChar('*');
	}

	ctrlDescription.Attach(GetDlgItem(IDC_DESCRIPTION));
	ctrlDescription.SetWindowText(description.c_str());

	ctrlOK.Attach(GetDlgItem(IDOK));
	ctrlOK.SetWindowText(ok.c_str());

	SetWindowText(title.c_str());

	::EnableWindow(GetDlgItem(IDCANCEL), false);

	CenterWindow(GetParent());
	SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	SetForegroundWindow(m_hWnd);
	return FALSE;
}

LRESULT PassDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (wID == IDOK) {
		int len = ctrlLine.GetWindowTextLength() + 1;
		TCHAR buf[128];
		GetDlgItemText(IDC_LINE, buf, len);
		line = buf;
	}

	EndDialog(wID);
	return 0;
}
