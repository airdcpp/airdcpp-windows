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

#if !defined(LINE_DLG_H)
#define LINE_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class LineDlg : public CDialogImpl<LineDlg>
{
	CEdit ctrlLine;
	CStatic ctrlDescription;
public:
	tstring line;
	tstring description;
	tstring title;
	bool password;
	bool disable;

	enum { IDD = IDD_LINE };
	
	BEGIN_MSG_MAP(LineDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()
	
	LineDlg() : password(false) { }
	
	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlLine.SetFocus();
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		ctrlLine.Attach(GetDlgItem(IDC_LINE));
		ctrlLine.SetFocus();
		ctrlLine.SetWindowText(line.c_str());
		ctrlLine.SetSelAll(TRUE);
		if(password) {
			ctrlLine.SetWindowLongPtr(GWL_STYLE, ctrlLine.GetWindowLongPtr(GWL_STYLE) | ES_PASSWORD);
			ctrlLine.SetPasswordChar('*');
		}

		ctrlDescription.Attach(GetDlgItem(IDC_DESCRIPTION));
		ctrlDescription.SetWindowText(description.c_str());
		
		SetWindowText(title.c_str());
		

		CenterWindow(GetParent());
		return FALSE;
	}
	
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(wID == IDOK) {
			line.resize(ctrlLine.GetWindowTextLength() + 1);
			line.resize(GetDlgItemText(IDC_LINE, &line[0], line.size()));
		}
		EndDialog(wID);
		return 0;
	}
	
};

class LineComboDlg : public CDialogImpl<LineComboDlg>
{
	CEdit ctrlLine;
	CStatic ctrlDescription;
	CStatic ctrlComboDescription;
	CComboBox ctrlCombo;
public:
	tstring line;
	tstring description;
	tstring comboDescription;
	tstring title;
	int curSel;

	enum { IDD = IDD_LINECOMBO };
	
	BEGIN_MSG_MAP(LineComboDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()
	
	LineComboDlg() : curSel(0) { }
	//LineComboDlg(const CComboBox& aCombo) : curSel(0), ctrlCombo(aCombo) { }
	/*LineComboDlg(const StringList& aStrings) : curSel(0) { 
		for(auto j = aStrings.begin(); j != aStrings.end(); j++) {
			ctrlCombo.AddString(Text::toT(*j).c_str());
		}
	}*/
	
	void setList(const StringList& aStrings) { 
		for(auto j = aStrings.begin(); j != aStrings.end(); j++) {
			ctrlCombo.AddString(Text::toT(*j).c_str());
		}
	}

	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlLine.SetFocus();
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		ctrlLine.Attach(GetDlgItem(IDC_LINE));
		ctrlLine.SetFocus();
		ctrlLine.SetWindowText(line.c_str());
		ctrlLine.SetSelAll(TRUE);

		ctrlCombo.Attach(GetDlgItem(IDC_COMBO));

		ctrlDescription.Attach(GetDlgItem(IDC_COMBO_DESC));
		ctrlDescription.SetWindowText(description.c_str());

		ctrlComboDescription.Attach(GetDlgItem(IDC_DESCRIPTION));
		ctrlDescription.SetWindowText(comboDescription.c_str());
		
		SetWindowText(title.c_str());
		

		CenterWindow(GetParent());
		return FALSE;
	}
	
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(wID == IDOK) {
			line.resize(ctrlLine.GetWindowTextLength() + 1);
			line.resize(GetDlgItemText(IDC_LINE, &line[0], line.size()));
			curSel = ctrlCombo.GetCurSel();
		}
		EndDialog(wID);
		return 0;
	}
};

class ComboDlg : public CDialogImpl<ComboDlg>
{
	CStatic ctrlDescription;
	CComboBox ctrlCombo;
public:
	tstring description;
	tstring title;
	int curSel;
	StringList strings;

	enum { IDD = IDD_COMBO };
	
	BEGIN_MSG_MAP(ComboDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()
	
	ComboDlg() : curSel(0) { }
	ComboDlg(const CComboBox& aCombo) : curSel(0), ctrlCombo(aCombo) { }
	ComboDlg(const StringList& aStrings) : curSel(0) { 
		for(auto j = aStrings.begin(); j != aStrings.end(); j++) {
			ctrlCombo.AddString(Text::toT(*j).c_str());
		}
	}

	/*void setList(const StringList& aStrings) { 
		for(auto j = aStrings.begin(); j != aStrings.end(); j++) {
			ctrlCombo.AddString(Text::toT(*j).c_str());
		}
	}*/

	void setList(const StringList& aStrings) { strings = aStrings; }
	
	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlCombo.SetFocus();
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		ctrlCombo.Attach(GetDlgItem(IDC_COMBO));
		ctrlCombo.SetFocus();
		int n = 0;
		for(auto j = strings.begin(); j != strings.end(); j++) {
			ctrlCombo.InsertString(n, Text::toT(*j).c_str());
			n++;
		}
		ctrlCombo.SetCurSel(0);

		ctrlDescription.Attach(GetDlgItem(IDC_DESCRIPTION));
		ctrlDescription.SetWindowText(description.c_str());
		
		SetWindowText(title.c_str());
		
		CenterWindow(GetParent());
		return FALSE;
	}
	
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(wID == IDOK) {
			curSel = ctrlCombo.GetCurSel();
		}
		EndDialog(wID);
		return 0;
	}
};

class KickDlg : public CDialogImpl<KickDlg> {
	CComboBox ctrlLine;
	CStatic ctrlDescription;
public:
	tstring line;
	static tstring m_sLastMsg;
	tstring description;
	tstring title;

	enum { IDD = IDD_KICK };
	
	BEGIN_MSG_MAP(KickDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()
	
	KickDlg() {};
	
	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlLine.SetFocus();
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	tstring Msgs[20];
};
class ChngPassDlg : public CDialogImpl<ChngPassDlg>
{
	CEdit ctrlOldLine;
	CEdit ctrlNewLine;
	CEdit ctrlConfirmLine;
	CStatic ctrlOldDescription;
	CStatic ctrlNewDescription;
	CStatic ctrlConfirmDescription;
	CButton ctrlOK;
	CButton ctrlCancel;

public:
	tstring Oldline;
	tstring Newline;
	tstring Confirmline;
	tstring Olddescription;
	tstring Newdescription;
	tstring Confirmdescription;
	tstring title;
	tstring ok;
	tstring cancel;
	bool	hideold;
	bool	okexit;

	enum { IDD = IDD_CHANGE_PASS };
	
	BEGIN_MSG_MAP(LineDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()
	
	ChngPassDlg() : hideold(false), okexit(true), ok(_T("OK")), cancel(_T("Cancel")) { };
	
	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		if(hideold) {
			ctrlNewLine.SetFocus();
		} else {
			ctrlOldLine.SetFocus();
		}
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		ctrlOldLine.Attach(GetDlgItem(IDC_LINE));
		ctrlOldLine.SetWindowText(_T(""));
		if(hideold) {
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
		if(hideold) ctrlNewLine.SetFocus();

		ctrlConfirmLine.Attach(GetDlgItem(IDC_LINE3));
		ctrlConfirmLine.SetWindowLong(GWL_STYLE, ctrlConfirmLine.GetWindowLong(GWL_STYLE) | ES_PASSWORD);
		ctrlConfirmLine.SetPasswordChar('*');
		ctrlConfirmLine.SetWindowText(_T(""));

		ctrlOldDescription.Attach(GetDlgItem(IDC_PSWD_CHNG_OLD));
		ctrlOldDescription.SetWindowText(Olddescription.c_str());

		ctrlNewDescription.Attach(GetDlgItem(IDC_PSWD_CHNG_NEW));
		ctrlNewDescription.SetWindowText(Newdescription.c_str());

		ctrlConfirmDescription.Attach(GetDlgItem(IDC_PSWD_CHNG_CONFIRM_NEW));
		ctrlConfirmDescription.SetWindowText(Confirmdescription.c_str());

		ctrlOK.Attach(GetDlgItem(IDOK));
		ctrlOK.SetWindowText(ok.c_str());

		ctrlCancel.Attach(GetDlgItem(IDCANCEL));
		ctrlCancel.SetWindowText(cancel.c_str());

		SetWindowText(title.c_str());
		
//		::EnableWindow(GetDlgItem(IDCANCEL), false);

		CenterWindow(GetParent());
		return FALSE;
	}
	
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(wID == IDOK && okexit) {
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
		}
		EndDialog(wID);
		return 0;
	}
	};
class PassDlg : public CDialogImpl<PassDlg>
{
	CEdit ctrlLine;
	CStatic ctrlDescription;
	CButton ctrlOK;
public:
	tstring line;
	tstring description;
	tstring title;
	bool password;
	tstring ok;

	enum { IDD = IDD_PASS };
	
	BEGIN_MSG_MAP(PassDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
	END_MSG_MAP()
	
	PassDlg() : password(true) { };

	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlLine.SetFocus();
		return FALSE;
	}

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		ctrlLine.Attach(GetDlgItem(IDC_LINE));
		ctrlLine.SetFocus();
		ctrlLine.SetWindowText(line.c_str());
		ctrlLine.SetSelAll(TRUE);
		if(password) {
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
		return FALSE;
	}
	
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(wID == IDOK) {
			int len = ctrlLine.GetWindowTextLength() + 1;
			TCHAR buf[128];
			GetDlgItemText(IDC_LINE, buf, len);
			line = buf;
		}

		EndDialog(wID);
		return 0;
	}
	
};

#endif // !defined(LINE_DLG_H)

/**
 * @file
 * $Id: LineDlg.h 392 2008-06-21 21:10:31Z BigMuscle $
 */
